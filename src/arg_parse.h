/**
 ** @node arg_parse.h
 ** @section arg_parse.h
 **
 ** This is a simple command-line argument parser. It differentiates
 ** between @emph{flags}, @emph{options} and @emph{positional
 ** arguments}. A @emph{flag} is an argument of the form @code{-foo}
 ** (true) or @code{-no-foo} (false). An @emph{option} is an argument
 ** of the form @code{-name=value}. @emph{Positional arguments} are
 ** everything else. An argument of @code{--} will switch off
 ** processing and make everything appear as a positional argument.
 **/

#ifndef ARG_PARSE_H
#define ARG_PARSE_H

#include <glib.h>

/**
 ** @deftp {Data Type} arg_parse_on_flag
 ** Callback for when the parser finds a flag.
 **
 ** @example
 ** typedef void (*arg_parse_on_flag)(const gchar* flagname,
 **                                   gboolean value,
 **                                   gpointer user_data);
 ** @end example
 ** @end deftp
 **/
typedef void (*arg_parse_on_flag)(const gchar* flagname,
                                  gboolean value,
                                  gpointer user_data);
/**
 ** @deftp {Data Type} arg_parse_on_option
 ** Callback for when the parser finds an option.
 **
 ** @example
 ** typedef void (*arg_parse_on_option)(const gchar* name,
 **                                     const gchar* value,
 **                                     gpointer user_data);
 ** @end example
 ** @end deftp
 **/
typedef void (*arg_parse_on_option)(const gchar* name,
                                    const gchar* value,
                                    gpointer user_data);
/**
 ** @deftp {Data Type} arg_parse_on_positional
 ** Callback for when the parser is finished; only positional
 ** arguments remain.
 **
 ** @example
 ** typedef void (*arg_parse_on_positional)(gint argc,
 **                                         gchar* argv[],
 **                                         gpointer user_data);
 ** @end example
 ** @end deftp
 **/
typedef void (*arg_parse_on_positional)(gint argc,
                                        gchar* argv[],
                                        gpointer user_data);

/**
 ** @deftp{Data Type} {struct arg_parse_funcs}
 ** Holds pointers for the arg_parse callbacks. It is okay for unused
 ** callbacks to be @code{NULL}.
 **
 ** @deftypeivar{struct arg_parse_funcs} arg_parse_on_flag on_flag
 ** Callback for when a flag is parsed.
 ** @end deftypeivar
 ** @deftypeivar{struct arg_parse_funcs} arg_parse_on_option on_option
 ** Callback for when an option is parsed.
 ** @end deftypeivar
 ** @deftypeivar{struct arg_parse_funcs} arg_parse_on_positional on_positional
 ** Callback to handle any remaining posistional arguments.
 ** @end deftypeivar
 ** @end deftp
 **/
struct arg_parse_funcs {
  arg_parse_on_flag on_flag;
  arg_parse_on_option on_option;
  arg_parse_on_positional on_positional;
};

/**
 ** @deftypefun void arg_parse                     @
 **   (gint                          @var{argc},   @
 **    gchar*                        @var{argv}[], @
 **    const struct arg_parse_funcs* @var{funcs},  @
 **    gpointer                      @var{user_data})
 ** Parse command line arguments. Call the callbacks in @var{funcs} as
 ** parsing progresses. @var{user_data} is not directly used in this
 ** function, but passed to each callback unmodified.
 **
 ** When passing @var{argc} and @var{argv} from @code{main()}, be sure
 ** to pass @code{argc - 1} and @code{argv + 1}, as it doesn't
 ** understand the value of @code{argv[0]} passed to @code{main()}.
 ** @end deftypefun
 **/
void arg_parse(gint argc,
               gchar* argv[],
               const struct arg_parse_funcs* funcs,
               gpointer user_data);

#endif
