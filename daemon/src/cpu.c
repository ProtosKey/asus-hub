#include "cpu.h"
#include "utils.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FORMAT_SIZE 64
#define BUFFER_SIZE 256

static char *get_cpu_model(void) {
  char *cpu_info = read_file("/proc/cpuinfo");
  if (!cpu_info) {
    return NULL;
  }

  char *model = NULL;
  char *cpu_line = strstr(cpu_info, "model name");
  if (cpu_line) {
    char format[FORMAT_SIZE];
    char buffer[BUFFER_SIZE];
    snprintf(format, sizeof(format), "model name : %%%d[^\n]", BUFFER_SIZE - 1);
    if (sscanf(cpu_line, format, buffer) == 1) {
      model = strdup(buffer);
    }
  }

  free(cpu_info);
  return model;
}

cJSON *get_cpu_array(void) {
  cJSON *cpu_array = cJSON_CreateArray();
  cJSON *cpu_obj = cJSON_CreateObject();

  char *model = get_cpu_model();
  cJSON_AddStringToObject(cpu_obj, "name", model ? model : "Unknown CPU");
  if (model) {
    free(model);
  }

  char *temp_str = run_cmd("sensors | grep Tctl | awk '{print $2}'");
  if (temp_str && strlen(temp_str) > 0) {
    cJSON_AddNumberToObject(cpu_obj, "temp", atoi(temp_str + 1));
    free(temp_str);
  } else {
    cJSON_AddNullToObject(cpu_obj, "temp");
    if (temp_str) {
      free(temp_str);
    }
  }

  cJSON_AddItemToArray(cpu_array, cpu_obj);
  return cpu_array;
}
