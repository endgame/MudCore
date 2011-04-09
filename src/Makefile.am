## Process this file with automake to generate Makefile.in
AM_CFLAGS = $(GLib_CFLAGS) $(Lua_CFLAGS) $(ZeroMQ_CFLAGS)
LDADD = $(GLib_LIBS) $(Lua_LIBS) $(ZeroMQ_LIBS)

bin_PROGRAMS = mudcore
mudcore_SOURCES = \
	log.c \
	log.h \
	lua_api.c \
	lua_api.h \
	lua_log.c \
	lua_log.h \
	mudcore.c
