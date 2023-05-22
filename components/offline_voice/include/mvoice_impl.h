/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef _MVOICE_IMPL_H_
#define _MVOICE_IMPL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mvoice_alg.h"

typedef struct {
    char *name;
    alg_data_format_t data_format;
    int (*init)(mvoice_event_cb cb, void *user_data);
    int (*deinit)();
    int (*kws_proc)(void *voice_data);          // for offline process
    int (*asr_proc)(void *voice_data);          // for offline process
} mvoice_alg_t;

#define VAG_CB(evt, data, size, user_data) do { if (mvoice_alg_callback) mvoice_alg_callback(evt, data, size, user_data); } while(0)


#ifdef __cplusplus
}
#endif

#endif