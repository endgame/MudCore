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
 ** @deftypeivar {struct descriptor} gboolean will_echo
 ** Tracks whether or not the descriptor is doing local echo. Note
 ** that echoing doesn't happen automatically, so that things like
 ** password input work correctly. User code should echo what is deems
 ** necessary.
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
  gint thread_ref;
  gboolean skip_until_newline;
  gboolean needs_prompt;
  gboolean will_echo;
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
 **
 ** Add a pollitem to @var{pollitems} (a @code{GArray*} of
 ** @code{zmq_pollitem_t}) for every descriptor in the global table
 ** that needs to do some I/O.
 ** @end deftypefun
 **/
void descriptor_add_pollitems(GArray* /* of zmq_pollitem_t */ pollitems);

/**
 ** @deftypefun void descriptor_handle_pollitems @
 **   (GArray* @var{pollitems},                  @
 **    gint    @var{poll_count})
 ** Process up to @var{poll_count} items from @var{pollitems} (a
 ** @code{GArray*} of @code{zmq_pollitem_t}).
 ** @end deftypefun
 **/
void descriptor_handle_pollitems(GArray* /* of zmq_pollitem_t */ pollitems,
                                 gint poll_count);

/**
 ** @deftypefun void descriptor_handle_commands @
 **   (void)
 ** Process one command on all descriptors that have pending commands.
 ** @end deftypefun
 **/
void descriptor_handle_commands(void);

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
