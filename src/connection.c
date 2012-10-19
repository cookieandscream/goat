#include <config.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "connection.h"
#include "sm.h"

static ssize_t _conn_recv_data(goat_connection_t *);
static ssize_t _conn_send_data(goat_connection_t *);

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

    switch (conn->m_state) {
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
    switch (conn->m_state) {
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
    switch (conn->m_state) {
        case GOAT_CONN_RESOLVING:
            return 1;

        default:
            return 0;
    }
}

int conn_tick(goat_connection_t *conn, int socket_readable, int socket_writeable) {
    assert(conn != NULL);

    if (0 == pthread_mutex_lock(&conn->m_mutex)) {
        conn->m_socket_is_readable = socket_readable;
        conn->m_socket_is_writeable = socket_writeable;
        goat_conn_state_t next_state;
        switch (conn->m_state) {
            case GOAT_CONN_DISCONNECTED:
            case GOAT_CONN_RESOLVING:
            case GOAT_CONN_CONNECTING:
            case GOAT_CONN_CONNECTED:
            case GOAT_CONN_DISCONNECTING:
            case GOAT_CONN_ERROR:
                next_state = state_execute[conn->m_state](conn);
                if (next_state != conn->m_state) {
                    state_exit[conn->m_state](conn);
                    conn->m_state = next_state;
                    state_enter[conn->m_state](conn);
                }
                break;

            default:
                assert(0 == "shouldn't get here");
                conn->m_error = GOAT_E_STATE;
                conn->m_state = GOAT_CONN_ERROR;
                state_enter[conn->m_state](conn);
                break;
        }
        pthread_mutex_unlock(&conn->m_mutex);
    }

    if (conn->m_state == GOAT_CONN_ERROR)  return -1;

    return !STAILQ_EMPTY(&conn->m_read_queue);  // cheap estimate of number of events
}

int conn_reset_error(goat_connection_t *conn) {
    assert(conn!= NULL);

    if (0 == pthread_mutex_lock(&conn->m_mutex)) {
        conn->m_error = GOAT_E_NONE;

        if (conn->m_state == GOAT_CONN_ERROR) {
            state_exit[conn->m_state](conn);
            conn->m_state = GOAT_CONN_DISCONNECTED;
            state_enter[conn->m_state](conn);
        }

        pthread_mutex_unlock(&conn->m_mutex);
        return 0;
    }
    else {
        return -1;
    }
}

int conn_queue_message(
        goat_connection_t *restrict conn,
        const char *restrict prefix,
        const char *restrict command,
        const char **restrict params
) {
    assert(conn != NULL);
    char buf[516] = { 0 };

    // n.b. internal only, so trusts caller to provide valid args
    if (prefix) {
        strcat(buf, ":");
        strcat(buf, prefix);
        strcat(buf, " ");
    }

    strcat(buf, command);

    while (params) {
        strcat(buf, " ");
        if (strchr(*params, ' ')) {
            strcat(buf, ":");
            strcat(buf, *params);
            break;
        }
        strcat(buf, *params);
        ++ params;
    }

    strcat(buf, "\x0d\x0a");

    if (0 == pthread_mutex_lock(&conn->m_mutex)) {
        // now stick it on the connection's write queue
        size_t len = strlen(buf);
        str_queue_entry_t *entry = malloc(sizeof(str_queue_entry_t) + len + 1);
        entry->len = len;
        entry->has_eol = 1;
        strcpy(entry->str, buf);
        STAILQ_INSERT_TAIL(&conn->m_write_queue, entry, entries);

        pthread_mutex_unlock(&conn->m_mutex);
        return 0;
    }
    else {
        return -1;
    }
}

char *conn_pop_message(goat_connection_t *conn) {
    assert(conn != NULL);

    char *message = NULL;

    str_queue_entry_t *node = STAILQ_FIRST(&conn->m_read_queue);

    if (node != NULL && node->has_eol) {
        if (NULL != (message = malloc(node->len + 1))) {
            strncpy(message, node->str, node->len);
            message[node->len] = '\0';

            STAILQ_REMOVE_HEAD(&conn->m_read_queue, entries);
            free(node);
        }
    }

    return message;
}

ssize_t _conn_send_data(goat_connection_t *conn) {
    assert(conn != NULL && conn->m_state == GOAT_CONN_CONNECTED);
    ssize_t total_bytes_sent = 0;

    while (!STAILQ_EMPTY(&conn->m_write_queue)) {
        str_queue_entry_t *node = STAILQ_FIRST(&conn->m_write_queue);

        ssize_t wrote = write(conn->m_socket, node->str, node->len);

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

            size_t len = node->len - wrote;
            str_queue_entry_t *tmp = malloc(sizeof(str_queue_entry_t) + len + 1);
            tmp->len = len;
            tmp->has_eol = node->has_eol;
            strcpy(tmp->str, &node->str[wrote]);

            STAILQ_INSERT_HEAD(&conn->m_write_queue, tmp, entries);

            free(node);
            return 1;
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
    assert(conn != NULL && conn->m_state == GOAT_CONN_CONNECTED);

    char buf[516], saved[516] = {0};
    ssize_t bytes, total_bytes_read = 0;

    bytes = read(conn->m_socket, buf, sizeof(buf));
    while (bytes > 0) {
        const char const *end = &buf[bytes];
        char *curr = buf, *next = NULL;

        while (curr != end) {
            next = curr;
            while (next != end && *(next++) != '\x0a') ;

            if (*(next - 1) == '\x0a') {
                // found a complete line, queue it
                // if the previously-queued line was incomplete, dequeue it and combine with this
                // if we have saved data from the last read, combine with this
                str_queue_entry_t *const partial = STAILQ_LAST(
                    &conn->m_read_queue,
                    s_str_queue_entry,
                    entries
                );

                size_t partial_len = partial->has_eol ? 0 : partial->len;
                size_t saved_len = strnlen(saved, sizeof(saved));
                size_t len = next - curr;

                str_queue_entry_t *node = malloc(
                    sizeof(str_queue_entry_t) + partial_len + saved_len + len + 1
                );
                node->len = partial_len + saved_len + len;
                node->has_eol = 1;
                memset(node->str, '\0', node->len + 1);

                if (partial_len) {
                    strncat(node->str, partial->str, partial_len);
                    STAILQ_REMOVE(&conn->m_read_queue, partial, s_str_queue_entry, entries);
                    free(partial);
                }

                if (saved_len) {
                    strncat(node->str, saved, saved_len);
                    memset(saved, 0, sizeof(saved));
                }

                strncat(node->str, curr, len);

                STAILQ_INSERT_TAIL(&conn->m_read_queue, node, entries);
            }
            else {
                // FIXME if >1 read in a row doesn't contain an eol, all data except the last one
                // FIXME will be discarded.
                // found a partial line, save it for the next read
                assert(next == end);
                strncpy(saved, curr, next - curr);
                saved[next - curr] = '\0';
            }

            curr = next;
        }

        total_bytes_read += bytes;
        bytes = read(conn->m_socket, buf, sizeof(buf));
    }

    if (saved[0] != '\0') {
        // no eol at end of last read, queue partial line anyway
        size_t len = strnlen(saved, sizeof(saved));

        str_queue_entry_t *n = malloc(sizeof(str_queue_entry_t) + len + 1);
        n->len = len;
        n->has_eol = 0;
        strncpy(n->str, saved, len);
        n->str[len] = '\0';
        STAILQ_INSERT_TAIL(&conn->m_read_queue, n, entries);

        memset(saved, 0, sizeof(saved));
    }

    return total_bytes_read;
}

CONN_STATE_ENTER(DISCONNECTED) { }

CONN_STATE_EXECUTE(DISCONNECTED) {
    assert(conn != NULL && conn->m_state == GOAT_CONN_DISCONNECTED);
    // no automatic progression to any other state
    return conn->m_state;
}

CONN_STATE_EXIT(DISCONNECTED) { }

CONN_STATE_ENTER(RESOLVING) {
    // set up a resolver and kick it off
}

CONN_STATE_EXECUTE(RESOLVING) {
    assert(conn != NULL && conn->m_state == GOAT_CONN_RESOLVING);
    // see if we've got a result yet
    if (0) {
        // got a result!  start connecting
        return GOAT_CONN_CONNECTING;
    }

    return conn->m_state;
}

CONN_STATE_EXIT(RESOLVING) {
    // clean up resolver
}

CONN_STATE_ENTER(CONNECTING) {
    // start up a connection attempt
}

CONN_STATE_EXECUTE(CONNECTING) {
    assert(conn != NULL && conn->m_state == GOAT_CONN_CONNECTING);
    if (conn->m_socket_is_writeable) {
        return GOAT_CONN_CONNECTED;
    }

    return conn->m_state;
}

CONN_STATE_EXIT(CONNECTING) { }

CONN_STATE_ENTER(CONNECTED) { }

CONN_STATE_EXECUTE(CONNECTED) {
    assert(conn != NULL && conn->m_state == GOAT_CONN_CONNECTED);

    if (conn->m_socket_is_readable) {
        if (_conn_recv_data(conn) <= 0) {
            return GOAT_CONN_DISCONNECTING;
        }
    }
    if (conn->m_socket_is_writeable) {
        if (_conn_send_data(conn) <= 0) {
            return GOAT_CONN_DISCONNECTING;
        }
    }

    return conn->m_state;
}

CONN_STATE_EXIT(CONNECTED) { }

CONN_STATE_ENTER(DISCONNECTING) { }

CONN_STATE_EXECUTE(DISCONNECTING) {
    assert(conn != NULL && conn->m_state == GOAT_CONN_DISCONNECTING);
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
    assert(conn != NULL && conn->m_state == GOAT_CONN_ERROR);


    return GOAT_CONN_ERROR;
}

CONN_STATE_EXIT(ERROR) {
    // FIXME recover to newly-initialised state
}
