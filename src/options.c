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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "options.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arg_parse.h"
#include "log.h"

#define DEFAULT_PORT "5000"
#define DEFAULT_PULSE_LENGTH 100000 /* in microseconds */

static gboolean file_logging = TRUE;
static gchar* port = NULL;
static gint pulse_length = DEFAULT_PULSE_LENGTH;

static void options_on_flag(const gchar* flagname,
                            gboolean value,
                            gpointer user_data) {
  (void) user_data; /* Not used. */
  if (strcmp(flagname, "file-logging") == 0) {
    file_logging = value;
  } else if (strcmp(flagname, "help") == 0 && value) {
    puts("MudCore Command Line:\n"
         "=====================\n"
         "  -(no-)file-logging         (dis)able logging to file\n"
         "  -help                      print this message, then exit\n"
         "  -log-level=LEVEL           set logging level to one of:\n"
         "                             debug, info, warn, error or fatal\n"
         "  -port=PORT                "
         " port for the main socket [" DEFAULT_PORT "]\n"
         "  -pulse-length=LENGTH      "
         " length of each pulse in usec"
         " [" G_STRINGIFY(DEFAULT_PULSE_LENGTH) "]\n"
         "\n"
         "Lua code may use other flags not documented here.");
    exit(EXIT_SUCCESS);
  }
}

static void options_on_option(const gchar* name,
                              const gchar* value,
                              gpointer user_data) {
  (void) user_data; /* Not used. */
  if (strcmp(name, "log-level") == 0) {
    for (enum log_level level = LOG_LEVEL_DEBUG;
         level <= LOG_LEVEL_FATAL;
         level++) {
      if (strcmp(value, log_level_to_string(level)) == 0) {
        log_set_level(level);
        return;
      }
    }
    printf("Unknown log level \"%s\"\n", value);
    exit(EXIT_FAILURE);
  } else if (strcmp(name, "port") == 0) {
    g_free(port);
    port = g_strdup(value);
  } else if (strcmp(name, "pulse-length") == 0) {
    errno = 0;
    pulse_length = strtol(value, NULL, 10);
    if (errno != 0 || pulse_length >= 1000000) {
      fprintf(stderr, "Invalid number for -pulse-length: %s\n", value);
      exit(EXIT_FAILURE);
    }
  }
}

void options_init(gint argc, gchar* argv[]) {
  port = g_strdup(DEFAULT_PORT);
  struct arg_parse_funcs funcs = {
    .on_flag       = options_on_flag,
    .on_option     = options_on_option,
    .on_positional = NULL
  };
  arg_parse(argc, argv, &funcs, NULL);
}

void options_deinit(void) {
  g_free(port);
}

gboolean options_file_logging(void) {
  return file_logging;
}

gchar* options_port(void) {
  return port;
}

gint options_pulse_length(void) {
  return pulse_length;
}
