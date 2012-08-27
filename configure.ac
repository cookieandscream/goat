#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([goat], [0.0], [https://github.com/flrl/goat])

AC_CONFIG_SRCDIR([src/public.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_CXX
AC_PROG_RANLIB
AM_PROG_AR

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_RESTRICT
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([select])

AM_INIT_AUTOMAKE([foreign silent-rules subdir-objects -Wall -Werror])
LT_INIT

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
