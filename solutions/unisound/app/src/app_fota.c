/*
 */

#include <app_config.h>

#include <aos/aos.h>

#include <yoc/fota.h>
#include <yoc/lpm.h>
#include <uservice/eventid.h>
#include <uservice/uservice.h>
#include <mvoice_alg.h>

#include "msg_process_center.h"


#define KV_FOTA_CYCLE_MS        "fota_cycle"
#define KV_FOTA_ENABLED         "fota_en"

#define TAG "app_fota"

static fota_t *g_fota_handle = NULL;
extern color_hsv_t HSV_Color;

/**
 *  return 0: still do the default handle
 *  not zero: only do the user handle
 */
static int fota_event_cb(void *arg, fota_event_e event)
{
    static int fota_progressing = 0;
    fota_t *fota = (fota_t *)arg;
    switch (event) {
        case FOTA_EVENT_VERSION:
            /* get new version, return -1 if we do not want upgrade */
            // printf("-----FOTA_EVENT_VERSION----\n");
            
            aos_kv_setint("ota_flag",1);
            int ret=aos_kv_set("HSV_Color",&HSV_Color,sizeof(HSV_Color),1);
            printf("ret=%d,HSV_Color:%d ,%d ,%d ,%d\n",ret,HSV_Color.light_switch,HSV_Color.h,HSV_Color.s,HSV_Color.v);
            LOGD(TAG, "FOTA VERSION :%x", fota->status);
            break;

        case FOTA_EVENT_PROGRESS:
            // printf("-----FOTA_EVENT_PROGRESS----\n");
            if (!fota_progressing) {
                fota_progressing = 1;
                mvoice_process_pause();
            }
            break;

        case FOTA_EVENT_FAIL:
            /* fota download or flash error
             * it will try next version check
             */
            // printf("-----FOTA_EVENT_FAIL----\n");
            LOGD(TAG, "FOTA FAIL :%x", fota->status);
            if (fota_progressing) {
                fota_progressing = 0;
                mvoice_process_resume();
            }
            break;

        case FOTA_EVENT_FINISH:
            // printf("-----FOTA_EVENT_FINISH----\n");
            LOGD(TAG, "FOTA FINISH :%x", fota->status);
            aos_kv_del("fota_offset");
            aos_reboot();
            break;

        default:
            break;
    }
    return 0;
}

void app_fota_init(void)
{
    int ret;
    LOGD(TAG, "%s", aos_get_app_version());

    int fota_en = 1;
    ret = aos_kv_getint(KV_FOTA_ENABLED, &fota_en);

    if (ret == 0 && fota_en == 0) {
        LOGI(TAG, "fota disabled, use \"kv setint fota_en 1\" to enable");
        return;
    }

    fota_register_cop();
    netio_register_http();
    netio_register_flash();
    g_fota_handle = fota_open("cop", "flash://misc", fota_event_cb);

    fota_config_t config;

    config.sleep_time = 720*60*1000;
    // config.sleep_time = 15*60*1000;
    config.timeoutms = 10000;
    config.retry_count = 0;

    if (pm_get_policy() == LPM_POLICY_NO_POWER_SAVE) {
        config.auto_check_en = 1;
    } else {
        config.auto_check_en = 0;
    }

    aos_kv_getint(KV_FOTA_CYCLE_MS, &(config.sleep_time));

    fota_config(g_fota_handle, &config);
/*
    set kv for debug
    const char *url = "http://cid.c-sky.com/api/image/ota/pull";
    aos_kv_setstring("otaurl", url);
    aos_kv_setstring("device_id", "ca14d54804400000315e5741718cc5a3");
    aos_kv_setstring("model", "sc5654a");
*/
}

void app_fota_start(void)
{
    if (g_fota_handle != NULL) {
        fota_start(g_fota_handle);
    }
}

int app_fota_is_downloading(void)
{
    if (g_fota_handle == NULL) {
        return 0;
    }
    if (fota_get_status(g_fota_handle) == FOTA_DOWNLOAD) {
        return 1;
    }
    return 0;
}

void app_fota_do_check(void)
{
    if (g_fota_handle == NULL) {
        return;
    }

    LOGD(TAG, "app_fota_do_check");

    /* do sleep here, for avoid lpm conflict */
    aos_msleep(200);
    fota_do_check(g_fota_handle);
}

void app_fota_set_auto_check(int enable)
{
    if (g_fota_handle == NULL) {
        return;
    }

    fota_set_auto_check(g_fota_handle, enable);
}
