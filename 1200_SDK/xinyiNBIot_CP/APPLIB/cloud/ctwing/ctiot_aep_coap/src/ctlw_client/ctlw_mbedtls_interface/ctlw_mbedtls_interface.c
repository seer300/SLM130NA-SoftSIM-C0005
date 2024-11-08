
/**************************************************************

***************************************************************/
#include <stdint.h>
#include <string.h>
#include "ctlw_mbedtls_interface.h"

#if CTIOT_CHIPSUPPORT_DTLS == 1
int ctlw_dtls_write(mbedtls_ssl_context* ssl, const unsigned char* buf, size_t len,uint8_t raiIndex)
{
	/*raiIndex 需要芯片适配是否支持rai*/
    int ret = mbedtls_ssl_write(ssl, (unsigned char *) buf, len);

    if (ret == MBEDTLS_ERR_SSL_WANT_WRITE)
    {
        return 0;
    }
    else
    {
        return ret;
    }
}
int ctlw_dtls_read(mbedtls_ssl_context* ssl, unsigned char* buf, size_t len, uint32_t timeout)
{
    int ret;
    mbedtls_ssl_conf_read_timeout((mbedtls_ssl_config *)ssl->conf, timeout);
    ret = mbedtls_ssl_read(ssl, buf, len);

    if (ret == MBEDTLS_ERR_SSL_WANT_READ)
    {
        return 0;
    }
    else
    {
        return ret;
    }
}
int ctlw_dtls_ssl_create(ctiot_context_t* pContext)
{
	int ret;
	mbedtls_ssl_config *conf = NULL;
    mbedtls_entropy_context *entropy = NULL;
    mbedtls_ctr_drbg_context *ctr_drbg = NULL;
    mbedtls_timing_delay_context *timer = NULL;
    const char *pers = "ssl_client";
	if(pContext== NULL)
	{
		return CTIOT_OTHER_ERROR;
	}
   
	//create时，用户需保证pContext上的ssl指向空地址，或者ssl指向的地址已经被正确释放
	pContext->ssl = ctlw_lwm2m_malloc(sizeof(mbedtls_ssl_context));
    conf      = ctlw_lwm2m_malloc(sizeof(mbedtls_ssl_config));
    entropy   = ctlw_lwm2m_malloc(sizeof(mbedtls_entropy_context));
    ctr_drbg  = ctlw_lwm2m_malloc(sizeof(mbedtls_ctr_drbg_context));

    if (NULL == pContext->ssl || NULL == conf || NULL == entropy || NULL == ctr_drbg)
    {
        goto exit_fail;
    }

    timer = ctlw_lwm2m_malloc(sizeof(mbedtls_timing_delay_context));
    if (NULL == timer)
    {
        goto exit_fail;
    }

    mbedtls_ssl_init(pContext->ssl);
    mbedtls_ssl_config_init(conf);
    mbedtls_ctr_drbg_init(ctr_drbg);
    mbedtls_entropy_init(entropy);

    if ((ret = mbedtls_ctr_drbg_seed(ctr_drbg, mbedtls_entropy_func, entropy,(const unsigned char *) pers,strlen(pers))) != 0)
    {
        ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_DTLS_CLASS,"mbedtls_ctr_drbg_seed failed: -0x%x\n", -ret);
        goto exit_fail;
    }

    ret = mbedtls_ssl_config_defaults(conf,MBEDTLS_SSL_IS_CLIENT,MBEDTLS_SSL_TRANSPORT_DATAGRAM,MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0)
    {
        ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_DTLS_CLASS,"mbedtls_ssl_config_defaults failed: -0x%x\n", -ret);
        goto exit_fail;
    }
    mbedtls_ssl_conf_authmode(conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_rng(conf, mbedtls_ctr_drbg_random, ctr_drbg);
    //mbedtls_ssl_conf_read_timeout(conf, 1);

    if ((ret = mbedtls_ssl_conf_psk(conf,(unsigned char *)pContext->psk,pContext->pskLen,(unsigned char *)pContext->pskID,strlen(pContext->pskID))) != 0)
    {
        ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_DTLS_CLASS,"mbedtls_ssl_conf_psk failed: -0x%x\n", -ret);
        goto exit_fail;
    }

    if ((ret = mbedtls_ssl_setup(pContext->ssl, conf)) != 0)
    {
        ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_DTLS_CLASS,"mbedtls_ssl_setup failed: -0x%x", -ret);
        goto exit_fail;
    }

    mbedtls_ssl_set_timer_cb(pContext->ssl, timer, mbedtls_timing_set_delay,mbedtls_timing_get_delay);

    ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_DTLS_CLASS,"set SSL structure succeed\n");
    return CTIOT_NB_SUCCESS;
exit_fail:
    if (conf)
    {
        mbedtls_ssl_config_free(conf);
        ctlw_lwm2m_free(conf);
    }

    if (ctr_drbg)
    {
        mbedtls_ctr_drbg_free(ctr_drbg);
        ctlw_lwm2m_free(ctr_drbg);
    }

    if (entropy)
    {
        mbedtls_entropy_free(entropy);
        ctlw_lwm2m_free(entropy);
    }
    if (timer)
    {
        ctlw_lwm2m_free(timer);
    }
    if (pContext->ssl)
    {
        mbedtls_ssl_free(pContext->ssl);
        ctlw_lwm2m_free(pContext->ssl);
		pContext->ssl = NULL;
    }
	return ret;
}

int ctlw_dtls_shakehand(ctiot_context_t* pContext)
{

	ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_DTLS_CLASS,"ctlw_dtls_shakehand >>\r\n");
	int ret = MBEDTLS_ERR_NET_CONNECT_FAILED;
	uint32_t retryCount = 0;
	if(pContext->ssl == NULL)
	{
		goto exit;
	}
#ifdef PLATFORM_XINYI
    #if defined(MBEDTLS_SSL_PROTO_DTLS)
        mbedtls_ssl_conf_handshake_timeout(pContext->ssl->conf, 4000, 32000);
    #endif
#endif
	do
    {
        ret = mbedtls_ssl_handshake(pContext->ssl);
        retryCount++;
    }
    while ((ret == MBEDTLS_ERR_SSL_WANT_READ ||
            ret == MBEDTLS_ERR_SSL_WANT_WRITE ||
            ret == MBEDTLS_ERR_SSL_TIMEOUT) &&
            (retryCount < MAX_HANDSHAKE_RETRY_COUNT));

	ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_DTLS_CLASS,"ctlw_dtls_shakehand <<\r\n");
exit:
	return ret;
}
void ctlw_dtls_ssl_destroy(ctiot_context_t* pContext)
{
	mbedtls_ssl_config           *conf = NULL;
    mbedtls_ctr_drbg_context     *ctr_drbg = NULL;
    mbedtls_entropy_context      *entropy = NULL;
    mbedtls_net_context          *server_fd = NULL;
    mbedtls_timing_delay_context *timer = NULL;
	if(pContext->ssl == NULL)
	{
		return;
	}
	conf       =(mbedtls_ssl_config *) pContext->ssl->conf;
    server_fd  = (mbedtls_net_context *)pContext->ssl->p_bio;
    timer      = (mbedtls_timing_delay_context *)pContext->ssl->p_timer;
    if (conf)
    {
        ctr_drbg   = conf->p_rng;

        if (ctr_drbg)
        {
            entropy =  ctr_drbg->p_entropy;
        }
    }

    if (server_fd->fd >= 0)
    {
        mbedtls_net_free(server_fd);
		pContext->clientInfo.sock = -1;
    }

    if (conf)
    {
        mbedtls_ssl_config_free(conf);
        ctlw_lwm2m_free(conf);
        //pContext->ssl->conf = NULL; //  need by mbedtls_debug_print_msg(), see mbedtls_ssl_free(ssl)
    }

    if (ctr_drbg)
    {
        mbedtls_ctr_drbg_free(ctr_drbg);
        ctlw_lwm2m_free(ctr_drbg);
    }

    if (entropy)
    {
		mbedtls_entropy_free(entropy);
        ctlw_lwm2m_free(entropy);
    }

    if (timer)
    {
        ctlw_lwm2m_free(timer);
    }
    mbedtls_ssl_free(pContext->ssl);
    ctlw_lwm2m_free(pContext->ssl);
	pContext->ssl = NULL;
}
int ctlw_dtls_update_socket_fd(ctiot_context_t* pContext)//2.1版本已保证调用时ssl上下文已初始化,所以调用时都不判断返回值
{
	ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_DTLS_CLASS,"ctlw_dtls_update_socket_fd >>\r\n");
	if(pContext->ssl == NULL)
	{
		return -1;//fail
	}
	mbedtls_ssl_set_bio(pContext->ssl, &pContext->clientInfo.sock , mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);
	ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_DTLS_CLASS,"ctlw_dtls_update_socket_fd <<\r\n");
	return 0;//success
}
#endif
