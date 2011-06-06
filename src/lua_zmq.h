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

/**
 ** @deftypefun void lua_zmq_deinit @
 **   (void)
 ** Release memory used by the ZeroMQ api.
 ** @end deftypefun
 **/
void lua_zmq_deinit(void);

/**
 ** @deftypefun void lua_zmq_add_pollitems @
 **   (GArray* @var{pollitems})
 ** Add a pollitem to @var{pollitems} (a @code{GArray*} of
 ** @code{zmq_pollitem_t}) for every ZeroMQ socket that has a watcher
 ** enabled.
 ** @end deftypefun
 **/
void lua_zmq_add_pollitems(GArray* /* of zmq_pollitem_t */ pollitems);

/**
 ** @deftypefun void lua_zmq_handle_pollitems @
 **   (GArray* @var{pollitems},               @
 **    gint*   @var{poll_count})
 ** Process up to @var{poll_count} items from @var{pollitems} (a
 ** @code{GArray*} of @code{zmq_pollitem_t}). Call any callbacks
 ** defined by @code{mudcore.zmq_socket.watch()}
 ** (@pxref{mud.zmq}). Adjust @var{poll_count} to reflect the number
 ** of items processed.
 ** @end deftypefun
 **/
void lua_zmq_handle_pollitems(GArray* /* of zmq_pollitem_t */ pollitems,
                              gint* poll_count);

/**
 ** @deftypefun void lua_zmq_remove_unwatched @
 **   (void)
 ** Remove any unused watcher entries.
 ** @end deftypefun
 **/
void lua_zmq_remove_unwatched(void);

#endif
