#include "log.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <zmq.h>

#define LOG_DIR "log"

static enum log_level log_level = LOG_LEVEL_DEBUG;
static FILE *log_file = NULL;
static struct tm last_log;

static void log_close(void) {
  fclose(log_file);
  log_file = NULL;
}

static void log_rotate(void) {
  if (log_file != NULL) log_close();
  time_t now = time(NULL);
  gchar today_str[9]; /* "YYYYMMDD" + \0 */
  strftime(today_str, sizeof(today_str), "%Y%m%d", localtime(&now));
  gchar *filename = g_strdup_printf(LOG_DIR G_DIR_SEPARATOR_S "%s.log",
                                    today_str);
  if ((log_file = fopen(filename, "a")) == NULL) {
    perror("log_rotate: fopen"); /* Not log_perror: avoid recursion. */
  }
  g_free(filename);
}

const gchar *log_level_to_string(enum log_level level) {
  switch (level) {
  case LOG_LEVEL_DEBUG: return "debug";
  case LOG_LEVEL_INFO:  return "info";
  case LOG_LEVEL_WARN:  return "warn";
  case LOG_LEVEL_ERROR: return "error";
  case LOG_LEVEL_FATAL: return "fatal";
  }
  return "";
}

enum log_level log_get_level(void) {
  return log_level;
}

void log_set_level(enum log_level new_level) {
  log_level = new_level;
}

void log_printf(enum log_level level, const gchar *format, ...) {
  if (level < log_level) return;

  time_t now = time(NULL);
  struct tm now_tm;
  localtime_r(&now, &now_tm);
  gchar now_str[18]; /* "YYYYMMDD HH:MM:DD" + \0 */
  strftime(now_str, sizeof(now_str), "%Y%m%d %H:%M:%S", &now_tm);

  if (log_file == NULL
      || now_tm.tm_year > last_log.tm_year
      || now_tm.tm_yday > last_log.tm_yday) {
    log_rotate();
  }
  last_log = now_tm;

  static const char labels[] = { 'D', 'I', 'W', 'E', 'F' };
  va_list args;
  va_start(args, format);
  gchar *message = g_strdup_vprintf(format, args);
  va_end(args);
  printf("[%s] (%c) %s\n", now_str, labels[level], message);
  if (log_file != NULL) {
    fprintf(log_file, "[%s] (%c) %s\n", now_str, labels[level], message);
    fflush(log_file);
  }
  g_free(message);

  if (level == LOG_LEVEL_FATAL) {
    if (log_file != NULL) log_close();
    exit(EXIT_FAILURE);
  }
}

void log_perror(enum log_level level, const gchar *message) {
  log_printf(level, "%s: %s", message, zmq_strerror(errno));
}
