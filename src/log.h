/**
 ** @node log.h
 ** @section log.h
 **/

#ifndef LOG_H
#define LOG_H

#include <glib.h>

/**
 ** @deftp {Data Type} {enum log_level}
 ** @code{enum log_level} is an enumeration of all possible logging
 ** levels. Each level includes messages from higher levels. Logging
 ** to @code{LOG_LEVEL_FATAL} will cause the server to @code{abort(3)}.
 ** @end deftp
 **
 ** @deftp {enum log_level} LOG_LEVEL_DEBUG
 ** @deftpx {enum log_level} LOG_LEVEL_INFO
 ** @deftpx {enum log_level} LOG_LEVEL_WARN
 ** @deftpx {enum log_level} LOG_LEVEL_ERROR
 ** @deftpx {enum log_level} LOG_LEVEL_FATAL
 ** The various logging levels.
 ** @end deftp
 **/
enum log_level {
  LOG_LEVEL_DEBUG = 0,
  LOG_LEVEL_INFO  = 1,
  LOG_LEVEL_WARN  = 2,
  LOG_LEVEL_ERROR = 3,
  LOG_LEVEL_FATAL = 4
};

/**
 ** @deftypefun {const gchar*} log_level_to_string @
 **   (enum log_level @var{level})
 ** Convert a log level to a string form. For example,
 ** @code{log_level_to_string(LOG_LEVEL_INFO)} returns
 ** @code{"info"}. The returned value is statically allocated.
 ** @end deftypefun
 **/
const gchar* log_level_to_string(enum log_level level);

/**
 ** @deftypefun {enum log_level} log_get_level @
 **   (void)
 ** @deftypefunx void log_set_level @
 **   (enum log_level @var{new_level})
 ** Get and set the global logging level.
 ** @end deftypefun
 **/
enum log_level log_get_level(void);
void log_set_level(enum log_level new_level);

/**
 ** @deftypefun void log_printf     @
 **   (enum log_level @var{level},  @
 **    const gchar*   @var{format}, @
 **    ...)
 ** Log a message to @var{level}, with printf-style format
 ** specification.
 ** @end deftypefun
 **/
void log_printf(enum log_level level, const gchar* format, ...)
  G_GNUC_PRINTF(2, 3);

/**
 ** @deftypefun void log_perror      @
 **   (enum log_level @var{level},   @
 **    const gchar*   @var{message}) @
 ** Log a message to @var{level}, looking like it came from stdio's
 ** @code{perror(3)}.
 ** @end deftypefun
 **/
void log_perror(enum log_level level, const gchar* message);

/**
 ** @defmac DEBUG (...)
 ** @defmacx INFO (...)
 ** @defmacx WARN (...)
 ** @defmacx ERROR (...)
 ** @defmacx FATAL (...)
 ** These are convenience wrappers around @code{log_printf}, which
 ** pass along the correct @var{LEVEL} parameter.
 ** @end defmac
 **/
#define DEBUG(...) log_printf(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define INFO(...)  log_printf(LOG_LEVEL_INFO,  __VA_ARGS__)
#define WARN(...)  log_printf(LOG_LEVEL_WARN,  __VA_ARGS__)
#define ERROR(...) log_printf(LOG_LEVEL_ERROR, __VA_ARGS__)
#define FATAL(...) log_printf(LOG_LEVEL_FATAL, __VA_ARGS__)

#endif
