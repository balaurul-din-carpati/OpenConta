#ifndef MOD_ACCOUNTING_H
#define MOD_ACCOUNTING_H

#include "server_defs.h"

void acc_get_transaction(
  int fd, PGconn *conn, 
  char *body, 
  route_params *params
  );
void acc_seed_transaction(
  int fd, PGconn *conn, 
  char *body, 
  route_params *params
  );
void acc_get_all_accounts(
  int fd, 
  PGconn *conn, 
  char *body, 
  route_params *params
  );

#endif
