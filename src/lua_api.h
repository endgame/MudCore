/**
 ** @node lua_api.h
 ** @section lua_api.h
 **/

#ifndef LUA_API_H
#define LUA_API_H

#include <glib.h>
#include <lua.h>

/**
 ** @deftypefun void lua_api_init  @
 **   (gpointer @var{zmq_context}, @
 **    gint     @var{argc},        @
 **    gchar*   @var{argv}[])
 ** Construct a @code{lua_State*} (that is accessible using
 ** @code{lua_api_get()} that:
 ** @itemize
 ** @item Uses GLib for memory management.
 ** @item Has the Lua standard library opened.
 ** @item Has the MudCore Lua library opened.
 ** @end itemize
 ** @end deftypefun
 **/
void lua_api_init(gpointer zmq_context, gint argc, gchar* argv[]);

/**
 ** @deftypefun void lua_api_deinit @
 **   (void)
 ** Release all memory used by lua code.
 ** @end deftypefun
 **/
void lua_api_deinit(void);

/**
 ** @deftypefun lua_State* lua_api_get @
 **   (void)
 ** Return the lua state of the main thread, or @code{NULL} if
 ** unitialised.
 ** @end deftypefun
 **/
lua_State* lua_api_get(void);

#endif
