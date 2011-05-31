#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "lua_zmq.h"

#include <lauxlib.h>
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

static void zmq_socket_destroy(gpointer s) {
  struct lua_zmq_socket* socket = s;
  lua_State* lua = lua_api_get();
  if (zmq_close(socket->socket) == -1) PWARN("zmq_socket_destroy(zmq_close)");
  luaL_unref(lua, LUA_REGISTRYINDEX, socket->in_watcher);
  luaL_unref(lua, LUA_REGISTRYINDEX, socket->out_watcher);
}

static gint lua_zmq_close(lua_State* lua) {
  /* Even though this might fire twice (socket:close(), followed by
     __gc()), it's OK because the destructor code is executed once
     (when it's removed by the hashtable's DestroyNotify callback). */
  struct lua_zmq_socket* socket = luaL_checkudata(lua, 1, SOCKET_TYPE);
  g_hash_table_remove(sockets, socket->socket);
  return 0;
}

static gint lua_zmq_socket(lua_State* lua) {
  gint type = luaL_checkint(lua, 1);
  gpointer socket = zmq_socket(context, type);
  if (socket == NULL) {
    luaL_Buffer buf;
    luaL_buffinit(lua, &buf);
    luaL_addstring(&buf, "zmq_socket error: ");
    luaL_addstring(&buf, zmq_strerror(errno));
    luaL_pushresult(&buf);
    lua_error(lua);
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

  lua_setfield(lua, -2, "zmq");
  lua_pop(lua, 1);

  /* Set up the socket metatable. */
  luaL_newmetatable(lua, SOCKET_TYPE);
  static const luaL_Reg zmq_methods[] = {
    { "__gc" , lua_zmq_close },
    { "close", lua_zmq_close },
    { NULL   , NULL          }
  };
  lua_newtable(lua);
  luaL_register(lua, NULL, zmq_methods);
  lua_setfield(lua, -2, "__index");
  lua_pop(lua, 1);
}

void lua_zmq_deinit(void) {
  g_hash_table_unref(sockets);
}
