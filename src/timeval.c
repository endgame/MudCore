#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "timeval.h"

struct timeval* timeval_add(struct timeval* t1, const struct timeval* t2) {
  t1->tv_sec += t2->tv_sec + ((t1->tv_usec + t2->tv_usec) / 1000000);
  t1->tv_usec = (t1->tv_usec + t2->tv_usec) % 1000000;
  return t1;
}

struct timeval* timeval_add_delay(struct timeval* t, gdouble delay) {
  gint sec = delay;
  gint usec = (delay - sec) * 1000000;
  t->tv_sec += sec + (t->tv_usec + usec) / 1000000;
  t->tv_usec =(t->tv_usec + usec) % 1000000;
  return t;
}

gint timeval_compare(const struct timeval* t1, const struct timeval* t2) {
  if (t1->tv_sec < t2->tv_sec) return -1;
  if (t1->tv_sec > t2->tv_sec) return 1;
  if (t1->tv_usec < t2->tv_usec) return -1;
  if (t1->tv_usec > t2->tv_usec) return 1;
  return 0;
}

struct timeval* timeval_sub(struct timeval* t1, const struct timeval* t2) {
  if (timeval_compare(t1, t2) <= 0) {
    t1->tv_sec = 0;
    t1->tv_usec = 0;
  } else {
    t1->tv_sec -= t2->tv_sec;
    if (t1->tv_usec < t2->tv_usec) {
      t1->tv_usec = t2->tv_usec - t1->tv_usec;
      t1->tv_sec--;
    } else {
      t1->tv_usec -= t2->tv_usec;
    }
  }
  return t1;
}
