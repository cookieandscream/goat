#include <config.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "connection.h"

static const state_enter_function state_enter[] = {
    STATE_ENTER_NAME(DISCONNECTED),
    STATE_ENTER_NAME(RESOLVING),
    STATE_ENTER_NAME(CONNECTING),
    STATE_ENTER_NAME(CONNECTED),
    STATE_ENTER_NAME(DISCONNECTING),
    STATE_ENTER_NAME(ERROR),
};

static const state_execute_function state_execute[] = {
    STATE_EXECUTE_NAME(DISCONNECTED),
    STATE_EXECUTE_NAME(RESOLVING),
    STATE_EXECUTE_NAME(CONNECTING),
    STATE_EXECUTE_NAME(CONNECTED),
    STATE_EXECUTE_NAME(DISCONNECTING),
    STATE_EXECUTE_NAME(ERROR),
};

static const state_exit_function state_exit[] = {
    STATE_EXIT_NAME(DISCONNECTED),
    STATE_EXIT_NAME(RESOLVING),
    STATE_EXIT_NAME(CONNECTING),
    STATE_EXIT_NAME(CONNECTED),
    STATE_EXIT_NAME(DISCONNECTING),
    STATE_EXIT_NAME(ERROR),
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

    switch (conn->state) {
        case GOAT_CONN_CONNECTING:
        case GOAT_CONN_CONNECTED:
        case GOAT_CONN_DISCONNECTING:
            return 1;

        default:
            return 0;
    }
}

int conn_wants_write(const goat_connection *conn) {
    assert(conn != NULL);
    switch (conn->state) {
        case GOAT_CONN_CONNECTED:
            if (STAILQ_EMPTY(&conn->write_queue))  return 0;
            /* fall through */
        case GOAT_CONN_CONNECTING:
            return 1;

        default:
            return 0;
    }
}

int conn_wants_timeout(const goat_connection *conn) {
    assert(conn != NULL);
    switch (conn->state) {
        case GOAT_CONN_RESOLVING:
            return 1;

        default:
            return 0;
    }
}

int conn_pump_socket(goat_connection *conn, int socket_readable, int socket_writeable) {
    assert(conn != NULL);

    if (0 == pthread_mutex_lock(&conn->mutex)) {
        goat_conn_state next_state;
        switch (conn->state) {
            case GOAT_CONN_DISCONNECTED:
            case GOAT_CONN_RESOLVING:
            case GOAT_CONN_CONNECTING:
            case GOAT_CONN_CONNECTED:
            case GOAT_CONN_DISCONNECTING:
            case GOAT_CONN_ERROR:
                next_state = state_execute[conn->state](conn, socket_readable, socket_writeable);
                if (next_state != conn->state) {
                    state_exit[conn->state](conn);
                    state_enter[next_state](conn);
                }
                break;

            default:
                assert(0 == "shouldn't get here");
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
