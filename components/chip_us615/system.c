/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */


/******************************************************************************
 * @file     system.c
 * @brief    CSI Device System Source File for YUNVOICE
 * @version  V1.0
 * @date     06. Mar 2019
 * @vendor   csky
 * @chip     pangu
 ******************************************************************************/

#include <stdint.h>
#include <io.h>
#include <soc.h>
#include <csi_core.h>
#include <drv/irq.h>

#include <sys_freq.h>

#include "uni_type_def.h"
#include "uni_cpu.h"
#include "uni_pmu.h"
#include "uni_include.h"

extern uint32_t __Vectors[];

extern void irq_vectors_init(void);
extern ATTRIBUTE_ISR void CORET_IRQHandler(void);

#ifndef CONFIG_SYSTICK_HZ
#define CONFIG_SYSTICK_HZ 500
#endif

const unsigned int HZ = CONFIG_SYSTICK_HZ;


/**
  * @brief  initialize the system
  *         Initialize the psr and vbr.
  * @param  None
  * @return None
  */
void SystemInit(void)
{
    __set_VBR((uint32_t) & (__Vectors));

    __set_CHR(__get_CHR() | CHR_IAE_Msk);
	__set_PSR(__get_PSR() | PSR_MM_Msk);
    /* Clear active and pending IRQ */
    VIC->IABR[0] = 0x0;
    VIC->ICPR[0] = 0xFFFFFFFF;
    VIC->ICER[0] = 0xFFFFFFFF;
	
#ifndef CONFIG_DISABLE_IRQ
#ifdef CONFIG_KERNEL_NONE
    __enable_excp_irq();
#endif

#ifndef CONFIG_SUPPORT_TSPEND
    irq_vectors_init();
#endif

	drv_irq_register(SYS_TICK_IRQn, CORET_IRQHandler);
    drv_irq_enable(SYS_TICK_IRQn);
#endif
}

