ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libgoat.la

libgoat_la_SOURCES =                    \
    src/connection.c src/connection.h   \
    src/error.c src/error.h             \
    src/sm.h                            \
    src/goat.c

libgoat_la_LDFLAGS = -export-symbols-regex '^goat_'

include_HEADERS = src/goat.h
