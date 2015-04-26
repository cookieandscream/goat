#ifndef GOAT_CMOCKA_MAIN_H
#define GOAT_CMOCKA_MAIN_H

// cmocka.h needs these pre-included for some reason
#include <stdio.h>
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
} TestGroup;

extern const TestGroup test_group;

#endif
