/**
 * Copyright 2020, unisound.com. All rights reserved.
 */

#include <stdint.h>
#include <stdlib.h>
#include "common.h"
#include "app_sys.h"
#include "ofa_consts.h"
#include "osal/osal-log.h"
#include "osal/osal-time.h"
#include "ual_ofa.h"
#include "kws_grammar.h"
#include "asrfix.h"
#include "uni_osal.h"
#include "uni_mem.h"
#include "cJSON.h"

#include "uni_cust_config.h"
#include "uni_json.h"
#include "uni_nlu.h"
#include "uni_lasr_result_parser.h"
#include "uni_share_mem.h"
#include "uni_iot.h"
#include <mvoice_alg.h>
#include "mvoice_impl.h"
#include "iot_dispatcher.h"
#include "uni_kws.h"
#include "uni_lasr_service.h"
#include "user_player.h"
#include "uni_auto_control.h"
#include "ssp_product_check.h"

#define KWS_TAG "KWS"

static mvoice_event_cb mvoice_alg_callback;
static void *g_user_data;

#ifdef KWS_WAKEUP_SCORE_THRED
#define WUW_STD_THRES   (KWS_WAKEUP_SCORE_THRED)
#else
#define WUW_STD_THRES   (-1.35f)
#endif
#ifdef KWS_SLEEP_SCORE_THRED
#define WUW_SLEEP_THRES KWS_SLEEP_SCORE_THRED
#else
#define WUW_SLEEP_THRES (WUW_STD_THRES + 2.0f)
#endif
#define WUW_LOW_THRES   (-10.0f)

#ifdef KWS_CMD_SCORE_THRED
#define CMD_STD_THRES   (KWS_CMD_SCORE_THRED)
#else
#define CMD_STD_THRES   (1.56f)
#endif
#define CMD_LOW_THRES   (-10.0f)

#define KWS_CMD_TIMEOUT_MS       (UNI_ASR_TIMEOUT * 1000)
#define KWS_WUW_SLEEP_TIMEOUT_MS (60000)

#ifdef KWS_HASHTABLE_SIZE
#define KEYWORD_NUM     KWS_HASHTABLE_SIZE
#else
#define KEYWORD_NUM     50
#endif

#define KWS_EVT_START   (1 << 0)
#define KWS_EVT_STOP    (1 << 1)

#if defined(CONFIG_DEBUG) && CONFIG_DEBUG
#define DEBUG_RTF 1
#else
#define DEBUG_RTF 0
#endif

#define KWS_DATA_MONITOR

/* KWS work context */
typedef struct kws_context {
  HANDLE                    kws;
  kws_work_state            state;
  engine_kws_mode           engine_mode;
  engine_kws_mode           last_mode;
  uni_bool                  running;
  uni_bool                  working;
  uni_sem_t                 started_sem;
  uni_sem_t                 stopped_sem;
  uni_mutex_t               mutex;
  aos_event_t               event;
  uni_u32                   timeout;
  uni_u32                   sleep_timeout;
  uni_u32                   cur_timestamp;
  aos_queue_t               kws_queue;
  void                     *kws_queue_buf;
  float                     std_thres;
  int                       am_id;
  char                      cmd[32];
}kws_context_t;

static kws_context_t g_kws_context = {0};

#if DEBUG_RTF
static int64_t engine_time = 0.0;
static int64_t wav_time = 0.0;
static uint32_t rtf_cnt = 1000;
#endif

static uni_err_t _kws_semaphore_init(void) {

  uni_pthread_mutex_init(&g_kws_context.mutex);
  uni_sem_init(&g_kws_context.started_sem, 0);
  uni_sem_init(&g_kws_context.stopped_sem, 0);
  aos_event_new(&g_kws_context.event, 0);
  return UNI_NO_ERR;
}

int kws_state(void) {
  return g_kws_context.state;
}

bool kws_is_running(void) {
  return (KWS_WORK_RUNNING == g_kws_context.state);
}

uni_err_t kws_switch_mode(engine_kws_mode mode) {
  LOGD(KWS_TAG, "kws switch");
  uni_pthread_mutex_lock(&g_kws_context.mutex);
  if (KWS_WORK_RUNNING != g_kws_context.state) {
    LOGD(KWS_TAG, "kws is not running, skip switch");
    uni_pthread_mutex_unlock(&g_kws_context.mutex);
    return UNI_INVALID_STATE_ERR;
  }
  if (g_kws_context.engine_mode == mode) {
    LOGD(KWS_TAG, "same mode, skip switch");
    uni_pthread_mutex_unlock(&g_kws_context.mutex);
    return UNI_NO_ERR;
  }
  LOGD(KWS_TAG, "switch to mode %d", mode);
  g_kws_context.last_mode = g_kws_context.engine_mode;
  g_kws_context.engine_mode = mode;
  g_kws_context.sleep_timeout = 0;
  g_kws_context.timeout = 0;
  g_kws_context.cur_timestamp = 0;
  aos_event_set(&g_kws_context.event, KWS_EVT_STOP, AOS_EVENT_OR);
  uni_sem_wait(g_kws_context.stopped_sem);

  // skip re-lock
  //share_mem_unlock();
  //share_mem_lock(0);
  aos_event_set(&g_kws_context.event, KWS_EVT_START, AOS_EVENT_OR);
  uni_sem_wait(g_kws_context.started_sem);
  g_kws_context.state = KWS_WORK_RUNNING;
  LOGD(KWS_TAG, "kws switch done");
  uni_pthread_mutex_unlock(&g_kws_context.mutex);
  return UNI_NO_ERR;
}

uni_err_t kws_relaunch(void) {
  LOGD(KWS_TAG, "kws relaunch");
  uni_pthread_mutex_lock(&g_kws_context.mutex);
  if (KWS_WORK_IDLE != g_kws_context.state) {
    uni_pthread_mutex_unlock(&g_kws_context.mutex);
    LOGD(KWS_TAG, "kws is running, skip relaunch");
    return UNI_INVALID_STATE_ERR;
  }
  LOGD(KWS_TAG, "kws relaunch lock");
  if (share_mem_lock(0)) {
    uni_pthread_mutex_unlock(&g_kws_context.mutex);
    LOGD(KWS_TAG, "kws relaunch lock failed");
    return UNI_MODULE_BUSY_ERR;
  }
  LOGD(KWS_TAG, "kws relaunch locked");
  if (uni_strstr(g_kws_context.cmd, "wakeup_uni") != NULL) {
    g_kws_context.engine_mode = ENGINE_KWS_MODE_CMD;
  } else if (uni_strstr(g_kws_context.cmd, "exitUni") != NULL) {
    g_kws_context.engine_mode = ENGINE_KWS_MODE_WAKEUP;
  }
  memset(g_kws_context.cmd, 0, sizeof(g_kws_context.cmd));
  g_kws_context.cur_timestamp = 0;
  aos_event_set(&g_kws_context.event, KWS_EVT_START, AOS_EVENT_OR);
  uni_sem_wait(g_kws_context.started_sem);
  g_kws_context.state = KWS_WORK_RUNNING;
  LOGD(KWS_TAG, "kws relaunch done");
  uni_pthread_mutex_unlock(&g_kws_context.mutex);
  return UNI_NO_ERR;
}

uni_err_t kws_start(void) {
  LOGD(KWS_TAG, "kws start");
  uni_pthread_mutex_lock(&g_kws_context.mutex);
  if (KWS_WORK_IDLE != g_kws_context.state) {
    uni_pthread_mutex_unlock(&g_kws_context.mutex);
    LOGD(KWS_TAG, "kws is running, skip start");
    return UNI_INVALID_STATE_ERR;
  }
  LOGD(KWS_TAG, "kws start lock");
  if (share_mem_lock(0)) {
    uni_pthread_mutex_unlock(&g_kws_context.mutex);
    LOGD(KWS_TAG, "kws start lock failed");
    return UNI_MODULE_BUSY_ERR;
  }
  LOGD(KWS_TAG, "kws start locked");
  g_kws_context.cur_timestamp = 0;
  aos_event_set(&g_kws_context.event, KWS_EVT_START, AOS_EVENT_OR);
  uni_sem_wait(g_kws_context.started_sem);
  g_kws_context.state = KWS_WORK_RUNNING;
  LOGD(KWS_TAG, "kws start done");
  uni_pthread_mutex_unlock(&g_kws_context.mutex);
  return UNI_NO_ERR;
}

uni_err_t kws_stop(void) {
  LOGD(KWS_TAG, "kws stop");
  uni_pthread_mutex_lock(&g_kws_context.mutex);
  if (KWS_WORK_RUNNING != g_kws_context.state) {
    uni_pthread_mutex_unlock(&g_kws_context.mutex);
    LOGD(KWS_TAG, "kws is not running, skip stop");
    return UNI_INVALID_STATE_ERR;
  }
  g_kws_context.timeout = 0;
  g_kws_context.sleep_timeout = 0;
  aos_event_set(&g_kws_context.event, KWS_EVT_STOP, AOS_EVENT_OR);
  uni_sem_wait(g_kws_context.stopped_sem);
  g_kws_context.state = KWS_WORK_IDLE;
  LOGD(KWS_TAG, "kws stop unlock");
  if (share_mem_unlock()) {
    uni_pthread_mutex_unlock(&g_kws_context.mutex);
    LOGD(KWS_TAG, "kws stop unlock failed");
    return UNI_MODULE_BUSY_ERR;
  }
  LOGD(KWS_TAG, "kws stop unlocked");
  LOGD(KWS_TAG, "kws stop done");
  uni_pthread_mutex_unlock(&g_kws_context.mutex);
  return UNI_NO_ERR;
}

static int uni_pcm_play(const char* pcm, const char* cmd) {
  uni_strcpy(g_kws_context.cmd, cmd);
  user_player_play(pcm);
  return 0;
}

static int uni_pcm_play_order(const char* pcm, const char* cmd) {
  uni_strcpy(g_kws_context.cmd, cmd);
  user_player_reply_list_in_order(pcm);
  return 0;
}

static int uni_pcm_play_ramdom(const char* pcm, const char* cmd) {
  uni_strcpy(g_kws_context.cmd, cmd);
  user_player_reply_list_random(pcm);
  return 0;
}

void kws_audio_init_done(void) {
  #ifdef DEFAULT_PCM_WELCOME
  LOGD(KWS_TAG, "play welcome start");
  uni_pcm_play(DEFAULT_PCM_WELCOME, "exitUni");
  #else
  kws_start();
  #endif
}

static void _write_arpt_log(const char *command, float score, const char *json,
                                char is_wakeup, char is_basic, char is_rasr) {
#if defined(CONFIG_ARPT_PRINT) && CONFIG_ARPT_PRINT
  char buffer_log[256] = {0};
  char tag[16] = {0};
  char result[16] = {0};
  char kws[8] = {0};
  char pure_cmd[64] = {0};
  char is_more = !(is_basic || is_rasr);
  /* basic content */
  snprintf(tag, sizeof(tag), "%s_%s", is_wakeup ? "asr" : "wakeup",
               "normal");
  snprintf(kws, sizeof(kws), "%s", is_basic ? "KWS\t" : "");
  snprintf(result, sizeof(result), "%s", is_rasr ? "online_json" : "offline_result");
  if (is_rasr) {
    snprintf(buffer_log, sizeof(buffer_log), "%s%s:[%s]\t", kws, result, tag);
  } else {
    //StrSubEx(pure_cmd, command, ">", "</");
    snprintf(pure_cmd, sizeof(pure_cmd), "%s", command);
    snprintf(buffer_log, sizeof(buffer_log), "%s%s:[%s]\tcommand[%s]\tscore[%.02f]\t",
             kws, result, tag, pure_cmd, score);
  }
  // ArptPrint(buffer_log);
  /* more content */
  if (!is_more) {
    // ArptPrint(json);
  }
  // ArptPrint("\n");
#endif
}

static uni_err_t _check_kws_timeout(void) {
  if ((0 != g_kws_context.sleep_timeout) &&
      (g_kws_context.sleep_timeout == g_kws_context.cur_timestamp)) {
    // LOGT(KWS_TAG, "kws goto sleep");
    g_kws_context.std_thres = WUW_SLEEP_THRES;
  }
  if ((0 != g_kws_context.timeout) &&
      (g_kws_context.timeout == g_kws_context.cur_timestamp)) {
    // LOGT(KWS_TAG, "kws timeout");
    // printf("---------kws timeout--------");
    kws_result_msg_t kws_msg = {0};
    uni_memset(&kws_msg, 0, sizeof(kws_result_msg_t));
    kws_msg.event_id = VUI_LOCAL_ASR_TIMEOUT_EVENT;
    aos_queue_send(&g_kws_context.kws_queue, (void *)&kws_msg, sizeof(kws_result_msg_t));
  }
  return UNI_NO_ERR;
}

static Result _lasr_check_score(float score) {
  if (score >= g_kws_context.std_thres) {
    return E_OK;
  }
  return E_FAILED;
}

static void _assign_nlu_type(cJSON *jnlu, kws_result_msg_t *kws_result) {
  char *cmd = NULL;
  char *pcm = NULL;
  uint8_t need_reply = 1;
  uint8_t need_post = 1;
  int is_play = 0;
  int ret;

  if (0 != JsonReadItemString(jnlu, "pcm", &pcm)) {
    goto L_END;
  }
  if (0 != JsonReadItemString(jnlu, "cmd", &cmd)) {
    goto L_END;
  }
  _write_arpt_log(kws_result->command, kws_result->score, cmd, g_kws_context.engine_mode, 1, 0);
  ret = user_gpio_handle_action_event(cmd, &need_reply, &need_post);
  if (need_reply) {   //quick replay
    if (uni_pcm_play_ramdom(pcm, cmd) == 0) {
      is_play = 1;
    }
  }
  if (need_post && ret == 0) {
    if (iotdispatcher_voice_set(cmd) != 0) {
      LOGW(KWS_TAG, "iot dispatcher %s not found", cmd);
    }
  }
  if (is_play) {
    goto L_END;
  }
  if (0 == uni_strcmp(cmd, "volumeUpUni")
      || 0 == uni_strcmp(cmd, "volumeDownUni")
      || 0 == uni_strcmp(cmd, "volumeMinUni")
      || 0 == uni_strcmp(cmd, "volumeMaxUni")
      || 0 == uni_strcmp(cmd, "volumeMidUni")) {
    //TBD
    goto L_END;
  }
  if (uni_strstr(cmd, "wakeup_uni") != NULL) {
    kws_switch_mode(ENGINE_KWS_MODE_CMD);
    goto L_END;

  }
  if (uni_strstr(cmd, "exitUni") != NULL) {
    kws_switch_mode(ENGINE_KWS_MODE_WAKEUP);
    goto L_END;
  }
L_END:
  if (cmd) {
    uni_free(cmd);
  }
  if (pcm) {
    uni_free(pcm);
  }
}

static void ShowResourceInfo(void* kws) {
  UalOFAPrintResourceInfo(kws);

  const char* default_lang =
      UalOFAGetOptionString(kws, KWS_CURRENT_LANGUAGE_ID);

  int default_am_id = UalOFAGetActiveAmId(kws);

  const char* default_grammar_domain = UalOFAGetActiveGrammarDomain(kws);

  printf("Default Info:\n");
  printf("AmID: %d\n", default_am_id);
  printf("Language: %s\n", default_lang);
  printf("GrammarDomain: %s\n", default_grammar_domain);
}

static void HandleKwsMsgTaskProc(void *param)
{
  kws_result_msg_t kws_result = {0};
  tls_os_status_t status = TLS_OS_SUCCESS;
  cJSON *jnlu = NULL;
  int recv_size;

  for(;;)
  {
    status = aos_queue_recv(&g_kws_context.kws_queue, AOS_WAIT_FOREVER, &kws_result, &recv_size);
    if (status == 0 && recv_size == sizeof(kws_result_msg_t)) {
      LOGD(KWS_TAG,"recv result:%s, score:%.2f", kws_result.command, kws_result.score);
      do {
        if (kws_result.event_id == VUI_LOCAL_ASR_TIMEOUT_EVENT && kws_is_running()) {
          #ifdef DEFAULT_PCM_ASR_TIMEOUT
          uni_pcm_play(DEFAULT_PCM_ASR_TIMEOUT, "exitUni");
          #else
          kws_switch_mode(ENGINE_KWS_MODE_WAKEUP);
          #endif
          break;
        }
        if (NULL == (jnlu = NluParseLasr(kws_result.command))) {
          break;
        }
        _assign_nlu_type(jnlu, &kws_result);
      } while(0);
      if (jnlu != NULL) {
        cJSON_Delete(jnlu);
        jnlu = NULL;
      }
    } else {
      LOGE(KWS_TAG,"kws msg err status = %d, recv_size = %d[%d]", status, recv_size, sizeof(kws_result_msg_t));
    }
  }
}

static int _internal_kws_stop(void) {
  UalOFAStop(g_kws_context.kws);
  LOGI(KWS_TAG, "inter kws stop");
  g_kws_context.working = FALSE;
  uni_sem_post(g_kws_context.stopped_sem);
}

static int _internal_kws_start(void) {
  int status = 0;
  void* decoder_pool = share_mem_get_addr();
  int decoder_size = share_mem_get_size();
  if (NULL == decoder_pool || 0 == decoder_size) {
    LOGE(KWS_TAG, "decoder_pool set failed.");
    return -1;
  }

  if (g_kws_context.engine_mode == ENGINE_KWS_MODE_WAKEUP) {
    UalOFASetOptionInt(g_kws_context.kws, KWS_BEAM_ID, 10);
    status =
      UalOFAStart(g_kws_context.kws, "wakeup", g_kws_context.am_id, decoder_pool, decoder_size);
    g_kws_context.std_thres = WUW_STD_THRES;
    g_kws_context.sleep_timeout = KWS_WUW_SLEEP_TIMEOUT_MS / 16;
    // ArptPrint("enter wakeup_normal\n");
  } else {
    if (KEYWORD_NUM < 110) {
      UalOFASetOptionInt(g_kws_context.kws, KWS_BEAM_ID, 10);
    } else {
      UalOFASetOptionInt(g_kws_context.kws, KWS_BEAM_ID, 8);
    }
    status =
      UalOFAStart(g_kws_context.kws, "ivm", g_kws_context.am_id, decoder_pool, decoder_size);
    g_kws_context.std_thres = CMD_STD_THRES;
    g_kws_context.timeout = KWS_CMD_TIMEOUT_MS / 16;
    // ArptPrint("enter asr_normal\n");
  }

  if (status != ASR_RECOGNIZER_OK) {
    LOGE(KWS_TAG, "Start Engine failed.");
    return -1;
  }
  LOGI(KWS_TAG, "kws start in %d mode", g_kws_context.engine_mode);
  g_kws_context.working = TRUE;
  uni_sem_post(g_kws_context.started_sem);
  return 0;
}

static int _internal_kws_init(void) {
  if (KWS_WORK_INIT != g_kws_context.state) {
    return UNI_INVALID_STATE_ERR;
  }
  _kws_semaphore_init();

  g_kws_context.running = TRUE;
  g_kws_context.working = FALSE;
  g_kws_context.engine_mode = ENGINE_KWS_MODE_WAKEUP;
  g_kws_context.last_mode = g_kws_context.engine_mode;
  g_kws_context.state = KWS_WORK_IDLE;
  g_kws_context.timeout = 0;
  g_kws_context.kws_queue_buf = uni_malloc(KWS_QUEUE_BUF_SIZE);
  if (g_kws_context.kws_queue_buf == NULL) {
    LOGE(KWS_TAG, "malloc kws_queue_buf failed");
    return -1;
  }
  aos_queue_new(&g_kws_context.kws_queue, g_kws_context.kws_queue_buf, KWS_QUEUE_BUF_SIZE, sizeof(kws_result_msg_t));

  int64_t len_am_buffer = sizeof(global_kws_lp_acoutstic_model);
  const char* am_buffer = global_kws_lp_acoutstic_model;

  int64_t len_grammar_buffer = sizeof(grammar);
  const char* grammar_buffer = grammar;

  g_kws_context.kws = UalOFAInitializeHandle();
  if (g_kws_context.kws == NULL) {
    LOGE(KWS_TAG, "Error initializing!");
    return 1;
  }
  UalOFASetOptionInt(g_kws_context.kws, KWS_SET_BUNCH_FRAME_NUMBER, 12);
  if (UalOFALoadFromBuffer(g_kws_context.kws, am_buffer, grammar_buffer)) {
    LOGE(KWS_TAG, "Error load module!");
    return 1;
  }
  g_kws_context.am_id = UalOFAGetActiveAmId(g_kws_context.kws);

  const char* version = UalOFAGetVersion(g_kws_context.kws);
  LOGI(KWS_TAG, "kws version is :%s", version);
  UalOFAReset(g_kws_context.kws);

  aos_task_t tsk_handle;
  aos_task_new_ext(&tsk_handle, "HandleKwsMsgTaskProc", HandleKwsMsgTaskProc, NULL, 512 * sizeof(u32), 29);

  return 0;
}

static int alg_unisound_init(mvoice_event_cb cb, void *user_data)
{
    mvoice_alg_callback = cb;
    g_user_data = user_data;

    if (_internal_kws_init() < 0) 
    {
      LOGE(KWS_TAG, "module_kws_init Failed!");
      return -1;
    }

    return 0;
}

static int alg_unisound_deinit()
{
    kws_stop();
    UalOFARelease(g_kws_context.kws);
    return 0;
}

#if defined(CONFIG_UNI_UART_RECORD) && CONFIG_UNI_UART_RECORD
static int alg_unisound_record_proc(void *mic)
{
  LasrGetRecordData((uint8_t *)mic, CAPTURE_FRAME_SIZE_1CH);
  return 0;
}
#endif

int g_test_mic_flag = false;
int g_test_mic_result = -999;
static int g_test_mic_count = 0;
static void *g_handle_micTest = NULL;

static int alg_unisound_test_mic(void *mic)
{
  if (g_test_mic_flag == false) {
    return 0;
  } else {
    g_test_mic_result = -999;
    if (g_handle_micTest == NULL ) {
      g_handle_micTest = SspProductCheckInit(1, 0, 0.6, 0, 10, 0, 3, 1);
      if (g_handle_micTest) {
        int32_t min_len = 1000 * 64;  // 4s
        /* 增加set接口调用，配置检测时间,按单个通道数据量即可 */
        SspProductCheckSet(g_handle_micTest, SSP_PRODUCT_CHECK_SET_MIN, &min_len);
      } 
    } 
    int ret = 0;
    if (g_handle_micTest) {
        ret = SspProductCheckProcess(g_handle_micTest, mic, CAPTURE_FRAME_SIZE_1CH, NULL, 0);
    }
    //LOGE(KWS_TAG, "ret value is [%d]",ret);
   // 能量检测结果
    if (ret) {
      int32_t result = 0;
      int i = 0;

      ProductCheckResult* perrmic = SspProductCheckGetErrorMic(g_handle_micTest);
      SspProductCheckGetResult(g_handle_micTest, perrmic, &result);
      printf("result = %d\n", result);
      printf("\n---------------------------------\n");
 
      if (result & (1 << 0)) {
        printf("energy check not pass \n");
        g_test_mic_result = false;
        g_handle_micTest = NULL;
      } else {
        printf("energy check pass \n");
        g_test_mic_result = true;
        g_handle_micTest = NULL;
      }
      printf("channel[%d] energy :%d\n", 0, perrmic->energy_value[0]);
      printf("\n---------------------------------\n");
      g_test_mic_flag = false;
      g_test_mic_count = 0;
    }

    g_test_mic_count++;
    if (g_test_mic_count > 300) {
       LOGE(KWS_TAG, "timeout exit test mic");
       g_test_mic_count = 0;
       g_test_mic_flag = false;
       g_handle_micTest = NULL;
    }
  }
  
}

static int alg_unisound_kws_proc(void *mic)
{
    int ret = -1;
    unsigned int flag;

    //int time_offset = 0;  // ms
#if DEBUG_RTF
    long long start_time, end_time;
#endif
    kws_result_msg_t kws_msg = {0};

    /* here to test mic data*/
    alg_unisound_test_mic(mic);

    if (!g_kws_context.working) {
      if (aos_event_get(&g_kws_context.event, KWS_EVT_START, AOS_EVENT_OR_CLEAR, &flag, 0) == 0) {
        if (_internal_kws_start() != 0) {
          return ret;
        }
      } else {
        return ret;
      }
    }

#if DEBUG_RTF
    wav_time += (float)CAPTURE_FRAME_MS;
    start_time = aos_now() / 1000;
#endif
 
#ifdef KWS_DATA_MONITOR
    static int16_t last_value = 0;
    static int16_t monitor_cnt = 0;
    int16_t *p = (int16_t *)mic;
    if (*p == last_value && *p == *(p + CAPTURE_FRAME_SIZE_1CH / 2 - 1)) {
      if (++monitor_cnt > 200) {
        LOGE(KWS_TAG, "data abnormal: continuous 200 frames seems keep same value %04x", last_value);
        app_sys_set_boot_reason(BOOT_REASON_SILENT_RESET);
        aos_reboot();
      }
    } else {
      last_value = *p;
      monitor_cnt = 0;
    }
#endif

    g_kws_context.cur_timestamp += 1;
    ret = UalOFARecognize(g_kws_context.kws, (signed char*)mic, CAPTURE_FRAME_SIZE_1CH);
    if (ret < 0) {
      LOGE(KWS_TAG, "Recognize error!");
      return 1;
    }
    if (ret == 2) {
      const char* result_const = UalOFAGetResult(g_kws_context.kws);
      memset(&kws_msg, 0, sizeof(kws_result_msg_t));
      kws_msg.event_id = VUI_LOCAL_ASR_SELF_RESULT_EVENT;
      if (LasrResultParse(result_const, kws_msg.command, &kws_msg.score) != E_OK) {
        LOGE(KWS_TAG, "LasrResultParse failed!");
        return 1;
      }

      // LOGI(KWS_TAG, "command=%s, score=%.2f, std_thresh=%.2f", kws_msg.command, kws_msg.score, g_kws_context.std_thres);
      if (E_OK == _lasr_check_score(kws_msg.score)) {
        aos_queue_send(&g_kws_context.kws_queue, (void *)&kws_msg, sizeof(kws_result_msg_t));
      } else {
        _write_arpt_log(kws_msg.command, kws_msg.score, NULL, g_kws_context.engine_mode, 0, 0);
      }
    }

#if DEBUG_RTF
    end_time = aos_now() / 1000;
    engine_time += (end_time - start_time);
    rtf_cnt--;
    if (rtf_cnt == 0) {
      // printf("RTF = %lld (ms) / %lld (ms) = %.4fx\n", engine_time / 1000,
      //        wav_time, (float)engine_time / 1000 / wav_time);
      wav_time = 0;
      engine_time = 0;
      rtf_cnt = 1000;
    }
#endif
    _check_kws_timeout();

    if (aos_event_get(&g_kws_context.event, KWS_EVT_STOP, AOS_EVENT_OR_CLEAR, &flag, 0) == 0) {
      _internal_kws_stop();
    }
    return 0;
}

mvoice_alg_t g_alg_unisound = {
    .name = "unisound",
    .data_format = {
        .interleaved = 0,
        .samples_per_frame = CAPTURE_FRAME_LEN_1CH,
        .samples_bits = 16,
        .sample_freq = 16000,
    },
    .init = alg_unisound_init,
    .deinit = alg_unisound_deinit,
    .kws_proc = alg_unisound_kws_proc,
#if defined(CONFIG_UNI_UART_RECORD) && CONFIG_UNI_UART_RECORD
    /* use asr_proc to record audio */
    .asr_proc = alg_unisound_record_proc,
#else
    .asr_proc = NULL,
#endif
};

void *alg_unisound_obj_get(void)
{
    return &g_alg_unisound;
}

#define ALG_TSK_STACK_SIZE    (1024 * 4)
static char g_alg_tsk_stack[ALG_TSK_STACK_SIZE];
extern void *alg_unisound_obj_get(void);

static void app_ai_cb(mvoice_event_e evt_id, void *data, int size, void *user_data) {
  int ret;
  LOGD(KWS_TAG, "enter ai callback");
  // mvoice_process_pause();

  switch(evt_id) {
    case MVC_EVT_VAD_START:
      break;
    case MVC_EVT_VAD_END:
      break;
    case MVC_EVT_KWS:
      break;
    case MVC_EVT_ASR:
      LOGD(KWS_TAG, "kws detect %s", (char *)data);
      break;
    default:
      break;
  }

  // mvoice_process_resume();
  LOGD(KWS_TAG, "leave ai callback");
}

int app_ai_init() {
  extern mvoice_sampling_t us615_sample_ops;

  mvoice_sample_register(&us615_sample_ops);
  mvoice_alg_register(alg_unisound_obj_get);
  mvoice_alg_init(app_ai_cb, NULL); 

  mvoice_process_start(33, g_alg_tsk_stack, ALG_TSK_STACK_SIZE);
}

