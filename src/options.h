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

#endif
