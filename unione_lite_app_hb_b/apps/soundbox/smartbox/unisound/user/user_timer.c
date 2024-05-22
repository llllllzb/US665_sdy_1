#include "asm/br23.h"
#include "cpu.h"
#include "timer.h"
#include "user_timer.h"

#define LOG_TAG_CONST       USER_TIMER
#define LOG_TAG             "[USER_TIMER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

uint16_t user_timer_create(uint32_t msec, bool single_shot, user_timer_cb cb, void *priv)
{
  if (single_shot) {
    return usr_timeout_add(priv, cb, msec, 1);
  }
  return usr_timer_add(priv, cb, msec, 1);
}

int user_timer_delete(uint16_t id, bool single_shot)
{
  if (single_shot) {
    usr_timeout_del(id);
  } else {
    usr_timer_del(id);
  }
  return 0;
}
