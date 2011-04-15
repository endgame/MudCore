## Process this file with automake to generate Makefile.in
AM_CFLAGS = $(GLib_CFLAGS) $(Lua_CFLAGS) $(ZeroMQ_CFLAGS)
LDADD = $(GLib_LIBS) $(Lua_LIBS) $(ZeroMQ_LIBS)
TESTS = queue_test

bin_PROGRAMS = mudcore
check_PROGRAMS = $(TESTS)
mudcore_SOURCES = \
	arg_parse.c \
	arg_parse.h \
	buffer.c \
	buffer.h \
	io.c \
	io.h \
	log.c \
	log.h \
	lua_api.c \
	lua_api.h \
	lua_args.c \
	lua_args.h \
	lua_log.c \
	lua_log.h \
	mudcore.c \
	options.c \
	options.h \
	queue.c \
	queue.h \
	socket.c \
	socket.h

queue_test_SOURCES = queue.c queue.h queue_test.c
