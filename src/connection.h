#ifndef GOAT_CONNECTION_H
#define GOAT_CONNECTION_H

#include <config.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <netdb.h>
#include <pthread.h>

#include <tls.h>

#include "goat.h"
#include "message.h"
#include "tresolver.h"

typedef enum {
    GOAT_CONN_DISCONNECTED  = 0,
    GOAT_CONN_RESOLVING,
    GOAT_CONN_CONNECTING,
    GOAT_CONN_SSLHANDSHAKE,
    GOAT_CONN_CONNECTED,
    GOAT_CONN_DISCONNECTING,

    // keep error as last
    GOAT_CONN_ERROR
} goat_conn_state_t;

typedef struct s_str_queue_entry {
    STAILQ_ENTRY(s_str_queue_entry) entries;
    size_t  len;
    int     has_eol;
    char    str[0];
} str_queue_entry_t;

typedef STAILQ_HEAD(s_str_queue_head, s_str_queue_entry) str_queue_head_t;

typedef struct {
    struct addrinfo *ai;
} connecting_state_data_t;

typedef struct {
    int                 m_handle;
    pthread_mutex_t     m_mutex;
    struct {
        int                 socket;
        struct sockaddr     *address;
        socklen_t           address_len;
        char                *hostname;
        char                *servname;
        struct addrinfo     *ai0;
        struct tls          *tls;
    } m_network;
    struct {
        goat_conn_state_t   state;
        union {
            void                    *raw;
            connecting_state_data_t *connecting;
        } data;
        int                 socket_is_readable;
        int                 socket_is_writeable;
        goat_error_t        error;
        char                *change_reason;
        resolver_state_t    *res_state;
    } m_state;
    int                 m_use_ssl;
    str_queue_head_t    m_write_queue;
    str_queue_head_t    m_read_queue;
} goat_connection_t;

int conn_init(goat_connection_t *conn, int handle);
int conn_destroy(goat_connection_t *conn);

int conn_connect(goat_connection_t *conn, const char *hostname, const char *servname, int ssl);
int conn_disconnect(goat_connection_t *); // FIXME

int conn_wants_read(const goat_connection_t *);
int conn_wants_write(const goat_connection_t *);
int conn_wants_timeout(const goat_connection_t *);

int conn_reset_error(goat_connection_t *conn);

int conn_send_message(goat_connection_t *conn, const goat_message_t *message);

goat_message_t *conn_recv_message(goat_connection_t *conn);

int conn_tick(goat_connection_t *conn, int socket_readable, int socket_writeable);

#endif
