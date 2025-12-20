#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mod_auth.h"
#include "response.h"

static void generate_token(char *token) {
  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  srand(time(NULL));
  for (int i = 0; i < 32; i++) {
    token[i] = charset[rand() % (sizeof(charset) - 1)];
  }
  token[32] = '\0';
}

int auth_middleware_check(PGconn *conn, char *request_buffer) {
  char *auth_header = strstr(request_buffer, "Authorization: Bearer ");
  if (!auth_header) return 0;

  char token[33] = {0};
  sscanf(auth_header, "Authorization: Bearer %32s", token);

  char query[256];
  snprintf(query, sizeof(query), "SELECT user_id FROM sessions WHERE token = '%s' AND expires_at > NOW()", token);
  
  PGresult *res = PQexec(conn, query);
  int valid = (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0);
  PQclear(res);
  
  return valid;
}

void auth_login_handler(int fd, PGconn *conn, char *body, route_params *params) {
  char username[50] = {0};
  char password[50] = {0};

  char *u_ptr = strstr(body, "username=");
  char *p_ptr = strstr(body, "password=");

  if (u_ptr && p_ptr) {
    sscanf(u_ptr, "username=%49[^&]", username);
    sscanf(p_ptr, "password=%49s", password);
  } else {
    send_json(fd, "400 Bad Request", "{\"error\": \"Format invalid\"}");
    return;
  }

  char query[512];
  snprintf(query, sizeof(query), 
    "SELECT id FROM users WHERE username='%s' AND password_hash = crypt('%s', password_hash)", 
    username, password);

  PGresult *res = PQexec(conn, query);

  if (PQntuples(res) > 0) {
    int user_id = atoi(PQgetvalue(res, 0, 0));
    char token[33];
    generate_token(token);

    char insert_sess[512];
    snprintf(insert_sess, sizeof(insert_sess), "INSERT INTO sessions (token, user_id) VALUES ('%s', %d)", token, user_id);
    PQclear(PQexec(conn, insert_sess));

    char resp[128];
    snprintf(resp, sizeof(resp), "{\"status\": \"success\", \"token\": \"%s\"}", token);
    send_json(fd, "200 OK", resp);
  } else {
    send_json(fd, "401 Unauthorized", "{\"error\": \"Invalid credentials\"}");
  }
  PQclear(res);
}
