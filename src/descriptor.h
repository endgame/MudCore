/* MudCore - a simple, lua-scripted MUD server
 * Copyright (C) 2011, 2012  Jack Kelly <jack@jackkelly.name>
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
 ** @node descriptor.h
 ** @section descriptor.h
 **
 ** These are the functions that manipulate descriptors (which
 ** encapsulate the real fd that socket operations use), along with
 ** the table of all client descriptors.
 **/

#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <glib.h>
#include <libtelnet.h>
#include <sys/time.h>
#include <zmq.h>

struct buffer;
struct queue;

/**
 ** @deftp {Data Type} {enum descriptor_state}
 ** Possible states of a descriptor.
 ** @end deftp
 **
 ** @deftp {enum descriptor_state} DESCRIPTOR_STATE_OPEN
 ** Descriptor is able to read and write.
 ** @end deftp
 **
 ** @deftp {enum descriptor_state} DESCRIPTOR_STATE_DELAYING
 ** Descriptor is being delayed: it can be written to but not read
 ** from until after the descriptor's delay_end.
 ** @end deftp
 **
 ** @deftp {enum descriptor_state} DESCRIPTOR_STATE_DRAINING
 ** Descriptor is unable to read, and will close after emptying its
 ** output buffer.
 ** @end deftp
 **
 ** @deftp {enum descriptor_state} DESCRIPTOR_STATE_CLOSED
 ** Descriptor is unable to read or write, and will be removed by
 ** @code{descriptor_remove_closed()}.
 ** @end deftp
 **/
enum descriptor_state {
  DESCRIPTOR_STATE_OPEN,
  DESCRIPTOR_STATE_DELAYING,
  DESCRIPTOR_STATE_DRAINING,
  DESCRIPTOR_STATE_CLOSED
};

/**
 ** @deftp {Data Type} {struct descriptor}
 ** Encapsulates a file descriptor, handles telnet and command buffering.
 **
 ** @deftypeivar {struct descriptor} {enum descriptor_state} state
 ** The descriptor's state.
 ** @end deftypeivar
 ** @deftypeivar {struct descriptor} gint fd
 ** The underlying file descriptor.
 ** @end deftypeivar
 ** @deftypeivar {struct descriptor} telnet_t* telnet
 ** The telnet black-box state tracker.
 ** @end deftypeivar
 ** @deftypeivar {struct descriptor} gint extra_data_ref
 ** Reference to this descriptor's extra_data (a lua table).
 ** @end deftypeivar
 ** @deftypeivar {struct descriptor} gint fd_ref
 ** Reference to this descriptor's FD userdatum.
 ** @end deftypeivar
 ** @deftypeivar {struct descriptor} gint thread_ref
 ** Reference to this descriptor's lua thread.
 ** @end deftypeivar
 ** @deftypeivar {struct descriptor} gboolean skip_until_newline
 ** If the line buffer fills up, excess input until the next newline
 ** is discarded.
 ** @end deftypeivar
 ** @deftypeivar {struct descriptor} gboolean needs_prompt
 ** After output has beend sent to the descriptor, it will need to
 ** send a fresh prompt.
 ** @end deftypeivar
 ** @deftypeivar {struct descriptor} gboolean needs_newline
 ** A newline needs to be sent before fresh output if a complete
 ** command wasn't entered.
 ** @end deftypeivar
 ** @deftypeivar {struct descriptor} {struct timeval} delay_end
 ** If non-zero, the thread will not be awoken until after this time.
 ** @end deftypeivar
 ** @deftypeivar {struct descriptor} {struct buffer*} line_buffer
 ** Buffer for the current line under assembly.
 ** @end deftypeivar
 ** @deftypeivar {struct descriptor} {struct buffer*} output_buffer
 ** Buffer for data waiting to be sent on the socket. This data has
 ** already passed through @code{telnet_send()}.
 ** @end deftypeivar
 ** @deftypeivar {struct descriptor} {struct queue*} command_queue
 ** Queue of completed input commands waiting to be processed by Lua
 ** code.
 ** @end deftypeivar
 ** @end deftp
 **/
struct descriptor {
  enum descriptor_state state;
  gint fd;
  telnet_t* telnet;
  gint extra_data_ref;
  gint fd_ref;
  gint thread_ref;
  gboolean skip_until_newline;
  gboolean needs_prompt;
  gboolean needs_newline;
  struct timeval delay_end;
  struct buffer* line_buffer;
  struct buffer* output_buffer;
  struct queue* command_queue;
};

/**
 ** @deftypefun void descriptor_init @
 **   (void)
 ** @deftypefunx void descriptor_deinit @
 **   (void)
 ** Initialise/clean up the descriptor table.
 ** @end deftypefun
 **/
void descriptor_init(void);
void descriptor_deinit(void);

/**
 ** @deftypefun void descriptor_new_fd @
 **   (gint @var{fd})
 ** Create a new @code{struct descriptor} for @var{fd}, and store it
 ** in the descriptor table.
 ** @end deftypefun
 **/
void descriptor_new_fd(gint fd);

/**
 ** @deftypefun void descriptor_remove_closed @
 **   (void)
 ** Remove any closed descriptors from the descriptor table.
 ** @end deftypefun
 **/
void descriptor_remove_closed(void);

/**
 ** @deftypefun void descriptor_add_pollitems @
 **   (GArray* @var{pollitems})
 ** Add a pollitem to @var{pollitems} (a @code{GArray*} of
 ** @code{zmq_pollitem_t}) for every descriptor in the global table
 ** that needs to do some I/O.
 ** @end deftypefun
 **/
void descriptor_add_pollitems(GArray* /* of zmq_pollitem_t */ pollitems);

/**
 ** @deftypefun void descriptor_handle_pollitems @
 **   (GArray* @var{pollitems},                  @
 **    gint*   @var{poll_count})
 ** Process up to @var{poll_count} items from @var{pollitems} (a
 ** @code{GArray*} of @code{zmq_pollitem_t}). Adjust @var{poll_count}
 ** to reflect the number of items processed.
 ** @end deftypefun
 **/
void descriptor_handle_pollitems(GArray* /* of zmq_pollitem_t */ pollitems,
                                 gint* poll_count);

/**
 ** @deftypefun void descriptor_handle_commands @
 **   ()
 ** Process one command on all open descriptors that have pending
 ** commands.
 ** @end deftypefun
 **/
void descriptor_handle_commands(void);

/**
 ** @deftypefun void descriptor_handle_delays @
 **   (struct timeval* @var{start})
 ** Move any descriptors that are finished delaying into the open
 ** state and awaken their thread.
 ** @end deftypefun
 **/
void descriptor_handle_delays(const struct timeval* start);

/**
 ** @deftypefun void descriptor_send_prompt @
 **   (void)
 ** Send a fresh prompt to any descriptors that need one.
 ** @end deftypefun
 **/
void descriptor_send_prompts(void);

/**
 ** @deftypefun {struct descriptor*} descriptor_get @
 **   (gint @var{fd})
 ** Look up a descriptor in the table, or return @code{NULL} if it
 ** doesn't exist.
 ** @end deftypefun
 **/
struct descriptor* descriptor_get(gint fd);

/**
 ** @deftypefun void descriptor_append      @
 **   (struct descriptor* @var{descriptor}, @
 **    const gchar*       @var{msg})
 ** Queue @var{msg} (a null-terminated string) for output on
 ** @var{descriptor} (escaping telnet IACs and so on).
 ** @end deftypefun
 **/
void descriptor_append(struct descriptor* descriptor, const gchar* msg);

/**
 ** @deftypefun void descriptor_close @
 **   (struct descriptor* @var{descriptor})
 ** Close @var{descriptor}, immediately.
 ** @end deftypefun
 **/
void descriptor_close(struct descriptor* descriptor);

/**
 ** @deftypefun void descriptor_drain @
 **   (struct descriptor* @var{descriptor})
 ** Put @var{descriptor} into the draining state: it will send output,
 ** but process no more commands. Once the output buffer is empty,
 ** @var{descriptor} will close.
 ** @end deftypefun
 **/
void descriptor_drain(struct descriptor* descriptor);

/**
 ** @deftypefun void descriptor_will_echo   @
 **   (struct descriptor* @var{descriptor}, @
 **    gboolean           @var{will})
 ** Starts negotiating the telnet ECHO option. Turning it on but
 ** echoing nothing is common when doing password entry.
 ** @end deftypefun
 **/
void descriptor_will_echo(struct descriptor* descriptor, gboolean will);

#endif
