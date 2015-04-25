#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#include "cmocka/run.h"

#include "src/tresolver.h"
#include "src/util.h"

#define HALF_SECOND (500000000)

// copy of internal types so we can inspect state innards
enum e_resolver_status {
    RESOLVER_BUSY,
    RESOLVER_DONE,
    RESOLVER_ERROR,
    RESOLVER_CANCELLED,
};

struct resolver_state {
    pthread_t thread;
    pthread_mutex_t mutex;
    struct addrinfo *res;
    enum e_resolver_status status;
    int error;
    char *hostname;
    char *servname;
};

struct getaddrinfo_mock {
    void **state;
    const char *expect_hostname;
    const char *expect_servname;
    struct timespec delay;
    struct addrinfo **out_res;
    int out_return;
} getaddrinfo_mock;

int __wrap_getaddrinfo(const char *h, const char *s,
    const struct addrinfo *i, struct addrinfo **res)
{
    ARG_UNUSED(i);

    struct getaddrinfo_mock mock;
    memcpy(&mock, &getaddrinfo_mock, sizeof(mock));

    assert_string_equal(h, mock.expect_hostname);
    assert_string_equal(s, mock.expect_servname);
    assert_non_null(res);

    // pretend to do some blocking work
    nanosleep(&mock.delay, NULL);

    *res = *mock.out_res;

    return mock.out_return;
}

/* ====================================================================== */

int setup_resolver__getaddrinfo___with_successful_name_resolution(void **state) {
    getaddrinfo_mock.state = state;
    getaddrinfo_mock.expect_hostname = "irc.example.com";
    getaddrinfo_mock.expect_servname = "6667";
    getaddrinfo_mock.delay.tv_sec = 0;
    getaddrinfo_mock.delay.tv_nsec = HALF_SECOND;
    getaddrinfo_mock.out_return = 0;

    getaddrinfo_mock.out_res = calloc(1, sizeof(*getaddrinfo_mock.out_res));
    if (NULL == getaddrinfo_mock.out_res) return -1;

    *getaddrinfo_mock.out_res = (struct addrinfo *) "PACIFIER";
    return 0;
}

void test_resolver__getaddrinfo___with_successful_name_resolution(void **state) {
    ARG_UNUSED(state);
    ResolverState *resolver_state = NULL;
    struct addrinfo *ai;
    int r = resolver_getaddrinfo(&resolver_state, "irc.example.com", "6667", &ai);

    assert_int_equal(r, 0);
    assert_non_null(resolver_state);
    assert_null(ai);

    nanosleep(&getaddrinfo_mock.delay, NULL);
    nanosleep(&getaddrinfo_mock.delay, NULL);

    r = resolver_getaddrinfo(&resolver_state, "irc.example.com", "6667", &ai);

    assert_int_equal(r, 0);
    assert_null(resolver_state);
    assert_non_null(ai);
    assert_string_equal((const char *) ai, "PACIFIER");
}

int teardown_resolver__getaddrinfo___with_successful_name_resolution(void **state) {
    ARG_UNUSED(state);
    free(getaddrinfo_mock.out_res);
    memset(&getaddrinfo_mock, 0, sizeof(getaddrinfo_mock));
    return 0;
}

/* ====================================================================== */

int setup_resolver__getaddrinfo___with_unsuccessful_name_resolution(void **state) {
    getaddrinfo_mock.state = state;
    getaddrinfo_mock.expect_hostname = "irc.example.com";
    getaddrinfo_mock.expect_servname = "6667";
    getaddrinfo_mock.delay.tv_sec = 0;
    getaddrinfo_mock.delay.tv_nsec = HALF_SECOND;
    getaddrinfo_mock.out_return = EAI_FAIL;

    getaddrinfo_mock.out_res = calloc(1, sizeof(*getaddrinfo_mock.out_res));
    if (NULL == getaddrinfo_mock.out_res) return -1;

    *getaddrinfo_mock.out_res = NULL;
    return 0;
}

void test_resolver__getaddrinfo___with_unsuccessful_name_resolution(void **state) {
    ARG_UNUSED(state);
    ResolverState *resolver_state = NULL;
    struct addrinfo *ai;
    int r = resolver_getaddrinfo(&resolver_state, "irc.example.com", "6667", &ai);

    assert_int_equal(r, 0);
    assert_non_null(resolver_state);
    assert_null(ai);

    nanosleep(&getaddrinfo_mock.delay, NULL);
    nanosleep(&getaddrinfo_mock.delay, NULL);

    r = resolver_getaddrinfo(&resolver_state, "irc.example.com", "6667", &ai);

    assert_int_equal(r, EAI_FAIL);
    assert_null(resolver_state);
    assert_null(ai);
}

int teardown_resolver__getaddrinfo___with_unsuccessful_name_resolution(void **state) {
    ARG_UNUSED(state);
    free(getaddrinfo_mock.out_res);
    memset(&getaddrinfo_mock, 0, sizeof(getaddrinfo_mock));
    return 0;
}

/* ====================================================================== */

int setup_resolver__getaddrinfo___with_cancelled_request(void **state) {
    getaddrinfo_mock.state = state;
    getaddrinfo_mock.expect_hostname = "irc.example.com";
    getaddrinfo_mock.expect_servname = "6667";
    getaddrinfo_mock.delay.tv_sec = 2;
    getaddrinfo_mock.delay.tv_nsec = 0;
    getaddrinfo_mock.out_return = EAI_FAIL;

    getaddrinfo_mock.out_res = calloc(1, sizeof(*getaddrinfo_mock.out_res));
    if (NULL == getaddrinfo_mock.out_res) return -1;

    *getaddrinfo_mock.out_res = NULL;
    return 0;
}

void test_resolver__getaddrinfo___with_cancelled_request(void **state) {
    ARG_UNUSED(state);
    ResolverState *resolver_state = NULL;
    struct addrinfo *ai;
    int r = resolver_getaddrinfo(&resolver_state, "irc.example.com", "6667", &ai);

    assert_int_equal(r, 0);
    assert_non_null(resolver_state);
    assert_null(ai);

    // take a copy of the resolver state so we can get its thread id after cancellation
    ResolverState resolver_state_copy;
    memcpy(&resolver_state_copy, resolver_state, sizeof(resolver_state_copy));

    r = resolver_cancel(&resolver_state);

    assert_int_equal(r, 0);
    assert_null(resolver_state);

    // try to join the cancelled thread - should not be possible
    void *val;
    r = pthread_join(resolver_state_copy.thread, &val);
    assert_int_equal(r, EINVAL);
}

int teardown_resolver__getaddrinfo___with_cancelled_request(void **state) {
    ARG_UNUSED(state);
    // don't free getaddrinfo_mock.out_res, the cancelled thread will do it
    memset(&getaddrinfo_mock, 0, sizeof(getaddrinfo_mock));
    return 0;
}

/* ====================================================================== */

int setup_resolver__getaddrinfo___with_slow_lookup(void **state) {
    getaddrinfo_mock.state = state;
    getaddrinfo_mock.expect_hostname = "irc.example.com";
    getaddrinfo_mock.expect_servname = "6667";
    getaddrinfo_mock.delay.tv_sec = 2;
    getaddrinfo_mock.delay.tv_nsec = HALF_SECOND;
    getaddrinfo_mock.out_return = 0;

    getaddrinfo_mock.out_res = calloc(1, sizeof(*getaddrinfo_mock.out_res));
    if (NULL == getaddrinfo_mock.out_res) return -1;

    *getaddrinfo_mock.out_res = (struct addrinfo *) "PACIFIER";
    return 0;
}

void test_resolver__getaddrinfo___with_slow_lookup(void **state) {
    ARG_UNUSED(state);
    ResolverState *resolver_state = NULL;
    struct addrinfo *ai;
    int r = resolver_getaddrinfo(&resolver_state, "irc.example.com", "6667", &ai);

    assert_int_equal(r, 0);
    assert_non_null(resolver_state);
    assert_null(ai);

    r = resolver_getaddrinfo(&resolver_state, "irc.example.com", "6667", &ai);

    assert_int_equal(r, 0);
    assert_non_null(resolver_state);
    assert_null(ai);

    r = resolver_getaddrinfo(&resolver_state, "irc.example.com", "6667", &ai);

    assert_int_equal(r, 0);
    assert_non_null(resolver_state);
    assert_null(ai);

    nanosleep(&getaddrinfo_mock.delay, NULL);
    nanosleep(&getaddrinfo_mock.delay, NULL);

    r = resolver_getaddrinfo(&resolver_state, "irc.example.com", "6667", &ai);

    assert_int_equal(r, 0);
    assert_null(resolver_state);
    assert_non_null(ai);
    assert_string_equal((const char *) ai, "PACIFIER");
}

int teardown_resolver__getaddrinfo___with_slow_lookup(void **state) {
    ARG_UNUSED(state);
    free(getaddrinfo_mock.out_res);
    memset(&getaddrinfo_mock, 0, sizeof(getaddrinfo_mock));
    return 0;
}
