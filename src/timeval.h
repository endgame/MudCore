/**
 ** @node timeval.h
 ** @section timeval.h
 **/

#ifndef TIMEVAL_H
#define TIMEVAL_H

#include <glib.h>
#include <sys/time.h>

/**
 ** @deftypefun {struct timeval*} timeval_add @
 **   (struct timeval*       @var{t1},        @
 **    const struct timeval* @var{t2})
 ** Add @var{t2} to @var{t1} and return @var{t1}.
 ** @end deftypefun
 **/
struct timeval* timeval_add(struct timeval* t1, const struct timeval* t2);

/**
 ** @deftypefun {struct timeval*} timeval_add_delay @
 **   (struct timeval* @var{t},                     @
 **    gdouble         @var{delay})
 ** Add @var{delay} (specified in seconds), to @var{t} and return it.
 ** @end deftypefun
 **/
struct timeval* timeval_add_delay(struct timeval* t, gdouble delay);

/**
 ** @deftypefun gint timeval_compare   @
 **   (const struct timeval* @var{t1}, @
 **    const struct timeval* @var{t2})
 ** Return -1 if @var{t1} is before @var{t2}, 0 if they're the same or
 ** 1 if @var{t1} is after @var{t2}.
 ** @end deftypefun
 **/
gint timeval_compare(const struct timeval* t1, const struct timeval* t2);

/**
 ** @deftypefun {struct timeval*} timeval_add @
 **   (struct timeval*       @var{t1},        @
 **    const struct timeval* @var{t2})
 ** Subtract @var{t2} from @var{t1} and return @var{t1}. If @var{t1}
 ** is not after @var{t2}, zero @var{t1} and return it.
 ** @end deftypefun
 **/
struct timeval* timeval_sub(struct timeval* t1, const struct timeval* t2);

#endif
