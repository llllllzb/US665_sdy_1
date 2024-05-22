/**************************************************************************
 * Copyright (C) 2017-2017  Unisound
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
 * Description : user_gpio.h
 * Author      : yuanshifeng@unisound.com
 * Date        : 2021.03.01
 *
 **************************************************************************/
#ifndef USER_GPIO_H_
#define USER_GPIO_H_

#include <stdint.h>
#include "asm/br23.h"
#include "asm/mcpwm.h"
#include "asm/clock.h"
#include "asm/gpio.h"

#define PWM_HZ_MAX     (1000 * 1000)  // max 1MHz (recommand below 40KHz on HB-L)
#define GPIO_PWM_MAX   6
#define GPIO_UART_MAX  3
#define GPIO_PULSE_MAX 4

typedef struct {
  uint8_t used;
  uint32_t lhchannel;
  uint32_t freq;
  uint8_t duty;
  uint8_t inverse;
  struct pwm_platform_data handle;
  uint8_t      ch;
} pwm_info_t;

typedef struct {
  UNI_TIMER_TypeDef *timer_handler;
  int timer_id;
  int port;
  int period_ms;
  int times;
  int def_val;
  uint8_t used;
  uint8_t is_top;
} gpio_pulse_t;

/** @addtogroup uni_gpio_def
@{*/
/**
 * @brief gpio 电平状态
 */
typedef enum {
  UNI_GPIO_LEVEL_LOW = 0, ///< 低电平
  UNI_GPIO_LEVEL_HIGH     ///< 高电平
} UNI_GPIO_LEVEL_E;

/**
 * @brief gpio 方向
 */
typedef enum {
  UNI_GPIO_DIRECTION_OUTPUT = 0,    ///< GPIO端口作为输出
  UNI_GPIO_DIRECTION_INPUT,       ///< GPIO端口作为输入
} UNI_GPIO_DIRECTION_E;

/**
 * @brief gpio 引脚号
 */
typedef enum {
  UNI_PIN_NUM_1 = IO_PORT_DM,
  UNI_PIN_NUM_2 = IO_PORT_DP,
  UNI_PIN_NUM_3 = IO_PORTA_01,
  UNI_PIN_NUM_4 = IO_PORTA_00,
  UNI_PIN_NUM_13 = IO_PORTB_11,
  UNI_PIN_NUM_14 = IO_PORTB_10,
  UNI_PIN_NUM_15 = IO_PORTB_09,
  UNI_PIN_NUM_16 = IO_PORTB_08,
  UNI_PIN_NUM_17 = IO_PORTB_05,
  UNI_PIN_NUM_20 = IO_PORTB_01,
} UNI_PIN_NUM_E;

/**
 * @brief gpio 功能
 */
typedef enum {
  UNI_FUNC_GPIO,
  UNI_FUNC_UART_TX,
  UNI_FUNC_UART_RX,
  UNI_FUNC_PWM
} UNI_PIN_FUNC_E;
/** @}*/

typedef enum {
  UNI_PWM0H = 0,
  UNI_PWM0L,
  UNI_PWM1H,
  UNI_PWM1L,
  UNI_PWM2H,
  UNI_PWM2L,
  UNI_PWM3H,
  UNI_PWM3L,
  UNI_PWM4H,
  UNI_PWM4L,
  UNI_PWM5H,
  UNI_PWM5L,
} UNI_PWM_LR_E;

/** @addtogroup uni_gpio_interface
@{*/
/**
 * @brief GPIO端口功能配置
 * @param user_pin 端口号
 * @param user_func 端口功能
 */
void user_pin_set_func(UNI_PIN_NUM_E user_pin, UNI_PIN_FUNC_E user_func);

/**
 * @brief GPIO端口方向配置
 * @param user_pin 端口号
 * @param direction 方向
 * @retval 0 操作成功
 * @retval -1 操作失败
 */
int user_gpio_set_direction(UNI_PIN_NUM_E user_pin, UNI_GPIO_DIRECTION_E direction);

/**
 * @brief GPIO电平获取
 * @param user_pin 端口号
 * @retval 电平值 0或1
 */
int user_gpio_get_level(UNI_PIN_NUM_E pin);

/**
 * @brief GPIO电平配置
 * @param user_pin 端口号
 * @param level 电平，0或1
 * @retval 0 操作成功
 * @retval -1 操作失败
 */
int user_gpio_set_level(UNI_PIN_NUM_E user_pin, UNI_GPIO_LEVEL_E level);
/** @}*/

/** @addtogroup uni_pwm_interface
@{*/
/**
 * @brief PWM端口初始化
 * @param user_pin GPIO端口号
 * @param freq PWM波形频率
 * @param inverse 占空比翻转，0：不翻转，为高电平设置占空比；1：翻转，为低电平设置占空比
 * @param duty 占空比
 * @retval 0 操作成功
 * @retval -1 操作失败
 */
int user_pwm_init(UNI_PIN_NUM_E user_pin, uint32_t freq, uint8_t inverse, uint8_t duty);
/**
 * @brief PWM通道增加占空比
 * @param lhchannel PWM通道号
 * @param det_duty 占空比增加值
 * @retval 0 操作成功
 * @retval -1 操作失败
 */
int user_pwm_duty_inc(uint32_t lhchannel, uint8_t det_duty);
/**
 * @brief PWM通道减小占空比
 * @param lhchannel PWM通道号
 * @param det_duty 占空比减小值
 * @retval 0 操作成功
 * @retval -1 操作失败
 */
int user_pwm_duty_dec(uint32_t lhchannel, uint8_t det_duty);
/**
 * @brief PWM通道使能
 * @param lhchannel PWM通道号
 * @param duty 占空比
 * @retval 0 操作成功
 * @retval -1 操作失败
 */
int user_pwm_enable(uint32_t lhchannel, uint8_t duty);
/**
 * @brief PWM通道关闭
 * @param lhchannel PWM通道号
 * @retval 0 操作成功
 * @retval -1 操作失败
 */
int user_pwm_disable(uint32_t lhchannel);

/**
 * @brief PWM3端口启动
 * @param freq PWM波形频率
 * @param duty 占空比,0~10000对应占空比0~100%
 * @retval 0 操作成功
 * @retval -1 操作失败
 */
int user_pwm3_start(u32 freq, u32 duty);
/**
 * @brief PWM3通道占空比设置
 * @param duty 占空比,0~10000对应占空比0~100%
 */
void user_pwm3_set_duty(u32 duty);
/**
 * @brief PWM3通道停止
 */
void user_pwm3_stop(void);
/** @}*/

/** @addtogroup uni_uart_interface
@{*/
/**
 * @brief UART初始化
 * @param tx_pin GPIO发送端口号
 * @param rx_pin GPIO接收端口号
 * @param baudrate 波特率
 * @param data_bits 数据位（暂未使用）
 * @param stop_bits 停止位（暂未使用）
 * @param parity 校验位（暂未使用）
 * @retval 0 操作成功
 * @retval -1 操作失败
 */
int user_uart_init(UNI_PIN_NUM_E tx_pin, UNI_PIN_NUM_E rx_pin, uint32_t baudrate,
		   int data_bits, int stop_bits, int parity);
       /**
 * @brief UART数据发送
 * @param port UART端口号
 * @param data 待发送数据起始地址
 * @param size 待发送数据长度
 * @retval 0 操作成功
 * @retval -1 操作失败
 */
int user_uart_send(int port, uint8_t *data, uint32_t size);
/**
 * @brief UART数据接收
 * @param port UART端口号
 * @param data 接收数据的缓冲区起始地址
 * @param size 接收数据的长度
 * @param timeout 超时时间，单位为ms。
 * @retval 实际接收数据长度
 */
uint32_t user_uart_recv(int port, uint8_t *data, uint32_t size, uint32_t timeout);
/**
 * @brief UART数据单字节接收
 * @param port UART端口号
 * @param data 接收数据的缓冲区起始地址
 * @param timeout 超时时间，单位为ms。
 * @retval 0 操作成功
 * @retval -1 操作失败
 */
int user_uart_recv_byte(int port, uint8_t *data, uint32_t timeout);
/** @}*/

/** @addtogroup uni_gpio_interface
@{*/
/**
 * @brief GPIO端口定时输出脉冲设置
 * @param user_pin 端口号
 * @param period_ms 脉冲周期
 * @param times 脉冲次数
 * @param dev_val 脉冲结束持续电平，0或1
 * @retval 0 操作成功
 * @retval 1 操作失败
 */
int user_sw_timer_gpio_pulse(UNI_PIN_NUM_E user_pin, int period_ms, int times, uint8_t dev_val);
/** @}*/
#endif
