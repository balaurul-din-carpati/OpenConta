#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include "router.h"

#define PORT 5000
#define THREAD_POOL_SIZE 16
#define QUEUE_SIZE 1000

#define RATE_LIMIT_WINDOW 10
#define RATE_LIMIT_MAX 50
#define MAX_TRACKED_IPS 2000

typedef struct {
  int client_socket;
  char initial_buffer[4096];
} task_t;

typedef struct {
  unsigned long ip_hash;
  time_t start_time;
  int count;
} ip_track_t;

task_t task_queue[QUEUE_SIZE];
int queue_count = 0, queue_head = 0, queue_tail = 0;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

ip_track_t ip_history[MAX_TRACKED_IPS];
int ip_history_idx = 0;

unsigned long hash_ip(const char *str) {
  unsigned long hash = 5381;
  int c;
  while ((c = *str++)) hash = ((hash << 5) + hash) + c;
  return hash;
}

PGconn* connect_db() {
  const char *db_pass = getenv("DB_PASS");
  if (!db_pass) db_pass = "@aAmereacre1";
  char conn_str[256];
  snprintf(conn_str, sizeof(conn_str), "host=localhost user=openconta dbname=openconta_db password=%s", db_pass);
  return PQconnectdb(conn_str);
}

void *worker_function(void *arg) {
  PGconn *conn = connect_db();
  if (PQstatus(conn) != CONNECTION_OK) pthread_exit(NULL);

  while (1) {
    int client_socket = 0;
    char buffer[4096] = {0};

    pthread_mutex_lock(&queue_mutex);
    while (queue_count == 0) pthread_cond_wait(&queue_cond, &queue_mutex);

    client_socket = task_queue[queue_head].client_socket;
    memcpy(buffer, task_queue[queue_head].initial_buffer, 4096);

    queue_head = (queue_head + 1) % QUEUE_SIZE;
    queue_count--;
    pthread_mutex_unlock(&queue_mutex);

    router_handle_request(client_socket, conn, buffer);
    close(client_socket);
  }
  PQfinish(conn);
  return NULL;
}

void get_real_ip(char *buffer, struct sockaddr_in *client_addr, char *out_ip) {
  char *header = strstr(buffer, "X-Real-IP: ");

  if (header) {
    sscanf(header, "X-Real-IP: %s", out_ip);
    char *end = strpbrk(out_ip, "\r\n");
    if (end) *end = 0;
  } else {
    inet_ntop(AF_INET, &(client_addr->sin_addr), out_ip, INET_ADDRSTRLEN);
  }
}

int is_ip_blocked(const char *ip_str) {
  unsigned long current_hash = hash_ip(ip_str);
  time_t now = time(NULL);
  int found_idx = -1;

  for (int i = 0; i < MAX_TRACKED_IPS; i++) {
    if (ip_history[i].ip_hash == current_hash) {
      found_idx = i;
      break;
    }
  }

  if (found_idx != -1) {
    if (now - ip_history[found_idx].start_time > RATE_LIMIT_WINDOW) {
      ip_history[found_idx].start_time = now;
      ip_history[found_idx].count = 1;
      return 0;
    } else {
      ip_history[found_idx].count++;
      if (ip_history[found_idx].count > RATE_LIMIT_MAX) return 1;
      return 0;
    }
  } else {
    ip_history[ip_history_idx].ip_hash = current_hash;
    ip_history[ip_history_idx].start_time = now;
    ip_history[ip_history_idx].count = 1;
    ip_history_idx = (ip_history_idx + 1) % MAX_TRACKED_IPS;
    return 0;
  }
}

int main() {
  router_init();
  memset(ip_history, 0, sizeof(ip_history));

  pthread_t thread_pool[THREAD_POOL_SIZE];
  for (int i = 0; i < THREAD_POOL_SIZE; i++)
    pthread_create(&thread_pool[i], NULL, worker_function, NULL);

  int server_fd, new_socket;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  int opt = 1;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  bind(server_fd, (struct sockaddr *)&address, sizeof(address));
  listen(server_fd, 1000);

  printf("OpenConta Modular Server running on port %d\n", PORT);

  while (1) {
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) continue;

    char temp_buffer[4096] = {0};
    ssize_t bytes_read = read(new_socket, temp_buffer, 4096);

    if (bytes_read <= 0) {
      close(new_socket);
      continue;
    }

    char real_ip[64] = {0};
    get_real_ip(temp_buffer, &address, real_ip);

    if (is_ip_blocked(real_ip)) {
      printf("[BLOCK] IP Real %s blocked\n", real_ip);
      close(new_socket);
      continue;
    }

    pthread_mutex_lock(&queue_mutex);
    if (queue_count < QUEUE_SIZE) {
      task_queue[queue_tail].client_socket = new_socket;
      memcpy(task_queue[queue_tail].initial_buffer, temp_buffer, 4096);

      queue_tail = (queue_tail + 1) % QUEUE_SIZE;
      queue_count++;
      pthread_cond_signal(&queue_cond);
    } else {
      printf("[DROP] Queue full. Dropped %s\n", real_ip);
      close(new_socket);
    }
    pthread_mutex_unlock(&queue_mutex);
  }
  return 0;
}
