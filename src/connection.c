#include <config.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "connection.h"
#include "message.h"
#include "sm.h"

static ssize_t _conn_recv_data(goat_connection_t *);
static ssize_t _conn_send_data(goat_connection_t *);
static int _conn_enqueue_message(str_queue_head_t *queue, const goat_message_t *message);
static goat_message_t *_conn_dequeue_message(str_queue_head_t *queue);

static const char *const _conn_state_names[] = {
    [GOAT_CONN_DISCONNECTED]    = "disconnected",
    [GOAT_CONN_RESOLVING]       = "resolving",
    [GOAT_CONN_CONNECTING]      = "connecting",
    [GOAT_CONN_CONNECTED]       = "connected",
    [GOAT_CONN_DISCONNECTING]   = "disconnecting",
    [GOAT_CONN_ERROR]           = "error"
};

#define CONN_STATE_ENTER(name)   ST_ENTER(name, void, goat_connection_t *conn)
#define CONN_STATE_EXIT(name)    ST_EXIT(name, void, goat_connection_t *conn)
#define CONN_STATE_EXECUTE(name) ST_EXECUTE(name, goat_conn_state_t, goat_connection_t *conn)

#define CONN_STATE_DECL(name)       \
    static CONN_STATE_ENTER(name);  \
    static CONN_STATE_EXIT(name);   \
    static CONN_STATE_EXECUTE(name)

CONN_STATE_DECL(DISCONNECTED);
CONN_STATE_DECL(RESOLVING);
CONN_STATE_DECL(CONNECTING);
CONN_STATE_DECL(CONNECTED);
CONN_STATE_DECL(DISCONNECTING);
CONN_STATE_DECL(ERROR);

typedef void (*state_enter_function)(goat_connection_t *);
typedef goat_conn_state_t (*state_execute_function)(goat_connection_t *);
typedef void (*state_exit_function)(goat_connection_t *);

static const state_enter_function state_enter[] = {
    ST_ENTER_NAME(DISCONNECTED),
    ST_ENTER_NAME(RESOLVING),
    ST_ENTER_NAME(CONNECTING),
    ST_ENTER_NAME(CONNECTED),
    ST_ENTER_NAME(DISCONNECTING),
    ST_ENTER_NAME(ERROR),
};

static const state_execute_function state_execute[] = {
    ST_EXECUTE_NAME(DISCONNECTED),
    ST_EXECUTE_NAME(RESOLVING),
    ST_EXECUTE_NAME(CONNECTING),
    ST_EXECUTE_NAME(CONNECTED),
    ST_EXECUTE_NAME(DISCONNECTING),
    ST_EXECUTE_NAME(ERROR),
};

static const state_exit_function state_exit[] = {
    ST_EXIT_NAME(DISCONNECTED),
    ST_EXIT_NAME(RESOLVING),
    ST_EXIT_NAME(CONNECTING),
    ST_EXIT_NAME(CONNECTED),
    ST_EXIT_NAME(DISCONNECTING),
    ST_EXIT_NAME(ERROR),
};

//       The OpenSSL ssl library implements the Secure Sockets Layer (SSL v2/v3) and
//       Transport Layer Security (TLS v1) protocols. It provides a rich API which is
//       documented here.
//
//       At first the library must be initialized; see SSL_library_init(3).
//
//       Then an SSL_CTX object is created as a framework to establish TLS/SSL enabled
//       connections (see SSL_CTX_new(3)).  Various options regarding certificates,
//       algorithms etc. can be set in this object.
//
//       When a network connection has been created, it can be assigned to an SSL object.
//       After the SSL object has been created using SSL_new(3), SSL_set_fd(3) or
//       SSL_set_bio(3) can be used to associate the network connection with the object.
//
//       Then the TLS/SSL handshake is performed using SSL_accept(3) or SSL_connect(3)
//       respectively.  SSL_read(3) and SSL_write(3) are used to read and write data on the
//       TLS/SSL connection.  SSL_shutdown(3) can be used to shut down the TLS/SSL
//       connection.

int conn_init(goat_connection_t *conn, int handle) {
    assert(conn != NULL);
    STAILQ_INIT(&conn->m_write_queue);
    STAILQ_INIT(&conn->m_read_queue);
    return -1; // FIXME
}

int conn_destroy(goat_connection_t *conn) {
    assert(conn != NULL);
    return -1; // FIXME
}

int conn_wants_read(const goat_connection_t *conn) {
    assert(conn != NULL);

    switch (conn->m_state.state) {
        case GOAT_CONN_CONNECTING:
        case GOAT_CONN_CONNECTED:
        case GOAT_CONN_DISCONNECTING:
            return 1;

        default:
            return 0;
    }
}

int conn_wants_write(const goat_connection_t *conn) {
    assert(conn != NULL);
    switch (conn->m_state.state) {
        case GOAT_CONN_CONNECTED:
            if (STAILQ_EMPTY(&conn->m_write_queue))  return 0;
            /* fall through */
        case GOAT_CONN_CONNECTING:
            return 1;

        default:
            return 0;
    }
}

int conn_wants_timeout(const goat_connection_t *conn) {
    assert(conn != NULL);
    switch (conn->m_state.state) {
        case GOAT_CONN_RESOLVING:
            return 1;

        default:
            return 0;
    }
}

int conn_tick(goat_connection_t *conn, int socket_readable, int socket_writeable) {
    assert(conn != NULL);

    if (0 == pthread_mutex_lock(&conn->m_mutex)) {
        conn->m_state.socket_is_readable = socket_readable;
        conn->m_state.socket_is_writeable = socket_writeable;
        goat_conn_state_t next_state;
        switch (conn->m_state.state) {
            case GOAT_CONN_DISCONNECTED:
            case GOAT_CONN_RESOLVING:
            case GOAT_CONN_CONNECTING:
            case GOAT_CONN_CONNECTED:
            case GOAT_CONN_DISCONNECTING:
            case GOAT_CONN_ERROR:
                next_state = state_execute[conn->m_state.state](conn);
                if (next_state != conn->m_state.state) {
                    state_exit[conn->m_state.state](conn);

                    const char *params[] = {
                        "changed",
                        "from",
                        _conn_state_names[conn->m_state.state],
                        "to",
                        _conn_state_names[next_state],
                        conn->m_state.change_reason,
                        NULL
                    };

                    goat_message_t *message;
                    if (NULL != (message = message_new(":goat.connection", "state", params))) {
                        _conn_enqueue_message(&conn->m_read_queue, message);
                        message_delete(message);
                    }

                    if (conn->m_state.change_reason)  free(conn->m_state.change_reason);
                    conn->m_state.change_reason = NULL;

                    conn->m_state.state = next_state;
                    state_enter[conn->m_state.state](conn);
                }
                break;

            default:
                assert(0 == "shouldn't get here");
                conn->m_state.error = GOAT_E_STATE;
                conn->m_state.state = GOAT_CONN_ERROR;
                state_enter[conn->m_state.state](conn);
                break;
        }
        pthread_mutex_unlock(&conn->m_mutex);
    }

    if (conn->m_state.state == GOAT_CONN_ERROR)  return -1;

    return !STAILQ_EMPTY(&conn->m_read_queue);  // cheap estimate of number of events
}

int conn_reset_error(goat_connection_t *conn) {
    assert(conn!= NULL);

    if (0 == pthread_mutex_lock(&conn->m_mutex)) {
        conn->m_state.error = GOAT_E_NONE;

        if (conn->m_state.state == GOAT_CONN_ERROR) {
            state_exit[conn->m_state.state](conn);
            conn->m_state.state = GOAT_CONN_DISCONNECTED;
            state_enter[conn->m_state.state](conn);
        }

        pthread_mutex_unlock(&conn->m_mutex);
        return 0;
    }
    else {
        return -1;
    }
}

int conn_send_message(goat_connection_t *conn, const goat_message_t *message) {
    assert(conn != NULL);
    assert(message != NULL);

    if (0 == pthread_mutex_lock(&conn->m_mutex)) {
        // now stick it on the connection's write queue
        int ret = _conn_enqueue_message(&conn->m_write_queue, message);

        pthread_mutex_unlock(&conn->m_mutex);
        return ret;
    }
    else {
        return -1;
    }
}

goat_message_t *conn_recv_message(goat_connection_t *conn) {
    assert(conn != NULL);

    if (0 == pthread_mutex_lock(&conn->m_mutex)) {
        goat_message_t *message = _conn_dequeue_message(&conn->m_read_queue);

        pthread_mutex_unlock(&conn->m_mutex);
        return message;
    }
    else {
        return NULL;
    }
}

ssize_t _conn_send_data(goat_connection_t *conn) {
    assert(conn != NULL && conn->m_state.state == GOAT_CONN_CONNECTED);
    ssize_t total_bytes_sent = 0;

    while (!STAILQ_EMPTY(&conn->m_write_queue)) {
        str_queue_entry_t *node = STAILQ_FIRST(&conn->m_write_queue);

        ssize_t wrote = write(conn->m_network.socket, node->str, node->len);

        if (wrote < 0) {
            int e = errno;
            switch (e) {
                case EAGAIN:
#if EAGAIN != EWOULDBLOCK
                case EWOULDBLOCK:
#endif
                case EINTR:
                    return total_bytes_sent;

                default:
                    return -1;
            }
        }
        else if (wrote == 0) {
            // socket has been disconnected
            return total_bytes_sent ? total_bytes_sent : 0;
        }
        else if (wrote < node->len) {
            // partial write - reinsert the remainder at the queue head for next
            // time the socket is writeable
            STAILQ_REMOVE_HEAD(&conn->m_write_queue, entries);
            total_bytes_sent += wrote;

            size_t len = node->len - wrote;
            str_queue_entry_t *tmp = malloc(sizeof(str_queue_entry_t) + len + 1);
            tmp->len = len;
            tmp->has_eol = node->has_eol;
            strcpy(tmp->str, &node->str[wrote]);

            STAILQ_INSERT_HEAD(&conn->m_write_queue, tmp, entries);

            free(node);
            return total_bytes_sent;
        }
        else {
            // wrote the whole thing, remove it from the queue
            STAILQ_REMOVE_HEAD(&conn->m_write_queue, entries);
            total_bytes_sent += wrote;
        }
    }

    return total_bytes_sent;
}

ssize_t _conn_recv_data(goat_connection_t *conn) {
    assert(conn != NULL && conn->m_state.state == GOAT_CONN_CONNECTED);

    char buf[516] = {0};
    ssize_t bytes, total_bytes_read = 0;

    bytes = read(conn->m_network.socket, buf, sizeof(buf));
    while (bytes > 0) {
        const char * const end = &buf[bytes];
        char *curr = buf, *next = NULL;

        while (curr != end) {
            next = curr;
            while (next != end && *(next++) != '\x0a') ;

            if (*(next - 1) == '\x0a') {
                // found a complete line, queue it
                // if the previously-queued line was incomplete, dequeue it and combine with this
                str_queue_entry_t *const partial = STAILQ_LAST(
                    &conn->m_read_queue,
                    s_str_queue_entry,
                    entries
                );

                size_t partial_len = partial->has_eol ? 0 : partial->len;
                size_t len = next - curr;

                str_queue_entry_t *node = malloc(
                    sizeof(str_queue_entry_t) + partial_len + len + 1
                );
                node->len = partial_len + len;
                node->has_eol = 1;
                memset(node->str, '\0', node->len + 1);

                if (partial_len) {
                    strncat(node->str, partial->str, partial_len);
                    STAILQ_REMOVE(&conn->m_read_queue, partial, s_str_queue_entry, entries);
                    free(partial);
                }

                strncat(node->str, curr, len);

                STAILQ_INSERT_TAIL(&conn->m_read_queue, node, entries);
            }
            else {
                // found a partial line, queue it for completion later
                size_t len = next - curr;

                str_queue_entry_t *node = malloc(sizeof(str_queue_entry_t) + len + 1);
                node->len = len;
                node->has_eol = 0;
                strncpy(node->str, curr, len);
                node->str[len] = '\0';

                STAILQ_INSERT_TAIL(&conn->m_read_queue, node, entries);
            }

            curr = next;
        }

        total_bytes_read += bytes;
        bytes = read(conn->m_network.socket, buf, sizeof(buf));
    }

    return total_bytes_read;
}

int _conn_enqueue_message(str_queue_head_t *queue, const goat_message_t *message) {
    assert(queue != NULL);
    assert(message != NULL);
    // FIXME assert is valid message

    char *tmp;
    str_queue_entry_t *entry;

    if (NULL != (tmp = message_strdup(message))) {
        if (NULL != (entry = malloc(sizeof(str_queue_entry_t) + message->m_len + 1))) {
            entry->len = message->m_len;
            entry->has_eol = 1;
            strcpy(entry->str, tmp);
            STAILQ_INSERT_TAIL(queue, entry, entries);
            free(tmp);
            return 0;
        }
        else {
            free(tmp);
            return -1;
        }
    }
    else {
        return -1;
    }
}

goat_message_t *_conn_dequeue_message(str_queue_head_t *queue) {
    assert(queue != NULL);

    goat_message_t *message = NULL;

    str_queue_entry_t *node = STAILQ_FIRST(queue);
    if (node != NULL && node->has_eol) {
        if (NULL != (message = message_new_from_string(node->str, node->len))) {
            STAILQ_REMOVE_HEAD(queue, entries);
            free(node);
        }
    }

    return message;
}


CONN_STATE_ENTER(DISCONNECTED) { }

CONN_STATE_EXECUTE(DISCONNECTED) {
    assert(conn != NULL && conn->m_state.state == GOAT_CONN_DISCONNECTED);
    // no automatic progression to any other state
    return conn->m_state.state;
}

CONN_STATE_EXIT(DISCONNECTED) { }

CONN_STATE_ENTER(RESOLVING) {
    // set up a resolver and kick it off
}

CONN_STATE_EXECUTE(RESOLVING) {
    assert(conn != NULL && conn->m_state.state == GOAT_CONN_RESOLVING);
    // see if we've got a result yet
    if (0) {
        // got a result!  start connecting
        return GOAT_CONN_CONNECTING;
    }

    return conn->m_state.state;
}

CONN_STATE_EXIT(RESOLVING) {
    // clean up resolver
}

CONN_STATE_ENTER(CONNECTING) {
    // start up a connection attempt
}

CONN_STATE_EXECUTE(CONNECTING) {
    assert(conn != NULL && conn->m_state.state == GOAT_CONN_CONNECTING);
    if (conn->m_state.socket_is_writeable) {
        return GOAT_CONN_CONNECTED;
    }

    return conn->m_state.state;
}

CONN_STATE_EXIT(CONNECTING) { }

CONN_STATE_ENTER(CONNECTED) { }

CONN_STATE_EXECUTE(CONNECTED) {
    assert(conn != NULL && conn->m_state.state == GOAT_CONN_CONNECTED);

    if (conn->m_state.socket_is_readable) {
        if (_conn_recv_data(conn) <= 0) {
            return GOAT_CONN_DISCONNECTING;
        }
    }
    if (conn->m_state.socket_is_writeable) {
        if (_conn_send_data(conn) <= 0) {
            return GOAT_CONN_DISCONNECTING;
        }
    }

    return conn->m_state.state;
}

CONN_STATE_EXIT(CONNECTED) { }

CONN_STATE_ENTER(DISCONNECTING) { }

CONN_STATE_EXECUTE(DISCONNECTING) {
    assert(conn != NULL && conn->m_state.state == GOAT_CONN_DISCONNECTING);
    // any processing we need to do during disconnect

    str_queue_entry_t *n1, *n2;
    n1 = STAILQ_FIRST(&conn->m_read_queue);
    while (n1 != NULL) {
        n2 = STAILQ_NEXT(n1, entries);
        free(n1);
        n1 = n2;
    }
    STAILQ_INIT(&conn->m_read_queue);

    n1 = STAILQ_FIRST(&conn->m_write_queue);
    while (n1 != NULL) {
        n2 = STAILQ_NEXT(n1, entries);
        free(n1);
        n1 = n2;
    }
    STAILQ_INIT(&conn->m_write_queue);

    return GOAT_CONN_DISCONNECTED;
}

CONN_STATE_EXIT(DISCONNECTING) { }

CONN_STATE_ENTER(ERROR) { }

CONN_STATE_EXECUTE(ERROR) {
    assert(conn != NULL && conn->m_state.state == GOAT_CONN_ERROR);


    return GOAT_CONN_ERROR;
}

CONN_STATE_EXIT(ERROR) {
    // FIXME recover to newly-initialised state
}
