#ifndef GOAT_CMOCKA_RUN_H
#define GOAT_CMOCKA_RUN_H

// cmocka needs these pre-included for some reason
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

typedef struct {
    const char *name;
    CMFixtureFunction setup;
    CMFixtureFunction teardown;
    size_t n_tests;
    const struct CMUnitTest tests[];
} goat_test_group_t;

#endif
