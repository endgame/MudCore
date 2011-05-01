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
