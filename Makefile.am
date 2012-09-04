ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libgoat.la

libgoat_la_SOURCES = src/core.c src/core.h src/goat.c src/connection.c src/connection.h
libgoat_la_LDFLAGS = -export-symbols-regex '^goat_'

include_HEADERS = src/goat.h
