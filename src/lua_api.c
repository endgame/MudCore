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
#include "lua_api.h"

#include <glib.h>
#include <lauxlib.h>
#include <lualib.h>

#include "io.h"
#include "log.h"
#include "lua_args.h"
#include "lua_descriptor.h"
#include "lua_log.h"
#include "lua_timer.h"
#include "lua_zmq.h"

static lua_State* lua = NULL;

/* Allocation function for lua using GLib's memory management. */
static gpointer lua_glib_alloc(gpointer ud,
                               gpointer ptr,
                               gsize osize,
                               gsize nsize) {
  (void) ud; /* Not used. */
  (void) osize; /* Not used. */
  if (nsize == 0) {
    g_free(ptr);
    return NULL;
  } else {
    return g_realloc(ptr, nsize);
  }
}

static gint lua_shutdown(lua_State* lua) {
  (void) lua; /* Not used. */
  io_shutdown();
  return 0;
}

void lua_api_init(gpointer zmq_context, gint argc, gchar* argv[]) {
  lua = lua_newstate(lua_glib_alloc, NULL);
  luaL_openlibs(lua);
  lua_newtable(lua);
  static const luaL_Reg funcs[] = {
    { "shutdown", lua_shutdown },
    { NULL      , NULL         }
  };
  luaL_setfuncs(lua, funcs, 0);
  lua_setglobal(lua, "mud");
  lua_args_init(lua, argc, argv);
  lua_descriptor_init(lua);
  lua_log_init(lua);
  lua_timer_init(lua);
  lua_zmq_init(lua, zmq_context);
}

void lua_api_deinit(void) {
  lua_timer_deinit();
  lua_close(lua);
  lua = NULL;
}

lua_State* lua_api_get(void) {
  return lua;
}
