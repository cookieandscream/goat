ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -Wall -Wextra

lib_LTLIBRARIES = libgoat.la

libgoat_la_SOURCES =                    \
    src/connection.c src/connection.h   \
    src/context.c src/context.h         \
    src/error.c src/error.h             \
    src/event.c src/event.h             \
    src/irc.c src/irc.h                 \
    src/message.c src/message.h         \
    src/tags.c src/tags.h               \
    src/tresolver.c src/tresolver.h     \
    src/util.c src/util.h               \
    src/sm.h                            \
    src/goat.c

libgoat_la_CPPFLAGS = $(AM_CPPFLAGS) $(TLS_CPPFLAGS)

libgoat_la_LDFLAGS = $(AM_LDFLAGS) $(TLS_LDFLAGS) -export-symbols-regex '^goat_'

include_HEADERS = src/goat.h

TESTS = cunit/basic

check_PROGRAMS = cunit/basic

CUNIT_TESTFILES =                \
    cunit/message/accessor.c     \
    cunit/message/constructor.c  \
    cunit/message/stringify.c

cunit_basic_SOURCES =   \
    cunit/basic.c       \
    cunit/suite-info.c  \
    $(CUNIT_TESTFILES)

cunit_basic_LDADD = -lcunit -lgoat

cunit/suite-info.c: cunit/gen-suite-info.pl $(CUNIT_TESTFILES)
	perl cunit/gen-suite-info.pl $(CUNIT_TESTFILES) > $@
