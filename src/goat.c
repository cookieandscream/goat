#include <config.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "goat.h"

#include "connection.h"
#include "context.h"
#include "error.h"
#include "event.h"
#include "irc.h"

const size_t CONN_ALLOC_INCR = 16;

static int _tls_init() {
    static pthread_mutex_t _tls_init_mutex = PTHREAD_MUTEX_INITIALIZER;
    int ret;

    if (0 == (ret = pthread_mutex_lock(&_tls_init_mutex))) {
        ret = tls_init();
        pthread_mutex_unlock(&_tls_init_mutex);
    }

    return ret;
}

GoatContext *goat_context_new() {
    // make sure TLS library is initialised
    if (0 != _tls_init()) return NULL;

    // we don't need to lock in here, because no other thread has a pointer to this context yet
    GoatContext *context = calloc(1, sizeof(GoatContext));
    if (!context)  return NULL;

    if (0 != pthread_rwlock_init(&context->m_rwlock, NULL))  goto cleanup;

    if ((context->m_connections = calloc(CONN_ALLOC_INCR, sizeof(Connection *)))) {
        context->m_connections_size = CONN_ALLOC_INCR;
        context->m_connections_count = 0;
    }
    else {
        goto cleanup;
    }

    if (NULL == (context->m_callbacks = calloc(GOAT_EVENT_LAST, sizeof(GoatCallback)))) {
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

int goat_context_delete(GoatContext *context) {
    assert(context != NULL);

    int r = pthread_rwlock_wrlock(&context->m_rwlock);
    if (r) return r;

    assert(context->m_callbacks != NULL);
    free(context->m_callbacks);
    context->m_callbacks = NULL;

    for (size_t i = 0; i < context->m_connections_size; i++) {
        if (context->m_connections[i] != NULL) {
            Connection *conn = context->m_connections[i];
            context->m_connections[i] = NULL;
            -- context->m_connections_count;

            conn_destroy(conn);
            memset(conn, 0, sizeof(Connection));
            free(conn);
        }
    }
    free(context->m_connections);
    context->m_connections = NULL;
    context->m_connections_size = 0;
    assert(context->m_connections_count == 0);

    if (context->m_tls_config) tls_config_free(context->m_tls_config);

    pthread_rwlock_unlock(&context->m_rwlock);
    pthread_rwlock_destroy(&context->m_rwlock);
    free(context);

    return 0;
}

GoatError goat_error(const GoatContext *context, GoatConnection connection) {
    assert(context != NULL);

    if (context == NULL) return EINVAL;
    if (connection < 0) return EINVAL;
    if ((size_t) connection > context->m_connections_count) return EINVAL;
    if (context->m_connections[connection] == NULL) return EINVAL;

    return context->m_connections[connection]->m_state.error;
}

const char *goat_strerror(int error) {
    if (error >= GOAT_E_FIRST && error < GOAT_E_LAST)  return error_strings[error];
    if (error < ELAST) return strerror(error);
    return NULL;
}

int goat_reset_error(GoatContext *context, int connection) {
    assert(context != NULL);

    if (context == NULL)  return EINVAL;
    if (connection < 0)  return EINVAL;
    if ((size_t) connection >= context->m_connections_size)  return EINVAL;
    if (context->m_connections[connection] == NULL)  return EINVAL;

    return conn_reset_error(context->m_connections[connection]);
}

GoatError goat_connection_new(GoatContext *context, GoatConnection *connection) {
    assert(context != NULL);
    if (context == NULL) return EINVAL;

    int handle = -1;
    int r;

    r = pthread_rwlock_wrlock(&context->m_rwlock);
    if (r) return r;

    if (context->m_connections_count == context->m_connections_size) {
        size_t new_size = context->m_connections_size + CONN_ALLOC_INCR;

        Connection **tmp = calloc(new_size, sizeof(Connection *));
        if (NULL == tmp) {
            r = errno;
            goto done;
        }

        memcpy(tmp, context->m_connections, context->m_connections_size * sizeof(Connection *));
        free(context->m_connections);
        context->m_connections = tmp;
        context->m_connections_size = new_size;
    }

    Connection *conn = malloc(sizeof(Connection));
    if (NULL == conn) {
        r = errno;
        goto done;
    }

    r = conn_init(conn);
    if (r) {
        free(conn);
        goto done;
    }

    handle = context->m_connections_count ++;
    context->m_connections[handle] = conn;

done:
    pthread_rwlock_unlock(&context->m_rwlock);

    if (r == 0 && handle >= 0) *connection = handle;
    return r;
}

GoatError goat_connection_delete(GoatContext *context, GoatConnection *connection) {
    assert(context != NULL);
    assert(connection != NULL);
    assert(connection >= 0);
    assert((size_t) *connection < context->m_connections_size);

    if (NULL == context) return EINVAL;
    if (NULL == connection) return EINVAL;
    if (*connection < 0) return EINVAL;
    if ((size_t) *connection >= context->m_connections_size) return EINVAL;

    int r = 0;

    r = pthread_rwlock_wrlock(&context->m_rwlock);
    if (r) return r;

    if (NULL == context->m_connections[*connection]) {
        r = EINVAL;
        goto done;
    }

    Connection *const tmp = context->m_connections[*connection];

    context->m_connections[*connection] = NULL;
    -- context->m_connections_count;
    *connection = -1;

    conn_destroy(tmp);
    free(tmp);

done:
    pthread_rwlock_unlock(&context->m_rwlock);
    return r;
}

GoatError goat_connect(GoatContext *context, int connection,
    const char *hostname, const char *servname, int ssl
) {
    if (NULL == context) return EINVAL;

    Connection *conn = context_get_connection(context, connection);

    if (NULL == conn) return EINVAL;

    return conn_connect(conn, hostname, servname, ssl);
}

GoatError goat_disconnect(GoatContext *context, int connection) {
    if (NULL == context) return EINVAL;

    Connection *conn = context_get_connection(context, connection);

    if (NULL == conn) return EINVAL;

    return conn_disconnect(conn);
}

// use this to get fdsets to select on from your app, if you have your own
// fds to block on as well
GoatError goat_select_fds(GoatContext *context,
    fd_set *restrict readfds, fd_set *restrict writefds
) {
    assert(context != NULL);

    if (NULL == context) return EINVAL;

    int r = pthread_rwlock_rdlock(&context->m_rwlock);
    if (r) return r;

    if (context->m_connections_count > 0) {
        for (size_t i = 0; i < context->m_connections_size; i++) {
            if (context->m_connections[i] != NULL) {
                Connection *const conn = context->m_connections[i];

                if (NULL != readfds && conn_wants_read(conn)) {
                    FD_SET(conn->m_network.socket, readfds);
                }
                if (NULL != writefds && conn_wants_write(conn)) {
                    FD_SET(conn->m_network.socket, writefds);
                }
            }
        }
    }

    pthread_rwlock_unlock(&context->m_rwlock);
    return 0;
}

int goat_tick(GoatContext *context, struct timeval *timeout) {
    fd_set readfds, writefds;
    int nfds = -1;
    int events = 0;

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    if (0 == pthread_rwlock_tryrdlock(&context->m_rwlock)) {
        if (context->m_connections_count > 0) {
            for (size_t i = 0; i < context->m_connections_size; i++) {
                if (context->m_connections[i] != NULL) {
                    Connection *const conn = context->m_connections[i];

                    if (conn_wants_read(conn)) {
                        nfds = (conn->m_network.socket > nfds ? conn->m_network.socket : nfds);
                        FD_SET(conn->m_network.socket, &readfds);
                    }

                    if (conn_wants_write(conn)) {
                        nfds = (conn->m_network.socket > nfds ? conn->m_network.socket : nfds);
                        FD_SET(conn->m_network.socket, &writefds);
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
                        Connection *const conn = context->m_connections[i];

                        int read_ready = FD_ISSET(conn->m_network.socket, &readfds);
                        int write_ready = FD_ISSET(conn->m_network.socket, &writefds);

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

GoatError goat_dispatch_events(GoatContext *context) {
    assert(context != NULL);

    if (NULL == context) return EINVAL;

    int r = pthread_rwlock_rdlock(&context->m_rwlock);
    if (r) return r;

    if (context->m_connections_count > 0) {
        for (size_t i = 0; i < context->m_connections_size; i++) {
            if (context->m_connections[i] != NULL) {
                Connection *const conn = context->m_connections[i];

                GoatMessage *message;
                while ((message = conn_recv_message(conn))) {
                    event_process(context, i, message);
                    goat_message_delete(message);
                }
            }
        }
    }

    pthread_rwlock_unlock(&context->m_rwlock);
    return 0;
}

GoatError goat_install_callback(GoatContext *context, GoatEvent event, GoatCallback callback) {
    assert(context != NULL);
    assert(event >= GOAT_EVENT_GENERIC);
    assert(event < GOAT_EVENT_LAST);

    if (NULL == context) return EINVAL;
    if (event < GOAT_EVENT_GENERIC) return EINVAL;
    if (event >= GOAT_EVENT_LAST) return EINVAL;

    int r = pthread_rwlock_wrlock(&context->m_rwlock);
    if (r) return r;

    context->m_callbacks[event] = callback;

    pthread_rwlock_unlock(&context->m_rwlock);
    return 0;
}

GoatError goat_uninstall_callback(GoatContext *context, GoatEvent event, GoatCallback callback) {
    assert(context != NULL);
    assert(event >= GOAT_EVENT_GENERIC);
    assert(event < GOAT_EVENT_LAST);

    if (NULL == context) return EINVAL;
    if (event < GOAT_EVENT_GENERIC) return EINVAL;
    if (event >= GOAT_EVENT_LAST) return EINVAL;

    int r = pthread_rwlock_wrlock(&context->m_rwlock);
    if (r) return r;

    if (context->m_callbacks[event] == callback) {
        context->m_callbacks[event] = NULL;
    }
    else {
        r = ECANCELED;
    }

    pthread_rwlock_unlock(&context->m_rwlock);
    return r;
}

GoatError goat_send_message(GoatContext *context, GoatConnection connection, const GoatMessage *message) {
    if (NULL == context) return EINVAL;
    if (NULL == message) return EINVAL;

    Connection *conn = context_get_connection(context, connection);
    if (NULL == conn) return EINVAL;

    return conn_send_message(conn, message);
}
