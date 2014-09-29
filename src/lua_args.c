/* MudCore - a simple, lua-scripted MUD server
 * Copyright (C) 2011  Jack Kelly <jack@jackkelly.name>
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
#include "lua_args.h"

#include "arg_parse.h"
#include "log.h"

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
  DEBUG("Creating mudcore.args table.");
  lua_getglobal(lua, "mudcore");
  lua_newtable(lua);
  struct arg_parse_funcs funcs = {
    .on_flag       = lua_args_on_flag,
    .on_option     = lua_args_on_option,
    .on_positional = lua_args_on_positional
  };
  arg_parse(argc, argv, &funcs, lua);
  lua_setfield(lua, -2, "args");
  lua_pop(lua, 1);
}
