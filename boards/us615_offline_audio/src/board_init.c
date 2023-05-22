/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */


#include <stdlib.h>
#include <stdio.h>
#include <core_804.h>
#include <aos/aos.h>
#include <pin.h>
#include <drv/irq.h>
#include "uni_type_def.h"
#include "uni_cpu.h"
#include "uni_pmu.h"
#include "uni_io.h"
#include "uni_gpio_afsel.h"
#if TLS_CONFIG_HARD_CRYPTO
#include "uni_crypto_hard.h"
#endif


extern void uart0Init (int bandrate);

extern ATTRIBUTE_ISR void GPSEC_IRQHandler(void);
extern ATTRIBUTE_ISR void RSA_IRQHandler(void);


static void board_pinmux_config(void)
{
    drv_pinmux_config(CONSOLE_TXD, CONSOLE_TXD_FUNC);
    drv_pinmux_config(CONSOLE_RXD, CONSOLE_RXD_FUNC);

	drv_pinmux_config(I2C_SCL, I2C_SCL_FUNC);
    drv_pinmux_config(I2C_SDA, I2C_SDA_FUNC);

	drv_pinmux_config(I2S_MCLK, I2S_MCLK_FUNC);
    drv_pinmux_config(I2S_BCLK, I2S_BCLK_FUNC);
	drv_pinmux_config(I2S_LRCK, I2S_LRCK_FUNC);
    drv_pinmux_config(I2S_DATA_IN, I2S_DATA_IN_FUNC);
	drv_pinmux_config(I2S_DATA_OUT, I2S_DATA_OUT_FUNC);

//	drv_pinmux_config(SPI_FLASH_DI, PB0_SPI_DI);
//	drv_pinmux_config(SPI_FLASH_CK, PB1_SPI_CK);
//	drv_pinmux_config(SPI_FLASH_CS, PB4_SPI_CS);
//	drv_pinmux_config(SPI_FLASH_DO, PB5_SPI_DO);
}

static void uni_gpio_config()
{
	/* must call first */
	uni_gpio_af_disable();	

	uni_uart0_tx_config(UNI_IO_PB_19);
	uni_uart0_rx_config(UNI_IO_PB_20);

	uni_uart1_rx_config(UNI_IO_PB_07);
	uni_uart1_tx_config(UNI_IO_PB_06);
}

void board_init(void)
{

    tls_pmu_clk_select(1);

    u32 value = 0;

    /*Switch to DBG*/
	value = tls_reg_read32(HR_PMU_BK_REG);
	value &=~(BIT(19));
	tls_reg_write32(HR_PMU_BK_REG, value);
	value = tls_reg_read32(HR_PMU_PS_CR);
	value &= ~(BIT(5));
	tls_reg_write32(HR_PMU_PS_CR, value);	
	
	/*Close those not used clk*/
	tls_reg_write32(HR_CLK_BASE_ADDR,tls_reg_read32(HR_CLK_BASE_ADDR)&
		(~(BIT(6)|BIT(14)|BIT(18)|BIT(19)|BIT(21))));	
	
	tls_sys_clk_set(CPU_CLK_240M);
	
    /* must call first to configure gpio Alternate functions according the hardware design */
	uni_gpio_config();

#if TLS_CONFIG_CRYSTAL_24M
	tls_wl_hw_using_24m_crystal();
#endif	
#if USE_UART0_PRINT
	uart0Init(115200);
#endif
    board_pinmux_config();
    //flash_csky_register(0);
#if TLS_CONFIG_HARD_CRYPTO
	drv_irq_register(RSA_IRQn, RSA_IRQHandler);
    drv_irq_register(CRYPTION_IRQn, GPSEC_IRQHandler);
    tls_crypto_init();
#endif

#if 0
    unsigned int v0 = *(volatile unsigned int*)0xE000EC10;
    *(volatile unsigned int*)0xE000EC10 = 0xFF;
    unsigned int v1 = *(volatile unsigned int*)0xE000EC10;
    printf("v0=%x, v1=%x, %s\n", v0, v1, (v0 != v1) ? "TSPEND" : "NOTSPEND");
#endif
}

