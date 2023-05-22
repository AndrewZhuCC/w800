/*
 */

#include <stdlib.h>
#include <string.h>
#include <aos/aos.h>
#include <cJSON.h>
#include "app_main.h"

#define TAG "appiot"

enum iot_property {
  IOT_PROP_UNI_AUTO = 0,
};

const static iotdispatcher_config_t iot_configs[] = {
  {0, IOT_PROP_UNI_AUTO, IOT_SRC_SMARTLIVING, IOT_TYPE_VALUE, "brightness"},
  {0, IOT_PROP_UNI_AUTO, IOT_SRC_SMARTLIVING, IOT_TYPE_VALUE, "LightMode"},
  {0, IOT_PROP_UNI_AUTO, IOT_SRC_SMARTLIVING, IOT_TYPE_VALUE, "WIFI_Band"},
  {0, IOT_PROP_UNI_AUTO, IOT_SRC_SMARTLIVING, IOT_TYPE_VALUE, "Network"},
  {0, IOT_PROP_UNI_AUTO, IOT_SRC_SMARTLIVING, IOT_TYPE_VALUE, "colorTemperatureInKelvin"},
  {0, IOT_PROP_UNI_AUTO, IOT_SRC_SMARTLIVING, IOT_TYPE_VALUE, "Heartbeat"},
  {0, IOT_PROP_UNI_AUTO, IOT_SRC_SMARTLIVING, IOT_TYPE_VALUE, "LightScene"},
  {0, IOT_PROP_UNI_AUTO, IOT_SRC_SMARTLIVING, IOT_TYPE_VALUE, "LightType"},
  {0, IOT_PROP_UNI_AUTO, IOT_SRC_SMARTLIVING, IOT_TYPE_VALUE, "HSVColor"},
  {0, IOT_PROP_UNI_AUTO, IOT_SRC_SMARTLIVING, IOT_TYPE_VALUE, "mode"},
  {0, IOT_PROP_UNI_AUTO, IOT_SRC_SMARTLIVING, IOT_TYPE_VALUE, "color"},
  {0, IOT_PROP_UNI_AUTO, IOT_SRC_SMARTLIVING, IOT_TYPE_VALUE, "WIFI_AP_BSSID"},
  {0, IOT_PROP_UNI_AUTO, IOT_SRC_SMARTLIVING, IOT_TYPE_VALUE, "CloudSequence"},
  {0, IOT_PROP_UNI_AUTO, IOT_SRC_SMARTLIVING, IOT_TYPE_VALUE, "ScenesColor"},
  {0, IOT_PROP_UNI_AUTO, IOT_SRC_SMARTLIVING, IOT_TYPE_VALUE, "powerstate"},
};

static void iot_user_cb(iotdispatcher_config_t *config) {
  LOGD(TAG, "iot user cb %d %d", config->src_type, config->property);
  //mvoice_process_pause();

  switch (config->property) {
    case IOT_PROP_UNI_AUTO: {
      int val;
      if (iotdispatcher_get_int_result(&val) == 0) {
        uint8_t need_reply = 1;
        uint8_t need_post = 1;
        char action[32] = {0};
        snprintf(action, sizeof(action), "%s#val#%d", config->match_str, val);
        user_gpio_handle_action_event(action, &need_reply, &need_post);
        if (need_post) {
          char property_payload[256] = {0};
          snprintf(property_payload, sizeof(property_payload), "{\"%s\":%d}",
                   config->match_str, val);
          user_post_given_property(property_payload, sizeof(property_payload));
        }
      }
      break;
    }

      //        case IOT_PROP_WIFI_PROV: {
      //            LOGD(TAG, "start ble wifi provisioning");
      //            extern int wifi_prov_method;
      //            wifi_prov_method = WIFI_PROVISION_SL_BLE;
      //            aos_kv_setint("wprov_method", wifi_prov_method);
      //
      //            aos_kv_del("AUTH_AC_AS");
      //            aos_kv_del("AUTH_KEY_PAIRS");
      //            app_sys_set_boot_reason(BOOT_REASON_WIFI_CONFIG);
      //            aos_reboot();
      //            break;
      //        }

    default:
      break;
  }

  //mvoice_process_resume();
}

int app_iot_init() {
  return iotdispatcher_init(
    iot_configs, sizeof(iot_configs) / sizeof(iotdispatcher_config_t),
    iot_user_cb);
}
