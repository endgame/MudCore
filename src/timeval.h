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
 ** @deftypefun {struct timeval*} timeval_sub @
 **   (struct timeval*       @var{t1},        @
 **    const struct timeval* @var{t2})
 ** Subtract @var{t2} from @var{t1} and return @var{t1}. If @var{t1}
 ** is not after @var{t2}, zero @var{t1} and return it.
 ** @end deftypefun
 **/
struct timeval* timeval_sub(struct timeval* t1, const struct timeval* t2);

#endif
