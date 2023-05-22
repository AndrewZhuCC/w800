/*
 */

#include <yoc/atserver.h>
#include <at_cmd.h>
#include <wifi_provisioning.h>
#include <smartliving/exports/iot_export_linkkit.h>
#include <cJSON.h>
#include "litepoint.h"
#include "app_main.h"
#include "uni_wifi.h"
#include "wifi.h"
#include "uni_efuse.h"
#include "../app_sys.h"

#define TAG "app"

typedef enum {
    APP_AT_CMD_IWS_START,
    APP_AT_CMD_WJAP,
} APP_AT_CMD_TYPE;

typedef struct at_aui_info {
    int micEn;
    cJSON *js_info;
} at_aui_info_t;

static int g_at_init = 0;
static at_aui_info_t g_at_aui_ctx = {1, NULL};

#define AT_RESP_CME_ERR(errno)    atserver_send("\r\n+CME ERROR:%d\r\n", errno)

void event_publish_app_at_cmd(APP_AT_CMD_TYPE type)
{
    event_publish(EVENT_APP_AT_CMD, (void *)type);
}

/* remove double quote in first character and last character */
static char *remove_double_quote(char *str)
{
    char *  str_new_p = str;
    uint8_t len       = strlen(str);
    if ('\"' == *str) {
        str_new_p = str + 1;
    }
    if ('\"' == str[len - 1]) {
        str[len - 1] = '\0';
    }
    return str_new_p;
}

/* calculate number of '"', not include '\"' */
static uint8_t cal_quote_num(char *str)
{
    uint8_t cnt = 0;
    uint8_t last_slash = 0;
    while('\0' != *str) {
        if('\\' == *str) {
            last_slash = 1;
            str++;
            continue;
        }
        if('\"' == *str && 0 == last_slash) {
            cnt++;
        }
        str++;
        last_slash = 0;
    }
    return cnt;
}

/* replace '"' with '\"',  don't touch ogirinal '\"' */
static void add_slash_to_quote(const char *oldStr, char *newStr)
{
    uint8_t last_slash = 0;
    while('\0' != *oldStr) {
        if('\\' == *oldStr) {
            last_slash = 1;
            *newStr = *oldStr;
        } else if ('\"' == *oldStr && 0 == last_slash) {
            *newStr++ = '\\';
            *newStr = '\"';
            last_slash = 0;
        } else {
            *newStr = *oldStr;
            last_slash = 0;
        }
        oldStr++;
        newStr++;
    }
    *newStr = '\0';
}

static void convert_json_and_send(char *cmd, cJSON *js_info)
{
    if(NULL == cmd || NULL == js_info) {
        return;
    }

    char *str_js = cJSON_PrintUnformatted(g_at_aui_ctx.js_info);
    uint8_t quote_num = cal_quote_num(str_js);
    char new_str_js[strlen(str_js) + quote_num + 1];
    add_slash_to_quote(str_js, new_str_js);

    atserver_send("%s:%s\r\n", cmd, new_str_js);
    AT_BACK_OK();
    free(str_js);
}

void iws_start_handler(char *cmd, int type, char *data)
{
    int input_data = 0;
    extern int wifi_prov_method;
    if (type == TEST_CMD) {
        AT_BACK_RET_OK(cmd, "\"type\"");
    } else if (type == READ_CMD) {
        AT_BACK_RET_OK_INT(cmd, wifi_prov_method);
    } else if (type == WRITE_CMD) {
        AT_BACK_OK();
        input_data = atoi(data);
        if ((input_data >= WIFI_PROVISION_MAX) || (input_data < WIFI_PROVISION_MIN)
            || (input_data == WIFI_PROVISION_SOFTAP)) {
            AT_RESP_CME_ERR(100);
        } else {
            wifi_prov_method = input_data;
            //event_publish_app_at_cmd(APP_AT_CMD_IWS_START);
            aos_kv_setint("wprov_method", wifi_prov_method);
            if (wifi_prov_method == WIFI_PROVISION_SL_BLE) {
                aos_kv_del("AUTH_AC_AS");
                aos_kv_del("AUTH_KEY_PAIRS");
            }
            app_sys_set_boot_reason(BOOT_REASON_WIFI_CONFIG);
            aos_reboot();
        }
    } else {
        AT_RESP_CME_ERR(100);
    }
}

void iws_stop_handler(char *cmd, int type, char *data) {
    if (type == TEST_CMD) {
        AT_BACK_OK();
    } else if (type == EXECUTE_CMD) {
        if (wifi_prov_get_status() != WIFI_PROV_STOPED) {
            wifi_prov_stop();
        }
        AT_BACK_OK();
    }
}

void idm_au_handler(char *cmd, int type, char *data) {
		uint32_t pid = 0;
    if (type == WRITE_CMD) {
        // "PK","DN","DS","PS","PID"
        if (*data != '\"'
            || strlen(data) > PRODUCT_KEY_LEN + DEVICE_NAME_LEN
                              + DEVICE_SECRET_LEN + PRODUCT_SECRET_LEN
														  + PRODUCT_ID_LEN + 14) {
            AT_RESP_CME_ERR(50);
            return;
        }
        char *buffer[5];
        char *start = ++data;
        char *token = NULL;

        // PK
        token = strstr(start, "\",\"");
        if (!token || token - start > PRODUCT_KEY_LEN) {
            AT_RESP_CME_ERR(50);
            return;
        }
        *token = '\0';
        buffer[0] = start;

        // DN
        start = token + 3;
        token = strstr(start, "\",\"");
        if (!token || token - start > DEVICE_NAME_LEN) {
            AT_RESP_CME_ERR(50);
            return;
        }
        *token = '\0';
        buffer[1] = start;

        // DS
        start = token + 3;
        token = strstr(start, "\",\"");
        if (!token || token - start > DEVICE_SECRET_LEN) {
            AT_RESP_CME_ERR(50);
            return;
        }
        *token = '\0';
        buffer[2] = start;

        // PS
        start = token + 3;
        token = strstr(start, "\",\"");
        if (!token || token - start > PRODUCT_SECRET_LEN) {
            AT_RESP_CME_ERR(50);
            return;
        }
        *token = '\0';
        buffer[3] = start;

        // PID
        start = token + 3;
        token = strstr(start, "\"");
        if (!token || token - start > PRODUCT_ID_LEN) {
            AT_RESP_CME_ERR(50);
            return;
        }
        *token = '\0';
        buffer[4] = start;

        HAL_SetProductKey(buffer[0]);
        HAL_SetDeviceName(buffer[1]);
        HAL_SetDeviceSecret(buffer[2]);
        HAL_SetProductSecret(buffer[3]);

        pid = atoi(buffer[4]);
        aos_kv_setint("hal_devinfo_pid", pid);

        AT_BACK_OK();
    } else if (type == READ_CMD) {
        char pk[PRODUCT_KEY_LEN + 1];
        char dn[DEVICE_NAME_LEN + 1];
        char ds[DEVICE_SECRET_LEN + 1];
        char ps[PRODUCT_SECRET_LEN + 1];
        HAL_GetProductKey(pk);
        HAL_GetDeviceName(dn);
        HAL_GetDeviceSecret(ds);
        HAL_GetProductSecret(ps);
    		aos_kv_getint("hal_devinfo_pid", &pid);
        atserver_send("OK+IDMAU:%s,%s,%s,%s,%d\r\n", pk, dn, ds, ps, pid);
    }
}
#if defined(EN_COMBO_NET) && (EN_COMBO_NET == 1)
void idm_pid_handler(char *cmd, int type, char *data)
{
    extern int g_combo_pid;
    if (type == TEST_CMD) {
        AT_BACK_RET_OK(cmd, "\"pid\"");
    } else if (type == READ_CMD) {
        aos_kv_getint("hal_devinfo_pid", &g_combo_pid);
        AT_BACK_RET_OK_INT(cmd, g_combo_pid);
    } else if (type == WRITE_CMD) {
        AT_BACK_OK();
        g_combo_pid = atoi(data);
        aos_kv_setint("hal_devinfo_pid", g_combo_pid);
    } else {
        AT_RESP_CME_ERR(100);
    }
}
#endif

void idm_con_handler(char *cmd, int type, char *data) {
    if (type == TEST_CMD) {
        AT_BACK_OK();
    } else if (type == EXECUTE_CMD) {
        smartliving_client_control(1);
        AT_BACK_OK();
    }
}

void idm_cls_handler(char *cmd, int type, char *data) {
    if (type == TEST_CMD) {
        AT_BACK_OK();
    } else if (type == EXECUTE_CMD) {
        smartliving_client_control(0);
        AT_BACK_OK();
    }
}

void idm_sta_handler(char *cmd, int type, char *data)
{
    if (type == TEST_CMD) {
        AT_BACK_OK();
    } else if (type == READ_CMD) {
        AT_BACK_RET_OK_INT(cmd, smartliving_client_is_connected() << 1);
    }
}

static void event_app_at_cmd_handler(uint32_t event_id, const void *param, void *context)
{
    if (event_id == EVENT_APP_AT_CMD) {
        APP_AT_CMD_TYPE type = (APP_AT_CMD_TYPE)param;
        switch (type) {
            case APP_AT_CMD_IWS_START:
                smartliving_client_control(0);
                wifi_pair_start();
                break;
            case APP_AT_CMD_WJAP:
                app_network_reinit();
                break;
            default:
                break;
        }
    }
}

void at_cmd_dev_info(char *cmd, int type, char *data)
{
    int ret;
    if (type == TEST_CMD) {
        AT_BACK_STR("+DEVINFO:\"str\"\r\n");
    } else if (type == READ_CMD) {
        char devInfo[256] = {0};
        ret                        = aos_kv_getstring("devInfo", devInfo, sizeof(devInfo));
        AT_BACK_RET_OK(cmd, devInfo);
    } else if (type == WRITE_CMD) {
        data = remove_double_quote(data);
        aos_kv_setstring("devInfo", data);
        AT_BACK_OK();
    }
}

void at_cmd_aui_cfg(char *cmd, int type, char *data)
{
    //int ret;
    if (type == TEST_CMD) {
        AT_BACK_STR("+AUICFG:\"per,vol,spd,pit\"\r\n");
    } else if (type == READ_CMD) {
        char auiCfg[32] = {0};
        aos_kv_getstring("auiCfg", auiCfg, sizeof(auiCfg));
        AT_BACK_RET_OK(cmd, auiCfg);
    } else if (type == WRITE_CMD) {
        data = remove_double_quote(data);
        aos_kv_setstring("auiCfg", data);
        AT_BACK_OK();
    }
}

void at_cmd_aui_fmt(char *cmd, int type, char *data)
{
    int auiFmt = 0;
    if (type == TEST_CMD) {
        AT_BACK_STR("+AUIFMT:<fmt>\r\n");
    } else if (type == READ_CMD) {
        aos_kv_getint("auiFmt", &auiFmt);
        AT_BACK_RET_OK_INT(cmd, auiFmt);
    } else if (type == WRITE_CMD) {
        data = remove_double_quote(data);
        auiFmt = atoi(data);
        aos_kv_setint("auiFmt", auiFmt);
        AT_BACK_OK();
    }
}

void at_cmd_aui_micen(char *cmd, int type, char *data)
{
#ifdef NEVER
    if (type == TEST_CMD) {
        AT_BACK_STR("+AUIMICEN:\"val\"\r\n");
    } else if (type == READ_CMD) {
        AT_BACK_RET_OK_INT(cmd, g_at_aui_ctx.micEn);
    } else if (type == WRITE_CMD) {
        data = remove_double_quote(data);
        g_at_aui_ctx.micEn = atoi(data);
        aui_mic_set_wake_enable(g_at_aui_ctx.micEn);
        AT_BACK_OK();
    }
#endif /* NEVER */
}

void at_cmd_wwv_en(char *cmd, int type, char *data)
{
    int auiWWVen = 0;
    if (type == TEST_CMD) {
        AT_BACK_STR("+AUIWWVEN:<en>\r\n");
    } else if (type == READ_CMD) {
        aos_kv_getint("auiWWVen", &auiWWVen);
        AT_BACK_RET_OK_INT(cmd, auiWWVen);
    } else if (type == WRITE_CMD) {
        data = remove_double_quote(data);
        auiWWVen = atoi(data);
        aos_kv_setint("auiWWVen", auiWWVen);
        AT_BACK_OK();
    }
}

void at_cmd_aui_kws(char *cmd, int type, char *data)
{
    if (type == TEST_CMD) {
        AT_BACK_OK();
    }
}

void at_cmd_aui_ctrl(char *cmd, int type, char *data)
{
    if (type == TEST_CMD) {
        AT_BACK_OK();
    } else if (type == READ_CMD) {
        if(NULL != g_at_aui_ctx.js_info) {
            convert_json_and_send("+AUICTRL", g_at_aui_ctx.js_info);
        } else {
            atserver_send("+AUICTRL:\r\n");
            AT_BACK_OK();
        }
    }
}

static char g_simple_spin_flag = 0;
static void _wifi_scan_cb(void)
{
	g_simple_spin_flag = 1;
}

static void _monitor_data_handler(wifi_promiscuous_pkt_t *buf, wifi_promiscuous_pkt_type_t type)
{
	return;
}

void wscan_handler(char *cmd, int type, char *data)
{
	int ret = -1;
	int i = 0;
	int cnt = 0;
	int max_try = 5;
	char *token = NULL;
	char *start = NULL;
	char *ssid = NULL;
	struct tls_scan_bss_t *wsr;
	struct tls_bss_info_t *bss_info;
	signed char rssi_max = 0;
	signed char rssi_min = 0;
	if (type == WRITE_CMD) {
		// ssid
		start = data;
		token = strchr(start, ',');
		if (!token || token - start > 32) {
			AT_RESP_CME_ERR(50);
			return;
		}
		*token = '\0';
		ssid = start;

		//rssi_max
		start = token + 1;
		token = strchr(start, ',');
		if (!token || token - start > 4) {
			AT_RESP_CME_ERR(50);
			return;
		}
		*token = '\0';
		rssi_max = atoi(start);

		//rssi_min
		start = token + 1;
		rssi_min = atoi(start);

		if (rssi_max < rssi_min) {
			AT_RESP_CME_ERR(50);
			return;
		}
		// printf("get cmd:%s %d %d\n", ssid, rssi_max, rssi_min);
		wifi_pair_stop();

    aos_dev_t *wifi_dev = device_open_id("wifi", 0);
		if (wifi_dev == NULL) {
			AT_RESP_CME_ERR(50);
			return;
		}
		hal_wifi_start_monitor(wifi_dev, _monitor_data_handler);

		while (max_try--) {
			g_simple_spin_flag = 0;
			tls_wifi_scan_result_cb_register(_wifi_scan_cb);

			cnt = 0;
			ret = tls_wifi_scan();
			// printf("start scan:%d\n", ret);
			while (UNI_SUCCESS != ret && UNI_WIFI_SCANNING_BUSY != ret) {
				if (++cnt > 5) {
					AT_RESP_CME_ERR(50);
					return;
				}
				aos_msleep(2000);
				ret = tls_wifi_scan();
				// printf("start scan:%d\n", ret);
			}

			cnt = 0;
			while (!g_simple_spin_flag) {
				// printf("wait g_simple_spin_flag:%d\n", cnt);
				if (++cnt > 10) {
					AT_RESP_CME_ERR(50);
					return;
				}
				aos_msleep(1000);
			}

			// get wifi info
			token = aos_malloc(2000);
			if (!token) {
				// printf("token malloc failed\n");
				AT_RESP_CME_ERR(50);
				return;
			}
    	if (tls_wifi_get_scan_rslt(token, 2000)) {
				// printf("get result failed\n");
				aos_free(token);
				AT_RESP_CME_ERR(50);
				return;
			}

			wsr = (struct tls_scan_bss_t *)token;
			bss_info = (struct tls_bss_info_t *)(token + 8);

			for(i = 0; i < wsr->count; i++) {
				// printf("loop wsr[%d]\n%s %d\n", i, bss_info->ssid, (signed char)bss_info->rssi);
				if (strlen(ssid) == bss_info->ssid_len && !strncmp(bss_info->ssid, ssid, bss_info->ssid_len)) {
					if ((signed char)bss_info->rssi > rssi_max || (signed char)bss_info->rssi < rssi_min) {
						AT_RESP_CME_ERR(50);
					} else {
						AT_BACK_OK();
					}
					aos_free(token);
					return;
				}
        bss_info ++;
			}
			aos_free(token);
		}
		// max_try over, cant't find specific ssid
		AT_RESP_CME_ERR(100);
	}
}

extern int u2c_uart_printf(const char *fmt, ...);
void do_wscan_handler(char *cmd, int argc, char *argv[])
{
    //printf("argc is %d\r\n",argc);
	int ret = -1;
	int i = 0;
	int cnt = 0;
	int max_try = 1;
	char *token = NULL;
	char *start = NULL;
	char *ssid = NULL;
	struct tls_scan_bss_t *wsr;
	struct tls_bss_info_t *bss_info;
	signed char rssi_max = 0;
	signed char rssi_min = 0;
    aos_dev_t *wifi_dev = device_open_id("wifi", 0);
    if (wifi_dev == NULL) {
        AT_RESP_CME_ERR(50);
        return;
    }
    hal_wifi_start_monitor(wifi_dev, _monitor_data_handler);
    // printf("max_try is %d\r\n",max_try);
    while (max_try--) {
        g_simple_spin_flag = 0;
        tls_wifi_scan_result_cb_register(_wifi_scan_cb);

        cnt = 0;
        ret = tls_wifi_scan();
        // printf("start scan:%d\n", ret);
        while (UNI_SUCCESS != ret && UNI_WIFI_SCANNING_BUSY != ret) {
            if (++cnt > 5) {
                AT_RESP_CME_ERR(50);
                return;
            }
            aos_msleep(2000);
            ret = tls_wifi_scan();
            // printf("start scan:%d\n", ret);
        }

        cnt = 0;
        while (!g_simple_spin_flag) {
            // printf("wait g_simple_spin_flag:%d\n", cnt);
            if (++cnt > 10) {
                AT_RESP_CME_ERR(50);
                return;
            }
            aos_msleep(1000);
        }
        // get wifi info
        token = aos_malloc(2000);
        if (!token) {
            // printf("token malloc failed\n");
            AT_RESP_CME_ERR(50);
            return;
        }
    	if (tls_wifi_get_scan_rslt(token, 2000)) {
				// printf("get result failed\n");
				aos_free(token);
				AT_RESP_CME_ERR(50);
				return;
			}

			wsr = (struct tls_scan_bss_t *)token;
			bss_info = (struct tls_bss_info_t *)(token + 8);
			for(i = 0; i < wsr->count; i++) {
                //printf("ssid_len is %d\r\n",bss_info->ssid_len);
                bss_info->ssid[bss_info->ssid_len]=0;
				// printf("%s: %d\r\n", bss_info->ssid, (signed char)bss_info->rssi);
                //printf("argv[1] is %s  bss_info->ssid is %s\r\n",argv[1],bss_info->ssid);
                //int ret=strstr(bss_info->ssid, argv[1]);
                //printf("ret is %d\r\n",ret);
                if(!argv[1]){
                    aos_free(token);
                    u2c_loguart_printf("ERROR\r\n");
                    return; 
                }else if(strstr(bss_info->ssid, argv[1])){
                    u2c_loguart_printf("%ddB\r\n",(signed char)bss_info->rssi);
                    aos_free(token);
                    return; 
                }
                bss_info ++;
			}
			aos_free(token);
            u2c_loguart_printf("ERROR\r\n");
		}
		// max_try over, cant't find specific ssid
		//AT_RESP_CME_ERR(100);
}

void wjap_handler(char *cmd, int type, char *data)
{
    if (type == WRITE_CMD) {
        char *token = strchr(data, ',');
        int ret = -1;
        if (token) {
            *token = '\0';
            int len = token - data;
            if (len <= 32 && len > 0) {
                ++token;
                if (strlen(token) <= 64) {
                    aos_kv_setstring("wifi_ssid", data);
                    aos_kv_setstring("wifi_psk", token);
                    ret = 0;
                    event_publish_app_at_cmd(APP_AT_CMD_WJAP);
                }
            }
        }

        if (ret) {
            AT_RESP_CME_ERR(50);
        } else {
            AT_BACK_OK();
        }
    }
}

void wjapd_handler(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
        aos_kv_del("wifi_ssid");
        aos_kv_del("wifi_psk");
        AT_BACK_OK();
    }
}

void wjapq_handler(char *cmd, int type, char *data)
{
    if (type == EXECUTE_CMD) {
        char ssid[33];
        char psk[65];
        int rst = aos_kv_getstring("wifi_ssid", ssid, sizeof(ssid));
        if (rst >= 0) {
            rst = aos_kv_getstring("wifi_psk", psk, sizeof(psk));
        }
        if (rst < 0) {
            AT_BACK_ERR();
        } else {
            AT_BACK_RET_OK2(cmd, ssid, psk);
        }
    }
}

void idm_pp_handler(char *cmd, int type, char *data)
{
    if (type == TEST_CMD) {
        AT_BACK_RET_OK(cmd, "\"device_id\",\"message\"");
    } else if (type == WRITE_CMD) {
        if (strncmp(data, "0,\"", 3) != 0) {
            AT_RESP_CME_ERR(50);
            return;
        }
        char *msg = data + 3;
        int len = strlen(msg);
        if (msg == NULL || len == 0 || msg[len - 1] != '\"') {
            AT_RESP_CME_ERR(50);
            return;
        }
        msg[len - 1] = '\0';

        char *str = msg;
        int count = 0;
        while (str && (str = strstr(str, "\\\"")) != NULL) {
            *str = ' ';
            str += 2;
            if (!str || (str = strstr(str, "\\\"")) == NULL) {
                AT_RESP_CME_ERR(50);
                return;
            }
            *str++ = '\"';
            *str++ = ' ';
            ++count;
        }
        if (count == 0) {
            AT_RESP_CME_ERR(50);
            return;
        }
        cJSON *root = cJSON_Parse(msg);
        
        if (root == NULL) {
            AT_RESP_CME_ERR(50);
            return;
        }

        cJSON_Delete(root);
        
        int rst = user_at_post_property(0, msg, strlen(msg));
        if (rst > 0) {
            AT_BACK_RET_OK_INT(cmd, rst);
        } else {
            AT_RESP_CME_ERR(100);
        }
    }
}

void idm_ps_handler(char *cmd, int type, char *data)
{
    if (type == TEST_CMD) {
        AT_BACK_OK();
    }
}

void idm_ep_handler(char *cmd, int type, char *data)
{
    if (type == TEST_CMD) {
        AT_BACK_RET_OK(cmd, "\"device_id\",\"event_id\",\"event_payload\"");
    } else if (type == WRITE_CMD) {
        if (strncmp(data, "0,\"", 3) != 0) {
            AT_RESP_CME_ERR(50);
            return;
        }
        char *evt_id = data + 3;
        char *payload = strstr(evt_id, "\",\"");
        if (payload == NULL || payload == evt_id || payload[strlen(payload) - 1] != '\"') {
            AT_RESP_CME_ERR(50);
            return;
        }

        payload[strlen(payload) - 1] = '\0';
        *payload = '\0';
        payload += 3;

        char *str = payload;
        int count = 0;
        while (str && (str = strstr(str, "\\\"")) != NULL) {
            *str = ' ';
            str += 2;
            if (!str || (str = strstr(str, "\\\"")) == NULL) {
                AT_RESP_CME_ERR(50);
                return;
            }
            *str++ = '\"';
            *str++ = ' ';
            ++count;
        }
        if (count == 0) {
            AT_RESP_CME_ERR(50);
            return;
        }
        cJSON *root = cJSON_Parse(payload);
        
        if (root == NULL) {
            AT_RESP_CME_ERR(50);
            return;
        }

        cJSON_Delete(root);
        
        int rst = user_at_post_event(0, evt_id, strlen(evt_id), payload, strlen(payload));
        if (rst >= 0) {
            AT_BACK_RET_OK_INT(cmd, rst);
        } else {
            AT_RESP_CME_ERR(100);
        }
    }
}

static int str2macaddr(char *str, uint8_t mac[6])
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


extern int g_test_mic_flag;
extern int g_test_mic_result;

void at_cmd_testMic(char *cmd, int type, char *data)
{
  int max_try = 5;
  LOGD(TAG, "at_cmd_testMic\n");

  if (type == EXECUTE_CMD) {
    g_test_mic_flag = true;  
    while (max_try--) {
      aos_msleep(1000);
      if (g_test_mic_result == true) {
        AT_BACK_OK();
        return;
      } else if (g_test_mic_result == false) {
        AT_BACK_ERR();
        return;
      }
    }
  }
  LOGD(TAG, "exit at_cmd_testMic\n");
  AT_BACK_ERR();
}

void at_cmd_qmac(char *cmd, int type, char *data)
{
  unsigned char mac[6];
	int ret = 0;
	char *start = NULL;
	char *token = NULL;
	if (type == READ_CMD) {
		tls_get_mac_addr(mac);
		atserver_send("OK+QMAC:%02x%02x%02x%02x%02x%02x\r\n\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	} else if (type == TEST_CMD || type == EXECUTE_CMD) {
        //LOGD(TAG, "enter TEST_CMD @@@@@@@@@@@@@@@@@@\n");
		tls_get_mac_addr(mac);
		atserver_send("+OK=%02x%02x%02x%02x%02x%02x\r\n\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	} else if (type == WRITE_CMD) {
		if (data[0] == '!') {
			data++;
		}
		if (data[0] != '\"') {
			AT_RESP_CME_ERR(50);
		}
		start = data + 1;
		token = strstr(start, "\"");
		if (!token || token - start != 12) {
			AT_RESP_CME_ERR(50);
		}
		ret = str2macaddr(start, mac);
		if (!tls_set_mac_addr(mac)) {
			AT_BACK_OK();
		} else {
			AT_RESP_CME_ERR(100);
		}
	}
}

void app_at_cmd_property_report_set(const char *msg, const int len)
{
    atserver_send("+IDMPS:0,%d,%.*s\r\nOK\r\n", len, len, msg);
}

void app_at_cmd_property_report_reply(const int packet_id, const int code, const char *reply, const int len)
{
    if (len) {
        atserver_send("+IDMPP:0,%d,%d,%d,%.*s\r\n", packet_id, code, len, len, reply);
    } else {
        atserver_send("+IDMPP:0,%d,%d\r\n", packet_id, code);
    }
}

void app_at_cmd_event_report_reply(const int packet_id, const int code, const char *evtid, const int evtid_len, const char *reply, const int len)
{
    if (len) {
        atserver_send("+IDMEP:0,%d,%d,%.*s,%d,%.*s\r\n", packet_id, code, evtid_len, evtid, len, len, reply);
    } else {
        atserver_send("+IDMEP:0,%d,%d,%.*s\r\n", packet_id, code, evtid_len, evtid);
    }
}

void app_at_cmd_sta_report()
{
    atserver_send("+IDMSTA:%d\r\nOK\r\n", smartliving_client_is_connected() << 1);
}

static void us615_lpinit(void) {
    tls_wifi_disconnect();
    tls_wifi_softap_destroy();
    tls_litepoint_start();
}

void at_cmd_lptpd(char *cmd, int type, char *data) {
	unsigned int period;
	if (type == WRITE_CMD) {
		if (data[0] == '!') {
			data++;
		}
		period = atoi(data);
		printf("set tx interval %u\n", period);

		tls_set_tx_litepoint_period(period);
		AT_BACK_POK();
	}
}

void at_cmd_lpchl(char *cmd, int type, char *data) {
	unsigned char channel;
	unsigned char bandwidth = 0;
	if (type == WRITE_CMD) {
		if (data[0] == '!') {
			data++;
		}

		char *buffer[2];
		char *start = data;
		char *token = NULL;
		char exist_bw = 0;

		// channel
		token = strstr(start, ",");
		if (!token) {
			exist_bw = 0;
			buffer[0] = start;
		} else {
			exist_bw = 1;
			*token = '\0';
			buffer[0] = start;
			buffer[1] = token + 1;
		}

    channel = strtol(buffer[0], NULL, 0);
		if (exist_bw) {
			bandwidth = strtol(buffer[1], NULL, 0);
		}

		printf("set test channel %d bandwidth %d\n", channel, bandwidth);
    us615_lpinit();
    tls_set_test_channel(channel, bandwidth);

		AT_BACK_POK();
	}
}

void at_cmd_lptstr(char *cmd, int type, char *data) {
	unsigned int tempcomp;
	unsigned int packetcnt;
	unsigned short psdulen;
	unsigned int gain;
	unsigned int txrate;
	if (type == WRITE_CMD) {
		if (data[0] == '!') {
			data++;
		}
		// temperaturecompensation,PacketCount,PsduLen,TxGain,DataRate
		if (/* data[0] != '0' || data[1] != 'x' */
				/* || */strlen(data) > 5 * 11) {
			AT_RESP_CME_ERR(50);
			return;
		}
		char *buffer[5];
		char *start = data;
		char *token = NULL;

		// temperaturecompensation
		token = strstr(start, ",");
		if (!token /* || data[0] != '0' || data[1] != 'x' */
			  || token - start > 11) {
			AT_RESP_CME_ERR(50);
			return;
		}
		*token = '\0';
		buffer[0] = start;

		// PacketCount
		start = token + 1;
		token = strstr(start, ",");
		if (!token /* || data[0] != '0' || data[1] != 'x' */
			  || token - start > 11) {
			AT_RESP_CME_ERR(50);
			return;
		}
		*token = '\0';
		buffer[1] = start;

		// PsduLen
		start = token + 1;
		token = strstr(start, ",");
		if (!token /* || data[0] != '0' || data[1] != 'x' */
			  || token - start > 11) {
			AT_RESP_CME_ERR(50);
			return;
		}
		*token = '\0';
		buffer[2] = start;

		// TxGain
		start = token + 1;
		token = strstr(start, ",");
		if (!token /* || data[0] != '0' || data[1] != 'x' */
			  || token - start > 11) {
			AT_RESP_CME_ERR(50);
			return;
		}
		*token = '\0';
		buffer[3] = start;
		
		// DataRate
		start = token + 1;
		buffer[4] = start;

    tempcomp = strtol(buffer[0], NULL, 16);
    packetcnt = strtol(buffer[1], NULL, 16);
    psdulen = strtol(buffer[2], NULL, 16);
    gain = strtol(buffer[3], NULL, 16);
    txrate = strtol(buffer[4], NULL, 16);
    printf("set tx params: tempcomp %x, cnt %x, len %x, gain %x, rate %x\n", tempcomp, packetcnt, psdulen, gain, txrate);

		us615_lpinit();
    tls_tx_litepoint_test_start(tempcomp, packetcnt, psdulen, gain, txrate, 0, 0, 0);

		AT_BACK_POK();
	}
}

void at_cmd_lptstp(char *cmd, int type, char *data) {
	if (type == EXECUTE_CMD) {
    printf("stop tx process\n");

    tls_txrx_litepoint_test_stop();
		AT_BACK_POK();
	}
}

void at_cmd_lptstt(char *cmd, int type, char *data) {
	if (type == EXECUTE_CMD) {
		// printf("tx reslut\n");

		atserver_send("+OK=%x\r\n", tls_tx_litepoint_test_get_totalsnd());
	}
}


void at_cmd_lprstr(char *cmd, int type, char *data) {
	unsigned char channel;
	unsigned char bandwidth = 0;
	if (type == WRITE_CMD) {
		if (data[0] == '!') {
			data++;
		}

		char *buffer[2];
		char *start = data;
		char *token = NULL;
		char exist_bw = 0;

		// channel
		token = strstr(start, ",");
		if (!token) {
			exist_bw = 0;
			buffer[0] = start;
		} else {
			exist_bw = 1;
			*token = '\0';
			buffer[0] = start;
			// bandwidth
			buffer[1] = token + 1;
		}

    /* here itest Rx receive is 16 Hexadecimal type data , different Tx data, Tx is Decimal  */
    channel = strtol(buffer[0], NULL, 16);
		if (exist_bw) {
			bandwidth = strtol(buffer[1], NULL, 16);
		}

    printf("set channel %hhu, bandwidth %hhu\n", channel, bandwidth);

    us615_lpinit();
    tls_rx_litepoint_test_start(channel, bandwidth);
		AT_BACK_POK();
	}
}

void at_cmd_lprstp(char *cmd, int type, char *data) {
	if (type == EXECUTE_CMD) {
    printf("stop rx process\n");

    tls_txrx_litepoint_test_stop();
		AT_BACK_POK();
	}
}

void at_cmd_lprstt(char *cmd, int type, char *data) {
	u32 cnt_total = 0;
	u32 cnt_good = 0;
	u32 cnt_bad = 0;
	if (type == EXECUTE_CMD) {
		// printf("rx reslut\n");

		tls_rx_litepoint_test_result(&cnt_total, &cnt_good, &cnt_bad);
		atserver_send("+OK=%x,%x,%x\r\n", cnt_total, cnt_good, cnt_bad);
	}
}

extern int tls_tx_gainindex_map2_gainvalue(u8 *dst_txgain, u8 *srcgain_index);
extern int tls_tx_gainvalue_map2_gainindex(u8 *dst_txgain_index, u8 *srcgain);
void at_cmd_txgi(char *cmd, int type, char *data)
{
	u8 tx_gain[TX_GAIN_LEN];
	u8* param_tx_gain = ieee80211_get_tx_gain();
	int i = 0;
	int len = 0;
	u8 tx_gain_index[TX_GAIN_LEN/3];
	int ret = 0;
	char str[128];

	if(type == WRITE_CMD)
	{
        LOGE(TAG, "at_cmd_txgi type is WRITE_CMD, data is [%s]\n",data+1);
        str2hex(tx_gain_index,(data+1),strlen(data+1));	// data delete "!" , so it should add 1	
		tls_tx_gainindex_map2_gainvalue(tx_gain, tx_gain_index);
		for (i = 0; i < TX_GAIN_LEN/3; i++)
		{
            //LOGE(TAG, "########tx_gain_index[%d] is [%x]\n",i,tx_gain_index[i]);
			if (tx_gain_index[i] == 0xFF)
			{
				tx_gain[i] = 0xFF;
				tx_gain[i+TX_GAIN_LEN/3] = 0xFF;
				tx_gain[i+TX_GAIN_LEN*2/3] = 0xFF;
			}
			else
			{
				param_tx_gain[i] = tx_gain[i];
				param_tx_gain[i+TX_GAIN_LEN/3] = tx_gain[i];
				param_tx_gain[i+TX_GAIN_LEN*2/3] = tx_gain[i];
			}
		}
		ret = tls_set_tx_gain(tx_gain);
		if (ret == 0)
		{
            LOGE(TAG, "tls_set_tx_gain ok\n");
			AT_BACK_POK();
		}
		else
		{
            LOGE(TAG, "tls_set_tx_gain fail\n");
			AT_BACK_ERR();
		}
	}

	if (type == READ_CMD || type == TEST_CMD || type == EXECUTE_CMD)
	{
		/*ÈçÊµ·´Ó³flash²ÎÊýÇøµÄÊµ¼Ê´æ´¢Çé¿ö*/
		ret = tls_get_tx_gain(tx_gain);
        LOGE(TAG, "at_cmd_txgi tls_get_tx_gain ret is [%d]\n",ret);
        aos_msleep(100);
		if (ret == 0)
		{
			memset(tx_gain_index, 0xFF, TX_GAIN_LEN/3);
			tls_tx_gainvalue_map2_gainindex(tx_gain_index, tx_gain);
			len = 0;
			for (i = 0; i < TX_GAIN_LEN/3; i++)
			{
				len += sprintf(str + len, "%02x", tx_gain_index[i]);
			}
			atserver_send("\r\n+OK=%s\r\n", str);
		}
		else
		{
			AT_BACK_ERR();
		}
	}
}

const atserver_cmd_t app_at_cmd[] = {
    {"AT+IDMAU", idm_au_handler},
#if defined(EN_COMBO_NET) && (EN_COMBO_NET == 1)    
    {"AT+IDMPID", idm_pid_handler},
#endif    
#if 0
    {"AT+IWSSTART", iws_start_handler},
    {"AT+IWSSTOP", iws_stop_handler},
    {"AT+DEVINFO", at_cmd_dev_info},
    {"AT+AUICFG", at_cmd_aui_cfg},
    {"AT+AUIFMT", at_cmd_aui_fmt},
    {"AT+AUIMICEN", at_cmd_aui_micen},
    {"AT+AUIWWVEN", at_cmd_wwv_en},
    {"AT+AUIKWS", at_cmd_aui_kws},
    {"AT+AUICTRL", at_cmd_aui_ctrl},
#endif
    {"AT+WSCAN", wscan_handler},
    {"AT+WJAP", wjap_handler},
    {"AT+WJAPD", wjapd_handler},
    {"AT+WJAPQ", wjapq_handler},
#if 0
    {"AT+IDMCON", idm_con_handler},
    {"AT+IDMCLS", idm_cls_handler},
    {"AT+IDMSTA", idm_sta_handler},
    {"AT+IDMPP", idm_pp_handler},
    {"AT+IDMPS", idm_ps_handler},
    {"AT+IDMEP", idm_ep_handler},
#endif
    {"AT+&MIC", at_cmd_testMic},
    {"AT+QMAC", at_cmd_qmac},
    {"AT+&MAC", at_cmd_qmac},
    {"AT+&LPCHL", at_cmd_lpchl},
    {"AT+&LPTPD", at_cmd_lptpd},
    {"AT+&LPTSTR", at_cmd_lptstr},
    {"AT+&LPTSTP", at_cmd_lptstp},
    {"AT+&LPTSTT", at_cmd_lptstt},
    {"AT+&LPRSTR", at_cmd_lprstr},
    {"AT+&LPRSTP", at_cmd_lprstp},
    {"AT+&LPRSTT", at_cmd_lprstt},
    {"AT+&TXGI", at_cmd_txgi},
    // TODO:
    AT_NULL,
};

void app_at_kws_notify(uint8_t notify)
{
    /*
        AT协议为 --->"+AUIKWS=1\r\n"
    */
    if(0 == g_at_init) {
        return;
    }
    atserver_send("+AUIKWS:%d\r\n", notify);
    AT_BACK_OK();
}

void app_at_ctrl_notify(cJSON *js)
{
    /*
        AT协议为 --->"+AUICTRL=1\r\n"
    */
    if(0 == g_at_init) {
        return;
    }

    if(NULL != g_at_aui_ctx.js_info) {
        cJSON_Delete(g_at_aui_ctx.js_info);
    }
    g_at_aui_ctx.js_info = cJSON_CreateObject();
    cJSON *action = cJSON_GetObjectItemByPath(js, "payload.action");
    if (cJSON_IsString(action)) {
        cJSON_AddStringToObject(g_at_aui_ctx.js_info, "action", action->valuestring);
    } else {
        LOGD(TAG, "cJSON object doesn't have 'action' item, return!\n");
        cJSON_Delete(g_at_aui_ctx.js_info);
        g_at_aui_ctx.js_info = NULL;
        return;
    }
    cJSON *action_params = cJSON_GetObjectItemByPath(js, "payload.action_params");
    char *str_params = cJSON_PrintUnformatted(action_params);
    if (NULL != str_params) {
        cJSON_AddStringToObject(g_at_aui_ctx.js_info, "action_params", str_params);
    }

    convert_json_and_send("+AUICTRL", g_at_aui_ctx.js_info);

    free(str_params);
}

int app_at_cmd_init()
{
    atserver_add_command(app_at_cmd);
    event_subscribe(EVENT_APP_AT_CMD, event_app_at_cmd_handler, NULL);
    g_at_init = 1;
    return 0;
}
