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
} ConnState;

typedef struct str_queue_entry {
    STAILQ_ENTRY(str_queue_entry) entries;
    size_t  len;
    int     has_eol;
    char    str[0];
} StrQueueEntry;

typedef STAILQ_HEAD(str_queue_head, str_queue_entry) StrQueueHead;

typedef struct {
    struct addrinfo *ai;
} ConnectingStateData;

typedef struct {
    pthread_mutex_t         m_mutex;
    struct {
        int                 socket;
        char                *hostname;
        char                *servname;
        struct addrinfo     *ai0;
        struct tls          *tls;
    } m_network;
    struct {
        ConnState           state;
        union {
            void                *raw;
            ConnectingStateData *connecting;
            ResolverState       *resolving;
        } data;
        int                 socket_is_readable;
        int                 socket_is_writeable;
        GoatError           error;
        char                *change_reason;
    } m_state;
    int                 m_use_ssl;
    StrQueueHead        m_write_queue;
    StrQueueHead        m_read_queue;
} Connection;

int conn_init(Connection *conn);
int conn_destroy(Connection *conn);

int conn_connect(Connection *conn, const char *hostname, const char *servname, int ssl);
int conn_disconnect(Connection *); // FIXME

int conn_wants_read(const Connection *);
int conn_wants_write(const Connection *);
int conn_wants_timeout(const Connection *);

int conn_reset_error(Connection *conn);

int conn_send_message(Connection *conn, const GoatMessage *message);

GoatMessage *conn_recv_message(Connection *conn);

int conn_tick(Connection *conn, int socket_readable, int socket_writeable);

#endif
