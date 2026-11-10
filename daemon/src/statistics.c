#include "statistics.h"
#include "cpu.h"
#include "fans.h"
#include "gpu.h"
#include "utils.h"
#include <cjson/cJSON.h>
#include <stdlib.h>
#include <string.h>

char *collect_statistics(void) {
  cJSON *root = cJSON_CreateObject();
  if (!root) {
    return NULL;
  }

  char *profile =
      run_cmd("asusctl profile get | grep Active | awk '{print $3}'");
  if (profile && strlen(profile) > 0) {
    cJSON_AddStringToObject(root, "profile", profile);
    free(profile);
  } else {
    cJSON_AddStringToObject(root, "profile", "Unknown");
    if (profile) {
      free(profile);
    }
  }

  cJSON_AddItemToObject(root, "cpu", get_cpu_array());
  cJSON_AddItemToObject(root, "gpu", get_gpu_array());
  cJSON_AddItemToObject(root, "fans", get_fans_object());

  char *json_str = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);
  return json_str;
}
