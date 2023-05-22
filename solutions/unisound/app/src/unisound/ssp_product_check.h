/*
 * Copyright 2020 Unisound AI Technology Co., Ltd.
 * Author: Liu Zhiming, Li Yang
 * All Rights Reserved.
 */
#ifndef SSP_PRODUCT_CHECK_SSP_PRODUCT_CHECK_H_
#define SSP_PRODUCT_CHECK_SSP_PRODUCT_CHECK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define SSP_PRODUCT_CHECK_SET_MIN 1
#define SSP_PRODUCT_CHECK_SET_MAX 2

typedef struct tagProductCheckResult {
  int32_t* mic_position_result_status;
  int32_t* energy_result_status;
  int32_t* energy_value;
  int32_t* aec_energy_result_status;
  int32_t* aec_energy_value;
  int32_t* channel_corr_status;
  float* chanel_corr_confidence;
  int32_t doa_angle;
} ProductCheckResult;

/***************************************
** mic_num: mic number
** speaker_num: aec number
** channel_consistency_threshold: 0.6
** doa: reference doa angle
** doa_threshold: bias of doa
** array_type: 0
** mic_distence: cm
** mark: energy_check << 0
**       aec_energy_check << 1
**       channel_check << 2
**       doa_check << 3
****************************************/

void* SspProductCheckInit(int32_t mic_num, int32_t speaker_num,
                          float channel_consistency_threshold, int32_t doa,
                          int32_t doa_threshold, int32_t array_type,
                          int32_t mic_distence, int32_t mark);

int32_t SspProductCheckSet(void* handle, int32_t type, void* args);

int32_t SspProductCheckProcess(void* handle, int16_t* mic_data,
                               int32_t mic_data_size, int16_t* ref_data,
                               int32_t ref_data_size);

int32_t SspProductCheckGetResult(void* handle, ProductCheckResult* out,
                                 int32_t* result);

void SspProductCheckRelease(void* handle);

ProductCheckResult* SspProductCheckGetErrorMic(void* handle);

#ifdef __cplusplus
}
#endif

#endif  //  SSP_PRODUCT_CHECK_SSP_PRODUCT_CHECK_H_
