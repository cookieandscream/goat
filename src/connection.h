#ifndef GOAT_CONNECTION_H
#define GOAT_CONNECTION_H

#include <pthread.h>

#include "goat.h"

typedef enum {
    GOAT_CONN_ERROR         = -1,
    GOAT_CONN_DISCONNECTED  = 0,
    GOAT_CONN_CONNECTING    = 1,
    GOAT_CONN_CONNECTED     = 2,
    GOAT_CONN_DISCONNECTING = 3,
} goat_connection_state;

typedef struct {
    const goat_handle handle;
    pthread_mutex_t mutex;
    int socket;
    goat_connection_state state;
    int ssl;
    int write_queue;
    int read_queue;
} goat_connection;

int conn_init(goat_connection *);
int conn_destroy(goat_connection *);

int conn_wants_read(const goat_connection *);
int conn_wants_write(const goat_connection *);

int conn_pump_socket(goat_connection *, int, int); // read ready, write ready

#endif
