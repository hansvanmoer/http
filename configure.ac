#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([http], [0.0.1], [hans.vanmoer@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

AX_PTHREAD([], [AC_MSG_ERROR([POSIX threading library not found])])

# Checks for header files.
AC_CHECK_HEADERS([
	assert.h
	errno.h
	netdb.h
	stdbool.h
	stdint.h
	stdio.h
	stdlib.h
	string.h
	sys/socket.h
	sys/types.h
	unistd.h

])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_OUTPUT
