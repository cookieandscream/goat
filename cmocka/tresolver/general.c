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

// global state for mocked getaddrinfo
struct getaddrinfo_mock {
    void **state;
    const char *expect_hostname;
    const char *expect_servname;
    struct timespec delay;
    struct addrinfo **out_res;
    int out_return;
} g_getaddrinfo_mock;

pthread_mutex_t g_getaddrinfo_mock_mutex = PTHREAD_MUTEX_INITIALIZER;

static int mock_init(void **state, const char *expect_hostname, const char *expect_servname,
    struct timespec delay, struct addrinfo *out_res, int out_return)
{
    struct addrinfo **tmp = calloc(1, sizeof(*tmp));
    if (NULL == tmp) return errno;
    *tmp = out_res;

    int r = pthread_mutex_lock(&g_getaddrinfo_mock_mutex);
    if (r) return r;

    g_getaddrinfo_mock.state = state;
    g_getaddrinfo_mock.expect_hostname = expect_hostname;
    g_getaddrinfo_mock.expect_servname = expect_servname;
    g_getaddrinfo_mock.delay = delay;
    g_getaddrinfo_mock.out_return = out_return;

    g_getaddrinfo_mock.out_res = tmp;

    r = pthread_mutex_unlock(&g_getaddrinfo_mock_mutex);
    return r;
}

static int mock_destroy(int freeres) {
    int r = pthread_mutex_lock(&g_getaddrinfo_mock_mutex);
    if (r) return r;

    if (freeres) free(g_getaddrinfo_mock.out_res);
    memset(&g_getaddrinfo_mock, 0, sizeof(g_getaddrinfo_mock));

    r = pthread_mutex_unlock(&g_getaddrinfo_mock_mutex);
    return r;
}

int __wrap_getaddrinfo(const char *h, const char *s,
    const struct addrinfo *i, struct addrinfo **res)
{
    ARG_UNUSED(i);

    struct getaddrinfo_mock mock;

    int r = pthread_mutex_lock(&g_getaddrinfo_mock_mutex);
    if (r) return r;

    memcpy(&mock, &g_getaddrinfo_mock, sizeof(mock));

    pthread_mutex_unlock(&g_getaddrinfo_mock_mutex);

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
    struct timespec delay = { 0, HALF_SECOND };

    return mock_init(state, "irc.example.com", "6667", delay, (struct addrinfo *) "PACIFIER", 0);
}

void test_resolver__getaddrinfo___with_successful_name_resolution(void **state) {
    ARG_UNUSED(state);
    ResolverState *resolver_state = NULL;
    struct addrinfo *ai;
    int r = resolver_getaddrinfo(&resolver_state, "irc.example.com", "6667", &ai);

    assert_int_equal(r, 0);
    assert_non_null(resolver_state);
    assert_null(ai);

    struct timespec wait = { 1, 0 };
    nanosleep(&wait, NULL);

    r = resolver_getaddrinfo(&resolver_state, "irc.example.com", "6667", &ai);

    assert_int_equal(r, 0);
    assert_null(resolver_state);
    assert_non_null(ai);
    assert_string_equal((const char *) ai, "PACIFIER");
}

int teardown_resolver__getaddrinfo___with_successful_name_resolution(void **state) {
    ARG_UNUSED(state);

    return mock_destroy(1);
}

/* ====================================================================== */

int setup_resolver__getaddrinfo___with_unsuccessful_name_resolution(void **state) {
    struct timespec delay = { 0, HALF_SECOND };

    return mock_init(state, "irc.example.com", "6667", delay, NULL, EAI_FAIL);
}

void test_resolver__getaddrinfo___with_unsuccessful_name_resolution(void **state) {
    ARG_UNUSED(state);
    ResolverState *resolver_state = NULL;
    struct addrinfo *ai;
    int r = resolver_getaddrinfo(&resolver_state, "irc.example.com", "6667", &ai);

    assert_int_equal(r, 0);
    assert_non_null(resolver_state);
    assert_null(ai);

    struct timespec wait = { 1, 0 };
    nanosleep(&wait, NULL);

    r = resolver_getaddrinfo(&resolver_state, "irc.example.com", "6667", &ai);

    assert_int_equal(r, EAI_FAIL);
    assert_null(resolver_state);
    assert_null(ai);
}

int teardown_resolver__getaddrinfo___with_unsuccessful_name_resolution(void **state) {
    ARG_UNUSED(state);

    return mock_destroy(1);
}

/* ====================================================================== */

int setup_resolver__getaddrinfo___with_cancelled_request(void **state) {
    struct timespec delay = { 2, 0 };

    return mock_init(state, "irc.example.com", "6667", delay, NULL, EAI_FAIL);
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

    return mock_destroy(0);
}

/* ====================================================================== */

int setup_resolver__getaddrinfo___with_slow_lookup(void **state) {
    struct timespec delay = { 2, HALF_SECOND };

    return mock_init(state, "irc.example.com", "6667", delay, (struct addrinfo *) "PACIFIER", 0);
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

    struct timespec wait = { 5, 0 };
    nanosleep(&wait, NULL);

    r = resolver_getaddrinfo(&resolver_state, "irc.example.com", "6667", &ai);

    assert_int_equal(r, 0);
    assert_null(resolver_state);
    assert_non_null(ai);
    assert_string_equal((const char *) ai, "PACIFIER");
}

int teardown_resolver__getaddrinfo___with_slow_lookup(void **state) {
    ARG_UNUSED(state);

    return mock_destroy(1);
}
