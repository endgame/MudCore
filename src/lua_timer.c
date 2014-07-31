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
#include "lua_timer.h"

#include <lauxlib.h>

#include "log.h"
#include "lua_api.h"
#include "timeval.h"

#define TIMER_TYPE "mudcore.timer"

struct timer_entry {
  gboolean dead;
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
  gint token = *(gint*)luaL_checkudata(lua, 1, TIMER_TYPE);
  struct timer_entry* timer = g_hash_table_lookup(timers,
                                                  GINT_TO_POINTER(token));
  if (timer == NULL) {
    return luaL_error(lua, "timer:cancel(): Call to an expired timer.");
  }
  timer->dead = TRUE;
  return 0;
}

static gint lua_timer_new(lua_State* lua) {
  struct timer_entry* t = g_new(struct timer_entry, 1);
  t->dead = FALSE;
  gettimeofday(&t->time, NULL);
  timeval_add_delay(&t->time, luaL_checknumber(lua, 1));
  t->func = luaL_ref(lua, LUA_REGISTRYINDEX);
  g_hash_table_insert(timers, GINT_TO_POINTER(t->func), t);

  gint* token = lua_newuserdata(lua, sizeof(*token)) ;
  *token = t->func;
  luaL_getmetatable(lua, TIMER_TYPE);
  lua_setmetatable(lua, -2);
  return 1;
}

static gint lua_timer_remaining(lua_State* lua) {
  gint token = *(gint*)luaL_checkudata(lua, 1, TIMER_TYPE);
  struct timer_entry* timer = g_hash_table_lookup(timers,
                                                  GINT_TO_POINTER(token));
  if (timer == NULL) {
    return luaL_error(lua, "timer:remaining(): Call to an expired timer.");
  }
  struct timeval now;
  gettimeofday(&now, NULL);
  struct timeval delta = timer->time;
  timeval_sub(&delta, &now);
  gdouble rv = delta.tv_sec + (gdouble)delta.tv_usec / 1000000;
  lua_pushnumber(lua, rv);
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
    { "new", lua_timer_new },
    { NULL , NULL          }
  };
  luaL_setfuncs(lua, timer_funcs, 0);
  lua_setfield(lua, -2, "timer");
  lua_pop(lua, 1);

  luaL_newmetatable(lua, TIMER_TYPE);
  static const luaL_Reg timer_methods[] = {
    { "cancel"   , lua_timer_cancel    },
    { "remaining", lua_timer_remaining },
    { NULL       , NULL                }
  };
  lua_newtable(lua);
  luaL_setfuncs(lua, timer_methods, 0);
  lua_setfield(lua, -2, "__index");
  lua_pop(lua, 1);
}

void lua_timer_deinit(void) {
  g_hash_table_unref(timers);
}

void lua_timer_execute(const struct timeval* start) {
  lua_State* lua = lua_api_get();
  lua_Number newdelay;
  TIMER_FOREACH(iter, timer) {
    if (!timer->dead
        && timeval_compare(start, &timer->time) > 0) {
      lua_rawgeti(lua, LUA_REGISTRYINDEX, timer->func);
      if (lua_pcall(lua, 0, 1, 0) != 0) {
        const gchar* what = lua_tostring(lua, -1);
        ERROR("Error in timer callback: %s", what);
      } else if (lua_isnumber(lua, -1)
                 && (newdelay = lua_tonumber(lua, -1)) > 0) {
        timer->time = *start;
        timeval_add_delay(&timer->time, newdelay);
      } else {
        timer->dead = TRUE;
      }
      lua_pop(lua, 1);
    }
  }
}

void lua_timer_remove_dead(void) {
  TIMER_FOREACH(iter, timer) {
    if (timer->dead) g_hash_table_iter_remove(&iter);
  }
}
