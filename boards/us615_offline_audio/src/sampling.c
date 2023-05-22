/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include <aos/debug.h>
#include <aos/kernel.h>
#include <drv/codec.h>
#include <mvoice_alg.h>

#define TAG                    "sampling"

// #define SAMPLE_BUF_SIZE  (16 * 2 * 60)      // 60 ms buffer

#define EVT_SAMPLE_READ_AVAILABLE  1
#define EVT_SAMPLE_BUF_FULL        (1 << 1)

typedef struct {
    int freq;
    int bits;
    int frame_size;
} sample_info_t;

// static uint8_t sample_buf[SAMPLE_BUF_SIZE];
static uint8_t *sample_buf;
static sample_info_t sample_info;
static aos_event_t evt_sample_input;
static codec_input_t sample_codec_hdl;
// static int ao_first_write = 0;      // need to buffer sufficient data when first

static int sample_init(int sample_freq, int sample_bits, int samples_per_frame);
static int sample_open(void);
static int sample_start(void);
static uint32_t sample_read(uint8_t *buf, uint32_t count);
static int sample_stop(void);
static int sample_close(void);

static void codec_rec_event_cb(int idx, codec_event_t event, void *arg)
{
    /* read complete event triggers when half of the buffer is received, 30 ms in this case */
    if (event == CODEC_EVENT_PERIOD_READ_COMPLETE) {
        aos_event_set(&evt_sample_input, EVT_SAMPLE_READ_AVAILABLE, AOS_EVENT_OR);
    } else if (event == CODEC_EVENT_READ_BUFFER_FULL) {
        // printf("sample buf full %u\n", csi_codec_input_buf_avail(&sample_codec_hdl));
        aos_event_set(&evt_sample_input, EVT_SAMPLE_BUF_FULL, AOS_EVENT_OR);
    }
}

static int sample_init(int sample_freq, int sample_bits, int samples_per_frame)
{
    aos_check_return_einval(sample_freq == 16000 && sample_bits == 16 && samples_per_frame > 0);

    aos_event_new(&evt_sample_input, 0);

    sample_info.freq = sample_freq;
    sample_info.bits = sample_bits;
    sample_info.frame_size = sample_bits / 8 * samples_per_frame;
    sample_buf = (uint8_t *)aos_zalloc_check(sample_info.frame_size * 2);  // buffer size need to be twice of the frame size
    LOGI(TAG, "samples_per_frame=%d", samples_per_frame);
    LOGI(TAG, "freq=%d, bits=%d, frame_size=%d, buf_size=%d", sample_info.freq, sample_info.bits, sample_info.frame_size, sample_info.frame_size * 2);

    return 0;
}

static int sample_deinit()
{
    aos_free(sample_buf);
    aos_event_free(&evt_sample_input);
}

static int sample_open(void)
{
    int ret;

	sample_codec_hdl.cb = codec_rec_event_cb;
	sample_codec_hdl.buf = sample_buf;
	sample_codec_hdl.buf_size = sample_info.frame_size * 2;
	sample_codec_hdl.period = sample_info.frame_size;

	codec_input_config_t input_config;
	input_config.bit_width = sample_info.bits;
	input_config.sample_rate = sample_info.freq;
	input_config.channel_num = 1;

	ret = csi_codec_input_open(&sample_codec_hdl);
    CHECK_RET_TAG_WITH_GOTO(ret == 0, ERR);

	ret = csi_codec_input_config(&sample_codec_hdl, &input_config);
    CHECK_RET_TAG_WITH_GOTO(ret == 0, ERR);

    // csi_codec_input_set_digital_gain(&sample_codec_hdl, 50);

    LOGD(TAG, "sample open success, bit width %d, sample rate %d", sample_info.bits, sample_info.freq);
    return 0;

ERR:
    LOGD(TAG, "sample open failed");
    sample_stop();
    sample_close();
    return -1;
}

static int sample_start(void)
{
	int ret = csi_codec_input_start(&sample_codec_hdl);
    if (ret < 0) {
        LOGD(TAG, "codec input start failed");
        // sample_stop();
        // sample_close();
        // return -1;
    }

    // aos_event_set(&evt_sample_input, EVT_SAMPLE_READ_AVAILABLE, AOS_EVENT_OR);
    LOGD(TAG, "codec sampling start success");
    return 0;
}

// char g_input_buf[16 * 2 * 1000];
// size_t g_input_buf_offset = 0;
// static int cnt = 0;
static uint32_t sample_read(uint8_t *buf, uint32_t count)
{
    unsigned int flags = 0;
    uint32_t ret = 0;

    if (csi_codec_input_buf_avail(&sample_codec_hdl) >= count) {
        // LOGD(TAG, "input read 2 %d", csi_codec_input_buf_avail(&sample_codec_hdl));
        //aos_event_get(&evt_sample_input, EVT_SAMPLE_READ_AVAILABLE | EVT_SAMPLE_BUF_FULL, AOS_EVENT_OR_CLEAR, &flags, AOS_NO_WAIT);
        ret = csi_codec_input_read(&sample_codec_hdl, buf, count);
    } else {
        while (1) {
            aos_event_get(&evt_sample_input, EVT_SAMPLE_READ_AVAILABLE | EVT_SAMPLE_BUF_FULL, AOS_EVENT_OR_CLEAR, &flags, AOS_WAIT_FOREVER);
            if (((flags & EVT_SAMPLE_READ_AVAILABLE) || (flags & EVT_SAMPLE_BUF_FULL)) && (csi_codec_input_buf_avail(&sample_codec_hdl) >= count)) {
                // LOGD(TAG, "input read %d", csi_codec_input_buf_avail(&sample_codec_hdl));
                ret = csi_codec_input_read(&sample_codec_hdl, buf, count);
                break;
            } else {
                // LOGW(TAG, "sample read evt %d %u", flags, csi_codec_input_buf_avail(&sample_codec_hdl));
            }
        }
    }

    // if (++cnt > 130 && g_input_buf_offset + ret <= sizeof(g_input_buf)) {
    //     // cnt = 101;
    //     memcpy(g_input_buf + g_input_buf_offset, buf, ret);
    //     g_input_buf_offset += ret;
    // }

    if (ret == 0) {
        LOGD(TAG, "codec read failed");
        sample_stop();
        sample_close();
    }

    return ret;
}

static int sample_stop(void)
{
    csi_codec_input_stop(&sample_codec_hdl);
}

static int sample_close(void)
{
    csi_codec_input_close(&sample_codec_hdl);
}

mvoice_sampling_t us615_sample_ops = {
    .init = sample_init,
    .deinit = sample_deinit,
    .open = sample_open,
    .close = sample_close,
    .start = sample_start,
    .stop = sample_stop,
    .read = sample_read
};
