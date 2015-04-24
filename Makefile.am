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

TESTS =
check_PROGRAMS =

if HAVE_CMOCKA
    TESTS += cmocka/run
    check_PROGRAMS += cmocka/run

    CMOCKA_TESTFILES =                  \
        cmocka/message/accessor.c       \
        cmocka/message/constructor.c    \
        cmocka/message/stringify.c      \
        cmocka/message/tags.c           \
        cmocka/tresolver/general.c

    cmocka_run_SOURCES =        \
        $(libgoat_la_SOURCES)   \
        cmocka/run.c            \
        cmocka/test-db.c        \
        $(CMOCKA_TESTFILES)

    cmocka_run_CPPFLAGS = $(libgoat_la_CPPFLAGS) $(CMOCKA_CPPFLAGS)
    # FIXME detect and use --wrap on gcc platforms
    cmocka_run_LDFLAGS = $(libgoat_la_LDFLAGS) $(CMOCKA_LDFLAGS) -Wl,-alias,___wrap_getaddrinfo,_getaddrinfo
    cmocka_run_LDADD = $(libgoat_la_LDADD) -ltls -lcmocka
endif

cmocka/test-db.c: cmocka/gen-test-db.pl $(CMOCKA_TESTFILES)
	perl cmocka/gen-test-db.pl $(CMOCKA_TESTFILES) > $@
