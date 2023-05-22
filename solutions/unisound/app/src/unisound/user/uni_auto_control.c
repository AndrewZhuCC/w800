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

#include "msg_process_center.h"
#include <aos/kernel.h>
#include "app_sys.h"
#include "app_main.h"
#include <smartliving/exports/iot_export_linkkit.h>

#include "uni_gpio.h"

#define LOG_TAG "[uni_auto_ctrl]"

extern int wifi_prov_method;

static void _user_gpio_set_pinmux(void) {

  user_pin_set_func(PA1, PA1_IIC_SCL);
  user_pin_set_func(PA4, PA4_IIC_SDA);

  user_pin_set_func(PA0, PIN_FUNC_GPIO);
  user_pin_set_func(PB0, PB0_PWM);
  user_pin_set_func(PB1, PB1_PWM);
  user_pin_set_func(PB2, PB2_PWM);
  user_pin_set_func(PB4, PB4_UART4_TX);
  user_pin_set_func(PB5, PB5_UART4_RX);
  user_pin_set_func(PB6, PB6_UART1_TX);
  user_pin_set_func(PB7, PB7_UART1_RX);
}

int user_gpio_init(void) {
  _user_gpio_set_pinmux();

  tls_i2c_init(5000);

  user_gpio_set_direction(PA0, GPIO_DIRECTION_OUTPUT);
  user_gpio_set_level(PA0, UNI_GPIO_LEVEL_LOW);
  user_pwm_init(PB0, 1000, 0, 100);
  user_pwm_init(PB1, 1000, 0, 100);
  user_pwm_init(PB2, 1000, 0, 100);

  user_uart_init(PB6, 115200, 8, 1, 0);

  LOGI(LOG_TAG, " %s success", __func__);
  return 0;
}

// extern led_strip_msg_t msg;

void uni_iot_set_brightness(int val) {
  led_strip_msg_t msg;
  switch (val) {
    case 0: {
      msg.hsvcolor[0].v = 0;
      break;
    }
    case 10: {
      msg.hsvcolor[0].v = 10;
      break;
    }
    case 20: {
      msg.hsvcolor[0].v = 20;
      break;
    }
    case 30: {
      msg.hsvcolor[0].v = 30;
      break;
    }
    case 40: {
      msg.hsvcolor[0].v = 40;
      break;
    }
    case 50: {
      msg.hsvcolor[0].v = 50;
      break;
    }
    case 60: {
      msg.hsvcolor[0].v = 60;
      break;
    }
    case 70: {
      msg.hsvcolor[0].v = 70;
      break;
    }
    case 80: {
      msg.hsvcolor[0].v = 80;
      break;
    }
    case 90: {
      msg.hsvcolor[0].v = 90;
      break;
    }
    case 100: {
      msg.hsvcolor[0].v = 100;
      break;
    }

    default:
      LOGW(LOG_TAG, "%s val %d unknown", __func__, val);
      break;
  }
  // aos_kv_setint(HSV_Value,msg.hsvcolor[0].v);
  msg.validindex = Brightness_Cnt;
  send_msg_to_queue(&msg);
}

void uni_iot_set_LightMode(int val) {
  switch (val) {

    default:
      LOGW(LOG_TAG, "%s val %d unknown", __func__, val);
      break;
  }
}

void uni_iot_set_Network(int val) {
  switch (val) {
    case 1: {
      UniPlayStop();
      mvoice_alg_deinit();

      aos_kv_setint("start_en", 1);

      awss_report_reset(0);
      aos_msleep(500);

      aos_kv_setint("wprov_method", wifi_prov_method);
			if (wifi_prov_method == WIFI_PROVISION_SL_BLE) {
				printf("wifi_prov_method=WIFI_PROVISION_SL_BLE\n");
				aos_kv_del("AUTH_AC_AS");
				aos_kv_del("AUTH_KEY_PAIRS");
        aos_kv_del("wifi_ssid");
        aos_kv_del("wifi_psk");
			}
			app_sys_set_boot_reason(BOOT_REASON_WIFI_CONFIG);//zz
			printf("222222222222222222222222222222\r\n");
      // user_sw_timer_gpio_pulse(PB4, 1, 100, 0);
			aos_reboot(); 
      // break;
    }

    default:
      LOGW(LOG_TAG, "%s val %d unknown", __func__, val);
      break;
  }
}

void uni_iot_set_colorTemperatureInKelvin(int val) {
  switch (val) {

    default:
      LOGW(LOG_TAG, "%s val %d unknown", __func__, val);
      break;
  }
}

void uni_iot_set_Heartbeat(int val) {
  switch (val) {

    default:
      LOGW(LOG_TAG, "%s val %d unknown", __func__, val);
      break;
  }
}

void uni_iot_set_LightScene(int val) {
  switch (val) {

    default:
      LOGW(LOG_TAG, "%s val %d unknown", __func__, val);
      break;
  }
}

void uni_iot_set_LightType(int val) {
  switch (val) {

    default:
      LOGW(LOG_TAG, "%s val %d unknown", __func__, val);
      break;
  }
}

void uni_iot_set_HSVColor(int val) {
  switch (val) {

    default:
      LOGW(LOG_TAG, "%s val %d unknown", __func__, val);
      break;
  }
}

void uni_iot_set_mode(int val) {
  led_strip_msg_t msg;
  switch (val) {
    case 0: {                 
      msg.hsvcolor[0].h = 0;
      msg.hsvcolor[0].s = 0;
      msg.hsvcolor[0].v = 0;
      break;
    }
    case 14: {                 
      msg.hsvcolor[0].h = 37;
      msg.hsvcolor[0].s = 57;
      msg.hsvcolor[0].v = 48;
      break;
    }
    case 4: {                  //影院模式  
      msg.hsvcolor[0].h = 224;
      msg.hsvcolor[0].s = 58;
      msg.hsvcolor[0].v = 25;
      break;
    }
    case 3: {                    
      msg.hsvcolor[0].h = 43;
      msg.hsvcolor[0].s = 21;
      msg.hsvcolor[0].v = 100;
      break;
    }
    case 57: {                   
      msg.hsvcolor[0].h = 120;
      msg.hsvcolor[0].s = 21;
      msg.hsvcolor[0].v = 50;
      break;
    }
    default:
      LOGW(LOG_TAG, "%s val %d unknown", __func__, val);
      break;
  }
  msg.validindex = WK_VALID;
  send_msg_to_queue(&msg);
}

void uni_iot_set_color(int val) {
  led_strip_msg_t msg;
  switch (val) {
    case 0: {
      // user_pwm_enable(PB0, 0);
      break;
    }
    case 1: {
      msg.hsvcolor[0].h = 0;
      msg.hsvcolor[0].s = 100;
      msg.hsvcolor[0].v = 100;
      break;
    }
    case 2: {
      msg.hsvcolor[0].h = 30;
      msg.hsvcolor[0].s = 100;
      msg.hsvcolor[0].v = 100;
      break;
    }
    case 3: {
      msg.hsvcolor[0].h = 60;
      msg.hsvcolor[0].s = 100;
      msg.hsvcolor[0].v = 100;
      break;
    }
    case 4: {
      msg.hsvcolor[0].h = 120;
      msg.hsvcolor[0].s = 100;
      msg.hsvcolor[0].v = 100;
      break;
    }
    case 5: {
      msg.hsvcolor[0].h = 160;
      msg.hsvcolor[0].s = 100;
      msg.hsvcolor[0].v = 100;
      break;
    }
    case 6: {
      msg.hsvcolor[0].h = 240;
      msg.hsvcolor[0].s = 100;
      msg.hsvcolor[0].v = 100;
      break;
    }
    case 7: {
      msg.hsvcolor[0].h = 300;
      msg.hsvcolor[0].s = 100;
      msg.hsvcolor[0].v = 100;
      break;
    }
    case 8: {
      msg.hsvcolor[0].h = 359;
      msg.hsvcolor[0].s = 0;
      msg.hsvcolor[0].v = 100;
      break;
    }

    default:
      LOGW(LOG_TAG, "%s val %d unknown", __func__, val);
      break;
  }
  msg.validindex = HSV_VALID;
  send_msg_to_queue(&msg);
}

void uni_iot_set_WIFI_AP_BSSID(int val) {
  switch (val) {

    default:
      LOGW(LOG_TAG, "%s val %d unknown", __func__, val);
      break;
  }
}

void uni_iot_set_CloudSequence(int val) {
  switch (val) {

    default:
      LOGW(LOG_TAG, "%s val %d unknown", __func__, val);
      break;
  }
}

void uni_iot_set_ScenesColor(int val) {
  switch (val) {

    default:
      LOGW(LOG_TAG, "%s val %d unknown", __func__, val);
      break;
  }
}

void uni_iot_set_powerstate(int val) {
  led_strip_msg_t msg;
  switch (val) {
    case 0: {
      msg.lightswitch = 0;
      break;
    }
    case 1: {
      msg.lightswitch = 1;
      break;
    }

    default:
      LOGW(LOG_TAG, "%s val %d unknown", __func__, val);
      break;
  } 
  msg.validindex = LS_VALID;
  send_msg_to_queue(&msg);
}

int user_gpio_handle_action_event(const char *action,
                               uint8_t *need_reply, uint8_t *need_post) {
  led_strip_msg_t msg;
  if (action != NULL) {
    LOGD(LOG_TAG, "handle kws result action: %s", action);
    if (0 == strcmp(action, "powerstate#val#0")) {
      uni_iot_set_powerstate(0);
    } else if (0 == strcmp(action, "powerstate#val#1")) {
      uni_iot_set_powerstate(1);
    } else if (0 == strcmp(action, "brightness#val#0")) {
      uni_iot_set_brightness(0);
    } else if (0 == strcmp(action, "brightness#val#10")) {
      uni_iot_set_brightness(10);
    } else if (0 == strcmp(action, "brightness#val#20")) {
      uni_iot_set_brightness(20);
    } else if (0 == strcmp(action, "brightness#val#30")) {
      uni_iot_set_brightness(30);
    } else if (0 == strcmp(action, "brightness#val#40")) {
      uni_iot_set_brightness(40);
    } else if (0 == strcmp(action, "brightness#val#50")) {
      uni_iot_set_brightness(50);
    } else if (0 == strcmp(action, "brightness#val#60")) {
      uni_iot_set_brightness(60);
    } else if (0 == strcmp(action, "brightness#val#70")) {
      uni_iot_set_brightness(70);
    } else if (0 == strcmp(action, "brightness#val#80")) {
      uni_iot_set_brightness(80);
    } else if (0 == strcmp(action, "brightness#val#90")) {
      uni_iot_set_brightness(90);
    } else if (0 == strcmp(action, "brightness#val#100")) {
      uni_iot_set_brightness(100);
    } else if (0 == strcmp(action, "mode#val#0")) {
      uni_iot_set_mode(0);
    } else if (0 == strcmp(action, "mode#val#3")) {
      uni_iot_set_mode(3);
    } else if (0 == strcmp(action, "mode#val#4")) {
      uni_iot_set_mode(4);
    } else if (0 == strcmp(action, "mode#val#14")) {
      uni_iot_set_mode(14);
    } else if (0 == strcmp(action, "mode#val#57")) {
      uni_iot_set_mode(57);
    } else if (0 == strcmp(action, "color#val#1")) {
      uni_iot_set_color(1);
    } else if (0 == strcmp(action, "color#val#2")) {
      uni_iot_set_color(2);
    } else if (0 == strcmp(action, "color#val#3")) {
      uni_iot_set_color(3);
    } else if (0 == strcmp(action, "color#val#4")) {
      uni_iot_set_color(4);
    } else if (0 == strcmp(action, "color#val#5")) {
      uni_iot_set_color(5);
    } else if (0 == strcmp(action, "color#val#6")) {
      uni_iot_set_color(6);
    } else if (0 == strcmp(action, "color#val#7")) {
      uni_iot_set_color(7);
    } else if (0 == strcmp(action, "color#val#8")) {
      uni_iot_set_color(8);
    } else if (0 == strcmp(action, "Network#null#0")) {
      uni_iot_set_Network(1);
    } else if (0 == strcmp(action, "wakeup_uni")) {
      user_pwm_enable(PB0, 100);
      user_pwm_enable(PB1, 100);
      user_pwm_enable(PB2, 100);
      aos_msleep(100);
      user_pwm_enable(PB0, 0);
      user_pwm_enable(PB1, 0);
      user_pwm_enable(PB2, 0);
      aos_msleep(100);
      user_pwm_enable(PB0, 100);
      user_pwm_enable(PB1, 100);
      user_pwm_enable(PB2, 100);
      aos_msleep(100);
      user_pwm_enable(PB0, 0);
      user_pwm_enable(PB1, 0);
      user_pwm_enable(PB2, 0);  
      aos_msleep(50);
      msg.validindex = Wake_Up;
      send_msg_to_queue(&msg);
    } else if (0 == strcmp(action, "exitUni")) {
      // uni_iot_set_Network(1);
      user_pwm_enable(PB0, 100);
      user_pwm_enable(PB1, 100);
      user_pwm_enable(PB2, 100);
      aos_msleep(100);
      user_pwm_enable(PB0, 0);
      user_pwm_enable(PB1, 0);
      user_pwm_enable(PB2, 0);
      aos_msleep(100);
      user_pwm_enable(PB0, 100);
      user_pwm_enable(PB1, 100);
      user_pwm_enable(PB2, 100);
      aos_msleep(100);
      user_pwm_enable(PB0, 0);
      user_pwm_enable(PB1, 0);
      user_pwm_enable(PB2, 0);  
      aos_msleep(50);
      msg.validindex = Wake_Up;
      send_msg_to_queue(&msg);
    } else if (NULL != strstr(action, "#val#")) {
      return 0;
    } else {
      return -1;
    }
  } else {
    return -1;
  }

  return 0;
}
