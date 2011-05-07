/**
 ** @node lua_descriptor.h
 ** @section lua_descriptor.h
 **/

#ifndef LUA_DESCRIPTOR_H
#define LUA_DESCRIPTOR_H

#include <glib.h>
#include <lua.h>

struct descriptor;

/**
 ** @deftypefun void lua_descriptor_init @
 **   (lua_State* @var{lua})
 ** Add the @code{mud.descriptor} api to @var{lua} (@pxref{mud.descriptor}).
 ** @end deftypefun
 **/
void lua_descriptor_init(lua_State* lua);

/**
 ** @deftypefun void lua_descriptor_start @
 **   (struct descriptor* @var{descriptor})
 ** Create a new thread for @var{descriptor}, store it in the
 ** @code{descriptor->thread_ref} field and start it with the
 ** descriptor number.
 ** @end deftypefun
 **/
void lua_descriptor_start(struct descriptor* descriptor);

/**
 ** @deftypefun void lua_descriptor_resume  @
 **   (struct descriptor* @var{descriptor}, @
 **    const gchar*       @var{command})
 ** Resume a descriptor's thread. If @var{command} is not @code{NULL},
 ** it is sent to the descriptor as the next line of input.
 ** @end deftypefun
 **/
void lua_descriptor_resume(struct descriptor* descriptor,
                           const gchar* command);

#endif
