#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uni_include.h"
#include "uni_gpio.h"
#include "drv/timer.h"
#include "drv/gpio.h"

#define FLASH_EXT_ADDR     0x1000
#define FLASH_PAGE_SIZE    0x100

static enum tls_io_name ir_rx_pin;
static timer_handle_t timer_handler = NULL;
static gpio_pin_handle_t gpio_pin_hander = NULL;
static unsigned short *rx_data = NULL;
static volatile int rx_data_offset = 0;
static int rx_data_len = 0;
static uint32_t g_last_t;

//extern const unsigned char g_infrared_index_buf[6398];

static uint32_t IrGetDvalue(uint32_t curr_t, uint32_t last_t)
{
	if(curr_t >= last_t)
		return (curr_t - last_t);
	else
		return ((uint32_t)(0xFFFFFFFF) - last_t + curr_t);
}

static void rx_pin_cb(int32_t idx)
{
	uint32_t curr_t;
	bool pin_val;

	if(rx_data && rx_data_offset < rx_data_len)
	{
		csi_timer_get_current_value(timer_handler, &curr_t);
		csi_gpio_pin_read(gpio_pin_hander, &pin_val);
		if(pin_val)
		{
			rx_data[rx_data_offset++] = (unsigned short)IrGetDvalue(curr_t, g_last_t);
		}
		else
		{
			if(rx_data_offset > 0)
			{
				rx_data[rx_data_offset++] = (unsigned short)IrGetDvalue(curr_t, g_last_t);
			}
		}
		g_last_t = curr_t;
	}
}
int infrared_rx(unsigned short *in, int length, uint32_t timerout_ms)
{
	uint32_t ticks = HZ * timerout_ms / 1000;
	uint32_t curr_tick = tls_os_get_time();
	int last_offset = 0;
	int volatile idle_times = 0;

	if(in == NULL || length <= 0)
	{
		printf("infrared_rx invalid parameters\n");
		return -2;
	}
	
	if(rx_data == NULL)
	{
		rx_data = in;
		rx_data_len = length;
		rx_data_offset = 0;

		while(1)
		{
			if(rx_data_offset > 0)
			{
				if(rx_data_offset == last_offset)
				{
					idle_times++;
				}
				else
				{
					idle_times = 0;
				}
			}
			last_offset = rx_data_offset;
			tls_os_time_delay(1);
			if(tls_os_get_time() - curr_tick > ticks || idle_times > 100 || rx_data_offset >= rx_data_len)
			{
				if(rx_data_offset < rx_data_len)
				{
					//rx_data[rx_data_offset++] = 1000;
				}
				break;
			}
		}
		rx_data = NULL;
		return rx_data_offset;
	}
	else
	{
		return -1;
	}
}
int infrared_rx_init(int timer_id, enum tls_io_name pin)
{
	ir_rx_pin = pin;
	
	timer_handler = csi_timer_initialize(timer_id, NULL);
	if(timer_handler)
	{
		csi_timer_config(timer_handler, TIMER_MODE_RELOAD);
		csi_timer_set_timeout(timer_handler, 0xFFFFFFFF);
		csi_timer_start(timer_handler);
	}
	else
	{
		printf("timer %d initialize fail\n", timer_id);
		return -1;
	}
	
	gpio_pin_hander = csi_gpio_pin_initialize(pin, rx_pin_cb);
	if(gpio_pin_hander)
	{
	    csi_gpio_pin_config_mode(gpio_pin_hander, GPIO_MODE_PULLNONE);
	    csi_gpio_pin_config_direction(gpio_pin_hander, GPIO_DIRECTION_INPUT);
	    csi_gpio_pin_set_irq(gpio_pin_hander, UNI_GPIO_IRQ_TRIG_DOUBLE_EDGE, 1);
	}
	else
	{
		printf("gpio pin %d initialize fail\n", pin);
		return -1;
	}
	return 0;
}

void infrared_rx_deinit(void)
{
	csi_timer_stop(timer_handler);
	csi_timer_uninitialize(timer_handler);

	csi_gpio_pin_uninitialize(gpio_pin_hander);
}
int infrared_learn_pwm(unsigned int *pattern, unsigned int out_len, uint32_t timerout_ms)
{
	int err = 0;
	unsigned short *in_buf = NULL;
	unsigned short ret_num;
	int i;

	err = infrared_rx_init(1, UNI_IO_PB_07);
	if(err)
	{
		printf("infrared_rx_init failed error:%d\n", err);
		goto out;
	}
	in_buf = (unsigned short *)tls_mem_alloc(1024*2);
	if(!in_buf)
	{
		printf("malloc rx buffer error\n");
		err = -1;
		goto out;
	}
	ret_num = infrared_rx(in_buf, 1024, timerout_ms);
	if(ret_num > 0)
	{
		printf("rx data:\n");
		for (i = 0; i < ret_num; i++) {
			printf("%d,", in_buf[i]);
		}
		printf("\n");
	}
	else
	{
		printf("infrared_rx error:%d\n", ret_num);
		err = -1;
		goto out;
	}
	if(out_len < ret_num)
	{
		err = -1;
		goto out;
	}
	for(i = 0; i < ret_num; i++)
	{
		pattern[i] = (int)in_buf[i];
	}
	err = ret_num;
out:
	if(in_buf)
	{
		tls_mem_free(in_buf);
	}
	infrared_rx_deinit();
	return err;
}

int infrared_rx_test(void)
{
	int err = 0;
	unsigned short *in_buf = NULL;
	unsigned short ret_num;
	int i;

	err = spiflash_init();
	if(err)
	{
		printf("spi flash init failed\n");
		goto out;
	}
	err = infrared_rx_init(1, UNI_IO_PB_07);
	if(err)
	{
		printf("infrared_rx_init failed error:%d\n", err);
		goto out;
	}
	in_buf = (unsigned short *)tls_mem_alloc(1024*2);
	if(!in_buf)
	{
		printf("malloc rx buffer error\n");
		err = -1;
		goto out;
	}
	ret_num = infrared_rx(in_buf, 1024, 5000);
	if(ret_num > 0)
	{
		printf("rx data:\n");
		for (i = 0; i < ret_num; i++) {
			printf("%d,", in_buf[i]);
		}
		printf("\n");
	}
	else
	{
		printf("infrared_rx error:%d\n", ret_num);
		err = -1;
		goto out;
	}
	
out:
	if(in_buf)
	{
		tls_mem_free(in_buf);
	}
	infrared_rx_deinit();
	return err;
}
