#ifndef MOD_AUTH_H
#define MOD_AUTH_H

#include "server_defs.h"

int auth_middleware_check(PGconn *conn, char *request_buffer);
void auth_login_handler(int fd, PGconn *conn, char *body, route_params *params);

#endif
