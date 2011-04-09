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
