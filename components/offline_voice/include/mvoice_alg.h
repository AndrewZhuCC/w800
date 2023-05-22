/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef _MVOICE_ALG_H_
#define _MVOICE_ALG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
    MVC_EVT_VAD_START = 0,
    MVC_EVT_VAD_END,
    MVC_EVT_KWS,
    MVC_EVT_ASR,
} mvoice_event_e;

typedef struct {
    int interleaved;
    uint16_t samples_per_frame;      // sample number per process
    uint16_t sample_freq;
    uint16_t samples_bits;
} alg_data_format_t;

typedef struct {
    int (*init)(int sample_freq, int sample_bits, int samples_per_frame);
    int (*deinit)(void);
    int (*open)(void);
    int (*close)(void);
    int (*start)(void);
    int (*stop)(void);
    uint32_t (*read)(uint8_t *buf, uint32_t count);
} mvoice_sampling_t;

typedef void (* mvoice_event_cb)(mvoice_event_e evt_id, void *data, int size, void *user_data);

typedef void *(*alg_obj_get)(void); 

/**
 * @brief  register voice algorithm
 * @param  [in] alg_get_func          : get function to get specific algorithm
 * @return 0 on success, < 0 on failed
 */
int mvoice_alg_register(alg_obj_get alg_get_func);

/**
 * @brief  register sample functions
 * @param  [in] sample_ops          : user sample operation functions
 * @return 0 on success, < 0 on failed
 */
int mvoice_sample_register(mvoice_sampling_t *sample_ops);

/**
 * @brief  init voice algorithm, and register user callback
 * @param  [in] cb          : algorithm event callback
 * @param  [in] user_data   : user private data
 * @return 0 on success, < 0 on failed
 */
int mvoice_alg_init(mvoice_event_cb cb, void *user_data);

/**
 * @brief  deinit voice algorithm
 * @return 0 on success, < 0 on failed
 */
int mvoice_alg_deinit();

/**
 * @brief  start voice process
 * @param  [in] prio algorithm thread priority
 * @param  [in] p_stack algorithm thread stack，NULL: default
 * @param  [in] stack_size algorithm thread size
 * @return 0 on success, < 0 on failed
 */
int mvoice_process_start(int prio, void *p_stack, size_t stack_size);

/**
 * @brief  stop voice process
 * @return 0 on success, < 0 on failed
 */
int mvoice_process_stop(void);

/**
 * @brief  pause voice process
 * @return 0 on success, < 0 on failed
 */
int mvoice_process_pause(void);

/**
 * @brief  resume voice process
 * @return 0 on success, < 0 on failed
 */
int mvoice_process_resume(void);

#ifdef __cplusplus
}
#endif

#endif