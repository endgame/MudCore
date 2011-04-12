#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "socket.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "log.h"

#define BACKLOG 5

static gboolean make_nonblocking(gint fd) {
  gint flags = fcntl(fd, F_GETFL);
  if (flags == -1) {
    PWARN("make_nonblocking(fcntl(F_GETFL))");
    return FALSE;
  }

  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    PWARN("make_nonblocking(fcntl(F_SETFL))");
    return FALSE;
  }

  return TRUE;
}

gint socket_server(const gchar* port) {
  gint fd = -1;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  struct addrinfo* infos;
  gint gai_status = getaddrinfo(NULL, port, &hints, &infos);
  if (gai_status != 0) {
    FATAL("socket_server(getaddrinfo): %s", gai_strerror(gai_status));
  }

  for (struct addrinfo* info = infos; info != NULL; info = info->ai_next) {
    if ((fd = socket(info->ai_family,
                     info->ai_socktype,
                     info->ai_protocol)) == -1) {
      PWARN("socket_server(socket)");
      continue;
    }

    gint yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
      PWARN("socket_server(setsockopt)");
      socket_close(fd);
      fd = -1;
      continue;
    }

    if (bind(fd, info->ai_addr, info->ai_addrlen) == -1) {
      PWARN("socket_server(bind)");
      socket_close(fd);
      fd = -1;
      continue;
    }

    if (listen(fd, BACKLOG) == -1) {
      PWARN("socket_server(listen)");
      socket_close(fd);
      fd = -1;
      continue;
    }

    if (!make_nonblocking(fd)) {
      socket_close(fd);
      fd = -1;
      continue;
    }

    break;
  }
  freeaddrinfo(infos);
  if (fd != -1) DEBUG("Socket created: fd %d, bound to port %s.", fd, port);
  return fd;
}

gint socket_accept(gint fd) {
  gint new_fd;
  while ((new_fd = accept(fd, NULL, NULL)) == -1) {
    if (errno == EAGAIN
        || errno == EWOULDBLOCK
        || errno == ECONNABORTED) break;
    if (errno == EINTR) continue;
    PWARN("socket_accept(accept)");
  }

  if (new_fd != -1 && !make_nonblocking(new_fd)) {
    WARN("Couldn't make new socket nonblocking. Closing it.");
    socket_close(new_fd);
    new_fd = -1;
  }
  return new_fd;
}

void socket_close(gint fd) {
  while (close(fd) == -1 && errno == EINTR);
}
