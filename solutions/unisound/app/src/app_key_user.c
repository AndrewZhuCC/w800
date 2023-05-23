/*
 */
#include <yoc/button.h>
#include "app_main.h"
//#include "app_lpm.h"
//#include <aos/hal/adc.h>
#include "app_sys.h"
#include "msg_process_center.h"

#define TAG "keyusr"

extern int wifi_prov_method;
extern color_hsv_t HSV_Color;

typedef enum {
    button_id0 = 0,
} button_id_t;

typedef enum {
    nothing = 1,
    network,
} button_event_t;

void change_color(void)
{
  led_strip_msg_t msg;
  msg.validindex = HSV_VALID;
  HSV_Color.h += 120;
  if (HSV_Color.h >= 360) {
	HSV_Color.h = 0;
  }
  HSV_Color.s = 100;
  HSV_Color.v = 100;
  msg.hsvcolor[0].h = HSV_Color.h;
  msg.hsvcolor[0].s = HSV_Color.s;
  msg.hsvcolor[0].v = HSV_Color.v;
  msg.hsvcolor[0].light_switch = 1;
  send_msg_to_queue(&msg);
}

void button_evt(int event_id, void *priv)
{
    LOGD(TAG, "button(%s)\n", (char *)priv);
    switch (event_id) {
		case network:
			LOGE(TAG, "ble prov");
			aos_kv_setint("wprov_method", wifi_prov_method);
			if (wifi_prov_method == WIFI_PROVISION_SL_BLE) {
				printf("wifi_prov_method=WIFI_PROVISION_SL_BLE\n");
				aos_kv_del("AUTH_AC_AS");
				aos_kv_del("AUTH_KEY_PAIRS");
			}
			app_sys_set_boot_reason(BOOT_REASON_WIFI_CONFIG);//zz
			aos_reboot();
			break;
		case nothing:
			LOGE(TAG, "change color");
			change_color();
			break;
		default:
			break;
    }
}

void app_button_init(void)
{
    button_init();
    button_add_gpio(button_id0, PA0, LOW_LEVEL);//添加gpio按键

    button_evt_t b_tbl[] = {
        {
            .button_id  = button_id0,
            .event_id   = BUTTON_PRESS_UP,//1
            .press_time = 0,
        }
    };
	button_add_event(nothing, b_tbl, sizeof(b_tbl)/sizeof(button_evt_t), button_evt, "nothing");//为当前按键表参数 添加事件
	
	b_tbl[0].button_id = button_id0;
	b_tbl[0].event_id = BUTTON_PRESS_LONG_DOWN;//2
	b_tbl[0].press_time = 5000;
	button_add_event(network, b_tbl, sizeof(b_tbl)/sizeof(button_evt_t), button_evt, "network");//为当前按键表参数 添加事件
}
