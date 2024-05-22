#include "bt_preproc.h"
#include "device_model.h"
#include <string.h>
#include <stdint.h>
#include "cJSON.h"

static char g_bt_buf[256];
static char g_cmd_buf[64];

static const char* GetUdid() {
  return "abcdef";
}

static unsigned int GetTraceId() {
  static unsigned int trace_id = 0;
  return trace_id++;
}

char* uni_bt_preproc_pack(const char *cmd) {
  const char *bt_str = "{\"header\":{\"traceId\":\"%010u\",\"payloadVersion\":1,\"udid\":\"%s\",\"timestamp\":0,\"method\":\"device-properties-report\"},\"body\":{\"properties\":[{\"id\":\"%s\",\"value\":%s%s%s}]}}";
  PropertyDataType data_type;
  //step1: parse property:value
  char *p;
  char property[32] = {0};
  char value[32] = {0};
  if (cmd == NULL) {
    return NULL;
  }
  p = strstr(cmd, "#");
  if (p == NULL || p == cmd) {
    return NULL;
  }
  strncpy(property, cmd, p - cmd);
  property[p - cmd] = '\0';
  strcpy(value, p + 1);
  //step2: pack bt frame
  data_type = GetPropertyDataType(property);
  if (DATA_TYPE_UNKNOWN == data_type) {
    printf("property \"%s\" not found in device model\n", property);
    return NULL;
  } else if (DATA_TYPE_STRING == data_type) {
    p = "\"";
  } else {
    p = "";
  }
  snprintf(g_bt_buf, sizeof(g_bt_buf), bt_str, GetTraceId(), GetUdid(), property, p, value, p);
  return g_bt_buf;
}

char* uni_bt_preproc_unpack(const char *bt_data) {
  cJSON *jdata, *jbody, *jproperties, *jproperty, *jid, *jvalue;
  PropertyDataType data_type;
  if (bt_data == NULL) {
    printf("invalid input\n");
    return NULL;
  }
  jdata = cJSON_Parse(bt_data);
  if (jdata == NULL) {
    printf("parse data failed\n");
    return NULL;
  }
  jbody = cJSON_GetObjectItem(jdata, "body");
  if (jbody == NULL) {
    printf("parse body failed\n");
    cJSON_Delete(jdata);
    return NULL;
  }
  jproperties = cJSON_GetObjectItem(jbody, "properties");
  if (jproperties == NULL) {
    printf("parse properties failed\n");
    cJSON_Delete(jdata);
    return NULL;
  }
  if (cJSON_GetArraySize(jproperties) < 1) {
    cJSON_Delete(jdata);
    return NULL;
  }
  jproperty = cJSON_GetArrayItem(jproperties, 0);
  jid = cJSON_GetObjectItem(jproperty, "id");
  jvalue = cJSON_GetObjectItem(jproperty, "value");
  if (jid == NULL || jvalue == NULL) {
    printf("parse %s failed\n", jid == NULL ? "id" : "value");
    cJSON_Delete(jdata);
    return NULL;
  }
  data_type = GetPropertyDataType(jid->valuestring);
  if (data_type == DATA_TYPE_INT) {
    snprintf(g_cmd_buf, sizeof(g_cmd_buf), "%s#%lld", jid->valuestring, jvalue->valueint);
  } else if (data_type == DATA_TYPE_DOUBLE) {
    snprintf(g_cmd_buf, sizeof(g_cmd_buf), "%s#%f", jid->valuestring, jvalue->valuedouble);
  } else if (data_type == DATA_TYPE_STRING) {
    snprintf(g_cmd_buf, sizeof(g_cmd_buf), "%s#%s", jid->valuestring, jvalue->valuestring);
  } else {
    printf("property \"%s\" not found in device model\n", jid->valuestring);
    cJSON_Delete(jdata);
    return NULL;
  }
  cJSON_Delete(jdata);
  return g_cmd_buf;
}

