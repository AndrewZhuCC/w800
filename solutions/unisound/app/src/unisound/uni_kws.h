#ifndef __UNI_KWS_H__
#define __UNI_KWS_H__

typedef enum {
  KWS_WORK_INIT = 0,
  KWS_WORK_IDLE,
  KWS_WORK_RUNNING
}kws_work_state;

typedef enum {
  ENGINE_KWS_STA_CONTINUE = 0,
  ENGINE_KWS_STA_RESULT,
  ENGINE_KWS_STA_ERROR
}engine_kws_status;

typedef enum {
  ENGINE_KWS_MODE_WAKEUP = 0,
  ENGINE_KWS_MODE_CMD,
  ENGINE_KWS_MODE_INVALID
}engine_kws_mode;

typedef struct kws_result_msg {
  float         score;
  int32_t       event_id;
  char          command[256];
} kws_result_msg_t;

#define CAPTURE_SAMPLE_BYTE       2
#define CAPTURE_SAMPLE_RATE       16000
#define CAPTURE_SAMPLE_LEN_MS     (CAPTURE_SAMPLE_RATE / 1000)
#define CAPTURE_FRAME_MS          16
#define CAPTURE_FRAME_LEN_1CH     (CAPTURE_FRAME_MS * CAPTURE_SAMPLE_LEN_MS)
#define CAPTURE_FRAME_SIZE_1CH    (CAPTURE_FRAME_LEN_1CH * CAPTURE_SAMPLE_BYTE)

#define KWS_QUEUE_SIZE          3
#define KWS_QUEUE_BUF_SIZE      sizeof(kws_result_msg_t) * KWS_QUEUE_SIZE
#endif
