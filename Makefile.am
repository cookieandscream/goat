ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -Wall -Wextra
AM_CPPFLAGS =
AM_LDFLAGS =

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

TESTS =
check_PROGRAMS =

if HAVE_CMOCKA
    AM_CPPFLAGS += $(CMOCKA_CPPFLAGS)
    AM_LDFLAGS += $(CMOCKA_LDFLAGS)

    check_PROGRAMS +=               \
        tests/msg-accessor          \
        tests/msg-constructor       \
        tests/msg-stringify         \
        tests/msg-tags              \
        tests/tresolver

    TESTS += $(check_PROGRAMS)

    tests_msg_accessor_LDADD = $(CMOCKA_LIBS) -lgoat
    tests_msg_constructor_LDADD = $(CMOCKA_LIBS) -lgoat
    tests_msg_stringify_LDADD = $(CMOCKA_LIBS) -lgoat
    tests_msg_tags_LDADD = $(CMOCKA_LIBS) -lgoat

    tests_tresolver_SOURCES = $(libgoat_la_SOURCES) tests/tresolver.c
    tests_tresolver_CPPFLAGS = $(libgoat_la_CPPFLAGS)
    tests_tresolver_LDFLAGS = $(libgoat_la_LDFLAGS) $(CMOCKA_LDFLAGS) -Wl,-alias,___wrap_getaddrinfo,_getaddrinfo
    tests_tresolver_LDADD = $(CMOCKA_LIBS)
endif

%.c : %.cmocka cmocka/main.c cmocka/wrap-test.pl
	cmocka/wrap-test.pl $< > $@.tmp && mv $@.tmp $@
