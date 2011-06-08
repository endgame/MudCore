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
 ** @node queue.h
 ** @section queue.h
 **
 ** This is a FIFO queue of strings.
 **/

#ifndef QUEUE_H
#define QUEUE_H

#include <glib.h>

/**
 ** @deftp {Data Type} {struct queue}
 ** A FIFO queue of strings.
 **
 ** @deftypeivar{struct queue} gint size
 ** Capacity of the queue.
 ** @end deftypeivar
 ** @deftypeivar{struct queue} gint used
 ** Number of strings held in the queue.
 ** @end deftypeivar
 ** @deftypeivar{struct queue} gint front
 ** Index of the front of the queue.
 ** @end deftypeivar
 ** @deftypeivar{struct queue} gint back
 ** Index of one-past-the-last element in the queue.
 ** @end deftypeivar
 ** @deftypeivar{struct queue} gchar* strings[]
 ** Array of pointers to the queue elements.
 ** @end deftypeivar
 ** @end deftp
 **/
struct queue {
  gint size;
  gint used;
  gint front;
  gint back;
  gchar* strings[];
};

/**
 ** @deftypefun {struct queue*} queue_new @
 **   (gint @var{size})
 ** Allocate and return a pointer to a queue that can hold @var{size}
 ** strings.
 ** @end deftypefun
 **/
struct queue* queue_new(gint size);

/**
 ** @deftypefun void queue_free @
 **   (struct queue* @var{queue})
 ** Free all the resources associated with @var{queue}(.
 ** @end deftypefun
 **/
void queue_free(struct queue* queue);

/**
 ** @deftypefun gboolean queue_push_back @
 **   (struct queue* @var{queue},        @
 **    const gchar*  @var{string})
 ** Equivalent to @code{queue_push_back_len(queue, string,
 ** strlen(string))}.
 ** @end deftypefun
 **/
gboolean queue_push_back(struct queue* queue, const gchar* string);

/**
 ** @deftypefun gboolean queue_push_back_len @
 **   (struct queue* @var{queue},            @
 **    const gchar*  @var{string},           @
 **    gint          @var{len})
 ** Attempt to add @var{string} of length @var{len} to the end of
 ** @var{queue}. Return @code{TRUE} iff there was room.
 ** @end deftypefun
 **/
gboolean queue_push_back_len(struct queue* queue,
                             const gchar* string,
                             gint len);

/**
 ** @deftypefun gchar* queue_pop_front @
 **   (struct queue* @var{queue})
 ** Return the front element from the queue. The caller owns the
 ** returned string and must @code{g_free()} it.
 ** @end deftypefun
 **/
gchar* queue_pop_front(struct queue* queue);

#endif
