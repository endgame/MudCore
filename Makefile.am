## Process this file with automake to generate Makefile.in
ACLOCAL_AMFLAGS = -I m4 --install
AM_CFLAGS = $(GLib_CFLAGS) $(Lua_CFLAGS) $(ZeroMQ_CFLAGS)
LDADD = $(GLib_LIBS) $(Lua_LIBS) $(ZeroMQ_LIBS)
SUBDIRS = doc

bin_PROGRAMS = mudcore
mudcore_SOURCES = \
	src/log.c \
	src/log.h \
	src/lua_api.c \
	src/lua_api.h \
	src/mudcore.c
