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
 * Description : Uniapp uni_share_mem.h
 * Author      : unisound.com
 * Date        : 2021.9.7
 *
 **************************************************************************/

#include "uni_share_mem.h"
#include "uni_log.h"

#define TAG "SHARE_MEM"

#define KWS_DECODER_BUF_SIZE (30 * 1024)
uint8_t g_kws_share_mem[KWS_DECODER_BUF_SIZE] __attribute__((aligned(16)));

typedef struct kws_share_mem {
  uint8_t   *buf;
  int       size;
  uni_sem_t sem;
  uint8_t   inited;
}kws_share_mem_t;

static kws_share_mem_t g_share_mem = {0};


int share_mem_init(void) {
  if (g_share_mem.inited) {
    LOGE(TAG, "share mem already inited");
    return -1;
  }
  g_share_mem.buf = g_kws_share_mem;
  g_share_mem.size = sizeof(g_kws_share_mem);
  uni_sem_init(&g_share_mem.sem, 1);
  g_share_mem.inited = 1;
  return 0;
}

uint8_t *share_mem_get_addr(void) {
  if (g_share_mem.inited) {
    return g_share_mem.buf;
  } else {
    return NULL;
  }
}

int share_mem_get_size(void) {
  if (g_share_mem.inited) {
    return g_share_mem.size;
  } else {
    return 0;
  }
}

int share_mem_lock(int timeout_ms) {
  if (g_share_mem.inited) {
    if (timeout_ms) {
      int ret = uni_sem_wait_ms(g_share_mem.sem, timeout_ms);
      if (ret) {
        LOGE(TAG, "share mem lock timeout[%d]:%d", timeout_ms, ret);
      }
      return ret;
    } else {
      uni_sem_wait(g_share_mem.sem);
      return 0;
    }
  } else {
    return -1;
  }
}

int share_mem_unlock(void) {
  if (g_share_mem.inited) {
    uni_sem_post(g_share_mem.sem);
    return 0;
  } else {
    return -1;
  }
}

