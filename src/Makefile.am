## Process this file with automake to generate Makefile.in
AM_CFLAGS = $(GLib_CFLAGS) $(libtelnet_CFLAGS) $(Lua_CFLAGS) $(ZeroMQ_CFLAGS)
LDADD     = $(GLib_LIBS)   $(libtelnet_LIBS)   $(Lua_LIBS)   $(ZeroMQ_LIBS)

bin_PROGRAMS = mudcore

mudcore_SOURCES = \
	arg_parse.c \
	arg_parse.h \
	buffer.c \
	buffer.h \
	descriptor.c \
	descriptor.h \
	io.c \
	io.h \
	log.c \
	log.h \
	lua_api.c \
	lua_api.h \
	lua_args.c \
	lua_args.h \
	lua_descriptor.c \
	lua_descriptor.h \
	lua_log.c \
	lua_log.h \
	lua_timer.c \
	lua_timer.h \
	lua_zmq.c \
	lua_zmq.h \
	mudcore.c \
	options.c \
	options.h \
	queue.c \
	queue.h \
	socket.c \
	socket.h \
	timeval.c \
	timeval.h
