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
  gpointer zmq_context = zmq_init(1);
  if (zmq_context == NULL) {
    PERROR("main(zmq_init)");
    error = 1;
    goto err0;
  }

  DEBUG("Creating ZMQ_REP socket.");
  gpointer zmq_rep_socket = zmq_socket(zmq_context, ZMQ_REP);
  if (zmq_rep_socket == NULL) {
    PERROR("main(zmq_socket(ZMQ_REP))");
    error = 1;
    goto err1;
  }

  if (zmq_bind(zmq_rep_socket, options_zmq_rep_endpoint()) == -1) {
    PERROR("main(zmq_bind(ZMQ_REP))");
    error = 1;
    goto err2;
  }

  DEBUG("Creating ZMQ_PUB socket.");
  gpointer zmq_pub_socket = zmq_socket(zmq_context, ZMQ_PUB);
  if (zmq_pub_socket == NULL) {
    PERROR("main(zmq_socket(ZMQ_PUB))");
    error = 1;
    goto err2;
  }

  if (zmq_bind(zmq_pub_socket, options_zmq_pub_endpoint()) == -1) {
    PERROR("main(zmq_bind(ZMQ_PUB))");
    error = 1;
    goto err3;
  }

  DEBUG("Creating server on port %s.", options_port());
  gint socket = socket_server(options_port());
  if (socket == -1) {
    error = 1;
    goto err4;
  }

  INFO("Initialising Lua API.");
  lua_api_init(zmq_pub_socket, argc - 1, argv + 1);
  lua_State* lua = lua_api_get();
  DEBUG("Running " LUA_START_FILE);
  if (luaL_dofile(lua, LUA_START_FILE) == 1) {
    ERROR("%s", lua_tostring(lua, -1));
    error = 1;
    goto err4;
  }

  io_mainloop(socket, zmq_rep_socket);

 err4:
  DEBUG("Closing server socket.");
  socket_close(socket);
 err3:
  DEBUG("Closing ZMQ_PUB socket.");
  zmq_close(zmq_pub_socket);
 err2:
  DEBUG("Closing ZMQ_REP socket.");
  zmq_close(zmq_rep_socket);
 err1:
  DEBUG("Terminating ZeroMQ context.");
  zmq_term(zmq_context);
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
