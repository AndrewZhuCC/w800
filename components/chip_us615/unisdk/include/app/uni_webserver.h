/**
 * @file    uni_webserver.h
 *
 * @brief   WEB SERVER
 *
 *
 */

#ifndef __WEBSERVER_H__
#define __WEBSERVER_H__

/**
 * @defgroup APP_APIs APP APIs
 * @brief APP APIs
 */

/**
 * @addtogroup APP_APIs
 * @{
 */

/**
 * @defgroup WEB_APIs WEB APIs
 * @brief WEB server APIs
 */

/**
 * @addtogroup WEB_APIs
 * @{
 */

/**
 * @brief          This function is used to start WEB SERVER service
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
void tls_webserver_init(void);

/**
 * @brief          This function is used to deinit WEB SERVER service
 *
 * @param[in]      None
 *
 * @return         None
 *
 * @note           None
 */
void tls_webserver_deinit(void);

/**
 * @}
 */

/**
 * @}
 */

#endif /*__WEBSERVER_H__*/

