#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "lua_zmq.h"

#include <lauxlib.h>
#include <zmq.h>

#include "log.h"

static gint lua_zmq_version(lua_State* lua) {
  gint major;
  gint minor;
  gint patch;
  zmq_version(&major, &minor, &patch);
  lua_createtable(lua, 3, 0);
  lua_pushinteger(lua, major);
  lua_rawseti(lua, -2, 1);
  lua_pushinteger(lua, minor);
  lua_rawseti(lua, -2, 2);
  lua_pushinteger(lua, patch);
  lua_rawseti(lua, -2, 3);
  return 1;
}

void lua_zmq_init(lua_State* lua, gpointer zmq_context) {
  // TODO finish
  DEBUG("Creating mud.zmq table.");
  lua_getglobal(lua, "mud");

  /* Put functions in the zmq table. */
  lua_newtable(lua);
  static const luaL_Reg zmq_funcs[] = {
    { "version", lua_zmq_version },
    { NULL     , NULL            }
  };
  luaL_register(lua, NULL, zmq_funcs);
  lua_setfield(lua, -2, "zmq");
  lua_pop(lua, 1);
}
