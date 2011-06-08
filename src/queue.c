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
#include "queue.h"

#include <string.h>

struct queue* queue_new(gint size) {
  struct queue* rv = g_malloc(sizeof(*rv) + size * sizeof(rv->strings[0]));
  rv->size = size;
  rv->used = 0;
  rv->front = 0;
  rv->back = 0;
  return rv;
}

void queue_free(struct queue* queue) {
  while (queue->used > 0) g_free(queue_pop_front(queue));
  g_free(queue);
}

gboolean queue_push_back(struct queue* queue, const gchar* string) {
  return queue_push_back_len(queue, string, strlen(string));
}

gboolean queue_push_back_len(struct queue* queue,
                             const gchar* string,
                             gint len) {
  if (queue->used == queue->size) return FALSE;
  queue->strings[queue->back] = g_strndup(string, len);
  queue->back = (queue->back + 1) % queue->size;
  queue->used++;
  return TRUE;
}

gchar* queue_pop_front(struct queue* queue) {
  if (queue->used == 0) return NULL;
  gchar* rv = queue->strings[queue->front];
  queue->front = (queue->front + 1) % queue->size;
  queue->used--;
  return rv;
}
