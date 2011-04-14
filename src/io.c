#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "io.h"

#include "log.h"

void io_mainloop(lua_State* lua,
                 gint server,
                 gpointer zmq_rep_socket,
                 gpointer zmq_pub_socket) {
  INFO("Entering main loop.");
}
