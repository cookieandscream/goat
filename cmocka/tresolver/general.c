#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "cmocka/run.h"

#include "src/tresolver.h"

int tresolver_general_group_setup(void **state UNUSED) {
    return 0;
}

int tresolver_general_group_teardown(void **state UNUSED) {
    return 0;
}

static double fsec(const struct timespec *t) {
    return t->tv_sec + (t->tv_nsec / 1000000000.0);
}

int __wrap_getaddrinfo(const char *h, const char *s,
    const struct addrinfo *hi UNUSED, struct addrinfo **res)
{
    check_expected_ptr(h);
    check_expected_ptr(s);

    const struct timespec *delay = mock_ptr_type(const struct timespec *);

    fprintf(stderr, "%s: sleeping for %g seconds\n", __func__, fsec(delay));
    nanosleep(delay, NULL);

    *res = mock_ptr_type(struct addrinfo *);

    return mock_type(int);
}

void test_resolver__getaddrinfo___something_something(void **state UNUSED) {
    expect_string(__wrap_getaddrinfo, h, "irc.example.com");
    expect_string(__wrap_getaddrinfo, s, "6667");

    struct timespec delay = { 1, 0 };
    const struct addrinfo *fake_res = (const struct addrinfo *) "PACIFIER";

    will_return(__wrap_getaddrinfo, 0);
    will_return(__wrap_getaddrinfo, cast_ptr_to_largest_integral_type(fake_res));
    will_return(__wrap_getaddrinfo, cast_ptr_to_largest_integral_type(&delay));

    ResolverState *resolver_state = NULL;
    struct addrinfo *ai;
    int r = resolver_getaddrinfo(&resolver_state, "irc.example.com", "6667", &ai);

    assert_int_equal(r, 0);
    assert_non_null(resolver_state);
    assert_null(ai);

    nanosleep(&delay, NULL);
    nanosleep(&delay, NULL);

    r = resolver_getaddrinfo(&resolver_state, "irc.example.com", "6667", &ai);

    assert_int_equal(r, 0);
    assert_null(resolver_state);
    assert_non_null(ai);
    assert_string_equal((const char *) ai, "PACIFIER");
}
