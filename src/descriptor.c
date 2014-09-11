/* MudCore - a simple, lua-scripted MUD server
 * Copyright (C) 2011, 2012  Jack Kelly <jack@jackkelly.name>
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
#include "descriptor.h"

#include <arpa/inet.h>
#include <lauxlib.h>
#include <string.h>
#include <sys/socket.h>

#include "buffer.h"
#include "log.h"
#include "lua_api.h"
#include "lua_descriptor.h"
#include "socket.h"
#include "timeval.h"

#define LINE_BUFFER_SIZE 512
#define OUTPUT_BUFFER_SIZE 4096
#define RECV_BUFFER_SIZE 1024

/* Table of all client descriptors. */
static GHashTable* /* of int(fd) -> struct descriptor* */ descriptors;

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
  luaL_unref(lua, LUA_REGISTRYINDEX, descriptor->extra_data_ref);
  luaL_unref(lua, LUA_REGISTRYINDEX, descriptor->fd_ref);
  luaL_unref(lua, LUA_REGISTRYINDEX, descriptor->thread_ref);
  telnet_free(descriptor->telnet);
  buffer_free(descriptor->line_buffer);
  buffer_free(descriptor->output_buffer);
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

    DEBUG("FD %d: Sent %" G_GSSIZE_FORMAT " bytes.", descriptor->fd, count);
    buffer_drain(descriptor->output_buffer, count);
  }
}

/* Read from the socket, passing it to libtelnet. */
static void descriptor_do_recv(struct descriptor* descriptor) {
  gchar buf[RECV_BUFFER_SIZE];
  gssize count = recv(descriptor->fd, buf, sizeof(buf), 0);
  if (count == 0) {
    descriptor_close(descriptor);
    return;
  }

  if (count == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) return;
    PERROR("descriptor_do_recv(recv)");
    descriptor_close(descriptor);
    return;
  }

  telnet_recv(descriptor->telnet, buf, count);
}

/* Add data to the output buffer, flushing to the socket if necessary. */
static void descriptor_buffer_output(struct descriptor* descriptor,
                                     const gchar* data,
                                     gsize len) {
  if (descriptor->state == DESCRIPTOR_STATE_CLOSED) return;
  guint added = buffer_append(descriptor->output_buffer, data, len);

  /* If we couldn't fit everything in the buffer, try flushing. */
  if (added < len) {
    descriptor_do_send(descriptor);

    added += buffer_append(descriptor->output_buffer,
                           data + added,
                           len - added);

    /* If we're still full, the descriptor's flooded out. Kill it. */
    if (added < len) {
      WARN("FD %d flooded out!", descriptor->fd);
      descriptor_close(descriptor);
    }
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
      lua_descriptor_accept_command(descriptor);
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

/* Switch off NAWS (RFC 1073) and remove the "size" field from the lua
   descriptor. */
static void descriptor_disable_naws(struct descriptor* descriptor) {
  lua_State* lua = lua_api_get();
  lua_rawgeti(lua, LUA_REGISTRYINDEX, descriptor->extra_data_ref);
  lua_pushliteral(lua, "width");
  lua_pushnil(lua);
  lua_rawset(lua, -3);
  lua_pushliteral(lua, "height");
  lua_pushnil(lua);
  lua_rawset(lua, -3);
  lua_pop(lua, 1);
  telnet_negotiate(descriptor->telnet, TELNET_DONT, TELNET_TELOPT_NAWS);
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
    if (event->neg.telopt == TELNET_TELOPT_COMPRESS2) {
      telnet_begin_compress2(descriptor->telnet);
    }
    break;
  case TELNET_EV_WONT:
    if (event->neg.telopt == TELNET_TELOPT_NAWS) {
      descriptor_disable_naws(descriptor);
    }
    break;
  case TELNET_EV_SUBNEGOTIATION:
    if (event->sub.telopt == TELNET_TELOPT_NAWS) {
      if (event->sub.size != 4) {
        WARN("Malformed NAWS option. Disabling NAWS on fd %d", descriptor->fd);
        descriptor_disable_naws(descriptor);
        break;
      }

      lua_State* lua = lua_api_get();
      lua_rawgeti(lua, LUA_REGISTRYINDEX, descriptor->extra_data_ref);
      lua_pushliteral(lua, "width");
      lua_pushinteger(lua, ntohs(*(guint16*)event->sub.buffer));
      lua_rawset(lua, -3);
      lua_pushliteral(lua, "height");
      lua_pushinteger(lua, ntohs(*(guint16*)(event->sub.buffer + 2)));
      lua_rawset(lua, -3);
      lua_pop(lua, 1);
    }
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

/* Return the list of open FDs in random order, to prevent certain
   clients from getting an unfair advantage. g_free() the result. */
static gpointer* descriptor_fds_in_random_order(guint* length) {
  gpointer* fds = g_hash_table_get_keys_as_array(descriptors, length);
  gpointer tmp;

  /* Shuffle. */
  for (guint i = 0; i < *length; i++) {
    guint j = g_random_int_range(i, *length);
    tmp = fds[i];
    fds[i] = fds[j];
    fds[j] = tmp;
  }

  return fds;
}

/* Send a fresh prompt (from Lua), followed by IAC GA. */
static void descriptor_send_prompt(struct descriptor* descriptor) {
  lua_State* lua = lua_api_get();
  lua_rawgeti(lua, LUA_REGISTRYINDEX, descriptor->extra_data_ref);
  lua_getfield(lua, -1, "prompt");
  lua_remove(lua, -2);

  if (lua_type(lua, -1) == LUA_TFUNCTION) {
    lua_rawgeti(lua, LUA_REGISTRYINDEX, descriptor->fd_ref);
    if (lua_pcall(lua, 1, 1, 0) != LUA_OK) {
      const gchar* what = lua_tostring(lua, -1);
      ERROR("Error in prompt callback function: %s", what);
      lua_pop(lua, 1);
      descriptor_close(descriptor);
      return;
    }
  }

  descriptor_append(descriptor, lua_tostring(lua, -1));
  lua_pop(lua, 1);

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

void descriptor_new(gint fd) {
  struct descriptor* descriptor = g_new(struct descriptor, 1);
  descriptor->state = DESCRIPTOR_STATE_OPEN;
  descriptor->fd = fd;

  static const telnet_telopt_t telopts[] = {
    { TELNET_TELOPT_ECHO     , TELNET_WILL, TELNET_DONT },
    { TELNET_TELOPT_COMPRESS2, TELNET_WILL, TELNET_DONT },
    { TELNET_TELOPT_NAWS     , TELNET_WONT, TELNET_DO   },
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

  descriptor->extra_data_ref = LUA_NOREF;
  descriptor->fd_ref = LUA_NOREF;
  descriptor->thread_ref = LUA_NOREF;
  descriptor->skip_until_newline = FALSE;
  descriptor->needs_prompt = TRUE;
  descriptor->needs_newline = FALSE;
  descriptor->self_delayed = FALSE;
  descriptor->delay_end.tv_sec = 0;
  descriptor->delay_end.tv_usec = 0;
  descriptor->line_buffer = buffer_new(LINE_BUFFER_SIZE);
  descriptor->output_buffer = buffer_new(OUTPUT_BUFFER_SIZE);
  g_hash_table_insert(descriptors, GINT_TO_POINTER(fd), descriptor);

  /* Offer supported telnet options. */
  telnet_negotiate(descriptor->telnet, TELNET_WILL, TELNET_TELOPT_COMPRESS2);
  telnet_negotiate(descriptor->telnet, TELNET_DO, TELNET_TELOPT_NAWS);
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
                                 gint* poll_count) {
  for (guint i = 0; i < pollitems->len; i++) {
    if (*poll_count == 0) break;
    zmq_pollitem_t* pollitem = &g_array_index(pollitems, zmq_pollitem_t, i);
    if (pollitem->socket != NULL) continue;

    struct descriptor* descriptor = descriptor_get(pollitem->fd);
    if (descriptor == NULL) continue; /* Could be the server fd, or a
                                         ZeroMQ socket. */

    if (pollitem->revents != 0) (*poll_count)--;
    if (pollitem->revents & ZMQ_POLLERR) {
      ERROR("FD %d in error state. Closing.", pollitem->fd);
      descriptor_close(descriptor);
      continue;
    }
    if (pollitem->revents & ZMQ_POLLOUT) descriptor_do_send(descriptor);
    if (pollitem->revents & ZMQ_POLLIN)  descriptor_do_recv(descriptor);
  }
}

void descriptor_handle_commands(void) {
  guint length;
  gpointer* fds = descriptor_fds_in_random_order(&length);

  for (guint i = 0; i < length; i++) {
    struct descriptor* descriptor = descriptor_get(GPOINTER_TO_INT(fds[i]));
    if (descriptor->state == DESCRIPTOR_STATE_OPEN) {
      lua_descriptor_handle_command(descriptor);
    }
  }
  g_free(fds);
}

void descriptor_handle_delays(const struct timeval* start) {
  DESCRIPTOR_FOREACH(iter, descriptor) {
    if (descriptor->state == DESCRIPTOR_STATE_DELAYING
        && timeval_compare(start, &descriptor->delay_end) > 0) {
      descriptor->state = DESCRIPTOR_STATE_OPEN;
      if (descriptor->self_delayed) {
        descriptor->self_delayed = FALSE;
        lua_descriptor_resume(descriptor, 0);
      }
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
    lua_rawgeti(lua, LUA_REGISTRYINDEX, descriptor->fd_ref);
    if (lua_pcall(lua, 1, 0, 0) != LUA_OK) {
      const gchar* what = lua_tostring(lua, -1);
      ERROR("Error in mud.descriptor.on_close: %s", what);
      lua_pop(lua, 1);
    }
  }
  descriptor->state = DESCRIPTOR_STATE_CLOSED;
  socket_close(descriptor->fd);
}

void descriptor_delay(struct descriptor* descriptor, gdouble delay) {
  if (descriptor == NULL) {
    WARN("Attempting to delay nonexistent descriptor.");
    return;
  }

  if (descriptor->state != DESCRIPTOR_STATE_OPEN
      && descriptor->state != DESCRIPTOR_STATE_DELAYING) return;

  if (delay < 0) {
    WARN("Attempting to delay descriptor by negative amount.");
    return;
  }
  if (descriptor->state == DESCRIPTOR_STATE_OPEN) {
    gettimeofday(&descriptor->delay_end, NULL);
    descriptor->state = DESCRIPTOR_STATE_DELAYING;
  }
  timeval_add_delay(&descriptor->delay_end, delay);
}

void descriptor_drain(struct descriptor* descriptor) {
  descriptor->state = DESCRIPTOR_STATE_DRAINING;
}

void descriptor_will_echo(struct descriptor* descriptor, gboolean will) {
  telnet_negotiate(descriptor->telnet,
                   will ? TELNET_WILL : TELNET_WONT,
                   TELNET_TELOPT_ECHO);
}
