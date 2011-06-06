#ifndef LUA_TIMER_H
#define LUA_TIMER_H

#include <lua.h>
#include <sys/time.h>

/**
 ** @deftypefun void lua_zmq_init @
 **   (lua_State* @var{lua})
 ** Add the @code{mud.timer} api to @var{lua} (@pxref{mud.timer}).
 ** @end deftypefun
 **/
void lua_timer_init(lua_State* lua);

/**
 ** @deftypefun void lua_timer_deinit @
 **   (void)
 ** Release memory used by the timer api.
 ** @end deftypefun
 **/
void lua_timer_deinit(void);

/**
 ** @deftypefun void lua_timer_execute @
 **   (const struct timeval* @var{start})
 ** Execute any timers that are scheduled to run before @var{start}.
 ** @end deftypefun
 **/
void lua_timer_execute(const struct timeval* start);

/**
 ** @deftypefun void lua_timer_remove_dead @
 **   (void)
 ** Remove any dead timers. A timer is dead if and only if it has
 ** fired and has not been rescheduled.
 ** @end deftypefun
 **/
void lua_timer_remove_dead(void);

#endif
