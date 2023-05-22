/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef __PIN_H__
#define __PIN_H__

#include <stdint.h>
#include "pinmux.h"

#define CONSOLE_UART_IDX  0       //not used, pls refer to "CONSOLE_IDX"

#define CONSOLE_TXD                 PB6
#define CONSOLE_RXD                 PB7
#define CONSOLE_TXD_FUNC            PB6_UART1_TX
#define CONSOLE_RXD_FUNC            PB7_UART1_RX


#if 1
#define I2C_SCL                     PA1
#define I2C_SDA                     PA4
#define I2C_SCL_FUNC                PA1_IIC_SCL
#define I2C_SDA_FUNC                PA4_IIC_SDA

#else

#define I2C_SCL                     PB20
#define I2C_SDA                     PB19
#define I2C_SCL_FUNC                PB20_IIC_SCL
#define I2C_SDA_FUNC                PB19_IIC_SDA

#endif

#define I2S_MCLK						PA7
#define I2S_BCLK						PB8
#define I2S_LRCK						PB9
#define I2S_DATA_IN						PB10
#define I2S_DATA_OUT					PB11


#define I2S_MCLK_FUNC					PA7_IIS_EXTCLK
#define I2S_BCLK_FUNC					PB8_IIS_CK
#define I2S_LRCK_FUNC					PB9_IIS_WS
#define I2S_DATA_IN_FUNC				PB10_IIS_DI
#define I2S_DATA_OUT_FUNC				PB11_IIS_DO

/* spi flash pins */
#define SPI_FLASH_DI                PB0
#define SPI_FLASH_DO                PB5
#define SPI_FLASH_CK                PB1
#define SPI_FLASH_CS                PB4

#define USER_PA_PIN				    PB3	// PB3



#endif
