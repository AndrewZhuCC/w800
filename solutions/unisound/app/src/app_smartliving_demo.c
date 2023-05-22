/*
 */
#include "stdio.h"
#include <aos/kernel.h>
#include <aos/debug.h>
#include <ulog/ulog.h>
#include <smartliving/exports/iot_export_linkkit.h>
#include "cJSON.h"
#include "app_main.h"

#include "msg_process_center.h"

#define TAG "SL"

#define USER_EXAMPLE_YIELD_TIMEOUT_MS  (10)                  //(200)

#define EXAMPLE_TRACE(...)                               \
    do {                                                     \
        HAL_Printf("\033[1;32;40m%s.%d: ", __func__, __LINE__);  \
        HAL_Printf(__VA_ARGS__);                                 \
        HAL_Printf("\033[0m\r\n");                                   \
    } while (0)
// led_strip_msg_t msg;
static int property_setting_handle(const char *request, const int request_len, led_strip_msg_t * msg);

typedef struct {
    int master_devid;
    int cloud_connected;
    int master_initialized;
    int started;
} user_example_ctx_t;

int rto_flag = 0;

static user_example_ctx_t g_user_example_ctx;

static user_example_ctx_t *user_example_get_ctx(void)
{
    return &g_user_example_ctx;
}

static int user_master_dev_available(void);

extern color_hsv_t HSV_Color;
static int user_connected_event_handler(void)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    printf("Cloud Connected\n");
    user_example_ctx->cloud_connected = 1;

#if defined(CONFIG_SMARTLIVING_AT_MODULE) && CONFIG_SMARTLIVING_AT_MODULE
        app_at_cmd_sta_report();
#endif

#if defined(OTA_ENABLED) && OTA_ENABLED
    extern int app_smartliving_fota_version_rpt(void);
    app_smartliving_fota_version_rpt();
#endif
    //user_player_play("[106]");
    //kws_relaunch();
    printf("HSV_Color:%d ,%d ,%d ,%d\n",HSV_Color.light_switch,HSV_Color.h,HSV_Color.s,HSV_Color.v);
    char property_payload[256];
    if(HSV_Color.light_switch==1) {
        snprintf(property_payload, sizeof(property_payload),
                "{\"powerstate\":%d,\"HSVColor\":{\"Saturation\":%d, \"Value\":%d, \"Hue\":%d}}", HSV_Color.light_switch, HSV_Color.s, HSV_Color.v, HSV_Color.h);
    } else {
        snprintf(property_payload, sizeof(property_payload),
                "{\"powerstate\":%d,\"HSVColor\":{\"Saturation\":%d, \"Value\":%d, \"Hue\":%d}}", HSV_Color.light_switch, HSV_Color.s, HSV_Color.v, HSV_Color.h);
    }
    // snprintf(property_payload, sizeof(property_payload),
    //             "{\"powerstate\":%d,\"HSVColor\":{\"Saturation\":%d, \"Value\":%d, \"Hue\":%d}}", 1, HSV_Color.s, HSV_Color.v, HSV_Color.h);
    user_post_given_property(property_payload, strlen(property_payload));

    return 0;
}

static int user_disconnected_event_handler(void)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    printf("Cloud Disconnected\n");

    user_example_ctx->cloud_connected = 0;

#if defined(CONFIG_SMARTLIVING_AT_MODULE) && CONFIG_SMARTLIVING_AT_MODULE
    app_at_cmd_sta_report();
#endif

    return 0;
}

static int user_service_request_event_handler(const int devid, const char *serviceid, const int serviceid_len,
        const char *request, const int request_len,
        char **response, int *response_len)
{
    cJSON *root = NULL;
    cJSON *hsvJson, *durJson;
    printf("Service Request Received, Devid: %d, Service ID: %.*s, Payload: %s", devid, serviceid_len, serviceid,
            request);

    root = cJSON_Parse(request);
    if (root == NULL || !cJSON_IsObject(root)) {
        printf("JSON Parse Error");
        return -1;
    }

    led_strip_msg_t msg;
    msg.arraysize = 1;
    if (strlen("Rhythm") == serviceid_len && memcmp("Rhythm", serviceid, serviceid_len) == 0)
    {
        hsvJson = cJSON_GetObjectItem(root, "Hue");
        msg.hsvcolor[0].h = hsvJson->valueint;

        hsvJson = cJSON_GetObjectItem(root, "Saturation");
        msg.hsvcolor[0].s = hsvJson->valueint;

        hsvJson = cJSON_GetObjectItem(root, "Value");
        msg.hsvcolor[0].v = hsvJson->valueint;

        durJson = cJSON_GetObjectItem(root, "LightDuration");
        msg.duration = durJson->valueint;

        msg.validindex = RHY_VALID;
        send_msg_to_queue(&msg);
    }

    cJSON_Delete(root);
    return 0;
}

int user_combo_get_dev_status_handler(uint8_t *buffer, uint32_t length)
{
    combo_ble_operation_e ble_ops = COMBO_BLE_MAX;
    cJSON *root = NULL;

    /* Parse Root */
    root = cJSON_Parse(buffer);
    if (root == NULL || !cJSON_IsObject(root)) {
        printf("%d, JSON Parse Error\n", __LINE__);
        return -1;
    }

    cJSON *method = cJSON_GetObjectItem(root, "method");
    if (strcmp((const char*)method->valuestring, "thing.service.property.set") == 0) {
        printf("combo: ble set\r\n");
        ble_ops = COMBO_BLE_SET;
    } else if (strcmp((const char*)method->valuestring, "thing.service.property.get") == 0) {
        printf("combo: ble get\r\n");
        ble_ops = COMBO_BLE_GET;
    }

    if (ble_ops == COMBO_BLE_SET) {
        cJSON *params = cJSON_GetObjectItemByPath(root, "params");
        if (!cJSON_IsObject(params)) {
            printf("%d, JSON Parse Error\n", __LINE__);
            return -1;
        }

        /* TODO: combo ble set 
         */
        combo_net_post_ext();
    }

    cJSON_Delete(root);

    return 0;
}

static int property_scene_handle(cJSON * root, led_strip_msg_t * msg)
{
    cJSON *item = NULL;
    light_scene_t *scenes = &msg->scenes;

    msg->validindex = SCE_VALID;

    if ((item = cJSON_GetObjectItem(root, "SceneId")) != NULL && cJSON_IsString(item)) {
        strncpy(scenes->name, item->valuestring, sizeof(scenes->name) -1);
    }

    if ((item = cJSON_GetObjectItem(root, "LightMode")) != NULL && cJSON_IsNumber(item)) {
        msg->lightmode = item->valueint;
    }

    if ((item = cJSON_GetObjectItem(root, "SceneMode")) != NULL && cJSON_IsNumber(item)) {
        scenes->scenemode = item->valueint;
    }

    if ((item = cJSON_GetObjectItem(root, "ColorSpeed")) != NULL && cJSON_IsNumber(item)) {
        msg->colorspeed = item->valueint;
    }

    if ((item = cJSON_GetObjectItem(root, "Brightness")) != NULL && cJSON_IsString(item)) {
        cJSON *btnJson = cJSON_Parse(item->valuestring);
        item = cJSON_GetObjectItem(btnJson, "max");
        if (item) {
            scenes->brightness_max = item->valueint;
        }
        item = cJSON_GetObjectItem(btnJson, "min");
        if (item) {
            scenes->brightness_min = item->valueint;
        }
        cJSON_Delete(btnJson);
    }

    if ((item = cJSON_GetObjectItem(root, "SceneItems")) != NULL && cJSON_IsString(item)) {
        cJSON *simJson = cJSON_Parse(item->valuestring);

        if ((item = cJSON_GetObjectItem(simJson, "Brightness")) != NULL && cJSON_IsNumber(item)) {
            msg->brightness[0] = item->valueint;
        }
        if ((item = cJSON_GetObjectItem(simJson, "ColorTemperature")) != NULL && cJSON_IsNumber(item)) {
            msg->colortemperature[0] = item->valueint;
        }
        cJSON_Delete(simJson);
    }

    if ((item = cJSON_GetObjectItem(root, "ColorArr")) != NULL && cJSON_IsString(item)) {
        
        char ColorArr_str[512] = {0};
        strncpy(ColorArr_str, item->valuestring, 512);
        cJSON *colorArr_json = cJSON_Parse(ColorArr_str);
        if (colorArr_json == NULL || !cJSON_IsArray(colorArr_json)) {     
            printf("color array isn't json format\n colorArr:%s\n colorArr_json ret=%d\n", ColorArr_str,colorArr_json->type);
            msg->validindex = INVALID;
        } else {
            int arrySize = cJSON_GetArraySize(colorArr_json);

            if (arrySize > MAX_SETTING_SIZE)
                arrySize = MAX_SETTING_SIZE;
            msg->arraysize = 0;
            if (arrySize > 0) {
                cJSON *arr_item = colorArr_json->child;
                cJSON *hsvJson, *enableJson;
                int i;

                for (i = 0; i < arrySize; i++) {
                    hsvJson = cJSON_GetObjectItem(arr_item, "Hue");
                    msg->hsvcolor[i].h = hsvJson->valueint;

                    hsvJson = cJSON_GetObjectItem(arr_item, "Saturation");
                    msg->hsvcolor[i].s = hsvJson->valueint;

                    hsvJson = cJSON_GetObjectItem(arr_item, "Value");
                    msg->hsvcolor[i].v = hsvJson->valueint;
                    arr_item = arr_item->next;
                }
                msg->arraysize = i;
            }
        }
        if (colorArr_json != NULL)
            cJSON_Delete(colorArr_json);
    }

    if ((item = cJSON_GetObjectItem(root, "Enable")) != NULL && cJSON_IsNumber(item)) {
        scenes->isenable = item->valueint;
    }
}

static int property_setting_handle(const char *request, const int request_len, led_strip_msg_t * msg)
{
    cJSON *root = NULL, *item = NULL;
    int ret = 0;

    if ((root = cJSON_Parse(request)) == NULL) {
        printf("property set payload is not JSON format\n");
        return -1;
    }
    msg->validindex = INVALID;

    if ((item = cJSON_GetObjectItem(root, "HSVColor")) != NULL && cJSON_IsObject(item)) {
        cJSON *hsvJson = cJSON_GetObjectItem(item, "Hue");

        msg->hsvcolor[0].h = hsvJson->valueint;

        hsvJson = cJSON_GetObjectItem(item, "Saturation");
        msg->hsvcolor[0].s = hsvJson->valueint;

        hsvJson = cJSON_GetObjectItem(item, "Value");
        msg->hsvcolor[0].v = hsvJson->valueint;

        msg->arraysize = 1;
        msg->validindex = HSV_VALID;
    }
    else if ((item = cJSON_GetObjectItem(root, "LightScene")) != NULL && cJSON_IsObject(item)) {
        msg->validindex = SCE_VALID;

        property_scene_handle(item, msg);
        char *fmt_str = cJSON_PrintUnformatted(item);
        if (fmt_str == NULL) {
            printf("cJSON_PrintUnformatted Error\n");
            cJSON_Delete(root);
            return -1;
        }
    }
    else if ((item = cJSON_GetObjectItem(root, "powerstate")) != NULL && cJSON_IsNumber(item)) {
        msg->lightswitch = item->valueint;
        msg->validindex = LS_VALID;
    }
#ifdef AOS_TIMER_SERVICE
    else if ((item = cJSON_GetObjectItem(root, "LocalTimer")) != NULL && cJSON_IsArray(item)) {
        timer_service_property_set(request);
    }
#endif
    else {
        printf("property is invalid\n");
        ret = -1;
    }

    cJSON_Delete(root);
    if (msg->validindex != INVALID)
        send_msg_to_queue(msg);

    return ret;
}


static int user_property_set_event_handler(const int devid, const char *request, const int request_len)
{
    printf("Property Set Received, Devid: %d, Request: %s\n", devid, request);

    led_strip_msg_t msg;

    property_setting_handle(request, request_len, &msg);

    iotdispatcher_smartliving_set((char *)request);

#if defined(CONFIG_SMARTLIVING_AT_MODULE) && CONFIG_SMARTLIVING_AT_MODULE
        app_at_cmd_property_report_set(request, request_len);
#endif

    return 0;
}

static int user_property_get_event_handler(const int devid, const char *request, const int request_len, char **response,
        int *response_len)
{
    cJSON *request_root = NULL, *item_propertyid = NULL;
    cJSON *response_root = NULL;
    int index = 0;
    printf("Property Get Received, Devid: %d, Request: %s\n", devid, request);

    /* Parse Request */
    request_root = cJSON_Parse(request);
    if (request_root == NULL || !cJSON_IsArray(request_root)) {
        printf("JSON Parse Error\n");
        return -1;
    }

    /* Prepare Response */
    response_root = cJSON_CreateObject();
    if (response_root == NULL) {
        printf("No Enough Memory1\n");
        cJSON_Delete(request_root);
        return -1;
    }

    for (index = 0; index < cJSON_GetArraySize(request_root); index++) {
        item_propertyid = cJSON_GetArrayItem(request_root, index);
        if (item_propertyid == NULL || !cJSON_IsString(item_propertyid)) {
            printf("JSON Parse Error\n");
            cJSON_Delete(request_root);
            cJSON_Delete(response_root);
            return -1;
        }

        printf("Property ID, index: %d, Value: %s\n", index, item_propertyid->valuestring);

        if (strcmp("WIFI_Tx_Rate", item_propertyid->valuestring) == 0) {
            cJSON_AddNumberToObject(response_root, "WIFI_Tx_Rate", 1111);
        } else if (strcmp("WIFI_Rx_Rate", item_propertyid->valuestring) == 0) {
            cJSON_AddNumberToObject(response_root, "WIFI_Rx_Rate", 2222);
        } else if (strcmp("WorkMode", item_propertyid->valuestring) == 0) {
            cJSON_AddNumberToObject(response_root, "WorkMode", 4);
        } else if (strcmp("NightLightSwitch", item_propertyid->valuestring) == 0) {
            cJSON_AddNumberToObject(response_root, "NightLightSwitch", 1);
        } else if (strcmp("brightness", item_propertyid->valuestring) == 0) {
            cJSON_AddNumberToObject(response_root, "brightness", 30);
        } else if (strcmp("powerstate", item_propertyid->valuestring) == 0) {
            cJSON_AddNumberToObject(response_root, "powerstate", 1);
        } else if (strcmp("ColorTemperature", item_propertyid->valuestring) == 0) {
            cJSON_AddNumberToObject(response_root, "ColorTemperature", 2800);
        } else if (strcmp("PropertyCharacter", item_propertyid->valuestring) == 0) {
            cJSON_AddStringToObject(response_root, "PropertyCharacter", "testprop");
        } else if (strcmp("Propertypoint", item_propertyid->valuestring) == 0) {
            cJSON_AddNumberToObject(response_root, "Propertypoint", 50);
        } else if (strcmp("LocalTimer", item_propertyid->valuestring) == 0) {
            cJSON *array_localtimer = cJSON_CreateArray();
            if (array_localtimer == NULL) {
                cJSON_Delete(request_root);
                cJSON_Delete(response_root);
                return -1;
            }

            cJSON *item_localtimer = cJSON_CreateObject();
            if (item_localtimer == NULL) {
                cJSON_Delete(request_root);
                cJSON_Delete(response_root);
                cJSON_Delete(array_localtimer);
                return -1;
            }
            cJSON_AddStringToObject(item_localtimer, "Timer", "10 11 * * * 1 2 3 4 5");
            cJSON_AddNumberToObject(item_localtimer, "Enable", 1);
            cJSON_AddNumberToObject(item_localtimer, "IsValid", 1);
            cJSON_AddItemToArray(array_localtimer, item_localtimer);
            cJSON_AddItemToObject(response_root, "LocalTimer", array_localtimer);
        }
        else if (strcmp("HSVColor", item_propertyid->valuestring) == 0) {
            cJSON *item_hsvcolor = cJSON_CreateObject();
            if (item_hsvcolor == NULL) {
                cJSON_Delete(request_root);
                cJSON_Delete(response_root);
                return -1;
            }
            cJSON_AddNumberToObject(item_hsvcolor, "Value", 98);
            cJSON_AddNumberToObject(item_hsvcolor, "Hue", 153);
            cJSON_AddNumberToObject(item_hsvcolor, "Saturation", 64);
            cJSON_AddItemToObject(response_root, "HSVColor", item_hsvcolor);
        }
    }
    cJSON_Delete(request_root);

    *response = cJSON_PrintUnformatted(response_root);
    if (*response == NULL) {
        printf("No Enough Memory2\n");
        cJSON_Delete(response_root);
        return -1;
    }
    cJSON_Delete(response_root);
    *response_len = strlen(*response);

    printf("Property Get Response: %s\n", *response);

    return SUCCESS_RETURN;
}

static int user_report_reply_event_handler(const int devid, const int msgid, const int code, const char *reply,
        const int reply_len)
{
    const char *reply_value = (reply == NULL) ? ("NULL") : (reply);
    const int reply_value_len = (reply_len == 0) ? (strlen("NULL")) : (reply_len);

    printf("Message Post Reply Received, Devid: %d, Message ID: %d, Code: %d, Reply: %.*s\n", devid, msgid, code,
                  reply_value_len,
                  reply_value);
#if defined(CONFIG_SMARTLIVING_AT_MODULE) && CONFIG_SMARTLIVING_AT_MODULE
    app_at_cmd_property_report_reply(msgid, code, reply, reply_len);
#endif
    return 0;
}

static int user_trigger_event_reply_event_handler(const int devid, const int msgid, const int code, const char *eventid,
        const int eventid_len, const char *message, const int message_len)
{
    printf("Trigger Event Reply Received, Devid: %d, Message ID: %d, Code: %d, EventID: %.*s, Message: %.*s\n", devid,
                  msgid, code,
                  eventid_len,
                  eventid, message_len, message);

#if defined(CONFIG_SMARTLIVING_AT_MODULE) && CONFIG_SMARTLIVING_AT_MODULE
    app_at_cmd_event_report_reply(msgid, code, eventid, eventid_len, message, message_len);
#endif

    return 0;
}

static int user_timestamp_reply_event_handler(const char *timestamp)
{
    printf("Current Timestamp: %s\n", timestamp);

    return 0;
}

static int user_initialized(const int devid)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    printf("Device Initialized, Devid: %d\n", devid);

    if (user_example_ctx->master_devid == devid) {
        user_example_ctx->master_initialized = 1;
    }

    return 0;
}

/** type:
  *
  * 0 - new firmware exist
  *
  */
#if defined(OTA_ENABLED) && OTA_ENABLED
static int user_fota_event_handler(int type, const char *version)
{
    char buffer[128] = {0};
    int buffer_length = 128;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    if (type == 0) {
        EXAMPLE_TRACE("New Firmware Version: %s", version);

        int total = 0,used = 0,mfree = 0,peak = 0;
    aos_get_mminfo(&total, &used, &mfree, &peak);
    printf("                   total      used      free      peak \r\n");
    printf("memory usage: %10d%10d%10d%10d\r\n\r\n",
           total, used, mfree, peak);

        IOT_Linkkit_Query(user_example_ctx->master_devid, ITM_MSG_QUERY_FOTA_DATA, (unsigned char *)buffer, buffer_length);
    }

    return 0;
}
#endif

/** type:
  *
  * 0 - new config exist
  *
  */
static int user_cota_event_handler(int type, const char *config_id, int config_size, const char *get_type,
                                   const char *sign, const char *sign_method, const char *url)
{
    char buffer[128] = {0};
    int buffer_length = 128;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    if (type == 0) {
        EXAMPLE_TRACE("New Config ID: %s", config_id);
        EXAMPLE_TRACE("New Config Size: %d", config_size);
        EXAMPLE_TRACE("New Config Type: %s", get_type);
        EXAMPLE_TRACE("New Config Sign: %s", sign);
        EXAMPLE_TRACE("New Config Sign Method: %s", sign_method);
        EXAMPLE_TRACE("New Config URL: %s", url);

        IOT_Linkkit_Query(user_example_ctx->master_devid, ITM_MSG_QUERY_COTA_DATA, (unsigned char *)buffer, buffer_length);
    }

    return 0;
}


static uint64_t user_update_sec(void)
{
    static uint64_t time_start_ms = 0;

    if (time_start_ms == 0) {
        time_start_ms = HAL_UptimeMs();
    }

    return (HAL_UptimeMs() - time_start_ms) / 1000;
}

#if defined(CONFIG_SMARTLIVING_DEMO) && CONFIG_SMARTLIVING_DEMO
void user_post_property(void)
{
    if (!user_master_dev_available()) {
        return;
    }

    /* TODO:  post all property
    int res = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    char property_payload[256];

    int light_switch = 1;
    int brightness = 1;
    uint8_t red = 0, green = 0, blue = 0;

    snprintf(property_payload, sizeof(property_payload),
             "{\"powerstate\":%d,\"brightness\":%d,\"RGBColor\":{\"Red\":%d, \"Green\":%d, \"Blue\":%d}}",\
              light_switch, brightness, red, green, blue);

    res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_POST_PROPERTY,
                             (unsigned char *)property_payload, strlen(property_payload));

    printf("Post Property Message ID: %d\n", res);
    */
}

int user_post_given_property(char *message, int len)
{
    if (!user_master_dev_available()) {
        return -1;
    }
    int res = 0;
    res = IOT_Linkkit_Report(user_example_get_ctx()->master_devid, ITM_MSG_POST_PROPERTY,
                              (unsigned char *)message, len);
    printf("Post Given Property Message ID: %d MSG: %s\n", res, message);
    return res;
}

int user_at_post_property(int device_id, char *message, int len)
{
    if (device_id != 0 || !user_master_dev_available()) {
        return -1;
    }

    return IOT_Linkkit_Report(user_example_get_ctx()->master_devid, ITM_MSG_POST_PROPERTY,
                              (unsigned char *)message, len);
}

int user_at_post_event(int device_id, char *evtid, int evtid_len, char *evt_payload, int len)
{
    if (device_id != 0 || !user_master_dev_available()) {
        return -1;
    }

    return IOT_Linkkit_TriggerEvent(user_example_get_ctx()->master_devid, evtid, evtid_len,
                                    evt_payload, len);
}

#endif

// void user_post_raw_data(void)
// {
//     int res = 0;
//     user_example_ctx_t *user_example_ctx = user_example_get_ctx();
//     unsigned char raw_data[7] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

//     res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_POST_RAW_DATA,
//                              raw_data, 7);
//     printf("Post Raw Data Message ID: %d\n", res);
// }

static int user_master_dev_available(void)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    if (user_example_ctx->cloud_connected && user_example_ctx->master_initialized) {
        return 1;
    }

    return 0;
}

static int user_get_iot_info(iotx_linkkit_dev_meta_info_t *iot_info)
{
    iotx_linkkit_dev_meta_info_t    master_meta_info;
    memset(&master_meta_info, 0, sizeof(iotx_linkkit_dev_meta_info_t));

    if (!HAL_GetProductKey(master_meta_info.product_key)
        || !HAL_GetDeviceName(master_meta_info.device_name)
        || !HAL_GetDeviceSecret(master_meta_info.device_secret)
        || !HAL_GetProductSecret(master_meta_info.product_secret)) {
        return -1;
    }

    if (iot_info) {
        *iot_info = master_meta_info;
    }

    return 0;
}

void linkkit_thread(void *paras)
{
    uint64_t                        time_prev_sec = 0, time_now_sec = 0;
    iotx_linkkit_dev_meta_info_t    master_meta_info;
    user_example_ctx_t             *user_example_ctx = user_example_get_ctx();

    user_example_get_ctx()->started = 1;

    /* Register Callback */
    IOT_RegisterCallback(ITE_CONNECT_SUCC, user_connected_event_handler);
    IOT_RegisterCallback(ITE_DISCONNECTED, user_disconnected_event_handler);
    // IOT_RegisterCallback(ITE_RAWDATA_ARRIVED, user_down_raw_data_arrived_event_handler);
    // IOT_RegisterCallback(ITE_SERVICE_REQUEST, user_service_request_event_handler);
    IOT_RegisterCallback(ITE_PROPERTY_SET, user_property_set_event_handler);
    IOT_RegisterCallback(ITE_PROPERTY_GET, user_property_get_event_handler);
    IOT_RegisterCallback(ITE_REPORT_REPLY, user_report_reply_event_handler);
    IOT_RegisterCallback(ITE_TRIGGER_EVENT_REPLY, user_trigger_event_reply_event_handler);
    IOT_RegisterCallback(ITE_TIMESTAMP_REPLY, user_timestamp_reply_event_handler);
    IOT_RegisterCallback(ITE_INITIALIZE_COMPLETED, user_initialized);
#if defined(OTA_ENABLED) && OTA_ENABLED
    IOT_RegisterCallback(ITE_FOTA, user_fota_event_handler);
#endif
    IOT_RegisterCallback(ITE_COTA, user_cota_event_handler);

    // IOT_DumpMemoryStats(IOT_LOG_DEBUG);
    // IOT_SetLogLevel(IOT_LOG_DEBUG);

    // check iot devinfo
    //memset(&master_meta_info, 0, sizeof(iotx_linkkit_dev_meta_info_t));
    if (user_get_iot_info(&master_meta_info) < 0) {
        LOGE(TAG, "missing iot devinfo\n");
        aos_task_exit(-1);
        return;
    }

    /* Choose Login Server, domain should be configured before IOT_Linkkit_Open() */
    int domain_type = IOTX_CLOUD_REGION_SHANGHAI;
    IOT_Ioctl(IOTX_IOCTL_SET_DOMAIN, (void *)&domain_type);

    /* Choose Login Method */
    int dynamic_register = 0;
    IOT_Ioctl(IOTX_IOCTL_SET_DYNAMIC_REGISTER, (void *)&dynamic_register);

    /* Choose Whether You Need Post Property/Event Reply */
    int post_event_reply = 1;
    IOT_Ioctl(IOTX_IOCTL_RECV_EVENT_REPLY, (void *)&post_event_reply);

    /* Create Master Device Resources */
    user_example_ctx->master_devid = IOT_Linkkit_Open(IOTX_LINKKIT_DEV_TYPE_MASTER, &master_meta_info);
    if (user_example_ctx->master_devid < 0) {
        printf("IOT_Linkkit_Open Failed\n");
        aos_task_exit(-1);
        return;
    }

    /* Start Connect Aliyun Server */
    while (IOT_Linkkit_Connect(user_example_ctx->master_devid) < 0) {
        HAL_SleepMs(1000);
    }

    while (user_example_get_ctx()->started) {
        if (rto_flag == 1) {
            rto_flag = 0;
            extern void wifi_network_reconnect(char *ssid, char *psk);
            static char wifi_ssid[32 + 1];
            int         wifi_ssid_len = sizeof(wifi_ssid);
            static char wifi_psk[64 + 1];
            int         wifi_psk_len = sizeof(wifi_psk);

            aos_kv_get("wifi_ssid", wifi_ssid, &wifi_ssid_len);
            aos_kv_get("wifi_psk", wifi_psk, &wifi_psk_len);

            if (strlen(wifi_ssid) > 0) {
                wifi_network_reconnect(wifi_ssid, wifi_psk);
            }
        }
        if (wifi_internet_is_connected() == 0){
            aos_msleep(1000);
            continue;
        }
        IOT_Linkkit_Yield(USER_EXAMPLE_YIELD_TIMEOUT_MS);
//         HAL_SleepMs(USER_EXAMPLE_YIELD_TIMEOUT_MS);

//         time_now_sec = user_update_sec();
//         if (time_prev_sec == time_now_sec) {
//             continue;
//         }

// #if defined(CONFIG_SMARTLIVING_DEMO) && CONFIG_SMARTLIVING_DEMO
//         /* Post Proprety Example */
//         if (time_now_sec % 30 == 0 && user_master_dev_available()) {
//             user_post_property();
//         }
// #endif
//         time_prev_sec = time_now_sec;
    }

    IOT_Linkkit_Close(user_example_ctx->master_devid);
    memset(user_example_ctx, 0, sizeof(user_example_ctx_t));

#if defined(CONFIG_SMARTLIVING_AT_MODULE) && CONFIG_SMARTLIVING_AT_MODULE
    app_at_cmd_sta_report();
#endif

    return;
}

int smartliving_client_control(const int start)
{
    LOGD(TAG, "enter smartliving_client_control, %d", start);
    if (start) {
        if (user_example_get_ctx()->started) {
            LOGW(TAG, "smartliving client already started\n");
            return 0;
        }

        if (user_get_iot_info(NULL)  < 0) {
            LOGW(TAG, "missing iot devinfo\n");
            return -1;
        }

        aos_task_t tsk;
        int ret = aos_task_new_ext(&tsk, "mqttkit", linkkit_thread, NULL, 1024*5, AOS_DEFAULT_APP_PRI);
        if (ret == 0) {
            LOGD(TAG, "smartliving client started\n");
        } else {
            LOGE(TAG, "start smartliving client failed\n");
        }
        return ret;
    } else {
        if (!user_example_get_ctx()->started) {
            LOGW(TAG, "smartliving client not started\n");
            return -1;
        }

        user_example_get_ctx()->started = 0;

        return 0;
    }
}

int smartliving_client_is_connected(void)
{
    return user_master_dev_available();
}
