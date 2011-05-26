#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "lua_descriptor.h"

#include <lauxlib.h>

#include "descriptor.h"
#include "log.h"
#include "lua_api.h"
#include "timeval.h"

#define FD_TYPE "mudcore.fd"

/* If the value at index is an integer and that integer is an active
   client FD, return its descriptor. Otherwise, return NULL. Signal a
   Lua error if the value at index is not an integer. */
static struct descriptor* lua_descriptor_get(lua_State* lua, gint index) {
  gint fd = *(gint*)luaL_checkudata(lua, index, FD_TYPE);
  return descriptor_get(fd);
}

static gint lua_descriptor_close(lua_State* lua) {
  struct descriptor* descriptor = lua_descriptor_get(lua, 1);
  if (descriptor != NULL) descriptor_drain(descriptor);
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
  if (lua_gettop(lua) == 1 && luaL_checknumber(lua, 1)) {
    lua_pushvalue(lua, -1);
    return lua_yield(lua, 1);
  }
  return lua_yield(lua, 0);
}

static gint lua_descriptor_send(lua_State* lua) {
  struct descriptor* descriptor = lua_descriptor_get(lua, 1);
  const gchar* str = luaL_checkstring(lua, 2);
  if (descriptor != NULL) descriptor_append(descriptor, str);
  return 0;
}

static gint lua_descriptor_send_prompt(lua_State* lua) {
  static gboolean warned = FALSE;
  struct descriptor* descriptor = lua_descriptor_get(lua, 1);
  if (descriptor != NULL) {
    if (!warned) {
      WARN("Using default mud.descriptor.send_prompt.");
      warned = TRUE;
    }
    descriptor_append(descriptor, "? ");
  }
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

  /* Put functions in the descriptor table. */
  lua_newtable(lua);
  static const luaL_Reg descriptor_funcs[] = {
    { "close"      , lua_descriptor_close       },
    { "on_open"    , lua_descriptor_on_open     },
    { "read"       , lua_descriptor_read        },
    { "send"       , lua_descriptor_send        },
    { "send_prompt", lua_descriptor_send_prompt },
    { "will_echo"  , lua_descriptor_will_echo   },
    { NULL         , NULL                       }
  };
  luaL_register(lua, NULL, descriptor_funcs);

  lua_setfield(lua, -2, "descriptor");
  lua_pop(lua, 1);

  luaL_newmetatable(lua, FD_TYPE);
  lua_pop(lua, 1);
}

void lua_descriptor_start(struct descriptor* descriptor) {
  lua_State* lua = lua_api_get();
  lua_State* thread = lua_newthread(lua);
  descriptor->thread_ref = luaL_ref(lua, LUA_REGISTRYINDEX);

  /* Call mud.descriptor.on_open in the new thread. */
  lua_getglobal(thread, "mud");
  lua_getfield(thread, -1, "descriptor");
  lua_remove(thread, -2);
  lua_getfield(thread, -1, "on_open");
  lua_remove(thread, -2);

  gint* fd = lua_newuserdata(thread, sizeof(*fd));
  *fd = descriptor->fd;
  luaL_getmetatable(thread, FD_TYPE);
  lua_setmetatable(thread, -2);
  lua_descriptor_resume(descriptor, NULL);
}

void lua_descriptor_resume(struct descriptor* descriptor,
                           const gchar* command) {
  lua_State* lua = lua_api_get();
  lua_rawgeti(lua, LUA_REGISTRYINDEX, descriptor->thread_ref);
  lua_State* thread;
  if (lua_isnil(lua, -1) || (thread = lua_tothread(lua, -1)) == NULL) {
    ERROR("Descriptor has lost its thread!");
    descriptor_close(descriptor);
  } else {
    if (command != NULL) lua_pushstring(thread, command);
    /* Descriptor threads are started with no string argument, but
       have the fd number as the only argument instead. Therefore,
       it's always safe to pass 1 to lua_resume. */
    switch (lua_resume(thread, 1)) {
    case 0: /* Terminated. */
      descriptor_drain(descriptor);
      return;
    case LUA_YIELD:
      if (lua_gettop(thread) == 1
          && lua_isnumber(thread, 1)) {
        lua_Number delay = lua_tonumber(thread, 1);
        if (delay > 0) {
          gettimeofday(&descriptor->next_command, NULL);
          timeval_add_delay(&descriptor->next_command, delay);
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
  lua_pop(lua, 1);
}
