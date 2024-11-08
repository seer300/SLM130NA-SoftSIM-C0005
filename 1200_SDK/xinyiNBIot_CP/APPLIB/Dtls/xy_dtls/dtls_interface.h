/**
 * @file dtls_interface.h
 * @brief 芯翼提供的 ssl/tls 操作API,方便客户使用 mbedtls 功能 
 */
#ifndef DTLS_INTERFACE_H
#define DTLS_INTERFACE_H

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/mbedtls_config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include "mbedtls/platform.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/timing.h"
#include "ssl_misc.h"
#include "mbedtls_init.h"
#include "xy_utils.h"

#ifndef TLS_SHAKEHAND_TIMEOUT
#define TLS_SHAKEHAND_TIMEOUT 1000
#endif

typedef enum
{
    VERIFY_WITH_PSK = 0,
    VERIFY_WITH_CERT,
}verify_type_e;

typedef struct
{
    union
    {
        struct
        {
            const char *host;
            const char *port;
        }c;
        struct
        {
            const char *local_port;
        }s;
    }u;
    uint32_t timeout;
    int client_or_server;
    int udp_or_tcp;
    verify_type_e psk_or_cert;
    void (*step_notify)(void *param);
    void (*finish_notify)(void *param);
    void *param;
}dtls_shakehand_info_s;

typedef struct
{
    union
    {
        struct
        {
            const unsigned char *psk;
            uint32_t psk_len;
            const unsigned char *psk_identity;
        }p;
        struct
        {
            const unsigned char *ca_cert;
            uint32_t ca_cert_len;
			const char *client_cert;
			uint32_t client_cert_len;
			const char *client_pk;
			uint32_t client_pk_len;
        }c;
    }v;
    verify_type_e psk_or_cert;
    int udp_or_tcp;
}dtls_establish_info_s;

/**
 * @brief   创建 SSL 上下文
 * @param   info [IN] 创建 ssl 上下文所需信息  @see @ref dtls_establish_info_s
 * @param   endpoint [IN] MBEDTLS_SSL_IS_CLIENT or MBEDTLS_SSL_IS_SERVER
 * @return  成功返回 SSL 上下文，失败返回 NULL
 */
mbedtls_ssl_context *dtls_ssl_new(dtls_establish_info_s *info, int endpoint);

/**
 * @brief   进行 SSL 握手流程
 * @param   ssl [IN] ssl 上下文
 * @param   info [IN] 握手所需信息 @see @ref dtls_shakehand_info_s
 * @return  成功返回0，否则返回失败原因
 */
int dtls_shakehand(mbedtls_ssl_context *ssl, const dtls_shakehand_info_s *info);

/**
 * @brief   销毁 SSL 上下文
 * @param   ssl [IN] ssl 上下文
 * @return  无
 */
void dtls_ssl_destroy(mbedtls_ssl_context* ssl);

/**
 * @brief   尝试写应用数据
 * @param   ssl [IN] ssl 上下文
 * @param   buf [IN] 保存数据的缓冲区
 * @param   len [IN] 尝试写入的应用数据的长度
 * @return  返回实际写入的数据长度，或者失败原因
 */
int dtls_write(mbedtls_ssl_context* ssl, const unsigned char* buf, size_t len);

/**
 * @brief   读应用数据
 * @param   ssl [IN] ssl 上下文
 * @param   buf [IN] 将保存数据的缓冲区
 * @param   len [IN] 要读取的最大字节数
 * @param   timeout [IN] 超时值（以 ms 为单位），使用 0 表示无超时（默认）
 * @return  返回实际读入的数据长度，或者失败原因
 */
int dtls_read(mbedtls_ssl_context* ssl, unsigned char* buf, size_t len, uint32_t timeout);

/**
 * @brief   接受远端客户端的连接
 * @param   bind_ctx [IN] 相关 socket
 * @param   client_ctx [IN] 将包含已连接的客户端 socket
 * @param   client_ip [IN] 将包含的客户端 IP 地址
 * @param   buf_size [IN] 客户端 IP 地址缓冲区的大小
 * @param   ip_len [IN] 将接收写入的 IP 地址的长度
 * @return  成功返回0，否则返回失败原因
 */
int dtls_accept( mbedtls_net_context *bind_ctx,
                            mbedtls_net_context *client_ctx,
                            void *client_ip, size_t buf_size, size_t *ip_len );

#endif
