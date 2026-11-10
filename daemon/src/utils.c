#include "utils.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 256

char *run_cmd(const char *cmd) {
  FILE *pipe = popen(cmd, "r");
  if (!pipe) {
    return NULL;
  }

  char buffer[BUFFER_SIZE];
  if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
    buffer[strcspn(buffer, "\n")] = 0;
  } else {
    buffer[0] = '\0';
  }

  int status = pclose(pipe);
  if (status == -1) {
    return NULL;
  }

  return strdup(buffer);
}

char *read_file(const char *path) {
  char *buffer = NULL;
  int fd = open(path, O_RDONLY);
  if (fd == -1) {
    goto error_exit;
  }

  size_t capacity = BUFFER_SIZE;
  buffer = malloc(capacity * sizeof(char));
  if (buffer == NULL) {
    goto error_exit;
  }

  size_t total_bytes = 0;
  ssize_t bytes_read = 0;
  while (1) {
    if (capacity - total_bytes <= 1) {
      capacity *= 2;
      char *new_buffer = realloc(buffer, capacity * sizeof(char));
      if (new_buffer == NULL) {
        goto error_exit;
      }
      buffer = new_buffer;
    }

    bytes_read = read(fd, buffer + total_bytes, capacity - total_bytes - 1);
    if (bytes_read == 0) {
      break;
    }
    if (bytes_read == -1) {
      if (errno == EINTR) {
        continue;
      }
      goto error_exit;
    }
    total_bytes += bytes_read;
  }

  close(fd);
  buffer[total_bytes] = '\0';
  if (total_bytes > 0 && buffer[total_bytes - 1] == '\n') {
    buffer[total_bytes - 1] = '\0';
  }

  return buffer;
error_exit:
  if (fd != -1)
    close(fd);
  free(buffer);
  return NULL;
}
