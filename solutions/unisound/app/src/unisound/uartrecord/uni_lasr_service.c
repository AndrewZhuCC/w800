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
 * Description : uni_lasr_service.c
 * Author      : shangjinlong@unisound.com
 * Date        : 2020.03.12
 *
 **************************************************************************/
#include "uni_lasr_service.h"
#include "uni_log.h"
#include "uni_iot.h"
#include "uni_lasr_result_parser.h"
#include <dev_ringbuf.h>
#include "uni_channel.h"

#define TAG                "service"

#define RECORD_DATABUF_SIZE   (8 * 1024)

typedef struct {
  uni_u32       vui_session_id;
  dev_ringbuf_t raw_data_record;
  uni_bool      inited;
  uni_bool      push_data;
} LasrService;

static LasrService g_lasr_service = {0};

static void _push_raw_data_2_iot_device(ChnlNoiseReductionPcmData *data) {
  int now;
  static int cost;
  static int avg_speed;
  static int total_len = 0;
  static int start_time = -1;
  static int start;

  if (start_time == -1) {
    start_time = uni_get_clock_time_ms();
    start = start_time;
  }

  now = uni_get_clock_time_ms();
  int ret = ChnlNoiseReductionPcmDataPush(data);
  if (ret == 0) {
    total_len += sizeof(data->data);
    if (now - start >= 5000) {
      cost = (now - start_time) / 1000;
      avg_speed = (int)((float)total_len / (float)(now - start_time) * 1000.0 / 1024.0);
      LOGW(TAG, "total=%dKB, cost=%d-%02d:%02d:%02d, speed=%dKB/s",
         total_len >> 10,
         cost / (3600 * 24),
         cost % (3600 * 24) / 3600,
         cost % (3600 * 24) % 3600 / 60,
         cost % (3600 * 24) % 3600 % 60,
         avg_speed);
      start = now;
    }
  }
}

static void _service_worker_task(void *args) {
  static ChnlNoiseReductionPcmData data;
  data.type = CHNL_MESSAGE_NOISE_REDUCTION_RAW_DATA;

  while (true) {
    while (dev_ringbuf_len(&g_lasr_service.raw_data_record) < sizeof(data.data)) {
      uni_msleep(1);
    }
    dev_ringbuf_out(&g_lasr_service.raw_data_record, data.data, sizeof(data.data));
    if (g_lasr_service.push_data) {
      _push_raw_data_2_iot_device(&data);
    }
  }
}

static void _create_service_task() {
  aos_task_t taskhandle;
  aos_task_new_ext(&taskhandle, "unicomm", _service_worker_task, NULL, 1024, 30);
}

static void _create_raw_databuf() {
  uint8_t *buff = uni_malloc(RECORD_DATABUF_SIZE);
  if (buff == NULL) {
    LOGE(TAG, "create record buf %d failed", RECORD_DATABUF_SIZE);
    return;
  }
  g_lasr_service.raw_data_record.buffer = buff;
  g_lasr_service.raw_data_record.size = RECORD_DATABUF_SIZE;
  g_lasr_service.raw_data_record.read = g_lasr_service.raw_data_record.write = g_lasr_service.raw_data_record.data_len = 0;
}

int LasrGetRecordData(uint8_t *buf, int len) {
  int ret = 0;
  if (g_lasr_service.raw_data_record.buffer != NULL && g_lasr_service.push_data) {
    if (dev_ringbuf_avail(&g_lasr_service.raw_data_record) > len) {
      ret = dev_ringbuf_in(&g_lasr_service.raw_data_record, buf, len);
      if (ret != len) {
        LOGW(TAG, "record in mismatch in=%d, excepted=%d", ret, len);
      }
    } else {
      LOGW(TAG, "record in overflow, send too slow");
    }
  }
  return ret;
}

int LasrPullRawData(int mode) {
  if (!g_lasr_service.inited) {
    LOGW(TAG, "module not init, ingnore cmd[%d]", mode);
    return -1;
  }

  if (mode != 0 && mode != 1) {
    LOGW(TAG, "mode[%d] invalid, must be 0 or 1");
    return -1;
  }

  g_lasr_service.push_data = mode;
  LOGT(TAG, "push data status[%d]", g_lasr_service.push_data);

  return 0;
}

int LasrServiceInit(uni_s64 record) {
  _create_raw_databuf();
  _create_service_task();
  g_lasr_service.inited = true;
  return 0;
}

