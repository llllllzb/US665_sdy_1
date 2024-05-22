#include "device_model.h"

typedef struct PropertyModel_ {
  char id[16];
  PropertyAccessMode access_mode;
  PropertyDataType data_type;
} PropertyModel;

// auto_code.py edit start
typedef struct DeviceModel_ {
  const char *device_type;
  //STUB: array length
  int property_number;
  PropertyModel property_models[8];
} DeviceModel;

//STUB: model init
static DeviceModel g_device_model = {
  "device_type_stub",
  8,
  {
    {"volume", ACCESS_MODE_RW, DATA_TYPE_INT},
    {"mesageGear", ACCESS_MODE_RW, DATA_TYPE_INT},
    {"heatGear", ACCESS_MODE_RW, DATA_TYPE_INT},
    {"ac_power_off", ACCESS_MODE_RW, DATA_TYPE_INT},
    {"messageMode", ACCESS_MODE_RW, DATA_TYPE_INT},
    {"speed", ACCESS_MODE_RW, DATA_TYPE_INT},
    {"heatSwitch", ACCESS_MODE_RW, DATA_TYPE_INT},
    {"powerSwitch", ACCESS_MODE_RW, DATA_TYPE_INT},
    {"id_stub", ACCESS_MODE_UNKNOWN, DATA_TYPE_UNKNOWN}
  }
};
// auto_code.py edit end

const char *GetDeviceType() {
  return g_device_model.device_type;
}

PropertyAccessMode GetPropertyAccessMode(const char *property_id) {
  int i;
  for (i = 0; i < g_device_model.property_number; i++) {
    if (strcmp(g_device_model.property_models[i].id, property_id) == 0) {
      return g_device_model.property_models[i].access_mode;
    }
  }
  return ACCESS_MODE_UNKNOWN;
}

PropertyDataType GetPropertyDataType(const char *property_id) {
  int i;
  for (i = 0; i < g_device_model.property_number; i++) {
    if (strcmp(g_device_model.property_models[i].id, property_id) == 0) {
      return g_device_model.property_models[i].data_type;
    }
  }
  return DATA_TYPE_UNKNOWN;
}

#if 0
void main() {
  printf("%s %d %d\n", GetDeviceType(), GetPropertyAccessMode("id_stub2"), GetPropertyDataType("id_stub2"));
}
#endif
