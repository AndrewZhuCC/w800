/*
 */

#include <string.h>
#include <aos/debug.h>
#include <aos/kernel.h>
#include <string.h>
#include <drv/codec.h>
#include "local_audio.h"
#include "app_sys.h"
#include "user_player.h"
#include <aos/ringbuffer.h>
#include "mp3_decoder.h"
#include "uni_iot.h"
#include "uni_fastresample.h"
#include "uni_share_mem.h"

#define TAG                    "LAUDIO"

#define DEBUG_MP3(...)  //printf(__VA_ARGS__)

#define DAC_BUF_SIZE            (1024)
static uint8_t dac_buf[DAC_BUF_SIZE];

static data_cb g_play_data_cb = NULL;
static finish_cb g_play_finish_cb = NULL;

#define FEED_DATA_THRES         (512)
#define DECODE_PERIOD_SIZE      (1024)
#define MP3_PLAY_START_EVENT    (1 << 0)
#define MP3_PLAY_END_EVENT      (1 << 1)
#define MP3_PLAY_STARTED_EVENT  (1 << 2)
#define MP3_PLAY_STOPPED_EVENT  (1 << 3)


// use share memory, not persistent
typedef struct mp3_decoder_context {
  dev_ringbuf_t             ring;
  uint8_t                   decode_buf[DECODE_PERIOD_SIZE];
  uint8_t                   tmp_decode_buf[1152];
  uint8_t                   tmp_resample_buf[1152];
  uint8_t                   play_buf[512 + 16];
  uint8_t                   ring_buf[2 * 1152 + 1];
  uint8_t                   is_end;
  uint8_t                   is_resample_remain;
  uint8_t                   no_data_left;
  uint32_t                  decode_len;
  uint32_t                  bytes_left;
} mp3_decoder_context_t;

static mp3_decoder_context_t *g_mp3_context = NULL;
static aos_event_t            g_mp3_event;
static RSMPHD                 g_resample;
static uint8_t                g_mp3_run_state = 0;
static uint8_t                g_mp3_force_stop = 0;
static uni_mutex_t            g_mp3_mutex;

int UniPlayStart(void) {
  unsigned int actl_flags = 0;
  uni_pthread_mutex_lock(&g_mp3_mutex);
  if (g_mp3_run_state) {
    LOGW(TAG, "MP3 player is running, skip start");
    uni_pthread_mutex_unlock(&g_mp3_mutex);
    return -1;
  }
  LOGD(TAG, "play start lock");
  if (share_mem_lock(3000)) {
    uni_pthread_mutex_unlock(&g_mp3_mutex);
    LOGE(TAG, "share mem lock failed");
    return -1;
  }
  LOGD(TAG, "play start locked");
  aos_event_set(&g_mp3_event, MP3_PLAY_START_EVENT, AOS_EVENT_OR);
  aos_event_get(&g_mp3_event, MP3_PLAY_STARTED_EVENT, AOS_EVENT_OR_CLEAR, &actl_flags, AOS_WAIT_FOREVER);
  uni_pthread_mutex_unlock(&g_mp3_mutex);
  return 0;
}

int UniPlayStop(void) {
  uni_pthread_mutex_lock(&g_mp3_mutex);
  unsigned int actl_flags = 0;
  if (!g_mp3_run_state) {
    LOGW(TAG, "MP3 player is not running, skip stop");
    uni_pthread_mutex_unlock(&g_mp3_mutex);
    return -1;
  }
  g_mp3_force_stop = 1;
  aos_event_get(&g_mp3_event, MP3_PLAY_STOPPED_EVENT, AOS_EVENT_OR, &actl_flags, AOS_WAIT_FOREVER);
  uni_pthread_mutex_unlock(&g_mp3_mutex);
  return 0;
}

int local_audio_vol_get() {
  int32_t vol = 0;
  csi_codec_input_get_digital_gain((codec_input_t *)1, &vol);
  return vol;
}

int local_audio_vol_set(uint8_t vol) {
  csi_codec_input_set_digital_gain((codec_input_t *)1, vol);
  return 0;
}

static int _inter_start(void) {
  int ret;
  codec_output_config_t output_config;
  output_config.bit_width = 16;
  output_config.sample_rate = 16000;
  output_config.mono_mode_en = 1;

  ret = csi_codec_output_config((codec_output_t *)1, &output_config);
  if (ret < 0) {
    LOGE(TAG, "codec output config failed %d", ret);
    return -1;
  }
  LOGD(TAG, "codec output configed");
  ret = csi_codec_output_start((codec_output_t *)1);
  if (ret < 0) {
    LOGE(TAG, "codec output start failed %d", ret);
    return -1;
  }
  LOGD(TAG, "codec output started");
  if (lvp_mp3_decoder_init() < 0) {
    LOGE(TAG, "mp3 decorder init failed");
    return -1;
  }
  g_resample = resample_init(1152 / 4, 0);
  LOGD(TAG, "inter started");
  return 0;
}

static int _inter_stop(void) {
  int ret = 0;
  resample_release(g_resample);
  lvp_mp3_decoder_deinit();
  ret = csi_codec_output_stop((codec_output_t *)1);
  if (ret < 0) {
    LOGE(TAG, "codec output stop failed %d", ret);
  }
  LOGD(TAG, "play stop unlock");
  share_mem_unlock();
  LOGD(TAG, "play stop unlocked");
  return ret;
}

static void codec_event_cb(int idx, codec_event_t event, void *arg) {
  int ret;
  char *buf = NULL;
  //printf("codec_event_cb event=%d, buf_avail=%d, idx=%d\n", event, csi_codec_output_buf_avail((codec_output_t *)1), idx);
  if (event == CODEC_EVENT_PERIOD_WRITE_COMPLETE || event == CODEC_EVENT_WRITE_BUFFER_EMPTY) {
    if (ringbuffer_available_read_space(&g_mp3_context->ring) >= DAC_BUF_SIZE / 2) {
      if (csi_codec_output_buf_avail((codec_output_t *)1) >= DAC_BUF_SIZE / 2) {
        ringbuffer_read(&g_mp3_context->ring, g_mp3_context->play_buf, DAC_BUF_SIZE / 2);
        csi_codec_output_write((codec_output_t *)1, g_mp3_context->play_buf, DAC_BUF_SIZE / 2);
        //DEBUG_MP3("read out 512, left %d\n", ringbuffer_available_read_space(&g_mp3_context->ring));
      } else {
        printf("csi_codec_output_buf_avail = %d, less than %d\n", csi_codec_output_buf_avail((codec_output_t *)1), DAC_BUF_SIZE / 2);
      }
    } else {
      //printf("set MP3_PLAY_END_EVENT!!!!!!\n");
      aos_event_set(&g_mp3_event, MP3_PLAY_END_EVENT, AOS_EVENT_OR);
    }
  }
}

static void _mp3_decoder_task(void *args) {
  unsigned int actl_flags = 0;
  int ret;
  int read_len = 0;
  int out_len = 0;
  uint8_t *decode_ptr;

  while (true) {
    g_mp3_run_state = 0;
    aos_event_get(&g_mp3_event, MP3_PLAY_START_EVENT, AOS_EVENT_OR_CLEAR, &actl_flags, AOS_WAIT_FOREVER);
    if (_inter_start()) {
      LOGE(TAG, "MP3 player inter start failed");
      break;
    }
    g_mp3_run_state = 1;
    /* clear stopped event */
    aos_event_get(&g_mp3_event, MP3_PLAY_STOPPED_EVENT, AOS_EVENT_OR_CLEAR, &actl_flags, AOS_NO_WAIT);
    g_mp3_force_stop = 0;

    aos_event_set(&g_mp3_event, MP3_PLAY_STARTED_EVENT, AOS_EVENT_OR);
    uint8_t first_start = 0;
    if (g_play_data_cb == NULL) {
      LOGE(TAG, "play_data_cb not registet");
      break;
    }
    g_mp3_context->decode_len = 0;
    g_mp3_context->bytes_left = 0;
    g_mp3_context->is_end = FALSE;
    g_mp3_context->is_resample_remain = FALSE;
    g_mp3_context->no_data_left = FALSE;
    ringbuffer_create(&g_mp3_context->ring, g_mp3_context->ring_buf, sizeof(g_mp3_context->ring_buf));

    read_len = g_play_data_cb(g_mp3_context->decode_buf, DECODE_PERIOD_SIZE);
    if (read_len > 0) {
      if (read_len < DECODE_PERIOD_SIZE) {
        g_mp3_context->no_data_left = TRUE;
      }
      g_mp3_context->bytes_left = read_len;
      MP3FrameInfo info;
      int first_offset = 0;
      lvp_mp3_getinfo(g_mp3_context->decode_buf, g_mp3_context->bytes_left, &first_offset, &info);
      DEBUG_MP3("after lvp_mp3_getinfo byteleft=%d, first_offset=%d\n", g_mp3_context->bytes_left, first_offset);
      if (first_offset > 0) {
        g_mp3_context->bytes_left -= first_offset;
        g_mp3_context->decode_len += first_offset;
      }
    }
    while (true) {
      if (g_mp3_force_stop) {
        LOGD(TAG, "force stop");
        break;
      }
      if (!g_mp3_context->is_end && ringbuffer_available_write_space(&g_mp3_context->ring) < 1152) {
        if (!first_start) {
          ringbuffer_read(&g_mp3_context->ring, g_mp3_context->play_buf, DAC_BUF_SIZE / 2);
          csi_codec_output_write((codec_output_t *)1, g_mp3_context->play_buf, DAC_BUF_SIZE / 2);
          first_start = 1;
          DEBUG_MP3("first start\n");
        } else {
          if (!aos_event_get(&g_mp3_event, MP3_PLAY_END_EVENT, AOS_EVENT_OR_CLEAR, &actl_flags, AOS_NO_WAIT)) {
            ringbuffer_read(&g_mp3_context->ring, g_mp3_context->play_buf, DAC_BUF_SIZE / 2);
            csi_codec_output_write((codec_output_t *)1, g_mp3_context->play_buf, DAC_BUF_SIZE / 2);
            LOGW(TAG, "continue play");
          }
        }
        uni_msleep(5);
        continue;
      }
      if (g_mp3_context->is_resample_remain) {
        resample_run_8k16k(g_resample, 
            (short *)(g_mp3_context->tmp_decode_buf + 1152 / 2),
            1152 / 4, (short *)g_mp3_context->tmp_resample_buf,
            RESP_8K_16K);
        g_mp3_context->is_resample_remain = FALSE;
        ringbuffer_write(&g_mp3_context->ring, g_mp3_context->tmp_resample_buf, out_len);
      } else {
        if (!g_mp3_context->no_data_left && g_mp3_context->bytes_left < FEED_DATA_THRES) {
          uni_memmove(g_mp3_context->decode_buf, &g_mp3_context->decode_buf[g_mp3_context->decode_len], g_mp3_context->bytes_left);
          read_len = g_play_data_cb(&g_mp3_context->decode_buf[g_mp3_context->bytes_left], DECODE_PERIOD_SIZE - g_mp3_context->bytes_left);
          if (read_len < (DECODE_PERIOD_SIZE - g_mp3_context->bytes_left)) {
            g_mp3_context->no_data_left = TRUE;
          }
          g_mp3_context->bytes_left += read_len;
          g_mp3_context->decode_len = 0;
          DEBUG_MP3("feed data byteleft=%d, read_len=%d\n", g_mp3_context->bytes_left, read_len);
        }
        int before_bytes_left = g_mp3_context->bytes_left;
        decode_ptr = &g_mp3_context->decode_buf[g_mp3_context->decode_len];
        out_len = 0;
        ret = lvp_mp3_decode(&decode_ptr, &g_mp3_context->bytes_left, g_mp3_context->tmp_decode_buf, &out_len);
        if (ret < 0 && 1152 != out_len) {
          LOGE(TAG, "decode error ,out len:%d", out_len);
          break;
        }
        DEBUG_MP3("after lvp_mp3_decode byteleft=%d, out_len=%d\n", g_mp3_context->bytes_left, out_len);
        g_mp3_context->decode_len += (before_bytes_left - g_mp3_context->bytes_left);

        if (out_len > 0 && ringbuffer_available_write_space(&g_mp3_context->ring) >= out_len) {
          resample_run_8k16k(g_resample, (short *)g_mp3_context->tmp_decode_buf, out_len / 4, (short *)g_mp3_context->tmp_resample_buf, RESP_8K_16K);
          g_mp3_context->is_resample_remain = TRUE;

          ringbuffer_write(&g_mp3_context->ring, g_mp3_context->tmp_resample_buf, out_len);
        } else {
          LOGW(TAG, "ringbuff write failed, out_len=%d, write_space=%d", out_len, ringbuffer_available_write_space(&g_mp3_context->ring));
          break;
        }
      }
      if (g_mp3_context->no_data_left && g_mp3_context->bytes_left == 0 && !g_mp3_context->is_resample_remain) {
        read_len = g_play_data_cb(g_mp3_context->decode_buf, DECODE_PERIOD_SIZE);
        if (read_len == 0) {
          break;
        } else {
          // play next
          g_mp3_context->no_data_left = (read_len < DECODE_PERIOD_SIZE) ? TRUE : FALSE;
          g_mp3_context->bytes_left = read_len;
          g_mp3_context->decode_len = 0;
          MP3FrameInfo info;
          int first_offset = 0;

          lvp_mp3_getinfo(g_mp3_context->decode_buf, g_mp3_context->bytes_left, &first_offset, &info);
          if (first_offset > 0) {
            g_mp3_context->decode_len += first_offset;
            g_mp3_context->bytes_left -= first_offset;
          }
        }
      }
    }
    if (first_start) {
      aos_event_get(&g_mp3_event, MP3_PLAY_END_EVENT, AOS_EVENT_OR_CLEAR, &actl_flags, AOS_WAIT_FOREVER);
    }
    if (_inter_stop()) {
      LOGE(TAG, "MP3 player inter stop failed");
      break;
    }
    aos_event_set(&g_mp3_event, MP3_PLAY_STOPPED_EVENT, AOS_EVENT_OR);
    if (g_play_finish_cb) {
      g_play_finish_cb();
    }
  }

err:
  LOGE(TAG, "player error");
  app_sys_set_boot_reason(BOOT_REASON_SILENT_RESET);
  aos_reboot();
}

int local_audio_register_cb(data_cb dc, finish_cb fc) {
  g_play_data_cb = dc;
  g_play_finish_cb = fc;
  return 0;
}

int local_audio_init() {
  int ret;
  char *share_mem_base = NULL;
  codec_output_t ao_codec_hdl = {0};

  ao_codec_hdl.buf = dac_buf;
  ao_codec_hdl.buf_size = DAC_BUF_SIZE;
  ao_codec_hdl.period = DAC_BUF_SIZE / 2;
  ao_codec_hdl.cb = codec_event_cb;

  LOGD(TAG, "local_audio_init buf_size=%d, period=%d", ao_codec_hdl.buf_size, ao_codec_hdl.period);
  share_mem_base = (char *)share_mem_get_addr();
  if (NULL == share_mem_base) {
    LOGE(TAG, "share mem get failed");
    return -1;
  }
  g_mp3_context = (mp3_decoder_context_t *)(share_mem_base + 24 * 1024 - 512);

  if (sizeof(mp3_decoder_context_t) > 512 + 6 * 1024) {
    LOGE(TAG, "mp3_decoder_context_t overlap");
    return -1;
  }
  DEBUG_MP3("size of mp3_context=%d\n", sizeof(mp3_decoder_context_t));

  ret = csi_codec_output_open(&ao_codec_hdl);
  if (ret != 0) {
    LOGE(TAG, "dac init open failed %d", ret);
    return ret;
  }

  user_player_init();

  uni_pthread_mutex_init(&g_mp3_mutex);
  aos_event_new(&g_mp3_event, 0);

  aos_task_t taskhandle;
  aos_task_new_ext(&taskhandle, "mp3_decoder", _mp3_decoder_task, NULL, 1024, 30);
  return 0;
}

