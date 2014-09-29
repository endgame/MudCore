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
 ** @node lua_log.h
 ** @section lua_log.h
 **/

#ifndef LUA_LOG_H
#define LUA_LOG_H

#include <lua.h>

/**
 ** @deftypefun void lua_log_init @
 **   (lua_State* @var{lua})
 ** Add the @code{mudcore.log} api to @var{lua} (@pxref{mudcore.log}).
 ** @end deftypefun
 **/
void lua_log_init(lua_State* lua);

#endif
