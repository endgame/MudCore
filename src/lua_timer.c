#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "lua_timer.h"

#include <lauxlib.h>

#include "log.h"
#include "lua_api.h"
#include "timeval.h"

struct timer_entry {
  gint func; /* Ref from luaL_ref. */
  struct timeval time;
};

static void timer_entry_destroy(gpointer t) {
  struct timer_entry* timer = t;
  luaL_unref(lua_api_get(), LUA_REGISTRYINDEX, timer->func);
  g_free(timer);
}

static GHashTable* /* of gint (ref to func) -> struct timer_entry* */ timers;

/* Iterate through the timer table. Iter names the iterator, Value the
   struct timer_entry pointer. */
#define TIMER_FOREACH(Iter, Value)                                      \
  GHashTableIter Iter;                                                  \
  g_hash_table_iter_init(&iter, timers);                                \
  struct timer_entry* Value;                                            \
  while (g_hash_table_iter_next(&Iter, NULL, (gpointer*)&Value))

static gint lua_timer_cancel(lua_State* lua) {
  gint token = luaL_checkint(lua, 1);
  g_hash_table_remove(timers, GINT_TO_POINTER(token));
  return 0;
}

static gint lua_timer_new(lua_State* lua) {
  struct timer_entry* t = g_new(struct timer_entry, 1);
  gettimeofday(&t->time, NULL);
  timeval_add_delay(&t->time, luaL_checknumber(lua, 1));
  t->func = luaL_ref(lua, LUA_REGISTRYINDEX);
  g_hash_table_insert(timers, GINT_TO_POINTER(t->func), t);
  lua_pushinteger(lua, t->func);
  return 1;
}

void lua_timer_init(lua_State* lua) {
  timers = g_hash_table_new_full(g_direct_hash,
                                 g_direct_equal,
                                 NULL,
                                 timer_entry_destroy);

  DEBUG("Creating mud.timer table.");
  lua_getglobal(lua, "mud");
  lua_newtable(lua);
  static const luaL_Reg timer_funcs[] = {
    { "cancel", lua_timer_cancel },
    { "new"   , lua_timer_new    },
    { NULL    , NULL             }
  };
  luaL_register(lua, NULL, timer_funcs);
  lua_setfield(lua, -2, "timer");
  lua_pop(lua, 1);
}

void lua_timer_deinit(void) {
  g_hash_table_unref(timers);
}

void lua_timer_execute(const struct timeval* start) {
  lua_State* lua = lua_api_get();
  lua_Number newdelay;
  TIMER_FOREACH(iter, timer) {
    if (timeval_compare(start, &timer->time) > 0) {
      lua_rawgeti(lua, LUA_REGISTRYINDEX, timer->func);
      if (lua_pcall(lua, 0, 1, 0) != 0) {
        const gchar* what = lua_tostring(lua, -1);
        ERROR("Error in timer callback: %s", what);
      } else if (lua_isnumber(lua, -1)
                 && (newdelay = lua_tonumber(lua, -1)) > 0) {
        timer->time = *start;
        timeval_add_delay(&timer->time, newdelay);
      } else {
        g_hash_table_iter_remove(&iter);
      }
      lua_pop(lua, 1);
    }
  }
}
