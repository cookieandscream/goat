#ifndef GOAT_CORE_H
#define GOAT_CORE_H

#include <pthread.h>

#include "goat.h"

typedef struct {
    pthread_mutex_t mutex;
    int running;
} goat_core_state;

typedef struct {
    pthread_mutex_t mutex;
    const goat_handle handle;
    const int socket;
    int has_pending_write_data;  // TODO
} goat_connection;

typedef struct {
    pthread_mutex_t mutex;
    size_t connection_pool_size;
    goat_connection **connection_pool;
} goat_core_connection_list;

extern goat_core_state core_state;
extern goat_core_connection_list core_connection_list;

void *_goat_core(void *);


#endif
