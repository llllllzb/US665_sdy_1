/**************************************************************************
 * Copyright (C) 2020-2020  Unisound
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **************************************************************************
 *
 * Description : uni_auto_control.c
 * Author      : yuanshifeng@unisound.com
 * Date        : 2021.03.01
 *
 **************************************************************************/
#include "user_gpio.h"
#include "uni_iot.h"

#define LOG_TAG_CONST       UNI_AUTO_CTRL
#define LOG_TAG             "[UNI_AUTO_CTRL]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

static void _user_gpio_set_pinmux(void) {
  user_pin_set_func(UNI_PIN_NUM_3, UNI_FUNC_GPIO);
  user_pin_set_func(UNI_PIN_NUM_4, UNI_FUNC_GPIO);
  user_pin_set_func(UNI_PIN_NUM_13, UNI_FUNC_GPIO);
  user_pin_set_func(UNI_PIN_NUM_14, UNI_FUNC_UART_RX);
  user_pin_set_func(UNI_PIN_NUM_15, UNI_FUNC_UART_TX);
  user_pin_set_func(UNI_PIN_NUM_16, UNI_FUNC_GPIO);
  user_pin_set_func(UNI_PIN_NUM_20, UNI_FUNC_GPIO);
}

int user_gpio_init(void) {
  _user_gpio_set_pinmux();

  user_gpio_set_direction(UNI_PIN_NUM_3, UNI_GPIO_DIRECTION_OUTPUT);
  user_gpio_set_level(UNI_PIN_NUM_3, UNI_GPIO_LEVEL_LOW);
  user_gpio_set_direction(UNI_PIN_NUM_4, UNI_GPIO_DIRECTION_OUTPUT);
  user_gpio_set_level(UNI_PIN_NUM_4, UNI_GPIO_LEVEL_LOW);
  user_gpio_set_direction(UNI_PIN_NUM_13, UNI_GPIO_DIRECTION_OUTPUT);
  user_gpio_set_level(UNI_PIN_NUM_13, UNI_GPIO_LEVEL_LOW);
  user_gpio_set_direction(UNI_PIN_NUM_16, UNI_GPIO_DIRECTION_OUTPUT);
  user_gpio_set_level(UNI_PIN_NUM_16, UNI_GPIO_LEVEL_LOW);
  user_gpio_set_direction(UNI_PIN_NUM_20, UNI_GPIO_DIRECTION_OUTPUT);
  user_gpio_set_level(UNI_PIN_NUM_20, UNI_GPIO_LEVEL_LOW);
  user_uart_init(UNI_PIN_NUM_15, UNI_PIN_NUM_14, 115200, 5, 1, 0);

  LOGI(LOG_TAG, " %s success", __func__);
  return 0;
}


int user_gpio_handle_action_event(const char *action,
                               uint8_t *need_reply, uint8_t *need_post) {
  if (action != NULL) {
    LOGD(LOG_TAG, "handle kws result action: %s", action);
    return -1;
  } else {
    return -1;
  }

  return 0;
}
