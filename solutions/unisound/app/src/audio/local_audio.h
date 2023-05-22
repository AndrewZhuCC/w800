/*
 */
#ifndef _LOCAL_AUDIO_H_
#define _LOCAL_AUDIO_H_

#define XIP_DATA __attribute__((section(".xip.data")))

#include <stdint.h>
typedef struct {
    const char  *content;
    unsigned char  *audio;
    unsigned int audio_len;
} audio_mapping_t;

typedef struct {
    unsigned char  *audio;
    unsigned int audio_len;
} audio_array_t;

typedef void (*finish_cb)();
typedef int (*data_cb)(char *buf, int len);

extern audio_mapping_t g_audio_name_map[];
extern int g_audio_name_map_len;
extern audio_array_t g_audio_name_array[];
extern int g_audio_name_array_len;
extern short audio_array_map[];
extern int audio_array_map_len;

int local_audio_init(void);
int local_audio_array_play(int i, int async);
int local_audio_vol_get();
int local_audio_vol_set(uint8_t vol);
int local_audio_register_cb(data_cb dc, finish_cb fc);
int UniPlayStart(void);
int UniPlayStop(void);
#endif
