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
#include "arg_parse.h"

#include <stdio.h>
#include <string.h>

void arg_parse(gint argc,
               gchar* argv[],
               const struct arg_parse_funcs* funcs,
               gpointer user_data) {
  gint arg;
  for (arg = 0; arg < argc; arg++) {
    if (strcmp(argv[arg], "--") == 0) { /* Force end of flags/opts? */
      arg++;
      break;
    }

    /* If it's not an option or a flag, we'll still need to save it */
    if (argv[arg][0] != '-') break;

    /* It's an option or a flag. */
    gchar* equals = strchr(argv[arg], '=');
    if (equals == NULL) { /* It's a flag. */
      if (funcs->on_flag == NULL) continue;

      if (strstr(argv[arg], "-no-") == argv[arg]) {
        funcs->on_flag
          (argv[arg] + 4, FALSE, user_data); /* +4: skip the "-no-" */
      } else {
        funcs->on_flag
          (argv[arg] + 1, TRUE, user_data); /* +1: skip the "-" */
      }
    } else if (funcs->on_option != NULL) {
      /* -1 because we're not copying the '-' */
      gchar* name = g_strndup(argv[arg] + 1, equals - argv[arg] - 1);
      funcs->on_option(name, equals + 1, user_data);
      g_free(name);
    }
  }

  if (funcs->on_positional != NULL) {
    funcs->on_positional(argc - arg, argv + arg, user_data);
  }
}
