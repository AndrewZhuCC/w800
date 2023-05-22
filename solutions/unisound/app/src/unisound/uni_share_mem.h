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

#ifndef UNISOUND_UNI_SHARE_MEM_H_
#define UNISOUND_UNI_SHARE_MEM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "uni_iot.h"

int share_mem_init(void);
uint8_t *share_mem_get_addr(void);
int share_mem_get_size(void);
int share_mem_lock(int timeout_ms);
int share_mem_unlock(void);

#ifdef __cplusplus
}
#endif
#endif  //  UNISOUND_UNI_SHARE_MEM_H_
