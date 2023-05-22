/*
 */

#include <stdlib.h>
#include <string.h>
#include <aos/aos.h>
#include <cJSON.h>
#include "iot_dispatcher.h"

#define TAG "iotdispatch"

typedef struct {
  int valid;
  iot_val_type_e type;
  iot_value_t val;
} iot_value_v_t;

static iotdispatcher_config_t *iotdispatcher_configs;
static int iotdispatcher_configs_len;
static iotdispatcher_cb_t iotdispatcher_cb;
static aos_mutex_t iotdispatcher_lock;
static iot_value_v_t iot_val_res;

static void iotdispatcher_cb_wrapper(iotdispatcher_config_t *config,
                                     iot_value_t val) {
  if (iotdispatcher_cb) {
    aos_mutex_lock(&iotdispatcher_lock, AOS_WAIT_FOREVER);

    iot_val_res.type = config->val_type;
    iot_val_res.val = val;
    iot_val_res.valid = 1;

    iotdispatcher_cb(config);

    iot_val_res.valid = 0;

    aos_mutex_unlock(&iotdispatcher_lock);
  }
}

int iotdispatcher_init(iotdispatcher_config_t *configs, int configs_len,
                       iotdispatcher_cb_t cb) {
  aos_check_param(configs && cb && configs_len >= 0);

  aos_mutex_new(&iotdispatcher_lock);

  iotdispatcher_configs = configs;
  iotdispatcher_configs_len = configs_len;
  iotdispatcher_cb = cb;
  return 0;
}

bool iotdispatcher_result_is_num() {
  return iot_val_res.type == IOT_TYPE_VALUE ||
         iot_val_res.type == IOT_TYPE_DELTA;
}

bool iotdispatcher_result_is_string() {
  return iot_val_res.type == IOT_TYPE_STRING;
}

bool iotdispatcher_result_is_object() {
  return iot_val_res.type == IOT_TYPE_OBJECT;
}

int iotdispatcher_get_float_result(double *val) {
  aos_check_param(val);

  if (iot_val_res.valid && (iot_val_res.type == IOT_TYPE_VALUE ||
                            iot_val_res.type == IOT_TYPE_DELTA)) {
    if (iot_val_res.type == IOT_TYPE_DELTA) {
      *val = *val + iot_val_res.val.num;
    } else {
      *val = iot_val_res.val.num;
    }
    return 0;
  }

  return -1;
}

int iotdispatcher_get_int_result(int *val) {
  aos_check_param(val);

  double dval = *val;
  int ret;

  ret = iotdispatcher_get_float_result(&dval);
  if (ret == 0) {
    *val = (int)dval;
  }

  return ret;
}

int iotdispatcher_get_string_result(const char **str) {
  if (iot_val_res.valid && iot_val_res.type == IOT_TYPE_STRING) {
    *str = iot_val_res.val.str;
    return 0;
  }

  return -1;
}

int iotdispatcher_get_object_result(cJSON **obj) {
  if (iot_val_res.valid && iot_val_res.type == IOT_TYPE_OBJECT) {
    *obj = iot_val_res.val.obj;
    return 0;
  }

  return -1;
}

/* called in smartliving callback */
int iotdispatcher_smartliving_set(char *js) {
  aos_check_param(js);

  if (!iotdispatcher_configs) {
    LOGE(TAG, "call iotdispatcher_init first");
    return -1;
  }

  int ret = -1;
  cJSON *root = cJSON_Parse(js);
  CHECK_RET_TAG_WITH_GOTO(root, END);

  for (int i = 0; i < iotdispatcher_configs_len; i++) {
    if (iotdispatcher_configs[i].src_type == IOT_SRC_SMARTLIVING) {
      cJSON *item =
        cJSON_GetObjectItemByPath(root, iotdispatcher_configs[i].match_str);
      if (iotdispatcher_configs[i].val_type == IOT_TYPE_STRING) {
        if (item && (cJSON_IsString(item) || cJSON_IsObject(item))) {
          char *objstr = NULL;
          if (cJSON_IsObject(item)) {
            objstr = cJSON_PrintUnformatted(item);
          } else {
            objstr = item->valuestring;
          }
          iotdispatcher_cb_wrapper(&iotdispatcher_configs[i],
                                   (iot_value_t)(char *)objstr);
          if (cJSON_IsObject(item)) free(objstr);

          ret = 0;
          break;
        }
      } else if (iotdispatcher_configs[i].val_type == IOT_TYPE_OBJECT) {
        if (item && cJSON_IsObject(item)) {
          iotdispatcher_cb_wrapper(&iotdispatcher_configs[i],
                                   (iot_value_t)item);
          ret = 0;
          break;
        }
      } else {
        if (item && (cJSON_IsNumber(item) || cJSON_IsBool(item))) {
          double val = cJSON_IsTrue(item)    ? 1
                       : cJSON_IsFalse(item) ? 0
                                             : item->valuedouble;
          iotdispatcher_cb_wrapper(&iotdispatcher_configs[i], (iot_value_t)val);
          ret = 0;
          break;
        }
      }
    }
  }

END:
  cJSON_Delete(root);
  return ret;
}

/* called in voice recognition callback */
int iotdispatcher_voice_set(char *kw_str) {
  aos_check_param(kw_str);

  aos_mutex_lock(&iotdispatcher_lock, AOS_WAIT_FOREVER);
  char property_payload[256] = {0};
  char property[32] = {0};
  char val[32] = {0};
  char *p = strstr(kw_str, "#");
  if (p == NULL) {
    aos_mutex_unlock(&iotdispatcher_lock);
    return -1;
  }
  strncpy(property, kw_str, p - kw_str);
  property[p - kw_str] = '\0';
  p = strstr(p + 1, "#") + 1;
  if (p == NULL) {
    aos_mutex_unlock(&iotdispatcher_lock);
    return -1;
  }
  strcpy(val, p);
  snprintf(property_payload, sizeof(property_payload), "{\"%s\":%d}", property,
           atoi(val));
  user_post_given_property(property_payload, sizeof(property_payload));

  aos_mutex_unlock(&iotdispatcher_lock);

  return 0;
}
