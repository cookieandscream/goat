#include <assert.h>

#include "connection.h"

int conn_init(goat_connection *conn) {
    assert(conn != NULL);
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
    else if (conn->state == GOAT_CONN_CONNECTED && conn->write_queue) { // FIXME
        return 1;
    }
    else {
        return 0;
    }
}

int conn_pump_socket(goat_connection *conn, int read_ready, int write_ready) {
    assert(conn != NULL);
    return -1; // FIXME
}
