/**
 * @file    uni_os_config.h
 *
 * @brief   WM OS select freertos or ucos
 *
 *
 */
 
#ifndef __WM_OS_CONFIG_H__
#define __WM_OS_CONFIG_H__
#define OS_CFG_ON  1
#define OS_CFG_OFF 0

#define TLS_OS_UCOS                         OS_CFG_OFF  /*UCOSII  need to modify uni_config.inc*/
#undef TLS_OS_FREERTOS
#define TLS_OS_FREERTOS                     OS_CFG_ON   /*FreeRTOS need to modify uni_config.inc*/
#endif
