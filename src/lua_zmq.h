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
 **    gpointer   @var{zmq_context})
 ** Add the @code{mud.zmq} api to @var{lua}
 ** (@pxref{mud.zmq}). @code{zmq_context} is the ZeroMQ context used
 ** by all operations.
 ** @end deftypefun
 **/
void lua_zmq_init(lua_State* lua, gpointer zmq_context);

void lua_zmq_deinit(void);

#endif
