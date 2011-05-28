/**
 ** @node options.h
 ** @section options.h
 **/

#ifndef OPTIONS_H
#define OPTIONS_H

#include <glib.h>

/**
 ** @deftypefun void options_init @
 **   (gint   @var{argc},         @
 **    gchar* @var{argv}[])
 ** Populate options using the command line.
 ** @end deftypefun
 **/
void options_init(gint argc, gchar* argv[]);

/**
 ** @deftypefun void options_deinit @
 **   (void)
 ** Free memory used by @code{options_init()}.
 ** @end deftypefun
 **/
void options_deinit(void);

/**
 ** @deftypefun gboolean options_file_logging @
 **   (void)
 ** Get the value of the @code{-file-logging} flag.
 ** @end deftypefun
 **/
gboolean options_file_logging(void);

/**
 ** @deftypefun gchar* options_port @
 **   (void)
 ** Get the value of the @code{-port} flag.
 ** @end deftypefun
 **/
gchar* options_port(void);

/**
 ** @deftypefun gint options_pulse_length @
 **   (void)
 ** Get the value of the @code{-pulse-length} option.
 ** @end deftypefun
 **/
gint options_pulse_length(void);

/**
 ** @deftypefun gint options_zmq_io_threads @
 **   (void)
 ** Get the value of the @code{-zmq-io-threads} option.
 ** @end deftypefun
 **/
gint options_zmq_io_threads(void);

/**
 ** @deftypefun gchar* options_zmq_pub_endpoint @
 **   (void)
 ** Get the value of the @code{-zmq-pub-endpoint} option.
 ** @end deftypefun
 **/
gchar* options_zmq_pub_endpoint(void);

/**
 ** @deftypefun gchar* options_zmq_rep_endpoint @
 **   (void)
 ** Get the value of the @code{-zmq-rep-endpoint} option.
 ** @end deftypefun
 **/
gchar* options_zmq_rep_endpoint(void);

#endif
