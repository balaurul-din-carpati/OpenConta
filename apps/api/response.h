#ifndef RESPONSE_H
#define RESPONSE_H

#include <stddef.h>

void send_response(
  int socket, 
  const char* status, 
  const char* body
  );
void send_json(
  int socket, 
  const char* status, 
  const char* json_body
  );
void send_large_json(
  int socket, 
  const char* status, 
  const char* data, 
  size_t len
  );

#endif
