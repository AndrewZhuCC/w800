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
 * Description : uni_adpcm.h
 * Author      : yzs.unisound.com
 * Date        : 2018.06.19
 *
 **************************************************************************/
/* 
 * adpcm.h - include file for adpcm coder. 
 * 
 * Version 1.0, 7-Jul-92. 
 **/ 

#ifndef UTILS_CODEC_SRC_ADPCM_UNI_ADPCM_H_
#define UTILS_CODEC_SRC_ADPCM_UNI_ADPCM_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "uni_iot.h"

typedef struct adpcm_state { 
    uni_s16  valprev;        /* Previous output value */ 
    char     index;          /* Index uni_s32o stepsize table */ 

} adpcm_state_t; 

/*len unit is bytes*/
void adpcm_coder(uni_s16 indata[], char outdata[], uni_s32 len, adpcm_state_t *state);
void adpcm_decoder(char indata[], uni_s16 outdata[], uni_s32 len, adpcm_state_t *state);

#ifdef __cplusplus
}
#endif
#endif  //UTILS_CODEC_SRC_ADPCM_UNI_ADPCM_H_

