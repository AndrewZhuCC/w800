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
#include <aos/aos.h>
#include <devices/uart.h>
#include "drv/timer.h"
#include "uni_timer.h"
#include "user_gpio.h"
#include "uni_iot.h"

#define LOG_TAG "[user_gpio]"

static pwm_info_t g_pwm_info[GPIO_PWM_MAX] = {0};
static aos_dev_t *g_uart_handler[GPIO_UART_MAX] = {NULL, NULL, NULL, NULL, NULL};
static gpio_pulse_t *g_gpio_pulse[GPIO_PULSE_MAX] = {0};

void user_pin_set_func(pin_name_e user_pin, pin_func_e user_func) {
  drv_pinmux_config(user_pin, user_func);
}

int user_gpio_set_direction(pin_name_e user_pin, gpio_direction_e direction) {
  gpio_pin_handle_t     handle;
  handle = csi_gpio_pin_initialize(user_pin, NULL);

  if (handle) {
      csi_gpio_pin_config_direction(handle, direction);
      csi_gpio_pin_uninitialize(handle);
  } else {
      return -1;
  }

  return 0;
}

int user_gpio_set_level(pin_name_e pin, UNI_GPIO_LEVEL level) {
  gpio_pin_handle_t     handle;
  handle = csi_gpio_pin_initialize(pin, NULL);

  if (handle) {
      csi_gpio_pin_write(handle, level);
      csi_gpio_pin_uninitialize(handle);
  } else {
      return -1;
  }

  return 0;
}

int user_pwm_init(pin_name_e user_pin, uint32_t freq, uint8_t inverse,
                  uint8_t duty) {
  int i;
  int pwm_index = -1;

  for (i = 0; i < GPIO_PWM_MAX; ++i) {
    if (g_pwm_info[i].used == 0) {
      pwm_index = i;
      break;
    }
  }
  if (pwm_index == -1) {
    LOGE(LOG_TAG, "error:no more pwm channel can init");
    return -1;
  }

  g_pwm_info[pwm_index].handle = csi_pwm_initialize(pwm_index);
  g_pwm_info[pwm_index].ch = pwm_index;
  g_pwm_info[pwm_index].used = 1;
  g_pwm_info[pwm_index].mapped_gpio_port = user_pin;
  g_pwm_info[pwm_index].freq = freq;
  g_pwm_info[pwm_index].inverse = inverse;
  g_pwm_info[pwm_index].duty = duty;
  return 0;
}

int _find_pwm_info(uint32_t pwm_port) {
  int i;
  for (i = 0; i < GPIO_PWM_MAX; ++i) {
    if (g_pwm_info[i].used == 1 && g_pwm_info[i].mapped_gpio_port == pwm_port) {
      return i;
    }
  }
  return -1;
}

int user_pwm_duty_inc(uint32_t pwm_port, uint8_t det_duty) {
  int pwm_index = _find_pwm_info(pwm_port);
  int new_duty = 0;
  int ret = 0;

  if (pwm_index == -1) {
    LOGE(LOG_TAG, "error:pwm_port %d is not init", pwm_port);
    return -1;
  }

  if (g_pwm_info[pwm_index].duty + det_duty > 100) {
    new_duty = 100;
  } else {
    new_duty = g_pwm_info[pwm_index].duty + det_duty;
  }

  return user_pwm_enable(pwm_port, new_duty);
}

int user_pwm_duty_dec(uint32_t pwm_port, uint8_t det_duty) {
  int pwm_index = _find_pwm_info(pwm_port);
  int new_duty = 0;

  if (pwm_index == -1) {
    LOGE(LOG_TAG, "error:pwm_port %d is not init", pwm_port);
    return -1;
  }

  if (g_pwm_info[pwm_index].duty - det_duty < 0 ||
      g_pwm_info[pwm_index].duty - det_duty > 100) {
    new_duty = 0;
  } else {
    new_duty = g_pwm_info[pwm_index].duty - det_duty;
  }

  return user_pwm_enable(pwm_port, new_duty);
}

int user_pwm_disable(uint32_t pwm_port) {
  int pwm_index = _find_pwm_info(pwm_port);
  pwm_handle_t handle = g_pwm_info[pwm_index].handle;
  uint8_t ch = g_pwm_info[pwm_index].ch;

  csi_pwm_stop(handle, ch);
  return 0;
}

int user_pwm_enable(uint32_t pwm_port, uint8_t duty) {
  int ret = 0;
  int pwm_index = _find_pwm_info(pwm_port);

  if (pwm_index == -1) {
    LOGE(LOG_TAG, "error:pwm_port %d is not init", pwm_port);
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
  uint32_t period = 1000000 / g_pwm_info[pwm_index].freq;
  uint32_t pulse_width_us = period * duty / 100;
  uint8_t ch = g_pwm_info[pwm_index].ch;
  pwm_handle_t handle = g_pwm_info[pwm_index].handle;

  ret = csi_pwm_config(handle, ch, period, pulse_width_us);
  if (ret != 0) {
    LOGE(LOG_TAG, "csi_pwm_config failed ch %d, period %d, pulse_width_us %d", ch, period, pulse_width_us);
    return -1;
  }
  csi_pwm_start(handle, ch);

  return ret;
}

static int _get_uart_port_num(pin_name_e user_pin) {
  switch (user_pin) {
    case PB19:
      return 0;
    case PB6:
      return 1;
    case PB2:
      return 2;
    case PB0:
      return 3;
    case PB4:
      return 4;
  }
  return -1;
}

static hal_uart_data_width_t _get_uart_data_bits(int data_bits) {
  switch (data_bits) {
    case 5:
      return DATA_WIDTH_5BIT;
    case 6:
      return DATA_WIDTH_6BIT;
    case 7:
      return DATA_WIDTH_7BIT;
    case 8:
      return DATA_WIDTH_8BIT;
    case 9:
      return DATA_WIDTH_9BIT;
    default:
      LOGW(LOG_TAG, "data_bits %d invalid", data_bits);
      return DATA_WIDTH_8BIT;
  }
}

static hal_uart_parity_t _get_uart_parity(int parity) {
  switch (parity) {
    case 0:
      return PARITY_NONE;
    case 1:
      return PARITY_EVEN;
    case 2:
      return PARITY_ODD;
    default:
      LOGW(LOG_TAG, "parity %d invalid", parity);
      return PARITY_NONE;
  }
}

static hal_uart_stop_bits_t _get_uart_stop_bits(int stop_bits) {
  switch (stop_bits) {
    case 1:
      return STOP_BITS_1;
    case 2:
      return STOP_BITS_2;
    default:
      LOGW(LOG_TAG, "stop_bits %d invalid", stop_bits);
      return STOP_BITS_1;
  }
}

int user_uart_init(pin_name_e user_pin, uint32_t baudrate, int data_bits,
                   int stop_bits, int parity) {
  int port = _get_uart_port_num(user_pin);

  if (port < 0 || port > 4) {
    LOGE(LOG_TAG, "error:uart port %d invalid", port);
    return -1;
  }
  uart_csky_register(port);

  g_uart_handler[port] = uart_open_id("uart", port);
  if (g_uart_handler[port] == NULL) {
    LOGE(LOG_TAG, "uart open failed");
    return -1;
  }

  uart_set_buffer_size(g_uart_handler[port], 1024*2);
  uart_config_t config;
  uart_config_default(&config);
  config.baud_rate = baudrate;
  config.data_width = _get_uart_data_bits(data_bits);
  config.parity = _get_uart_parity(parity);
  config.stop_bits = _get_uart_stop_bits(stop_bits);
  uart_config(g_uart_handler[port], &config);
  return 0;
};

int user_uart_send(int port, uint8_t *data, uint32_t size) {
  if (g_uart_handler[port] == NULL) {
    return -1;
  }
  return uart_send(g_uart_handler[port], data, size);
}

int user_uart_recv(int port, uint8_t *buf, uint32_t size) {
  if (g_uart_handler[port] == NULL) {
    return -1;
  }
  return uart_recv(g_uart_handler[port], buf, size, 5000);
}

static void _usr_gpio_pulse_callback(int32_t idx, timer_event_e event)
{
	if (idx >= GPIO_PULSE_MAX) {
    LOGE(LOG_TAG, "error:invalid timer id [%d]", idx);
		return;
	}
  if (g_gpio_pulse[idx]->is_top) {
    user_gpio_set_level(g_gpio_pulse[idx]->port, !g_gpio_pulse[idx]->def_val);
    g_gpio_pulse[idx]->is_top = 0;
  } else {
    user_gpio_set_level(g_gpio_pulse[idx]->port, g_gpio_pulse[idx]->def_val);
    g_gpio_pulse[idx]->is_top = 1;
    g_gpio_pulse[idx]->times--;
    if (g_gpio_pulse[idx]->times <= 0) {
			csi_timer_stop(g_gpio_pulse[idx]->timer_handler);
			csi_timer_uninitialize(g_gpio_pulse[idx]->timer_handler);
      uni_free(g_gpio_pulse[idx]);
			g_gpio_pulse[idx] = NULL;
    }
  }
	return;
}

int user_sw_timer_gpio_pulse(int port, int period_ms, int times,
                             uint8_t dev_val) {

	int idx = 0;
	timer_handle_t timer_handler;
	for (idx = 0; idx < GPIO_PULSE_MAX; idx ++) {
		timer_handler = csi_timer_initialize(idx, _usr_gpio_pulse_callback);
		if(timer_handler) {
			csi_timer_config(timer_handler, TIMER_MODE_RELOAD);
			csi_timer_set_timeout(timer_handler, period_ms * 500);
			break;
		}
	}
	if (idx < GPIO_PULSE_MAX) {
		if (NULL != g_gpio_pulse[idx]) {
			LOGE(LOG_TAG, "error:timer[%d] already exist", idx);
			return -1;
		}
		g_gpio_pulse[idx] = uni_malloc(sizeof(gpio_pulse_t));
		memset(g_gpio_pulse[idx], 0, sizeof(gpio_pulse_t));
		g_gpio_pulse[idx]->timer_handler = timer_handler;
		g_gpio_pulse[idx]->timer_id = idx;
		g_gpio_pulse[idx]->port = port;
		g_gpio_pulse[idx]->times = times;
		g_gpio_pulse[idx]->def_val = dev_val;
		g_gpio_pulse[idx]->period_ms = period_ms;

		user_gpio_set_level(port, dev_val);
		g_gpio_pulse[idx]->is_top = 1;

		csi_timer_start(timer_handler);
    LOGD(LOG_TAG, "debug:timer[%d] start", idx);
	} else {
    LOGE(LOG_TAG, "error:%s timer register failed", __func__);
		return -1;
	}
	return 0;
}
