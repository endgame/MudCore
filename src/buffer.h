/**
 ** @node buffer.h
 ** @section buffer.h
 **/

#ifndef BUFFER_H
#define BUFFER_H

#include <glib.h>

/**
 ** @deftp {Data Type} {struct buffer}
 ** A dequeue-style buffer. Note that this is not a ring buffer, and
 ** that calling @code{buffer_drain()} will @code{memmove(3)} the
 ** remaining data in the buffer forwards.
 **
 ** @deftypeivar{struct buffer} gint size
 ** Capacity of the buffer.
 ** @end deftypeivar
 ** @deftypeivar{struct buffer} gint used
 ** Number of bytes currently used.
 ** @end deftypeivar
 ** @deftypeivar{struct buffer} gchar data[]
 ** The buffered data.
 ** @end deftypeivar
 ** @end deftp
 **/
struct buffer {
  gint size;
  gint used;
  gchar data[];
};

/**
 ** @deftypefun {struct buffer*} buffer_new @
 **   (gint @var{size})
 ** Allocate and return a new buffer that can hold @var{size} bytes of
 ** data. This function will never fail (it aborts on allocation
 ** failure).
 ** @end deftypefun
 **/
struct buffer* buffer_new(gint size);

/**
 ** @deftypefun void buffer_free @
 **   (struct buffer* @var{buffer})
 ** Free memory used by @var{buffer}.
 ** @end deftypefun
 **/
void buffer_free(struct buffer* buffer);

/**
 ** @deftypefun gint buffer_append  @
 **   (struct buffer* @var{buffer}, @
 **    const gchar*   @var{data},   @
 **    gint           @var{size})
 ** Add @var{size} bytes from @var{data} to @var{buffer}. If the
 ** buffer is too full, trailing data is not added. Returns the number
 ** of bytes actually added.
 ** @end deftypefun
 **/
gint buffer_append(struct buffer* buffer, const gchar* data, gint size);

/**
 ** @deftypefun gboolean buffer_append_c @
 **   (struct buffer* @var{buffer},      @
 **    gchar          @var{ch})
 ** Append a single character to @var{buffer}. Return @code{TRUE} if
 ** the character fit in the buffer.
 ** @end deftypefun
 **/
gboolean buffer_append_c(struct buffer* buffer, gchar ch);

/**
 ** @deftypefun void buffer_backspace @
 **   (struct buffer* @var{buffer})
 ** Remove the final character in @var{buffer}. This is a no-op if
 ** @var{buffer} is empty.
 ** @end deftypefun
 **/
void buffer_backspace(struct buffer* buffer);

/**
 ** @deftypefun void buffer_clear @
 **   (struct buffer* @var{buffer})
 ** Remove all data in @var{buffer}.
 ** @end deftypefun
 **/
void buffer_clear(struct buffer* buffer);

/**
 ** @deftypefun void buffer_drain   @
 **   (struct buffer* @var{buffer}, @
 **    gint           @var{size})
 ** Remove the first @var{size} bytes from @var{buffer}.
 ** @end deftypefun
 **/
void buffer_drain(struct buffer* buffer, gint size);

#endif
