#ifndef __MSG_PROCESS_CENTER_H__
#define __MSG_PROCESS_CENTER_H__

#include <stdint.h>

#define LIGHT_DURATION_DEFAULT_VALUE 500
#define MAX_SETTING_SIZE 8

#define HSV_Hue "hsv_h"
#define HSV_Saturation "hsv_s"
#define HSV_Value "hsv_v"

typedef struct  _COLOR_HSV {
    uint16_t h;  /* 0 - 359 */
    uint8_t s;   /* 0 - 100 */
    uint8_t v;   /* 0 - 100 */
    int light_switch;
}color_hsv_t;  /*  Use integers to reduce CPU computation load */

typedef struct  _COLOR_RGB {
    uint8_t r;   /* 0 - 255 */
    uint8_t g;   /* 0 - 255 */
    uint8_t b;   /* 0 - 255 */
}color_rgb_t;  /*  Use RGB888 */

typedef enum _LIGHT_MSG_TYPE{
    INVALID = -1, //unkwon
    LS_VALID = 0, //light switch
    // LM_VALID,     //light mode
    WK_VALID,     //work mode
    // BTS_VALID,    //brightness,only usued for MONO lightmode
    // CRT_VALID,    //colortemperature,only usued for MONO lightmode
    HSV_VALID,    // hsv
    CRS_VALID,    //colorspeed
    RHY_VALID,    //rhythm
    CAR_VALID,    //ColorArr
    // BT_VALID,     //brightness + colortemperature
    // BTLM_VALID,   //brightness + colortemperature + lightmode
    // BTSLM_VALID,  //brightness + lightmode
    CRTLM_VALID,  //colortemperature + lightmode
    HSVLM_VALID,  //hsv + lightmode
    SCE_VALID,    //scene setting
    SRY_VALID,     //scene reply
    RHYE_VALID,     // rhythm enable
    RHYS_VALID,     // rhythm sensitivity
    RHYSRC_VALID,   // rhythm source

    Wake_Up,
    Brightness_Cnt, 
} light_msg_type_t;

typedef struct _LIGHT_SCENE {
    char name[8];
    uint8_t scenemode;
    unsigned int brightness_max;
    unsigned int brightness_min;
    uint8_t isenable;
} light_scene_t;

typedef enum _MSG_FROM{
    FROM_PROPERTY_SET = 0,
    FROM_SERVICE_SET
} msg_from_t;

typedef struct _LED_STRIP_MSG{
    int validindex; //light_msg_type_t
    // int msgid;      //not used now
    uint8_t lightswitch;
    uint8_t lightmode;
    // uint8_t workmode;
    uint8_t arraysize;
    unsigned int brightness[MAX_SETTING_SIZE];
    unsigned int colortemperature[MAX_SETTING_SIZE];
    color_hsv_t hsvcolor[MAX_SETTING_SIZE];
    uint8_t colorspeed;
    uint16_t duration;
    // uint16_t rhythm_mode;
    // uint8_t rhythm_enable;
    // uint16_t rhythm_sensitivity;
    // uint8_t rhythm_source;
    light_scene_t scenes;
    // int flag;
    // char seq[24];
    uint8_t method;
    // uint8_t from;
    // uint8_t scenereply;
} led_strip_msg_t;

void init_msg_queue(void);
void send_msg_to_queue(led_strip_msg_t* cmd_msg);
void msg_process_task(void *argv);


#endif
