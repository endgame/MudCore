#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "lua_zmq.h"

#include <lauxlib.h>
#include <string.h>
#include <zmq.h>

#include "log.h"
#include "lua_api.h"

#define SOCKET_TYPE "mudcore.zmq_socket"

struct lua_zmq_socket {
  gpointer socket;
  gint in_watcher; /* Ref from luaL_ref. */
  gint out_watcher; /* Ref from luaL_ref. */
};

static gpointer context;
static GHashTable* /* of gpointer (zmq socket) ->
                      struct lua_zmq_socket* */ sockets;

/* Iterate through the socket table. Iter names the iterator, Value
   the struct lua_zmq_socket pointer. */
#define SOCKET_FOREACH(Iter, Value)                                     \
  GHashTableIter Iter;                                                  \
  g_hash_table_iter_init(&iter, sockets);                               \
  struct lua_zmq_socket* Value;                                         \
  while (g_hash_table_iter_next(&Iter, NULL, (gpointer*)&Value))

/* Callback for ZeroMQ to free data. */
static void zmq_g_free(gpointer data, gpointer hint) {
  (void) hint; /* Not used. */
  g_free(data);
}

/* GLib's callback to destroy a socket when it's removed from the hash
   table. */
static void zmq_socket_destroy(gpointer s) {
  struct lua_zmq_socket* socket = s;
  lua_State* lua = lua_api_get();
  if (zmq_close(socket->socket) == -1) PWARN("zmq_socket_destroy(zmq_close)");
  luaL_unref(lua, LUA_REGISTRYINDEX, socket->in_watcher);
  luaL_unref(lua, LUA_REGISTRYINDEX, socket->out_watcher);
}

/* Raise an error with message MSG..": "..zmq_strerror(errno). */
static gint lua_zmq_error(lua_State* lua, const gchar* msg) {
  return luaL_error(lua, "%s: %s", msg, zmq_strerror(errno));
}

static gint lua_zmq_bind(lua_State* lua) {
  struct lua_zmq_socket* socket = luaL_checkudata(lua, 1, SOCKET_TYPE);
  const gchar* endpoint = luaL_checkstring(lua, 2);
  if (zmq_bind(socket->socket, endpoint) == -1) {
    return lua_zmq_error(lua, "zmq_bind");
  }
  return 0;
}

static gint lua_zmq_close(lua_State* lua) {
  /* Even though this might fire twice (socket:close(), followed by
     __gc()), it's OK because the destructor code is executed once
     (when it's removed by the hashtable's DestroyNotify callback). */
  struct lua_zmq_socket* socket = luaL_checkudata(lua, 1, SOCKET_TYPE);
  g_hash_table_remove(sockets, socket->socket);
  return 0;
}

static gint lua_zmq_connect(lua_State* lua) {
  struct lua_zmq_socket* socket = luaL_checkudata(lua, 1, SOCKET_TYPE);
  const gchar* endpoint = luaL_checkstring(lua, 2);
  if (zmq_connect(socket->socket, endpoint) == -1) {
    return lua_zmq_error(lua, "zmq_connect");
  }
  return 0;
}

static gint lua_zmq_getopt(lua_State* lua) {
  struct lua_zmq_socket* socket = luaL_checkudata(lua, 1, SOCKET_TYPE);
  gint option = luaL_checkint(lua, 2);
  union {
    gchar s[255]; /* ZMQ_IDENTITY max is 255, see man zmq_getsockopt(). */
    gint i;
    gint32 i32;
    gint64 i64;
    guint64 u64;
  } buf;
  gsize buf_size = sizeof(buf);
  if (zmq_getsockopt(socket->socket, option, &buf, &buf_size) == -1) {
    return lua_zmq_error(lua, "zmq_getsockopt");
  }

  switch (option) {
  case ZMQ_BACKLOG:
  case ZMQ_FD:
  case ZMQ_LINGER:
  case ZMQ_RECONNECT_IVL:
  case ZMQ_RECONNECT_IVL_MAX:
  case ZMQ_TYPE:
    lua_pushinteger(lua, buf.i);
    return 1;
  case ZMQ_MCAST_LOOP:
  case ZMQ_RATE:
  case ZMQ_RECOVERY_IVL:
  case ZMQ_RECOVERY_IVL_MSEC:
  case ZMQ_RCVMORE:
  case ZMQ_SWAP:
    lua_pushinteger(lua, buf.i64);
    return 1;
  case ZMQ_AFFINITY:
  case ZMQ_HWM:
  case ZMQ_RCVBUF:
  case ZMQ_SNDBUF:
    lua_pushinteger(lua, buf.u64);
    return 1;
  case ZMQ_IDENTITY:
    lua_pushlstring(lua, buf.s, buf_size);
    return 1;
  case ZMQ_EVENTS:
    lua_pushinteger(lua, buf.i32);
    return 1;
  }
  return 0;
}

static gint lua_zmq_recv(lua_State* lua) {
  struct lua_zmq_socket* socket = luaL_checkudata(lua, 1, SOCKET_TYPE);
  gint flags = luaL_optint(lua, 2, 0);
  zmq_msg_t msg;
  zmq_msg_init(&msg);
  if (zmq_recv(socket->socket, &msg, flags) == -1) {
    zmq_msg_close(&msg);
    return lua_zmq_error(lua, "zmq_recv");
  }
  lua_pushlstring(lua, zmq_msg_data(&msg), zmq_msg_size(&msg));
  zmq_msg_close(&msg);
  return 1;
}

static gint lua_zmq_send(lua_State* lua) {
  gsize len;
  struct lua_zmq_socket* socket = luaL_checkudata(lua, 1, SOCKET_TYPE);
  const gchar* str = luaL_checklstring(lua, 2, &len);
  gint flags = luaL_optint(lua, 3, 0);
  zmq_msg_t msg;
  zmq_msg_init_data(&msg,
                    memcpy(g_new(gchar, len), str, len),
                    len,
                    zmq_g_free,
                    NULL);
  if (zmq_send(socket->socket, &msg, flags) == -1) {
    zmq_msg_close(&msg);
    return lua_zmq_error(lua, "zmq_send");
  }
  return 0;
}

static gint lua_zmq_socket(lua_State* lua) {
  gint type = luaL_checkint(lua, 1);
  gpointer socket = zmq_socket(context, type);
  if (socket == NULL) {
    return lua_zmq_error(lua, "zmq_socket");
  }
  struct lua_zmq_socket* result = lua_newuserdata(lua, sizeof(*result));
  luaL_getmetatable(lua, SOCKET_TYPE);
  lua_setmetatable(lua, -2);
  result->socket = socket;
  result->in_watcher = LUA_NOREF;
  result->out_watcher = LUA_NOREF;

  g_hash_table_insert(sockets, socket, result);
  return 1;
}

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

#define DECLARE_CONST(C)                        \
  G_STMT_START {                                \
    lua_pushinteger(lua, ZMQ_##C);              \
    lua_setfield(lua, -2, G_STRINGIFY(C));      \
  } G_STMT_END

void lua_zmq_init(lua_State* lua, gpointer zmq_context) {
  // TODO finish
  context = zmq_context;
  sockets = g_hash_table_new_full(g_direct_hash,
                                  g_direct_equal,
                                  NULL,
                                  zmq_socket_destroy);
  DEBUG("Creating mud.zmq table.");
  lua_getglobal(lua, "mud");

  /* Put functions in the zmq table. */
  lua_newtable(lua);
  static const luaL_Reg zmq_funcs[] = {
    { "socket" , lua_zmq_socket  },
    { "version", lua_zmq_version },
    { NULL     , NULL            }
  };
  luaL_register(lua, NULL, zmq_funcs);

  /* Socket types. */
  DECLARE_CONST(REQ);
  DECLARE_CONST(REP);
  DECLARE_CONST(DEALER);
  DECLARE_CONST(ROUTER);
  DECLARE_CONST(PUB);
  DECLARE_CONST(SUB);
  DECLARE_CONST(PUSH);
  DECLARE_CONST(PULL);
  DECLARE_CONST(PAIR);

  /* Readable socket options. */
  DECLARE_CONST(AFFINITY);
  DECLARE_CONST(BACKLOG);
  DECLARE_CONST(EVENTS);
  DECLARE_CONST(FD);
  DECLARE_CONST(HWM);
  DECLARE_CONST(IDENTITY);
  DECLARE_CONST(LINGER);
  DECLARE_CONST(MCAST_LOOP);
  DECLARE_CONST(RATE);
  DECLARE_CONST(RCVBUF);
  DECLARE_CONST(RECONNECT_IVL);
  DECLARE_CONST(RECONNECT_IVL_MAX);
  DECLARE_CONST(RECOVERY_IVL);
  DECLARE_CONST(RECOVERY_IVL_MSEC);
  DECLARE_CONST(RCVMORE);
  DECLARE_CONST(SNDBUF);
  DECLARE_CONST(SWAP);
  DECLARE_CONST(TYPE);

  /* Send/receive flags. */
  DECLARE_CONST(NOBLOCK);

  /* Send-only flags. */
  DECLARE_CONST(SNDMORE);

  lua_setfield(lua, -2, "zmq");
  lua_pop(lua, 1);

  /* Set up the socket metatable. */
  luaL_newmetatable(lua, SOCKET_TYPE);
  static const luaL_Reg zmq_methods[] = {
    { "__gc"   , lua_zmq_close   },
    { "bind"   , lua_zmq_bind    },
    { "close"  , lua_zmq_close   },
    { "connect", lua_zmq_connect },
    { "getopt" , lua_zmq_getopt  },
    { "recv"   , lua_zmq_recv    },
    { "send"   , lua_zmq_send    },
    { NULL     , NULL            }
  };
  lua_newtable(lua);
  luaL_register(lua, NULL, zmq_methods);
  lua_setfield(lua, -2, "__index");
  lua_pop(lua, 1);
}

void lua_zmq_deinit(void) {
  g_hash_table_unref(sockets);
}
