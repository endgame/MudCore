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

#include "buffer.h"
#include "descriptor.h"
#include "log.h"
#include "lua_api.h"
#include "timeval.h"

#define COMMAND_QUEUE_FIELD "command_queue"
#define COMMAND_QUEUE_SIZE 10
#define DESCRIPTOR_TYPE "mudcore.descriptor"

/* If the value at index is a userdatum that represents a descriptor,
   return that descriptor. Otherwise, return NULL. Signal a Lua error
   if the value at index is not of the correct type. */
static struct descriptor* lua_descriptor_get(lua_State* lua, gint index) {
  gint fd = *(gint*)luaL_checkudata(lua, index, DESCRIPTOR_TYPE);
  return descriptor_get(fd);
}

/* Return TRUE iff the descriptor's thread is the given lua state. */
static gboolean descriptor_is_active(struct descriptor* descriptor,
                                     lua_State* lua) {
  if (descriptor == NULL || lua == NULL) return FALSE;
  lua_rawgeti(lua, LUA_REGISTRYINDEX, descriptor->thread_ref);
  lua_State* thread = lua_tothread(lua, -1);
  gboolean rv = lua == thread;
  lua_pop(lua, 1);
  return rv;
}

static gint lua_descriptor_close(lua_State* lua) {
  struct descriptor* descriptor = lua_descriptor_get(lua, 1);
  if (descriptor != NULL) descriptor_drain(descriptor);
  if (descriptor_is_active(descriptor, lua)) return lua_yield(lua, 0);
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
    lua_remove(lua, -2);
    if (lua_isnil(lua, -1) && lua_getmetatable(lua, 1) != 0) {
      lua_remove(lua, -2);
      lua_pushvalue(lua, 2);
      lua_rawget(lua, -2);
    }
  } else {
    lua_pushnil(lua);
  }
  return 1;
}

static gint lua_descriptor_is_active(lua_State* lua) {
  struct descriptor* descriptor = lua_descriptor_get(lua, 1);
  lua_pushboolean(lua, descriptor_is_active(descriptor, lua));
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
  struct descriptor* descriptor = lua_descriptor_get(lua, 1);
  lua_Number delay = luaL_checknumber(lua, 2);
  descriptor_delay(descriptor, delay);
  if (descriptor_is_active(descriptor, lua)) {
    descriptor->self_delayed = TRUE;
    return lua_yield(lua, 0);
  } else {
    return 0;
  }
}

static gint lua_descriptor_on_command(lua_State* lua) {
  struct descriptor* descriptor = lua_descriptor_get(lua, 1);

  /* We aim to call table.insert(descriptor->command_queue, command) */
  lua_getglobal(lua, "table");
  lua_getfield(lua, -1, "insert");
  lua_remove(lua, -2);
  lua_rawgeti(lua, LUA_REGISTRYINDEX, descriptor->extra_data_ref);
  lua_getfield(lua, -1, COMMAND_QUEUE_FIELD);
  lua_remove(lua, -2);

  /* Check if the command queue is full. */
  lua_len(lua, -1);
  gint len = lua_tonumber(lua, -1);
  lua_pop(lua, 1);

  if (len >= COMMAND_QUEUE_SIZE) {
    descriptor_append(descriptor,
                      "Input queue full. Command discarded.\r\n");
    lua_pop(lua, 2); /* table.insert, descriptor->command_queue */
  } else {
    lua_pushvalue(lua, 2);
    lua_call(lua, 2, 0);
  }

  return 0;
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
  struct descriptor* descriptor = lua_descriptor_get(lua, 1);
  if (!descriptor_is_active(descriptor, lua)) {
    return luaL_error(lua, "Attempting to read another thread's descriptor.");
  }
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
  luaL_newmetatable(lua, DESCRIPTOR_TYPE);
  static const luaL_Reg descriptor_methods[] = {
    { "__index"   , lua_descriptor_index      },
    { "__newindex", lua_descriptor_newindex   },
    { "close"     , lua_descriptor_close      },
    { "delay"     , lua_descriptor_delay      },
    { "extra_data", lua_descriptor_extra_data },
    { "is_active" , lua_descriptor_is_active  },
    { "on_command", lua_descriptor_on_command },
    { "on_open"   , lua_descriptor_on_open    },
    { "read"      , lua_descriptor_read       },
    { "send"      , lua_descriptor_send       },
    { "will_echo" , lua_descriptor_will_echo  },
    { NULL        , NULL                      }
  };
  luaL_setfuncs(lua, descriptor_methods, 0);
  lua_setfield(lua, -2, "descriptor");
  lua_pop(lua, 1);
}

void lua_descriptor_resume(struct descriptor* descriptor, gint nargs) {
  lua_State* lua = lua_api_get();
  lua_rawgeti(lua, LUA_REGISTRYINDEX, descriptor->thread_ref);
  lua_State* thread;

  if (lua_isnil(lua, -1) || (thread = lua_tothread(lua, -1)) == NULL) {
    ERROR("Descriptor has lost its thread!");
    descriptor_close(descriptor);
  } else {
    switch (lua_resume(thread, NULL, nargs)) {
    case LUA_OK:
      descriptor_drain(descriptor);
      return;
    case LUA_YIELD:
      lua_settop(thread, 0);
      break;
    default: /* Error. */
      ERROR("Error in lua code: %s", lua_tostring(thread, -1));
      descriptor_drain(descriptor);
      break;
    }
  }

  lua_pop(lua, 1);
}

void lua_descriptor_start(struct descriptor* descriptor) {
  lua_State* lua = lua_api_get();
  lua_State* thread = lua_newthread(lua);
  descriptor->thread_ref = luaL_ref(lua, LUA_REGISTRYINDEX);

  lua_newtable(lua);
  lua_pushliteral(lua, "? ");
  lua_setfield(lua, -2, "prompt");

  lua_newtable(lua);
  lua_setfield(lua, -2, "command_queue");
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
  lua_descriptor_resume(descriptor, 1);
}

void lua_descriptor_accept_command(struct descriptor* descriptor) {
  lua_State* lua = lua_api_get();

  /* Call mud.descriptor.on_command(descriptor, contents-of-buffer). */
  lua_getglobal(lua, "mud");
  lua_getfield(lua, -1, "descriptor");
  lua_remove(lua, -2);
  lua_getfield(lua, -1, "on_command");
  lua_remove(lua, -2);
  lua_rawgeti(lua, LUA_REGISTRYINDEX, descriptor->fd_ref);
  lua_pushlstring(lua,
                  descriptor->line_buffer->data,
                  descriptor->line_buffer->used);
  buffer_clear(descriptor->line_buffer);

  if (lua_pcall(lua, 2, 0, 0) != LUA_OK) {
    const gchar* what = lua_tostring(lua, -1);
    ERROR("Error in mud.descriptor.on_command: %s", what);
    lua_pop(lua, 1);
    descriptor_close(descriptor);
  }
}

void lua_descriptor_handle_command(struct descriptor* descriptor) {
  lua_State* lua = lua_api_get();

  lua_rawgeti(lua, LUA_REGISTRYINDEX, descriptor->thread_ref);
  lua_State* thread;
  if (lua_isnil(lua, -1) || (thread = lua_tothread(lua, -1)) == NULL) {
    ERROR("Descriptor has lost its thread!");
    descriptor_close(descriptor);
    lua_pop(lua, 1);
    return;
  }

  /* We aim to call table.remove(descriptor->command_queue, 1) */
  lua_getglobal(lua, "table");
  lua_getfield(lua, -1, "remove");
  lua_remove(lua, -2);
  lua_rawgeti(lua, LUA_REGISTRYINDEX, descriptor->extra_data_ref);
  lua_getfield(lua, -1, COMMAND_QUEUE_FIELD);
  lua_remove(lua, -2);

  /* Check if there's actually a command to process. */
  lua_len(lua, -1);
  gint len = lua_tonumber(lua, -1);
  lua_pop(lua, 1);

  if (len == 0) {
    lua_pop(lua, 3); /* thread, table.remove, descriptor->command_queue */
    return;
  }

  descriptor->needs_prompt = TRUE;

  /* Get the command. */
  lua_pushnumber(lua, 1);
  lua_call(lua, 2, 1);

  lua_xmove(lua, thread, 1);
  lua_descriptor_resume(descriptor, 1);
  lua_pop(lua, 1);
}
