#include "dtls_interface.h"

#define MBEDTLS_DEBUG

#ifdef MBEDTLS_DEBUG
#define MBEDTLS_LOG(fmt, ...) \
    do \
    { \
        user_printf("[MBEDTLS][%s:%d] "fmt "\r\n", \
        __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while (0)
#else
#define MBEDTLS_LOG(fmt, ...) ((void)0)
#endif

#define DEBUG_LEVEL 0

static void dtls_debug(void *ctx, int level, const char *file, int line, const char *str)
{
	xy_printf(0, XYAPP, WARN_LOG, "[MBEDTLS][%d:%s:%d] %s\r\n", \
		level, __FILE__, __LINE__, str);
}

mbedtls_ssl_context *dtls_ssl_new(dtls_establish_info_s *info, int endpoint)
{
    int ret;
	int authmode = MBEDTLS_SSL_VERIFY_NONE;
    mbedtls_ssl_context *ssl = NULL;
    mbedtls_ssl_config *conf = NULL;
    mbedtls_entropy_context *entropy = NULL;
    mbedtls_ctr_drbg_context *ctr_drbg = NULL;
    mbedtls_timing_delay_context *timer = NULL;
	
#if defined(MBEDTLS_X509_CRT_PARSE_C)
    mbedtls_x509_crt *cacert = NULL;
	mbedtls_x509_crt *client_cert = NULL;
#endif

#if defined(MBEDTLS_PK_PARSE_C)
    mbedtls_pk_context *client_pk = NULL;
#endif

    const char *pers = "dtls_client";

    ssl       = mbedtls_calloc(1, sizeof(mbedtls_ssl_context));
    conf      = mbedtls_calloc(1, sizeof(mbedtls_ssl_config));
    entropy   = mbedtls_calloc(1, sizeof(mbedtls_entropy_context));
    ctr_drbg  = mbedtls_calloc(1, sizeof(mbedtls_ctr_drbg_context));

    if (NULL == info || NULL == ssl
        || NULL == conf || NULL == entropy
        || NULL == ctr_drbg
        )
    {
        goto exit_fail;
    }

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	if (info->psk_or_cert == VERIFY_WITH_CERT)
	{
		if (info->v.c.ca_cert != NULL)
		{
			cacert= mbedtls_calloc(1, sizeof(mbedtls_x509_crt));
			if (NULL == cacert)
			{
				goto exit_fail;	
			}
        	authmode = MBEDTLS_SSL_VERIFY_REQUIRED;
		}
		
#if defined(MBEDTLS_PK_PARSE_C)
		if (info->v.c.client_cert != NULL && info->v.c.client_pk != NULL)
    	{
    		client_cert = mbedtls_calloc(1, sizeof(mbedtls_x509_crt));
    		if (client_cert == NULL)
    		{
				MBEDTLS_LOG("client_cert calloc failed in xy_tls_ssl_new");
				goto exit_fail;
    		}
    		client_pk = mbedtls_calloc(1, sizeof(mbedtls_pk_context));
    		if (client_pk == NULL)
    		{
				MBEDTLS_LOG("client_pk calloc failed in xy_tls_ssl_new");
				goto exit_fail;
    		}
    	}
#endif
	}
#endif
	
    if (info->udp_or_tcp == MBEDTLS_NET_PROTO_UDP)
    {
        timer = mbedtls_calloc(1, sizeof(mbedtls_timing_delay_context));
        if (NULL == timer) 
			goto exit_fail;
    }

    mbedtls_ssl_init(ssl);
    mbedtls_ssl_config_init(conf);
    mbedtls_ctr_drbg_init(ctr_drbg);
    mbedtls_entropy_init(entropy);

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    if (info->psk_or_cert == VERIFY_WITH_CERT)
    {
		if (cacert != NULL)
			mbedtls_x509_crt_init(cacert);
		if (client_cert != NULL)
    		mbedtls_x509_crt_init(client_cert);
#if defined(MBEDTLS_PK_PARSE_C)
    	if (client_pk != NULL)
    		mbedtls_pk_init(client_pk);
#endif
    }
#endif

    if ((ret = mbedtls_ctr_drbg_seed(ctr_drbg, mbedtls_entropy_func, entropy,
                                     (const unsigned char *) pers,
                                     strlen(pers))) != 0)
    {
        MBEDTLS_LOG("mbedtls_ctr_drbg_seed failed: -0x%x", -ret);
        goto exit_fail;
    }

	/*
	 * Set the maximum fragment length
	 */
	if ((ret = mbedtls_ssl_conf_max_frag_len(conf, MBEDTLS_SSL_MAX_FRAG_LEN_4096)) < 0) {
		MBEDTLS_LOG("mbedtls_ssl_conf_max_frag_len failed: -0x%x", -ret);
        goto exit_fail;
	}
	
    MBEDTLS_LOG("setting up the SSL structure");

    if (info->udp_or_tcp == MBEDTLS_NET_PROTO_UDP)
    {
        ret = mbedtls_ssl_config_defaults(conf,
                                          endpoint,
                                          MBEDTLS_SSL_TRANSPORT_DATAGRAM,
                                          MBEDTLS_SSL_PRESET_DEFAULT);
    }
    else
    {
        ret = mbedtls_ssl_config_defaults(conf,
                                          endpoint,
                                          MBEDTLS_SSL_TRANSPORT_STREAM,
                                          MBEDTLS_SSL_PRESET_DEFAULT);
    }

    if (ret != 0)
    {
        MBEDTLS_LOG("mbedtls_ssl_config_defaults failed: -0x%x", -ret);
        goto exit_fail;
    }

    mbedtls_ssl_conf_authmode(conf, authmode);

	MBEDTLS_LOG("ssl config authmode:%d", authmode);
	
    mbedtls_ssl_conf_rng(conf, mbedtls_ctr_drbg_random, ctr_drbg);
	
#if defined(MBEDTLS_DEBUG_C)
	mbedtls_debug_set_threshold(DEBUG_LEVEL);
	mbedtls_ssl_conf_dbg(conf, dtls_debug, NULL);
#endif

    if (info->udp_or_tcp == MBEDTLS_NET_PROTO_TCP)
    {
        mbedtls_ssl_conf_read_timeout(conf, TLS_SHAKEHAND_TIMEOUT);
    }

#if defined(MBEDTLS_KEY_EXCHANGE_SOME_PSK_ENABLED)
    if (info->psk_or_cert == VERIFY_WITH_PSK)
    {
        if ((ret = mbedtls_ssl_conf_psk(conf,
                                        info->v.p.psk,
                                        info->v.p.psk_len,
                                        info->v.p.psk_identity,
                                        strlen((const char *)info->v.p.psk_identity))) != 0)
        {
            MBEDTLS_LOG("mbedtls_ssl_conf_psk failed: -0x%x", -ret);
            goto exit_fail;
        }
    }
#endif

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    if (info->psk_or_cert == VERIFY_WITH_CERT)
    {
    	if (cacert != NULL)
    	{
			ret = mbedtls_x509_crt_parse(cacert, info->v.c.ca_cert, info->v.c.ca_cert_len);
	        if(ret < 0)
	        {
	            MBEDTLS_LOG("mbedtls_x509_crt_parse failed -0x%x", -ret);
	            goto exit_fail;
	        }
	        mbedtls_ssl_conf_ca_chain(conf, cacert, NULL);
		}

#if defined(MBEDTLS_PK_PARSE_C)
		if (client_cert != NULL && client_pk != NULL)
		{
			ret = mbedtls_x509_crt_parse(client_cert, info->v.c.client_cert, info->v.c.client_cert_len);
			if(ret < 0)
			{
				MBEDTLS_LOG("mbedtls_x509_crt_parse failed -0x%x", -ret);
				{
					goto exit_fail;
				}
			}

			ret = mbedtls_pk_parse_key(client_pk, info->v.c.client_pk, info->v.c.client_pk_len, "", 0, NULL, NULL);
			if(ret < 0)
			{
				MBEDTLS_LOG("mbedtls_pk_parse_key failed -0x%x", -ret);
				{
					goto exit_fail;
				}
			}

			ret = mbedtls_ssl_conf_own_cert(conf, client_cert, client_pk);
			if(ret < 0)
			{
				MBEDTLS_LOG("mbedtls_ssl_conf_own_cert failed -0x%x", -ret);
				{
					goto exit_fail;
				}
			}
		}
#endif
    }
#endif

#if defined(MBEDTLS_SSL_SRV_C)
    if (info->udp_or_tcp == MBEDTLS_NET_PROTO_UDP)
    {
        mbedtls_ssl_conf_dtls_cookies(conf, NULL, NULL,NULL);
    }
#endif

    if ((ret = mbedtls_ssl_setup(ssl, conf)) != 0)
    {
        MBEDTLS_LOG("mbedtls_ssl_setup failed: -0x%x", -ret);
        goto exit_fail;
    }

    if (info->udp_or_tcp == MBEDTLS_NET_PROTO_UDP)
    {
        mbedtls_ssl_set_timer_cb(ssl, timer, mbedtls_timing_set_delay,
                                 mbedtls_timing_get_delay);
    }

    MBEDTLS_LOG("set SSL structure succeed");

    return ssl;

exit_fail:

    if (conf)
    {
        mbedtls_ssl_config_free(conf);
        mbedtls_free(conf);
    }

    if (ctr_drbg)
    {
        mbedtls_ctr_drbg_free(ctr_drbg);
        mbedtls_free(ctr_drbg);
    }

    if (entropy)
    {
        mbedtls_entropy_free(entropy);
        mbedtls_free(entropy);
    }

    if (timer)
    {
        mbedtls_free(timer);
    }

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    if (cacert)
    {
        mbedtls_x509_crt_free(cacert);
        mbedtls_free(cacert);
    }
	
	if (client_cert)
	{
		mbedtls_x509_crt_free(client_cert);
		mbedtls_free(client_cert);
	}
#endif

#if defined(MBEDTLS_PK_PARSE_C)
    if (client_pk)
	{
    	mbedtls_pk_free(client_pk);
		mbedtls_free(client_pk);
	}
#endif

    if (ssl)
    {
        mbedtls_ssl_free(ssl);
        mbedtls_free(ssl);
    }
    return NULL;
}

int dtls_shakehand(mbedtls_ssl_context *ssl, const dtls_shakehand_info_s *info)
{
    int ret = MBEDTLS_ERR_NET_CONNECT_FAILED;
    uint32_t change_value = 0;
    mbedtls_net_context *server_fd = NULL;
    uint32_t max_value;
#if defined(MBEDTLS_X509_CRT_PARSE_C)
    unsigned int flags;
#endif

	server_fd = mbedtls_calloc(1, sizeof(mbedtls_net_context));
	
    MBEDTLS_LOG("connecting to server");

	if (MBEDTLS_SSL_IS_CLIENT == info->client_or_server)
    {
        ret = mbedtls_net_connect(server_fd, info->u.c.host, info->u.c.port, info->udp_or_tcp);
    }
    else
    {
        ret = mbedtls_net_bind(server_fd, NULL, info->u.s.local_port, info->udp_or_tcp);
    }

    if (ret)
    {
        MBEDTLS_LOG("connect failed! mode %d", info->client_or_server);
        ret = MBEDTLS_ERR_NET_CONNECT_FAILED;
        goto exit_fail;
    }
	
#if defined(MBEDTLS_SSL_PROTO_DTLS)
    mbedtls_ssl_conf_handshake_timeout(ssl->conf, 4000, 32000);
#endif

    mbedtls_ssl_set_bio(ssl, server_fd,
    		mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);

    MBEDTLS_LOG("performing the SSL/TLS handshake");

    max_value = ((MBEDTLS_SSL_IS_SERVER == info->client_or_server || info->udp_or_tcp == MBEDTLS_NET_PROTO_UDP) ?
                (dtls_gettime() + info->timeout) :  50);

    do
    {
        ret = mbedtls_ssl_handshake(ssl);
        MBEDTLS_LOG("mbedtls_ssl_handshake %d %d", change_value, max_value);
        if (MBEDTLS_SSL_IS_CLIENT == info->client_or_server && info->udp_or_tcp == MBEDTLS_NET_PROTO_TCP)
        {
            change_value++;
        }
        else
        {
            change_value = dtls_gettime();
        }

        if (info->step_notify)
        {
            info->step_notify(info->param);
        }
    }
    while ((ret == MBEDTLS_ERR_SSL_WANT_READ || 
			ret == MBEDTLS_ERR_SSL_WANT_WRITE || 
			(ret == MBEDTLS_ERR_SSL_TIMEOUT && 
			info->udp_or_tcp == MBEDTLS_NET_PROTO_TCP)) &&
            (change_value < max_value));

    if (info->finish_notify)
    {
        info->finish_notify(info->param);
    }

    if (ret != 0)
    {
        MBEDTLS_LOG("mbedtls_ssl_handshake failed: -0x%x", -ret);
        goto exit_fail;
    }

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    if (info->psk_or_cert == VERIFY_WITH_CERT)
    {
        if((flags = mbedtls_ssl_get_verify_result(ssl)) != 0)
        {
            char vrfy_buf[512];
            mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);
            MBEDTLS_LOG("cert verify failed: %s", vrfy_buf);
            goto exit_fail;
        }
        else
            MBEDTLS_LOG("cert verify succeed");
    }
#endif

    MBEDTLS_LOG("handshake succeed");

    return 0;

exit_fail:

    if (server_fd)
    {
        mbedtls_net_free(server_fd);
        mbedtls_free(server_fd);
        ssl->p_bio = NULL;
    }

    return ret;

}
void dtls_ssl_destroy(mbedtls_ssl_context *ssl)
{
    mbedtls_ssl_config           *conf = NULL;
    mbedtls_ctr_drbg_context     *ctr_drbg = NULL;
    mbedtls_entropy_context      *entropy = NULL;
    mbedtls_net_context          *server_fd = NULL;
    mbedtls_timing_delay_context *timer = NULL;
#if defined(MBEDTLS_X509_CRT_PARSE_C)
    mbedtls_x509_crt             *cacert = NULL;
	mbedtls_x509_crt             *client_cert = NULL;
#endif
#if defined(MBEDTLS_PK_PARSE_C)
    mbedtls_pk_context           *client_pk = NULL;
#endif

    if (ssl == NULL)
    {
        return;
    }

    conf       = ssl->conf;
    server_fd  = (mbedtls_net_context *)ssl->p_bio;
    timer      = (mbedtls_timing_delay_context *)ssl->p_timer;
	
#if defined(MBEDTLS_X509_CRT_PARSE_C)
    cacert     = (mbedtls_x509_crt *)conf->ca_chain;
#endif

#if defined(MBEDTLS_X509_CRT_PARSE_C) && defined(MBEDTLS_PK_PARSE_C)
	if(conf->key_cert != NULL)
    {
        client_cert = conf->key_cert->cert;
        client_pk  =  conf->key_cert->key;
    }
#endif

    if (conf)
    {
        ctr_drbg   = conf->p_rng;

        if (ctr_drbg)
        {
            entropy =  ctr_drbg->p_entropy;
        }
    }

    if (server_fd)
    {
        mbedtls_net_free(server_fd);
        mbedtls_free(server_fd);
    }

    if (conf)
    {
        mbedtls_ssl_config_free(conf);
        mbedtls_free(conf);
        ssl->conf = NULL;
    }

    if (ctr_drbg)
    {
        mbedtls_ctr_drbg_free(ctr_drbg);
        mbedtls_free(ctr_drbg);
    }

    if (entropy)
    {
        mbedtls_entropy_free(entropy);
        mbedtls_free(entropy);
    }

    if (timer)
    {
        mbedtls_free(timer);
    }

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    if (cacert)
    {
        mbedtls_x509_crt_free(cacert);
        mbedtls_free(cacert);
    }

	if (client_cert)
	{
		mbedtls_x509_crt_free(client_cert);
		mbedtls_free(client_cert);
	}
#endif

#if defined(MBEDTLS_PK_PARSE_C)

    if (client_pk)
	{
    	mbedtls_pk_free(client_pk);
		mbedtls_free(client_pk);
	}
#endif

    mbedtls_ssl_free(ssl);
    mbedtls_free(ssl);
}

int dtls_write(mbedtls_ssl_context *ssl, const unsigned char *buf, size_t len)
{
    int ret = mbedtls_ssl_write(ssl, (unsigned char *) buf, len);

    if (ret == MBEDTLS_ERR_SSL_WANT_WRITE)
    {
        return 0;
    }
    else if (ret < 0)
    {
        return -1;
    }

    return ret;
}

int dtls_read(mbedtls_ssl_context *ssl, unsigned char *buf, size_t len, uint32_t timeout)
{
    int ret;

    mbedtls_ssl_conf_read_timeout(ssl->conf, timeout);

    ret = mbedtls_ssl_read(ssl, buf, len);

    if (ret == MBEDTLS_ERR_SSL_WANT_READ)
    {
        return 0;
    }
    else if (ret == MBEDTLS_ERR_SSL_TIMEOUT)
    {
        return -2;
    }
    else if (ret <= 0)
    {
        return -1;
    }

    return ret;
}

int dtls_accept( mbedtls_net_context *bind_ctx,
                            mbedtls_net_context *client_ctx,
                            void *client_ip, size_t buf_size, size_t *ip_len )
{
    return mbedtls_net_accept(bind_ctx, client_ctx, client_ip, buf_size, ip_len);
}

