#ifndef GOAT_CONNECTION_H
#define GOAT_CONNECTION_H

#include <config.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <pthread.h>

#include "goat.h"
#include "connsm.h"

typedef enum {
    GOAT_CONN_ERROR         = -1,
    GOAT_CONN_DISCONNECTED  = 0,
    GOAT_CONN_RESOLVING,
    GOAT_CONN_CONNECTING,
    GOAT_CONN_CONNECTED,
    GOAT_CONN_DISCONNECTING,
} goat_conn_state;

typedef struct str_queue_entry_s {
    STAILQ_ENTRY(str_queue_entry_s) entries;
    size_t len;
    char str[0];
} str_queue_entry;

typedef STAILQ_HEAD(str_queue_head_s, str_queue_entry_s) str_queue_head;

typedef struct {
    const goat_handle handle;
    pthread_mutex_t mutex;
    int socket;
    struct sockaddr *address;
    socklen_t address_len;
    goat_conn_state state;
    void *state_data;
    int ssl;
    str_queue_head write_queue;
    str_queue_head read_queue;
} goat_connection;

int conn_init(goat_connection *);
int conn_destroy(goat_connection *);

int conn_connect(goat_connection *); // FIXME
int conn_disconnect(goat_connection *); // FIXME

int conn_wants_read(const goat_connection *);
int conn_wants_write(const goat_connection *);
int conn_wants_timeout(const goat_connection *);

int conn_queue_message(goat_connection *restrict, const char *restrict, const char *restrict, const char **restrict);

int conn_pump_socket(goat_connection *, int, int); // read ready, write ready

STATE_DECL(DISCONNECTED);
STATE_DECL(RESOLVING);
STATE_DECL(CONNECTING);
STATE_DECL(CONNECTED);
STATE_DECL(DISCONNECTING);
STATE_DECL(ERROR);

typedef void (*state_enter_function)(goat_connection *);
typedef goat_conn_state (*state_execute_function)(goat_connection *, int, int);
typedef void (*state_exit_function)(goat_connection *);

#endif
