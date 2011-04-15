#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "queue.h"

#include <glib.h>

static void test_empty_queue(void) {
  struct queue* queue = queue_new(3);
  g_assert(queue_pop_front(queue) == NULL);
  queue_free(queue);
}

static void test_push_full(void) {
  struct queue* queue = queue_new(1);
  queue_push_back(queue, "Hello");
  gboolean status = queue_push_back(queue, "Goodbye");
  g_assert(!status);
  queue_free(queue);
}

static void test_push_one(void) {
  struct queue* queue = queue_new(3);
  gboolean status = queue_push_back(queue, "Hello");
  g_assert(status);
  gchar* str = queue_pop_front(queue);
  g_assert_cmpstr(str, ==, "Hello");
  g_free(str);
  queue_free(queue);
}

static void test_wrap_around(void) {
  struct queue* queue = queue_new(3);
  gboolean status = queue_push_back(queue, "Hello");
  g_assert(status);
  status = queue_push_back(queue, "Goodbye");
  g_assert(status);
  status = queue_push_back(queue, "Hello again");
  g_assert(status);
  status = queue_push_back(queue, "Too full for this");
  g_assert(!status);
  gchar* str = queue_pop_front(queue);
  g_assert_cmpstr(str, ==, "Hello");
  g_free(str);
  status = queue_push_back(queue, "Goodbye again");
  g_assert(status);
  str = queue_pop_front(queue);
  g_assert_cmpstr(str, ==, "Goodbye");
  g_free(str);
  str = queue_pop_front(queue);
  g_assert_cmpstr(str, ==, "Hello again");
  g_free(str);
  str = queue_pop_front(queue);
  g_assert_cmpstr(str, ==, "Goodbye again");
  g_free(str);
  status = queue_push_back(queue, "Last one");
  g_assert(status);
  queue_free(queue);
}

int main(int argc, char *argv[]) {
  g_test_init(&argc, &argv, NULL);
  g_test_add_func("/queue/empty_queue", test_empty_queue);
  g_test_add_func("/queue/push_full"  , test_push_full);
  g_test_add_func("/queue/push_one"   , test_push_one);
  g_test_add_func("/queue/wrap_around", test_wrap_around);
  return g_test_run();
}
