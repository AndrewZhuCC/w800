/**************************************************************************
 * Copyright (C) 2012-2021  Unisound
 *
 * DO NOT MODIFY.
 *
 * This file was generated by res_build_tool.py
 *
 **************************************************************************/
#ifndef INC_UNI_NLU_CONTENT_H_
#define INC_UNI_NLU_CONTENT_H_

#include "uni_nlu_content_type.h"

const char* g_nlu_content_str[] = {
  [eCMD_wakeup_uni] = "{\"cmd\":\"wakeup_uni\",\"pcm\":\"[103]\"}",
  [eCMD_exitUni] = "{\"cmd\":\"exitUni\",\"pcm\":\"[104]\"}",
  [eCMD_Network_null_0] = "{\"cmd\":\"Network#null#0\",\"pcm\":\"[106]\"}",
  [eCMD_color_val_0] = "{\"cmd\":\"color#val#0\",\"pcm\":\"[106]\"}",
  [eCMD_color_val_1] = "{\"cmd\":\"color#val#1\",\"pcm\":\"[106]\"}",
  [eCMD_color_val_2] = "{\"cmd\":\"color#val#2\",\"pcm\":\"[106]\"}",
  [eCMD_color_val_3] = "{\"cmd\":\"color#val#3\",\"pcm\":\"[106]\"}",
  [eCMD_color_val_4] = "{\"cmd\":\"color#val#4\",\"pcm\":\"[106]\"}",
  [eCMD_color_val_5] = "{\"cmd\":\"color#val#5\",\"pcm\":\"[106]\"}",
  [eCMD_color_val_6] = "{\"cmd\":\"color#val#6\",\"pcm\":\"[106]\"}",
  [eCMD_color_val_7] = "{\"cmd\":\"color#val#7\",\"pcm\":\"[106]\"}",
  [eCMD_color_val_8] = "{\"cmd\":\"color#val#8\",\"pcm\":\"[106]\"}",
  [eCMD_powerstate_val_0] = "{\"cmd\":\"powerstate#val#0\",\"pcm\":\"[106]\"}",
  [eCMD_powerstate_val_1] = "{\"cmd\":\"powerstate#val#1\",\"pcm\":\"[106]\"}",
  [eCMD_brightness_val_0] = "{\"cmd\":\"brightness#val#0\",\"pcm\":\"[106]\"}",
  [eCMD_brightness_val_10] = "{\"cmd\":\"brightness#val#10\",\"pcm\":\"[106]\"}",
  [eCMD_brightness_val_20] = "{\"cmd\":\"brightness#val#20\",\"pcm\":\"[106]\"}",
  [eCMD_brightness_val_30] = "{\"cmd\":\"brightness#val#30\",\"pcm\":\"[106]\"}",
  [eCMD_brightness_val_40] = "{\"cmd\":\"brightness#val#40\",\"pcm\":\"[106]\"}",
  [eCMD_brightness_val_50] = "{\"cmd\":\"brightness#val#50\",\"pcm\":\"[106]\"}",
  [eCMD_brightness_val_60] = "{\"cmd\":\"brightness#val#60\",\"pcm\":\"[106]\"}",
  [eCMD_brightness_val_70] = "{\"cmd\":\"brightness#val#70\",\"pcm\":\"[106]\"}",
  [eCMD_brightness_val_80] = "{\"cmd\":\"brightness#val#80\",\"pcm\":\"[106]\"}",
  [eCMD_brightness_val_90] = "{\"cmd\":\"brightness#val#90\",\"pcm\":\"[106]\"}",
  [eCMD_brightness_val_100] = "{\"cmd\":\"brightness#val#100\",\"pcm\":\"[106]\"}",
  [eCMD_mode_val_0] = "{\"cmd\":\"mode#val#0\",\"pcm\":\"[106]\"}",
  [eCMD_mode_val_3] = "{\"cmd\":\"mode#val#3\",\"pcm\":\"[106]\"}",
  [eCMD_mode_val_4] = "{\"cmd\":\"mode#val#4\",\"pcm\":\"[106]\"}",
  [eCMD_mode_val_5] = "{\"cmd\":\"mode#val#5\",\"pcm\":\"[106]\"}",
  [eCMD_mode_val_6] = "{\"cmd\":\"mode#val#6\",\"pcm\":\"[106]\"}",
  [eCMD_mode_val_8] = "{\"cmd\":\"mode#val#8\",\"pcm\":\"[106]\"}",
  [eCMD_mode_val_14] = "{\"cmd\":\"mode#val#14\",\"pcm\":\"[106]\"}",
  [eCMD_mode_val_15] = "{\"cmd\":\"mode#val#15\",\"pcm\":\"[106]\"}",
  [eCMD_mode_val_33] = "{\"cmd\":\"mode#val#33\",\"pcm\":\"[106]\"}",
  [eCMD_mode_val_57] = "{\"cmd\":\"mode#val#57\",\"pcm\":\"[106]\"}",
  [eCMD_mode_val_226] = "{\"cmd\":\"mode#val#226\",\"pcm\":\"[106]\"}",
};

/*TODO perf sort by hashcode O(logN), now version O(N)*/
const uni_nlu_content_mapping_t g_nlu_content_mapping[] = {
  {2947460911U/*你好魔方*/, eCMD_wakeup_uni, NULL},
  {2835566874U/*你好小海*/, eCMD_wakeup_uni, NULL},
  {2835567363U/*你好小科*/, eCMD_wakeup_uni, NULL},
  {2835564444U/*你好小凌*/, eCMD_wakeup_uni, NULL},
  {2497873774U/*退下*/, eCMD_exitUni, NULL},
  {2389288886U/*再见*/, eCMD_exitUni, NULL},
  {299987594U/*开始配网*/, eCMD_Network_null_0, NULL},
  {3964302964U/*配置网络*/, eCMD_Network_null_0, NULL},
  {1167464996U/*进入配网模式*/, eCMD_Network_null_0, NULL},
  {369722877U/*调为黑色*/, eCMD_color_val_0, NULL},
  {312047501U/*调为红色*/, eCMD_color_val_1, NULL},
  {267450374U/*调为橙色*/, eCMD_color_val_2, NULL},
  {369335594U/*调为黄色*/, eCMD_color_val_3, NULL},
  {313834961U/*调为绿色*/, eCMD_color_val_4, NULL},
  {342047038U/*调为青色*/, eCMD_color_val_5, NULL},
  {304510378U/*调为蓝色*/, eCMD_color_val_6, NULL},
  {306774494U/*调为紫色*/, eCMD_color_val_7, NULL},
  {282375665U/*调为白色*/, eCMD_color_val_8, NULL},
  {2506143786U/*关闭灯关*/, eCMD_powerstate_val_0, NULL},
  {3447216717U/*打开灯光*/, eCMD_powerstate_val_1, NULL},
  {207325899U/*亮度零*/, eCMD_brightness_val_0, NULL},
  {207321568U/*亮度十*/, eCMD_brightness_val_10, NULL},
  {167342204U/*亮度二十*/, eCMD_brightness_val_20, NULL},
  {165405789U/*亮度三十*/, eCMD_brightness_val_30, NULL},
  {167789069U/*亮度四十*/, eCMD_brightness_val_40, NULL},
  {167580532U/*亮度五十*/, eCMD_brightness_val_50, NULL},
  {148007845U/*亮度六十*/, eCMD_brightness_val_60, NULL},
  {165227043U/*亮度七十*/, eCMD_brightness_val_70, NULL},
  {147948263U/*亮度八十*/, eCMD_brightness_val_80, NULL},
  {166925130U/*亮度九十*/, eCMD_brightness_val_90, NULL},
  {165140025U/*亮度一百*/, eCMD_brightness_val_100, NULL},
  {630568308U/*手动模式*/, eCMD_mode_val_0, NULL},
  {3922175799U/*阅读模式*/, eCMD_mode_val_3, NULL},
  {3374130034U/*影院模式*/, eCMD_mode_val_4, NULL},
  {1985272548U/*温暖模式*/, eCMD_mode_val_5, NULL},
  {4274869873U/*夜灯模式*/, eCMD_mode_val_6, NULL},
  {270836452U/*起床模式*/, eCMD_mode_val_8, NULL},
  {2627810873U/*睡眠模式*/, eCMD_mode_val_14, NULL},
  {891556084U/*生活模式*/, eCMD_mode_val_15, NULL},
  {3158896343U/*音乐模式*/, eCMD_mode_val_33, NULL},
  {771824384U/*护眼模式*/, eCMD_mode_val_57, NULL},
  {3744587893U/*明亮模式*/, eCMD_mode_val_226, NULL},
};

#endif
