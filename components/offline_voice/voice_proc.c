/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <aos/debug.h>
#include <aos/kernel.h>
#include <k_api.h>

#include "mvoice_alg.h"
#include "mvoice_impl.h"

#define TAG "VCPROC"

typedef struct {
    char *data;
    int size;
    int used_size;
} ai_data_chache_t;

#define EVT_ALG_STAT_QUIT   1


typedef struct {
    int alg_running;
    int alg_mute;
    int alg_mute_nested;
    aos_mutex_t mutex;
    aos_event_t alg_state;
    mvoice_sampling_t *sample_ops;
    mvoice_alg_t *ai_alg;
} mvoice_t;

static mvoice_t *g_mvoice;

static int voice_kws_process(char *mic)
{
    CHECK_PARAM(mic && g_mvoice && g_mvoice->ai_alg, -1);

    if (g_mvoice->ai_alg->kws_proc) {
        g_mvoice->ai_alg->kws_proc(mic);
    }

    return 0;
}

static int voice_asr_process(char *mic)
{
    CHECK_PARAM(mic && g_mvoice && g_mvoice->ai_alg, -1);

    if (g_mvoice->ai_alg->asr_proc) {
        g_mvoice->ai_alg->asr_proc(mic);
    }

    return 0;
}

// uint8_t playdata[50*1024];
// uint32_t playdata_len = 0;
static void voice_process_tsk(void *arg)
{
    uint32_t frame_size = g_mvoice->ai_alg->data_format.samples_per_frame * g_mvoice->ai_alg->data_format.samples_bits / 8;
    uint32_t read_size;
    int ret = 0;
    uint8_t *frame_buf;

    frame_buf = (uint8_t *)aos_zalloc_check(frame_size);
    LOGI(TAG, "frame_size=%d", frame_size);
    g_mvoice->alg_running = 1;

	while (g_mvoice->alg_running) {
		
	    // read_size = sample_read(frame_buf, frame_size);
	    read_size = g_mvoice->sample_ops->read(frame_buf, frame_size);
        if (read_size != frame_size) {
            LOGW(TAG, "sample read err return %u", read_size);
            continue;
        }
        
        if (g_mvoice->alg_mute) {
            continue;
        }

        voice_kws_process(frame_buf);
        voice_asr_process(frame_buf);
	}

    aos_free(frame_buf);
    aos_event_set(&g_mvoice->alg_state, EVT_ALG_STAT_QUIT, AOS_EVENT_OR);
}

int mvoice_alg_register(alg_obj_get get_func)
{
    CHECK_PARAM(get_func, -1);

    if (!g_mvoice) {
        g_mvoice = (mvoice_t *)aos_zalloc_check(sizeof(mvoice_t));
    }

    g_mvoice->ai_alg = (mvoice_alg_t *)get_func();

    return 0;
}

int mvoice_sample_register(mvoice_sampling_t *sample_ops)
{
    CHECK_PARAM(sample_ops, -1);

    if (!g_mvoice) {
        g_mvoice = (mvoice_t *)aos_zalloc_check(sizeof(mvoice_t));
    }

    g_mvoice->sample_ops = sample_ops;

    return 0;
}

int mvoice_alg_init(mvoice_event_cb cb, void *user_data)
{
  int ret = 0;
    CHECK_PARAM(cb && g_mvoice && g_mvoice->ai_alg && g_mvoice->sample_ops, -1);

    aos_event_new(&g_mvoice->alg_state, 0);
    aos_mutex_new(&g_mvoice->mutex);
  ret += g_mvoice->ai_alg->init(cb, user_data);
  LOGW(TAG, "samples_per_frame=%d", g_mvoice->ai_alg->data_format.samples_per_frame);
	ret += g_mvoice->sample_ops->init(g_mvoice->ai_alg->data_format.sample_freq,
                g_mvoice->ai_alg->data_format.samples_bits, 
                4 * g_mvoice->ai_alg->data_format.samples_per_frame);
                
  return ret;
}

int mvoice_alg_deinit()
{
    if (g_mvoice) {
        mvoice_process_stop();
        
        if (g_mvoice->ai_alg)
            g_mvoice->ai_alg->deinit();

        if (g_mvoice->sample_ops)
            g_mvoice->sample_ops->deinit();

        aos_event_free(&g_mvoice->alg_state);
        aos_free(g_mvoice);
        g_mvoice = NULL;
    }

    return 0;
}

int mvoice_process_start(int prio, void *p_stack, size_t stack_size)
{
    int ret;

    CHECK_PARAM(g_mvoice && g_mvoice->sample_ops && g_mvoice->ai_alg && prio > 0 && prio < 64 && \
                stack_size > 0, -1);

    if (!g_mvoice->ai_alg) {
        LOGD(TAG, "register an ai algorithm first");
        return -1;
    }

	ret = g_mvoice->sample_ops->open();
    CHECK_RET_TAG_WITH_GOTO(ret == 0, END);

	ret = g_mvoice->sample_ops->start();
    CHECK_RET_TAG_WITH_GOTO(ret == 0, END);

    if (p_stack) {
        static ktask_t alg_tsk_handle;
        return krhino_task_create(&alg_tsk_handle, "algproc", NULL,
                            prio, 0u, (cpu_stack_t *)p_stack,
                            stack_size / 4, voice_process_tsk, 1u);
    } else {
        aos_task_t tsk_alg;
        return aos_task_new_ext(&tsk_alg, "algproc", voice_process_tsk, NULL, stack_size, prio);
    }

    return 0;

END:
    if (g_mvoice->sample_ops) {
        g_mvoice->sample_ops->stop();
        g_mvoice->sample_ops->close(); 
    }
    return ret;
}

int mvoice_process_stop(void)
{
    CHECK_PARAM(g_mvoice && g_mvoice->sample_ops, -1);
    int flags = 0;

    if (g_mvoice->alg_running) {
        kws_stop();
        g_mvoice->alg_running = 0;
        aos_event_get(&g_mvoice->alg_state, EVT_ALG_STAT_QUIT, AOS_EVENT_OR_CLEAR, &flags, AOS_WAIT_FOREVER);

        g_mvoice->sample_ops->stop();
        g_mvoice->sample_ops->close();         
    }
    return 0;
}

int mvoice_process_pause(void)
{
    CHECK_PARAM(g_mvoice && g_mvoice->sample_ops, -1);
    aos_mutex_lock(&g_mvoice->mutex, -1);
    LOGD(TAG, "ai mute [%d]", g_mvoice->alg_mute_nested);

    if (!g_mvoice->alg_mute_nested++) {
        kws_stop();
        // LOGD(TAG, "ai muted");
        g_mvoice->sample_ops->stop();
        g_mvoice->sample_ops->close();
    }
    aos_mutex_unlock(&g_mvoice->mutex);

    return 0;
}

int mvoice_process_resume(void)
{
    CHECK_PARAM(g_mvoice && g_mvoice->sample_ops, -1);
    int ret;
    
    aos_mutex_lock(&g_mvoice->mutex, -1);
    LOGD(TAG, "ai unmute [%d]", g_mvoice->alg_mute_nested);

    if (g_mvoice->alg_mute_nested) {
#if 0 //use alg_mute_nested
        if (!--g_mvoice->alg_mute_nested) {
            // LOGD(TAG, "ai unmuted");
            // g_mvoice->alg_mute = 0;
            ret = g_mvoice->sample_ops->open();
            aos_check_return_einval(ret == 0);

            ret = g_mvoice->sample_ops->start();
            aos_check_return_einval(ret == 0);
        }
#else
        g_mvoice->alg_mute_nested = 0;
        // LOGD(TAG, "ai unmuted");
        // g_mvoice->alg_mute = 0;
        ret = g_mvoice->sample_ops->open();
        // aos_check_return_einval(ret == 0);
        if (ret) {
          goto err;
        }

        ret = g_mvoice->sample_ops->start();
        // aos_check_return_einval(ret == 0);
        if (ret) {
          goto err;
        }
#endif
        kws_relaunch();
    }
    aos_mutex_unlock(&g_mvoice->mutex);
    return 0;

err:
    aos_mutex_unlock(&g_mvoice->mutex);
    return ret;
}
