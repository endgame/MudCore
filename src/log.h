#ifndef LOG_H
#define LOG_H

#include <glib.h>

enum log_level {
  LOG_LEVEL_DEBUG = 0,
  LOG_LEVEL_INFO  = 1,
  LOG_LEVEL_WARN  = 2,
  LOG_LEVEL_ERROR = 3,
  LOG_LEVEL_FATAL = 4
};

const gchar *log_level_to_string(enum log_level level);

enum log_level log_get_level(void);
void log_set_level(enum log_level new_level);

void log_printf(enum log_level level, const gchar *format, ...)
  G_GNUC_PRINTF(2, 3);
void log_perror(enum log_level level, const gchar *message);

#define DEBUG(...) log_printf(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define INFO(...)  log_printf(LOG_LEVEL_INFO,  __VA_ARGS__)
#define WARN(...)  log_printf(LOG_LEVEL_WARN,  __VA_ARGS__)
#define ERROR(...) log_printf(LOG_LEVEL_ERROR, __VA_ARGS__)
#define FATAL(...) log_printf(LOG_LEVEL_FATAL, __VA_ARGS__)

#endif
