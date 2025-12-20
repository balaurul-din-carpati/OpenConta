#ifndef ROUTER_H
#define ROUTER_H

#include "server_defs.h"

void router_init(void);

void register_route(
  const char *method, 
  const char *path, 
  route_type_t type, 
  route_handler handler
  );

void router_handle_request(
  int fd, 
  PGconn *conn, 
  char *buffer
  );

void register_all_routes_config(void);

#endif
