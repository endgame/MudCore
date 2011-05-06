#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "lua_descriptor.h"

#include <lauxlib.h>

#include "descriptor.h"
#include "log.h"
#include "lua_api.h"

/* If the value at index is an integer and that integer is an active
   client FD, return its descriptor. Otherwise, return NULL. Signal a
   Lua error if the value at index is not an integer. */
static struct descriptor* lua_descriptor_get(lua_State* lua, gint index) {
  gint fd = luaL_checkint(lua, index);
  return descriptor_get(fd);
}

static int lua_descriptor_close(lua_State* lua) {
  struct descriptor* descriptor = lua_descriptor_get(lua, 1);
  if (descriptor != NULL) descriptor_close(descriptor);
  return 0;
}

static int lua_descriptor_drain(lua_State* lua) {
  struct descriptor* descriptor = lua_descriptor_get(lua, 1);
  if (descriptor != NULL) descriptor_drain(descriptor);
  return 0;
}

static int lua_descriptor_on_open(lua_State* lua) {
  struct descriptor* descriptor = lua_descriptor_get(lua, 1);
  if (descriptor != NULL) {
    descriptor_append(descriptor,
                      "You need to redefine mud.descriptor.on_open().\r\n");
    descriptor_drain(descriptor);
  }
  return 0;
}

static int lua_descriptor_read(lua_State* lua) {
  // TODO: add input delay support.
  return lua_yield(lua, 0);
}

static int lua_descriptor_send(lua_State* lua) {
  struct descriptor* descriptor = lua_descriptor_get(lua, 1);
  const gchar* str = luaL_checkstring(lua, 2);
  descriptor_append(descriptor, str);
  return 0;
}

void lua_descriptor_init(lua_State* lua) {
  DEBUG("Creating mud.descriptor table.");
  lua_getglobal(lua, "mud");

  /* Put functions in the descriptor table. */
  lua_newtable(lua);
  static const luaL_Reg descriptor_funcs[] = {
    { "close"  , lua_descriptor_close   },
    { "drain"  , lua_descriptor_drain   },
    { "on_open", lua_descriptor_on_open },
    { "read"   , lua_descriptor_read    },
    { "send"   , lua_descriptor_send    },
    { NULL     , NULL                   }
  };
  luaL_register(lua, NULL, descriptor_funcs);

  lua_setfield(lua, -2, "descriptor");
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
  lua_pushinteger(thread, descriptor->fd);
  switch (lua_resume(thread, 1)) {
  case 0:
    descriptor_drain(descriptor);
    break;
  default:
    FATAL("wha?");
  }
}
