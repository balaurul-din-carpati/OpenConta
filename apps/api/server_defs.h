#ifndef SERVER_DEFS_H
#define SERVER_DEFS_H

#include <libpq-fe.h>

typedef enum {
  ROUTE_PUBLIC = 0,
  ROUTE_PROTECTED = 1
} route_type_t;

typedef struct {
  char key[32];
  char value[128];
} route_params;

typedef void (*route_handler)(int fd, PGconn *conn, char *body, route_params *params);

#endif
