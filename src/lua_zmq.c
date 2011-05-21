#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "lua_zmq.h"

#include <glib.h>
#include <lauxlib.h>
#include <string.h>
#include <zmq.h>

#include "log.h"
#include "lua_api.h"

static gpointer pub_socket = NULL;

static void zmq_g_free(gpointer data, gpointer hint) {
  (void) hint; /* Not used. */
  g_free(data);
}

static gchar* copy_lua_string(lua_State* lua, gint index, gsize* length) {
  const gchar* str = lua_tolstring(lua, index, length);
  return memcpy(g_new(gchar, *length), str, *length);
}

static int lua_zmq_publish(lua_State* lua) {
  luaL_checkstring(lua, 1);
  gsize msg_length;
  gchar* msg_data = copy_lua_string(lua, 1, &msg_length);

  zmq_msg_t msg;
  zmq_msg_init_data(&msg, msg_data, msg_length, zmq_g_free, NULL);

  while (zmq_send(pub_socket, &msg, 0) == -1) {
    if (errno == EINTR) continue;
    PERROR("lua_zmq_publish(zmq_send)");
    zmq_msg_close(&msg);
    break;
  }

  return 0;
}

void lua_zmq_init(lua_State* lua, gpointer zmq_pub_socket) {
  pub_socket = zmq_pub_socket;

  DEBUG("Creating mud.zmq table.");
  lua_getglobal(lua, "mud");
  lua_newtable(lua);
  static const luaL_Reg zmq_funcs[] = {
    { "publish", lua_zmq_publish },
    { NULL     , NULL            }
  };
  luaL_register(lua, NULL, zmq_funcs);
  lua_setfield(lua, -2, "zmq");
  lua_pop(lua, 1);
}

/* Send a 0-byte response on a zmq socket. */
static void send_empty_response(gpointer socket) {
  zmq_msg_t msg;
  zmq_msg_init_size(&msg, 0);
  while (zmq_send(socket, &msg, 0) == -1) {
    if (errno == EINTR) continue;
    PERROR("send_empty_response(zmq_send)");
    zmq_msg_close(&msg);
  }
}

void lua_zmq_on_request(gpointer socket, const gchar* message, gint length) {
  static gboolean warned = FALSE;
  lua_State* lua = lua_api_get();
  lua_getfield(lua, LUA_GLOBALSINDEX, "mud");
  lua_getfield(lua, -1, "zmq");
  lua_remove(lua, -2);
  lua_getfield(lua, -1, "on_request");
  lua_remove(lua, -2);

  if (lua_isnil(lua, -1)) { /* mud.zmq.on_request not defined */
    if (!warned) {
      warned = TRUE;
      WARN("mud.zmq.on_request not defined. Empty responses are being sent!");
    }
    lua_pop(lua, 1);
    send_empty_response(socket);
    return;
  }

  lua_pushlstring(lua, message, length);

  if (lua_pcall(lua, 1, 1, 0) != 0) {
    const gchar* what = lua_tostring(lua, -1);
    ERROR("Error in mud.zmq.on_request: %s", what);
    lua_pop(lua, 1); /* The error string. */
    send_empty_response(socket);
    return;
  }

  gsize msg_length;
  gchar* msg_data = copy_lua_string(lua, -1, &msg_length);
  lua_pop(lua, 1); /* The result string. */

  zmq_msg_t msg;
  zmq_msg_init_data(&msg, msg_data, msg_length, zmq_g_free, NULL);
  while (zmq_send(socket, &msg, 0) == -1) {
    if (errno == EINTR) continue;
    PERROR("lua_zmq_on_request(zmq_send)");
    zmq_msg_close(&msg);
  }
}
