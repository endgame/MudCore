#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "buffer.h"

#include <string.h>

#include "log.h"

struct buffer* buffer_new(gint size) {
  struct buffer* rv = g_malloc(sizeof(*rv) + size * sizeof(rv->data[0]));
  rv->size = size;
  rv->used = 0;
  return rv;
}

void buffer_free(struct buffer* buffer) {
  g_free(buffer);
}

gint buffer_append(struct buffer* buffer, const gchar* data, gint size) {
  gint available = buffer->size - buffer->used;
  if (size > available) {
    memcpy(buffer->data + buffer->used, data, available);
    buffer->used = buffer->size;
    return available;
  } else {
    memcpy(buffer->data + buffer->used, data, size);
    buffer->used += size;
    return size;
  }
}

gboolean buffer_append_c(struct buffer* buffer, gchar ch) {
  if (buffer->size == buffer->used) return FALSE;
  buffer->data[buffer->used++] = ch;
  return TRUE;
}

void buffer_backspace(struct buffer* buffer) {
  if (buffer->used > 0) buffer->used--;
}

void buffer_clear(struct buffer* buffer) {
  buffer->used = 0;
}

void buffer_drain(struct buffer* buffer, gint size) {
  if (size > buffer->used) {
    ERROR("Draining more than currently in buffer! (%d > %d)",
          size,
          buffer->used);
    buffer->used = 0;
  } else {
    buffer->used -= size;
    memmove(buffer->data, buffer->data + size, buffer->used);
  }
}
