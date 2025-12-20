#include <stddef.h>
#include "router.h"
#include "mod_auth.h"
#include "mod_accounting.h"

void register_all_routes_config(void) {
  // RUTE PUBLICE
  register_route("POST", "/api/login",    ROUTE_PUBLIC, auth_login_handler);
  register_route("GET",  "/api/accounts", ROUTE_PUBLIC, acc_get_all_accounts);

  // RUTE PROTEJATE
  register_route("GET",  "/api/transaction/:id", ROUTE_PROTECTED, acc_get_transaction);
  register_route("POST", "/api/seed",            ROUTE_PROTECTED, acc_seed_transaction);
}
