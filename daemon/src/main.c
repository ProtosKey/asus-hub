#include "statistics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/asus-hub.sock"
#define BUFFER_SIZE 512

int main(void) {
  int server_fd, client_fd;
  struct sockaddr_un addr;
  char buffer[BUFFER_SIZE];

  server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (server_fd == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
  unlink(SOCKET_PATH);

  if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    perror("bind");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  if (listen(server_fd, 5) == -1) {
    perror("listen");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  printf("ASUS-HUB daemon listening on %s\n", SOCKET_PATH);

  while (1) {
    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd == -1) {
      continue;
    }

    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);

    if (bytes_read > 0) {
      buffer[strcspn(buffer, "\n")] = 0;

      if (strncmp(buffer, "status", 6) == 0) {
        char *json = collect_statistics();
        if (json) {
          write(client_fd, json, strlen(json));
          free(json);
        } else {
          const char *err = "{\"error\":\"failed to collect\"}";
          write(client_fd, err, strlen(err));
        }
      } else if (strncmp(buffer, "set_profile ", 12) == 0) {
        const char *profile = buffer + 12;
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "asusctl profile -P %s", profile);

        if (system(cmd) == 0) {
          const char *ok = "{\"status\":\"ok\"}";
          write(client_fd, ok, strlen(ok));
        } else {
          const char *err = "{\"status\":\"error\"}";
          write(client_fd, err, strlen(err));
        }
      } else {
        const char *err = "{\"error\":\"unknown command\"}";
        write(client_fd, err, strlen(err));
      }
    }
    close(client_fd);
  }

  close(server_fd);
  return 0;
}
