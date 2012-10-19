#include <config.h>

#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "goat.h"

#include "connection.h"
#include "error.h"
#include "event.h"

const size_t CONN_ALLOC_INCR = 16;

struct s_goat_context {
    pthread_rwlock_t    m_rwlock;
    goat_connection_t   **m_connections;
    size_t              m_connections_size;
    size_t              m_connections_count;
    goat_callback_t     *m_callbacks;
    goat_error_t        m_error;
};

goat_context_t *goat_context_new() {
    // we don't need to lock in here, because no other thread has a pointer to this context yet
    goat_context_t *context = calloc(1, sizeof(goat_context_t));
    if (!context)  return NULL;

    if (0 != pthread_rwlock_init(&context->m_rwlock, NULL))  goto cleanup;

    if ((context->m_connections = calloc(CONN_ALLOC_INCR, sizeof(goat_connection_t *)))) {
        context->m_connections_size = CONN_ALLOC_INCR;
        context->m_connections_count = 0;
    }
    else {
        goto cleanup;
    }

    if ((context->m_callbacks = malloc(sizeof(goat_callback_t) * GOAT_EVENT_LAST))) {
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
        context->m_connections_size = 0;
        assert(context->m_connections_count == 0);

        pthread_rwlock_unlock(&context->m_rwlock);
        pthread_rwlock_destroy(&context->m_rwlock);
        return 0;
    }
    else {
        return -1;
    }
}

goat_error_t goat_error(goat_context_t *context, int connection) {
    assert(context != NULL);

    if (context == NULL)  return GOAT_E_ERRORINV;
    if (connection < 0)   return context->m_error;
    if (connection >= context->m_connections_size)  return GOAT_E_ERRORINV;
    if (context->m_connections[connection] == NULL)  return GOAT_E_ERRORINV;

    return context->m_connections[connection]->m_error;
}

const char *goat_strerror(goat_error_t error) {
    assert(error >= GOAT_E_NONE);
    assert(error < GOAT_E_LAST);
    if (error >= GOAT_E_NONE && error < GOAT_E_LAST)  return error_strings[error];
    return NULL;
}

int goat_reset_error(goat_context_t *context, int connection) {
    assert(context != NULL);

    if (context == NULL)  return -1;
    if (connection < 0)  context->m_error = GOAT_E_NONE;
    if (connection >= context->m_connections_size)  return -1;
    if (context->m_connections[connection] == NULL)  return -1;

    return conn_reset_error(context->m_connections[connection]);
}

int goat_connection_new(goat_context_t *context) {
    assert(context != NULL);
    goat_connection_t *conn;
    int handle = -1;

    if (0 == pthread_rwlock_wrlock(&context->m_rwlock)) {
        if (context->m_connections_count == context->m_connections_size) {
            size_t new_size = context->m_connections_size + CONN_ALLOC_INCR;
            goat_connection_t **tmp;
            if (NULL != (tmp = calloc(new_size, sizeof(goat_connection_t *)))) {
                memcpy(
                    tmp,
                    context->m_connections,
                    context->m_connections_size * sizeof(goat_connection_t *)
                );
                free(context->m_connections);
                context->m_connections = tmp;
                context->m_connections_size = new_size;
            }
            else {
                pthread_rwlock_unlock(&context->m_rwlock);
                return -1;
            }
        }

        if (NULL != (conn = malloc(sizeof(goat_connection_t)))) {
            if (0 == conn_init(conn, context->m_connections_count)) {
                handle = context->m_connections_count;
                context->m_connections[handle] = conn;
                ++ context->m_connections_count;
            }
        }

        pthread_rwlock_unlock(&context->m_rwlock);
    }

    return handle;
}

int goat_connection_delete(goat_context_t *context, int connection) {
    assert(context != NULL);
    assert(connection >= 0);
    assert(connection < context->m_connections_size);

    if (0 == pthread_rwlock_wrlock(&context->m_rwlock)) {
        if (context->m_connections[connection] != NULL) {
            goat_connection_t *const conn = context->m_connections[connection];

            context->m_connections[connection] = NULL;
            -- context->m_connections_count;

            pthread_rwlock_unlock(&context->m_rwlock);

            conn_destroy(conn);
            free(conn);
            return 0;
        }
        else {
            pthread_rwlock_unlock(&context->m_rwlock);
            return -1;
        }
    }
    else {
        return -1;
    }
}

int goat_select_fds(goat_context_t *context, fd_set *restrict readfds, fd_set *restrict writefds) {
    assert(context != NULL);
    assert(readfds != NULL);
    assert(writefds != NULL);

    if (0 == pthread_rwlock_rdlock(&context->m_rwlock)) {
        if (context->m_connections_count > 0) {
            for (size_t i = 0; i < context->m_connections_size; i++) {
                if (context->m_connections[i] != NULL) {
                    goat_connection_t *const conn = context->m_connections[i];
                    if (conn_wants_read(conn)) {
                        FD_SET(conn->m_socket, readfds);
                    }
                    if (conn_wants_write(conn)) {
                        FD_SET(conn->m_socket, writefds);
                    }
                }
            }
        }

        pthread_rwlock_unlock(&context->m_rwlock);
        return 0;
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
                        nfds = (conn->m_socket > nfds ? conn->m_socket : nfds);
                        FD_SET(conn->m_socket, &readfds);
                    }

                    if (conn_wants_write(conn)) {
                        nfds = (conn->m_socket > nfds ? conn->m_socket : nfds);
                        FD_SET(conn->m_socket, &writefds);
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
                        goat_connection_t *const conn = context->m_connections[i];

                        int read_ready = FD_ISSET(conn->m_socket, &readfds);
                        int write_ready = FD_ISSET(conn->m_socket, &writefds);

                        int conn_events = conn_tick(conn, read_ready, write_ready);

                        if (conn_events > 0)  events += conn_events;
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
            for (size_t i = 0; i < context->m_connections_size; i++) {
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
