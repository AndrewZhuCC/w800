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
 * Description : uni_lasr_service.h
 * Author      : shangjinlong@unisound.com
 * Date        : 2020.03.12
 *
 **************************************************************************/

#ifndef SDK_SERVICE_INC_UNI_LASR_SERVICE_H_
#define SDK_SERVICE_INC_UNI_LASR_SERVICE_H_
#include "uni_iot.h"

#ifdef __cplusplus
extern "C" {
#endif

int LasrServiceInit(uni_s64 record);
int LasrPullRawData(int mode);
int LasrGetRecordData(uint8_t *buf, int len);
void LasrClearRawdataBuf(void);

#ifdef __cplusplus
}
#endif
#endif  // SDK_SERVICE_INC_UNI_LASR_SERVICE_H_
