/**
 * Copyright 2020, unisound.com. All rights reserved.
 */

#include <stdlib.h>
#include <string.h>
#include <aos/aos.h>
#include "aos/cli.h"
#include "app_main.h"
#include <aos/yloop.h>
#include <drv/codec.h>
#include "uni_cust_config.h"
#include "user_player.h"
#include "uni_record_save.h"
#include "local_audio.h"

#include "uni_gpio.h"

#include "msg_process_center.h"

#define TAG "APP"

u8 pwm_cnt=0,time_cnt=0,cnt_dowm=0;

static aos_task_t task_msg_process;
static aos_task_t task_awss_process;
static aos_task_t task_ir_state;
static aos_task_t task_setkv_process;

aos_timer_t _start_light;
extern color_hsv_t HSV_Color;
extern void led_static_color_show_other(uint16_t h,uint8_t s,uint8_t v);
extern void hlk_u2c_app_start();
extern void cli_reg_cmd_app(void);
static void cli_reg_cmds(void) {
  cli_reg_cmd_app();
  //cli_reg_cmd_iperf();
  //cli_reg_cmd_us615();
  cli_reg_cmd_ifconfig();
  cli_reg_cmd_sysinfo();
}

int red_sta=0,AWSS_START=0;

static void awss_timer_callback(void *arg1, void *arg2)
{
    // MPC_LOG("scene_timer timeout");
    // if(AWSS_START==1)
    {
        if(red_sta)
        {
            user_pwm_enable(PB1,50);
            user_pwm_enable(PB0,100);
            user_pwm_enable(PB2,100);
            red_sta=0;
        }
        else if(red_sta==0)
        {
            user_pwm_enable(PB1,100);
            user_pwm_enable(PB0,100);
            user_pwm_enable(PB2,100);
            red_sta=1;
        }
    }  
}

static void start_light_timer_callback(void *arg1, void *arg2)
{
  pwm_cnt++;
  if(cnt_dowm==0 &&pwm_cnt<=100)
  {
    user_pwm_enable(PB0, 0);
    user_pwm_enable(PB1, 0);
    user_pwm_enable(PB2, pwm_cnt);
    if(pwm_cnt==100)
    cnt_dowm++;
  }
  else if(cnt_dowm==1 && pwm_cnt > 100 && pwm_cnt<=200)
  {
    // pwm_cnt -= 100;
    user_pwm_enable(PB0, 0);
    user_pwm_enable(PB2, 0);
    user_pwm_enable(PB1, pwm_cnt-100);
    if(pwm_cnt==200)
    cnt_dowm++;
  }
  else if(cnt_dowm==2 && pwm_cnt > 200 && pwm_cnt<=300)
  {
    // pwm_cnt -= 200;
    user_pwm_enable(PB2, 0);
    user_pwm_enable(PB1, 0);
    user_pwm_enable(PB0, pwm_cnt-200);
    if(pwm_cnt==300)
    cnt_dowm++;
  }
  else if(cnt_dowm==3 && pwm_cnt > 300)
  {
    // pwm_cnt -= 200;
    user_pwm_enable(PB2, 100);
    user_pwm_enable(PB1, 100);
    user_pwm_enable(PB0, 100);
    // if(pwm_cnt==300)
    cnt_dowm++;
  }
  else {
    user_pwm_enable(PB2, 0);
    user_pwm_enable(PB1, 0);
    user_pwm_enable(PB0, 0);
    HSV_Color.light_switch=1;
    HSV_Color.h = 359;
    HSV_Color.s = 0;
    HSV_Color.v = 100;
    aos_timer_stop(&_start_light);
    aos_timer_free(&_start_light);
  }
}

static void start_lighting(void) {
  int ret,start_en,cloud_suc,ota_flag,error_flag,len;
  aos_timer_t awss_timer;
  // color_hsv_t Color_aa;
  aos_timer_new_ext(&awss_timer, awss_timer_callback, NULL, 200, 1, 0);
  aos_timer_new_ext(&_start_light, start_light_timer_callback, NULL, 10, 1, 0);
  len = sizeof(color_hsv_t);
  if(aos_kv_getint("start_en", &start_en)==0)
  {
    red_sta=1;
    // AWSS_START=1;
    aos_timer_stop(&awss_timer);
    aos_timer_start(&awss_timer);
    aos_kv_del("start_en");
  }
  else if(aos_kv_getint("ota_flag", &ota_flag)==0)
  {
    ret=aos_kv_get("HSV_Color",&HSV_Color,&len);
    if(HSV_Color.light_switch==1) {
      led_static_color_show_other(HSV_Color.h, HSV_Color.s, HSV_Color.v);
    }
    printf("ret=%d,HSV_Color:%d ,%d ,%d ,%d\n",ret,HSV_Color.light_switch,HSV_Color.h,HSV_Color.s,HSV_Color.v);
    aos_kv_del("ota_flag");
    aos_kv_del("HSV_Color");
  }
  else
  {
    aos_timer_start(&_start_light);
  }
}

static void w_dog_timer_callback(void *arg1, void *arg2)
{
  tls_watchdog_clr();
}

static void watchdog_init_and_clr(void) {
  aos_timer_t _watchdog_flag;
  aos_timer_new_ext(&_watchdog_flag, w_dog_timer_callback, NULL, 5*1000, 1, 0);
  tls_watchdog_init(10000000);
  aos_timer_start(&_watchdog_flag);
}

extern uni_err_t kws_start(void);

int main() {
  int tick_counter = 0;
  board_yoc_init();
  LOGD(TAG, "%s\n", aos_get_app_version());
  printf("fffff*****************HLK_800_PRO-V1.0.0*******************\r\n");
  //aos_set_log_level(AOS_LL_INFO);
  check_pid_effective();
  event_service_init(NULL);
  app_sys_init();
  board_base_init();
  sys_event_init();
  cli_reg_cmds();

  int ret = csi_codec_init(0);
  if (ret < 0) {
    LOGE(TAG, "codec init failed");
  }

  local_audio_init();
  app_iot_init();
#if 1	// use PA0 as wifi pairing button
  app_button_init();
#endif
#if 0   //at cmd
  utask_t *task = NULL;
  app_at_server_init(task, "uart1");
  app_at_cmd_init();
#endif

#if defined(CONFIG_UNI_UART_RECORD) && CONFIG_UNI_UART_RECORD
  RecordSaveInit();
#endif

#if defined(APP_FOTA_EN) && APP_FOTA_EN
  app_fota_init();
#endif

  wifi_mode_e mode = app_network_init();
  LOGI(TAG, "wifi in mode %d", mode);

#ifdef DEFAULT_PCM_WELCOME
  int reason = app_sys_get_boot_reason();
  if (BOOT_REASON_POWER_ON == reason || BOOT_REASON_POWER_KEY == reason) {
    user_player_play(DEFAULT_PCM_WELCOME);
  } else {
    kws_start();
  }

  // aos_timer_new_ext(&wake_up_start, wake_up_timer_callback, NULL, 20, 1, 1);
#else
  kws_start();
#endif
  
  // hlk_u2c_app_start();
  
  init_msg_queue();
  aos_task_new_ext(&task_msg_process, "cmd msg process", msg_process_task, NULL, 2048, AOS_DEFAULT_APP_PRI);

  start_lighting();
  watchdog_init_and_clr();

  return 0;
}
