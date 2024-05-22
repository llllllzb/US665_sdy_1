#ifndef DEVICD_MODEL_H_
#define DEVICD_MODEL_H_

#include <stdlib.h>
#include <stdio.h>

typedef enum {
  ACCESS_MODE_RO,
  ACCESS_MODE_WO,
  ACCESS_MODE_RW,
  ACCESS_MODE_UNKNOWN
} PropertyAccessMode;

typedef enum {
  DATA_TYPE_INT,
  DATA_TYPE_DOUBLE,
  DATA_TYPE_STRING,
  DATA_TYPE_UNKNOWN
} PropertyDataType;

const char *GetDeviceType();
PropertyAccessMode GetPropertyAccessMode(const char *property_id);
PropertyDataType GetPropertyDataType(const char *property_id);
#endif

