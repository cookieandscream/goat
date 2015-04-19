#include <assert.h>

#include "context.h"

Connection *context_get_connection(GoatContext *context, int index) {
    assert(NULL != context);

    Connection *conn = NULL;

    if (0 == pthread_rwlock_rdlock(&context->m_rwlock)) {
        if (index >= 0
            && index < (int) context->m_connections_size
            && context->m_connections[index] != NULL
        ) {
            conn = context->m_connections[index];
        }

        pthread_rwlock_unlock(&context->m_rwlock);
    }

    return conn;
}
