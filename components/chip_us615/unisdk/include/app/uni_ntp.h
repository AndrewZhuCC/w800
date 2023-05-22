/**
 * @file    uni_ntp.h
 *
 * @brief   ntp module
 *
 *
 */

#ifndef UNI_NTP_H
#define UNI_NTP_H

#include "uni_type_def.h"

/**
 * @defgroup APP_APIs APP APIs
 * @brief APP APIs
 */

/**
 * @addtogroup APP_APIs
 * @{
 */

/**
 * @defgroup NTP_APIs NTP APIs
 * @brief NTP APIs
 */

/**
 * @addtogroup NTP_APIs
 * @{
 */

/**
 * @brief          This function is used to get network time.
 *
 * @param          None
 *
 * @retval         time value
 *
 * @note           None
 */
u32 tls_ntp_client(void);

/**
 * @brief          This function is used to set ntp servers.
 *
 * @param[in]      *ipaddr      xxx.xxx.xxx.xxx
 * @param[in]      server_no    max num is three
 *
 * @retval         UNI_SUCCESS     success
 * @retval         UNI_FAILED failed
 *
 * @note           None
 */
int tls_ntp_set_server(char *ipaddr, int server_no);

/**
 * @brief          This function is used to query params of the ntp servers 
 *
 *
 * @retval         UNI_SUCCESS     success
 * @retval         UNI_FAILED failed
 *
 * @note           None
 */
int tls_ntp_query_sntpcfg(void);

/**
 * @}
 */

/**
 * @}
 */

#endif /* UNI_NTP_H */

