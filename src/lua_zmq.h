/**
 ** @node lua_zmq.h
 ** @section lua_zmq.h
 **/

#ifndef LUA_ZMQ_H
#define LUA_ZMQ_H

#include <glib.h>
#include <lua.h>

/**
 ** @deftypefun void lua_zmq_init @
 **   (lua_State* @var{lua},      @
 **    gpointer   @var{zmq_pub_socket})
 ** Add the @code{mud.zmq} api to @var{lua}
 ** (@pxref{mud.zmq}). @code{zmq_pub_socket} is a @code{ZMQ_PUB}
 ** socket that is used for @code{mud.zmq.publish}.
 ** @end deftypefun
 **/
void lua_zmq_init(lua_State* lua, gpointer zmq_pub_socket);

/**
 ** @deftypefun void lua_zmq_on_request @
 **   (gpointer     @var{socket},       @
 **    const gchar* @var{message},      @
 **    gint         @var{length})
 **
 ** Call @code{mud.zmq.on_request} (@pxref{mud.zmq}) with
 ** @var{message}. The value returned from lua, is sent on the socket.
 ** string. If @code{mud.zmq.on_request} is not defined fails with an
 ** error, send an empty message on @var{socket}.
 ** @end deftypefun
 **/
void lua_zmq_on_request(gpointer socket, const gchar* message, gint length);

#endif
