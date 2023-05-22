/*
*/
#ifndef CONFIG_KERNEL_NONE

#include <string.h>
#include <math.h>
#include <drv/irq.h>
#include <drv/i2s.h>
#include <drv/codec.h>
#include <devices/device.h>
#include <aos/aos.h>
#include <devices/iic.h>
#include <pin.h>
#include <aos/aos.h>
#include <soc.h>
#include <dev_ringbuf.h>

#include "uni_i2s.h"
#include "drv/pwm.h"
#include "uni_pwm.h"
#include "uni_gpio.h"
#include "uni_irq.h"

#define TAG "us615_codec"

#define CONFIG_CODEC_NUM    1
#define ES8311_ADDR 0x18
#define ES8311_I2C_TIMEOUT     0

#define CODEC_ALWAYS_AS_MASTER_FTR 1

#define USE_LOCAL_ADC_BUFF   1

#if USE_LOCAL_ADC_BUFF
#define ADC_BUFLEN      (512)
static char g_adc_buf[ADC_BUFLEN * 2] __attribute__((aligned(4))) = {0}; 
#endif

typedef enum {
  CODEC_INIT = (1 << 0),
  CODEC_READT_TO_START = (1 << 1),
  CODEC_RUNING         =  (1 << 2),
  CODEC_PAUSE          =  (1 << 3)
} us615_codec_sta;

typedef struct {
  dev_ringbuf_t fifo;
  uint32_t period;
  codec_event_cb_t cb;
  void *cb_arg;
}uni_codec_config_t;

typedef struct  {
  uint32_t base;
  uint32_t irq_num;
  void *irq_handle;

  us615_codec_sta      out_sta;
  us615_codec_sta      in_sta;
  uni_dma_handler_type dma_tx;
  uni_dma_handler_type dma_rx;
  I2S_InitDef        paras;
  uni_codec_config_t tx_bufhdl;
  uni_codec_config_t rx_bufhdl;
}us615_i2s_t;


static us615_i2s_t codec_instance[CONFIG_CODEC_NUM];
static aos_dev_t *g_i2c_dev;

extern int32_t target_i2s_init(int32_t idx, uint32_t *base, uint32_t *irq, void **handler);


static void UserIOInit(void)
{
  tls_gpio_cfg(USER_PA_PIN, UNI_GPIO_DIR_OUTPUT, UNI_GPIO_ATTR_FLOATING);
  tls_gpio_write(USER_PA_PIN, 0);
}

//power gain 
static void UserPACtrl(uint8_t flag)
{

  tls_gpio_write(USER_PA_PIN, flag ^ 0x1);
}

static int es8311_i2c_read_reg(aos_dev_t *dev, uint8_t addr, uint8_t *val)
{
  int ret = 0;
  ret = iic_master_send(dev,ES8311_ADDR,&addr,1,1);

  if(ret)
  {
    LOGE(TAG, "es8311 read reg failed");
    return ret;
  }

  return iic_master_recv(dev,ES8311_ADDR,val,1,0);
}


static int es8311_i2c_write_reg(aos_dev_t *dev, uint8_t addr, uint8_t val)
{
  uint8_t reg_val[2] = {addr, val};

  return iic_master_send(dev, ES8311_ADDR, reg_val, 2, ES8311_I2C_TIMEOUT);
}


static int es8311_i2c_init()
{
  iic_config_t config;

  g_i2c_dev = iic_open_id("iic", 0);

  if(g_i2c_dev == NULL)
  {
    LOGE(TAG, "I2C init failed");
    return -1;
  }
  config.mode = MODE_MASTER;
  config.speed = BUS_SPEED_STANDARD;
  config.addr_mode = ADDR_7BIT;
  config.slave_addr = ES8311_ADDR;

  return iic_config(g_i2c_dev, &config);

}


#define ADC_VOLUME_GAIN 0xBF //0xEF
#define DADC_GAIN 0x1A //0x17
#define BCLK_DIV  0x0F// bclk=400k

static int es8311_config(aos_dev_t *i2c_dev)
{
  int ret = 0;
  uint8_t value1 = 0;
  uint8_t value2 = 0;

  ret = es8311_i2c_read_reg(i2c_dev, 0xFD, &value1);
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  ret = es8311_i2c_read_reg(i2c_dev, 0xFE, &value2);
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  if(value1 != 0x83 || value2 != 0x11)
  {
    LOGE(TAG,"codec err, id = 0x%x 0x%x ver = 0x%x\n", value1, value2);	
    return -1;
  }

  //the sequence for start up codec
  // RESET CODEC
  ret = es8311_i2c_write_reg(i2c_dev, 0x00, 0x1F);
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x45, 0x00);
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  // SET ADC/DAC CLK
  ret = es8311_i2c_write_reg(i2c_dev, 0x01, 0x30); //mclk blck
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x02, 0x90);  //mclk = 6400k 0x00
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x03, 0x19);  //adc 64fs(ss)
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x16, 0x03);  // bit5:0~non standard audio clock   adc sync,24dB
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x04, 0x19);  //dac  64fs(ss)
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x05, 0x00);  //adc_mclk=dig_mclk dac_mclk=dig_mclk
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  /*new cfg*/
  ret = es8311_i2c_write_reg(i2c_dev, 0x06, BCLK_DIV);  //
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x07, 0x01);  //
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x08, 0xF3);  // 0xff
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  //SET SYSTEM POWER UP
  ret = es8311_i2c_write_reg(i2c_dev, 0x0B, 0x00);
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x0C, 0x00);
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x10, 0x1F);//higher DAC bias setting
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x11, 0x7F);//Internal use
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x00, 0xC0);//Chip current state machine power on
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  aos_msleep(20);
  ret = es8311_i2c_write_reg(i2c_dev, 0x0D, 0x01); //start up vmid normal speed charge
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x01, 0x3F);//clock manager
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  // SET ADC
  ret = es8311_i2c_write_reg(i2c_dev, 0x14, DADC_GAIN); //ADC PGA gain
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  // SET DAC
  ret = es8311_i2c_write_reg(i2c_dev, 0x12, 0x00); //enable DAC
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  // ENABLE HP DRIVE
  ret = es8311_i2c_write_reg(i2c_dev, 0x13, 0x10); //default for line out drive
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  // SET ADC/DAC DATA FORMAT
  ret = es8311_i2c_write_reg(i2c_dev, 0x09, 0x0C);//left channel to dac, 16bit,i2s format
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x0A, 0x0C); //16bit, left and right normal polarity
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  // SET LOW OR NORMAL POWER MODE
  ret = es8311_i2c_write_reg(i2c_dev, 0x0E, 0x02);//low power mode of internal reference voltage
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x0F, 0x44);//low power mode for DAC reference and PGA
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  // SET ADC
  ret = es8311_i2c_write_reg(i2c_dev, 0x15, 0x00);//0.25dB/32LRCK
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x1B, 0x05);//ADC auto mute out gain select and ADCHPF stage1 coeff
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x1C, 0x65);//bypass, dynamic HPF,ADCHPF stage2 coeff
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x17, ADC_VOLUME_GAIN);//ADC volume
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  // SET DAC
  ret = es8311_i2c_write_reg(i2c_dev, 0x37, 0x08); //DACEQ bypass,0.25dB/32LRCK
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  ret = es8311_i2c_write_reg(i2c_dev, 0x32, 0xBF);//  DAC VOL
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);


  return 0;

err:
  LOGE(TAG, "es8311 config failed");
  return -1;
}

/* only for 8K out &&  16K in*/
static int es8311_reinit(unsigned int sr)
{
  int ret = 0;
  static unsigned int last_sr = 0;
  aos_dev_t *i2c_dev = g_i2c_dev;
  uint8_t reg02, reg05, reg06, reg07, reg08;
  if (sr == last_sr) {
    LOGD(TAG, "skip config same sr:%d", sr);
    return 0;
  }
  if (8000 == sr) {
    reg02 = 0x08;
    reg05 = 0x44;
    reg06 = 0x18;
    reg07 = 0x03;
    reg08 = 0xe7;
  } else if (16000 == sr) {
    reg02 = 0x90;
    reg05 = 0x00;
    reg06 = 0x0f;
    reg07 = 0x01;
    reg08 = 0xf3;
  } else {
    goto err;
  }

  //the sequence for start up codec
  // RESET CODEC
  ret = es8311_i2c_write_reg(i2c_dev, 0x00, 0x1F);
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x45, 0x00);
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  // SET ADC/DAC CLK
  ret = es8311_i2c_write_reg(i2c_dev, 0x01, 0x30); //mclk blck
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x02, reg02);  //mclk = 6400k 0x00
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x03, 0x19);  //adc 64fs(ss)
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x16, 0x03);  // bit5:0~non standard audio clock   adc sync,24dB
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x04, 0x19);  //dac  64fs(ss)
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x05, reg05);  //adc_mclk=dig_mclk dac_mclk=dig_mclk
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  /*new cfg*/
  ret = es8311_i2c_write_reg(i2c_dev, 0x06, reg06);  //
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x07, reg07);  //
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x08, reg08);  // 0xff
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  //SET SYSTEM POWER UP
  ret = es8311_i2c_write_reg(i2c_dev, 0x0B, 0x00);
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x0C, 0x00);
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x10, 0x1F);//higher DAC bias setting
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x11, 0x7F);//Internal use
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x00, 0xC0);//Chip current state machine power on
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  aos_msleep(20);
  ret = es8311_i2c_write_reg(i2c_dev, 0x0D, 0x01); //start up vmid normal speed charge
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x01, 0x3F);//clock manager
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  // SET ADC
  ret = es8311_i2c_write_reg(i2c_dev, 0x14, DADC_GAIN); //ADC PGA gain
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  // SET DAC
  ret = es8311_i2c_write_reg(i2c_dev, 0x12, 0x00); //enable DAC
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  // ENABLE HP DRIVE
  ret = es8311_i2c_write_reg(i2c_dev, 0x13, 0x10); //default for line out drive
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  // SET ADC/DAC DATA FORMAT
  ret = es8311_i2c_write_reg(i2c_dev, 0x09, 0x0C);//left channel to dac, 16bit,i2s format
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x0A, 0x0C); //16bit, left and right normal polarity
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  // SET LOW OR NORMAL POWER MODE
  ret = es8311_i2c_write_reg(i2c_dev, 0x0E, 0x02);//low power mode of internal reference voltage
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x0F, 0x44);//low power mode for DAC reference and PGA
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  // SET ADC
  ret = es8311_i2c_write_reg(i2c_dev, 0x15, 0x00);//0.25dB/32LRCK
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x1B, 0x05);//ADC auto mute out gain select and ADCHPF stage1 coeff
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x1C, 0x65);//bypass, dynamic HPF,ADCHPF stage2 coeff
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
  ret = es8311_i2c_write_reg(i2c_dev, 0x17, ADC_VOLUME_GAIN);//ADC volume
  CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  // SET DAC
  // ret = es8311_i2c_write_reg(i2c_dev, 0x37, 0x08); //DACEQ bypass,0.25dB/32LRCK
  // CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  // ret = es8311_i2c_write_reg(i2c_dev, 0x32, 0xBF);//  DAC VOL
  // CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

  last_sr = sr;
  return 0;

err:
  LOGE(TAG, "es8311 config failed");
  return -1;
}

static int es8311_init()
{
  int ret = 0;

  LOGD(TAG, "start es8311 config");

  ret = es8311_i2c_init();

  if(ret)
  {
    return ret;
  }

  es8311_config(g_i2c_dev);

  LOGD(TAG, "end es8311 config");
  return 0;
}

//DAC 
static void DAC_CodecMute(u8 ifmute)
{
  int ret = 0;

  if(ifmute == 0)
  {
    es8311_i2c_write_reg(g_i2c_dev, 0x31, 0x00); 
  }
  else if(ifmute == 1)
  {
    es8311_i2c_write_reg(g_i2c_dev, 0x31, 0x60);
  }
}

/* dac gain -95.5 ~ 32 dB, gain -955~320 */
static int DAC_CodecVolume(int gain)
{
  aos_check_return_einval(gain >= -955 && gain <= 320);

  uint8_t reg_val = (uint8_t)((gain + 955) / 5);

  es8311_i2c_write_reg(g_i2c_dev, 0x32, reg_val);

  //printf("set es8311 dac reg to %d", reg_val);
  return 0;
}

/* dac gain -95.5 ~ 32 dB, gain -955~320 */
static int DAC_GetVolume()
{
  uint8_t ret_val;
  int ret;

  ret = es8311_i2c_read_reg(g_i2c_dev, 0x32, &ret_val);
  aos_check_return_einval(ret == 0);

  return  ((int)ret_val * 5 - 955);
}

//ADC 
/*0x00 – -95.5dB (default)
  0x01 – -90.5dB
  … 0.5dB/step
  0xBE – -0.5dB
  0xBF – 0dB
  0xC0 – +0.5dB
  …
  0xFF – +32dB
  */
static int ADC_CodecVolume(int vol)
{
  int vol_temp=0;

  if(vol <= 127 && vol >= 0)
  {
    vol_temp = vol * 2 + 1; // -95dB ~ +5dB
    es8311_i2c_write_reg(g_i2c_dev, 0x17, vol_temp);
    return 0;
  }

  return -1;
}

static int ADC_GetVolume()
{
  uint8_t vol_temp;

  es8311_i2c_read_reg(g_i2c_dev, 0x17, &vol_temp);

  return  (vol_temp/2-95.5);
}

static void CodecMCLKOut(uint8_t div)
{
  u8 channel = 4;

  tls_pwm_stop(channel);
  uni_pwm5_config(I2S_MCLK);  //I2S_MCLK  attention
  tls_pwm_out_mode_config(channel, UNI_PWM_OUT_MODE_INDPT);
  tls_pwm_cnt_type_config(channel, UNI_PWM_CNT_TYPE_EDGE_ALIGN_OUT);
  tls_pwm_freq_config(channel, 0, div - 1);
  tls_pwm_duty_config(channel, (div-1)/2);
  tls_pwm_loop_mode_config(channel, UNI_PWM_LOOP_TYPE_LOOP);
  tls_pwm_out_inverse_cmd(channel, DISABLE);
  tls_pwm_output_en_cmd(channel, UNI_PWM_OUT_EN_STATE_OUT);
  tls_pwm_start(channel);
}

static void CodecSetSampleRate(unsigned int sr)
{
  int ret = 0;
  static int mclk = 0;
  static int switchflag = 0;

  switch(sr)
  {
    case 8000:

      if (mclk == 0)
      {
        CodecMCLKOut(5);
        mclk = 1;
      }
#if CODEC_ALWAYS_AS_MASTER_FTR
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x06, 0x18);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x07, 0x03);  //
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x08, 0xE7);  // 0xff
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
#endif

      ret = es8311_i2c_write_reg(g_i2c_dev, 0x02, 0x08);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x05, 0x44);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

      if (switchflag == 0)
      {
        switchflag = 1;
        ret = es8311_i2c_write_reg(g_i2c_dev, 0x03, 0x19);
        CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
        ret = es8311_i2c_write_reg(g_i2c_dev, 0x04, 0x19);
        CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      }

      break;
    case 16000:

      if (mclk == 0)
      {
        CodecMCLKOut(5);
        mclk = 1;
      }
#if CODEC_ALWAYS_AS_MASTER_FTR
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x06, 0x0F);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x07, 0x01);  //
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x08, 0xF3);  // 0xff
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
#endif
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x02, 0x90);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x05, 0x00);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

      if (switchflag == 0)
      {
        switchflag = 1;
        ret = es8311_i2c_write_reg(g_i2c_dev, 0x03, 0x19);
        CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
        ret = es8311_i2c_write_reg(g_i2c_dev, 0x04, 0x19);
        CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      }

      break;

    case 32000:
      if (mclk == 0)
      {
        CodecMCLKOut(5);
        mclk = 1;
      }
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x02, 0x18);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x05, 0x44);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      if (switchflag == 0)
      {
        switchflag = 1;
        ret = es8311_i2c_write_reg(g_i2c_dev, 0x03, 0x19);
        CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
        ret = es8311_i2c_write_reg(g_i2c_dev, 0x04, 0x19);
        CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      }

      break;

    case 44100:
      mclk = 0;
      switchflag = 0;
      CodecMCLKOut(28);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x02, (0x03 << 3));
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x05, 0x00);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x03, 0x10);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x04, 0x10);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

      break;

    case 22050:
      mclk = 0;
      switchflag = 0;
      CodecMCLKOut(28);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x02, (0x02 << 3));
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x05, 0x00);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x03, 0x10);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x04, 0x10);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

      break;

    case 11025:
      mclk = 0;			
      switchflag = 0;
      CodecMCLKOut(28);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x02, (0x01 << 3));
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x05, 0x00);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x03, 0x10);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x04, 0x10);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

      break;

    case 48000:
      mclk = 0;		
      switchflag = 0;
      CodecMCLKOut(26);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x02, (0x03 << 3));
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x05, 0x00);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x03, 0x10);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x04, 0x10);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

      break;
    case 24000:
      mclk = 0;		
      switchflag = 0;
      CodecMCLKOut(26);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x02, (0x02 << 3));
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x05, 0x00);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x03, 0x10);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x04, 0x10);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

      break;
    case 12000:
      mclk = 0;			
      switchflag = 0;
      CodecMCLKOut(26);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x02, (0x01 << 3));
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x05, 0x00);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x03, 0x10);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);
      ret = es8311_i2c_write_reg(g_i2c_dev, 0x04, 0x10);
      CHECK_RET_TAG_WITH_GOTO(ret == 0, err);

      break;
    default:
      break;
  }

  return;

err:
  LOGE(TAG, "es8311 config failed");
}



static int I2s_tx_dma_send(uint8_t *data, uint16_t len)
{
  aos_check_return_einval(len > 0);

  us615_i2s_t *handle = &codec_instance[0];

  DMA_SRCADDR_REG(handle->dma_tx.channel) = (uint32_t )data;

  DMA_CTRL_REG(handle->dma_tx.channel) &= ~0xFFFF00;

  DMA_CTRL_REG(handle->dma_tx.channel) |= (len<<8);

  DMA_CHNLCTRL_REG(handle->dma_tx.channel) = DMA_CHNL_CTRL_CHNL_ON;

  uni_i2s_tx_dma_enable(1);
  uni_i2s_tx_enable(1);
  uni_i2s_enable(1);
}

static uint8_t *i2s_tx_buffer;
static int I2s_tx_send()
{
  us615_i2s_t *i2s_handle = &codec_instance[0];

  if (!i2s_tx_buffer) {
    i2s_tx_buffer = (uint8_t *)aos_malloc_check(i2s_handle->tx_bufhdl.period);
  }

  uint32_t send_bytes = dev_ringbuf_out(&i2s_handle->tx_bufhdl.fifo,i2s_tx_buffer,i2s_handle->tx_bufhdl.period);
  I2s_tx_dma_send(i2s_tx_buffer, send_bytes);

  return 0;
}

static void I2s_DMA_TX_IRQHandler(void *p)
{
  us615_i2s_t *i2s_handle = &codec_instance[0];
  int ret_len = 0;
  codec_event_t event;

  ret_len = dev_ringbuf_len(&i2s_handle->tx_bufhdl.fifo);

  if(ret_len >= i2s_handle->tx_bufhdl.period)
  {
    if(i2s_handle->tx_bufhdl.cb !=NULL)
    {
      event = CODEC_EVENT_PERIOD_WRITE_COMPLETE;
      i2s_handle->tx_bufhdl.cb(0,event,i2s_handle->tx_bufhdl.cb_arg);
    }
    I2s_tx_send();
  }
  else
  {
    // printf("empxx\n");
    if(i2s_handle->tx_bufhdl.cb !=NULL)
    {
      i2s_handle->out_sta &= ~CODEC_RUNING;
      event = CODEC_EVENT_WRITE_BUFFER_EMPTY;
      i2s_handle->tx_bufhdl.cb(0,event,i2s_handle->tx_bufhdl.cb_arg);
    }
  }
}

static int I2s_tx_dma_init()
{
  us615_i2s_t *handle = &codec_instance[0];
  uint32_t dma_ctrl;

  handle->dma_tx.channel = tls_dma_request(UNI_I2S_TX_DMA_CHANNEL, TLS_DMA_FLAGS_CHANNEL_SEL(TLS_DMA_SEL_I2S_TX) | TLS_DMA_FLAGS_HARD_MODE);

  if(handle->dma_tx.channel == 0)
  {
    return -1;
  }
  if (tls_dma_stop(handle->dma_tx.channel))
  {
    return -1;
  }

  tls_dma_irq_register(handle->dma_tx.channel, I2s_DMA_TX_IRQHandler, &handle->dma_tx, TLS_DMA_IRQ_TRANSFER_DONE);

  DMA_INTMASK_REG &= ~(0x02<<(handle->dma_tx.channel*2));
  DMA_DESTADDR_REG(handle->dma_tx.channel) = HR_I2S_TX;
  DMA_CTRL_REG(handle->dma_tx.channel) = DMA_CTRL_SRC_ADDR_INC | DMA_CTRL_DATA_SIZE_WORD | DMA_CTRL_BURST_SIZE1;
  DMA_MODE_REG(handle->dma_tx.channel) = DMA_MODE_SEL_I2S_TX | DMA_MODE_HARD_MODE;
  DMA_CTRL_REG(handle->dma_tx.channel) &= ~0xFFFF00;

  return 0;
}

static uint32_t DMA_ringbuf_in(dev_ringbuf_t *fifo,  uint32_t len)
{
  uint32_t writelen = 0, tmplen = 0;

  if(dev_ringbuf_is_full(fifo))
    return 0;

  tmplen = fifo->size - fifo->data_len;
  writelen = tmplen > len ? len : tmplen;

  uint32_t stat = csi_irq_save();
  fifo->write = (fifo->write + writelen) % fifo->size;
  fifo->data_len += writelen;
  csi_irq_restore(stat);
  return writelen;
}

static void i2sDmaRecvCpltCallback(uni_dma_handler_type *hdma)
{
  codec_event_t event;
  uint32_t ret = 0;
  us615_i2s_t *i2s_handle = &codec_instance[0];

#if USE_LOCAL_ADC_BUFF
  ret = dev_ringbuf_in(&i2s_handle->rx_bufhdl.fifo, g_adc_buf + sizeof(g_adc_buf) / 2, sizeof(g_adc_buf) / 2);
#else
  ret = DMA_ringbuf_in(&i2s_handle->rx_bufhdl.fifo,i2s_handle->rx_bufhdl.period);
#endif

  if(ret == 0){
    event = CODEC_EVENT_READ_BUFFER_FULL;
  }
  else{
    event = CODEC_EVENT_PERIOD_READ_COMPLETE;
  }

  if(i2s_handle->rx_bufhdl.cb !=NULL)
  {
    i2s_handle->rx_bufhdl.cb(0,event,i2s_handle->rx_bufhdl.cb_arg);
  }

  //printf("recieve complete \n");
}

static void i2sDmaRecvHalfCpltCallback(uni_dma_handler_type *hdma)
{
  codec_event_t event;
  uint32_t ret = 0;
  us615_i2s_t *i2s_handle = &codec_instance[0];

#if USE_LOCAL_ADC_BUFF
  ret = dev_ringbuf_in(&i2s_handle->rx_bufhdl.fifo, g_adc_buf, sizeof(g_adc_buf) / 2);
#else
  ret = DMA_ringbuf_in(&i2s_handle->rx_bufhdl.fifo,i2s_handle->rx_bufhdl.period);
#endif

  if(ret == 0){
    event = CODEC_EVENT_READ_BUFFER_FULL;
  }
  else{
    event = CODEC_EVENT_PERIOD_READ_COMPLETE;
  }

  if(i2s_handle->rx_bufhdl.cb !=NULL)
  {
    i2s_handle->rx_bufhdl.cb(0,event,i2s_handle->rx_bufhdl.cb_arg);
  }
  //printf("recieve half \n");
}

int32_t csi_codec_init(uint32_t idx)
{
  int ret = 0;
  us615_i2s_t *handle = &codec_instance[idx];

  if (idx >= CONFIG_CODEC_NUM) {
    return -1;
  }

  ret = target_i2s_init(idx, &handle->base, &handle->irq_num, &handle->irq_handle);

  if (ret != idx) {
    return -1;
  }

  //register interrupt handler
  memset(&handle->dma_tx, 0, sizeof(uni_dma_handler_type));
  memset(&handle->dma_rx, 0, sizeof(uni_dma_handler_type));

  handle->out_sta = 0;
  handle->in_sta = 0;

  handle->dma_rx.XferCpltCallback = i2sDmaRecvCpltCallback;
  handle->dma_rx.XferHalfCpltCallback = i2sDmaRecvHalfCpltCallback;

  ret = I2s_tx_dma_init();
  if(ret){
    return -1;
  }

  extern ATTRIBUTE_ISR void DMA_Channel0_IRQHandler(void);
  extern ATTRIBUTE_ISR void DMA_Channel1_IRQHandler(void);
  extern ATTRIBUTE_ISR void DMA_Channel2_IRQHandler(void);
  extern ATTRIBUTE_ISR void DMA_Channel3_IRQHandler(void);
  extern ATTRIBUTE_ISR void DMA_Channel4_7_IRQHandler(void);
  drv_irq_register(DMA_Channel0_IRQn, DMA_Channel0_IRQHandler);
  drv_irq_register(DMA_Channel1_IRQn, DMA_Channel1_IRQHandler);
  drv_irq_register(DMA_Channel2_IRQn, DMA_Channel2_IRQHandler);
  drv_irq_register(DMA_Channel3_IRQn, DMA_Channel3_IRQHandler);
  drv_irq_register(DMA_Channel4_7_IRQn,DMA_Channel4_7_IRQHandler);

  UserIOInit();

  // init CLOCK
  CodecMCLKOut(5);

  //init es8311
  ret = es8311_init();
  if(ret){
    return -1;
  }

  //other
  drv_irq_register(handle->irq_num, handle->irq_handle);
  drv_irq_enable(handle->irq_num);   
  return 0;
}

void csi_codec_lpm(uint32_t idx, codec_lpm_state_t state)
{        

}

void csi_codec_uninit(uint32_t idx)
{
  us615_i2s_t *handle  = &codec_instance[0];

  drv_irq_disable(handle->irq_num);
  drv_irq_unregister(handle->irq_num);

  drv_irq_unregister(DMA_Channel0_IRQn);
  drv_irq_unregister(DMA_Channel1_IRQn);
  drv_irq_unregister(DMA_Channel2_IRQn);
  drv_irq_unregister(DMA_Channel3_IRQn);
  drv_irq_unregister(DMA_Channel4_7_IRQn);

  /*to do reset the codec module*/
}

int32_t csi_codec_power_control(int32_t idx, csi_power_stat_e state)
{
  return -4;
}

int32_t csi_codec_input_open(codec_input_t *handle)
{
  us615_i2s_t *i2s_handle  = &codec_instance[0];

  if (handle == NULL) {
    return -1;
  }
  i2s_handle->rx_bufhdl.cb = handle->cb;
  i2s_handle->rx_bufhdl.cb_arg = handle->cb_arg;
  i2s_handle->rx_bufhdl.period = handle->period;
  i2s_handle->rx_bufhdl.fifo.buffer = handle->buf;
  i2s_handle->rx_bufhdl.fifo.size = handle->buf_size;
  i2s_handle->rx_bufhdl.fifo.read = i2s_handle->rx_bufhdl.fifo.write = i2s_handle->rx_bufhdl.fifo.data_len = 0;
  LOGD(TAG, "period=%d, fifo_size=%d", i2s_handle->rx_bufhdl.period, i2s_handle->rx_bufhdl.fifo.size);

  i2s_handle->in_sta |= CODEC_INIT;

  //printf("csi_codec_input_open \n");

  return 0;
}

int32_t csi_codec_input_close(codec_input_t *handle)
{
  if (handle == NULL) {
    return -1;
  }
  us615_i2s_t *i2s_handle  = &codec_instance[0];

  return 0;
}

int32_t csi_codec_input_config(codec_input_t *handle, codec_input_config_t *config)
{
  us615_i2s_t *i2s_handle  = &codec_instance[0];

  if (handle == NULL || config == NULL) {
    return 0;
  }

  if ((i2s_handle->in_sta & CODEC_INIT) == 0) {
    return -1;
  }

  switch(config->bit_width)
  {
    case 8:
      i2s_handle->paras.I2S_DataFormat = 	I2S_DataFormat_8;
      break;
    case 16:
      i2s_handle->paras.I2S_DataFormat = 	I2S_DataFormat_16;
      break;

    case 24:
      i2s_handle->paras.I2S_DataFormat = 	I2S_DataFormat_24;
      break;
    case 32:
      i2s_handle->paras.I2S_DataFormat = 	I2S_DataFormat_32;
      break;
    default:
      return -1;
  }

  if(config->sample_rate != 8000 && config->sample_rate != 16000 && config->sample_rate != 32000 &&
      config->sample_rate != 11025 && config->sample_rate != 22050 && config->sample_rate != 44100 &&
      config->sample_rate != 12000 && config->sample_rate != 24000 && config->sample_rate != 48000)
  {
    return -1;
  }

  if(config->sample_rate == 8000 && config->bit_width == 8){
    return -1;
  }

  if(config->channel_num)
  {
    i2s_handle->paras.I2S_Mode_SS = I2S_CTRL_MONO; 
  }
  else
  {
    i2s_handle->paras.I2S_Mode_SS = I2S_CTRL_STEREO; 
  }

  i2s_handle->paras.I2S_Mode_LR = I2S_LEFT_CHANNEL;
  i2s_handle->paras.I2S_Trans_STD = I2S_Standard;
  i2s_handle->paras.I2S_AudioFreq = config->sample_rate;


#if CODEC_ALWAYS_AS_MASTER_FTR	
  i2s_handle->paras.I2S_Mode_MS = I2S_MODE_SLAVE;
  i2s_handle->paras.I2S_MclkFreq = 8000000;
#else	
  i2s_handle->paras.I2S_Mode_MS = I2S_MODE_MASTER;
  i2s_handle->paras.I2S_MclkFreq = 8000000;
#endif	

  //CodecSetSampleRate(i2s_handle->paras.I2S_AudioFreq);
  es8311_reinit(i2s_handle->paras.I2S_AudioFreq);

  uni_i2s_port_init(&i2s_handle->paras);

  // printf("csi_codec_input_config\n");

  return 0;
}

int32_t csi_codec_input_start(codec_input_t *handle)
{
  us615_i2s_t *i2s_handle  = &codec_instance[0];
  int ret = 0;
  if (handle == NULL) {
    return -1;
  }

  if ((i2s_handle->in_sta & CODEC_INIT) == 0) {
    return -1;
  }

  if (i2s_handle->in_sta & CODEC_RUNING) {
    return 0;
  }

#if USE_LOCAL_ADC_BUFF
  ret = uni_i2s_receive_dma(&i2s_handle->dma_rx, (uint16_t *)g_adc_buf, sizeof(g_adc_buf)/2);
#else
  ret = uni_i2s_receive_dma(&i2s_handle->dma_rx, (uint16_t *)i2s_handle->rx_bufhdl.fifo.buffer, i2s_handle->rx_bufhdl.fifo.size / 2);
#endif
  if(ret == 0){
    i2s_handle->in_sta |= CODEC_RUNING;
  }

  //printf("csi_codec_input_start\n");
  return 0;
}

int32_t csi_codec_input_stop(codec_input_t *handle)
{
  us615_i2s_t *i2s_handle  = &codec_instance[0];

  if (handle == NULL) {
    return -1;
  }

  uni_i2s_rx_dma_enable(0);
  uni_i2s_rx_enable(0);
  uni_i2s_enable(0);
  dev_ringbuff_reset(&i2s_handle->rx_bufhdl.fifo);

  i2s_handle->in_sta &= ~CODEC_RUNING;

  //printf("csi_codec_input_stop\n");
  return 0;
}

uint32_t csi_codec_input_read(codec_input_t *handle, uint8_t *buf, uint32_t length)
{
  us615_i2s_t *i2s_handle  = &codec_instance[0];
  if (handle == NULL) {
    return -1;
  }

  return dev_ringbuf_out(&i2s_handle->rx_bufhdl.fifo, buf, length);
}

uint32_t csi_codec_input_buf_avail(codec_input_t *handle)
{
  us615_i2s_t *i2s_handle  = &codec_instance[0];
  if (handle == NULL) {
    return -1;
  }

  return dev_ringbuf_len(&i2s_handle->rx_bufhdl.fifo);
}

int32_t csi_codec_input_buf_reset(codec_input_t *handle)
{
  us615_i2s_t *i2s_handle  = &codec_instance[0];
  if (handle == NULL) {
    return -1;
  }

  dev_ringbuff_reset(&i2s_handle->rx_bufhdl.fifo);

  return 0;
}

int32_t csi_codec_input_pause(codec_input_t *handle)
{
  if (handle == NULL) {
    return -1;
  }

  uni_i2s_enable(0);

  return 0;
}

int32_t csi_codec_input_resume(codec_input_t *handle)
{
  if (handle == NULL) {
    return -1;
  }

  uni_i2s_enable(1);

  return 0;
}

int32_t csi_codec_output_open(codec_output_t *handle)
{
  us615_i2s_t *i2s_handle  = &codec_instance[0];

  if (handle == NULL) {
    return -1;
  }

  i2s_handle->tx_bufhdl.cb = handle->cb;
  i2s_handle->tx_bufhdl.cb_arg = handle->cb_arg;
  i2s_handle->tx_bufhdl.period = handle->period;
  i2s_handle->tx_bufhdl.fifo.buffer = handle->buf; 

  i2s_handle->tx_bufhdl.fifo.size = handle->buf_size;
  i2s_handle->tx_bufhdl.fifo.read = i2s_handle->tx_bufhdl.fifo.write = i2s_handle->tx_bufhdl.fifo.data_len = 0;

  i2s_handle->out_sta |= CODEC_INIT;
  UserPACtrl(1);
  //printf("csi_codec_output_open\n");
  return 0;
}

int32_t csi_codec_output_close(codec_output_t *handle)
{
  us615_i2s_t *i2s_handle  = &codec_instance[0];
  if (handle == NULL) {
    return -1;
  }

  if (i2s_handle->out_sta & CODEC_RUNING) {
    return -1;
  }

  return 0;
}


int32_t csi_codec_output_config(codec_output_t *handle, codec_output_config_t *config)
{
  us615_i2s_t *i2s_handle = &codec_instance[0];

  if (handle == NULL || config == NULL) {
    return -1;
  }

  if ((i2s_handle->out_sta & CODEC_INIT) == 0) {
    return -1;
  }

  switch(config->bit_width)
  {
    case 8:
      i2s_handle->paras.I2S_DataFormat = 	I2S_DataFormat_8;
      break;
    case 16:
      i2s_handle->paras.I2S_DataFormat = 	I2S_DataFormat_16;
      break;

    case 24:
      i2s_handle->paras.I2S_DataFormat = 	I2S_DataFormat_24;
      break;
    case 32:
      i2s_handle->paras.I2S_DataFormat = 	I2S_DataFormat_32;
      break;
    default:
      return -1;
  }

  if(config->sample_rate != 8000 && config->sample_rate != 16000 && config->sample_rate != 32000 &&
      config->sample_rate != 11025 && config->sample_rate != 22050 && config->sample_rate != 44100 &&
      config->sample_rate != 12000 && config->sample_rate != 24000 && config->sample_rate != 48000)
  {
    return -1;
  }

  if(config->sample_rate == 8000 && config->bit_width == 8){
    return -1;
  }

  if(config->mono_mode_en)
  {
    i2s_handle->paras.I2S_Mode_SS = I2S_CTRL_MONO; 
  }
  else
  {
    i2s_handle->paras.I2S_Mode_SS = I2S_CTRL_STEREO; 
  }

  i2s_handle->paras.I2S_Mode_LR = I2S_LEFT_CHANNEL;
  i2s_handle->paras.I2S_Trans_STD = I2S_Standard;
  i2s_handle->paras.I2S_AudioFreq = config->sample_rate;

#if CODEC_ALWAYS_AS_MASTER_FTR	

  i2s_handle->paras.I2S_Mode_MS = I2S_MODE_SLAVE;

  if (config->sample_rate == 8000){
    i2s_handle->paras.I2S_MclkFreq = 256 * i2s_handle->paras.I2S_AudioFreq * 2;
  }
  else{
    i2s_handle->paras.I2S_MclkFreq = 256 * i2s_handle->paras.I2S_AudioFreq;
  }
#else	
  i2s_handle->paras.I2S_Mode_MS = I2S_MODE_MASTER;
  i2s_handle->paras.I2S_MclkFreq = 8000000;

#endif	

  //CodecSetSampleRate(i2s_handle->paras.I2S_AudioFreq);
  es8311_reinit(i2s_handle->paras.I2S_AudioFreq);

  uni_i2s_port_init(&i2s_handle->paras);

  //printf("csi_codec_output_config\n");
  return 0;
}

int32_t csi_codec_output_start(codec_output_t *handle)
{
  int ret_len = 0;
  us615_i2s_t *i2s_handle  = &codec_instance[0];

  if (handle == NULL) {
    return -1;
  }
  if ((i2s_handle->out_sta & CODEC_INIT) == 0) {
    return -1;
  }

  if (i2s_handle->out_sta & CODEC_RUNING) {
    return 0;
  }

  i2s_handle->out_sta |= CODEC_READT_TO_START;

  ret_len = dev_ringbuf_len(&i2s_handle->tx_bufhdl.fifo);

  if (ret_len != 0) {
    I2s_tx_send();
    i2s_handle->out_sta |= CODEC_RUNING;
    UserPACtrl(1);
  }
  //printf("csi_codec_output_start\n");
  return 0;
}


int32_t csi_codec_output_stop(codec_output_t *handle)
{
  if (handle == NULL) {
    return -1;
  }
  us615_i2s_t *i2s_handle = &codec_instance[0];

  i2s_handle->out_sta &= ~CODEC_RUNING;

  UserPACtrl(0);
  uni_i2s_tx_enable(0);
  dev_ringbuff_reset(&i2s_handle->tx_bufhdl.fifo);

  //printf("csi_codec_output_stop\n");

  return 0;
}

uint32_t csi_codec_output_write(codec_output_t *handle, uint8_t *buf, uint32_t length)
{
  us615_i2s_t *i2s_handle  = &codec_instance[0];

  if (handle == NULL || buf == NULL) {
    return -1;
  }
  int ret_len = dev_ringbuf_in(&i2s_handle->tx_bufhdl.fifo, buf, length);

  if ((i2s_handle->out_sta & CODEC_RUNING) == 0
      && (i2s_handle->out_sta & CODEC_READT_TO_START)) {
    I2s_tx_send();
    i2s_handle->out_sta |= CODEC_RUNING;
    UserPACtrl(1);
    //printf("csi_codec_output_write 0\n");
  }

  return ret_len;
}

int32_t csi_codec_output_buf_reset(codec_output_t *handle)
{
  us615_i2s_t *i2s_handle = &codec_instance[0];

  if (handle == NULL) {
    return -1;
  }

  dev_ringbuff_reset(&i2s_handle->tx_bufhdl.fifo);

  return 0;
}

uint32_t csi_codec_output_buf_avail(codec_output_t *handle)
{
  us615_i2s_t *i2s_handle = &codec_instance[0];
  if (handle == NULL) {
    return -1;
  }

  return dev_ringbuf_avail(&i2s_handle->tx_bufhdl.fifo);;
}

int32_t csi_codec_output_pause(codec_output_t *handle)
{
  if (handle == NULL) {
    return -1;
  }

  uni_i2s_tx_enable(0);
  return 0;
}

int32_t csi_codec_output_resume(codec_output_t *handle)
{
  if (handle == NULL) {
    return -1;
  }

  uni_i2s_tx_enable(1);
  return 0;
}

int32_t csi_codec_input_set_digital_gain(codec_input_t *handle, int32_t gain)
{
  if (handle == NULL) {
    return -1;
  }
  if (ADC_CodecVolume(gain))
    return -1;

  return 0;
}

int32_t csi_codec_input_get_digital_gain(codec_input_t *handle, int32_t *gain)
{
  if (handle == NULL) {
    return -1;
  }

  *gain= ADC_GetVolume();

  return 0;
}

int32_t csi_codec_input_set_analog_gain(codec_input_t *handle, int32_t gain)
{
  return -4;
}

int32_t csi_codec_input_get_analog_gain(codec_input_t *handle, int32_t *gain)
{
  return -4;
}

int32_t csi_codec_input_set_mixer_gain(codec_input_t *handle, int32_t gain)
{
  if (handle == NULL) {
    return -1;
  }

  if (ADC_CodecVolume(gain))
    return -1;

  return 0;
}

int32_t csi_codec_input_get_mixer_gain(codec_input_t *handle, int32_t *gain)
{
  if (handle == NULL) {
    return -1;
  }

  *gain= ADC_GetVolume();

  return 0;
}

int32_t csi_codec_input_mute(codec_input_t *handle, int en)
{
  if (handle == NULL) {
    return -1;
  }
#if 0
  if(en){
    CodecMute(1);
  }
  else{
    CodecMute(0);
  }
#endif
  return 0;
}

int32_t csi_codec_output_set_digital_left_gain(codec_output_t *handle, int32_t val)
{
  if (handle == NULL) {
    return -1;
  }

  if (DAC_CodecVolume(val))
    return -1;


  return 0;
}

int32_t csi_codec_output_set_digital_right_gain(codec_output_t *handle, int32_t val)
{
  if (handle == NULL) {
    return -1;
  }

  if (DAC_CodecVolume(val))
    return -1;

  return 0;
}

int32_t csi_codec_output_set_analog_left_gain(codec_output_t *handle, int32_t val)
{

  return -4;
}

int32_t csi_codec_output_set_analog_right_gain(codec_output_t *handle, int32_t val)
{

  return -4;
}

int32_t csi_codec_output_set_mixer_left_gain(codec_output_t *handle, int32_t val)
{
  if (handle == NULL) {
    return -1;
  }

  if (DAC_CodecVolume(val))
    return -1;

  return 0;
}
int32_t csi_codec_output_set_mixer_right_gain(codec_output_t *handle, int32_t val)
{
  if (handle == NULL) {
    return -1;
  }

  if (DAC_CodecVolume(val))
    return -1;

  return 0;
}

int32_t csi_codec_output_get_mixer_left_gain(codec_output_t *handle, int32_t *val)
{
  if (handle == NULL) {
    return -1;
  }

  *val= DAC_GetVolume();

  return 0;
}

int32_t csi_codec_output_get_mixer_right_gain(codec_output_t *handle, int32_t *val)
{
  if (handle == NULL) {
    return -1;
  }

  *val= DAC_GetVolume();

  return 0;
}

int32_t csi_codec_output_get_digital_left_gain(codec_output_t *handle, int32_t *val)
{
  if (handle == NULL) {
    return -1;
  }

  *val= DAC_GetVolume();

  return 0;
}

int32_t csi_codec_output_get_digital_right_gain(codec_output_t *handle, int32_t *val)
{
  if (handle == NULL) {
    return -1;
  }

  *val= DAC_GetVolume();

  return 0;
}

int32_t csi_codec_output_get_analog_left_gain(codec_output_t *handle, int32_t *val)
{
  return -4;
}

int32_t csi_codec_output_get_analog_right_gain(codec_output_t *handle, int32_t *val)
{
  return -4;
}

int32_t csi_codec_output_mute(codec_output_t *handle, int en)
{
  if (handle == NULL) {
    return -1;
  }

  if(en){
    DAC_CodecMute(1);
  }
  else{
    DAC_CodecMute(0);
  }

  return 0;
}

int32_t csi_codec_vad_enable(codec_input_t *handle, int en)
{
  return -4;
}
#endif

