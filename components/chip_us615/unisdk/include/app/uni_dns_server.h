/**
 * @file    uni_dns_server.h
 *
 * @brief   DNS SERVER
 *
 *
 */

#ifndef UNI_DNS_SERVER_H
#define UNI_DNS_SERVER_H

/**
 * @defgroup APP_APIs APP APIs
 * @brief APP APIs
 */

/**
 * @addtogroup APP_APIs
 * @{
 */

/**
 * @defgroup DNSS_APIs DNSS APIs
 * @brief DNS server APIs
 */

/**
 * @addtogroup DNSS_APIs
 * @{
 */

/**
 * @brief          This function is used to start DNS service
 *
 * @param[in]      *dnsname    Specify the server's dns name
 *
 * @retval         UNI_SUCCESS     success
 * @retval         UNI_FAILED	  failed
 *
 * @note           None
 */
s8 tls_dnss_start(u8 *dnsname);

/**
 * @brief          This function is used to stop DNS service
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
void tls_dnss_stop(void);

/**
 * @}
 */

/**
 * @}
 */

#endif /* UNI_DNS_SERVER_H */

