/**************************************************************************
 * Copyright (C) 2020-2020  Unisound
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
 * Description : user_player.c
 * Author      : yuanshifeng@unisound.com
 * Date        : 2021.06.17
 *
 **************************************************************************/
#include <stdlib.h>

#include "pcm_bin.h"
#include "uni_iot.h"
#include "uni_pcm_array.h"
#include "uni_cust_config.h"
#include "local_audio.h"
#include "adpcm/uni_adpcm.h"
#include "mp3_decoder.h"
#include "mvoice_alg.h"

#define LOG_TAG "user_player"

typedef struct {
  char *files;
  int play_cnt;
  int play_index;
  unsigned char *play_addr;
  uni_u32 len;
  int state;
  uni_mutex_t mutex;
} UniPlayer;
static UniPlayer g_uni_player;

#define LIST_COUNT_MAX 5
static int play_list[LIST_COUNT_MAX] = {0};
static int _parse_pcm_list(const char *str_list) {
  int count = 0, sum = 0;
  char last_ch = 0;
  const char *p = str_list;
  g_uni_player.play_index = 0;
  memset(play_list, 0, sizeof(play_list));
  if (!str_list || str_list[0] != '[') {
    return -1;
  }
  while (*p) {
    if (*p == '[') {
      last_ch = *p;
      p++;
      continue;
    }
    if (*p == ',' || *p == ']') {
      if ((last_ch == '[') || (last_ch == ',')) {
        /* invalid style */
        sum = 0;
        last_ch = *p;
        if (*p == ']') {
          /* finished if ']' */
          break;
        }
        continue;
      }
      play_list[count++] = sum;
      sum = 0;
      last_ch = *p;
      if (count >= LIST_COUNT_MAX) break;
      if (*p == ']') {
        /* finished if ']' */
        break;
      }
    } else if (*p >= '0' && *p <= '9') {
      sum = sum * 10 + *p - '0';
      last_ch = *p;
    } else if (*p != ' ' && *p != '\t') {
      /* failed style if there is other char */
      return -1;
    }
    p++;
  }
  if (count <= 0) {
    return -1;
  }
  return count;
}

static int _uni_get_number_pcm(const char *str_list, int num) {
  int count = 0;
  if (num < 0) {
    return -1;
  }
  count = _parse_pcm_list(str_list);
  if (count <= 0 || count <= num) {
    return -1;
  }
  return play_list[num];
}

static int _uni_get_random_pcm(const char *str_list) {
  int count = _parse_pcm_list(str_list);
  if (count <= 0) {
    return -1;
  }
  return play_list[rand() % count];
}

static adpcm_state_t encode_status = {0};
static int _uni_get_play_info(int name, unsigned char **play_addr,
                              uni_u32 *play_len) {
  uni_u32 val = name;
  int source_len = sizeof(g_pcm_arry) / sizeof(PCM_RESOURCE);
  int i = 0;
  for (i = 0; i < source_len; i++) {
    if (val == g_pcm_arry[i].number) {
      *play_addr = (unsigned char *)&pcm_bin[g_pcm_arry[i].offset];
      *play_len = g_pcm_arry[i].len;
      memset(&encode_status, 0, sizeof(adpcm_state_t));
      return 0;
    }
  }
  return -1;
}

static int _uni_pcm_play_next() {
  if (g_uni_player.play_cnt > g_uni_player.play_index) {
    int num;
    num = play_list[g_uni_player.play_index];
    return num;
  }
  return -1;
}

static void _finish_cb(void) {
  g_uni_player.state = 0;
  // ArptPrint("TTS END\n");
  mvoice_process_resume();
}

static int _feed_data_cb(char *buf, int len) {
  int read_len = 0;
  if (g_uni_player.len > 0) {
    read_len = uni_min(g_uni_player.len, len);
    uni_memcpy(buf, g_uni_player.play_addr, read_len);
    g_uni_player.play_addr += read_len;
    g_uni_player.len -= read_len;
  } else {
    int next_num = 0;
    next_num = _uni_pcm_play_next();
    LOGD(LOG_TAG, "next num is %d", next_num);
    if (next_num > 0) {
      if (_uni_get_play_info(next_num, &g_uni_player.play_addr,
                             &g_uni_player.len)) {
        LOGE(LOG_TAG, "can't find pcm :%d\n", next_num);
        g_uni_player.state = 0;
        return 0;
      }
      g_uni_player.play_index++;
      LOGD(LOG_TAG, "next play %d file, addr=%x, len=%d", next_num,
           (unsigned int)g_uni_player.play_addr, g_uni_player.len);
      read_len = uni_min(g_uni_player.len, len);
      uni_memcpy(buf, g_uni_player.play_addr, read_len);
      g_uni_player.play_addr += read_len;
      g_uni_player.len -= read_len;
      return read_len;
    }
    LOGD(LOG_TAG, "feed data end");
    g_uni_player.state = 0;
    return 0;
  }
  return read_len;
}

static int _uni_play_filenum(int play_num) {
  if (play_num < 0) {
    LOGE(LOG_TAG, "play file %d invalid", play_num);
    return -1;
  }

  if (_uni_get_play_info(play_num, &g_uni_player.play_addr,
                         &g_uni_player.len)) {
    LOGE(LOG_TAG, "cannot found file %d", play_num);
    return -1;
  }
  g_uni_player.state = 1;
  g_uni_player.play_index++;
  LOGD(LOG_TAG, "play %d file, addr=%x, len=%d", play_num,
       (unsigned int)g_uni_player.play_addr, g_uni_player.len);
  // ArptPrint("TTS START\n");
  mvoice_process_pause();
  // ArptPrint("TTS START\n");
  return UniPlayStart();
}

int user_player_init(void) {
  local_audio_register_cb(_feed_data_cb, _finish_cb);
  return 0;
}

int user_player_play_filenum(int play_num) {
  return _uni_play_filenum(play_num);
}

int user_player_play(const char *file) {
  int play_num = 0;
  if (file == NULL) {
    LOGE(LOG_TAG, "play file invalid");
    return -1;
  }
  play_num = _uni_get_number_pcm(file, 0);
  g_uni_player.play_cnt = 1;
  return user_player_play_filenum(play_num);
}

int user_player_reply_list_num(const char *file_list, int num) {
  int play_num = 0;
  if (NULL == file_list) {
    LOGE(LOG_TAG, "Invalid file name is NULL");
    return -1;
  }
  play_num = _uni_get_number_pcm(file_list, num);
  if (play_num == -1) {
    LOGE(LOG_TAG, "Cannot found %dst file in list %s", num, file_list);
    return -1;
  }
  g_uni_player.play_cnt = 1;
  return user_player_play_filenum(play_num);
}

int user_player_reply_list_random(const char *file_list) {
  int play_num = 0;
  if (NULL == file_list) {
    LOGE(LOG_TAG, "Invalid file name is NULL");
    return -1;
  }
  play_num = _uni_get_random_pcm(file_list);
  if (play_num == -1) {
    LOGE(LOG_TAG, "Cannot found any file in list %s", file_list);
    return -1;
  }
  g_uni_player.play_cnt = 1;
  return user_player_play_filenum(play_num);
}

int user_player_reply_list_in_order(const char *file_list) {
  int count = 0;

  if (NULL == file_list) {
    LOGE(LOG_TAG, "Invalid file name is NULL");
    return -1;
  }

  count = _parse_pcm_list(file_list);
  if (count <= 0) {
    return -1;
  }
  g_uni_player.play_cnt = count;
  user_player_play_filenum(play_list[g_uni_player.play_index]);
  return 0;
}

int user_player_stop(void) {
  // TBD
  return 0;
}

int user_player_set_volume_min(void) {
  // TBD
  return 0;
}

int user_player_set_volume_max(void) {
  // TBD
  return 0;
}

int user_player_set_volume_mid(void) {
  // TBD
  return 0;
}

int user_player_set_volume_up(void) {
  // TBD
  return 0;
}

int user_player_set_volume_down(void) {
  // TBD
  return 0;
}

int user_player_speaker_mute(void) {
  // TBD
  return 0;
}

int user_player_speaker_unmute(void) {
  // TBD
  return 0;
}

int user_player_shutup_mode(void) {
  // TBD
  return 0;
}

int user_player_shutup_exit(void) {
  // TBD
  return 0;
}
