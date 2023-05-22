/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */


/******************************************************************************
 * @file     pinmux.c
 * @brief    source file for the pinmux
 * @version  V1.0
 * @date     02. June 2017
 * @vendor   csky
 * @chip     pangu
 ******************************************************************************/
#include <stdint.h>
#include "pinmux.h"
#include "pin_name.h"
#include <drv/gpio.h>

#include "uni_type_def.h"
#include "uni_io.h"
#include "uni_gpio_afsel.h"

int32_t drv_pinmux_config(pin_name_e pin, pin_func_e pin_func)
{
    if (PIN_FUNC_GPIO == pin_func)
    {
        tls_io_cfg_set((enum tls_io_name)pin, UNI_IO_OPT5_GPIO);
    }
    else if (PA0 == pin)
    {
        if (PA0_SPI_CS == pin_func)
        {
            uni_spi_cs_config(UNI_IO_PA_00);
        }
        else if (PA0_IIS_DO == pin_func)
        {
            uni_i2s_do_config(UNI_IO_PA_00);
        }
        else if (PA0_IIS_MCLK == pin_func)
        {
            uni_i2s_mclk_config(UNI_IO_PA_00);
        }
        else if (PA0_PWM == pin_func)
        {
            uni_pwm3_config(UNI_IO_PA_00);
        }
    }
    else if (PA1 == pin)
    {
        if (PA1_IIS_WS == pin_func)
        {
            uni_i2s_ws_config(UNI_IO_PA_01);
        }
        else if (PA1_IIC_SCL == pin_func)
        {
            uni_i2c_scl_config(UNI_IO_PA_01);
        }
        else if (PA1_PWM == pin_func)
        {
            uni_pwm4_config(UNI_IO_PA_01);
        }
		else if (PA1_ADC == pin_func)
		{
			uni_adc_config(0);
		}
    }
    else if (PA4 == pin)
    {
        if (PA4_IIS_CK == pin_func)
        {
            uni_i2s_ck_config(UNI_IO_PA_04);
        }
        else if (PA4_IIC_SDA == pin_func)
        {
            uni_i2c_sda_config(UNI_IO_PA_04);
        }
        else if (PA4_PWM == pin_func)
        {
            uni_pwm5_config(UNI_IO_PA_04);
        }
		else if (PA4_ADC == pin_func)
		{
			uni_adc_config(1);
        }
    }
    else if (PA7 == pin)
    {
        if (PA7_SPI_DO == pin_func)
        {
            uni_spi_do_config(UNI_IO_PA_07);
        }
        else if (PA7_IIS_DI == pin_func)
        {
            uni_i2s_di_config(UNI_IO_PA_07);
        }
        else if (PA7_IIS_EXTCLK == pin_func)
        {
            uni_i2s_extclk_config(UNI_IO_PA_07);
        }
        else if (PA7_PWM == pin_func)
        {
            uni_pwm5_config(UNI_IO_PA_07);
        }
    }
    else if (PB0 == pin)
    {
        if (PB0_UART3_TX == pin_func)
        {
            uni_uart3_tx_config(UNI_IO_PB_00);
        }
        else if (PB0_SPI_DI == pin_func)
        {
            uni_spi_di_config(UNI_IO_PB_00);
        }
        else if (PB0_PWM == pin_func)
        {
            uni_pwm1_config(UNI_IO_PB_00);
        }
    }
    else if (PB1 == pin)
    {
        if (PB1_UART3_RX == pin_func)
        {
            uni_uart3_rx_config(UNI_IO_PB_01);
        }
        else if (PB1_SPI_CK == pin_func)
        {
            uni_spi_ck_config(UNI_IO_PB_01);
        }
        else if (PB1_PWM == pin_func)
        {
            uni_pwm2_config(UNI_IO_PB_01);
        }
    }
    else if (PB2 == pin)
    {
        if (PB2_UART2_TX == pin_func)
        {
            uni_uart2_tx_scio_config(UNI_IO_PB_02);
        }
        else if (PB2_SPI_CK == pin_func)
        {
            uni_spi_ck_config(UNI_IO_PB_02);
        }
        else if (PB2_PWM == pin_func)
        {
            uni_pwm3_config(UNI_IO_PB_02);
        }
    }
    else if (PB3 == pin)
    {
        if (PB3_UART2_RX == pin_func)
        {
            uni_uart2_rx_config(UNI_IO_PB_03);
        }
        else if (PB3_SPI_DI == pin_func)
        {
            uni_spi_di_config(UNI_IO_PB_03);
        }
        else if (PB3_PWM == pin_func)
        {
            uni_pwm4_config(UNI_IO_PB_03);
        }
    }
    else if (PB4 == pin)
    {
        if (PB4_UART2_RTS == pin_func)
        {
            uni_uart2_rts_scclk_config(UNI_IO_PB_04);
        }
        else if (PB4_UART4_TX == pin_func)
        {
            uni_uart4_tx_config(UNI_IO_PB_04);
        }
        else if (PB4_SPI_CS == pin_func)
        {
            uni_spi_cs_config(UNI_IO_PB_04);
        }
    }
    else if (PB5 == pin)
    {
        if (PB5_UART2_CTS == pin_func)
        {
            uni_uart2_cts_config(UNI_IO_PB_05);
        }
        else if (PB5_UART4_RX == pin_func)
        {
            uni_uart4_rx_config(UNI_IO_PB_05);
        }
        else if (PB5_SPI_DO == pin_func)
        {
            uni_spi_do_config(UNI_IO_PB_05);
        }
    }
    else if (PB6 == pin)
    {
        if (PB6_UART1_TX == pin_func)
        {
            uni_uart1_tx_config(UNI_IO_PB_06);
        }
    }
    else if (PB7 == pin)
    {
        if (PB7_UART1_RX == pin_func)
        {
            uni_uart1_rx_config(UNI_IO_PB_07);
        }
    }
    else if (PB8 == pin)
    {
        if (PB8_IIS_CK == pin_func)
        {
            uni_i2s_ck_config(UNI_IO_PB_08);
        }
    }
    else if (PB9 == pin)
    {
        if (PB9_IIS_WS == pin_func)
        {
            uni_i2s_ws_config(UNI_IO_PB_09);
        }
    }
    else if (PB10 == pin)
    {
        if (PB10_IIS_DI == pin_func)
        {
            uni_i2s_di_config(UNI_IO_PB_10);
        }
    }
    else if (PB11 == pin)
    {
        if (PB11_IIS_DO == pin_func)
        {
            uni_i2s_do_config(UNI_IO_PB_11);
        }
    }
    else if (PB19 == pin)
    {
        if (PB19_UART0_TX == pin_func)
        {
            uni_uart0_tx_config(UNI_IO_PB_19);
        }
        else if (PB19_UART1_RTS == pin_func)
        {
            uni_uart1_rts_config(UNI_IO_PB_19);
        }
        else if (PB19_IIC_SDA == pin_func)
        {
            uni_i2c_sda_config(UNI_IO_PB_19);
        }
        else if (PB19_PWM == pin_func)
        {
            uni_pwm1_config(UNI_IO_PB_19);
        }
    }
    else if (PB20 == pin)
    {
        if (PB20_UART0_RX == pin_func)
        {
            uni_uart0_rx_config(UNI_IO_PB_20);
        }
        else if (PB20_UART1_CTS == pin_func)
        {
            uni_uart1_cts_config(UNI_IO_PB_20);
        }
        else if (PB20_IIC_SCL == pin_func)
        {
            uni_i2c_scl_config(UNI_IO_PB_20);
        }
        else if (PB20_PWM == pin_func)
        {
            uni_pwm2_config(UNI_IO_PB_20);
        }
    }

    return 0;
}

int32_t drv_pin_config_mode(port_name_e port, uint8_t offset, gpio_mode_e pin_mode)
{
    enum tls_io_name pinidx;

    if (port == PORTA)
        pinidx = UNI_IO_PA_00 + offset;
    else if (port == PORTB)
        pinidx = UNI_IO_PB_00 + offset;
    else
        return DRV_ERROR_UNSUPPORTED;

    if (GPIO_MODE_PULLNONE == pin_mode)
        tls_gpio_cfg_attr(pinidx, UNI_GPIO_ATTR_FLOATING);
    else if (GPIO_MODE_PULLUP == pin_mode)
        tls_gpio_cfg_attr(pinidx, UNI_GPIO_ATTR_PULLHIGH);
    else if (GPIO_MODE_PULLDOWN == pin_mode)
        tls_gpio_cfg_attr(pinidx, UNI_GPIO_ATTR_PULLLOW);
    else if (GPIO_MODE_OPEN_DRAIN == pin_mode)
        tls_io_cfg_set(pinidx, UNI_IO_OPTION5);
    else if (GPIO_MODE_PUSH_PULL == pin_mode)
        return DRV_ERROR_UNSUPPORTED;
    else
        return DRV_ERROR_UNSUPPORTED;

    return 0;
}
