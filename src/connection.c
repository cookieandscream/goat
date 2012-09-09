#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "connection.h"

typedef void (*state_function)(goat_connection *, int, int);

static void _state_disconnected(goat_connection *, int, int);
static void _state_resolving(goat_connection *, int, int);
static void _state_connecting(goat_connection *, int, int);
static void _state_connected(goat_connection *, int, int);
static void _state_disconnecting(goat_connection *, int, int);

static void _conn_pump_read_queue(goat_connection *);
static void _conn_pump_write_queue(goat_connection *);

const static state_function do_state[] = {
    _state_disconnected,
    _state_resolving,
    _state_connecting,
    _state_connected,
    _state_disconnecting,
};

int conn_init(goat_connection *conn) {
    assert(conn != NULL);
    STAILQ_INIT(&conn->write_queue);
    STAILQ_INIT(&conn->read_queue);
    return -1; // FIXME
}

int conn_destroy(goat_connection *conn) {
    assert(conn != NULL);
    return -1; // FIXME
}

int conn_wants_read(const goat_connection *conn) {
    assert(conn != NULL);

    return conn->state >= GOAT_CONN_CONNECTING;
}

int conn_wants_write(const goat_connection *conn) {
    assert(conn != NULL);
    if (conn->state == GOAT_CONN_CONNECTING) {
        return 1;
    }
    else if (conn->state == GOAT_CONN_CONNECTED && !STAILQ_EMPTY(&conn->write_queue)) {
        return 1;
    }
    else {
        return 0;
    }
}

int conn_wants_timeout(const goat_connection *conn) {
    assert(conn != NULL);
    if (conn->state == GOAT_CONN_RESOLVING) {
        return 1;
    }
    else {
        return 0;
    }
}

int conn_pump_socket(goat_connection *conn, int socket_readable, int socket_writeable) {
    assert(conn != NULL);

    if (0 == pthread_mutex_lock(&conn->mutex)) {
        switch (conn->state) {
            case GOAT_CONN_DISCONNECTED:
            case GOAT_CONN_RESOLVING:
            case GOAT_CONN_CONNECTING:
            case GOAT_CONN_CONNECTED:
            case GOAT_CONN_DISCONNECTING:
                do_state[conn->state](conn, socket_readable, socket_writeable);
                break;

            default:
                assert(0 == "shouldn't get here");
            case GOAT_CONN_ERROR:
                break;
        }
        pthread_mutex_unlock(&conn->mutex);
    }

    return (conn->state == GOAT_CONN_ERROR) ? -1 : 0;
}

int conn_queue_message(
        goat_connection *restrict conn,
        const char *restrict prefix,
        const char *restrict command,
        const char **restrict params
) {
    assert(conn != NULL);
    char buf[516] = { 0 };
    size_t len = 0;

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

    if (0 == pthread_mutex_lock(&conn->mutex)) {
        // now stick it on the connection's write queue
        size_t len = strlen(buf);
        str_queue_entry *entry = malloc(sizeof(str_queue_entry) + len + 1);
        entry->len = len;
        strcpy(entry->str, buf);
        STAILQ_INSERT_TAIL(&conn->write_queue, entry, entries);

        pthread_mutex_unlock(&conn->mutex);
        return 0;
    }
    else {
        return -1;
    }
}

void _state_disconnected(goat_connection *conn, int socket_readable, int socket_writeable) {
    assert(conn != NULL && conn->state == GOAT_CONN_DISCONNECTED);
    // no automatic progression to any other state
}

void _state_resolving(goat_connection *conn, int socket_readable, int socket_writeable) {
    assert(conn != NULL && conn->state == GOAT_CONN_RESOLVING);
    // see if we've got a result yet
    if (0) {
        // got a result!  start connecting
        conn->state = GOAT_CONN_CONNECTING;
    }
}

void _state_connecting(goat_connection *conn, int socket_readable, int socket_writeable) {
    assert(conn != NULL && conn->state == GOAT_CONN_CONNECTING);
    if (socket_writeable) {
        conn->state = GOAT_CONN_CONNECTED;
    }
}

void _state_connected(goat_connection *conn, int socket_readable, int socket_writeable) {
    assert(conn != NULL && conn->state == GOAT_CONN_CONNECTED);
    if (socket_readable) {
        _conn_pump_read_queue(conn);
    }
    if (socket_writeable && conn->state == GOAT_CONN_CONNECTED) {
        _conn_pump_write_queue(conn);
    }
}

void _state_disconnecting(goat_connection *conn, int socket_readable, int socket_writeable) {
    assert(conn != NULL && conn->state == GOAT_CONN_DISCONNECTING);
    // any processing we need to do during disconnect

    str_queue_entry *n1, *n2;
    n1 = STAILQ_FIRST(&conn->read_queue);
    while (n1 != NULL) {
        n2 = STAILQ_NEXT(n1, entries);
        free(n1);
        n1 = n2;
    }
    STAILQ_INIT(&conn->read_queue);

    n1 = STAILQ_FIRST(&conn->write_queue);
    while (n1 != NULL) {
        n2 = STAILQ_NEXT(n1, entries);
        free(n1);
        n1 = n2;
    }
    STAILQ_INIT(&conn->write_queue);

    conn->state = GOAT_CONN_DISCONNECTED;
}

void _conn_pump_write_queue(goat_connection *conn) {
    assert(conn != NULL && conn->state == GOAT_CONN_CONNECTED);

    while (!STAILQ_EMPTY(&conn->write_queue)) {
        str_queue_entry *n = STAILQ_FIRST(&conn->write_queue);

        ssize_t wrote = write(conn->socket, n->str, n->len);

        if (wrote < 0) {
            // FIXME write failed for some reason
            break;
        }
        else if (wrote == 0) {
            // socket has been disconnected
            conn->state = GOAT_CONN_DISCONNECTING;
            break;
        }
        else if (wrote < n->len) {
            // partial write - reinsert the remainder at the queue head for next
            // time the socket is writeable
            STAILQ_REMOVE_HEAD(&conn->write_queue, entries);

            size_t len = n->len - wrote;
            str_queue_entry *tmp = malloc(sizeof(str_queue_entry) + len + 1);
            tmp->len = len;
            strcpy(tmp->str, &n->str[wrote]);

            STAILQ_INSERT_HEAD(&conn->write_queue, tmp, entries);

            free(n);
            break;
        }
        else {
            // wrote the whole thing, remove it from the queue
            STAILQ_REMOVE_HEAD(&conn->write_queue, entries);
            free(n);
        }
    }
}

// FIXME function name... this isn't really pumping the queue so much as its populating it
void _conn_pump_read_queue(goat_connection *conn) {
    assert(conn != NULL && conn->state == GOAT_CONN_CONNECTED);

    char buf[516], saved[516] = {0};
    ssize_t bytes;

    bytes = read(conn->socket, buf, sizeof(buf));
    while (bytes > 0) {
        const char const *end = &buf[bytes];
        char *curr = buf, *next = NULL;

        while (curr != end) {
            next = curr;
            while (next != end && *(next++) != '\x0a') ;

            if (*(next - 1) == '\x0a') {
                // found a complete line, queue it
                size_t saved_len = strnlen(saved, sizeof(saved));
                size_t len = next - curr;

                str_queue_entry *n = malloc(sizeof(str_queue_entry) + saved_len + len + 1);
                n->len = len;
                n->str[0] = '\0';
                if (saved[0] != '\0') {
                    strncat(n->str, saved, saved_len);
                    memset(saved, 0, sizeof(saved));
                }
                strncat(n->str, curr, len);
                STAILQ_INSERT_TAIL(&conn->read_queue, n, entries);
            }
            else {
                // found a partial line, save it for the next read
                assert(next == end);
                strncpy(saved, curr, next - curr);
                saved[next - curr] = '\0';
            }

            curr = next;
        }

        bytes = read(conn->socket, buf, sizeof(buf));
    }

    if (saved[0] != '\0') {
        // no \r\n at end of last read, queue it anyway
        size_t len = strnlen(saved, sizeof(saved));

        str_queue_entry *n = malloc(sizeof(str_queue_entry) + len + 1);
        n->len = len;
        strncpy(n->str, saved, len);
        n->str[len] = '\0';
        STAILQ_INSERT_TAIL(&conn->read_queue, n, entries);

        memset(saved, 0, sizeof(saved));
    }

    if (bytes == 0) {
        // FIXME disconnected
        conn->state = GOAT_CONN_DISCONNECTING;
    }
}
