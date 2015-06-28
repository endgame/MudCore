#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([mudcore], [0], [jack@jackkelly.name])
AC_CONFIG_AUX_DIR([auxfiles])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/mudcore.c])
AM_INIT_AUTOMAKE([1.11 foreign -Wall -Werror color-tests])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CC_C99
AC_PROG_SED

# Checks for libraries.
PKG_CHECK_MODULES([GLib], [glib-2.0])
PKG_CHECK_MODULES([libtelnet], [libtelnet])

AC_ARG_WITH([lua_package],
  [AS_HELP_STRING([--with-lua-package=PACKAGE],
    [specify lua pkg-config file to use @<:@default=check@:>@])],
  [],
  [with_lua_package=check])

AS_IF([test "$with_lua_package" = check -a -z "$Lua_CFLAGS" -a -z "$Lua_LIBS"],
  [AC_MSG_CHECKING([for lua package])
   lua_package=no
   m4_foreach([TEST_PKG], [lua, lua5.2, lua-5.2],
     [PKG_CHECK_EXISTS([TEST_PKG], [lua_package=TEST_PKG])m4_newline])
   AC_MSG_RESULT([$lua_package])
   AS_IF([test $lua_package = no],
     [AC_MSG_WARN([could not autodetect lua package])
      lua_package=lua])],
  [lua_package=$with_lua_package])

PKG_CHECK_MODULES([Lua], [$lua_package])
PKG_CHECK_MODULES([ZeroMQ], [libzmq >= 4.0.4])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_DECL([zmq_curve_keypair],
  [AC_DEFINE([HAVE_ZMQ_CURVE_KEYPAIR],
    [1], [Define if zmq_curve_keypair is provided by your zmq.h.])],,
  [[#include <zmq.h>]])

AC_CONFIG_FILES([Makefile doc/Makefile src/Makefile])
AC_OUTPUT
