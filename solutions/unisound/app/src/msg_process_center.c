#include "stdio.h"
#include "app_main.h"
#include "msg_process_center.h"
#include "user_gpio.h"
#include "uni_iot.h"
#include "uni_gpio.h"
#include "uni_timer.h"
#include "combo_net.h"

#define MB_RGBSTATUS_COUNT 10

extern gpio_pin_handle_t IR_io;
extern void ir_test_callback(void *arg);

static timer_handle_t timer_handler = NULL;

static aos_queue_t *g_cmd_msg_queue_id = NULL;
static char *g_cmd_msg_queue_buff = NULL;
// aos_queue_t *g_device_state_changed_queue_id = NULL;
// char *g_device_state_changed_queue_buff = NULL;

static aos_timer_t h_t_timer;

int color_mode=0;


led_strip_msg_t msg;
static int mode_num=0;
color_hsv_t HSV_Color;
color_hsv_t HSV_Color_Arr[8];
int scen_color_num=0,scen_color_cnt=0;

#define fLightToolGetMIN(a,b)    (((a) < (b)) ? (a) : (b))
#define fLightToolGetMAX(a,b)    (((a) > (b)) ? (a) : (b))


void post_property_task(int watch,int color_a,int color_b,int color_c)
{
    char property_payload[256];
    // printf("watch=%d\n",watch);
    snprintf(property_payload, sizeof(property_payload),"{\"powerstate\":%d,\"HSVColor\":{\"Saturation\":%d, \"Value\":%d, \"Hue\":%d}}",watch,color_b,color_c,color_a);
            // "{\"powerstate\":%d,\"HSVColor\":{\"Saturation\":%d, \"Value\":%d, \"Hue\":%d}}",\
            msg.lightswitch,hsv.s,hsv.v,hsv.h);
    user_post_given_property(property_payload, strlen(property_payload));
}

void vLightToolHSV2RGB(uint16_t h,uint8_t s,uint8_t v,uint8_t *red,uint8_t *green,uint8_t *blue)
{
    int i;
    float RGB_min, RGB_max;

    int difs;
    float RGB_Adj;

    if (h >= 360) {
		h = 0;
	}

    RGB_max = v*2.55f;
    RGB_min = RGB_max*(100 - s) / 100.0f;

    //  RGB_max = v*1.0f;
    //  RGB_min = RGB_max*(255 - s) / 255.0f;

    i = h / 60;
    difs = h % 60; // factorial part of h

        // RGB adjustment amount by hue 
    RGB_Adj = (RGB_max - RGB_min)*difs / 60.0f;
    // printf("RGB_max=%.2f,RGB_min==%.2f,RGB_Adj=%.2f,difs=%d,i=%d\n",RGB_max,RGB_min,RGB_Adj,difs,i);

    switch (i) {
    case 0:
        *red = (uint8_t)RGB_max;
        *green = (uint8_t)(RGB_min + RGB_Adj);
        *blue = (uint8_t)RGB_min;
        break;
    case 1:
        *red = (uint8_t)(RGB_max - RGB_Adj);
        *green = (uint8_t)RGB_max;
        *blue = (uint8_t)RGB_min;
        break;
    case 2:
        *red = (uint8_t)RGB_min;
        *green = (uint8_t)RGB_max;
        *blue = (uint8_t)(RGB_min + RGB_Adj);
        break;
    case 3:
        *red = (uint8_t)RGB_min;
        *green = (uint8_t)(RGB_max - RGB_Adj);
        *blue = (uint8_t)RGB_max;
        break;
    case 4:
        *red = (uint8_t)(RGB_min + RGB_Adj);
        *green = (uint8_t)RGB_min;
        *blue = (uint8_t)RGB_max;
        break;
    default:  // case 5:
        *red = (uint8_t)RGB_max;
        *green = (uint8_t)RGB_min;
        *blue = (uint8_t)(RGB_max - RGB_Adj);
        break;
    }
}

void led_static_color_show(uint16_t h,uint8_t s,uint8_t v,uint8_t r,uint8_t g,uint8_t b)
{
    uint8_t red, blue, green;
    vLightToolHSV2RGB(h, s, v, &red, &green, &blue);
    r = red *100 /255;
    g = green *100 /255;
    b = blue *100 /255;
    // printf("r = %d,g = %d,b = %d\n",r,g,b);
    user_pwm_enable(PB0, b);
    user_pwm_enable(PB1, g);
    user_pwm_enable(PB2, r);
}

void led_static_color_show_other(uint16_t h,uint8_t s,uint8_t v)
{
    uint8_t red, blue, green;
    vLightToolHSV2RGB(h, s, v, &red, &green, &blue);
    red = red *100 /255;
    green = green *100 /255;
    blue = blue *100 /255;
    // printf("red = %d,green = %d,blue = %d\n",red,green,blue);
    user_pwm_enable(PB2, 100-blue);
    user_pwm_enable(PB0, 100-green);
    user_pwm_enable(PB1, 100-red);
}

void send_msg_to_queue(led_strip_msg_t * cmd_msg)
{
    int ret = aos_queue_send(g_cmd_msg_queue_id, cmd_msg, sizeof(led_strip_msg_t));
    if (0 != ret)
        printf("###############ERROR: CMD MSG: aos_queue_send failed! #################\r\n");
}

void init_msg_queue()
{
    if (g_cmd_msg_queue_buff == NULL) {
        g_cmd_msg_queue_id = (aos_queue_t *) aos_malloc(sizeof(aos_queue_t));
        g_cmd_msg_queue_buff = aos_malloc(MB_RGBSTATUS_COUNT * sizeof(led_strip_msg_t));

        aos_queue_new(g_cmd_msg_queue_id, g_cmd_msg_queue_buff, MB_RGBSTATUS_COUNT * sizeof(led_strip_msg_t),
                sizeof(led_strip_msg_t));
    }
}

void cht8305_GetTempHumi(float  *ct8305_temp,float  *ct8305_humi) 
{	
	u8 temps_H;		
	u8 temps_L;		
	u8 humi_H;
	u8 humi_L;
	tls_i2c_write_byte(0x80, 1); 
	tls_i2c_wait_ack();	   
	tls_i2c_write_byte(0x00, 0);
	tls_i2c_wait_ack(); 	 										  		   	   
 	tls_i2c_stop();
	tls_os_time_delay(30);
	
	tls_i2c_write_byte(0x80|0x01,1);   
	tls_i2c_wait_ack(); 
	
	temps_H=tls_i2c_read_byte(1,0);
	tls_i2c_wait_ack();	 
	temps_L=tls_i2c_read_byte(1,0);
	tls_i2c_wait_ack();	 
	humi_H=tls_i2c_read_byte(1,0);
	tls_i2c_wait_ack();	 
	humi_L=tls_i2c_read_byte(1,0);
	tls_i2c_stop();
	
	*ct8305_temp=((temps_H<<8)|temps_L )* 165.0 / 65535.0 - 40.0;;
	*ct8305_humi=((humi_H<<8)|humi_L) * 100.0 / 65535.0 ;
	// printf("ct8305_temp %f ct8305_humi %f \r\n ",ct8305_temp,ct8305_humi);
}

static void h_t_timer_callback(void *arg1, void *arg2)
{
    float  ct8305_temp ;
	float  ct8305_humi;
    u32 HUM,TEMPER;
    cht8305_GetTempHumi(&ct8305_temp,&ct8305_humi);
    HUM  = (ct8305_humi +0.5)*1;
	TEMPER = (ct8305_temp +0.5)*1;
	// printf("HUM %d,TEMPER %d\r\n",HUM ,TEMPER );
}

void msg_process_task(void *argv)
{ 
    color_hsv_t hsv;
    color_rgb_t rgb;
    unsigned int rcvLen;
    // aos_timer_t scene_timer;
    int hsv_h=0,hsv_s=0,hsv_v=0;

    aos_timer_new_ext(&h_t_timer, h_t_timer_callback, NULL, 2000, 1, 1);

    while (true) {
        if (aos_queue_recv(g_cmd_msg_queue_id, AOS_WAIT_FOREVER, &msg, &rcvLen) == 0) {
            
            switch (msg.validindex) {
                case LS_VALID:
                    if(msg.lightswitch)
                    {
                        HSV_Color.light_switch=1;
                        led_static_color_show_other(HSV_Color.h, HSV_Color.s, HSV_Color.v);
                    }
                    else{
                        HSV_Color.light_switch=0;
                        user_pwm_enable(PB0, 100);
                        user_pwm_enable(PB1, 100);
                        user_pwm_enable(PB2, 100);
                    }
                    
                    post_property_task(HSV_Color.light_switch,HSV_Color.h, HSV_Color.s, HSV_Color.v);
                    break;

                case HSV_VALID:
                    HSV_Color.light_switch=1;
                    HSV_Color.h = msg.hsvcolor[0].h;
                    HSV_Color.s = msg.hsvcolor[0].s;
                    HSV_Color.v = msg.hsvcolor[0].v;
                    led_static_color_show_other(HSV_Color.h, HSV_Color.s, HSV_Color.v);
                    post_property_task(HSV_Color.light_switch,HSV_Color.h, HSV_Color.s, HSV_Color.v);
                    break;

                case Brightness_Cnt:
                    HSV_Color.light_switch=1;
                    HSV_Color.v = msg.hsvcolor[0].v;
                    led_static_color_show_other(HSV_Color.h, HSV_Color.s, HSV_Color.v);
                    post_property_task(HSV_Color.light_switch,HSV_Color.h, HSV_Color.s, HSV_Color.v);
                    break;               

                case WK_VALID: 
                    HSV_Color.light_switch=1;
                    HSV_Color.h = msg.hsvcolor[0].h;
                    HSV_Color.s = msg.hsvcolor[0].s;
                    HSV_Color.v = msg.hsvcolor[0].v;
                    // led_static_color_show(hsv.h, hsv.s, hsv.v, rgb.r, rgb.g, rgb.b);
                    // aos_kv_set("HSV_color",&hsv,sizeof(hsv),1);
                    led_static_color_show_other(HSV_Color.h, HSV_Color.s, HSV_Color.v);
                    post_property_task(HSV_Color.light_switch,HSV_Color.h, HSV_Color.s, HSV_Color.v);
                    break; 

                case Wake_Up: 
                    if(HSV_Color.light_switch==1) {
                        led_static_color_show_other(HSV_Color.h, HSV_Color.s, HSV_Color.v);
                    } 
                    // else if(HSV_Color.light_switch==1 && color_mode==2)
                    // {
                    //     led_static_color_show_other(HSV_Color.h, HSV_Color.s, HSV_Color.v);
                    // }   
                    break;

                default:
                    break; 
            }
            
        }
        aos_msleep(5);
    }
}
