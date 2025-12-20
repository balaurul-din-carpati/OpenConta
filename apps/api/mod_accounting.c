#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mod_accounting.h"
#include "response.h"

void acc_get_transaction(int fd, PGconn *conn, char *body, route_params *params) {
  if (strlen(params->value) == 0) {
    send_json(fd, "400 Bad Request", "{\"error\": \"Missing ID\"}");
    return;
  }

  int id = atoi(params->value);
  char query[1024];

  snprintf(query, sizeof(query),
    "SELECT json_build_object('id', id, 'doc', document_ref, 'lines', ("
    "SELECT json_agg(json_build_object('cont', account_code, 'debit', debit, 'credit', credit)) "
    "FROM journal_lines WHERE entry_id = journal_entries.id)) FROM journal_entries WHERE id = %d;", id);

  PGresult *res = PQexec(conn, query);

  if (PQntuples(res) > 0) {
    char *data = PQgetvalue(res, 0, 0);
    size_t len = PQgetlength(res, 0, 0);
    send_large_json(fd, "200 OK", data, len);
  } else {
    send_json(fd, "404 Not Found", "{\"error\": \"Not found\"}");
  }
  PQclear(res);
}

void acc_seed_transaction(int fd, PGconn *conn, char *body, route_params *params) {
  PGresult *res = PQexec(conn, "INSERT INTO journal_entries (description) VALUES ('Modular Seed') RETURNING id");

  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    PQclear(res);
    send_json(fd, "500 Error", "{\"error\": \"DB Error\"}");
    return;
  }

  int id = atoi(PQgetvalue(res, 0, 0)); PQclear(res);

  char q1[256], q2[256];
  snprintf(q1, sizeof(q1), "INSERT INTO journal_lines (entry_id, account_code, debit) VALUES (%d, '4111', 100)", id);
  snprintf(q2, sizeof(q2), "INSERT INTO journal_lines (entry_id, account_code, credit) VALUES (%d, '707', 100)", id);

  PQclear(PQexec(conn, q1));
  PQclear(PQexec(conn, q2));

  char resp[64];
  snprintf(resp, sizeof(resp), "{\"status\": \"seeded\", \"id\": %d}", id);
  send_json(fd, "200 OK", resp);
}

void acc_get_all_accounts(int fd, PGconn *conn, char *body, route_params *params) {
  const char *query = "SELECT json_agg(json_build_object('cod', code, 'nume', name, 'tip', type)) FROM accounts";

  PGresult *res = PQexec(conn, query);

  if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
    char *json_ptr = PQgetvalue(res, 0, 0);
    size_t len = PQgetlength(res, 0, 0);

    if (json_ptr && len > 0) {
      send_large_json(fd, "200 OK", json_ptr, len);
    } else {
      send_json(fd, "200 OK", "[]");
    }
  } else {
    send_json(fd, "500 Error", "{\"error\": \"Query Failed\"}");
  }
  PQclear(res);
}
