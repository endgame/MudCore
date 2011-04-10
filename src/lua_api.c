#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "lua_api.h"

#include <glib.h>
#include <lualib.h>

#include "lua_args.h"
#include "lua_log.h"

static gpointer lua_glib_alloc(gpointer ud,
                               gpointer ptr,
                               gsize osize,
                               gsize nsize) {
  (void) ud; /* not used */
  (void) osize; /* not used */
  if (nsize == 0) {
    g_free(ptr);
    return NULL;
  } else {
    return g_realloc(ptr, nsize);
  }
}

lua_State* lua_api_init(gint argc, gchar* argv[]) {
  lua_State* lua = lua_newstate(lua_glib_alloc, NULL);
  luaL_openlibs(lua);
  lua_newtable(lua);
  lua_setglobal(lua, "mud");
  lua_args_init(lua, argc, argv);
  lua_log_init(lua);
  return lua;
}
