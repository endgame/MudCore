#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "descriptor.h"

#include <lauxlib.h>
#include <string.h>
#include <sys/socket.h>

#include "buffer.h"
#include "log.h"
#include "lua_api.h"
#include "lua_descriptor.h"
#include "queue.h"
#include "socket.h"
#include "timeval.h"

#define COMMAND_QUEUE_SIZE 10
#define LINE_BUFFER_SIZE 512
#define OUTPUT_BUFFER_SIZE 4096
#define RECV_BUFFER_SIZE 512

/* Encapsulated table of all client descriptors. */
static GHashTable* /* of int -> struct descriptor* */ descriptors;

/* Iterate through the descriptor table. Iter names the iterator,
   Value the descriptor pointer. */
#define DESCRIPTOR_FOREACH(Iter, Value)                                 \
  GHashTableIter Iter;                                                  \
  g_hash_table_iter_init(&Iter, descriptors);                           \
  struct descriptor* Value;                                             \
  while (g_hash_table_iter_next(&Iter, NULL, (gpointer*)&Value))

/* Close and deallocate a descriptor. */
static void descriptor_destroy(gpointer user_data) {
  struct descriptor* descriptor = user_data;
  descriptor_close(descriptor);
  lua_State* lua = lua_api_get();
  luaL_unref(lua, LUA_REGISTRYINDEX, descriptor->fd_ref);
  luaL_unref(lua, LUA_REGISTRYINDEX, descriptor->thread_ref);
  telnet_free(descriptor->telnet);
  buffer_free(descriptor->line_buffer);
  buffer_free(descriptor->output_buffer);
  queue_free(descriptor->command_queue);
  g_free(descriptor);
}

/* Do we want to try receiving data? */
static gboolean descriptor_should_recv(const struct descriptor* descriptor) {
  return descriptor->state == DESCRIPTOR_STATE_OPEN;
}

/* Do we want to send data on the descriptor? */
static gboolean descriptor_should_send(const struct descriptor* descriptor) {
  return (descriptor->state != DESCRIPTOR_STATE_CLOSED
          && descriptor->output_buffer->used > 0);
}

/* Send as much data from the output buffer as possible. */
static void descriptor_do_send(struct descriptor* descriptor) {
  while (descriptor_should_send(descriptor)) {
    gssize count = send(descriptor->fd,
                        descriptor->output_buffer->data,
                        descriptor->output_buffer->used,
                        0);

    if (count == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) return;
      if (errno == EINTR) continue;
      if (errno == ECONNRESET) {
        descriptor_close(descriptor);
        return;
      }

      PERROR("descriptor_do_send(send)");
      descriptor_close(descriptor);
      return;
    }

    DEBUG("FD %d: Sent %d bytes.", descriptor->fd, count);
    buffer_drain(descriptor->output_buffer, count);
  }
}

/* Read as much as possible from the socket, passing it to libtelnet. */
static void descriptor_do_recv(struct descriptor* descriptor) {
  gchar buf[RECV_BUFFER_SIZE];
  while (descriptor_should_recv(descriptor)) {
    gssize count = recv(descriptor->fd, buf, sizeof(buf), 0);
    if (count == 0) {
      descriptor_close(descriptor);
      return;
    }

    if (count == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) return;
      if (errno == EINTR) continue;
      PERROR("descriptor_do_recv(recv)");
      descriptor_close(descriptor);
      return;
    }

    telnet_recv(descriptor->telnet, buf, count);
  }
}

/* Add data to the output buffer, flushing to the socket if necessary. */
static void descriptor_buffer_output(struct descriptor* descriptor,
                                     const gchar* data,
                                     gsize len) {
  guint added = buffer_append(descriptor->output_buffer, data, len);
  while (added < len) {
    descriptor_do_send(descriptor);
    if (!descriptor_should_send(descriptor)) break;
    added += buffer_append(descriptor->output_buffer,
                           data + added,
                           len - added);
  }
}

/* Add data to the line buffer, enqueuing commands as complete lines
   are parsed. */
static void descriptor_handle_input(struct descriptor* descriptor,
                                    const gchar* data,
                                    gsize len) {
  for (guint i = 0; i < len; i++) {
    if (data[i] == '\r') continue;
    if (data[i] == '\n') {
      descriptor->needs_newline = FALSE;
      descriptor->skip_until_newline = FALSE;
      if (!queue_push_back_len(descriptor->command_queue,
                               descriptor->line_buffer->data,
                               descriptor->line_buffer->used)) {
        descriptor_append(descriptor,
                          "Input queue full. Command discarded.\r\n");
      }
      buffer_clear(descriptor->line_buffer);
      continue;
    }
    if (descriptor->skip_until_newline) continue;
    if (data[i] == '\b') {
      buffer_backspace(descriptor->line_buffer);
    } else if (!buffer_append_c(descriptor->line_buffer, data[i])) {
      descriptor_append(descriptor,
                        "Input line too long. Command truncated.\r\n");
      descriptor->skip_until_newline = TRUE;
    }
  }
}

/* Callback from libtelnet when something interesting happens. */
static void descriptor_on_telnet_event(telnet_t* telnet,
                                       telnet_event_t* event,
                                       gpointer user_data) {
  (void) telnet; /* Not used. */
  struct descriptor* descriptor = user_data;
  switch (event->type) {
  case TELNET_EV_DATA:
    descriptor_handle_input(descriptor, event->data.buffer, event->data.size);
    break;
  case TELNET_EV_SEND:
    descriptor_buffer_output(descriptor, event->data.buffer, event->data.size);
    break;
  case TELNET_EV_DO:
    switch (event->neg.telopt) {
    case TELNET_TELOPT_COMPRESS2:
      telnet_begin_compress2(descriptor->telnet);
      break;
    case TELNET_TELOPT_ECHO:
      descriptor->will_echo = TRUE;
      break;
    }
    break;
  case TELNET_EV_DONT:
    if (event->neg.telopt == TELNET_TELOPT_ECHO) descriptor->will_echo = FALSE;
    break;
  case TELNET_EV_WARNING:
    WARN("libtelnet warning: %s", event->error.msg);
    break;
  case TELNET_EV_ERROR:
    ERROR("libtelnet error: %s", event->error.msg);
    descriptor_close(descriptor);
    break;
  default:
    break;
  }
}

/* Send a fresh prompt (from Lua), followed by IAC GA. */
static void descriptor_send_prompt(struct descriptor* descriptor) {
  lua_State* lua = lua_api_get();
  /* Call mud.descriptor.send_prompt. */
  lua_getglobal(lua, "mud");
  lua_getfield(lua, -1, "descriptor");
  lua_remove(lua, -2);
  lua_getfield(lua, -1, "send_prompt");
  lua_remove(lua, -2);
  lua_rawgeti(lua, LUA_REGISTRYINDEX, descriptor->fd_ref);
  if (lua_pcall(lua, 1, 0, 0) != 0) {
    const gchar* what = lua_tostring(lua, -1);
    ERROR("Error in mud.descriptor.send_prompt: %s", what);
    lua_pop(lua, 1);
    descriptor_close(descriptor);
  }
  descriptor->needs_prompt = FALSE;
  descriptor->needs_newline = TRUE;
  telnet_iac(descriptor->telnet, TELNET_GA);
}

void descriptor_init(void) {
  descriptors = g_hash_table_new_full(g_direct_hash,
                                      g_direct_equal,
                                      NULL,
                                      descriptor_destroy);
}

void descriptor_deinit(void) {
  g_hash_table_unref(descriptors);
}

void descriptor_new_fd(gint fd) {
  struct descriptor* descriptor = g_new(struct descriptor, 1);
  descriptor->state = DESCRIPTOR_STATE_OPEN;
  descriptor->fd = fd;

  static const telnet_telopt_t telopts[] = {
    { TELNET_TELOPT_ECHO     , TELNET_WILL, TELNET_DONT },
    { TELNET_TELOPT_COMPRESS2, TELNET_WILL, TELNET_DONT },
    {                      -1,           0,           0 }
  };
  descriptor->telnet = telnet_init(telopts,
                                   descriptor_on_telnet_event,
                                   0,
                                   descriptor);
  if (descriptor->telnet == NULL) {
    ERROR("Failed to create telnet state tracker.");
    g_free(descriptor);
    return;
  }

  descriptor->fd_ref = LUA_NOREF;
  descriptor->thread_ref = LUA_NOREF;
  descriptor->skip_until_newline = FALSE;
  descriptor->needs_prompt = TRUE;
  descriptor->needs_newline = FALSE;
  descriptor->will_echo = FALSE;
  descriptor->next_command.tv_sec = 0;
  descriptor->next_command.tv_usec = 0;
  descriptor->line_buffer = buffer_new(LINE_BUFFER_SIZE);
  descriptor->output_buffer = buffer_new(OUTPUT_BUFFER_SIZE);
  descriptor->command_queue = queue_new(COMMAND_QUEUE_SIZE);
  g_hash_table_insert(descriptors, GINT_TO_POINTER(fd), descriptor);

  /* Offer supported telnet options. */
  telnet_negotiate(descriptor->telnet, TELNET_WILL, TELNET_TELOPT_COMPRESS2);
  lua_descriptor_start(descriptor);
}

void descriptor_remove_closed(void) {
  DESCRIPTOR_FOREACH(iter, descriptor) {
    if (descriptor->state == DESCRIPTOR_STATE_DRAINING
        && descriptor->output_buffer->used == 0) {
      descriptor_close(descriptor);
    }

    if (descriptor->state == DESCRIPTOR_STATE_CLOSED) {
      g_hash_table_iter_remove(&iter);
    }
  }
}

void descriptor_add_pollitems(GArray* /* of zmq_pollitem_t */ pollitems) {
  zmq_pollitem_t pollitem;
  memset(&pollitem, 0, sizeof(pollitem));
  DESCRIPTOR_FOREACH(iter, descriptor) {
    pollitem.fd = descriptor->fd;
    pollitem.events = ZMQ_POLLERR;
    if (descriptor_should_recv(descriptor)) pollitem.events |= ZMQ_POLLIN;
    if (descriptor_should_send(descriptor)) pollitem.events |= ZMQ_POLLOUT;
    g_array_append_val(pollitems, pollitem);
  }
}

void descriptor_handle_pollitems(GArray* /* of zmq_pollitem_t */ pollitems,
                                 gint poll_count) {
  for (guint i = 0; i < pollitems->len; i++) {
    if (poll_count == 0) break;
    zmq_pollitem_t* pollitem = &g_array_index(pollitems, zmq_pollitem_t, i);
    if (pollitem->socket != NULL) continue;

    struct descriptor* descriptor = descriptor_get(pollitem->fd);
    if (descriptor == NULL) continue; /* Could be the server fd. */

    if (pollitem->revents != 0) poll_count--;

    if (pollitem->revents & ZMQ_POLLERR) {
      ERROR("FD %d in error state. Closing", pollitem->fd);
      descriptor_close(descriptor);
      continue;
    }
    if (pollitem->revents & ZMQ_POLLOUT) descriptor_do_send(descriptor);
    if (pollitem->revents & ZMQ_POLLIN)  descriptor_do_recv(descriptor);
  }
}

void descriptor_handle_commands(const struct timeval* start) {
  DESCRIPTOR_FOREACH(iter, descriptor) {
    if (descriptor->state == DESCRIPTOR_STATE_OPEN
        && descriptor->command_queue->used > 0
        && timeval_compare(start, &descriptor->next_command) > 0) {
      descriptor->needs_prompt = TRUE;
      gchar* command = queue_pop_front(descriptor->command_queue);
      lua_descriptor_resume(descriptor, command);
      g_free(command);
    }
  }
}

void descriptor_send_prompts(void) {
  DESCRIPTOR_FOREACH(iter, descriptor) {
    if (descriptor->needs_prompt
        && descriptor->state == DESCRIPTOR_STATE_OPEN) {
      descriptor_send_prompt(descriptor);
    }
  }
}

struct descriptor* descriptor_get(gint fd) {
  return g_hash_table_lookup(descriptors, GINT_TO_POINTER(fd));
}

void descriptor_append(struct descriptor* descriptor, const gchar* msg) {
  if (descriptor->needs_newline) {
    telnet_send(descriptor->telnet, "\r\n", 2);
    descriptor->needs_newline = FALSE;
  }
  telnet_send(descriptor->telnet, msg, strlen(msg));
  descriptor->needs_prompt = TRUE;
}

void descriptor_close(struct descriptor* descriptor) {
  if (descriptor->state == DESCRIPTOR_STATE_CLOSED) return;
  lua_State* lua = lua_api_get();
  lua_getglobal(lua, "mud");
  lua_getfield(lua, -1, "descriptor");
  lua_remove(lua, -2);
  lua_getfield(lua, -1, "on_close");
  lua_remove(lua, -2);
  if (lua_isnil(lua, -1)) {
    lua_pop(lua, 1);
  } else {
    lua_pushinteger(lua, descriptor->fd);
    if (lua_pcall(lua, 1, 0, 0) != 0) {
      const gchar* what = lua_tostring(lua, -1);
      ERROR("Error in mud.descriptor.on_close: %s", what);
      lua_pop(lua, 1);
    }
  }
  descriptor->state = DESCRIPTOR_STATE_CLOSED;
  socket_close(descriptor->fd);
}

void descriptor_drain(struct descriptor* descriptor) {
  descriptor->state = DESCRIPTOR_STATE_DRAINING;
}

void descriptor_will_echo(struct descriptor* descriptor, gboolean will) {
  if (will != descriptor->will_echo) {
    telnet_negotiate(descriptor->telnet,
                     will ? TELNET_WILL : TELNET_WONT,
                     TELNET_TELOPT_ECHO);
  }
}
