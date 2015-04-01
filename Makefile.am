ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -Wall -Wextra

lib_LTLIBRARIES = libgoat.la

libgoat_la_SOURCES =                    \
    src/commands.c src/commands.h       \
    src/connection.c src/connection.h   \
    src/error.c src/error.h             \
    src/event.c src/event.h             \
    src/message.c src/message.h         \
    src/tresolver.c src/tresolver.h     \
    src/context.h                       \
    src/sm.h                            \
    src/goat.c

libgoat_la_LDFLAGS = $(AM_LDFLAGS) -export-symbols-regex '^goat_'

include_HEADERS = src/goat.h

TESTS = cunit/basic

check_PROGRAMS = cunit/basic

cunit_basic_SOURCES =               \
    cunit/message/suite.c           \
    cunit/message/constructor.c     \
    cunit/basic.c                   \
    cunit/suite-info.c

cunit_basic_LDADD = -lcunit -lgoat

cunit/suite-info.c: $(cunit_basic_SOURCES)
	perl cunit/gen-suite-info.pl > $@
