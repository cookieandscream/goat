#include <config.h>

#include <assert.h>
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

struct s_resolver_state {
    pthread_t thread;
    pthread_mutex_t mutex;
    struct addrinfo *res;
    enum e_resolver_status status;
    int error;
    size_t hostname_size;
    char hostname[0];
};

void *_resolver_thread(void *);

// returns:
//  0 - ok: if res is set, finished, otherwise still busy
//  nonzero - error
int resolver_getaddrinfo(struct s_resolver_state **statep, const char *hostname, struct addrinfo **resp) {
    if (! *statep) {
        size_t sz = strlen(hostname) + 1;
        struct s_resolver_state *state = calloc(1, sizeof(struct s_resolver_state) + sz);
        if (!state)  return -1;

        state->hostname_size = sz;
        strlcpy(state->hostname, hostname, sz);

        state->status = RESOLVER_BUSY;

        if (0 == pthread_mutex_init(&state->mutex, NULL)) {
            if (0 == pthread_create(&state->thread, NULL, _resolver_thread, state)) {
                *statep = state;
                *resp = NULL;
                return 0;
            }

            pthread_mutex_destroy(&state->mutex);
        }

        free(state);
        *statep = NULL;
        *resp = NULL;
        return -1;
    }
    else {
        struct s_resolver_state *state = *statep;

        if (0 == pthread_mutex_lock(&state->mutex)) {
            enum e_resolver_status status = state->status;
            int error = state->error;
            struct addrinfo *res = state->res;

            pthread_mutex_unlock(&state->mutex);

            switch(status) {
                case RESOLVER_BUSY:
                    *resp = NULL;
                    return 0;

                case RESOLVER_DONE:
                    *resp = state->res;
                    *statep = NULL;
                    pthread_join(state->thread, NULL);
                    pthread_mutex_destroy(&state->mutex);
                    free(state);
                    return 0;

                case RESOLVER_ERROR:
                case RESOLVER_CANCELLED:
                default:
                    *resp = NULL;
                    *statep = NULL;
                    pthread_join(state->thread, NULL);
                    pthread_mutex_destroy(&state->mutex);
                    free(state);
                    return error;
            }
        }

        // lock failed, maybe temporary?  return error but don't throw everything away
        *resp = NULL;
        return -1;
    }
}

int resolver_cancel(struct s_resolver_state **statep) {
    struct s_resolver_state *state = *statep;

    if (0 == pthread_mutex_lock(&state->mutex)) {
        *statep = NULL;
        state->status = RESOLVER_CANCELLED;
        pthread_detach(state->thread);
        pthread_mutex_unlock(&state->mutex);

        return 0;
    }

    return -1;
}

void *_resolver_thread(void *arg) {
    assert(arg != NULL);

    struct s_resolver_state *state = (struct s_resolver_state *) arg;
    struct addrinfo *res;

    int r = getaddrinfo(state->hostname, NULL, NULL, &res);

    if (0 == pthread_mutex_lock(&state->mutex)) {
        if (state->status == RESOLVER_CANCELLED) {
            // request has been cancelled and thread detached
            // clean up after yourself, no-one else has pointers anymore

            if (res)  freeaddrinfo(res);
            pthread_mutex_unlock(&state->mutex);
            pthread_mutex_destroy(&state->mutex);
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
