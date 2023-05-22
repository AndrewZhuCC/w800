/*
 */

#ifndef _IOT_DISPATCHER_H_
#define _IOT_DISPATCHER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <cJSON.h>

typedef enum {
    IOT_SRC_SMARTLIVING = 0,
} iot_src_e;

typedef enum {
    IOT_TYPE_VALUE = 0,
    IOT_TYPE_DELTA,
    IOT_TYPE_STRING,
    IOT_TYPE_OBJECT,
} iot_val_type_e;

typedef union {
    double    num;            // setting val, only valid when src_type is IOT_SRC_VOICE
    char      *str;
    cJSON     *obj;  
} iot_value_t;

typedef struct {
    int             device_id;      // reserved for iot router
    int             property;       // user defined property tag
    iot_src_e       src_type;       // event source
    iot_val_type_e  val_type;
    const char      *match_str;     // json key for IOT_SRC_SMARTLIVING; asr key word for IOT_SRC_VOICE
    iot_value_t     val;
} iotdispatcher_config_t;

/**
 * user callback
 *
 * @param[in]  config one of iotdispatcher_init configs
 */
typedef void (*iotdispatcher_cb_t)(iotdispatcher_config_t *config);

/**
 * @brief  init iot dispatcher
 * @param  [in] configs     : configurations for iot property
 * @param  [in] configs_len : configs len
 * @param  [in] cb          : user callback
 * @return 0 on success, < 0 on failed
 */
int iotdispatcher_init(iotdispatcher_config_t *configs, int configs_len, iotdispatcher_cb_t cb);

/**
 * @brief check engine result type
 *
 * @return true or false
 */
bool iotdispatcher_result_is_num();
bool iotdispatcher_result_is_string();
bool iotdispatcher_result_is_object();

/**
 * @brief  get engine result in different types 
 * @param  [inout] val     : val for the old state and to get the new result 
 * @return 0 on success, < 0 on failed
 */
int iotdispatcher_get_float_result(double *val);
int iotdispatcher_get_int_result(int *val);
int iotdispatcher_get_string_result(const char **val);
int iotdispatcher_get_object_result(cJSON **obj);

/**
 * @brief  set property from smartliving event
 * @param  [in] json         : property in json 
 * @return 0 on success, < 0 on failed
 */
int iotdispatcher_smartliving_set(char *json);

/**
 * @brief  set property from voice event
 * @param  [in] kw_str      : key word string 
 * @return 0 on success, < 0 on failed
 */
int iotdispatcher_voice_set(char *kw_str);

#ifdef __cplusplus
}
#endif

#endif