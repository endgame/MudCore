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
LUA_PACKAGE=lua
PKG_CHECK_EXISTS([lua5.1], [LUA_PACKAGE=lua5.1])
PKG_CHECK_EXISTS([lua-5.1], [LUA_PACKAGE=lua-5.1])
PKG_CHECK_MODULES([Lua], [$LUA_PACKAGE])
AC_SEARCH_LIBS([yajl_parse],
               [yajl yajl_s],
               [:],
               [AC_MSG_ERROR([YAJL required, but not found.])])
PKG_CHECK_MODULES([ZeroMQ], [libzmq >= 2.1])

# Checks for header files.
AC_CHECK_HEADERS([yajl/yajl_gen.h yajl/yajl_parse.h],
                 [:],
                 [AC_MSG_ERROR([A required YAJL header is missing.])])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile doc/Makefile src/Makefile])
AC_OUTPUT
