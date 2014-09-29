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
#include "lua_zmq.h"

#include <lauxlib.h>
#include <string.h>
#include <zmq.h>

#include "log.h"
#include "lua_api.h"

#define SOCKET_TYPE "mudcore.zmq_socket"
#define Z85_KEY_LENGTH 41 /* +1 for '\0' */
#define ZMQ_IDENTITY_MAXLEN 255

struct watcher {
  gint socket_ref; /* Ref from luaL_ref. */
  gint in_ref; /* Ref from luaL_ref. */
  gint out_ref; /* Ref from luaL_ref. */
};

static gpointer context;
static GHashTable* /* of gpointer (zmq socket) -> struct watcher* */ watchers;

/* Iterate through the watcher table. Iter names the iterator, Value
   the struct watcher pointer. */
#define WATCHER_FOREACH(Iter, Value)                                    \
  GHashTableIter Iter;                                                  \
  g_hash_table_iter_init(&iter, watchers);                              \
  struct watcher* Value;                                                \
  while (g_hash_table_iter_next(&Iter, NULL, (gpointer*)&Value))

/* Callback for ZeroMQ to free data. */
static void zmq_g_free(gpointer data, gpointer hint) {
  (void) hint; /* Not used. */
  g_free(data);
}

/* Callback to cleanup a removed watcher from the table. */
static void watcher_destroy(gpointer w) {
  struct watcher* watcher = w;
  lua_State* lua = lua_api_get();
  luaL_unref(lua, LUA_REGISTRYINDEX, watcher->socket_ref);
  luaL_unref(lua, LUA_REGISTRYINDEX, watcher->in_ref);
  luaL_unref(lua, LUA_REGISTRYINDEX, watcher->out_ref);
  watcher->socket_ref = LUA_NOREF;
  watcher->in_ref = LUA_NOREF;
  watcher->out_ref = LUA_NOREF;
  g_free(watcher);
}

/* Check if a ref points to a non-nil object. */
static gboolean real_ref(gint ref) {
  return (ref != LUA_NOREF && ref != LUA_REFNIL);
}

/* Raise an error with message MSG..": "..zmq_strerror(errno). */
static gint lua_zmq_error(lua_State* lua, const gchar* msg) {
  return luaL_error(lua, "%s: %s", msg, zmq_strerror(errno));
}

static gint lua_zmq_bind(lua_State* lua) {
  gpointer socket = *(gpointer*)luaL_checkudata(lua, 1, SOCKET_TYPE);
  const gchar* endpoint = luaL_checkstring(lua, 2);
  if (zmq_bind(socket, endpoint) == -1) {
    return lua_zmq_error(lua, "zmq_bind");
  }
  return 0;
}

static gint lua_zmq_close(lua_State* lua) {
  gpointer* socket = luaL_checkudata(lua, 1, SOCKET_TYPE);
  if (*socket != NULL) {
    struct watcher* watcher = g_hash_table_lookup(watchers, *socket);
    if (watcher != NULL) {
      lua_State* lua = lua_api_get();
      luaL_unref(lua, LUA_REGISTRYINDEX, watcher->socket_ref);
      luaL_unref(lua, LUA_REGISTRYINDEX, watcher->in_ref);
      luaL_unref(lua, LUA_REGISTRYINDEX, watcher->out_ref);
      watcher->socket_ref = LUA_NOREF;
      watcher->in_ref = LUA_NOREF;
      watcher->out_ref = LUA_NOREF;
    }
    zmq_close(*socket);
    *socket = NULL;
  }
  return 0;
}

static gint lua_zmq_connect(lua_State* lua) {
  gpointer socket = *(gpointer*)luaL_checkudata(lua, 1, SOCKET_TYPE);
  const gchar* endpoint = luaL_checkstring(lua, 2);
  if (zmq_connect(socket, endpoint) == -1) {
    return lua_zmq_error(lua, "zmq_connect");
  }
  return 0;
}

static gint lua_zmq_curve_keypair(lua_State* lua) {
#ifdef HAVE_ZMQ_CURVE_KEYPAIR
  gchar public_key[Z85_KEY_LENGTH];
  gchar secret_key[Z85_KEY_LENGTH];

  if (zmq_curve_keypair(public_key, secret_key) == -1) {
    return lua_zmq_error(lua, "zmq_curve_keypair");
  }

  lua_pushstring(lua, public_key);
  lua_pushstring(lua, secret_key);
  return 2;
#else
  return luaL_error(lua, "zmq_curve_keypair() not available.");
#endif
}

static gint lua_zmq_getopt(lua_State* lua) {
  gint opt = luaL_checkint(lua, 1);
  lua_pushinteger(lua, zmq_ctx_get(context, opt));
  return 1;
}

static gint lua_zmq_getsockopt(lua_State* lua) {
  gpointer socket = *(gpointer*)luaL_checkudata(lua, 1, SOCKET_TYPE);
  gint option = luaL_checkint(lua, 2);
  union {
    gchar s[ZMQ_IDENTITY_MAXLEN];
    gint i;
    gint64 i64;
    guint64 u64;
  } buf;
  gsize buf_size = sizeof(buf);

  if (option == ZMQ_CURVE_PUBLICKEY
      || option == ZMQ_CURVE_SECRETKEY
      || option == ZMQ_CURVE_SERVERKEY) {
    buf_size = Z85_KEY_LENGTH;
  }

  if (zmq_getsockopt(socket, option, &buf, &buf_size) == -1) {
    return lua_zmq_error(lua, "zmq_getsockopt");
  }

  switch (option) {
  case ZMQ_CURVE_SERVER:
  case ZMQ_IMMEDIATE:
  case ZMQ_IPV6:
  case ZMQ_PLAIN_SERVER:
  case ZMQ_RCVMORE:
    lua_pushboolean(lua, buf.i);
    return 1;
  case ZMQ_BACKLOG:
  case ZMQ_EVENTS:
  case ZMQ_FD:
  case ZMQ_LINGER:
  case ZMQ_MECHANISM:
  case ZMQ_MULTICAST_HOPS:
  case ZMQ_RATE:
  case ZMQ_RCVBUF:
  case ZMQ_RCVHWM:
  case ZMQ_RCVTIMEO:
  case ZMQ_RECONNECT_IVL:
  case ZMQ_RECONNECT_IVL_MAX:
  case ZMQ_RECOVERY_IVL:
  case ZMQ_SNDBUF:
  case ZMQ_SNDHWM:
  case ZMQ_SNDTIMEO:
  case ZMQ_TCP_KEEPALIVE:
  case ZMQ_TCP_KEEPALIVE_CNT:
  case ZMQ_TCP_KEEPALIVE_IDLE:
  case ZMQ_TCP_KEEPALIVE_INTVL:
  case ZMQ_TYPE:
    lua_pushinteger(lua, buf.i);
    return 1;
  case ZMQ_MAXMSGSIZE:
    lua_pushinteger(lua, buf.i64);
    return 1;
  case ZMQ_AFFINITY:
    lua_pushinteger(lua, buf.u64);
    return 1;
  case ZMQ_CURVE_PUBLICKEY:
  case ZMQ_CURVE_SECRETKEY:
  case ZMQ_CURVE_SERVERKEY:
  case ZMQ_IDENTITY:
  case ZMQ_LAST_ENDPOINT:
  case ZMQ_PLAIN_PASSWORD:
  case ZMQ_PLAIN_USERNAME:
  case ZMQ_ZAP_DOMAIN:
    lua_pushlstring(lua, buf.s, buf_size);
    return 1;
  }
  return 0;
}

static gint lua_zmq_recv(lua_State* lua) {
  gpointer socket = (gpointer*)luaL_checkudata(lua, 1, SOCKET_TYPE);
  gint flags = luaL_optint(lua, 2, 0);
  zmq_msg_t msg;
  zmq_msg_init(&msg);
  if (zmq_msg_recv(&msg, socket, flags) == -1) {
    zmq_msg_close(&msg);
    return lua_zmq_error(lua, "zmq_recv");
  }
  lua_pushlstring(lua, zmq_msg_data(&msg), zmq_msg_size(&msg));
  zmq_msg_close(&msg);
  return 1;
}

static gint lua_zmq_send(lua_State* lua) {
  gpointer socket = *(gpointer*)luaL_checkudata(lua, 1, SOCKET_TYPE);
  gsize len;
  const gchar* str = luaL_checklstring(lua, 2, &len);
  gint flags = luaL_optint(lua, 3, 0);
  zmq_msg_t msg;
  zmq_msg_init_data(&msg,
                    memcpy(g_new(gchar, len), str, len),
                    len,
                    zmq_g_free,
                    NULL);
  if (zmq_msg_send(&msg, socket, flags) == -1) {
    zmq_msg_close(&msg);
    return lua_zmq_error(lua, "zmq_msg_send");
  }
  return 0;
}

static gint lua_zmq_setopt(lua_State* lua) {
  gint option = luaL_checkint(lua, 1);
  gint value = luaL_checkint(lua, 2);
  if (zmq_ctx_set(context, option, value) == -1) {
    return lua_zmq_error(lua, "zmq_ctx_set");
  }
  return 0;
}

static gint lua_zmq_setsockopt(lua_State* lua) {
  gpointer socket = *(gpointer*)luaL_checkudata(lua, 1, SOCKET_TYPE);
  gint option = luaL_checkint(lua, 2);
  union {
    gint i;
    gint64 i64;
    guint64 u64;
  } optval;
  gboolean set_str = FALSE;
  const gchar* str = NULL;
  gsize optsize;
  switch (option) {
  case ZMQ_AFFINITY:
    optval.u64 = luaL_checkint(lua, 3);
    optsize = sizeof(optval.u64);
    break;
  case ZMQ_MAXMSGSIZE:
    optval.i64 = luaL_checkint(lua, 3);
    optsize = sizeof(optval.i64);
    break;
  case ZMQ_BACKLOG:
  case ZMQ_LINGER:
  case ZMQ_MULTICAST_HOPS:
  case ZMQ_RATE:
  case ZMQ_RCVBUF:
  case ZMQ_RCVHWM:
  case ZMQ_RCVTIMEO:
  case ZMQ_RECONNECT_IVL:
  case ZMQ_RECONNECT_IVL_MAX:
  case ZMQ_RECOVERY_IVL:
  case ZMQ_SNDBUF:
  case ZMQ_SNDHWM:
  case ZMQ_SNDTIMEO:
  case ZMQ_TCP_KEEPALIVE:
  case ZMQ_TCP_KEEPALIVE_CNT:
  case ZMQ_TCP_KEEPALIVE_IDLE:
  case ZMQ_TCP_KEEPALIVE_INTVL:
    optval.i = luaL_checkint(lua, 3);
    optsize = sizeof(optval.i);
    break;
  case ZMQ_CONFLATE:
  case ZMQ_CURVE_SERVER:
  case ZMQ_IMMEDIATE:
  case ZMQ_IPV6:
  case ZMQ_PLAIN_SERVER:
  case ZMQ_PROBE_ROUTER:
  case ZMQ_REQ_CORRELATE:
  case ZMQ_REQ_RELAXED:
  case ZMQ_ROUTER_MANDATORY:
  case ZMQ_XPUB_VERBOSE:
    optval.i = lua_toboolean(lua, 3);
    optsize = sizeof(optval.i);
    break;
  case ZMQ_TCP_ACCEPT_FILTER:
    if (lua_isnil(lua, 3)) {
      set_str = TRUE;
      break;
    }
    /* Fall through. */
  case ZMQ_CURVE_PUBLICKEY:
  case ZMQ_CURVE_SECRETKEY:
  case ZMQ_CURVE_SERVERKEY:
  case ZMQ_IDENTITY:
  case ZMQ_PLAIN_PASSWORD:
  case ZMQ_PLAIN_USERNAME:
  case ZMQ_SUBSCRIBE:
  case ZMQ_UNSUBSCRIBE:
  case ZMQ_ZAP_DOMAIN:
    str = luaL_checklstring(lua, 3, &optsize);
    set_str = TRUE;
    break;
  default:
    return lua_zmq_error(lua, "Unknown (or unimplemented) option.");
  }

  gint rc;
  if (set_str) {
    rc = zmq_setsockopt(socket, option, str, optsize);
  } else {
    rc = zmq_setsockopt(socket, option, &optval, optsize);
  }

  if (rc == -1) {
    return lua_zmq_error(lua, "zmq_setsockopt");
  }
  return 0;
}

static gint lua_zmq_socket(lua_State* lua) {
  gint type = luaL_checkint(lua, 1);
  gpointer socket = zmq_socket(context, type);
  if (socket == NULL) {
    return lua_zmq_error(lua, "zmq_socket");
  }
  gpointer* result = lua_newuserdata(lua, sizeof(*result));
  luaL_getmetatable(lua, SOCKET_TYPE);
  lua_setmetatable(lua, -2);
  *result = socket;
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

static gint lua_zmq_z85_decode(lua_State *lua) {
  const gchar* str = luaL_checkstring(lua, 1);
  gint len = luaL_len(lua, 1);
  if (len % 5 != 0) {
    return luaL_error(lua, "z85_decode: String length not divisible by 5.");
  }

  gint buflen = len * 4 / 5;
  guint8* buf = g_new(guint8, buflen);
  /* zmq_z85_decode should take a const char* but doesn't */
  guint8* rv = zmq_z85_decode(buf, (gchar*)str);

  if (rv == NULL) {
    lua_pushnil(lua);
  } else {
    lua_pushlstring(lua, (gchar*)buf, buflen);
  }

  g_free(buf);
  return 1;
}

static gint lua_zmq_z85_encode(lua_State *lua) {
  const gchar* str = luaL_checkstring(lua, 1);
  gint len = luaL_len(lua, 1);
  if (len % 4 != 0) {
    return luaL_error(lua, "z85_encode: Stirng length not divisible by 4.");
  }

  gint buflen = len * 5 / 4 + 1;
  gchar* buf = g_new(gchar, buflen);
  gchar* rv = zmq_z85_encode(buf, (guint8*)str, len);
  if (rv == NULL) {
    lua_pushnil(lua);
  } else {
    lua_pushlstring(lua, buf, buflen - 1); /* Don't copy terminator */
  }

  g_free(buf);
  return 1;
}

static gint lua_zmq_watch(lua_State* lua) {
  gpointer socket = *(gpointer*)luaL_checkudata(lua, 1, SOCKET_TYPE);
  if (socket == NULL) return luaL_error(lua, "Cannot watch closed socket.");
  struct watcher* watcher = g_hash_table_lookup(watchers, socket);

  if (watcher == NULL) {
    watcher = g_new(struct watcher, 1);
    lua_pushvalue(lua, 1);
    watcher->socket_ref = luaL_ref(lua, LUA_REGISTRYINDEX);
    watcher->in_ref = LUA_NOREF;
    watcher->out_ref = LUA_NOREF;
    g_hash_table_insert(watchers, socket, watcher);
  }
  gint nargs = lua_gettop(lua);
  if (nargs >= 2) {
    if (!lua_isnil(lua, 2)) luaL_checktype(lua, 2, LUA_TFUNCTION);
    luaL_unref(lua, LUA_REGISTRYINDEX, watcher->in_ref);
    lua_pushvalue(lua, 2);
    watcher->in_ref = luaL_ref(lua, LUA_REGISTRYINDEX);
  }
  if (nargs >= 3) {
    if (!lua_isnil(lua, 3)) luaL_checktype(lua, 3, LUA_TFUNCTION);
    luaL_unref(lua, LUA_REGISTRYINDEX, watcher->out_ref);
    lua_pushvalue(lua, 3);
    watcher->out_ref = luaL_ref(lua, LUA_REGISTRYINDEX);
  }
  return 0;
}

/* Call a watcher with a given socket. */
static void watcher_call(lua_State* lua, gint watcher_ref, gint socket_ref) {
  lua_rawgeti(lua, LUA_REGISTRYINDEX, watcher_ref);
  lua_rawgeti(lua, LUA_REGISTRYINDEX, socket_ref);
  if (lua_pcall(lua, 1, 0, 0) != LUA_OK) {
    const gchar* what = lua_tostring(lua, -1);
    ERROR("Error in mudcore.zmq_socket callback: %s", what);
    lua_pop(lua, 1);

    if (!real_ref(socket_ref)) return;

    lua_pushcfunction(lua, lua_zmq_close);
    lua_rawgeti(lua, LUA_REGISTRYINDEX, socket_ref);
    if (lua_pcall(lua, 1, 0, 0) != LUA_OK) {
      what = lua_tostring(lua, -1);
      ERROR("Cannot close socket: %s", what);
      lua_pop(lua, 1);
    }
  }
}

#define DECLARE_CONST(C)                        \
  G_STMT_START {                                \
    lua_pushinteger(lua, ZMQ_##C);              \
    lua_setfield(lua, -2, G_STRINGIFY(C));      \
  } G_STMT_END

void lua_zmq_init(lua_State* lua, gpointer zmq_context) {
  context = zmq_context;
  watchers = g_hash_table_new_full(g_direct_hash,
                                   g_direct_equal,
                                   NULL,
                                   watcher_destroy);
  DEBUG("Creating mudcore.zmq table.");
  lua_getglobal(lua, "mudcore");

  /* Put functions in the zmq table. */
  lua_newtable(lua);
  static const luaL_Reg zmq_funcs[] = {
    { "curve_keypair", lua_zmq_curve_keypair },
    { "getopt"       , lua_zmq_getopt        },
    { "setopt"       , lua_zmq_setopt        },
    { "socket"       , lua_zmq_socket        },
    { "version"      , lua_zmq_version       },
    { "z85_decode"   , lua_zmq_z85_decode    },
    { "z85_encode"   , lua_zmq_z85_encode    },
    { NULL           , NULL                  }
  };
  luaL_setfuncs(lua, zmq_funcs, 0);

  /* Context Options */
  DECLARE_CONST(IO_THREADS);
  DECLARE_CONST(MAX_SOCKETS);
  DECLARE_CONST(IPV6);

  /* Socket types. */
  DECLARE_CONST(REQ);
  DECLARE_CONST(REP);
  DECLARE_CONST(DEALER);
  DECLARE_CONST(ROUTER);
  DECLARE_CONST(PUB);
  DECLARE_CONST(SUB);
  DECLARE_CONST(XPUB);
  DECLARE_CONST(XSUB);
  DECLARE_CONST(PUSH);
  DECLARE_CONST(PULL);
  DECLARE_CONST(PAIR);

  /* Gettable socket options. */
  DECLARE_CONST(EVENTS);
  DECLARE_CONST(FD);
  DECLARE_CONST(LAST_ENDPOINT);
  DECLARE_CONST(MECHANISM);
  DECLARE_CONST(RCVMORE);
  DECLARE_CONST(TYPE);

  /* Gettable/settable socket options. */
  DECLARE_CONST(AFFINITY);
  DECLARE_CONST(BACKLOG);
  DECLARE_CONST(CURVE_PUBLICKEY);
  DECLARE_CONST(CURVE_SECRETKEY);
  DECLARE_CONST(CURVE_SERVER);
  DECLARE_CONST(CURVE_SERVERKEY);
  DECLARE_CONST(IDENTITY);
  DECLARE_CONST(IMMEDIATE);
  DECLARE_CONST(IPV6);
  DECLARE_CONST(LINGER);
  DECLARE_CONST(MAXMSGSIZE);
  DECLARE_CONST(MULTICAST_HOPS);
  DECLARE_CONST(PLAIN_PASSWORD);
  DECLARE_CONST(PLAIN_SERVER);
  DECLARE_CONST(PLAIN_USERNAME);
  DECLARE_CONST(RATE);
  DECLARE_CONST(RCVBUF);
  DECLARE_CONST(RCVHWM);
  DECLARE_CONST(RCVTIMEO);
  DECLARE_CONST(RECONNECT_IVL);
  DECLARE_CONST(RECONNECT_IVL_MAX);
  DECLARE_CONST(RECOVERY_IVL);
  DECLARE_CONST(SNDBUF);
  DECLARE_CONST(SNDHWM);
  DECLARE_CONST(SNDTIMEO);
  DECLARE_CONST(TCP_KEEPALIVE);
  DECLARE_CONST(TCP_KEEPALIVE_CNT);
  DECLARE_CONST(TCP_KEEPALIVE_IDLE);
  DECLARE_CONST(TCP_KEEPALIVE_INTVL);
  DECLARE_CONST(ZAP_DOMAIN);

  /* Settable socket options. */
  DECLARE_CONST(CONFLATE);
  DECLARE_CONST(PROBE_ROUTER);
  DECLARE_CONST(REQ_CORRELATE);
  DECLARE_CONST(REQ_RELAXED);
  DECLARE_CONST(ROUTER_MANDATORY);
  DECLARE_CONST(SUBSCRIBE);
  DECLARE_CONST(TCP_ACCEPT_FILTER);
  DECLARE_CONST(UNSUBSCRIBE);
  DECLARE_CONST(XPUB_VERBOSE);

  /* Flags for socket options. */
  DECLARE_CONST(CURVE);
  DECLARE_CONST(NULL);
  DECLARE_CONST(PLAIN);
  DECLARE_CONST(POLLIN);
  DECLARE_CONST(POLLOUT);

  /* Send/receive flags. */
  DECLARE_CONST(DONTWAIT);

  /* Send-only flags. */
  DECLARE_CONST(SNDMORE);

  lua_setfield(lua, -2, "zmq");
  lua_pop(lua, 1);

  /* Set up the socket metatable. */
  luaL_newmetatable(lua, SOCKET_TYPE);
  static const luaL_Reg zmq_methods[] = {
    { "__gc"   , lua_zmq_close      },
    { "bind"   , lua_zmq_bind       },
    { "close"  , lua_zmq_close      },
    { "connect", lua_zmq_connect    },
    { "getopt" , lua_zmq_getsockopt },
    { "recv"   , lua_zmq_recv       },
    { "send"   , lua_zmq_send       },
    { "setopt" , lua_zmq_setsockopt },
    { "watch"  , lua_zmq_watch      },
    { NULL     , NULL               }
  };
  lua_newtable(lua);
  luaL_setfuncs(lua, zmq_methods, 0);
  lua_setfield(lua, -2, "__index");
  lua_pop(lua, 1);
}

void lua_zmq_deinit(void) {
  g_hash_table_unref(watchers);
}

void lua_zmq_add_pollitems(GArray* /* of zmq_pollitem_t */ pollitems) {
  lua_State* lua = lua_api_get();
  zmq_pollitem_t pollitem;
  memset(&pollitem, 0, sizeof(pollitem));
  WATCHER_FOREACH(iter, watcher) {
    if (!real_ref(watcher->socket_ref)) continue;
    lua_rawgeti(lua, LUA_REGISTRYINDEX, watcher->socket_ref);
    pollitem.socket = *(gpointer*)lua_touserdata(lua, -1);
    lua_pop(lua, 1);
    pollitem.events = 0;
    if (real_ref(watcher->in_ref)) pollitem.events |= ZMQ_POLLIN;
    if (real_ref(watcher->out_ref)) pollitem.events |= ZMQ_POLLOUT;
    g_array_append_val(pollitems, pollitem);
  }
}

void lua_zmq_handle_pollitems(GArray* /* of zmq_pollitem_t */ pollitems,
                              gint* poll_count) {
  lua_State* lua = lua_api_get();
  for (guint i = 0; i < pollitems->len; i++) {
    if (*poll_count == 0) break;
    zmq_pollitem_t* pollitem = &g_array_index(pollitems, zmq_pollitem_t, i);
    if (pollitem->socket == NULL) continue;

    struct watcher* watcher = g_hash_table_lookup(watchers, pollitem->socket);
    if (watcher == NULL) continue; /* Could be the server fd or a
                                      client FD. */

    if (pollitem->revents != 0) (*poll_count)--;
    if (pollitem->revents & ZMQ_POLLIN
        && real_ref(watcher->socket_ref)
        && real_ref(watcher->in_ref)) {
      watcher_call(lua, watcher->in_ref, watcher->socket_ref);
    }
    if (pollitem->revents & ZMQ_POLLOUT
        && real_ref(watcher->socket_ref)
        && real_ref(watcher->out_ref)) {
      watcher_call(lua, watcher->out_ref, watcher->socket_ref);
    }
  }
}

void lua_zmq_remove_unwatched(void) {
  WATCHER_FOREACH(iter, watcher) {
    if (!real_ref(watcher->socket_ref)
        && !real_ref(watcher->in_ref)
        && !real_ref(watcher->out_ref)) g_hash_table_iter_remove(&iter);
  }
}
