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
 * Description : Uniapp uni_nlu.h
 * Author      : chenxiaosong@unisound.com
 * Date        : 2018.6.8
 *
 **************************************************************************/

#include "uni_nlu.h"
#include "uni_log.h"
#include "uni_nlu_content.h"
#include "cJSON.h"

#define TAG "nlu"

uni_u32 bkdr_hash(const char* key) {
  uni_u32 seed = 31; // 31 131 1313 13131 131313 etc..
  uni_u32 hash = 0;
  uni_u8 *cp = (uni_u8 *)key;
  if (NULL == cp) {
    LOGE(TAG, "hash input is NULL");
    return 0;
  }
  while (*cp) {
    hash = (hash * seed + (*cp++)) & 0xFFFFFFFF;
  }
  return hash;
}

static void _replace_asr_recongize(cJSON *reslut, const char *word) {
  cJSON *rec_node = cJSON_CreateString(word);
  if (reslut) {
    if (cJSON_GetObjectItem(reslut, "asr")) {
      cJSON_ReplaceItemInObject(reslut, "asr", rec_node);
    } else {
      cJSON_AddItemToObject(reslut, "asr", rec_node);
    }
  }
}

//TODO perf sort g_nlu_content_mapping by hashcode O(logN), now version O(N)
cJSON* NluParseLasr(const char *key_word) {
  cJSON *result = NULL;
  uni_u32 hashCode = bkdr_hash(key_word);
  int hashTableSize = sizeof(g_nlu_content_mapping) / sizeof(g_nlu_content_mapping[0]);
  int i;

  for (i = 0; i < hashTableSize; i++) {
    /* find same hashcode as keyword's */
    if (hashCode == g_nlu_content_mapping[i].key_word_hash_code) {
      LOGD(TAG, "found map %d", i);
      /* return immediately when no hash collision */
      if (NULL == g_nlu_content_mapping[i].hash_collision_orginal_str) {
        LOGT(TAG, "found result %s", g_nlu_content_str[g_nlu_content_mapping[i].nlu_content_str_index]);
        result = cJSON_Parse(g_nlu_content_str[g_nlu_content_mapping[i].nlu_content_str_index]);
        if (result == NULL) {
          LOGE(TAG, "json parse failed, memory may not enough, reboot now");
          //WatchDogReboot();
          return NULL;
        }
        _replace_asr_recongize(result, key_word);
        return result;
      }
      /* return when key_word equals hash_collision_orginal_str */
      if (0 == strcmp(key_word, g_nlu_content_mapping[i].hash_collision_orginal_str)) {
        LOGT(TAG, "found result %s", g_nlu_content_str[g_nlu_content_mapping[i].nlu_content_str_index]);
        result = cJSON_Parse(g_nlu_content_str[g_nlu_content_mapping[i].nlu_content_str_index]);
        if (result == NULL) {
          LOGE(TAG, "json parse failed, memory may not enough, reboot now");
          //WatchDogReboot();
          return NULL;
        }
        _replace_asr_recongize(result, key_word);
        return result;
      }
    }
  }
  return NULL;
}

cJSON* NluParseRasr(const char *rasr_result) {
  return cJSON_Parse(rasr_result);
}

