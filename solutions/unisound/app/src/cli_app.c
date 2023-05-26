/*
 */

#include "app_main.h"
#include <driver/uni_efuse.h>
#include <driver/uni_gpio_afsel.h>
#include "local_audio.h"
#include "user_at_command.h"

#define TAG "cliapp"

extern int wifi_prov_method;

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

static void cmd_app_func(char *wbuf, int wbuf_len, int argc, char **argv)
{
    int ret = 0;
    
    if (argc < 3) {
        return;
    }
    if (strcmp(argv[1], "log") == 0) {
        aos_set_log_level(atoi(argv[2]));
    } else if (strcmp(argv[1], "mute") == 0) {
        mvoice_process_pause();
    } else if (strcmp(argv[1], "unmute") == 0) {
        mvoice_process_resume();
#if 0
    } else if (strcmp(argv[1], "kws") == 0) {
        if (argc == 3) {
          if (strcmp(argv[2], "start") == 0) {
            kws_start();
            LOGD(TAG, "kws_start state = %d", kws_state());
          } else {
            if (strcmp(argv[2], "stop") == 0) {
              kws_stop();
              LOGD(TAG, "kws_stop state = %d", kws_state());
            }
          }
        }
    } else if (strcmp(argv[1], "mac") == 0) {
        // uint8_t mac[6];
        // wifi_getmac(mac);
        // printf("%02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
#endif
    } else if (strcmp(argv[1], "wifi_prov") == 0) {
        if (strcmp(argv[2], "smartconfig") == 0) {
            LOGD(TAG, "smart config");
            wifi_prov_method = WIFI_PROVISION_SL_SMARTCONFIG;
            aos_kv_setint("wprov_method", wifi_prov_method);
            app_sys_set_boot_reason(BOOT_REASON_WIFI_CONFIG);
            aos_reboot();
        } else if (strcmp(argv[2], "dev_ap") == 0) {
            LOGD(TAG, "dev ap");
            wifi_prov_method = WIFI_PROVISION_SL_DEV_AP;
            aos_kv_setint("wprov_method", wifi_prov_method);
            app_sys_set_boot_reason(BOOT_REASON_WIFI_CONFIG);
            aos_reboot();
        } else if (strcmp(argv[2], "ble") == 0) {
            LOGD(TAG, "ble");
            wifi_prov_method = WIFI_PROVISION_SL_BLE;
            aos_kv_setint("wprov_method", wifi_prov_method);

            aos_kv_del("AUTH_AC_AS");
            aos_kv_del("AUTH_KEY_PAIRS");
            app_sys_set_boot_reason(BOOT_REASON_WIFI_CONFIG);
            aos_reboot();
        } else if (strcmp(argv[2], "softap") == 0) {
            LOGD(TAG, "softap");
            wifi_prov_method = WIFI_PROVISION_SOFTAP;
            aos_kv_setint("wprov_method", wifi_prov_method);
            app_sys_set_boot_reason(BOOT_REASON_WIFI_CONFIG);
            aos_reboot();
        }
    } else if (strcmp(argv[1], "bt") == 0) {
        extern int tls_set_bt_mac_addr(uint8_t *mac);
        if (strcmp(argv[2], "mac") == 0) {
            if (argc >= 4) {
                if (strlen(argv[3]) == 12) {
                    uint8_t mac[6];
                    ret = str2macaddr(argv[3], mac);
                    if (ret != 0) {
                        printf("Usage: bt mac C01122334455\n");
                        return;
                    }
                    if (!tls_set_bt_mac_addr(mac)) {
                        printf("set bt mac successfully\n");
                    } else {
                        printf("set bt mac failed\n");
                    }
                } else {
                    printf("Usage: bt mac C01122334455\n");
                }
            } else {
                uint8_t mac[6];
                if (!tls_get_bt_mac_addr(mac)) {
                    printf("bt mac = %02X%02X%02X%02X%02X%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                } else {
                    printf("bt mac not set\n");
                }
            }
        }
    } else if (strcmp(argv[1], "wifi") == 0) {
        extern int tls_get_mac_addr(uint8_t *mac);
        if (strcmp(argv[2], "mac") == 0) {
            if (argc >= 4) {
                if (strlen(argv[3]) == 12) {
                    uint8_t mac[6];
                    ret = str2macaddr(argv[3], mac);
                    if (ret != 0) {
                        printf("Usage: wifi mac C01122334455\n");
                        return;
                    }
                    if (!tls_set_mac_addr(mac)) {
                        printf("set wifi mac successfully\n");
                    } else {
                        printf("set wifi mac failed\n");
                    }
                } else {
                    printf("Usage: wifi mac C01122334455\n");
                }
            } else {
                uint8_t mac_addr[6] = {0};   
                tls_get_mac_addr(mac_addr);
                printf("mac addr %02x%02x%02x%02x%02x%02x\r\n", mac_addr[0], mac_addr[1], mac_addr[2],
                        mac_addr[3], mac_addr[4], mac_addr[5]);
            }
        }
#if 0
    } else if (strcmp(argv[1], "audio") == 0) {
        user_player_reply_list_in_order(argv[2]);
    } else if (strcmp(argv[1], "wav") == 0) {

        // char url[128];
        // snprintf(url, 127, "mem://addr=%u&size=%u", audio_18, sizeof(audio_18));
        // LOGD(TAG, "play local audio %s", url);
        // mplayer_play(url);
#endif
    } else if (strcmp(argv[1], "vol") == 0) {
        int cur_vol = local_audio_vol_get();
        cur_vol = cur_vol < 0 ? 0 : cur_vol;

        int vol = -1;
        if (strcmp(argv[2], "+") == 0) {
            vol = cur_vol + 10 > 127 ? 127 : cur_vol + 10;
        } else if (strcmp(argv[2], "-") == 0) {
            vol = cur_vol - 10 < 0 ? 0 : cur_vol - 10;
        } else {
            vol = atoi(argv[2]);
        }

        if (vol <= 127 && vol >= 0) {
            local_audio_vol_set(vol);
        }
    } else if (strcmp(argv[1], "at") == 0) {
        printf("cli at %s %s\n", argv[2], argv[3]);
        uint32_t timeout = atol(argv[3]);
        send_at_command(argv[2], timeout);
#if 0
    } else if (strcmp(argv[1], "jtag") == 0) {
        uni_swd_config(atoi(argv[2]));
    } else if (strcmp(argv[1], "flash_test") == 0) {
        extern int app_extflash_test();
        app_extflash_test();
        // aos_task_t flash_tsk;
        // aos_task_new_ext(&flash_tsk, "flashtest", app_extflash_test, NULL, 4096, AOS_DEFAULT_APP_PRI - 2);
#endif
    } else {
        printf("app mute/unmute/vol/wifi_prov/wifi/bt\n");
    }
}

void cli_reg_cmd_app(void)
{
    /* 其他CLI命令 */
    static const struct cli_command cmd_info_app = {"app", "app test cmd", cmd_app_func};
    aos_cli_register_command(&cmd_info_app);
}
