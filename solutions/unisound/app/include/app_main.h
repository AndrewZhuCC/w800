/*
 */
#ifndef _APP_MAIN_H_
#define _APP_MAIN_H_

#include <app_config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <aos/aos.h>
#include <aos/cli.h>
#include <aos/cli_cmd.h>

#include <pin_name.h>
#include <drv/gpio.h>
#include <pinmux.h>
#include <devices/devicelist.h>

#include <yoc/yoc.h>
#include <uservice/eventid.h>
#include <uservice/uservice.h>

#include <iot_dispatcher.h>
#include <mvoice_alg.h>

#include "app_init.h"
#include "app_sys.h"

/* wifi & net */
typedef enum {
    MODE_WIFI_TEST = -2,
    MODE_WIFI_CLOSE = -1,
    MODE_WIFI_NORMAL = 0,
    MODE_WIFI_PAIRING = 1
}wifi_mode_e;

wifi_mode_e app_network_init(void);    

/* Smart Living */
/**
 * control smartliving client
 *
 * @param[in]  flag          0: stop, others: start
 *
 * @return  0: success
 */
int smartliving_client_control(const int flag);

/**
 * Get connection status of smartliving client
 *
 * @return 0: disconnected
 */
int smartliving_client_is_connected(void);

int user_post_given_property(char *message, int len);

/**
 * smartliving report property of AT command
 *
 * @param[in]  device_id     Device ID, should be 0
 * @param[in]  message       pointer to message
 * @param[in]  len           lenth of message
 *
 * @return >0: packet id, others: fail
 */
int user_at_post_property(int device_id, char *message, int len);

/**
 * smartliving report event of AT command
 *
 * @param[in]  device_id     Device ID, should be 0
 * @param[in]  evtid         pointer to Event ID
 * @param[in]  evtid_len     lenth of Event ID
 * @param[in]  evt_payload   pointer to event payload
 * @param[in]  len           lenth of event payload
 *
 * @return >0: Packet ID, others: fail
 */
int user_at_post_event(int device_id, char *evtid, int evtid_len, char *evt_payload, int len);



void wifi_pair_start(void);
wifi_mode_e app_network_reinit();

int app_iot_init(void);

/* sys event */
void sys_event_init(void);
void sys_event_handle(uint32_t event_id);

/* fota */
void app_fota_init(void);
void app_fota_start(void);
int app_fota_is_downloading(void);
void app_fota_do_check(void);
void app_fota_set_auto_check(int enable);

void app_button_init(void);

/* external flash */
int app_extflash_init(void);
int app_extflash_register(void);
int app_extflash_read(uint32_t addr, const void *data, uint32_t size);
int app_extflash_write(uint32_t addr, const void *data, uint32_t size);
void app_extflash_deinit(void);

void app_at_server_init(utask_t *task, const char *device_name);
int app_at_parser_init(utask_t *task, const char *device_name);
void app_pinmap_usart_init(int32_t uart_idx);
int app_at_cmd_init(void);
void app_at_cmd_sta_report();
void app_at_cmd_property_report_set(const char *msg, const int len);
void app_at_cmd_property_report_reply(const int packet_id, const int code, const char *reply, const int len);
void app_at_cmd_event_report_reply(const int packet_id, const int code, const char *evtid, const int evtid_len, const char *reply, const int len);
void cli_reg_cmd_us615(void);
void combo_net_post_ext(void);


/* wifi provisioning method */
#define WIFI_PROVISION_MIN              0
#define WIFI_PROVISION_SL_SMARTCONFIG   0
#define WIFI_PROVISION_SOFTAP           1
#define WIFI_PROVISION_SL_BLE           2
#define WIFI_PROVISION_SL_DEV_AP        3
#define WIFI_PROVISION_MAX              4


/* event id define */
#define EVENT_NTP_RETRY_TIMER       (EVENT_USER + 1)
#define EVENT_NET_CHECK_TIMER       (EVENT_USER + 2)
#define EVENT_NET_NTP_SUCCESS       (EVENT_USER + 3)
#define EVENT_NET_LPM_RECONNECT     (EVENT_USER + 4)

/* app at cmd */
#define EVENT_APP_AT_CMD            (EVENT_USER + 26)

/* event sl ble */
#define EVENT_SLBLE_AP_INFO         (EVENT_USER + 30)
#define EVENT_SLBLE_RESTART_ADV     (EVENT_USER + 31)

#define PWM_LED_LIGHT_MIN 0
#define PWM_LED_LIGHT_MAX 100

typedef enum {
    COMBO_BLE_SET = 0,
    COMBO_BLE_GET,
    COMBO_BLE_MAX
} combo_ble_operation_e;

#endif
