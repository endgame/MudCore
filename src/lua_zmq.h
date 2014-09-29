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
 ** Add the @code{mudcore.zmq} api to @var{lua}
 ** (@pxref{mudcore.zmq}). @code{zmq_context} is the ZeroMQ context
 ** used by all operations.
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
 ** (@pxref{mudcore.zmq}). Adjust @var{poll_count} to reflect the
 ** number of items processed.
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
