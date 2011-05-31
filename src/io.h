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
