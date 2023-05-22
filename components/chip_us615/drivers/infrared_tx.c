#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uni_type_def.h"
#include "uni_io.h"
#include "uni_include.h"
#include "uni_gpio_afsel.h"
//#include "infrared_index.h"
#include <drv/spi.h>
#include <drv/spiflash.h>
#include "drv/timer.h"
#include "drv/pwm.h"
#include <pinmux.h>

#define FLASH_EXT_ADDR     0x1000
#define FLASH_PAGE_SIZE    0x100
static spiflash_handle_t g_spiflash_handler = NULL;

unsigned short trans_pulse(unsigned short *out, unsigned short ret_num,
		unsigned short frequency, unsigned char frame_count) {
	unsigned short i;
	int pattern_value;
	double pulse;

	pulse = (double) 1000000 / frequency;
	for (i = 0; i < ret_num; i++) {
		pattern_value = pulse * out[i];
		if (pattern_value > 0xFFFF) {
			pattern_value = 0xFFFF;
		}

		out[i] = pattern_value;
	}

	if (frame_count > 1) {
		for (i = 1; i < frame_count; i++) {
			memcpy(&out[ret_num * i], out, ret_num * sizeof(out[0]));
		}
	}

	return ret_num * frame_count;
}

static void dump_data(uint8_t *d, uint32_t s)
{
    int i;

    for (i = 0; i < s; i++) {
        if (((i % 16) == 0)) {
            printf(" \n");
        }
        printf("%02x ", d[i]);
    }
}

static timer_handle_t timer_handler = NULL;
static pwm_handle_t pwm_handler = NULL;
static int ir_pwm_ch = -1;
static int tx_data_offset = 0;
static int tx_data_len = 0;
static int tx_level = 1;
static unsigned short * tx_data_buf = NULL;
static int tx_pin_check(enum tls_io_name pin)
{
	int ir_pwm_ch = 0;
	
	if(pin == UNI_IO_PB_00 || pin == UNI_IO_PB_19)
	{
		ir_pwm_ch = 0;
		uni_pwm1_config(pin);
	}
	else if(pin == UNI_IO_PB_01 || pin == UNI_IO_PB_20)
	{
		ir_pwm_ch = 1;
		uni_pwm2_config(pin);
	}
	else if(pin == UNI_IO_PA_00 || pin == UNI_IO_PB_02)
	{
		ir_pwm_ch = 2;
		uni_pwm3_config(pin);
	}
	else if(pin == UNI_IO_PA_01 || pin == UNI_IO_PB_03)
	{
		ir_pwm_ch = 3;
		uni_pwm4_config(pin);
	}
	else if(pin == UNI_IO_PA_04 || pin == UNI_IO_PA_07)
	{
		ir_pwm_ch = 4;
		uni_pwm5_config(pin);
	}
	else
	{
		return -1;
	}
	
	return ir_pwm_ch;
}
static void tx_bit(uint8_t out_level, uint32_t time)
{
	if(out_level)
		csi_pwm_start(pwm_handler, ir_pwm_ch);
	else
		csi_pwm_stop(pwm_handler, ir_pwm_ch);
	csi_timer_stop(timer_handler);
	csi_timer_set_timeout(timer_handler, time);
	csi_timer_start(timer_handler);
}

static void tx_timer_cb(int32_t idx, timer_event_e event)
{
	if(tx_data_buf && tx_data_offset < tx_data_len)
	{
		tx_level = !tx_level;
		tx_bit(tx_level, (uint32_t)tx_data_buf[tx_data_offset++]);
	}
	else
	{
		csi_pwm_stop(pwm_handler, ir_pwm_ch);
		csi_timer_stop(timer_handler);
		tx_data_buf = NULL;
	}
}
int infrared_tx(unsigned short *out, int length)
{
	if(!tx_data_buf)
	{
		tx_data_buf = out;
		tx_level = 1;
		tx_data_offset = 0;
		tx_data_len = length;
		tx_bit(tx_level, (uint32_t)out[tx_data_offset++]);
		while(tx_data_buf)
		{
			tls_os_time_delay(1);
		}
		return length;
	}
	return 0;
}
int infrared_tx_init(int timer_id, enum tls_io_name pin)
{
	int ir_tx_timer_id = timer_id;

	if((ir_pwm_ch = tx_pin_check(pin)) < 0)
		return -1;
	
	timer_handler = csi_timer_initialize(ir_tx_timer_id, tx_timer_cb);
	if(timer_handler)
	{
		csi_timer_config(timer_handler, TIMER_MODE_FREE_RUNNING);
		csi_timer_set_timeout(timer_handler, 9000);
	}
	else
	{
		printf("timer %d initialize fail\n", ir_tx_timer_id);
		return -1;
	}

	pwm_handler = csi_pwm_initialize(ir_pwm_ch);
	if(pwm_handler)
	{
		csi_pwm_config(pwm_handler, ir_pwm_ch, 26, 7);
	}
	else
	{
		printf("pwm %d initialize fail\n", ir_pwm_ch);
		return -1;
	}
	
	return 0;
}

void infrared_tx_deinit(void)
{
	csi_timer_stop(timer_handler);
	csi_timer_uninitialize(timer_handler);
	csi_pwm_stop(pwm_handler, ir_pwm_ch);
	csi_pwm_uninitialize(pwm_handler);
}

int spiflash_init(void)
{
	spiflash_handle_t fh;
	spi_handle_t spi_handler;
    int err;

	if(g_spiflash_handler)
		return 0;

	drv_pinmux_config(PB0, PB0_SPI_DI);
	drv_pinmux_config(PB1, PB1_SPI_CK);
	drv_pinmux_config(PB4, PB4_SPI_CS);
	drv_pinmux_config(PB5, PB5_SPI_DO);

	spi_handler = csi_spi_initialize(0, NULL);
	
    fh = csi_spiflash_initialize(0, NULL);
    if (fh == NULL) {
        printf("spi flash init fail \n");
        return -1;
    }
	g_spiflash_handler = fh;
	return 0;
}

spiflash_handle_t spiflash_handler(void)
{
	return g_spiflash_handler;
}

static int pattern[] = {3490,1740,411,474,411,1250,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,
		411,474,411,474,411,474,411,1250,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,1250,411,
		1250,411,1250,411,474,411,474,411,1250,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,
		411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,
		411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,1250,411,1250,411,474,411,
		474,411,474,411,474,411,474,411,10010,3490,1740,411,474,411,1250,411,474,411,474,411,474,411,474,411,
		474,411,474,411,474,411,474,411,474,411,474,411,474,411,1250,411,474,411,474,411,474,411,474,411,474,
		411,474,411,474,411,1250,411,1250,411,1250,411,474,411,474,411,1250,411,474,411,474,411,474,411,474,
		411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,1250,411,474,411,474,411,
		474,411,1250,411,1250,411,474,411,474,411,474,411,474,411,1250,411,474,411,1250,411,1250,411,474,411,
		474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,1250,411,1250,411,474,411,474,411,474,
		411,474,411,1250,411,474,411,1250,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,
		474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,1250,411,1250,411,474,411,474,
		411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,1250,411,1250,411,474,411,474,411,
		474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,
		474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,474,411,1250,411,474,411,474,411,474,
		411,474,411,474,411,474,411,474,411,474,411,474,411,1250,411,1250,411,474,411,474,411,474,411,474,411,
		474,411,474,411,474,411,474,411,1250,411,1250,411,1250,411,1250,411,474,411,1000};

int infrared_tx_test(void) {
	unsigned short ret_num;
	int ret = 0, i;
	unsigned short * data_out_buf = NULL;
	
	ret = infrared_tx_init(2, UNI_IO_PB_02);
	if(ret)
	{
		printf("infrared tx init failed\n");
		goto end;
	}
	ret_num = sizeof(pattern) / 4;
	
	data_out_buf = tls_mem_alloc(ret_num*sizeof(unsigned short));
	if(data_out_buf == NULL)
	{
		printf("malloc data_out_buf size %d failed\n", ret_num);
		ret = 1;
		goto end;
	}
	for (int i = 0; i < ret_num; i++) {
		data_out_buf[i] = (unsigned short)pattern[i];
		printf("%d,", data_out_buf[i]);
	}
	printf("\n");

	ret = infrared_tx(data_out_buf, ret_num);
	printf("infrared tx len %d\n", ret);
	
end:
	if(data_out_buf)
		tls_mem_free(data_out_buf);
	infrared_tx_deinit();
	return ret;
}

