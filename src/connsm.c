#include <config.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "connection.h"
#include "connsm.h"

static goat_conn_state _conn_pump_read_queue(goat_connection *);
static goat_conn_state _conn_pump_write_queue(goat_connection *);

STATE_EXECUTE(DISCONNECTED) {
    assert(conn != NULL && conn->state == GOAT_CONN_DISCONNECTED);
    // no automatic progression to any other state
}

STATE_EXECUTE(RESOLVING) {
    assert(conn != NULL && conn->state == GOAT_CONN_RESOLVING);
    // see if we've got a result yet
    if (0) {
        // got a result!  start connecting
        conn->state = GOAT_CONN_CONNECTING;
    }
}

STATE_EXECUTE(CONNECTING) {
    assert(conn != NULL && conn->state == GOAT_CONN_CONNECTING);
    if (socket_writeable) {
        conn->state = GOAT_CONN_CONNECTED;
    }
}

STATE_EXECUTE(CONNECTED) {
    assert(conn != NULL && conn->state == GOAT_CONN_CONNECTED);
    goat_conn_state next_state = conn->state;
    if (socket_readable) {
        next_state = _conn_pump_read_queue(conn);
    }
    if (socket_writeable && conn->state == next_state) {
        next_state = _conn_pump_write_queue(conn);
    }

    return next_state;
}

STATE_EXECUTE(DISCONNECTING) {
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

    return GOAT_CONN_DISCONNECTED;
}

STATE_EXECUTE(ERROR) {
    assert(conn != NULL && conn->state == GOAT_CONN_ERROR);

    // recover to newly-initialised state

    return GOAT_CONN_DISCONNECTED;
}

goat_conn_state _conn_pump_write_queue(goat_connection *conn) {
    assert(conn != NULL && conn->state == GOAT_CONN_CONNECTED);

    while (!STAILQ_EMPTY(&conn->write_queue)) {
        str_queue_entry *n = STAILQ_FIRST(&conn->write_queue);

        ssize_t wrote = write(conn->socket, n->str, n->len);

        if (wrote < 0) {
            // FIXME write failed for some reason
            return GOAT_CONN_ERROR;
        }
        else if (wrote == 0) {
            // socket has been disconnected
            return GOAT_CONN_DISCONNECTING;
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
            return conn->state;
        }
        else {
            // wrote the whole thing, remove it from the queue
            STAILQ_REMOVE_HEAD(&conn->write_queue, entries);
            return conn->state;
        }
    }
}

// FIXME function name... this isn't really pumping the queue so much as its populating it
goat_conn_state _conn_pump_read_queue(goat_connection *conn) {
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
        return GOAT_CONN_DISCONNECTING;
    }

    return conn->state;
}
