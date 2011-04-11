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
 ** Accept a connection on @var{fd}, put it in nonblocking mode and
 ** return it. Return -1 on error.
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
