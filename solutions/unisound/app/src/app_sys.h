/*
 */
#ifndef _APP_SYS_H_
#define _APP_SYS_H_

/* boot reason */
#define BOOT_REASON_POWER_ON        0
#define BOOT_REASON_SOFT_RESET      1
#define BOOT_REASON_POWER_KEY       2
#define BOOT_REASON_WAKE_STANDBY    3
#define BOOT_REASON_WIFI_CONFIG     4
#define BOOT_REASON_SILENT_RESET    5
#define BOOT_REASON_NONE            6

void app_sys_init();
void app_sys_reboot();
int app_sys_set_boot_reason(int reason);
int app_sys_get_boot_reason();

#endif
