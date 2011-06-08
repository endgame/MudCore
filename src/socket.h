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
 ** @node socket.h
 ** @section socket.h
 **/

#ifndef SOCKET_H
#define SOCKET_H

#include <glib.h>

/**
 ** @deftypefun gint socket_server @
 **   (const gchar* @var{port})
 ** Create a new socket, bound to @var{port}, in nonblocking
 ** mode. Return -1 on error.
 ** @end deftypefun
 **/
gint socket_server(const gchar* port);

/**
 ** @deftypefun gint socket_accept @
 **   (gint @var{fd})
 ** Accept a connection on @var{fd}, put it in nonblocking mode, set
 ** @code{SO_OOBINLINE} on it and return it. Return -1 on error.
 ** @end deftypefun
 **/
gint socket_accept(gint fd);

/**
 ** @deftypefun void socket_close @
 **   (gint @var{fd})
 ** Close a socket. This is better than @code{close(2)} as it handles
 ** @code{EINTR}.
 ** @end deftypefun
 **/
void socket_close(gint fd);

#endif
