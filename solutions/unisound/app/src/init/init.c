#include <stdbool.h>
#include <aos/aos.h>
#include <yoc/yoc.h>
#include <devices/devicelist.h>
#include <driver/uni_internal_flash.h>
#include <driver/uni_efuse.h>
#include <platform/uni_params.h>
#include <hci_hal_h4.h>

#include "uni_share_mem.h"
#include "uni_auto_control.h"
#include "app_main.h"
#include "pin_name.h"
#include "pin.h"

#define TAG "INIT"

#ifndef CONSOLE_IDX
#define CONSOLE_IDX 0
#endif

extern int app_ai_init();

void us615_param_init(void) {
  static int us615_board_is_init = 0;

  if (!us615_board_is_init) {
    tls_fls_init();
    tls_fls_sys_param_postion_init();

    tls_ft_param_init();
    tls_param_load_factory_default();
    tls_param_init();

    us615_board_is_init = 1;
  }
}

void board_yoc_init() {
  us615_param_init();

  uart_csky_register(CONSOLE_IDX);
  flash_csky_register(0);           // on-chip flash
  //spiflash_csky_register(1);      // external flash
  //app_extflash_register();

  console_init(CONSOLE_IDX, 115200, 128);

  ulog_init();
  aos_set_log_level(AOS_LL_DEBUG);
  share_mem_init();

  user_gpio_init();

  /* init early for fast ram */
  app_ai_init();

  //LOGI(TAG, "Build:%s,%s",__DATE__, __TIME__);
  /* load partition */
  int ret = partition_init();
  if (ret <= 0) {
    LOGE(TAG, "partition init failed");
  } else {
    LOGI(TAG, "find %d partitions", ret);
  }

  //app_extflash_init();

  aos_kv_init("kv");

  bt_us615_register();

  extern int hci_h4_driver_init();
  hci_h4_driver_init();

  board_cli_init();
}
