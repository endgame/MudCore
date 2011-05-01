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
 **   (gint       @var{server},         @
 **    gpointer   @var{zmq_rep_socket})
 ** Run the mud's main loop.
 ** @end deftypefun
 **/
void io_mainloop(gint server,
                 gpointer zmq_rep_socket);

#endif
