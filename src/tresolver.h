#ifndef GOAT_TRESOLVER_H
#define GOAT_TRESOLVER_H

#include <netdb.h>

typedef struct s_resolver_state resolver_state_t;

int resolver_getaddrinfo(resolver_state_t **statep, const char *hostname,
    const char *servname, struct addrinfo **resp);

int resolver_cancel(resolver_state_t **statep);

#endif
