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
 * Description : user_gpio.c
 * Author      : yuanshifeng@unisound.com
 * Date        : 2021.03.01
 *
 **************************************************************************/
#include "user_gpio.h"
#include "uni_iot.h"

#define LOG_TAG_CONST       USER_GPIO
#define LOG_TAG             "[USER_GPIO]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

static pwm_info_t g_pwm_info[GPIO_PWM_MAX] = {0};
static const uart_bus_t *g_uart_handler[GPIO_UART_MAX] = {NULL, NULL, NULL};
static gpio_pulse_t g_gpio_pulse[GPIO_PULSE_MAX] = {0};

/****************** IOMUX interface ******************/
void user_pin_set_func(UNI_PIN_NUM_E user_pin, UNI_PIN_FUNC_E user_func) {
  switch(user_func) {
    case UNI_FUNC_GPIO:
      if (user_pin == IO_PORT_DM || user_pin == IO_PORT_DP) {
        usb_iomode(1);
      }
      break;
    case UNI_FUNC_UART_TX:
    case UNI_FUNC_UART_RX:
      if (user_pin == IO_PORT_DM || user_pin == IO_PORT_DP) {
        usb_iomode(1); // need?
      }
      //set func in uart_dev_open api
      break;
    case UNI_FUNC_PWM:
      //seems not needed
      break;
  }
}

/****************** GPIO interface ******************/
int user_gpio_set_direction(UNI_PIN_NUM_E user_pin, UNI_GPIO_DIRECTION_E direction) {
  if (direction == UNI_GPIO_DIRECTION_INPUT) {
    gpio_set_die(user_pin, 1);
  }
  return gpio_set_direction(user_pin, direction);
}

int user_gpio_set_level(UNI_PIN_NUM_E pin, UNI_GPIO_LEVEL_E level) {
  return gpio_set_output_value(pin, level);
}

int user_gpio_get_level(UNI_PIN_NUM_E pin)
{
  return gpio_read(pin);
}

int user_gpio_set_pull_up(UNI_PIN_NUM_E pin, int value)
{
  return gpio_set_pull_up(pin, value);
}

int user_gpio_set_pull_down(UNI_PIN_NUM_E pin, int value)
{
  return gpio_set_pull_down(pin, value);
}

/****************** PWM interface ******************/
static u8 pwm_hw_h_pin[GPIO_PWM_MAX] = {IO_PORTA_00, IO_PORTB_00, IO_PORTB_04, IO_PORTB_09, IO_PORTA_09, IO_PORTC_04};
static u8 pwm_hw_l_pin[GPIO_PWM_MAX] = {IO_PORTA_01, IO_PORTB_02, IO_PORTB_06, IO_PORTB_10, IO_PORTA_10, IO_PORTC_05};
extern void mcpwm_init(struct pwm_platform_data *arg);
extern void mcpwm_close(pwm_ch_num_type pwm_ch, pwm_timer_num_type timer_ch);
extern void mcpwm_set_duty(pwm_ch_num_type pwm_ch, pwm_timer_num_type timer_ch, u16 duty);

static int _get_pwm_h_pin(UNI_PIN_NUM_E user_pin) {
  int i;
  for (i = 0; i < GPIO_PWM_MAX; ++i) {
    if (user_pin == pwm_hw_h_pin[i]) {
      return user_pin;
    }
  }
  return -1;
}

static int _get_pwm_l_pin(UNI_PIN_NUM_E user_pin) {
  int i;
  for (i = 0; i < GPIO_PWM_MAX; ++i) {
    if (user_pin == pwm_hw_l_pin[i]) {
      return user_pin;
    }
  }
  return -1;
}

static int _get_pwm_channel(UNI_PIN_NUM_E user_pin) {
  int i;
  for (i = 0; i < GPIO_PWM_MAX; ++i) {
    if (user_pin == pwm_hw_h_pin[i] || user_pin == pwm_hw_l_pin[i]) {
      return i;
    }
  }
  return -1;
}

static int _pwm_pin_2_lhchannel(UNI_PIN_NUM_E user_pin) {
  int i;
  for (i = 0; i < GPIO_PWM_MAX; ++i) {
    if (user_pin == pwm_hw_h_pin[i]) {
      return i * 2;
    }
    if (user_pin == pwm_hw_l_pin[i]) {
      return i * 2 + 1;
    }
  }
  return -1;
}

int user_pwm_init(UNI_PIN_NUM_E user_pin, uint32_t freq, uint8_t inverse, uint8_t duty) {
  int i;
  int pwm_index = -1;

  for (i = 0; i < GPIO_PWM_MAX; ++i) {
    if (g_pwm_info[i].used == 0) {
      pwm_index = i;
      break;
    }
  }
  if (pwm_index == -1) {
    LOGE(LOG_TAG, "no more pwm channel can init");
    return -1;
  }

  if (_get_pwm_channel(user_pin) == -1) {
    LOGE(LOG_TAG, "no channel for pin %d", user_pin);
    return -1;
  }

  g_pwm_info[pwm_index].handle.pwm_aligned_mode = pwm_edge_aligned;                    //边沿对齐
  g_pwm_info[pwm_index].handle.frequency = freq;                                       //Hz

  g_pwm_info[pwm_index].handle.pwm_ch_num = _get_pwm_channel(user_pin);                //通道
  g_pwm_info[pwm_index].handle.pwm_timer_num = _get_pwm_channel(user_pin);             //时基选择通道
  g_pwm_info[pwm_index].handle.duty = duty * 100;                                      //占空比
  g_pwm_info[pwm_index].handle.h_pin = _get_pwm_h_pin(user_pin);                       //没有则填 -1。h_pin_output_ch_num无效，可不配置
  g_pwm_info[pwm_index].handle.l_pin = _get_pwm_l_pin(user_pin);                       //硬件引脚，l_pin_output_ch_num无效，可不配置
  g_pwm_info[pwm_index].handle.complementary_en = 0;                                   //两个引脚的波形同步

  g_pwm_info[pwm_index].ch = pwm_index;
  g_pwm_info[pwm_index].used = 1;
  g_pwm_info[pwm_index].lhchannel = _pwm_pin_2_lhchannel(user_pin);
  g_pwm_info[pwm_index].freq = freq;
  g_pwm_info[pwm_index].inverse = inverse;
  g_pwm_info[pwm_index].duty = duty;
  user_pwm_enable(g_pwm_info[pwm_index].lhchannel, g_pwm_info[pwm_index].duty);
  return 0;
}

int _find_pwm_info(UNI_PWM_LR_E lhchannel) {
  int i;
  for (i = 0; i < GPIO_PWM_MAX; ++i) {
    if (g_pwm_info[i].used == 1 && g_pwm_info[i].lhchannel == lhchannel) {
      return i;
    }
  }
  return -1;
}

int user_pwm_duty_inc(uint32_t lhchannel, uint8_t det_duty) {
  int pwm_index = _find_pwm_info(lhchannel);
  int new_duty = 0;

  if (pwm_index == -1) {
    LOGE(LOG_TAG, "lhchannel %d is not init", lhchannel);
    return -1;
  }

  if (g_pwm_info[pwm_index].duty + det_duty > 100) {
    new_duty = 100;
  } else {
    new_duty = g_pwm_info[pwm_index].duty + det_duty;
  }

  g_pwm_info[pwm_index].handle.duty = new_duty * 100;
  g_pwm_info[pwm_index].duty = new_duty;
  mcpwm_set_duty(g_pwm_info[pwm_index].handle.pwm_ch_num, g_pwm_info[pwm_index].handle.pwm_timer_num, g_pwm_info[pwm_index].handle.duty);

  return 0;
}

int user_pwm_duty_dec(uint32_t lhchannel, uint8_t det_duty) {
  int pwm_index = _find_pwm_info(lhchannel);
  int new_duty = 0;

  if (pwm_index == -1) {
    LOGE(LOG_TAG, "error:lhchannel %d is not init", lhchannel);
    return -1;
  }

  if (g_pwm_info[pwm_index].duty - det_duty < 0 ||
      g_pwm_info[pwm_index].duty - det_duty > 100) {
    new_duty = 0;
  } else {
    new_duty = g_pwm_info[pwm_index].duty - det_duty;
  }

  g_pwm_info[pwm_index].handle.duty = new_duty * 100;
  g_pwm_info[pwm_index].duty = new_duty;
  mcpwm_set_duty(g_pwm_info[pwm_index].handle.pwm_ch_num, g_pwm_info[pwm_index].handle.pwm_timer_num, g_pwm_info[pwm_index].handle.duty);

  return 0;
}

int user_pwm_disable(uint32_t lhchannel) {
  int pwm_index = _find_pwm_info(lhchannel);
  struct pwm_platform_data *handle = &g_pwm_info[pwm_index].handle;
  uint8_t ch = g_pwm_info[pwm_index].ch;

  mcpwm_close(handle->pwm_ch_num, handle->pwm_timer_num);
  return 0;
}

int user_pwm_enable(uint32_t lhchannel, uint8_t duty) {
  int pwm_index = _find_pwm_info(lhchannel);

  if (pwm_index == -1) {
    LOGE(LOG_TAG, "lhchannel %d is not init", lhchannel);
    return -1;
  }

  if (g_pwm_info[pwm_index].freq > PWM_HZ_MAX ||
      g_pwm_info[pwm_index].freq <= 0) {
    LOGE(LOG_TAG, "error:pwm freq %d is invalid",
           g_pwm_info[pwm_index].freq);
    return -1;
  }

  if (duty > 100) {
    LOGE(LOG_TAG, "error:pwm duty %d is invalid", duty);
    return -1;
  }

  if (1 == g_pwm_info[pwm_index].inverse) {
    duty = (100 - duty);
  }

  g_pwm_info[pwm_index].handle.duty = duty * 100;
  g_pwm_info[pwm_index].duty = duty;
  mcpwm_init(&g_pwm_info[pwm_index].handle);

  return 0;
}

int user_pwm3_start(u32 freq, u32 duty)
{
  return timer_pwm_init(UNI_TIMER3, freq, duty, IO_PORTB_05, 0);
}

void user_pwm3_set_duty(u32 duty)
{
  set_timer_pwm_duty(UNI_TIMER3, duty);
}

void user_pwm3_stop(void)
{
  u32 con = UNI_TIMER3->CON;
  con &= 0xFFFFFFFC;
  UNI_TIMER3->CON = con;
}

/****************** UART interface ******************/
static u8 uart_cbuf[GPIO_UART_MAX][32] __attribute__((aligned(4)));

static int _get_uart_port_num(UNI_PIN_NUM_E user_pin) {
  if (user_pin == IO_PORT_DP || user_pin == IO_PORTA_00) {
    return 1;
  } else if (user_pin == IO_PORTB_09) {
    return 2;
  }
  return -1;
}

int user_uart_init(UNI_PIN_NUM_E tx_pin, UNI_PIN_NUM_E rx_pin, uint32_t baudrate,
		   int data_bits, int stop_bits, int parity) {
  int port = _get_uart_port_num(tx_pin);

  if (port < 0 || port >= GPIO_UART_MAX) {
    LOGE(LOG_TAG, "pin %d uart port %d invalid", tx_pin, port);
    return -1;
  }

  struct uart_platform_data_t u_arg = {0};
  u_arg.tx_pin = tx_pin;
  u_arg.rx_pin = rx_pin;
  u_arg.rx_cbuf = uart_cbuf[port];
  u_arg.rx_cbuf_size = 32;
  u_arg.frame_length = 6;
  u_arg.rx_timeout = 100;
  u_arg.isr_cbfun = NULL;
  u_arg.baud = baudrate;
  u_arg.is_9bit = 0;

  g_uart_handler[port] = uart_dev_open(&u_arg);
  if (g_uart_handler[port] != NULL) {
    gpio_set_hd(tx_pin, 1);
    gpio_set_hd0(tx_pin, 1);
  } else {
    LOGE(LOG_TAG, "uart_dev_open port %d failed, pin %d", port, tx_pin);
    return -1;
  }

  return 0;
};

int user_uart_send(int port, uint8_t *data, uint32_t size) {
  if (g_uart_handler[port] == NULL) {
    return -1;
  }
  g_uart_handler[port]->write(data, size);
  return 0;
}

uint32_t user_uart_recv(int port, uint8_t *data, uint32_t size, uint32_t timeout)
{
  if (g_uart_handler[port] == NULL) {
    return 0;
  }
  return g_uart_handler[port]->read(data, size, timeout);
}

int user_uart_recv_byte(int port, uint8_t *data, uint32_t timeout)
{
  if (g_uart_handler[port] == NULL) {
    return -1;
  }
  g_uart_handler[port]->getbyte(data, timeout);
  return 0;
}

/****************** GPIO pulse interface ******************/
static const u32 timer_div[] = {
    /*0000*/    1,
    /*0001*/    4,
    /*0010*/    16,
    /*0011*/    64,
    /*0100*/    2,
    /*0101*/    8,
    /*0110*/    32,
    /*0111*/    128,
    /*1000*/    256,
    /*1001*/    4 * 256,
    /*1010*/    16 * 256,
    /*1011*/    64 * 256,
    /*1100*/    2 * 256,
    /*1101*/    8 * 256,
    /*1110*/    32 * 256,
    /*1111*/    128 * 256,
};

#define PULSE_TIMER_CLK         clk_get("timer")
#define MAX_TIME_CNT            0x7fff
#define MIN_TIME_CNT            0x100

___interrupt
static void _usr_gpio_pulse_timer0_isr()
{
  int idx = 0;

  g_gpio_pulse[idx].timer_handler->CON |= BIT(14);
  if (g_gpio_pulse[idx].is_top) {
    user_gpio_set_level(g_gpio_pulse[idx].port, !g_gpio_pulse[idx].def_val);
    g_gpio_pulse[idx].is_top = 0;
  } else {
    user_gpio_set_level(g_gpio_pulse[idx].port, g_gpio_pulse[idx].def_val);
    g_gpio_pulse[idx].is_top = 1;
    g_gpio_pulse[idx].times--;
    if (g_gpio_pulse[idx].times <= 0) {
      g_gpio_pulse[idx].timer_handler->CON = BIT(14);
      g_gpio_pulse[idx].used = 0;
    }
  }
  return;
}

___interrupt
static void _usr_gpio_pulse_timer1_isr()
{
  int idx = 1;
  if (g_gpio_pulse[idx].is_top) {
    user_gpio_set_level(g_gpio_pulse[idx].port, !g_gpio_pulse[idx].def_val);
    g_gpio_pulse[idx].is_top = 0;
  } else {
    user_gpio_set_level(g_gpio_pulse[idx].port, g_gpio_pulse[idx].def_val);
    g_gpio_pulse[idx].is_top = 1;
    g_gpio_pulse[idx].times--;
    if (g_gpio_pulse[idx].times <= 0) {
      g_gpio_pulse[idx].timer_handler->CON = BIT(14);
      g_gpio_pulse[idx].used = 0;
    }
  }
  return;
}

___interrupt
static void _usr_gpio_pulse_timer2_isr()
{
  int idx = 2;
  if (g_gpio_pulse[idx].is_top) {
    user_gpio_set_level(g_gpio_pulse[idx].port, !g_gpio_pulse[idx].def_val);
    g_gpio_pulse[idx].is_top = 0;
  } else {
    user_gpio_set_level(g_gpio_pulse[idx].port, g_gpio_pulse[idx].def_val);
    g_gpio_pulse[idx].is_top = 1;
    g_gpio_pulse[idx].times--;
    if (g_gpio_pulse[idx].times <= 0) {
      g_gpio_pulse[idx].timer_handler->CON = BIT(14);
      g_gpio_pulse[idx].used = 0;
    }
  }
  return;
}

___interrupt
static void _usr_gpio_pulse_timer3_isr()
{
  int idx = 3;
  if (g_gpio_pulse[idx].is_top) {
    user_gpio_set_level(g_gpio_pulse[idx].port, !g_gpio_pulse[idx].def_val);
    g_gpio_pulse[idx].is_top = 0;
  } else {
    user_gpio_set_level(g_gpio_pulse[idx].port, g_gpio_pulse[idx].def_val);
    g_gpio_pulse[idx].is_top = 1;
    g_gpio_pulse[idx].times--;
    if (g_gpio_pulse[idx].times <= 0) {
      g_gpio_pulse[idx].timer_handler->CON = BIT(14);
      g_gpio_pulse[idx].used = 0;
    }
  }
  return;
}

static uint8_t _get_gpio_pulse_irq(int idx) {
  uint8_t irq = 0xff;
  switch (idx) {
    case 0:
      irq = IRQ_TIME0_IDX;
      break;
    case 1:
      irq = IRQ_TIME1_IDX;
      break;
    case 2:
      irq = IRQ_TIME2_IDX;
      break;
    case 3:
      irq = IRQ_TIME3_IDX;
      break;
  }
  return irq;
}

static UNI_TIMER_TypeDef* _get_gpio_pulse_timer(int idx) {
  UNI_TIMER_TypeDef *idtimer = NULL;
  switch (idx) {
    case 0:
      idtimer = UNI_TIMER0;
      break;
    case 1:
      idtimer = UNI_TIMER1;
      break;
    case 2:
      idtimer = UNI_TIMER2;
      break;
    case 3:
      idtimer = UNI_TIMER3;
      break;
  }
  return idtimer;
}

static void _request_gpio_pulse_irq(int idx) {
  switch (idx) {
    case 0:
      request_irq(_get_gpio_pulse_irq(idx), 3, _usr_gpio_pulse_timer0_isr, 0);
      break;
    case 1:
      request_irq(_get_gpio_pulse_irq(idx), 3, _usr_gpio_pulse_timer1_isr, 0);
      break;
    case 2:
      request_irq(_get_gpio_pulse_irq(idx), 3, _usr_gpio_pulse_timer2_isr, 0);
      break;
    case 3:
      request_irq(_get_gpio_pulse_irq(idx), 3, _usr_gpio_pulse_timer3_isr, 0);
      break;
  }
}

int user_sw_timer_gpio_pulse(UNI_PIN_NUM_E user_pin, int period_ms, int times, uint8_t dev_val) {
  int idx = -1;
  int i;
  uint32_t prd_cnt;
  uint8_t index = 0xff;

  for (i = 0; i < GPIO_PULSE_MAX; ++i) {
    if (g_gpio_pulse[i].used == 0) {
      idx = i;
      break;
    }
  }

  if (idx == -1) {
    LOGE(LOG_TAG, "no more timer for gpio pulse for port %d", user_pin);
    return -1;
  }

  for (i = 0; i < (sizeof(timer_div) / sizeof(timer_div[0])); i++) {
    prd_cnt = period_ms / 2 * (PULSE_TIMER_CLK / 1000) / timer_div[i];
    if (prd_cnt > MIN_TIME_CNT && prd_cnt < MAX_TIME_CNT) {
      index = i;
      break;
    }
  }

  if (index == 0xff) {
    LOGE(LOG_TAG, "timer div calc failed for gpio pulse for port:%d, period_ms:%d", user_pin, period_ms);
    return -1;
  }

  g_gpio_pulse[idx].timer_handler = _get_gpio_pulse_timer(idx);
  g_gpio_pulse[idx].timer_handler->CNT = 0;
  g_gpio_pulse[idx].timer_handler->PRD = prd_cnt;
  _request_gpio_pulse_irq(idx);
  g_gpio_pulse[idx].timer_id = idx;
  g_gpio_pulse[idx].port = user_pin;
  g_gpio_pulse[idx].times = times;
  g_gpio_pulse[idx].def_val = dev_val;
  g_gpio_pulse[idx].period_ms = period_ms;

  user_gpio_set_level(user_pin, dev_val);
  g_gpio_pulse[idx].is_top = 1;
  g_gpio_pulse[idx].timer_handler->CON = (index << 4) | BIT(0) | BIT(3);

  LOGD(LOG_TAG, "timer[%d] start, prd:%d, index:%d", idx, prd_cnt, index);
  return 0;
}
