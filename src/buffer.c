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
