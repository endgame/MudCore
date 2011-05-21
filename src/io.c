#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "io.h"

#include <sys/time.h>
#include <zmq.h>

#include "descriptor.h"
#include "log.h"
#include "lua_zmq.h"
#include "options.h"
#include "socket.h"
#include "timeval.h"

/* Process any revents on the server fd and the ZMQ_REP socket. */
static void io_handle_servers(zmq_pollitem_t* server_item,
                              zmq_pollitem_t* zmq_rep_item,
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

  if (zmq_rep_item->revents != 0) {
    zmq_msg_t msg;
    zmq_msg_init(&msg);
    gint rc = zmq_recv(zmq_rep_item->socket, &msg, ZMQ_NOBLOCK);
    if (rc == -1) {
      if (errno == EAGAIN) return; /* No message? No problem. */
      PERROR("io_handle_servers(zmq_recv)");
      return;
    }

    lua_zmq_on_request(zmq_rep_item->socket,
                       zmq_msg_data(&msg),
                       zmq_msg_size(&msg));
    zmq_msg_close(&msg);
    (*pollitems)--;
  }
}

/* Handle one tick of the server. */
static void io_tick(void) {
  descriptor_handle_commands();
  // TODO: check/call/expire/timers.
}

void io_mainloop(gint server,
                 gpointer zmq_rep_socket) {
  INFO("Entering main loop.");

  GArray* pollitems = g_array_new(FALSE, FALSE, sizeof(zmq_pollitem_t));
  const zmq_pollitem_t items[] = {
    {
      .socket = NULL,
      .fd = server,
      .events = ZMQ_POLLIN | ZMQ_POLLERR,
      .revents = 0
    }, {
      .socket = zmq_rep_socket,
      .fd = 0,
      .events = ZMQ_POLLIN,
      .revents = 0
    }
  };

  const gint pulse_length = options_pulse_length();
  while (TRUE) {
    struct timeval poll_start;
    gettimeofday(&poll_start, NULL);

    descriptor_remove_closed();

    g_array_set_size(pollitems, 0);
    g_array_append_vals(pollitems, items, sizeof(items) / sizeof(items[0]));
    descriptor_add_pollitems(pollitems);

    /* Poll & process. */
    gint poll_count = zmq_poll((zmq_pollitem_t*)pollitems->data,
                               pollitems->len,
                               pulse_length);
    io_handle_servers(&g_array_index(pollitems,
                                     zmq_pollitem_t,
                                     0),
                      &g_array_index(pollitems,
                                     zmq_pollitem_t,
                                     1),
                      &poll_count);
    descriptor_handle_pollitems(pollitems, poll_count);

    struct timeval poll_end;
    gettimeofday(&poll_end, NULL);

    /* Count and execute any missed ticks. */
    struct timeval delta = poll_end;
    timeval_sub(&delta, &poll_start);
    const struct timeval pulse = {
      .tv_sec = 0,
      .tv_usec = pulse_length
    };
    gint missed_ticks = 0;
    while (timeval_compare(&delta, &pulse) > 1) {
      timeval_sub(&delta, &pulse);
      missed_ticks++;
    }
    if (missed_ticks > 0) WARN("Missed %d ticks.", missed_ticks);
    for (gint i = 0; i < missed_ticks; i++) io_tick();

    io_tick();
    descriptor_send_prompts();

    /* Sleep out remaining time if we have any to spare. */
    if (missed_ticks == 0) {
      struct timeval remain = pulse;
      timeval_sub(&remain, &delta);
      struct timespec to_sleep = {
        .tv_sec = remain.tv_sec,
        .tv_nsec = remain.tv_usec * 1000
      };

      while (nanosleep(&to_sleep, &to_sleep) == -1) {
        if (errno == EINTR) continue;
        break;
      }
    }
  }

  g_array_free(pollitems, TRUE);
}
