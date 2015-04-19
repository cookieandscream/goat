#include <config.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "connection.h"
#include "context.h"
#include "message.h"
#include "sm.h"
#include "tresolver.h"

static ssize_t _conn_recv_data(Connection *);
static ssize_t _conn_send_data(Connection *);
static int _conn_enqueue_message(StrQueueHead *queue, const GoatMessage *message);
static GoatMessage *_conn_dequeue_message(StrQueueHead *queue);
static void _conn_set_state(Connection *conn, ConnState new_state);
static int _conn_start_connect(Connection *conn, const struct addrinfo *ai);

static const char *const _conn_state_names[] = {
    [GOAT_CONN_DISCONNECTED]    = "disconnected",
    [GOAT_CONN_RESOLVING]       = "resolving",
    [GOAT_CONN_CONNECTING]      = "connecting",
    [GOAT_CONN_SSLHANDSHAKE]    = "ssl handshake",
    [GOAT_CONN_CONNECTED]       = "connected",
    [GOAT_CONN_DISCONNECTING]   = "disconnecting",
    [GOAT_CONN_ERROR]           = "error"
};

#define CONN_STATE_ENTER(name)   ST_ENTER(name, int, Connection *conn)
#define CONN_STATE_EXIT(name)    ST_EXIT(name, void, Connection *conn)
#define CONN_STATE_EXECUTE(name) ST_EXECUTE(name, ConnState, Connection *conn)

#define CONN_STATE_DECL(name)       \
    static CONN_STATE_ENTER(name);  \
    static CONN_STATE_EXIT(name);   \
    static CONN_STATE_EXECUTE(name)

CONN_STATE_DECL(DISCONNECTED);
CONN_STATE_DECL(RESOLVING);
CONN_STATE_DECL(CONNECTING);
CONN_STATE_DECL(SSLHANDSHAKE);
CONN_STATE_DECL(CONNECTED);
CONN_STATE_DECL(DISCONNECTING);
CONN_STATE_DECL(ERROR);

typedef int (*StateEnterFunction)(Connection *);
typedef ConnState (*StateExecuteFunction)(Connection *);
typedef void (*StateExitFunction)(Connection *);

static const StateEnterFunction state_enter[] = {
    ST_ENTER_NAME(DISCONNECTED),
    ST_ENTER_NAME(RESOLVING),
    ST_ENTER_NAME(CONNECTING),
    ST_ENTER_NAME(SSLHANDSHAKE),
    ST_ENTER_NAME(CONNECTED),
    ST_ENTER_NAME(DISCONNECTING),
    ST_ENTER_NAME(ERROR),
};

static const StateExecuteFunction state_execute[] = {
    ST_EXECUTE_NAME(DISCONNECTED),
    ST_EXECUTE_NAME(RESOLVING),
    ST_EXECUTE_NAME(CONNECTING),
    ST_EXECUTE_NAME(SSLHANDSHAKE),
    ST_EXECUTE_NAME(CONNECTED),
    ST_EXECUTE_NAME(DISCONNECTING),
    ST_EXECUTE_NAME(ERROR),
};

static const StateExitFunction state_exit[] = {
    ST_EXIT_NAME(DISCONNECTED),
    ST_EXIT_NAME(RESOLVING),
    ST_EXIT_NAME(CONNECTING),
    ST_EXIT_NAME(SSLHANDSHAKE),
    ST_EXIT_NAME(CONNECTED),
    ST_EXIT_NAME(DISCONNECTING),
    ST_EXIT_NAME(ERROR),
};

// A tls connection is represented as a context. A new context is created by
// either the tls_client() or tls_server() functions. The context can then be
// configured with the function tls_configure(). The same tls_config object
// can be used to configure multiple contexts.

// A client connection is initiated after configuration by calling
// tls_connect(). This function will create a new socket, connect to the
// specified host and port, and then establish a secure connection. The
// tls_connect_servername() function has the same behaviour, however the name
// to use for verification is explicitly provided, rather than being inferred
// from the host value. An already existing socket can be upgraded to a secure
// connection by calling tls_connect_socket(). Alternatively, a secure
// connection can be established over a pair of existing file descriptors by
// calling tls_connect_fds().

// A server can accept a new client connection by calling tls_accept_socket()
// on an already established socket connection.

// Two functions are provided for input and output, tls_read() and tls_write().

// After use, a tls context should be closed with tls_close(), and then freed
// by calling tls_free(). When no more contexts are to be created, the
// tls_config object should be freed by calling tls_config_free().

int conn_init(Connection *conn) {
    assert(conn != NULL);

    memset(conn, 0, sizeof(Connection));

    conn->m_network.socket = -1;

    STAILQ_INIT(&conn->m_write_queue);
    STAILQ_INIT(&conn->m_read_queue);

    return pthread_mutex_init(&conn->m_mutex, NULL);
}

int conn_destroy(Connection *conn) {
    assert(conn != NULL);

    int ret;

    if (0 == (ret = pthread_mutex_lock(&conn->m_mutex))) {
        state_exit[conn->m_state.state](conn);
        if (conn->m_state.change_reason) free(conn->m_state.change_reason);

        if (conn->m_network.hostname) free(conn->m_network.hostname);
        if (conn->m_network.servname) free(conn->m_network.servname);
        if (conn->m_network.ai0) freeaddrinfo(conn->m_network.ai0);
        if (conn->m_network.tls) tls_free(conn->m_network.tls);

        StrQueueEntry *node = STAILQ_FIRST(&conn->m_write_queue);
        while (NULL != node) {
            StrQueueEntry *next = STAILQ_NEXT(node, entries);
            free(node);
            node = next;
        }
        STAILQ_INIT(&conn->m_write_queue);

        node = STAILQ_FIRST(&conn->m_read_queue);
        while (NULL != node) {
            StrQueueEntry *next = STAILQ_NEXT(node, entries);
            free(node);
            node = next;
        }
        STAILQ_INIT(&conn->m_read_queue);

        pthread_mutex_unlock(&conn->m_mutex);
        ret = pthread_mutex_destroy(&conn->m_mutex);
    }

    return ret;
}

int conn_connect(Connection *conn, const char *hostname, const char *servname, int ssl) {
    assert(conn != NULL);
    assert(conn->m_state.state == GOAT_CONN_DISCONNECTED); // FIXME make this an error

    int r = pthread_mutex_lock(&conn->m_mutex);
    if (r) return r;

    conn->m_network.hostname = strdup(hostname);
    conn->m_network.servname = strdup(servname);
    conn->m_use_ssl = ssl;

    conn->m_state.change_reason = strdup("connect requested by client");
    _conn_set_state(conn, GOAT_CONN_RESOLVING);

    pthread_mutex_unlock(&conn->m_mutex);
    return 0;
}

int conn_disconnect(Connection *conn) {
    assert(conn != NULL);
    // FIXME check the current state?

    int r = pthread_mutex_lock(&conn->m_mutex);
    if (r) return r;

    conn->m_state.change_reason = strdup("disconnect requested by client");
    _conn_set_state(conn, GOAT_CONN_DISCONNECTING);

    pthread_mutex_unlock(&conn->m_mutex);
    return 0;
}

int conn_wants_read(const Connection *conn) {
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

int conn_wants_write(const Connection *conn) {
    assert(conn != NULL);
    switch (conn->m_state.state) {
        case GOAT_CONN_CONNECTED:
            if (STAILQ_EMPTY(&conn->m_write_queue))  return 0;
            /* fall through */
        case GOAT_CONN_CONNECTING:
        case GOAT_CONN_DISCONNECTING:
            return 1;

        default:
            return 0;
    }
}

int conn_wants_timeout(const Connection *conn) {
    assert(conn != NULL);
    switch (conn->m_state.state) {
        case GOAT_CONN_RESOLVING:
            return 1;

        default:
            return 0;
    }
}

int conn_tick(Connection *conn, int socket_readable, int socket_writeable) {
    assert(conn != NULL);

    if (0 == pthread_mutex_lock(&conn->m_mutex)) {
        conn->m_state.socket_is_readable = socket_readable;
        conn->m_state.socket_is_writeable = socket_writeable;
        ConnState next_state;
        switch (conn->m_state.state) {
            case GOAT_CONN_DISCONNECTED:
            case GOAT_CONN_RESOLVING:
            case GOAT_CONN_CONNECTING:
            case GOAT_CONN_SSLHANDSHAKE:
            case GOAT_CONN_CONNECTED:
            case GOAT_CONN_DISCONNECTING:
            case GOAT_CONN_ERROR:
                next_state = state_execute[conn->m_state.state](conn);
                if (next_state != conn->m_state.state) {
                    _conn_set_state(conn, next_state);
                }
                break;

            default:
                assert(0 == "shouldn't get here");
                conn->m_state.error = GOAT_E_STATE;
                _conn_set_state(conn, GOAT_CONN_ERROR);
                break;
        }
        pthread_mutex_unlock(&conn->m_mutex);
    }

    if (conn->m_state.state == GOAT_CONN_ERROR)  return -1;

    return !STAILQ_EMPTY(&conn->m_read_queue);  // cheap estimate of number of events
}

int conn_reset_error(Connection *conn) {
    assert(conn!= NULL);

    if (0 == pthread_mutex_lock(&conn->m_mutex)) {
        conn->m_state.error = GOAT_E_NONE;

        if (conn->m_state.state == GOAT_CONN_ERROR) {
            _conn_set_state(conn, GOAT_CONN_DISCONNECTED);
        }

        pthread_mutex_unlock(&conn->m_mutex);
        return 0;
    }
    else {
        return -1;
    }
}

int conn_send_message(Connection *conn, const GoatMessage *message) {
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

GoatMessage *conn_recv_message(Connection *conn) {
    assert(conn != NULL);

    if (0 == pthread_mutex_lock(&conn->m_mutex)) {
        GoatMessage *message = _conn_dequeue_message(&conn->m_read_queue);

        pthread_mutex_unlock(&conn->m_mutex);
        return message;
    }
    else {
        return NULL;
    }
}

void _conn_set_state(Connection *conn, ConnState new_state) {
    assert(conn != NULL);

    if (conn->m_state.state == new_state) return;

    ConnState old_state = conn->m_state.state;

    state_exit[old_state](conn);

    if (0 != state_enter[new_state](conn)) {
        // FIXME how to handle old change reason?
        if (conn->m_state.change_reason) free(conn->m_state.change_reason);
        conn->m_state.change_reason = strdup("state change failed");

        new_state = GOAT_CONN_ERROR;
        state_enter[GOAT_CONN_ERROR](conn);
    }

    conn->m_state.state = new_state;

    const char *params[] = {
        "changed",
        "from",
        _conn_state_names[old_state],
        "to",
        _conn_state_names[new_state],
        conn->m_state.change_reason,
        NULL
    };

    GoatMessage *message;
    if (NULL != (message = goat_message_new(":goat.connection", "state", params))) {
        _conn_enqueue_message(&conn->m_read_queue, message);
        goat_message_delete(message);
    }

    if (conn->m_state.change_reason)  free(conn->m_state.change_reason);
    conn->m_state.change_reason = NULL;
}

ssize_t _conn_send_data(Connection *conn) {
    assert(conn != NULL && conn->m_state.state == GOAT_CONN_CONNECTED);
    ssize_t total_bytes_sent = 0;

    while (!STAILQ_EMPTY(&conn->m_write_queue)) {
        StrQueueEntry *node = STAILQ_FIRST(&conn->m_write_queue);

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
        else if ((size_t) wrote < node->len) {
            // partial write - reinsert the remainder at the queue head for next
            // time the socket is writeable
            STAILQ_REMOVE_HEAD(&conn->m_write_queue, entries);
            total_bytes_sent += wrote;

            size_t len = node->len - wrote;
            StrQueueEntry *tmp = malloc(sizeof(StrQueueEntry) + len + 1);
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

ssize_t _conn_recv_data(Connection *conn) {
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
                StrQueueEntry *const partial = STAILQ_LAST(
                    &conn->m_read_queue,
                    str_queue_entry,
                    entries
                );

                size_t partial_len = partial->has_eol ? 0 : partial->len;
                size_t len = next - curr;

                StrQueueEntry *node = malloc(
                    sizeof(StrQueueEntry) + partial_len + len + 1
                );
                node->len = partial_len + len;
                node->has_eol = 1;
                memset(node->str, '\0', node->len + 1);

                if (partial_len) {
                    strncat(node->str, partial->str, partial_len);
                    STAILQ_REMOVE(&conn->m_read_queue, partial, str_queue_entry, entries);
                    free(partial);
                }

                strncat(node->str, curr, len);

                STAILQ_INSERT_TAIL(&conn->m_read_queue, node, entries);
            }
            else {
                // found a partial line, queue it for completion later
                size_t len = next - curr;

                StrQueueEntry *node = malloc(sizeof(StrQueueEntry) + len + 1);
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

int _conn_enqueue_message(StrQueueHead *queue, const GoatMessage *message) {
    assert(queue != NULL);
    assert(message != NULL);
    // FIXME assert is valid message

    char *tmp;
    size_t len;
    StrQueueEntry *entry;

    if (NULL != (tmp = goat_message_strdup(message))) {
        len = strlen(tmp) + 2;  // crlf
        if (NULL != (entry = malloc(sizeof(StrQueueEntry) + len + 1))) {
            entry->len = len;
            entry->has_eol = 1;
            snprintf(entry->str, len + 1, "%s\x0d\x0a", tmp);
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

GoatMessage *_conn_dequeue_message(StrQueueHead *queue) {
    assert(queue != NULL);

    GoatMessage *message = NULL;

    StrQueueEntry *node = STAILQ_FIRST(queue);
    if (node != NULL && node->has_eol) {
        if (NULL != (message = goat_message_new_from_string(node->str, node->len))) {
            STAILQ_REMOVE_HEAD(queue, entries);
            free(node);
        }
    }

    return message;
}

int _conn_start_connect(Connection *conn, const struct addrinfo *ai) {
    assert(conn != NULL);
    assert(ai != NULL);

    conn->m_network.socket = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);

    if (conn->m_network.socket < 0) return errno;

    int ret = connect(conn->m_network.socket, ai->ai_addr, ai->ai_addrlen);
    int err = errno;

    if (ret == 0 || err == EALREADY || err == EINPROGRESS)  return 0;

    return err;
}


CONN_STATE_ENTER(DISCONNECTED) { ST_UNUSED(conn); return 0; }

CONN_STATE_EXECUTE(DISCONNECTED) {
    assert(conn != NULL && conn->m_state.state == GOAT_CONN_DISCONNECTED);
    // no automatic progression to any other state
    return conn->m_state.state;
}

CONN_STATE_EXIT(DISCONNECTED) { ST_UNUSED(conn); }

CONN_STATE_ENTER(RESOLVING) {
    assert(conn != NULL);
    assert(conn->m_state.data.raw == NULL);

    conn->m_state.data.resolving = NULL;

    if (conn->m_network.ai0) {
        freeaddrinfo(conn->m_network.ai0);
        conn->m_network.ai0 = NULL;
    }

    return 0;
}

CONN_STATE_EXECUTE(RESOLVING) {
    assert(conn != NULL && conn->m_state.state == GOAT_CONN_RESOLVING);

    int r = resolver_getaddrinfo(
        &conn->m_state.data.resolving,
        conn->m_network.hostname,
        conn->m_network.servname,
        &conn->m_network.ai0
    );

    if (r != 0) {
        conn->m_state.change_reason = strdup(gai_strerror(r));
        return GOAT_CONN_ERROR;
    }

    if (conn->m_network.ai0) {
        // got a result
        return GOAT_CONN_CONNECTING;
    }

    return conn->m_state.state;
}

CONN_STATE_EXIT(RESOLVING) {
    assert(conn->m_state.state == GOAT_CONN_RESOLVING);

    if (conn->m_state.data.resolving) {
        // if there's still resolve state around, then we're exiting this state for
        // some reason other than completion of the resolve request, so explicitly
        // cancel it
        resolver_cancel(&conn->m_state.data.resolving);
    }
}

CONN_STATE_ENTER(CONNECTING) {
    // start up a connection attempt
    assert(conn != NULL);
    assert(conn->m_state.data.raw == NULL);
    assert(conn->m_network.ai0 != NULL);

    conn->m_state.data.connecting = calloc(1, sizeof(ConnectingStateData));
    if (NULL == conn->m_state.data.connecting) {
        return -1;
    }

    conn->m_state.data.connecting->ai = conn->m_network.ai0;

    int ret = _conn_start_connect(conn, conn->m_state.data.connecting->ai);

    if (0 != ret) {
        free(conn->m_state.data.connecting);
        conn->m_state.data.connecting = NULL;
        return ret;
    }

    return 0;
}

CONN_STATE_EXECUTE(CONNECTING) {
    assert(conn != NULL && conn->m_state.state == GOAT_CONN_CONNECTING);
    if (conn->m_state.socket_is_writeable) {
        int err;
        socklen_t errsize = sizeof(err);

        // "writeable" socket means connect() finished
        // getsockopt() can tell us whether it actually connected or not

        if (0 == getsockopt(conn->m_network.socket, SOL_SOCKET, SO_ERROR, &err, &errsize)) {
            if (err) {
                if (err == EALREADY || err == EINPROGRESS) {
                    // connect hasn't finished but for some reason we're writeable?
                    assert(0 == "shouldn't get here?");
                    // just keep waiting for it to finish?
                    return conn->m_state.state;
                }

                // connect failed -- try the next address if there is one
                if (conn->m_state.data.connecting->ai->ai_next != NULL) {
                    conn->m_state.data.connecting->ai =
                        conn->m_state.data.connecting->ai->ai_next;

                    // FIXME send a message about trying again

                    err = _conn_start_connect(conn, conn->m_state.data.connecting->ai);
                    if (0 == err) return conn->m_state.state;
                }

                conn->m_state.change_reason = strdup(strerror(err));
                return GOAT_CONN_ERROR;
            }

            if (conn->m_use_ssl)  return GOAT_CONN_SSLHANDSHAKE;

            return GOAT_CONN_CONNECTED;
        }

        // getsockopt itself failed, that's unexpected...
        conn->m_state.change_reason = strdup(strerror(errno));
        return GOAT_CONN_ERROR;
    }

    return conn->m_state.state;
}

CONN_STATE_EXIT(CONNECTING) {
    assert(conn != NULL);
    assert(conn->m_state.state == GOAT_CONN_CONNECTING);
    assert(conn->m_state.data.raw != NULL);
    free(conn->m_state.data.connecting);
    conn->m_state.data.connecting = NULL;
}

CONN_STATE_ENTER(SSLHANDSHAKE) {
    assert(conn != NULL && conn->m_state.state == GOAT_CONN_SSLHANDSHAKE);

    assert(conn->m_network.tls == NULL);

    struct tls *tls = tls_client();
    tls = tls_client();
    if (NULL == tls) {
        return -1;
    }

    if (0 != tls_configure(tls, NULL)) {
        tls_free(tls);
        return -1;
    }

    conn->m_network.tls = tls;
    return 0;
}

CONN_STATE_EXECUTE(SSLHANDSHAKE) {
    assert(conn != NULL && conn->m_state.state == GOAT_CONN_SSLHANDSHAKE);
    assert(conn->m_network.tls != NULL);

    int ret = tls_connect_socket(conn->m_network.tls,
        conn->m_network.socket, conn->m_network.hostname);

    switch (ret) {
        case 0:
            return GOAT_CONN_CONNECTED;

        case TLS_READ_AGAIN:
        case TLS_WRITE_AGAIN:
            return conn->m_state.state;
    }

    conn->m_state.change_reason = strdup(tls_error(conn->m_network.tls));
    return GOAT_CONN_ERROR;
}

CONN_STATE_EXIT(SSLHANDSHAKE) { ST_UNUSED(conn); }

CONN_STATE_ENTER(CONNECTED) { ST_UNUSED(conn); return 0; }

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

CONN_STATE_EXIT(CONNECTED) { ST_UNUSED(conn); }

CONN_STATE_ENTER(DISCONNECTING) {
    assert(conn != NULL && conn->m_state.state == GOAT_CONN_DISCONNECTING);

    // clear out the write queue, we're not going to send it
    StrQueueEntry *n1, *n2;
    n1 = STAILQ_FIRST(&conn->m_write_queue);
    while (n1 != NULL) {
        n2 = STAILQ_NEXT(n1, entries);
        free(n1);
        n1 = n2;
    }
    STAILQ_INIT(&conn->m_write_queue);

    return 0;
}

CONN_STATE_EXECUTE(DISCONNECTING) {
    assert(conn != NULL && conn->m_state.state == GOAT_CONN_DISCONNECTING);

    if (conn->m_use_ssl) {
        if (NULL == conn->m_network.tls)  goto queue_wait;

        int ret = tls_close(conn->m_network.tls);

        // FIXME depends on non-blocking tls_close
        switch(ret) {
            case 0:
                tls_free(conn->m_network.tls);
                conn->m_network.tls = NULL;
                goto queue_wait;

            case TLS_READ_AGAIN:
            case TLS_WRITE_AGAIN:
                // need to call close again to finish handshake
                return conn->m_state.state;

            default:
                break;
        }

        conn->m_state.change_reason = strdup(tls_error(conn->m_network.tls));
        return GOAT_CONN_ERROR;
    }
    else {
        if (0 == shutdown(conn->m_network.socket, SHUT_RDWR))  goto queue_wait;

        conn->m_state.change_reason = strdup(strerror(errno));
        return GOAT_CONN_ERROR;
    }

queue_wait:
    // once the socket is shut down, stay in disconnecting state until read queue
    // has been emptied (since it contains our status events, not just net io)
    if (STAILQ_EMPTY(&conn->m_read_queue))  return GOAT_CONN_DISCONNECTED;

    return conn->m_state.state;
}

CONN_STATE_EXIT(DISCONNECTING) { ST_UNUSED(conn); }

CONN_STATE_ENTER(ERROR) { ST_UNUSED(conn); return 0; }

CONN_STATE_EXECUTE(ERROR) {
    assert(conn != NULL && conn->m_state.state == GOAT_CONN_ERROR);


    return GOAT_CONN_ERROR;
}

CONN_STATE_EXIT(ERROR) { ST_UNUSED(conn); }
