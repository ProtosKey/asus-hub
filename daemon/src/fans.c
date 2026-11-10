#include "fans.h"
#include "utils.h"
#include <cjson/cJSON.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

cJSON *get_fans_object(void) {
  cJSON *fans = cJSON_CreateObject();
  DIR *dir = opendir("/sys/class/hwmon");
  if (!dir) {
    return fans;
  }

  struct dirent *entry;
  char path[256];
  while ((entry = readdir(dir)) != NULL) {
    if (strncmp(entry->d_name, "hwmon", 5) != 0) {
      continue;
    }

    snprintf(path, sizeof(path), "/sys/class/hwmon/%s/name", entry->d_name);
    char *name = read_file(path);
    if (name && strcmp(name, "asus") == 0) {
      snprintf(path, sizeof(path), "/sys/class/hwmon/%s/fan1_input",
               entry->d_name);
      char *fan1 = read_file(path);
      if (fan1) {
        cJSON_AddNumberToObject(fans, "cpu", atoi(fan1));
        free(fan1);
      }

      snprintf(path, sizeof(path), "/sys/class/hwmon/%s/fan2_input",
               entry->d_name);
      char *fan2 = read_file(path);
      if (fan2) {
        cJSON_AddNumberToObject(fans, "gpu", atoi(fan2));
        free(fan2);
      }

      free(name);
      break;
    }
    if (name) {
      free(name);
    }
  }
  closedir(dir);
  return fans;
}
