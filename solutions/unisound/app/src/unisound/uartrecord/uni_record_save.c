/**************************************************************************
 * Copyright (C) 2017-2017  Unisound
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **************************************************************************
 *
 * Description : uni_record_save.c
 * Author      : wufangfang@unisound.com
 * Date        : 2019.12.23
 *
 **************************************************************************/
#include "uni_iot.h"
#include "uni_log.h"
#include <aos/aos.h>
#include <devices/uart.h>
#include "uni_communication.h"
#include "uni_channel.h"

#define TAG "record"

aos_dev_t *g_uart_hanlder = NULL;
aos_event_t g_recv_event;

static int _record_uart_write(char *buf, int len) {
  if (g_uart_hanlder != NULL) {
    uart_send(g_uart_hanlder, buf, len);
  } else {
    LOGW(TAG, "g_uart_hanlder uninitialized");
  }
}

static void uart_record_event(aos_dev_t *dev, int event_id, void *priv)
{
  if (event_id == USART_EVENT_READ) {
    aos_event_set(&g_recv_event, 1, AOS_EVENT_OR);
  }
}

static void _record_uart_task(void *args) {
  unsigned int actl_flags = 0;
  char ch[1] = {0};
  int recv_len = 0;
  int i;
  while (true) {
    aos_event_get(&g_recv_event, 1, AOS_EVENT_OR_CLEAR, &actl_flags, AOS_WAIT_FOREVER);
    while (recv_len = uart_recv(g_uart_hanlder, &ch, 1, 0) > 0) {
//      printf("uart recv %d:", recv_len);
//      for (i = 0; i < recv_len; ++i) {
//        printf("0x%02x ", ch[i]);
//      }
//      printf("\n");
      CommProtocolReceiveUartData(&ch, recv_len);
    }
  }
}

Result RecordSaveInit(void) {
  g_uart_hanlder = uart_open_id("uart", 1);
  if (g_uart_hanlder == NULL) {
    LOGE(TAG, "uart open failed");
  }
  uart_config_t config;
  uart_config_default(&config);
  config.baud_rate = 1500000;

  aos_event_new(&g_recv_event, 0);

  uart_config(g_uart_hanlder, &config);
  uart_set_buffer_size(g_uart_hanlder, 256);
  uart_set_event(g_uart_hanlder, uart_record_event, NULL);
  aos_task_t taskhandle;
  aos_task_new_ext(&taskhandle, "record_uart", _record_uart_task, NULL, 1024, 30);

  if (E_OK != CommProtocolInit(_record_uart_write, ChnlReceiveCommProtocolPacket)) {
    LOGE(TAG, "comm protocol init failed");
    uart_close(g_uart_hanlder);
    return E_FAILED;
  }
  if (0 != LasrServiceInit(1)) {
    LOGE(TAG, "lasr service init failed");
    uart_close(g_uart_hanlder);
    return E_FAILED;
  }
  LOGI(TAG, "record init success");
  return E_OK;
}

Result RecordSaveFinal(void) {
  uart_close(g_uart_hanlder);
  CommProtocolFinal();
  return E_OK;
}

