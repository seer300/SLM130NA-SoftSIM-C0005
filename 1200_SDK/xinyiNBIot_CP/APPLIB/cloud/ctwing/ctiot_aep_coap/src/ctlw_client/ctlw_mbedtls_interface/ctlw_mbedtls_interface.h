#ifndef __CTLW_MBEDTLS_INTERFACE_H__
#define __CTLW_MBEDTLS_INTERFACE_H__


#include "ctlw_config.h"
#ifdef PLATFORM_XINYI
#if WITH_MBEDTLS_SUPPORT
#include "dtls_interface.h"
#include "xy_utils.h"
#endif
extern void mbedtls_timing_set_delay( void *data, uint32_t int_ms, uint32_t fin_ms );
extern int mbedtls_timing_get_delay( void *data );
#endif

#include "ctlw_lwm2m_sdk.h"
#if CTIOT_CHIPSUPPORT_DTLS == 1
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/timing.h"
#include "mbedtls/debug.h"
#define MAX_HANDSHAKE_RETRY_COUNT 3
#ifdef __cplusplus
extern "C" {
#endif

	int ctlw_dtls_ssl_create(ctiot_context_t* pContext);
	int ctlw_dtls_shakehand(ctiot_context_t* pContext);
	void ctlw_dtls_ssl_destroy(ctiot_context_t* pContext);
	int ctlw_dtls_update_socket_fd(ctiot_context_t* pContext);
	int ctlw_dtls_write(mbedtls_ssl_context* ssl, const unsigned char* buf, size_t len,uint8_t raiIndex);
	int ctlw_dtls_read(mbedtls_ssl_context* ssl, unsigned char* buf, size_t len, uint32_t timeout);
#ifdef __cplusplus
}
#endif
#endif
#endif//__CTLW_MBEDTLS_INTERFACE_H__

