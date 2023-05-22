/*
 */

#include <devices/wifi.h>
#include <us615_devops.h>
#include <yoc/netmgr.h>
#include <yoc/netmgr_service.h>
#include <ntp.h>

#include "app_main.h"
#include "app_sys.h"
//#include <softap_prov.h>
#include <wifi_provisioning.h>

#if defined(CONFIG_WIFI_SMARTLIVING) && CONFIG_WIFI_SMARTLIVING
#include <smartliving/sl_wifi_prov.h>
#endif
#if defined(EN_COMBO_NET) && EN_COMBO_NET
#include "combo_net.h"
#endif

#define TAG "app_net"

#define NET_RESET_DELAY_TIME            30 /* 最大网络重连时间 second*/
#define NET_LPM_RECONNECT_DELAY_TIME    180 /*second*/
#define MAX_NET_RESET_ERR_TIMES         5  /* -1无效，若设置该值次数，达到该次数后系统重启 */
#define MAX_NET_RETRY_TIMES             3   /* 该值在开启低功耗模式生效，达到次数允许进入睡眠 */

netmgr_hdl_t app_netmgr_hdl;
static int ntp_sync_flag;
static int wifi_pairing;
static int wifi_pair_need_broadcast;
static int net_reset_err_times = 0;
static int net_reset_delay_time = 2;
static int net_connecting = 1;
static int net_lpm_en = 0;
static int pair_thread_running = 0;
#if defined(CONFIG_WIFI_SMARTLIVING) && CONFIG_WIFI_SMARTLIVING
int wifi_prov_method = WIFI_PROVISION_SL_BLE;
#else
int wifi_prov_method = WIFI_PROVISION_SOFTAP;
#endif

void wifi_network_reconnect(char *ssid, char *psk);

int wifi_internet_is_connected()
{
    return (ntp_sync_flag == 1);
}

void wifi_set_lpm(int lpm_on)
{
    if (app_netmgr_hdl) {
        aos_dev_t *dev = netmgr_get_dev(app_netmgr_hdl);
        hal_wifi_set_lpm(dev, lpm_on ? WIFI_LPM_KEEP_LINK : WIFI_LPM_NONE);
    }
}

static void internet_set_connected(int connected)
{
    ntp_sync_flag = connected;
}

static int wifi_network_inited()
{
    return (app_netmgr_hdl != NULL);
}

int wifi_is_pairing()
{
    return (wifi_pairing);
}

static void wifi_set_pairing(int pairing)
{
    wifi_pairing = pairing;
}


static void network_reset_handle(uint32_t event_id)
{
    switch(event_id) {
    case EVENT_NETMGR_NET_DISCON:
        net_reset_err_times++;
        LOGD(TAG, "net reset cnt %d", net_reset_err_times);
        
        if (net_reset_err_times >= MAX_NET_RESET_ERR_TIMES && MAX_NET_RESET_ERR_TIMES > 0) {
            LOGD(TAG, "Net Reset times %d, reconnect", net_reset_err_times);
            static char wifi_ssid[32 + 1];
            int wifi_ssid_len = sizeof(wifi_ssid);
            static char wifi_psk[64 + 1];
            int wifi_psk_len = sizeof(wifi_psk);

            aos_kv_get("wifi_ssid", wifi_ssid, &wifi_ssid_len);
            aos_kv_get("wifi_psk", wifi_psk, &wifi_psk_len);

            if (strlen(wifi_ssid) > 0) {
                wifi_network_reconnect(wifi_ssid, wifi_psk);
            } else {
                aos_reboot();
            }
        } else {
            if (net_lpm_en && net_reset_err_times > MAX_NET_RETRY_TIMES) {
                LOGD(TAG, "Net Reset times %d, goto sleep", net_reset_err_times);
                // if in low power mode, net should stop retry to allow system to goto sleep
                event_publish(EVENT_NET_LPM_RECONNECT, NULL);
            } else {
                if (!wifi_is_pairing()) {
                    net_connecting = 1;
                    netmgr_reset(app_netmgr_hdl, net_reset_delay_time);

                    /* double delay time to reconnect */
                    net_reset_delay_time *= 2;
                    if (net_reset_delay_time > NET_RESET_DELAY_TIME) {
                        net_reset_delay_time = NET_RESET_DELAY_TIME;
                    }
                }
            }
        }
        break;
    case EVENT_NETMGR_GOT_IP:
        net_connecting = 0;
        net_reset_err_times = 0;
        net_reset_delay_time = 2;
        break;
    case EVENT_NET_LPM_RECONNECT:
        LOGD(TAG, "reconnect %d s later", NET_LPM_RECONNECT_DELAY_TIME);
        net_connecting = 0;
        net_reset_err_times = 0;
        net_reset_delay_time = 2;
        event_publish_delay(EVENT_NETMGR_NET_DISCON, NULL, NET_LPM_RECONNECT_DELAY_TIME * 1000);
    default:
        break;
    }
}

static void network_normal_handle(uint32_t event_id, const void *param)
{
    static int synced_before = 0;

    switch (event_id) {
    case EVENT_NETMGR_GOT_IP: {
        printf("got IP!!!\n");
        event_publish(EVENT_NTP_RETRY_TIMER, NULL);

        /* 配网成功后广播通知 */
        if (wifi_pair_need_broadcast == 1) {
            wifi_pair_need_broadcast = 2;

            if (wifi_prov_method == WIFI_PROVISION_SOFTAP) {
                static uint8_t mac[6] = {0};
                aos_dev_t *dev = netmgr_get_dev(app_netmgr_hdl);
                hal_wifi_get_mac_addr(dev, mac);
                //wifi_pair_broadcast(mac);
            }
#ifdef EN_COMBO_NET
            if (wifi_prov_method == WIFI_PROVISION_SL_BLE) {
                combo_ap_conn_notify();
            }
#endif

        }
    } break;

    case EVENT_NETMGR_NET_DISCON: {
        LOGD(TAG, "Net down");
        /* 不主动语音提示异常，等有交互再提示 */
        internet_set_connected(0);

        //app_status_update();
    } break;

    case EVENT_NET_CHECK_TIMER: {
        if (wifi_internet_is_connected() == 0) {
            LOGD(TAG, "wifi connection: check failed");
        } else {
            LOGI(TAG, "wifi connection: check ok");
        }
    } break;

    case EVENT_NTP_RETRY_TIMER:
        if (wifi_is_connected_to_ap() == 0) {
            break;
        }
        
        if (ntp_sync_time(NULL) == 0) {
#if defined(CONFIG_RTC_EN) && CONFIG_RTC_EN
            /* 网络对时成功,同步到RTC中 */
            rtc_from_system();
            app_clock_alarm_init();
#endif
            if (wifi_internet_is_connected() == 0){
                /* 同步到时间,确认网络成功,提示音和升级只在第一次启动 */
                internet_set_connected(1);

                //app_status_update();

                if (!synced_before || wifi_pair_need_broadcast) {
#if defined(CONFIG_WIFI_SMARTLIVING) && CONFIG_WIFI_SMARTLIVING
                    if (wifi_pair_need_broadcast) {
                        LOGD(TAG, "wifi provisiong success");
                        wifi_prov_sl_set_connected(1);
#if !(defined(CONFIG_SMARTLIVING_DEMO) && CONFIG_SMARTLIVING_DEMO)
                        wifi_prov_sl_start_report();
#endif
                    }
#endif

                    synced_before = 1;
                    wifi_pair_need_broadcast = 0;
                    
                    event_publish(EVENT_NET_NTP_SUCCESS, NULL);
#if defined(CONFIG_SMARTLIVING_DEMO) && CONFIG_SMARTLIVING_DEMO
                    smartliving_client_control(1);
#endif
#if defined(APP_FOTA_EN) && APP_FOTA_EN
                    app_fota_start();
#endif
                }
            }
        } else {
            /* 同步时间失败重试 */
            event_publish_delay(EVENT_NTP_RETRY_TIMER, NULL, 6000);
        }
        break;
    default:
        break;
    }
}


static void user_local_event_cb(uint32_t event_id, const void *param, void *context)
{
    if ((wifi_is_pairing() == 0) && wifi_network_inited()) {
        network_normal_handle(event_id, param);
        network_reset_handle(event_id);
    } else {
        LOGE(TAG, "Critical network status callback %d", event_id);
    }
}


static void wifi_network_deinit()
{
    LOGI(TAG, "Stop wifi network");
    if (app_netmgr_hdl == NULL) {
        LOGI(TAG, "network already stopped");
        return;
    }

    netmgr_stop(app_netmgr_hdl);
    netmgr_dev_wifi_deinit(app_netmgr_hdl);
    netmgr_service_deinit();

    app_netmgr_hdl = NULL;
}

netmgr_hdl_t wifi_network_init(char *ssid, char *psk)
{
    LOGI(TAG, "Start wifi network");
    LOGD(TAG, "SSID=%s PASS=%s", ssid, psk);

    utask_t *task = utask_new("netmgr", 1600, QUEUE_MSG_COUNT, AOS_DEFAULT_APP_PRI);

    netmgr_hdl_t hdl = netmgr_dev_wifi_init();
    netmgr_service_init(task);
    netmgr_config_wifi(hdl, ssid, strlen(ssid), psk, strlen(psk));
    aos_dev_t *dev = netmgr_get_dev(hdl);
    hal_net_set_hostname(dev, "uni-hbw");
    netmgr_start(hdl);
    app_netmgr_hdl = hdl;

    return hdl;
}

static void wifi_pair_callback(uint32_t method_id, wifi_prov_event_t event, wifi_prov_result_t *result)
{
    if (event == WIFI_PROV_EVENT_TIMEOUT) {
        LOGD(TAG, "wifi pair timeout...");

        // static char wifi_ssid[32 + 1];
        // int wifi_ssid_len = sizeof(wifi_ssid);
        // static char wifi_psk[64 + 1];
        // int wifi_psk_len = sizeof(wifi_psk);

        // aos_kv_get("wifi_ssid", wifi_ssid, &wifi_ssid_len);
        // aos_kv_get("wifi_psk", wifi_psk, &wifi_psk_len);

        // if (strlen(wifi_ssid) > 0)
        //     wifi_network_init(wifi_ssid, wifi_psk);

        app_sys_set_boot_reason(BOOT_REASON_SILENT_RESET);
        aos_reboot();
    } else if (event == WIFI_RPOV_EVENT_GOT_RESULT) {
        LOGD(TAG, "wifi pair got passwd...");

        aos_kv_setstring("wifi_ssid", result->ssid);
        aos_kv_setstring("wifi_psk", result->password);
        if (method_id == 0)/*ble method*/ {       
          wifi_network_init(result->ssid, result->password);
          event_publish_delay(EVENT_NET_CHECK_TIMER, NULL, 40*1000);
        } else if (method_id == 1)/*smart config method*/{
          HAL_SleepMs(300);
          app_sys_reboot();
        }
    } else if (event == WIFI_PROV_EVENT_FORCE_STOP) {
			// static char wifi_ssid[32 + 1];
			// int wifi_ssid_len = sizeof(wifi_ssid);
			// static char wifi_psk[64 + 1];
			// int wifi_psk_len = sizeof(wifi_psk);

			// aos_kv_get("wifi_ssid", wifi_ssid, &wifi_ssid_len);
			// aos_kv_get("wifi_psk", wifi_psk, &wifi_psk_len);

			// if (strlen(wifi_ssid) > 0)
			//     wifi_network_init(wifi_ssid, wifi_psk);

    }

    wifi_set_pairing(0);
}

static void _wifi_pair_thread(void *arg)
{
    pair_thread_running = 1;
    wifi_set_pairing(1);
    wifi_pair_need_broadcast = 1;

    LOGD(TAG, "start net config");
    //app_status_update();

    wifi_prov_sl_stop_report();
    wifi_prov_stop();

    internet_set_connected(0);

    if (wifi_network_inited()) {
        wifi_network_deinit();
    }

    switch (wifi_prov_method) {
        case WIFI_PROVISION_SOFTAP:
            wifi_prov_start(wifi_prov_get_method_id("softap"), wifi_pair_callback, 120);
            break;
#if defined(CONFIG_WIFI_SMARTLIVING) && CONFIG_WIFI_SMARTLIVING
        case WIFI_PROVISION_SL_DEV_AP:
            wifi_prov_start(wifi_prov_get_method_id("sl_dev_ap"), wifi_pair_callback, 120);
            break;

        case WIFI_PROVISION_SL_SMARTCONFIG:
            wifi_prov_start(wifi_prov_get_method_id("sl_smartconfig"), wifi_pair_callback, 120);
            break;

        case WIFI_PROVISION_SL_BLE:
            wifi_prov_start(wifi_prov_get_method_id("sl_ble"), wifi_pair_callback, 120);
            wifi_prov_start(wifi_prov_get_method_id("sl_smartconfig"), wifi_pair_callback, 120);
            break;
#endif
        default:
            LOGE(TAG, "unsupported wifi provisioning method!");
            wifi_set_pairing(0);
            break;
    }

    pair_thread_running = 0;
}

void wifi_pair_start(void)
{
    aos_task_t task_handle;

    if (pair_thread_running) {
        return;
    }

    if (0 != aos_task_new_ext(&task_handle, "pair_start", _wifi_pair_thread, NULL,
                                3*1024, AOS_DEFAULT_APP_PRI)) {
        LOGE(TAG, "Create pair_start task failed.");
    }
}

void wifi_pair_stop(void)
{
    if (wifi_prov_get_status() != WIFI_PROV_STOPED) {
        wifi_prov_stop();
        wifi_set_pairing(0);
        //app_status_update();
        char wifi_ssid[32 + 1] = "\0";
        int wifi_ssid_len = sizeof(wifi_ssid);
        char wifi_psk[64 + 1] = "\0";
        int wifi_psk_len = sizeof(wifi_psk);

        aos_kv_get("wifi_ssid", wifi_ssid, &wifi_ssid_len);
        aos_kv_get("wifi_psk", wifi_psk, &wifi_psk_len);

        if (strlen(wifi_ssid) > 0) {
            wifi_network_init(wifi_ssid, wifi_psk);
        }
    }
}

int wifi_connecting()
{
    return net_connecting;
}

void wifi_lpm_enable(int lpm_en)
{
    net_lpm_en = lpm_en;
}

int wifi_getmac(uint8_t mac[6])
{
    int ret = -1;

    aos_dev_t *dev = netmgr_get_dev(app_netmgr_hdl);

    if (dev) {
        ret = hal_wifi_get_mac_addr(dev, mac);
    }
    return ret;
}

static wifi_mode_e app_net_init(void)
{
    char ssid[32 + 1] = {0};
    int ssid_len = sizeof(ssid);
    char psk[64 + 1] = {0};
    int psk_len = sizeof(psk);

    /* 系统事件订阅 */
    event_subscribe(EVENT_NETMGR_GOT_IP, user_local_event_cb, NULL);
    event_subscribe(EVENT_NETMGR_NET_DISCON, user_local_event_cb, NULL);
  
    /* 使用系统事件的定时器 */
    event_subscribe(EVENT_NTP_RETRY_TIMER, user_local_event_cb, NULL);
    event_subscribe(EVENT_NET_CHECK_TIMER, user_local_event_cb, NULL);
    event_subscribe(EVENT_NET_LPM_RECONNECT, user_local_event_cb, NULL);

    aos_kv_get("wifi_ssid", ssid, &ssid_len);
    aos_kv_get("wifi_psk", psk, &psk_len);

    // if (strlen(ssid) == 0 || app_sys_get_boot_reason() == BOOT_REASON_WIFI_CONFIG) {
    if (strlen(ssid) == 0 ) {                                           //没有配网
        if (app_sys_get_boot_reason() == BOOT_REASON_WIFI_CONFIG) {     //没有配网但是进入配网状态
            int last_method = 0;
            if (aos_kv_getint("wprov_method", &last_method) == 0) {
                wifi_prov_method = last_method;
                aos_kv_del("wprov_method");
            }
            wifi_pair_start();
            // app_sys_set_boot_reason(BOOT_REASON_SOFT_RESET);
            return MODE_WIFI_PAIRING;
        }
        // wifi_pair_start();
        // app_sys_set_boot_reason(BOOT_REASON_SOFT_RESET);
        // return MODE_WIFI_PAIRING;
    } else {
        wifi_network_init(ssid, psk);
    }
    return MODE_WIFI_NORMAL;
}

wifi_mode_e app_network_init(void)
{
    int wifi_en;
    int ret;
    ret = aos_kv_getint("wifi_en", &wifi_en);
    if (ret < 0) {
        /* 若没有设置KV,默认使能 */
        wifi_en = 1;
    }

    if (wifi_en == 0) {
        return MODE_WIFI_CLOSE;
    }

    wifi_us615_register(NULL);

#if defined(CONFIG_YOC_SOFTAP_PROV) && CONFIG_YOC_SOFTAP_PROV
    wifi_prov_softap_register("YoC");
#endif
#if defined(CONFIG_WIFI_SMARTLIVING) && CONFIG_WIFI_SMARTLIVING
    wifi_prov_sl_register();
#endif

    return app_net_init();
}

wifi_mode_e app_network_reinit()
{
    //app_status_update();

    wifi_prov_sl_stop_report();
    wifi_prov_stop();

    internet_set_connected(0);

    if (wifi_network_inited()) {
        wifi_network_deinit();
    }
    return app_net_init();
}

void wifi_network_reconnect(char *ssid, char *psk)
{
    internet_set_connected(0);
    if (wifi_is_pairing) {
        wifi_prov_stop();
        wifi_set_pairing(0);
    }
    if (wifi_network_inited()) {
        wifi_network_deinit();
    }
    wifi_network_init(ssid, psk);
}

static void handle_netmgr_cmd(char *pwbuf, int blen, int argc, char **argv)
{
    const char *rtype = argc > 1 ? argv[1] : "";
    if ((strcmp(rtype, "connect") == 0) && (argc == 4)) {
#ifdef WIFI_PROVISION_ENABLED
        extern int awss_stop(void);
        awss_stop();
#endif
        int ret = aos_kv_setstring("wifi_ssid", argv[2]);
        printf("kv set string [%s:%s] %s\n", "wifi_ssid", argv[2], ret < 0 ? "failed" : "success");
        ret = aos_kv_setstring("wifi_psk", argv[3]);
        printf("kv set string [%s:%s] %s\n", "wifi_psk", argv[3], ret < 0 ? "failed" : "success");
        if (ret >= 0) {
            wifi_network_reconnect(argv[2], argv[3]);
        }
    } else {
        /* param not recognised */
        printf("Invalid parameters! Usage: \n\tnetmgr [connect ssid password|clear]\n");
    }
}

void cli_reg_cmd_netmgr(void)
{
    static const struct cli_command cmd_info = {
        "netmgr",
        "netmgr [connect ssid password|clear]",
        handle_netmgr_cmd,
    };
    aos_cli_register_command(&cmd_info);
}
