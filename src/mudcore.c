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

#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <zmq.h>

#include "descriptor.h"
#include "io.h"
#include "log.h"
#include "lua_api.h"
#include "lua_zmq.h"
#include "options.h"
#include "socket.h"

#define LUA_START_FILE "./boot.lua"

int main(int argc, char* argv[]) {
  gint error = 0;
  options_init(argc - 1, argv + 1);

  INFO("Starting up.");
  descriptor_init();

  DEBUG("Disabling SIGPIPE.");
  signal(SIGPIPE, SIG_IGN);

  DEBUG("Creating ZeroMQ context.");
  gpointer zmq_context = zmq_ctx_new();
  if (zmq_context == NULL) {
    PERROR("main(zmq_init)");
    error = 1;
    goto err0;
  }

  DEBUG("Creating server on port %s.", options_port());
  gint socket = socket_server(options_port());
  if (socket == -1) {
    error = 1;
    goto err1;
  }

  INFO("Initialising Lua API.");
  lua_api_init(zmq_context, argc - 1, argv + 1);
  lua_State* lua = lua_api_get();
  DEBUG("Running " LUA_START_FILE);
  if (luaL_dofile(lua, LUA_START_FILE) == 1) {
    ERROR("%s", lua_tostring(lua, -1));
    error = 1;
    goto err2;
  }

  io_mainloop(socket);

 err2:
  DEBUG("Closing server socket.");
  socket_close(socket);
 err1:
  DEBUG("Terminating ZeroMQ context.");
  /* This is separate from lua_api_deinit() to prevent zmq_ctx_term()
     from blocking forever. */
  lua_zmq_deinit();
  zmq_ctx_term(zmq_context);
 err0:
  DEBUG("Enabling SIGPIPE.");
  signal(SIGPIPE, SIG_DFL);
  options_deinit();
  descriptor_deinit();

  if (lua_api_get() != NULL) {
    DEBUG("Closing Lua state.");
    lua_api_deinit();
  }
  return error;
}
