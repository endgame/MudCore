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
#include "io.h"

#include <sys/time.h>
#include <zmq.h>

#include "descriptor.h"
#include "log.h"
#include "lua_timer.h"
#include "lua_zmq.h"
#include "options.h"
#include "socket.h"
#include "timeval.h"

static gboolean shutdown = FALSE;

/* Process any revents on the server fd. Adjust *pollitems if
   necessary. */
static void io_handle_server(zmq_pollitem_t* server_item,
                             gint* pollitems) {
  if (server_item->revents != 0) {
    if (server_item->revents & ZMQ_POLLERR) {
      FATAL("Server socket in error state!");
    }

    if (server_item->revents & ZMQ_POLLIN) {
      gint new_fd;
      while ((new_fd = socket_accept(server_item->fd)) != -1) {
        DEBUG("New FD: %d", new_fd);
        descriptor_new_fd(new_fd);
      }
    }
    (*pollitems)--;
  }
}

void io_mainloop(gint server) {
  INFO("Entering main loop.");

  GArray* pollitems = g_array_new(FALSE, FALSE, sizeof(zmq_pollitem_t));
  const zmq_pollitem_t server_item = {
    .socket = NULL,
    .fd = server,
    .events = ZMQ_POLLIN | ZMQ_POLLERR,
    .revents = 0
  };

  const gint pulse_length = options_pulse_length();
  while (!shutdown) {
    struct timeval start;
    gettimeofday(&start, NULL);

    lua_timer_execute(&start);

    descriptor_remove_closed();
    lua_timer_remove_dead();
    lua_zmq_remove_unwatched();
    g_array_set_size(pollitems, 0);
    g_array_append_val(pollitems, server_item);
    descriptor_add_pollitems(pollitems);
    lua_zmq_add_pollitems(pollitems);

    /* Poll & process sockets. */
    gint poll_count = zmq_poll((zmq_pollitem_t*)pollitems->data,
                               pollitems->len,
                               pulse_length);
    io_handle_server(&g_array_index(pollitems,
                                    zmq_pollitem_t,
                                    0),
                     &poll_count);
    descriptor_handle_pollitems(pollitems, &poll_count);
    lua_zmq_handle_pollitems(pollitems, &poll_count);
    if (poll_count != 0) WARN("Skipped %d poll items.", poll_count);
    descriptor_handle_commands(&start);
    descriptor_send_prompts();

    /* Sleep out remaining time on this pulse. */
    struct timeval remain = {
      .tv_sec = 0,
      .tv_usec = pulse_length
    };
    timeval_add(&remain, &start);

    struct timeval now;
    gettimeofday(&now, NULL);
    timeval_sub(&remain, &now);

    struct timespec to_sleep = {
      .tv_sec = remain.tv_sec,
      .tv_nsec = remain.tv_usec * 1000
    };

    while (nanosleep(&to_sleep, &to_sleep) == -1) {
      if (errno == EINTR) continue;
      break;
    }
  }

  g_array_free(pollitems, TRUE);
}

void io_shutdown(void) {
  shutdown = TRUE;
}
