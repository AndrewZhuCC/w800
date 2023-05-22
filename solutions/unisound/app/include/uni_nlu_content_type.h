/**************************************************************************
 * Copyright (C) 2012-2021  Unisound
 *
 * DO NOT MODIFY.
 *
 * This file was generated by res_build_tool.py
 *
 **************************************************************************/
#ifndef INC_UNI_NLU_CONTENT_TYPE_H_
#define INC_UNI_NLU_CONTENT_TYPE_H_
typedef struct {
  unsigned int  key_word_hash_code; /* 存放识别词汇对应的hashcode */
  unsigned char nlu_content_str_index; /* 存放nlu映射表中的索引，实现多个识别词汇可对应同一个nlu，暂支持256条，如果不够换u16 */
  char          *hash_collision_orginal_str; /* 类似Java String equal，当hash发生碰撞时，赋值为识别词汇，否则设置为NULL */
} uni_nlu_content_mapping_t;

enum {
  eCMD_wakeup_uni,
  eCMD_exitUni,
  eCMD_Network_null_0,
  eCMD_color_val_0,
  eCMD_color_val_1,
  eCMD_color_val_2,
  eCMD_color_val_3,
  eCMD_color_val_4,
  eCMD_color_val_5,
  eCMD_color_val_6,
  eCMD_color_val_7,
  eCMD_color_val_8,
  eCMD_powerstate_val_0,
  eCMD_powerstate_val_1,
  eCMD_brightness_val_0,
  eCMD_brightness_val_10,
  eCMD_brightness_val_20,
  eCMD_brightness_val_30,
  eCMD_brightness_val_40,
  eCMD_brightness_val_50,
  eCMD_brightness_val_60,
  eCMD_brightness_val_70,
  eCMD_brightness_val_80,
  eCMD_brightness_val_90,
  eCMD_brightness_val_100,
  eCMD_mode_val_0,
  eCMD_mode_val_3,
  eCMD_mode_val_4,
  eCMD_mode_val_5,
  eCMD_mode_val_6,
  eCMD_mode_val_8,
  eCMD_mode_val_14,
  eCMD_mode_val_15,
  eCMD_mode_val_33,
  eCMD_mode_val_57,
  eCMD_mode_val_226,
};

#endif
