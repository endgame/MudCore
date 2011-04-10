#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "lua_log.h"

#include <lauxlib.h>
#include <string.h>

#include "log.h"

static const gchar* log_levels[] = {
  "debug", "info", "warn", "error", "fatal", NULL
};

static int lua_log_call(lua_State* lua) {
  if (lua_gettop(lua) < 3) return 0;

  lua_remove(lua, 1);
  enum log_level level = luaL_checkoption(lua, 1, NULL, log_levels);
  log_printf(level, "%s", luaL_checkstring(lua, 2));
  return 0;
}

static int lua_log_index(lua_State* lua) {
  const gchar* key = luaL_checkstring(lua, 2);
  if (strcmp(key, "level") == 0) {
    lua_pushstring(lua, log_level_to_string(log_get_level()));
  } else {
    lua_pushnil(lua);
  }
  return 1;
}

static int lua_log_newindex(lua_State* lua) {
  const gchar* key = luaL_checkstring(lua, 2);
  if (strcmp(key, "level") == 0) {
    /* TODO: Make this not suck. Check type is string, and that option
       is valid. */
    enum log_level level = luaL_checkoption(lua, 3, NULL, log_levels);
    log_set_level(level);
  }
  return 0;
}

static int lua_log_debug(lua_State* lua) {
  const gchar* message = luaL_checkstring(lua, 1);
  DEBUG("%s", message);
  return 0;
}

static int lua_log_info(lua_State* lua) {
  const gchar* message = luaL_checkstring(lua, 1);
  INFO("%s", message);
  return 0;
}

static int lua_log_warn(lua_State* lua) {
  const gchar* message = luaL_checkstring(lua, 1);
  WARN("%s", message);
  return 0;
}

static int lua_log_error(lua_State* lua) {
  const gchar* message = luaL_checkstring(lua, 1);
  ERROR("%s", message);
  return 0;
}

static int lua_log_fatal(lua_State* lua) {
  const gchar* message = luaL_checkstring(lua, 1);
  FATAL("%s", message);
  return 0;
}

void lua_log_init(lua_State* lua) {
  DEBUG("Creating mud.log table.");
  lua_getglobal(lua, "mud");
  lua_newtable(lua);

  /* Put the convenience functions in the log table. */
  static const luaL_Reg log_funcs[] = {
    { "debug", lua_log_debug },
    { "info" , lua_log_info  },
    { "warn" , lua_log_warn  },
    { "error", lua_log_error },
    { "fatal", lua_log_fatal },
    { NULL   , NULL          }
  };

  luaL_register(lua, NULL, log_funcs);
  /* Build the log metatable. */
  static const luaL_Reg log_meta_funcs[] = {
    { "__call"    , lua_log_call     },
    { "__index"   , lua_log_index    },
    { "__newindex", lua_log_newindex },
    { NULL        , NULL             }
  };
  lua_newtable(lua);
  luaL_register(lua, NULL, log_meta_funcs);
  lua_setmetatable(lua, -2);
  lua_setfield(lua, -2, "log");
  lua_remove(lua, -1);
}
