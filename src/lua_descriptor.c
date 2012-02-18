/* MudCore - a simple, lua-scripted MUD server
 * Copyright (C) 2011, 2012  Jack Kelly <jack@jackkelly.name>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "lua_descriptor.h"

#include <lauxlib.h>
#include <string.h>

#include "descriptor.h"
#include "log.h"
#include "lua_api.h"
#include "timeval.h"

#define DESCRIPTOR_TYPE "mudcore.descriptor"

/* If the value at index is a userdatum that represents a descriptor,
   return that descriptor. Otherwise, return NULL. Signal a Lua error
   if the value at index is not of the correct type. */
static struct descriptor* lua_descriptor_get(lua_State* lua, gint index) {
  gint fd = *(gint*)luaL_checkudata(lua, index, DESCRIPTOR_TYPE);
  return descriptor_get(fd);
}

static gint lua_descriptor_close(lua_State* lua) {
  struct descriptor* descriptor = lua_descriptor_get(lua, 1);
  if (descriptor != NULL) descriptor_drain(descriptor);
  return 0;
}

static gint lua_descriptor_extra_data(lua_State* lua) {
  struct descriptor* descriptor = lua_descriptor_get(lua, 1);
  if (descriptor != NULL) {
    lua_rawgeti(lua, LUA_REGISTRYINDEX, descriptor->extra_data_ref);
  } else {
    lua_pushnil(lua);
  }
  return 1;
}

static gint lua_descriptor_index(lua_State* lua) {
  struct descriptor* descriptor = lua_descriptor_get(lua, 1);
  if (descriptor != NULL) {
    lua_rawgeti(lua, LUA_REGISTRYINDEX, descriptor->extra_data_ref);
    lua_pushvalue(lua, 2);
    lua_gettable(lua, -2);
    if (lua_isnil(lua, -1) && lua_getmetatable(lua, 1) != 0) {
      lua_pushvalue(lua, 2);
      lua_rawget(lua, -2);
    }
  } else {
    lua_pushnil(lua);
  }
  return 1;
}

static gint lua_descriptor_newindex(lua_State* lua) {
  struct descriptor* descriptor = lua_descriptor_get(lua, 1);
  if (descriptor != NULL) {
    lua_rawgeti(lua, LUA_REGISTRYINDEX, descriptor->extra_data_ref);
    lua_pushvalue(lua, 2);
    lua_pushvalue(lua, 3);
    lua_settable(lua, -3);
  }
  return 0;
}

static gint lua_descriptor_delay(lua_State* lua) {
  lua_Number delay = luaL_checknumber(lua, 1);
  if (delay <= 0) {
    WARN("Argument to mud.descriptor.delay should be > 0.");
    return 0;
  } else {
    lua_pushvalue(lua, 1);
    return lua_yield(lua, 1);
  }
}

static gint lua_descriptor_on_open(lua_State* lua) {
  struct descriptor* descriptor = lua_descriptor_get(lua, 1);
  if (descriptor != NULL) {
    descriptor_append(descriptor,
                      "You need to redefine mud.descriptor.on_open().\r\n");
    descriptor_drain(descriptor);
  }
  return 0;
}

static gint lua_descriptor_read(lua_State* lua) {
  return lua_yield(lua, 0);
}

static gint lua_descriptor_send(lua_State* lua) {
  struct descriptor* descriptor = lua_descriptor_get(lua, 1);
  const gchar* str = luaL_checkstring(lua, 2);
  if (descriptor != NULL) descriptor_append(descriptor, str);
  return 0;
}

static gint lua_descriptor_will_echo(lua_State* lua) {
  gboolean will = TRUE;
  struct descriptor* descriptor = lua_descriptor_get(lua, 1);
  if (lua_gettop(lua) == 2) will = lua_toboolean(lua, 2);
  descriptor_will_echo(descriptor, will);
  return 0;
}

void lua_descriptor_init(lua_State* lua) {
  DEBUG("Creating mud.descriptor table.");
  lua_getglobal(lua, "mud");

  lua_newtable(lua);
  static const luaL_Reg descriptor_funcs[] = {
    { "delay"      , lua_descriptor_delay       },
    { "on_open"    , lua_descriptor_on_open     },
    { "read"       , lua_descriptor_read        },
    { NULL         , NULL                       }
  };
  luaL_register(lua, NULL, descriptor_funcs);
  lua_setfield(lua, -2, "descriptor");
  lua_pop(lua, 1);

  luaL_newmetatable(lua, DESCRIPTOR_TYPE);
  static const luaL_Reg descriptor_methods[] = {
    { "__index"   , lua_descriptor_index      },
    { "__newindex", lua_descriptor_newindex   },
    { "close"     , lua_descriptor_close      },
    { "extra_data", lua_descriptor_extra_data },
    { "send"      , lua_descriptor_send       },
    { "will_echo" , lua_descriptor_will_echo  },
  };
  luaL_register(lua, NULL, descriptor_methods);
  lua_pop(lua, 1);
}

/* Wake up the lua thread corresponding to descriptor. nargs is the
   number of arguments to return from the wrapped call to yield. */
static void lua_descriptor_resume(struct descriptor* descriptor,
                                  lua_State* thread,
                                  gint nargs) {
  switch (lua_resume(thread, nargs)) {
  case 0: /* Terminated. */
    descriptor_drain(descriptor);
    return;
  case LUA_YIELD:
    if (lua_gettop(thread) == 1 && lua_isnumber(thread, 1)) {
      lua_Number delay = lua_tonumber(thread, 1);
      if (delay > 0) {
        descriptor->state = DESCRIPTOR_STATE_DELAYING;
        gettimeofday(&descriptor->delay_end, NULL);
        timeval_add_delay(&descriptor->delay_end, delay);
      }
    }
    lua_settop(thread, 0);
    break;
  default: /* Error. */
    ERROR("Error in lua code: %s", lua_tostring(thread, -1));
    descriptor_drain(descriptor);
    break;
  }
}

void lua_descriptor_start(struct descriptor* descriptor) {
  lua_State* lua = lua_api_get();
  lua_State* thread = lua_newthread(lua);
  descriptor->thread_ref = luaL_ref(lua, LUA_REGISTRYINDEX);

  lua_newtable(lua);
  lua_pushliteral(lua, "? ");
  lua_setfield(lua, -2, "prompt");
  descriptor->extra_data_ref = luaL_ref(lua, LUA_REGISTRYINDEX);

  /* Call mud.descriptor.on_open in the new thread. */
  lua_getglobal(thread, "mud");
  lua_getfield(thread, -1, "descriptor");
  lua_remove(thread, -2);
  lua_getfield(thread, -1, "on_open");
  lua_remove(thread, -2);

  gint* fd = lua_newuserdata(thread, sizeof(*fd));
  *fd = descriptor->fd;
  luaL_getmetatable(thread, DESCRIPTOR_TYPE);
  lua_setmetatable(thread, -2);
  lua_pushvalue(thread, -1);
  descriptor->fd_ref = luaL_ref(thread, LUA_REGISTRYINDEX);
  lua_descriptor_resume(descriptor, thread, 1);
}

void lua_descriptor_command(struct descriptor* descriptor,
                            const gchar* command) {
  lua_State* lua = lua_api_get();
  lua_rawgeti(lua, LUA_REGISTRYINDEX, descriptor->thread_ref);
  lua_State* thread;
  if (lua_isnil(lua, -1) || (thread = lua_tothread(lua, -1)) == NULL) {
    ERROR("Descriptor has lost its thread!");
    descriptor_close(descriptor);
  } else {
    lua_pushstring(thread, command);
    lua_descriptor_resume(descriptor, thread, 1);
  }
  lua_pop(lua, 1);
}

void lua_descriptor_continue(struct descriptor* descriptor) {
  lua_State* lua = lua_api_get();
  lua_rawgeti(lua, LUA_REGISTRYINDEX, descriptor->thread_ref);
  lua_State* thread;
  if (lua_isnil(lua, -1) || (thread = lua_tothread(lua, -1)) == NULL) {
    ERROR("Descriptor has lost its thread!");
    descriptor_close(descriptor);
  } else {
    lua_descriptor_resume(descriptor, thread, 0);
  }
  lua_pop(lua, 1);
}
