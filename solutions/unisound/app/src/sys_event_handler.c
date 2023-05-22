/*
 */

#include <app_config.h>

#include <stdlib.h>
#include <aos/debug.h>
#include <aos/kernel.h>
#include <ulog/ulog.h>

// #include <uservice/eventid.h>
// #include <uservice/uservice.h>

#include <driver/uni_gpio_afsel.h>
#include <csi_core.h>

#include "msg_process_center.h"

#define TAG "AppExp"
extern color_hsv_t HSV_Color;
static void app_except_process(int errno, const char *file, int line, const char *func_name, void *caller)
{
    LOGE(TAG, "Except! errno is %s, function: %s at %s:%d, caller: 0x%x\n", strerror(errno), func_name, file, line, caller);
    
    uni_swd_config(1);

    csi_irq_save();
#ifdef CONFIG_DEBUG
    while(1);
#else
    aos_reboot();
#endif
}

void sys_event_init(void)
{
    /*
    * 注册系统异常处理,aos_check_return_xxx 相关函数
    * 若错误，会调用到回调函数，可以做异常处理
    */
    aos_set_except_callback(app_except_process);

    /* 默认设置为0，软狗不生效 */
    // utask_set_softwdt_timeout(0);

    /* 若uService中的任务有超过指定时间没有退出，软狗系统会自动重启 */
    //utask_set_softwdt_timeout(60000);
}
