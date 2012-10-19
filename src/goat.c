#include <config.h>

#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "goat.h"

#include "connection.h"
#include "event.h"

typedef struct s_goat_connection goat_connection_t; // FIXME connection.h

const size_t CONN_ALLOC_INCR = 16;

struct s_goat_context {
    pthread_rwlock_t    m_rwlock;
    goat_connection_t   **m_connections;
    size_t              m_connections_size;
    pthread_mutex_t     m_connections_count;
    goat_callback_t     *m_callbacks;
    goat_error_t        m_error;
};

goat_context_t *goat_context_new() {
    // we don't need to lock in here, because no other thread has a pointer to this context yet
    goat_context_t *context = calloc(1, sizeof goat_context_t);
    if (!context)  return NULL;

    if (0 != pthread_rwlock_init(&context->m_rwlock, NULL))  goto cleanup;

    if ((m_connections = calloc(CONN_ALLOC_INCR, sizeof goat_connection_t *))) {
        context->m_connections_size = CONN_ALLOC_INCR;
        context->m_connections_count = 0;
    }
    else {
        goto cleanup;
    }

    if ((context->m_callbacks = malloc(sizeof goat_callback_t * GOAT_EVENT_LAST))) {
        // FIXME install default callbacks
    }
    else {
        goto cleanup;
    }

    return context;

cleanup:
    if (context->m_callbacks)  free(context->m_callbacks);
    if (context->m_connections)  free(context->m_connections);
    pthread_rwlock_destroy(&context->m_rwlock);
    free(context);
    return NULL;
}

int goat_context_delete(goat_context_t *context) {
    assert(context != NULL);

    if (0 == pthread_rwlock_wrlock(&context->m_rwlock)) {
        assert(context->m_callbacks != NULL);
        free(context->m_callbacks);
        context->m_callbacks = NULL;

        for (size_t i = 0; i < context->m_connections_size; i++) {
            if (context->m_connections[i] != NULL) {
                conn_destroy(context->m_connections[i]);
                context->m_connections[i] = NULL;
                -- context->m_connections_count;
            }
        }
        free(context->m_connections);
        context->m_connections = NULL;
        context->m_connections_size;
        assert(context->m_connections_count == 0);

        pthread_wrlock_unlock(&context->m_rwlock);
        pthread_wrlock_destroy(&context->m_rwlock);
    }
    else {
        return -1;
    }
}

goat_error_t goat_error(goat_context_t *context) {
    assert(context != NULL);
    return context->m_error;
}

int goat_select_fds(goat_context_t *context, fd_set *restrict readfds, fd_set *restrict writefds) {
    assert(context != NULL);
    assert(readfds != NULL);
    assert(writefds != NULL);

    if (0 == pthread_rwlock_rdlock(&context->m_rwlock)) {
        if (context->m_connections_count > 0) {
            for (size_t i = 0; i < m_connections_size; i++) {
                if (m_connections[i] != NULL) {
                    if (conn_wants_read(m_connections[i])) {
                        FD_SET(m_connections[i]->socket, readfds);
                    }
                    if (conn_wants_write(m_connections[i])) {
                        FD_SET(m_connections[i]->socket, writefds);
                    }
                }
            }
        }

        pthread_rwlock_unlock(&context->m_rwlock);
    }
    else {
        return -1;
    }
}

int goat_tick(goat_context_t *context, struct timeval *timeout) {
    fd_set readfds, writefds;
    int nfds = -1;
    int events = 0;

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    if (0 == pthread_rwlock_tryrdlock(&context->m_rwlock)) {
        if (context->m_connections_count > 0) {
            for (size_t i = 0; i < context->m_connections_size; i++) {
                if (context->m_connections[i] != NULL) {
                    goat_connection_t *const conn = context->m_connections[i];

                    if (conn_wants_read(conn)) {
                        nfds = (conn->socket > nfds ? conn->socket : nfds);
                        FD_SET(conn->socket, &readfds);
                    }

                    if (conn_wants_write(conn)) {
                        nfds = (conn->socket > nfds ? conn->socket : nfds);
                        FD_SET(conn->socket, &writefds);
                    }
                }
            }
        }

        pthread_rwlock_unlock(&context->m_rwlock);
    }

    // we unlock here because the select might block indefinitely,
    // and we don't want to do that while holding a lock

    if (select(nfds + 1, &readfds, &writefds, NULL, timeout) >= 0) {
        if (0 == pthread_rwlock_tryrdlock(&context->m_rwlock)) {
            if (context->m_connections_count > 0) {
                for (size_t i = 0; i < context->m_connections_size; i++) {
                    if (context->m_connections[i] != NULL) {
                        goat_connection_t *conn = context->m_connections[i];

                        int read_ready = FD_ISSET(conn->socket, &readfds);
                        int write_ready = FD_ISSET(conn->socket, &writefds);

                        // FIXME advance the connection state machine

                        events += 1; // FIXME
                    }
                }
            }

            pthread_rwlock_unlock(&context->m_rwlock);
        }
    }

    return events;
}

int goat_dispatch_events(goat_context_t *context) {
    assert(context != NULL);

    if (0 == pthread_rwlock_rdlock(&context->m_rwlock)) {
        if (context->m_connections_count > 0) {
            for (size_t i = 0; i < m_connections_size; i++) {
                if (context->m_connections[i] != NULL) {
                    goat_connection_t *const conn = context->m_connections[i];

                    // FIXME process events on the connection, call callbacks
                }
            }
        }

        pthread_rwlock_unlock(&context->m_rwlock);
    }
    else {
        return -1;
    }

    return 0;
}

#if 0

int goat_send_message(
        goat_handle handle,
        const char *restrict prefix,
        const char *restrict command,
        const char **restrict params
) {
    // validate:
    //   prefix/command cannot contain spaces
    //   if a param contains spaces it must be the last param
    //   sum of lengths must be <= 510... long line handling?
    return -1;
}

int goat_install_callback(goat_event event, goat_callback callback) {
    return -1;
}

int goat_uninstall_callback(goat_event event, goat_callback callback) {
    return -1;
}
#endif
