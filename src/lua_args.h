/**
 ** @node lua_args.h
 ** @section lua_args.h
 **/

#ifndef LUA_ARGS_H
#define LUA_ARGS_H

#include <glib.h>
#include <lua.h>

/**
 ** @deftypefun void lua_args_init @
 **   (lua_State* @var{lua},       @
 **    gint       @var{argc},      @
 **    gchar*     @var{argv}[])
 ** Add the @code{mud.args} api to @var{lua} (@pxref{mud.args}).
 ** @end deftypefun
 **/
void lua_args_init(lua_State* lua, gint argc, gchar* argv[]);

#endif
