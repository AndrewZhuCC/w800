/*
 */
#ifdef EN_COMBO_NET
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <devices/uart.h>

#include "aos/kernel.h"
#include <aos/yloop.h>
#include <driver/uni_efuse.h>
//#include <devices/wifi.h>
#include "yoc/netmgr.h"
#include "yoc/netmgr_service.h"
#include "smartliving/iot_export.h"
#include "smartliving/iot_import.h"
#include "smartliving/imports/iot_import_awss.h"
#include "breeze_export.h"
#include "combo_net.h"
#include "app_main.h"
#include "hlkkey.h"

#include "msg_process_center.h"

#define HLK_VER "HLK_800_PRO-V1.0.0"
#define HAL_IEEE80211_IS_BEACON        ieee80211_is_beacon
#define HAL_IEEE80211_IS_PROBE_RESP    ieee80211_is_probe_resp
#define HAL_IEEE80211_GET_SSID         ieee80211_get_ssid
#define HAL_IEEE80211_GET_BSSID        aw_ieee80211_get_bssid
#define HAL_AWSS_OPEN_MONITOR          HAL_Awss_Open_Monitor
#define HAL_AWSS_CLOSE_MONITOR         HAL_Awss_Close_Monitor
#define HAL_AWSS_SWITCH_CHANNEL        HAL_Awss_Switch_Channel

#define MAX_SSID_SIZE             32
#define MAX_PWD_SIZE           64

#ifndef MAX_SSID_LEN
#define MAX_SSID_LEN (MAX_SSID_SIZE+1)
#endif

static aos_dev_t *hlk_uart_handle = NULL;
static ALINKDEV_t g_hlk_devinfo = {0};

struct ieee80211_hdr {
    uint16_t frame_control;
    uint16_t duration_id;
    uint8_t addr1[ETH_ALEN];
    uint8_t addr2[ETH_ALEN];
    uint8_t addr3[ETH_ALEN];
    uint16_t seq_ctrl;
    uint8_t addr4[ETH_ALEN];
};

typedef struct {
    char ssid[MAX_SSID_SIZE + 1];
    uint8_t bssid[ETH_ALEN];
    char pwd[MAX_PWD_SIZE + 1];
} netmgr_ap_config_t;

typedef struct combo_user_bind_s {
    uint8_t bind_state;
    uint8_t sign_state;
    uint8_t need_sign;
} combo_user_bind_t;

static combo_user_bind_t g_combo_bind = { 0 };

#define LOG(format, args...)  printf(format"\n", ##args)
char awss_running; // todo
static breeze_apinfo_t apinfo;
static char monitor_got_bssid = 0;
static netmgr_ap_config_t config;
uint8_t g_ble_state = 0;
static uint8_t g_disconn_flag = 0;
extern uint8_t g_ap_state;
uint8_t g_bind_state = 0;
static wifi_prov_cb prov_cb = NULL;

extern uint8_t *os_wifi_get_mac(uint8_t mac[6]);
extern netmgr_hdl_t wifi_network_init(char *ssid, char *psk);

extern int user_combo_get_dev_status_handler(uint8_t *buffer, uint32_t length);

/* Device info(five elements) will be used by ble breeze */
/* ProductKey, ProductSecret, DeviceName, DeviceSecret, ProductID */
char g_combo_pk[PRODUCT_KEY_LEN + 1] = { 0 };
char g_combo_ps[PRODUCT_SECRET_LEN + 1] = { 0 };
char g_combo_dn[DEVICE_NAME_LEN + 1] = { 0 };
char g_combo_ds[DEVICE_SECRET_LEN + 1] = { 0 };
char g_fota_model[FOTA_MODEL_LEN + 1] = {0};
char g_fota_id[FOTA_ID_LEN + 1] = {0};
char g_fota_otaurl[FOTA_OTAURL_LEN + 1] = {0};

uint32_t g_combo_pid = 0;
unsigned int magic=0;
unsigned int ota_flag=0;

ALINKDEV_t alinkDev;

uint8_t combo_get_ap_state(void);

/*
 * Note:
 * the linkkit_event_monitor must not block and should run to complete fast
 * if user wants to do complex operation with much time,
 * user should post one task to do this, not implement complex operation in
 * linkkit_event_monitor
 */
// extern aos_queue_t *g_device_state_changed_queue_id;
static void linkkit_event_monitor(int event)
{   
    switch (event) {
        case IOTX_AWSS_START: // AWSS start without enbale, just supports device discover
            // operate led to indicate user
            LOG("IOTX_AWSS_START");
            break;
        case IOTX_AWSS_ENABLE: // AWSS enable, AWSS doesn't parse awss packet until AWSS is enabled.
            LOG("IOTX_AWSS_ENABLE");
            // operate led to indicate user
            break;
        case IOTX_AWSS_LOCK_CHAN: // AWSS lock channel(Got AWSS sync packet)
            LOG("IOTX_AWSS_LOCK_CHAN");
            // operate led to indicate user
            break;
        case IOTX_AWSS_PASSWD_ERR: // AWSS decrypt passwd error
            LOG("IOTX_AWSS_PASSWD_ERR");
            // operate led to indicate user
            break;
        case IOTX_AWSS_GOT_SSID_PASSWD:
            LOG("IOTX_AWSS_GOT_SSID_PASSWD");
            // operate led to indicate user
            //set_net_state(GOT_AP_SSID);
            break;
        case IOTX_AWSS_CONNECT_ADHA: // AWSS try to connnect adha (device
            // discover, router solution)
            LOG("IOTX_AWSS_CONNECT_ADHA");
            // operate led to indicate user
            break;
        case IOTX_AWSS_CONNECT_ADHA_FAIL: // AWSS fails to connect adha
            LOG("IOTX_AWSS_CONNECT_ADHA_FAIL");
            // operate led to indicate user
            break;
        case IOTX_AWSS_CONNECT_AHA: // AWSS try to connect aha (AP solution)
            LOG("IOTX_AWSS_CONNECT_AHA");
            // operate led to indicate user
            break;
        case IOTX_AWSS_CONNECT_AHA_FAIL: // AWSS fails to connect aha
            LOG("IOTX_AWSS_CONNECT_AHA_FAIL");
            // operate led to indicate user
            break;
        case IOTX_AWSS_SETUP_NOTIFY: // AWSS sends out device setup information
            // (AP and router solution)
            LOG("IOTX_AWSS_SETUP_NOTIFY");
            // operate led to indicate user
            break;
        case IOTX_AWSS_CONNECT_ROUTER: // AWSS try to connect destination router
            LOG("IOTX_AWSS_CONNECT_ROUTER");
            // operate led to indicate user
            break;
        case IOTX_AWSS_CONNECT_ROUTER_FAIL: // AWSS fails to connect destination
            // router.
            LOG("IOTX_AWSS_CONNECT_ROUTER_FAIL");
            //set_net_state(CONNECT_AP_FAILED);
            // operate led to indicate user
            break;
        case IOTX_AWSS_GOT_IP: // AWSS connects destination successfully and got
            // ip address
            LOG("IOTX_AWSS_GOT_IP");
            // operate led to indicate user
            break;
        case IOTX_AWSS_SUC_NOTIFY: // AWSS sends out success notify (AWSS
            // sucess)
            LOG("IOTX_AWSS_SUC_NOTIFY");
            // operate led to indicate user
            break;
        case IOTX_AWSS_BIND_NOTIFY: // AWSS sends out bind notify information to
            // support bind between user and device
            LOG("IOTX_AWSS_BIND_NOTIFY");
            /* FIXME: reboot to clear ble and smartliving memory usage */
            app_sys_set_boot_reason(BOOT_REASON_SILENT_RESET);
            aos_reboot();
            // operate led to indicate user
            //user_example_ctx_t *user_example_ctx = user_example_get_ctx();
            //user_example_ctx->bind_notified = 1;
            break;
        case IOTX_AWSS_ENABLE_TIMEOUT: // AWSS enable timeout
            // user needs to enable awss again to support get ssid & passwd of router
            LOG("IOTX_AWSS_ENALBE_TIMEOUT");
            // operate led to indicate user
            break;
        case IOTX_CONN_CLOUD: // Device try to connect cloud
            LOG("IOTX_CONN_CLOUD");
            // operate led to indicate user
            user_pwm_enable(PB0,50);
            user_pwm_enable(PB1,0);
            user_pwm_enable(PB2,0);
            break;
        case IOTX_CONN_CLOUD_FAIL: // Device fails to connect cloud, refer to
            // net_sockets.h for error code
            LOG("IOTX_CONN_CLOUD_FAIL");
            //set_net_state(CONNECT_CLOUD_FAILED);
            // operate led to indicate user
            break;
        case IOTX_CONN_CLOUD_SUC: // Device connects cloud successfully
            //set_net_state(CONNECT_CLOUD_SUCCESS);
            LOG("IOTX_CONN_CLOUD_SUC");
            // operate led to indicate user
            user_pwm_enable(PB0,0);
            user_pwm_enable(PB1,50);
            user_pwm_enable(PB2,0);
            break;
        case IOTX_RESET: // Linkkit reset success (just got reset response from
            // cloud without any other operation)
            LOG("IOTX_RESET");
            break;
        case IOTX_CONN_REPORT_TOKEN_SUC:
#ifdef EN_COMBO_NET
            combo_token_report_notify();
#endif
            LOG("---- report token success ----");
            break;
        default:
            break;
    }
}

static void combo_status_change_cb(breeze_event_t event)
{
   switch (event) {
        case CONNECTED:
            LOG("Ble Connected");
            LOG("Disable mvoice alg to release memory for device bind");
            mvoice_alg_deinit();
            g_ble_state = 1;
            break;

        case DISCONNECTED:
            LOG("Ble Disconnected");
            g_ble_state = 0;
            // aos_post_event(EV_BZ_COMBO, COMBO_EVT_CODE_RESTART_ADV, 0);
            event_publish(EVENT_SLBLE_RESTART_ADV, NULL);
            break;

        case AUTHENTICATED:
            LOG("Ble Authenticated");
            break;

        case TX_DONE:
            if (g_disconn_flag && g_ble_state) {
                LOG("Payload TX Done");
                breeze_disconnect_ble();
                breeze_stop_advertising();
                g_disconn_flag = 2;
            }
            break;

        case EVT_USER_BIND:
            LOG("Ble bind");
            g_bind_state = 1;
            awss_clear_reset();
            break;

        case EVT_USER_UNBIND:
            LOG("Ble unbind");
            g_bind_state = 0;
            break;

        default:
            break;
    }
}

void combo_net_post_ext(void)
{
    char format[] = "{\"code\":200,\"data\":{},\"id\":\"2\"}";
    breeze_post_ext_fast(3/*BZ_CMD_REPL*/, format, strlen(format)); // BZ_CMD_REPL
}

static void combo_set_dev_status_handler(uint8_t *buffer, uint32_t length)
{
    LOG("COM_SET:%.*s", length, buffer);
}

static void combo_get_dev_status_handler(uint8_t *buffer, uint32_t length)
{
    LOG("COM_GET:%.*s", length, buffer);

    user_combo_get_dev_status_handler(buffer, length);
}

static void combo_apinfo_rx_cb(breeze_apinfo_t * ap)
{
    if (!ap)
        return;

    memcpy(&apinfo, ap, sizeof(apinfo));
    // aos_post_event(EV_BZ_COMBO, COMBO_EVT_CODE_AP_INFO, (unsigned long)&apinfo);
    event_publish(EVENT_SLBLE_AP_INFO, &apinfo);
}

static int combo_80211_frame_callback(char *buf, int length, enum AWSS_LINK_TYPE link_type, int with_fcs, signed char rssi)
{
    uint8_t ssid[MAX_SSID_LEN] = {0}, bssid[ETH_ALEN] = {0};
    struct ieee80211_hdr *hdr;
    int fc;
    int ret = -1;

    if (link_type != AWSS_LINK_TYPE_NONE) {
        return -1;
    }
    hdr = (struct ieee80211_hdr *)buf;

    fc = hdr->frame_control;
    if (!HAL_IEEE80211_IS_BEACON(fc) && !HAL_IEEE80211_IS_PROBE_RESP(fc)) {
        return -1;
    }
    ret = HAL_IEEE80211_GET_BSSID((uint8_t*)hdr, bssid);
    if (ret < 0) {
        return -1;
    }
    if (memcmp(config.bssid, bssid, ETH_ALEN) != 0) {
        return -1;
    }
    ret = HAL_IEEE80211_GET_SSID((uint8_t*)hdr, length, ssid);
    if (ret < 0) {
        return -1;
    }
    monitor_got_bssid = 1;
    strncpy(config.ssid, ssid, sizeof(config.ssid) - 1);
    HAL_AWSS_CLOSE_MONITOR();
    return 0;
}

static char is_str_asii(char *str)
{
    for (uint32_t i = 0; str[i] != '\0'; ++i) {
        if ((uint8_t) str[i] > 0x7F) {
            return 0;
        }
    }
    return 1;
}

static void combo_connect_ap(breeze_apinfo_t * info)
{
    uint64_t pre_chn_time = HAL_UptimeMs();
    int ms_cnt = 0;
    uint8_t ap_connected = 0;
    ap_scan_info_t scan_result;
    int ap_scan_result = -1;

    memset(&config, 0, sizeof(netmgr_ap_config_t));
    if (!info)
        return;

    strncpy(config.ssid, info->ssid, sizeof(config.ssid) - 1);
    strncpy(config.pwd, info->pw, sizeof(config.pwd) - 1);
    if (info->apptoken_len > 0) {
        memcpy(config.bssid, info->bssid, ETH_ALEN);
        awss_set_token(info->apptoken, info->token_type);
#if 0        
        if (!is_str_asii(config.ssid)) {
            LOG("SSID not asicci encode");
            HAL_AWSS_OPEN_MONITOR(combo_80211_frame_callback);
            while (1) {
                aos_msleep(50);
                if (monitor_got_bssid) {
                    break;
                }
                uint64_t cur_chn_time = HAL_UptimeMs();
                if (cur_chn_time - pre_chn_time > 250) {
                    int channel = aws_next_channel();
                    HAL_AWSS_SWITCH_CHANNEL(channel, 0, NULL);
                    pre_chn_time = cur_chn_time;
                }
            }
        }
#endif        
    }
    // get region information
    if (info->region_type == REGION_TYPE_ID) {
        LOG("info->region_id: %d", info->region_id);
        iotx_guider_set_dynamic_region(info->region_id);
    } else if (info->region_type == REGION_TYPE_MQTTURL) {
        LOG("info->region_mqtturl: %s", info->region_mqtturl);
        iotx_guider_set_dynamic_mqtt_url(info->region_mqtturl);
    } else {
        LOG("REGION TYPE not supported");
        iotx_guider_set_dynamic_region(IOTX_CLOUD_REGION_INVALID);
    }
#if 0
    netmgr_set_ap_config(&config);
    hal_wifi_suspend_station(NULL);
    netmgr_reconnect_wifi();

    while (ms_cnt < 30000) {
        // set connect ap timeout
        if (netmgr_get_ip_state() == false) {
            aos_msleep(500);
            ms_cnt += 500;
            LOG("wait ms_cnt(%d)", ms_cnt);
        } else {
            LOG("AP connected");
            ap_connected = 1;
            break;
        }
    }

    if (!ap_connected) {
        uint16_t err_code = 0;

        // if AP connect fail, should inform the module to suspend station
        // to avoid module always reconnect and block Upper Layer running
        hal_wifi_suspend_station(NULL);

        // if ap connect failed in specified timeout, rescan and analyse fail reason
        memset(&scan_result, 0, sizeof(ap_scan_info_t));
        LOG("start awss_apscan_process");
        ap_scan_result = awss_apscan_process(NULL, config.ssid, &scan_result);
        LOG("stop awss_apscan_process");
        if ( (ap_scan_result == 0) && (scan_result.found) ) {
            if (scan_result.rssi < -70) {
                err_code = 0xC4E1; // rssi too low
            } else {
                err_code = 0xC4E3; // AP connect fail(Authentication fail or Association fail or AP exeption)
            }
            // should set all ap info to err msg
        } else {
            err_code = 0xC4E0; // AP not found
        }

        if(g_ble_state){
            uint8_t ble_rsp[DEV_ERRCODE_MSG_MAX_LEN + 8] = {0};
            uint8_t ble_rsp_idx = 0;
            ble_rsp[ble_rsp_idx++] = 0x01;                          // Notify Code Type
            ble_rsp[ble_rsp_idx++] = 0x01;                          // Notify Code Length
            ble_rsp[ble_rsp_idx++] = 0x02;                          // Notify Code Value, 0x02-fail
            ble_rsp[ble_rsp_idx++] = 0x03;                          // Notify SubErrcode Type
            ble_rsp[ble_rsp_idx++] = sizeof(err_code);              // Notify SubErrcode Length
            memcpy(ble_rsp + ble_rsp_idx, (uint8_t *)&err_code, sizeof(err_code));  // Notify SubErrcode Value
            ble_rsp_idx += sizeof(err_code);
            breeze_post(ble_rsp, ble_rsp_idx);
        }
    }
#else
    wifi_prov_result_t res;
    strncpy(res.ssid, info->ssid, sizeof(res.ssid) - 1);
    strncpy(res.password, info->pw, sizeof(res.password) - 1);

    if (prov_cb)
        prov_cb(0, WIFI_RPOV_EVENT_GOT_RESULT, &res);

    aos_msleep(3000);

    netmgr_hdl_t hdl = netmgr_get_handle("wifi");
    if (hdl) {
        while (ms_cnt < 30000) {
            if (!netmgr_is_gotip(hdl)) {
                aos_msleep(500);
                ms_cnt += 500;
                LOG("wait ms_cnt(%d)", ms_cnt);
            } else {
                LOG("AP connected");
                ap_connected = 1;
                break;
            }
        }
    }

    if (!ap_connected) {
        uint16_t err_code = 0;

        memset(&scan_result, 0, sizeof(ap_scan_info_t));
        LOG("start awss_apscan_process");
        ap_scan_result = awss_apscan_process(NULL, config.ssid, &scan_result);
        LOG("stop awss_apscan_process");
        if ( (ap_scan_result == 0) && (scan_result.found) ) {
            if (scan_result.rssi < -70) {
                err_code = 0xC4E1; // rssi too low
            } else {
                err_code = 0xC4E3; // AP connect fail(Authentication fail or Association fail or AP exeption)
            }
            // should set all ap info to err msg
        } else {
            err_code = 0xC4E0; // AP not found
        }

        if (g_ble_state){
            uint8_t ble_rsp[DEV_ERRCODE_MSG_MAX_LEN + 8] = {0};
            uint8_t ble_rsp_idx = 0;
            ble_rsp[ble_rsp_idx++] = 0x01;                          // Notify Code Type
            ble_rsp[ble_rsp_idx++] = 0x01;                          // Notify Code Length
            ble_rsp[ble_rsp_idx++] = 0x02;                          // Notify Code Value, 0x02-fail
            ble_rsp[ble_rsp_idx++] = 0x03;                          // Notify SubErrcode Type
            ble_rsp[ble_rsp_idx++] = sizeof(err_code);              // Notify SubErrcode Length
            memcpy(ble_rsp + ble_rsp_idx, (uint8_t *)&err_code, sizeof(err_code));  // Notify SubErrcode Value
            ble_rsp_idx += sizeof(err_code);
            breeze_post(ble_rsp, ble_rsp_idx);
        }
    } 
#endif
}

void combo_ap_conn_notify(void)
{
    uint8_t rsp[] = {0x01, 0x01, 0x01};
    if (g_ble_state) {
        breeze_post(rsp, sizeof(rsp));
    }
}

void combo_token_report_notify(void)
{
    uint8_t rsp[] = { 0x01, 0x01, 0x03 };
    if (g_ble_state) {
        breeze_post(rsp, sizeof(rsp));
        g_disconn_flag = 1;
    }
}

void combo_evt_tsk(void *args)
{
    int event_id = (int)args;

    if (event_id == EVENT_SLBLE_AP_INFO) {
        awss_running = 1;
        combo_connect_ap(&apinfo);
    } else if (event_id == EVENT_SLBLE_RESTART_ADV) {
        if (g_disconn_flag == 0) {
#if 1
            // FIXME: reboot to restart provision
            int wifi_prov_method = WIFI_PROVISION_SL_BLE;
            aos_kv_setint("wprov_method", wifi_prov_method);

            aos_kv_del("AUTH_AC_AS");
            aos_kv_del("AUTH_KEY_PAIRS");
            app_sys_set_boot_reason(BOOT_REASON_WIFI_CONFIG);
            aos_reboot();
#else
            printf("restart breeze advertising\n");
            breeze_stop_advertising();
            aos_msleep(500); // wait for adv stop
            breeze_start_advertising(g_combo_bind.bind_state, COMBO_AWSS_NEED);
#endif
        }
    } else {
        LOG("Combo event id err %d", event_id);
    }
}

static void combo_net_event_cb(uint32_t event_id, const void *param, void *context)
{
    aos_task_new("combo_evt", combo_evt_tsk, event_id, 2048);
}

static int reverse_byte_array(uint8_t *byte_array, int byte_len) {
    uint8_t temp = 0;
    int i = 0;
    if (!byte_array) {
        LOG("reverse_mac invalid params!");
        return -1;
    }
    for (i = 0; i < (byte_len / 2); i++) {
        temp = byte_array[i];
        byte_array[i] = byte_array[byte_len - i - 1];
        byte_array[byte_len - i - 1] = temp;
    }
    return 0;
}
//获取五元组
int hlk_getDevInfo()
{
    if(g_hlk_devinfo.magic != ALINKDEV_MAGIC)
    {
        memset(&g_hlk_devinfo, 0, sizeof(g_hlk_devinfo));
#if 1
        uint32_t addr = 0;
        //int r=hlk_alikey_read(&g_hlk_devinfo, sizeof(g_hlk_devinfo));
        //printf("r is %d\r\n",r);
		if(0 == hlk_alikey_read(&g_hlk_devinfo, sizeof(g_hlk_devinfo)))
        {
            printf("2222222222222222\r\n");
            printf("g_hlk_devinfo.magic is %d",g_hlk_devinfo.magic);
            printf("ALINKDEV_MAGIC is %d",ALINKDEV_MAGIC);
            printf("g_hlk_devinfo.productKey[0] is %s %s %s %s",g_hlk_devinfo.productKey[0],g_hlk_devinfo.productSecret[0],g_hlk_devinfo.deviceName[0],g_hlk_devinfo.deviceSecret[0]);
            if(g_hlk_devinfo.magic==ALINKDEV_MAGIC && g_hlk_devinfo.productKey[0] && g_hlk_devinfo.productSecret[0] && g_hlk_devinfo.deviceName[0] && g_hlk_devinfo.deviceSecret[0])
            {
                printf("============>hlk_getDevInfo ok, %s, %s\r\n", g_hlk_devinfo.productKey, g_hlk_devinfo.deviceName);
                return 0;                    
            }
        }
        memset(&g_hlk_devinfo, 0, sizeof(g_hlk_devinfo));
        printf("============>hlk_getDevInfo Err!\r\n");
        return -1;
#else
#if 0
#define PRODUCT_KEY      "a152gAOy2nJ"
#define PRODUCT_SECRET   "9UvNqCVIQWAcAQKp"
#define DEVICE_NAME      "UART2"
#define DEVICE_SECRET    "d894f6a498c99b05ba6a0c37217161a7"

#define PRODUCT_ID     	6019461
#else
#define PRODUCT_KEY      "a1I2kRDUJLD"
#define PRODUCT_SECRET   "dsNwl8tpFkTdqUcn"
#define DEVICE_NAME      "B36_SW16_001"
#define DEVICE_SECRET    "0e841f273f51b414814fbee2efd95092"

#define PRODUCT_ID     	5775633
#endif
        {
            strcpy(g_hlk_devinfo.productKey, PRODUCT_KEY);
            strcpy(g_hlk_devinfo.productSecret, PRODUCT_SECRET);
            strcpy(g_hlk_devinfo.deviceName, DEVICE_NAME);
            strcpy(g_hlk_devinfo.deviceSecret, DEVICE_SECRET);
            g_hlk_devinfo.magic = ALINKDEV_MAGIC;
			g_hlk_devinfo.productId = PRODUCT_ID;
        }
#endif
    }

    return 0;
}
int get_MAC(char *mac)
{
#if 0
	static aos_dev_t *wifi_dev = NULL;
    if (wifi_dev == NULL) {
        wifi_dev = device_open_id("wifi", 0);
    }
	hal_wifi_get_mac_addr(wifi_dev, mac);
#else	
	tls_get_mac_addr(mac);
#endif
}
void print_hex(unsigned char  *p, int len)
{
	int i;
    printf("\r\n**********************\r\n");
	for(i=0; i<len; i++){

        if(i && i%8==0){
            printf("\r\n");
        }
		printf("%02x ",p[i]);
	}
	printf("\r\n**********************\r\n");
}


static partition_t hlk_alikey_partition = 0;
int hlk_alikey_init()//五元组的flash内存空间
{
    hlk_alikey_partition = partition_open("alikey");
    //printf("77777777777777777777777hlk_alikey_partition is %d!!!!!!!!!!\r\n",hlk_alikey_partition);
    if (hlk_alikey_partition >= 0) {
        partition_info_t *lp = hal_flash_get_info(hlk_alikey_partition);
        aos_assert(lp);

        return 0;
    }

    return -1;
}
int hlk_alikey_write(char *data, int size)
{
    if (hlk_alikey_partition >= 0) {
		if(partition_erase(hlk_alikey_partition, 0, 1)==0){
			return partition_write(hlk_alikey_partition, 0, data, size);
		}
    }
    
    return -1;
}
int hlk_alikey_read(char *data, int size)
{
    if (hlk_alikey_partition >= 0) {
        return partition_read(hlk_alikey_partition, 0, data, size);
    }

    return -1;
}



//检测五元组进入配网
int combo_net_init(wifi_prov_cb cb)
{
    #if 0
    breeze_dev_info_t dinfo = { 0 };
    hlk_alikey_init();
    //printf("555555555  combo_net_init\r\n");
    if(hlk_getDevInfo() == 0)
	{
        printf("6666666 read right\r\n");
		HAL_SetProductKey(g_hlk_devinfo.productKey);
		HAL_SetProductSecret(g_hlk_devinfo.productSecret);
		HAL_SetDeviceName(g_hlk_devinfo.deviceName);
		HAL_SetDeviceSecret(g_hlk_devinfo.deviceSecret);
		HAL_SetProductId(g_hlk_devinfo.productId);
	}
    else{
        LOG("missing pid!");
        return -1;
    }

    prov_cb = cb;

    // aos_register_event_filter(EV_BZ_COMBO, combo_service_evt_handler, NULL);
    event_subscribe(EVENT_SLBLE_AP_INFO, combo_net_event_cb, NULL);
    event_subscribe(EVENT_SLBLE_RESTART_ADV, combo_net_event_cb, NULL);

	g_bind_state = breeze_get_bind_state();
    if ((strlen(g_hlk_devinfo.productKey) > 0) && (strlen(g_hlk_devinfo.productSecret) > 0)
            && (strlen(g_hlk_devinfo.deviceName) > 0) && (strlen(g_hlk_devinfo.deviceSecret) > 0) && g_hlk_devinfo.productId > 0) {
        printf("777777777777 net\r\n ");
        uint8_t combo_adv_mac[6] = {0};
        tls_get_mac_addr(combo_adv_mac);
        // Set wifi mac to breeze awss adv, to notify app this is a wifi dev
        // Awss use wifi module device info.
        // Only use BT mac when use breeze and other bluetooth communication functions
        reverse_byte_array(combo_adv_mac, 6);
        dinfo.product_id = g_hlk_devinfo.productId;
        dinfo.product_key = g_hlk_devinfo.productKey;
        dinfo.product_secret = g_hlk_devinfo.productSecret;
        dinfo.device_name = g_hlk_devinfo.deviceName;
        dinfo.device_secret = g_hlk_devinfo.deviceSecret;
        dinfo.dev_adv_mac = combo_adv_mac;
        breeze_awss_init(&dinfo,
                         combo_status_change_cb,
                         combo_set_dev_status_handler,
                         combo_get_dev_status_handler,
                         combo_apinfo_rx_cb,
                         NULL);
        breeze_awss_start();
        iotx_event_regist_cb(linkkit_event_monitor);
        breeze_start_advertising(g_combo_bind.bind_state, COMBO_AWSS_NEED);
    } else {
        LOG("combo device info not set!");
    }
    return 0;
    #endif
    breeze_dev_info_t dinfo = { 0 };
    HAL_GetProductKey(g_combo_pk);
    HAL_GetDeviceName(g_combo_dn);
    HAL_GetProductSecret(g_combo_ps);
    HAL_GetDeviceSecret(g_combo_ds);
    
    prov_cb = cb;
    
    //HAL_GetPartnerID(&g_combo_pid);
    aos_kv_getint("hal_devinfo_pid", &g_combo_pid);
    printf("99999  %s %s %s %s  %d \r\n",g_combo_pk,g_combo_dn,g_combo_ps,g_combo_ds,g_combo_pid);
  
    if (aos_kv_getint("hal_devinfo_pid", &g_combo_pid)) {
        LOG("missing pid!");
        return -1;
    }
    printf("777  %s %s %s %s  %d \r\n",g_combo_pk,g_combo_dn,g_combo_ps,g_combo_ds,g_combo_pid);

    // aos_register_event_filter(EV_BZ_COMBO, combo_service_evt_handler, NULL);
    event_subscribe(EVENT_SLBLE_AP_INFO, combo_net_event_cb, NULL);
    event_subscribe(EVENT_SLBLE_RESTART_ADV, combo_net_event_cb, NULL);

	g_bind_state = breeze_get_bind_state();
    if ((strlen(g_combo_pk) > 0) && (strlen(g_combo_ps) > 0)
            && (strlen(g_combo_dn) > 0) && (strlen(g_combo_ds) > 0) && g_combo_pid > 0) {
        //printf("1111111111111111111111111111111\r\n");
        uint8_t combo_adv_mac[6] = {0};
        tls_get_mac_addr(combo_adv_mac);
        // Set wifi mac to breeze awss adv, to notify app this is a wifi dev
        // Awss use wifi module device info.
        // Only use BT mac when use breeze and other bluetooth communication functions
        reverse_byte_array(combo_adv_mac, 6);
        dinfo.product_id = g_combo_pid;
        dinfo.product_key = g_combo_pk;
        dinfo.product_secret = g_combo_ps;
        dinfo.device_name = g_combo_dn;
        dinfo.device_secret = g_combo_ds;
        dinfo.dev_adv_mac = combo_adv_mac;
        breeze_awss_init(&dinfo,
                         combo_status_change_cb,
                         combo_set_dev_status_handler,
                         combo_get_dev_status_handler,
                         combo_apinfo_rx_cb,
                         NULL);
        breeze_awss_start();
        iotx_event_regist_cb(linkkit_event_monitor);
        breeze_start_advertising(g_combo_bind.bind_state, COMBO_AWSS_NEED);
    } else {
        LOG("combo device info not set!");
    }
    return 0;
}
/**
*************************
* 透传数到串口
*************************
*/
int u2c_uart_send(char *const data, int len)
{
	if(hlk_uart_handle){
		return uart_send(hlk_uart_handle, data, len);
	}
	return -1;
}

/**
*************************
* 从串口读取数据
*************************
*/

extern aos_dev_t *g_console_handle;
int u2c_uart_read(char *const data, int len)
{
	// if(hlk_uart_handle){
	// 	return uart_recv(hlk_uart_handle, data, len, 2);
	// }
    if (g_console_handle != NULL) {
        return uart_recv(g_console_handle, data, len, 2);
    }
	return -1;
}
/**
*************************
* 格式化数据到串口
*************************
*/
int u2c_uart_printf(const char *fmt, ...)
{
    int rc;      // return code
    char buf[128] = {0};
    va_list ap;  // for variable args

    va_start(ap, fmt); // init specifying last non-var arg
    rc = vsprintf(buf, fmt, ap);
    va_end(ap); // end var args

    if(rc > 0){
        u2c_uart_send(buf, rc);
    }
    return rc;
} 

/*
#define RESP_OK()              u2c_uart_printf("Ok\r\n")
#define RESP_OK_EQU(fmt, args...)   u2c_uart_printf(fmt, ##args)

//#define RESP_OK_EQU  u2c_uart_printf
#define RESP_ERROR(err_type)   u2c_uart_printf("ERROR\r\n")
*/

#define RESP_OK()                   u2c_loguart_printf("Ok\r\n")
#define RESP_OK_EQU(fmt, args...)   u2c_loguart_printf(fmt, ##args)

//#define RESP_OK_EQU  u2c_uart_printf
#define RESP_ERROR(err_type)    u2c_loguart_printf("ERROR\r\n")

/**
*************************
*************************
*/
int u2c_loguart_printf(const char *fmt, ...)
{
    int rc;         // return code
    char buf[128] = {0};
    va_list ap;     // for variable args

    va_start(ap, fmt);  // init specifying last non-var arg
    rc = vsprintf(buf, fmt, ap);
    va_end(ap); // end var args


    if(rc > 0){
        printf("%s",buf);
    }
    return rc;
}


#ifdef __cplusplus
}
#endif

#define CMD_MAXARGS      7

typedef struct cmd_tbl_s {
    char    *name;                                     /* Command Name */
    int     maxargs;                                   /* maximum number of arguments */
    int     (*cmd)(struct cmd_tbl_s *, int, char *[]);
    char    *usage;                                    /* Usage message(short)*/
} cmd_tbl_t;

static int do_at( cmd_tbl_t *cmd, int argc, char *argv[]);
static int do_Get_MAC( cmd_tbl_t *cmd, int argc, char *argv[]);
static int do_alinkdev_set( cmd_tbl_t *cmd, int argc, char *argv[]);
static int do_fota_set( cmd_tbl_t *cmd, int argc, char *argv[]);
static int do_mac_set( cmd_tbl_t *cmd, int argc, char *argv[]);
static int do_wscan( cmd_tbl_t *cmd, int argc, char *argv[]);
extern void do_wscan_handler(char *cmd, int type, char *data);
static int do_hlkver( cmd_tbl_t *cmd, int argc, char *argv[]);

static cmd_tbl_t cmd_list[] = {
    {
        "AT",               1,   do_at,
        "AT                 - AT"
    },
    {
        "AT+VER",           2,   do_hlkver,
        "AT+VER             - get ver"
    },
    {
        "at+Get_MAC",           2,   do_Get_MAC,
        "at+Get_MAC             - get mac"
    },
    {
        "at+devset",        8,   do_alinkdev_set,
        "at+devset          - set deveic info"
    },
    {
        "at+otaset",        8,   do_fota_set,
        "at+otaset          - set fota info"
    },
    {
        "at+macset",        8,   do_mac_set,
        "at+macset          - set fota info"
    },
    {
        "at+wscan",        8,   do_wscan_handler,
        "at+wscan          - set fota info"
    },
};
    
static unsigned int cmd_cntr = sizeof(cmd_list)/sizeof(cmd_tbl_t);


#define CMD_CBSIZE   256                                //128 //180815 NiuJG ?????闂佽法鍠愰弸濠氬箯缁屾攨y闂佽法鍠愰弸濠氬箯閿燂拷?????230??????
static char console_buffer[CMD_CBSIZE];
static char lastcommand[CMD_CBSIZE] = { 0, };

static int do_at( cmd_tbl_t *cmd, int argc, char *argv[])
{
    RESP_OK();
    return 0;
}
static int do_hlkver( cmd_tbl_t *cmd, int argc, char *argv[])
{
    RESP_OK_EQU("%s\r\n", HLK_VER);
    return 0;
}


static int do_Get_MAC( cmd_tbl_t *cmd, int argc, char *argv[])
{
    uint8_t  mac[6] = {0};
    if(*argv[1] == '?'){
        get_MAC(mac);
        RESP_OK_EQU("%02x:%02x:%02x:%02x:%02x:%02x\r\n",mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return 0;
    }
    RESP_ERROR(ERROR_ARG);
    return 0;
}

/*
************************************************************************
* 查询返回：产品key，设备名
* 设置参数：keying，产品key，产品SECRET，设备名，设备SECRET
************************************************************************
*/
static int do_alinkdev_set( cmd_tbl_t *cmd, int argc, char *argv[])
{
    //printf("11111111111 into alidev set!\r\n");
    //printf("argc is %d\r\n",argc);
    char keyimg[KEYIMG_LEN] = {0};
    uint32_t addr = 0;
   // ALINKDEV_t alinkDev;

    // printf("=====>do_alinkdev_set start\r\n");
#if 0
    if(*argv[1] == '?')
    {
        //printf("???????????????????\r\n");
        addr = 0;
        sizeof(&alinkDev, 0, sizeof(alinkDev));
        if(0 == hlk_alikey_read(&alinkDev, sizeof(alinkDev)))
        {
            if(alinkDev.magic != ALINKDEV_MAGIC)
            {
                RESP_OK_EQU("Err\r\n");
            }
            else
            {
                RESP_OK_EQU("%s,%s\r\n",alinkDev.productKey, alinkDev.deviceName);
            }
        }else{

            RESP_OK_EQU("Read Err\r\n");
        }
        return 0;
    }
#endif
    if(*argv[1] == '?')
    {
        /*HAL_GetProductKey(g_combo_pk);
        HAL_GetDeviceName(g_combo_dn);
        HAL_GetProductSecret(g_combo_ps);
        HAL_GetDeviceSecret(g_combo_ds);*/
        printf("11111 g_combo_pid is %d",g_combo_pid);
        if(magic != ALINKDEV_MAGIC)
        {
            RESP_OK_EQU("Error\r\n");
        }
        else
        {
            RESP_OK_EQU("%s,%s,%s,%s,%d\r\n",g_combo_pk, g_combo_dn,g_combo_ps,g_combo_ds,g_combo_pid);
        }
        //RESP_OK_EQU("%s,%s\r\n",g_combo_pk, g_combo_dn);
        return 0;
    }
    
    
    if (argc != 8 || !argv[1] || !argv[2] || !argv[3] || !argv[4] || !argv[5] || !argv[6] || !argv[7]) 
    {
        //printf("return!!!!!!!!!!!!!!!!!!!!\r\n");
        RESP_ERROR(ERROR_ARG);
        return 0;
    }
    // printf("=====>do_alinkdev_set h_keyCheck 1\r\n");
    if (strlen(argv[2])>PRODUCT_KEY_MAXLEN || strlen(argv[3])>DEVICE_SECRET_MAXLEN || strlen(argv[4])>DEVICE_NAME_MAXLEN 
      || strlen(argv[5])>DEVICE_SECRET_MAXLEN || strlen(argv[6])>9) 
    {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }
    // printf("=====>do_alinkdev_set h_keyCheck 2\r\n");
    hexstr2u8(keyimg, argv[1], KEYIMG_LEN*2);
    // print_hex(keyimg, KEYIMG_LEN);
    if(h_keyCheck(keyimg)==0)
    {
        unsigned int sum = chk_sum(argv[2], strlen(argv[2])) + chk_sum(argv[3], strlen(argv[3])) 
                          + chk_sum(argv[4], strlen(argv[4])) + chk_sum(argv[5], strlen(argv[5])) + chk_sum(argv[6], strlen(argv[6]));
        sum &= 0xFFFF;

        if(sum == hex2i(argv[7]))
        {
            //memset(&alinkDev, 0, sizeof(alinkDev));

            magic = ALINKDEV_MAGIC;
            strcpy(g_combo_pk, argv[2]);
            strcpy(g_combo_ps, argv[3]);
            strcpy(g_combo_dn, argv[4]);
            strcpy(g_combo_ds, argv[5]);
            g_combo_pid = atoi(argv[6]);    
            addr = 0;
            //printf("############\r\n");
            //int r=hlk_alikey_write(&alinkDev, sizeof(alinkDev));
            //printf("r is %d !!!!!!!!!!!!!\r\n",r);
            //printf("%d %d %s %s %s %s %d",ALINKDEV_MAGIC,magic,alinkDev.productKey,alinkDev.deviceName,alinkDev.deviceSecret,alinkDev.productId);
			/*if(0 == hlk_alikey_write(&alinkDev, sizeof(alinkDev)))
			{
                printf("write!!!!!!!!!!!!!!!!\r\n");
				RESP_OK_EQU("Ok\r\n");
				//state_gpio_output(LED_STATE_ON);  
				//while(1);
				return 0;
			}  */
            
            HAL_SetProductKey(g_combo_pk);
		    HAL_SetProductSecret(g_combo_ps);
		    HAL_SetDeviceName(g_combo_dn);
		    HAL_SetDeviceSecret(g_combo_ds);
		    //HAL_SetProductId(g_combo_pid);

            char *key = "hal_devinfo_pid";   
            int ret = aos_kv_setint(key, g_combo_pid);
            // printf("+++++++ g_combo_pid %d ret %d \r\n",g_combo_pid,ret);
            // aos_kv_setint("start_en", 1);

            RESP_OK_EQU("Ok\r\n");
            return 0;
        }
        RESP_OK_EQU("Verify  Err\r\n");
        printf("====>sum=%d, %d\r\n", sum, hex2i(argv[7]));
        return 0;
    }
    printf("=====>do_alinkdev_set h_keyCheck err\r\n");
    RESP_ERROR(ERROR_ARG);
    return 0;

}
static int do_fota_set( cmd_tbl_t *cmd, int argc, char *argv[])
{
    if(*argv[1] == '?')
    {
        /*HAL_GetProductKey(g_combo_pk);
        HAL_GetDeviceName(g_combo_dn);
        HAL_GetProductSecret(g_combo_ps);
        HAL_GetDeviceSecret(g_combo_ds);*/
        printf("22222 ota_flag is %d",ota_flag);
        if(ota_flag == 0)
        {
            RESP_OK_EQU("Error\r\n");
        }
        else
        {
            RESP_OK_EQU("%s,%s,%s\r\n",g_fota_model, g_fota_id,g_fota_otaurl);
        }
        //RESP_OK_EQU("%s,%s\r\n",g_combo_pk, g_combo_dn);
        return 0;
    }
    // printf("argv[1]=%s || !argv[2]=%s || !argv[3]=%s\n",argv[1],argv[2],argv[3]);
    if (!argv[1] || !argv[2] || !argv[3]) 
    {
        printf("return!!!!!!!!!!!!!!!!!!!!\r\n");
        RESP_ERROR(ERROR_ARG);
        return 0;
    }
    if (strlen(argv[1])>FOTA_MODEL_MAXLEN || strlen(argv[2])>FOTA_ID_MAXLEN || strlen(argv[3])>FOTA_OTAURL_MAXLEN) 
    {
        printf("return 22222222222222222222\r\n");
        RESP_ERROR(ERROR_ARG);
        return 0;
    }
    strcpy(g_fota_model, argv[1]);
    strcpy(g_fota_id, argv[2]);
    strcpy(g_fota_otaurl, argv[3]);
    char *model = "model"; 
    char *id = "device_id"; 
    char *url = "otaurl";
    char *value1 = g_fota_model; 
    char *value2 = g_fota_id;
    char *value3 = g_fota_otaurl; 
    aos_kv_setstring(model, value1);
    aos_kv_setstring(id, value2);
    aos_kv_setstring(url, value3);
    ota_flag=1;
    // printf("+++++++ %s %s %s\r\n",g_fota_model,g_fota_id,g_fota_otaurl);
    RESP_OK_EQU("Ok\r\n");
    return 0;
}
static int strmacaddr(char *str, uint8_t mac[6])
{
    char *p = str;
    for (int i = 0; i < 6; ++i) {
        uint8_t t = 0;
        for (int j = 0; j < 2; ++j) {
            t <<= 4;
            if (*p >= '0' && *p <= '9') {
                t += *p - '0';
            } else if (*p >= 'a' && *p <= 'f') {
                t += *p - 'a' + 10;
            } else if (*p >= 'A' && *p <= 'F') {
                t += *p - 'A' + 10;
            } else {
                return -1;
            }
            ++p;
        }
        mac[i] = t;
    }

    return 0;
}
static int do_mac_set( cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret;
    uint8_t mac[6];
    if (strlen(argv[1])==12)
    {
        ret = strmacaddr(argv[1], mac);
        //printf("1111111111111ret is %d\r\n",ret);
        if(ret == 0){
            printf("mac addr %02x%02x%02x%02x%02x%02x\r\n", mac[0], mac[1], mac[2],
                        mac[3], mac[4], mac[5]);
            if (!tls_set_mac_addr(mac)) {
                RESP_OK_EQU("Ok\r\n");
            } else {
                RESP_OK_EQU("Error\r\n");
            }
        }
    }
    return 0;
}
static int do_wscan( cmd_tbl_t *cmd, int argc, char *argv[])
{
   
    return 0;
}
static int parse_line (char *line, char *argv[])
{
    int nargs = 0;

    while (nargs <= CMD_MAXARGS) {
        /* skip any white space */
        while ((*line == ' ') || (*line == '\t') || (*line == ',')) {
            ++line;
        }

        if (*line == '\0') {    /* end of line, no more args    */
            argv[nargs] = 0;
            return (nargs);
        }

        /* Argument include space should be bracketed by quotation mark */
        if(*line == '\"') {
            /* Skip quotation mark */
            line++;

            /* Begin of argument string */
            argv[nargs++] = line;

            /* Until end of argument */
            while(*line && (*line != '\"')) {
                ++line;
            }
        } else {
            argv[nargs++] = line;    /* begin of argument string    */

            /* find end of string */
            while(*line && (*line != ',') && (*line != '=')) {
                ++line;
            }
        }
        /** miaodefang modified for RDA5991 VerG End */

        if (*line == '\0') {    /* end of line, no more args    */
            argv[nargs] = 0;
            return (nargs);
        }

        *line++ = '\0';         /* terminate current arg     */
    }

    return (nargs);
}
static cmd_tbl_t *find_cmd (const char *cmd)
{
    //printf("777777777777777777 find cmd!!!\r\n");
    cmd_tbl_t *cmdtp;
    cmd_tbl_t *cmdtp_temp = &cmd_list[0];    /* Init value */
    uint32_t len;
    int n_found = 0;
    int i;
        
    len = strlen(cmd);

    for (i = 0;i < (int)cmd_cntr;i++) {
        cmdtp = &cmd_list[i];
        if (strncmp(cmd, cmdtp->name, len) == 0) {
            if (len == strlen(cmdtp->name))
                return cmdtp;      /* full match */

            cmdtp_temp = cmdtp;    /* abbreviated command ? */
            n_found++;
        }
    }
    if (n_found == 1) {  /* exactly one match */
        return cmdtp_temp;
    }

    return 0;   /* not found or ambiguous command */
}

int run_command(char *cmd)
{
    //printf("000000000000 run command!!1111111\r\n");
    cmd_tbl_t *cmdtp;
    char *argv[CMD_MAXARGS + 1];    /* NULL terminated    */
    int argc;

    // printf("=====>run_command:%s\r\n", cmd);
    /* Extract arguments */
    if ((argc = parse_line(cmd, argv)) == 0) {
        return -1;
    }
    // printf("=====>run_command:%s,argc=%d\r\n", argv[0], argc);
    /* Look up command in command table */
    if ((cmdtp = find_cmd(argv[0])) == 0) {           
        RESP_ERROR();
        return -1;
    }

    /* found - check max args */
    if (argc > cmdtp->maxargs) {          
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    /* OK - call function to do the command */
    if ((cmdtp->cmd) (cmdtp, argc, argv) != 0) {
        return -1;
    }

    return 0;
}

int handle_char(const char c, char *prompt) {
    static char   *p   = console_buffer;
    static int    n    = 0;              /* buffer index        */
    static int    plen = 0;           /* prompt length    */
    static int    col;                /* output column cnt    */

    if (prompt) {
        plen = strlen(prompt);
        if(plen == 1 && prompt[0] == 'r')
            plen = 0;
        else
            ;
        p = console_buffer;
        n = 0;
        return 0;
    }
    col = plen;

    /* Special character handling */
    switch (c) {
        case '\r':                /* Enter        */
        case '\n':
            *p = '\0';
            return (p - console_buffer);

        case '\0':                /* nul            */
            return -1;
    
        default:
         /*  Must be a normal character then  */
            if (n < CMD_CBSIZE-2) {
        
                *p++ = c;
                ++n;
            } else {          /* Buffer full        */

            }
    }

    return -1;
}

#define U2W_UART_BUF_SZ  (CLOUD_PACKGE_LEN_MAX/2)
static void uart_monitor()
{
	int	ret, idx, nullCnt;
    ULONG timeStamp_work;
    ULONG timeStamp_idle;
	ULONG now = 0; 
	int size = 0;
    
	uint8_t pins[] = {16, 7}; // {tx_pin, rx_pin}
	/*extern uart_dev_t uart_1 ;
    uart_1.port = 1;
    uart_1.priv = pins;
    uart_1.config.baud_rate = pU2WCfg->baudrate;        
    hal_uart_init(&uart_1);
	*/
	uart_config_t config;
	
	uart_config_default(&config);
	//config.baud_rate =  pU2WCfg->baudrate;
	config.baud_rate =  115200;
	uart_config(hlk_uart_handle, &config);
	

    //aos_msleep(2000);

    // int ch ;
    idx = 0;
    timeStamp_work = 0;
    timeStamp_idle = 0;
    nullCnt = 0;
    printf("===>uart_monitor loop start\r\n");
	//u2c_uart_printf("===>uart_monitor loop start\r\n");
	while (1) {
            unsigned char ch;
            int len;
            ret = u2c_uart_read(&ch, 1);
            // ch = fgetc(NULL);
            // if(ch) ret=1;
            // printf("rec %c  ret :%d \r\n",ch,ret);
            if(ret > 0){
                //printf("66666666666666 %c", ch);
                timeStamp_idle = 0;
                len = handle_char(ch, 0);
                if (len >= 0) {
                    strcpy(lastcommand, console_buffer);

                    if (len > 0) {
                        if(run_command(lastcommand) < 0) {
                            printf("command fail\r\n");
                        }
                    }
                    handle_char('0', "r");//'r' means reset
                }
            } 
            else
            {
                if(nullCnt<10)                              //防抖
                {
                    nullCnt++;
                    continue; 
                }
                if(timeStamp_idle == 0)
                {
                    nullCnt = 0;
                    timeStamp_idle = aos_now_ms();
                }
                if(++nullCnt > 200)
                {
                    //aos_msleep(5);
                    nullCnt = 0;
                    if(aos_now_ms()-timeStamp_idle > 30*1000)
                    {
                        //u2c_uart_printf("idle timeout... %d ,%d\r\n",timeStamp_idle, aos_now_ms());
                        timeStamp_idle = 0;
                        //hlk_runmode(RUNMODE_U2C);
                        //state_gpio_output(LED_STATE_OFF);
                        //aos_reboot();
                    }
                }
                aos_msleep(5);
            }
            continue;
        }
		aos_msleep(10);
}

void hlk_u2c_app_start()
{
    int ret;
    aos_task_t task;
	
	hlk_uart_handle = uart_open("uart1");
    ret = aos_task_new("hlk_u2c", uart_monitor, 0, 2048*2);
}



int combo_net_deinit()
{
    breeze_awss_stop();
    return 0;
}

uint8_t combo_ble_conn_state(void)
{
    return g_ble_state;
}

uint8_t combo_get_bind_state(void)
{
    return 0;
}

uint8_t combo_get_ap_state(void)
{
    netmgr_hdl_t hdl = netmgr_get_handle("wifi");
    if (hdl) {
        return netmgr_is_gotip(hdl);
    }
    return 0;
}

uint8_t allow_log=1;
void SetLog_flag(uint8_t sw)
{
    allow_log = sw;
}

uint8_t GetLog_flag()
{
    return allow_log;
}

uint8_t check_pid_effective()
{
    HAL_GetProductKey(g_combo_pk);
    HAL_GetDeviceName(g_combo_dn);
    HAL_GetProductSecret(g_combo_ps);
    HAL_GetDeviceSecret(g_combo_ds);

    aos_kv_getint("hal_devinfo_pid", &g_combo_pid);
    // printf("99999  %s %s %s %s  %d \r\n",g_combo_pk,g_combo_dn,g_combo_ps,g_combo_ds,g_combo_pid);
  
    if (aos_kv_getint("hal_devinfo_pid", &g_combo_pid)) {
        LOG("missing pid!");
        // printf("----------------------     not --------_______");
        SetLog_flag(0);
        return -1;
    }
}

#endif
