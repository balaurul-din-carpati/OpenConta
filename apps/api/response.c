#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "response.h"

void send_response(int socket, const char* status, const char* body) {
  char header[512];
  snprintf(header, sizeof(header),
    "HTTP/1.1 %s\r\n"
    "Content-Type: application/json\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Connection: close\r\n"
    "Content-Length: %lu\r\n\r\n",
    status, strlen(body));

  write(socket, header, strlen(header));
  write(socket, body, strlen(body));
}

void send_json(int socket, const char* status, const char* json_body) {
  send_response(socket, status, json_body);
}

void send_large_json(int socket, const char* status, const char* data, size_t len) {
  char header[512];
  snprintf(header, sizeof(header),
    "HTTP/1.1 %s\r\n"
    "Content-Type: application/json\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Connection: close\r\n"
    "Content-Length: %lu\r\n\r\n",
    status, len);

  write(socket, header, strlen(header));

  size_t total_sent = 0;
  while (total_sent < len) {
    size_t chunk_size = 65536;
    if (len - total_sent < chunk_size) {
      chunk_size = len - total_sent;
    }

    ssize_t bytes_written = write(socket, data + total_sent, chunk_size);

    if (bytes_written < 0) {
      break;
    }

    total_sent += bytes_written;
  }
}
