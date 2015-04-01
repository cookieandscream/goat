#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([goat], [0.0], [https://github.com/flrl/goat])

AC_CONFIG_SRCDIR([src/goat.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AC_USE_SYSTEM_EXTENSIONS

# Checks for programs.
AM_PROG_AR
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.

# Check for libtls
AC_ARG_WITH([tls], AS_HELP_STRING([--with-tls=PATH],[path to libtls]),[],[with_tls=yes])

TLS_CPPFLAGS=
TLS_LDFLAGS=
AS_IF(
    [test "x$with_tls" = xyes], [],
    [test "x$with_tls" = xno], [],
    [TLS_CPPFLAGS="-I$with_tls/include"; TLS_LDFLAGS="-L$with_tls/lib"]
)

AC_SUBST([TLS_CPPFLAGS], [${TLS_CPPFLAGS}])
AC_SUBST([TLS_LDFLAGS], [${TLS_LDFLAGS}])

SAVED_CPPFLAGS=${CPPFLAGS}
SAVED_LDFLAGS=${LDFLAGS}
CPPFLAGS=${TLS_CPPFLAGS} ${CPPFLAGS}
LDFLAGS=${TLS_LDFLAGS} ${LDFLAGS}

AC_SEARCH_LIBS([tls_init], [tls], [], AC_MSG_ERROR([libtls is required: libressl.org]))

CPPFLAGS=${SAVED_CPPFLAGS}
LDFLAGS=${SAVED_LDFLAGS}

# Checks for header files.
AC_CHECK_HEADERS([sys/socket.h sys/time.h fcntl.h netdb.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_STRNLEN
AC_CHECK_FUNCS([memmove memset select socket stpcpy strchr strdup strerror])

AM_INIT_AUTOMAKE([foreign serial-tests silent-rules subdir-objects -Wall -Werror])
LT_INIT

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
