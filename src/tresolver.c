#include <config.h>

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "tresolver.h"

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

static int _resolver_init(ResolverState **statep, const char *hostname,
    const char *servname, struct addrinfo **resp);
static int _resolver_status(ResolverState **statep, struct addrinfo **resp);
static void *_resolver_thread(void *);

// returns:
//  0 - ok: if res is set, finished, otherwise still busy
//  nonzero - error
int resolver_getaddrinfo(ResolverState **statep, const char *hostname,
    const char *servname, struct addrinfo **resp)
{
    if (! *statep) {
        return _resolver_init(statep, hostname, servname, resp);
    }
    else {
        return _resolver_status(statep, resp);
    }
}

int resolver_cancel(ResolverState **statep) {
    ResolverState *state = *statep;

    int r = pthread_mutex_lock(&state->mutex);
    if (r) return r;

    *statep = NULL;
    state->status = RESOLVER_CANCELLED;
    pthread_detach(state->thread);
    pthread_mutex_unlock(&state->mutex);

    return 0;
}

static int _resolver_init(ResolverState **statep, const char *hostname,
    const char *servname, struct addrinfo **resp)
{
    if (NULL == statep) return EINVAL;
    if (NULL == hostname) return EINVAL;
    if (NULL == servname) return EINVAL;
    if (NULL == resp) return EINVAL;

    ResolverState *state = calloc(1, sizeof(ResolverState));
    if (!state)  return errno;

    state->hostname = strdup(hostname);
    if (NULL == state->hostname) return errno;

    state->servname = strdup(servname);
    if (NULL == state->servname) return errno;

    state->status = RESOLVER_BUSY;

    int r = pthread_mutex_init(&state->mutex, NULL);
    if (r) goto cleanup;

    r = pthread_create(&state->thread, NULL, _resolver_thread, state);
    if (r) goto cleanup;

    *statep = state;
    *resp = NULL;
    return 0;

cleanup:
    pthread_mutex_destroy(&state->mutex);
    free(state->servname);
    free(state->hostname);
    free(state);
    *statep = NULL;
    *resp = NULL;
    return r;
}

static int _resolver_status(ResolverState **statep, struct addrinfo **resp) {
    ResolverState *state = *statep;

    int r = pthread_mutex_lock(&state->mutex);
    if (r) {
        // lock failed, maybe temporary?  return error but don't throw everything away
        *resp = NULL;
        return r;
    }

    enum e_resolver_status status = state->status;
    int error = state->error;
    struct addrinfo *res = state->res;

    pthread_mutex_unlock(&state->mutex);

    switch(status) {
        case RESOLVER_BUSY:
            *resp = NULL;
            return 0;

        case RESOLVER_DONE:
            *resp = res;
            *statep = NULL;
            pthread_join(state->thread, NULL);
            pthread_mutex_destroy(&state->mutex);
            free(state->servname);
            free(state->hostname);
            free(state);
            return 0;

        case RESOLVER_ERROR:
        case RESOLVER_CANCELLED:
        default:
            *resp = NULL;
            *statep = NULL;
            pthread_join(state->thread, NULL);
            pthread_mutex_destroy(&state->mutex);
            free(state->servname);
            free(state->hostname);
            free(state);
            return error;
    }
}

void *_resolver_thread(void *arg) {
    assert(arg != NULL);

    ResolverState *state = (ResolverState *) arg;
    struct addrinfo *res;

    int r = getaddrinfo(state->hostname, state->servname, NULL, &res);

    if (0 == pthread_mutex_lock(&state->mutex)) {
        if (state->status == RESOLVER_CANCELLED) {
            // request has been cancelled and thread detached
            // clean up after yourself, no-one else has pointers anymore

            if (res)  freeaddrinfo(res);
            pthread_mutex_unlock(&state->mutex);
            pthread_mutex_destroy(&state->mutex);
            free(state->servname);
            free(state->hostname);
            free(state);
            return NULL;
        }

        if (0 == r) {
            // resolution succeeded
            state->status = RESOLVER_DONE;
            state->error = 0;
            state->res = res;
        }
        else {
            // resolution failed
            state->status = RESOLVER_ERROR;
            state->error = r;
            state->res = NULL;

            if (res) freeaddrinfo(res);
        }

        pthread_mutex_unlock(&state->mutex);

        // done, wait around to be joined
        return NULL;
    }

    // FIXME mutex lock failed for some reason, crap out
    return NULL;
}
