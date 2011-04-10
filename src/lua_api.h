/**
 ** @node lua_api.h
 ** @section lua_api.h
 **/

#ifndef LUA_API_H
#define LUA_API_H

#include <glib.h>
#include <lua.h>

/**
 ** @deftypefun lua_State* lua_api_init @
 **   (gint   @var{argc},               @
 **    gchar* @var{argv}[])
 ** Construct and return a @code{lua_State*} that:
 ** @itemize
 ** @item Uses GLib for memory management.
 ** @item Has the Lua standard library opened.
 ** @item Has the MudCore Lua library opened.
 ** @end itemize
 ** @end deftypefun
 **/
lua_State* lua_api_init(gint argc, gchar* argv[]);

#endif
