#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "io.h"

#include <zmq.h>

#include "descriptor.h"
#include "log.h"
#include "socket.h"

#define PULSE_LENGTH (100 * 1000) /* In microseconds */

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
    // TODO: Process, hand off to lua.
    FATAL("TODO: message received on ZMQ_REP socket. What do I do?");
    (*pollitems)--;
  }
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

  while (TRUE) {
    // TODO: Mark start time.
    descriptor_remove_closed();

    g_array_set_size(pollitems, 0);
    g_array_append_vals(pollitems, items, sizeof(items) / sizeof(items[0]));
    descriptor_add_pollitems(pollitems);

    /* Poll & process. */
    gint poll_count = zmq_poll((zmq_pollitem_t*)pollitems->data,
                               pollitems->len,
                               PULSE_LENGTH);
    io_handle_servers(&g_array_index(pollitems,
                                     zmq_pollitem_t,
                                     0),
                      &g_array_index(pollitems,
                                     zmq_pollitem_t,
                                     1),
                      &poll_count);
    descriptor_handle_pollitems(pollitems, poll_count);

    // TODO: Mark end time.
    // TODO: execute ticks.
    // TODO: actually count missed ticks.
    descriptor_handle_commands();
    // TODO: sleep out remaining time.
  }

  g_array_free(pollitems, TRUE);
}
