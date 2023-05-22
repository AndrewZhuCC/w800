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
 * Description : uni_log.h
 * Author      : wufangfang@unisound.com
 * Date        : 2019.09.10
 *
 **************************************************************************/

#ifndef HAL_INC_UNI_LOG_H_
#define HAL_INC_UNI_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "uni_types.h"
#include "uni_osal.h"
#include "ulog/ulog.h"

//#define LOG(level, tag, fmt, ...) do { \
//  u32 time = tls_os_get_time();	                 \
//  printf("<%d.%02d>[%s]%s:"fmt"\n", (time/100), (time%100), tag, __func__ , ##__VA_ARGS__); \
//} while (0)

#ifndef LOGD
#define LOGD(tag, ...) ulog(LOG_DEBUG, tag, ULOG_TAG, __VA_ARGS__)
#endif
#ifndef LOGT
#define LOGT(tag, ...) ulog(LOG_INFO, tag, ULOG_TAG, __VA_ARGS__)
#endif
#ifndef LOGW
#define LOGW(tag, ...) ulog(LOG_WARNING, tag, ULOG_TAG, __VA_ARGS__)
#endif
#ifndef LOGE
#define LOGE(tag, ...) ulog(LOG_ERR, tag, ULOG_TAG, __VA_ARGS__)
#endif

#if defined(CONFIG_ARPT_PRINT) && CONFIG_ARPT_PRINT
#define ArptPrint(...)     printf( __VA_ARGS__ )
#else
#define ArptPrint(...)
#endif

#ifdef __cplusplus
}

#endif
#endif /*HAL_INC_UNI_LOG_H_*/
