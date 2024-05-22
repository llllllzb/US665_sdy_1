/**
 * Copyright 2020, unisound.com. All rights reserved.
 */

#include "user_iot.h"
#include "uni_iot.h"
#include "bt_preproc.h"

#define LOG_TAG_CONST       USER_IOT
#define LOG_TAG             "[USER_IOT]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

uni_mutex_t user_iot_voice_lock;

int user_iot_init(void) {
  uni_pthread_mutex_init(&user_iot_voice_lock);
  return 0;
}

int user_iot_voice_set(char *kw_str) {
  uni_pthread_mutex_lock(&user_iot_voice_lock);
  char property_payload[256] = {0};
  char property[32] = {0};
  char val[32] = {0};
  char *p = NULL;
  char *post_pack = NULL;
  
  p = strstr(kw_str, "#");
  if (p == NULL) {
    goto END_ERR;
  }
  strncpy(property, kw_str, p - kw_str);
  property[p - kw_str] = '\0';
  p = strstr(p + 1, "#") + 1;
  if (p == NULL) {
    goto END_ERR;
  }
  strcpy(val, p);
  snprintf(property_payload, sizeof(property_payload), "%s#%s", property, val);
  post_pack = uni_bt_preproc_pack(property_payload);
  if (post_pack == NULL) {
    goto END_ERR;
  }
  //TBD send post_pack by bt api here

  uni_pthread_mutex_unlock(&user_iot_voice_lock);

  return 0;

END_ERR:
  uni_pthread_mutex_unlock(&user_iot_voice_lock);
  return -1;
}
