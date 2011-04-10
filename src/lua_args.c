#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "lua_args.h"

#include "arg_parse.h"

static void lua_args_on_flag(const gchar* flagname,
                             gboolean value,
                             gpointer user_data) {
  lua_State* lua = user_data;
  lua_pushboolean(lua, value);
  lua_setfield(lua, -2, flagname);
}

static void lua_args_on_option(const gchar* name,
                               const gchar* value,
                               gpointer user_data) {
  lua_State* lua = user_data;
  lua_pushstring(lua, value);
  lua_setfield(lua, -2, name);
}

static void lua_args_on_positional(gint argc,
                                   gchar* argv[],
                                   gpointer user_data) {
  lua_State* lua = user_data;
  for (int i = 0; i < argc; i++) {
    lua_pushstring(lua, argv[i]);
    lua_rawseti(lua, -2, i + 1); /* lua tables are 1-indexed. */
  }
}

void lua_args_init(lua_State* lua, gint argc, gchar* argv[]) {
  lua_getglobal(lua, "mud");
  lua_newtable(lua);
  struct arg_parse_funcs funcs = {
    .on_flag       = lua_args_on_flag,
    .on_option     = lua_args_on_option,
    .on_positional = lua_args_on_positional
  };
  arg_parse(argc, argv, &funcs, lua);
  lua_setfield(lua, -2, "args");
  lua_remove(lua, -1);
}
