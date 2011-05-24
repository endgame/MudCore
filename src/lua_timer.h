#ifndef LUA_TIMER_H
#define LUA_TIMER_H

#include <lua.h>
#include <sys/time.h>

void lua_timer_init(lua_State* lua);
void lua_timer_deinit(void);

/* Execute any timers that are scheduled after START. */
void lua_timer_execute(const struct timeval* start);

#endif
