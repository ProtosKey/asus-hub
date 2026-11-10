#include "gpu.h"
#include "utils.h"
#include <cjson/cJSON.h>
#include <dirent.h>
#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 256

static char *get_gpu_name_from_card(const char *card_name) {
  char syspath[BUFFER_SIZE];
  snprintf(syspath, sizeof(syspath), "/sys/class/drm/%s/device", card_name);

  struct udev *udev = udev_new();
  if (!udev) {
    return NULL;
  }

  struct udev_device *dev = udev_device_new_from_syspath(udev, syspath);
  if (!dev) {
    udev_unref(udev);
    return NULL;
  }

  const char *model =
      udev_device_get_property_value(dev, "ID_MODEL_FROM_DATABASE");
  char *result = model ? strdup(model) : NULL;

  udev_device_unref(dev);
  udev_unref(udev);
  return result;
}

static int get_gpu_temp_by_pci(const char *card_name) {
  char uevent_path[BUFFER_SIZE];
  snprintf(uevent_path, sizeof(uevent_path), "/sys/class/drm/%s/device/uevent",
           card_name);

  char *uevent = read_file(uevent_path);
  if (!uevent) {
    return -1;
  }

  char *pci_line = strstr(uevent, "PCI_SLOT_NAME=");
  if (!pci_line) {
    free(uevent);
    return -1;
  }

  char pci_addr[32];
  if (sscanf(pci_line, "PCI_SLOT_NAME=%31s", pci_addr) != 1) {
    free(uevent);
    return -1;
  }
  free(uevent);

  DIR *dir = opendir("/sys/class/hwmon");
  if (!dir) {
    return -1;
  }

  struct dirent *entry;
  char path[256];
  char device_link[256];
  int temp = -1;

  while ((entry = readdir(dir)) != NULL) {
    if (strncmp(entry->d_name, "hwmon", 5) != 0) {
      continue;
    }

    snprintf(path, sizeof(path), "/sys/class/hwmon/%s/device", entry->d_name);
    ssize_t len = readlink(path, device_link, sizeof(device_link) - 1);
    if (len == -1) {
      continue;
    }

    device_link[len] = '\0';
    if (strstr(device_link, pci_addr)) {
      snprintf(path, sizeof(path), "/sys/class/hwmon/%s/temp1_input",
               entry->d_name);
      char *temp_str = read_file(path);
      if (temp_str) {
        temp = atoi(temp_str) / 1000;
        free(temp_str);
      }
      break;
    }
  }

  closedir(dir);
  return temp;
}

static int get_gpu_vendor(const char *card_name) {
  char path[BUFFER_SIZE];
  snprintf(path, sizeof(path), "/sys/class/drm/%s/device/vendor", card_name);

  char *vendor_str = read_file(path);
  if (!vendor_str) {
    return -1;
  }

  int vendor = -1;
  if (sscanf(vendor_str, "0x%x", &vendor) != 1) {
    free(vendor_str);
    return -1;
  }
  free(vendor_str);
  return vendor;
}

cJSON *get_gpu_array(void) {
  cJSON *gpu_array = cJSON_CreateArray();

  char *nvidia_name = NULL;
  int nvidia_temp = -1;
  double nvidia_power = -1.0;

  char *nvidia_str =
      run_cmd("nvidia-smi --query-gpu=name,temperature.gpu,power.draw "
              "--format=csv,noheader,nounits");
  if (nvidia_str && strlen(nvidia_str) > 0) {
    char name_buf[BUFFER_SIZE];
    char format[64];
    snprintf(format, sizeof(format), " %%%d[^,], %%d, %%lf", BUFFER_SIZE - 1);
    if (sscanf(nvidia_str, format, name_buf, &nvidia_temp, &nvidia_power) >=
        1) {
      nvidia_name = strdup(name_buf);
    }
  }
  if (nvidia_str) {
    free(nvidia_str);
  }

  DIR *dir = opendir("/sys/class/drm");
  if (dir) {
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
      if (strncmp(entry->d_name, "card", 4) != 0) {
        continue;
      }
      if (strchr(entry->d_name, '-') != NULL) {
        continue;
      }

      char *name = get_gpu_name_from_card(entry->d_name);
      if (!name) {
        continue;
      }

      cJSON *gpu_obj = cJSON_CreateObject();
      cJSON_AddStringToObject(gpu_obj, "name", name);

      int vendor = get_gpu_vendor(entry->d_name);
      if (vendor == 0x10de && nvidia_name) {
        if (nvidia_temp >= 0) {
          cJSON_AddNumberToObject(gpu_obj, "temp", nvidia_temp);
        }
        if (nvidia_power >= 0) {
          cJSON_AddNumberToObject(gpu_obj, "power", nvidia_power);
        }
      } else {
        int temp = get_gpu_temp_by_pci(entry->d_name);
        if (temp >= 0) {
          cJSON_AddNumberToObject(gpu_obj, "temp", temp);
        }
      }

      cJSON_AddItemToArray(gpu_array, gpu_obj);
      free(name);
    }
    closedir(dir);
  }

  if (nvidia_name) {
    free(nvidia_name);
  }
  return gpu_array;
}
