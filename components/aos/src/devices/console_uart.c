/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <aos/aos.h>
#include <aos/kernel.h>
#include <aos/debug.h>
#include <devices/hal/uart_impl.h>
#ifdef CONFIG_CSI_V2
#include "drv/uart.h"
#else
#include "drv/usart.h"
#endif
#include <devices/console_uart.h>

aos_dev_t *g_console_handle = NULL;
static uint16_t g_console_buf_size = 128;
static aos_mutex_t g_console_mutex_handle;
const char *console_get_devname(void)
{
    static char console_devname[32] = {0};

    if (g_console_handle) {
        snprintf(console_devname, sizeof(console_devname), "%s%d",
                 ((driver_t *)g_console_handle->drv)->name, g_console_handle->id);
    }

    return console_devname;
}

uint16_t console_get_buffer_size(void)
{
    return g_console_buf_size;
}

void console_init(int idx, uint32_t baud, uint16_t buf_size)
{
    uart_config_t config;
    aos_mutex_new(&g_console_mutex_handle);
    g_console_handle = uart_open_id("uart", idx);

    aos_check(g_console_handle, EIO);

    if (g_console_handle != NULL) {
        uart_config_default(&config);
        config.baud_rate = baud;
        uart_config(g_console_handle, &config);
        uart_set_type(g_console_handle, UART_TYPE_CONSOLE);

        g_console_buf_size = buf_size;
    }
}

void console_deinit()
{
    if (g_console_handle) {
        uart_close(g_console_handle);
        g_console_handle = NULL;
    }
    if(g_console_mutex_handle.hdl != NULL) {
        aos_mutex_free(&g_console_mutex_handle);
    }

}

int uart_putc(int ch)
{
    int data;

    if (g_console_handle == NULL) {
        return -1;
    }

    if (ch == '\n') {
        data = '\r';
        uart_send(g_console_handle, &data, 1);
    }

    uart_send(g_console_handle, &ch, 1);

    return 0;
}

int uart_getc(void)
{
    if (g_console_handle != NULL) {
        char ch = 0;
        uart_recv(g_console_handle, &ch, 1, -1);

        return (int)ch;
    }

    return 0;
}

int os_critical_enter(unsigned int *lock)
{
    int ret;
    ret = aos_mutex_lock(&g_console_mutex_handle,10000);
    return ret;
}

int os_critical_exit(unsigned int *lock)
{
    int ret;
    ret = aos_mutex_unlock(&g_console_mutex_handle);
    return ret;
}
