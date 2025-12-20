#include <stdio.h>
#include <string.h>
#include "router.h"
#include "response.h"
#include "mod_auth.h"

typedef struct {
  char method[10];
  char path[100];
  route_type_t type;
  route_handler handler;
} route_entry;

static route_entry routes[100];
static int route_count = 0;

void register_route(const char *method, const char *path, route_type_t type, route_handler handler) {
  if (route_count < 100) {
    strncpy(routes[route_count].method, method, 9);
    strncpy(routes[route_count].path, path, 99);
    routes[route_count].type = type;
    routes[route_count].handler = handler;

    printf("[ROUTER] Inregistrat: [%s] %s\n", method, path);
    route_count++;
  }
}

void router_init(void) {
  printf("[ROUTER] Initializare rute...\n");
  route_count = 0;
  register_all_routes_config();
}

int match_path_params(const char *route_path, const char *req_path, route_params *params) {
  if (strchr(route_path, ':') == NULL) {
    return (strcmp(route_path, req_path) == 0);
  }

  char r_copy[100], p_copy[100];
  memset(r_copy, 0, 100);
  memset(p_copy, 0, 100);

  strncpy(r_copy, route_path, 99);
  strncpy(p_copy, req_path, 99);

  char *saveptr_r, *saveptr_p;
  char *r_token = strtok_r(r_copy, "/", &saveptr_r);
  char *p_token = strtok_r(p_copy, "/", &saveptr_p);

  while (r_token != NULL && p_token != NULL) {
    if (r_token[0] == ':') {
      strncpy(params->key, r_token + 1, 31);
      strncpy(params->value, p_token, 127);
    } else if (strcmp(r_token, p_token) != 0) {
      return 0;
    }

    r_token = strtok_r(NULL, "/", &saveptr_r);
    p_token = strtok_r(NULL, "/", &saveptr_p);
  }

  return (r_token == NULL && p_token == NULL);
}

void router_handle_request(int fd, PGconn *conn, char *buffer) {
  char method[10] = {0}, path[100] = {0};
  sscanf(buffer, "%s %s", method, path);

  printf("[REQ] Metoda: '%s' | Cale: '%s'\n", method, path);

  char *body = strstr(buffer, "\r\n\r\n");
  if (body) body += 4; else body = "";

  for (int i = 0; i < route_count; i++) {
    if (strcmp(routes[i].method, method) == 0) {
      route_params params = {0};

      if (match_path_params(routes[i].path, path, &params)) {
        printf("[MATCH] Ruta gasita: %s\n", routes[i].path);

        if (routes[i].type == ROUTE_PROTECTED) {
          if (!auth_middleware_check(conn, buffer)) {
            send_json(fd, "401 Unauthorized", "{\"error\": \"Login required\"}");
            return;
          }
        }

        routes[i].handler(fd, conn, body, &params);
        return;
      }
    }
  }

  printf("[404] Nicio ruta nu s-a potrivit pentru %s\n", path);
  send_json(fd, "404 Not Found", "{\"error\": \"Route not found\"}");
}
