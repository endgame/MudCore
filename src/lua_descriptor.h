/* MudCore - a simple, lua-scripted MUD server
 * Copyright (C) 2011, 2012  Jack Kelly <jack@jackkelly.name>
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
 ** @node lua_descriptor.h
 ** @section lua_descriptor.h
 **/

#ifndef LUA_DESCRIPTOR_H
#define LUA_DESCRIPTOR_H

#include <glib.h>
#include <lua.h>

struct descriptor;

/**
 ** @deftypefun void lua_descriptor_init @
 **   (lua_State* @var{lua})
 ** Add the @code{mud.descriptor} api to @var{lua} (@pxref{mud.descriptor}).
 ** @end deftypefun
 **/
void lua_descriptor_init(lua_State* lua);

/**
 ** @deftypefun void lua_descriptor_start @
 **   (struct descriptor* @var{descriptor})
 ** Create a new thread for @var{descriptor}, store it in the
 ** @code{descriptor->thread_ref} field and start it with the
 ** descriptor number.
 ** @end deftypefun
 **/
void lua_descriptor_start(struct descriptor* descriptor);

/**
 ** @deftypefun void lua_descriptor_command  @
 **   (struct descriptor* @var{descriptor},  @
 **    const gchar*       @var{command})
 ** Resume a descriptor's thread with @var{command} sent to the
 ** descriptor as the next line of input.
 ** @end deftypefun
 **/
void lua_descriptor_command(struct descriptor* descriptor,
                            const gchar* command);

/**
 ** @deftypefun void lua_descriptor_continue @
 **   (struct descriptor* @var{descriptor})
 ** Resume a descriptor's thread with nothing yielded (i.e., from a
 ** delay).
 ** @end deftypefun
 **/
void lua_descriptor_continue(struct descriptor* descriptor);

#endif
