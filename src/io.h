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
 ** @node io.h
 ** @section io.h
 **/

#ifndef IO_H
#define IO_H

#include <glib.h>
#include <lua.h>

/**
 ** @deftypefun void io_mainloop @
 **   (gint @var{server})
 ** Run the mud's main loop.
 ** @end deftypefun
 **/
void io_mainloop(gint server);

/**
 ** @deftypefun void io_shutdown @
 **   (void)
 ** Shut down the server.
 ** @end deftypefun
 **/
void io_shutdown(void);

#endif
