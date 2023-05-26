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

#include <drv/pwm.h>
#include <drv/gpio.h>
#include <drv/timer.h>
#include "pinmux.h"

#define PWM_HZ_MAX   (1000 * 1000)  // max 1MHz (recommand below 40KHz on HB-L)
#define GPIO_PWM_MAX 3
#define GPIO_UART_MAX 5
#define GPIO_PULSE_MAX 3						// use timer 0,1,2

typedef struct {
  uint8_t used;
  uint32_t mapped_gpio_port;
  uint32_t freq;
  uint8_t duty;
  uint8_t inverse;
  pwm_handle_t handle;
  uint8_t      ch;
} pwm_info_t;

typedef struct {
  timer_handle_t timer_handler;
  int timer_id;
  int port;
  int period_ms;
  int times;
  int def_val;
  uint8_t is_top;
} gpio_pulse_t;

/**
 * @brief gpio 电平状态
 */
typedef enum {
  UNI_GPIO_LEVEL_LOW = 0, ///< 低电平
  UNI_GPIO_LEVEL_HIGH     ///< 高电平
} UNI_GPIO_LEVEL;

typedef enum {
  USER_PIN_NUM_1 = PB20,
  USER_PIN_NUM_2 = PB19,
  USER_PIN_NUM_13 = PA0,
  USER_PIN_NUM_14 = PA1,
  USER_PIN_NUM_15 = PA4,
  USER_PIN_NUM_16 = PA7,
  USER_PIN_NUM_18 = PB0,
  USER_PIN_NUM_19 = PB1,
  USER_PIN_NUM_20 = PB2,
  USER_PIN_NUM_21 = PB3,
  USER_PIN_NUM_22 = PB4,
  USER_PIN_NUM_23 = PB5,
  USER_PIN_NUM_26 = PB6,
  USER_PIN_NUM_27 = PB7,
  USER_PIN_NUM_28 = PB8,
  USER_PIN_NUM_29 = PB9,
  USER_PIN_NUM_30 = PB10,
  USER_PIN_NUM_32 = PB11,
} UserPinNum;

void user_pin_set_func(pin_name_e user_pin, pin_func_e user_func);
int user_gpio_set_direction(pin_name_e user_pin, gpio_direction_e direction);
int user_gpio_set_level(pin_name_e user_pin, UNI_GPIO_LEVEL level);
int user_pwm_init(pin_name_e user_pin, uint32_t freq, uint8_t inverse,
                  uint8_t duty);
int user_pwm_duty_inc(uint32_t pwm_port, uint8_t det_duty);
int user_pwm_duty_dec(uint32_t pwm_port, uint8_t det_duty);
int user_pwm_enable(uint32_t pwm_port, uint8_t duty);
int user_pwm_disable(uint32_t pwm_port);
int user_uart_init(pin_name_e user_pin, uint32_t baudrate, int data_bits, int stop_bits,
                   int parity);
int user_uart_send(int port, uint8_t *data, uint32_t size);
int user_uart_recv(int port, uint8_t *buf, uint32_t size);
int user_sw_timer_gpio_pulse(int port, int period_ms, int times,
                             uint8_t dev_val);

#endif
