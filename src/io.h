/**
 ** @node io.h
 ** @section io.h
 **/

#ifndef IO_H
#define IO_H

#include <glib.h>
#include <lua.h>

/**
 ** @deftypefun void io_mainloop        @
 **   (lua_State* @var{lua},            @
 **    gint       @var{server},         @
 **    gpointer   @var{zmq_rep_socket}, @
 **    gpointer   @var{zmq_pub_socket})
 ** Run the mud's main loop.
 ** @end deftypefun
 **/
void io_mainloop(lua_State* lua,
                 gint server,
                 gpointer zmq_rep_socket,
                 gpointer zmq_pub_socket);

#endif
