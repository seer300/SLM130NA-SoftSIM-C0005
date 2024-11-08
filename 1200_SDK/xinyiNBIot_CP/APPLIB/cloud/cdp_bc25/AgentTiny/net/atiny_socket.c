/*----------------------------------------------------------------------------
 * Copyright (c) <2016-2018>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 *---------------------------------------------------------------------------*/

#if TELECOM_VER
#include "atiny_socket.h"
#include "atiny_osdep.h"
#include "xy_socket_api.h"
#include "lwip/opt.h"
#include "lwip/netdb.h"
#include "lwip/errno.h"
#include "cdp_backup.h"
#include "xy_net_api.h"
#include "atiny_context.h"
#include "oss_nv.h"
#include "cloud_utils.h"
#include "agent_tiny_demo.h"
#include "xy_rtc_api.h"
#include "cdp_backup.h"
#include "net_app_resume.h"
#include "xy_socket_api.h"

#if WITH_MBEDTLS_SUPPORT
#include "mbedtls/ssl.h"
#include "mbedtls/net_sockets.h"
#endif

#define SOCKET_DEBUG

#if defined(SOCKET_DEBUG)
#define SOCKET_LOG(fmt, ...) \
    do \
    { \
        (void)atiny_printf("[SOCKET][%s:%d] " fmt "\r\n", \
        __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while(0)
#else
#define SOCKET_LOG(fmt, ...) ((void)0)
#endif

unsigned short g_cdp_localPort = 0;
//将发送指令AT+NMGS/QLWULDATA/QLWULDATAEX携带的seq_num和msg_type存放至全局数组cdp_sn_info中
uint8_t cdp_sn_info[DATALIST_MAX_LEN][2];

//对cdp_sn_info全局数组操作锁保护
osMutexId_t cdp_sninfo_mux = NULL;

//将发送指令携带的seq_num和msg_type添加到cdp_sn_info数组中，最多添加DATALIST_MAX_LEN个；
void cdp_add_sninfo_node(uint8_t sequence, int msg_type)
{
	int i;
	if(sequence == 0)
		return;
	if(cdp_sninfo_mux == NULL)
		cdp_sninfo_mux = osMutexNew(NULL);
	osMutexAcquire(cdp_sninfo_mux, osWaitForever);

	for(i = 0; i< DATALIST_MAX_LEN;i++)
	{
		if(cdp_sn_info[i][0] == 0)
		{
			cdp_sn_info[i][0] = sequence;
			cdp_sn_info[i][1] = msg_type;
			break;
		}
	}
	osMutexRelease(cdp_sninfo_mux);
}

//使用完成后，从cdp_sn_info中删除其使用状态和消息类型
void cdp_del_sninfo_node(uint8_t sequence)
{
	int i;
	if(cdp_sninfo_mux == NULL)
		cdp_sninfo_mux = osMutexNew(NULL);
	osMutexAcquire(cdp_sninfo_mux, osWaitForever);
	
	for(i = 0; i< DATALIST_MAX_LEN; i++)
	{
		if(cdp_sn_info[i][0] == sequence)
		{
			memcpy(&cdp_sn_info[i][0], &cdp_sn_info[i+1][0], (DATALIST_MAX_LEN-i-1)*2);
			cdp_sn_info[DATALIST_MAX_LEN-1][0] = 0;
			cdp_sn_info[DATALIST_MAX_LEN-1][1] = 0;
			break;
		}
	}
	osMutexRelease(cdp_sninfo_mux);
}

//用于检测携带的seq_num 是否在使用中
bool cdp_find_match_udp_node(uint8_t sequence)
{
	int i;
	if(sequence == 0)
		return 0;
	for(i = 0; i < DATALIST_MAX_LEN; i++)
	{
		if(cdp_sn_info[i][0] == sequence)
		{
			return 1;
		}
	}
	return 0;
}

//用于检测携带的seq_num的消息类型
bool cdp_check_con_node(uint8_t sequence)
{
	int i;
	for(i = 0; i < DATALIST_MAX_LEN; i++)
	{
		if(cdp_sn_info[i][0] == sequence)
		{
			if(cdp_sn_info[i][1] == cdp_NON || cdp_sn_info[i][1] == cdp_NON_RAI || cdp_sn_info[i][1] == cdp_NON_WAIT_REPLY_RAI)
				return 0;
			else
				continue;
		}
	}
	return 1;
}

#if WITH_MBEDTLS_SUPPORT
//cdp 专用的dtls握手接口，用于区分通用接口
int atiny_dtls_shakehand(mbedtls_ssl_context *ssl, const dtls_shakehand_info_s *info)
{
    int ret = MBEDTLS_ERR_NET_CONNECT_FAILED;
    uint32_t change_value = 0;
    mbedtls_net_context *server_fd = NULL;
    uint32_t max_value;
#if defined(MBEDTLS_X509_CRT_PARSE_C)
    unsigned int flags;
#endif

    if (MBEDTLS_SSL_IS_CLIENT == info->client_or_server)
    {
        server_fd = atiny_net_connect(info->u.c.host, info->u.c.port, info->udp_or_tcp);
    }
    else
    {
        server_fd = (mbedtls_net_context*)atiny_net_bind(NULL, info->u.s.local_port, MBEDTLS_NET_PROTO_UDP);
    }

    if (server_fd == NULL)
    {
        xy_printf(0,XYAPP, WARN_LOG,"connect failed! mode %d", info->client_or_server);
        ret = MBEDTLS_ERR_NET_CONNECT_FAILED;
        goto exit_fail;
    }
	
#if defined(MBEDTLS_SSL_PROTO_DTLS)
    mbedtls_ssl_conf_handshake_timeout(ssl->conf, 4000, 32000);
#endif

	mbedtls_ssl_set_bio(ssl, server_fd,
                        atiny_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);


    max_value = ((MBEDTLS_SSL_IS_SERVER == info->client_or_server || info->udp_or_tcp == MBEDTLS_NET_PROTO_UDP) ?
                (cloud_gettime_s() + info->timeout) :  50);

    do
    {
        ret = mbedtls_ssl_handshake(ssl);
        //MBEDTLS_LOG("mbedtls_ssl_handshake %d %d", change_value, max_value);
        //LOS_TaskDelay(1);
        if (MBEDTLS_SSL_IS_CLIENT == info->client_or_server && info->udp_or_tcp == MBEDTLS_NET_PROTO_TCP)
        {
            change_value++;
        }
        else
        {
            change_value = cloud_gettime_s();
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
        xy_printf(0,XYAPP, WARN_LOG,"mbedtls_ssl_handshake failed: -0x%x", -ret);
        goto exit_fail;
    }

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    if (info->psk_or_cert == VERIFY_WITH_CERT)
    {
        if((flags = mbedtls_ssl_get_verify_result(ssl)) != 0)
        {
            char vrfy_buf[512];
            mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);
            xy_printf(0,XYAPP, WARN_LOG,"cert verify failed: %s", vrfy_buf);
            goto exit_fail;
        }
        else
            xy_printf(0,XYAPP, WARN_LOG,"cert verify succeed");
    }
#endif


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
#endif

void *atiny_net_bind(const char *host, const char *port, int proto)
{
#if defined (WITH_LWIP) || defined (WITH_LINUX)
    (void)host;
    (void)port;
    (void)proto;
    return NULL;
/*
    struct sockaddr_in sock_addr;
    int port_i;
    int ret = ATINY_NET_ERR;

    if (NULL == port || (proto != ATINY_PROTO_UDP && proto != ATINY_PROTO_TCP))
        return NULL;

    ctx = atiny_malloc(sizeof(atiny_net_context));

    sscanf(port , "%d", &port_i);
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = lwip_htons(port_i);
    sock_addr.sin_addr.s_addr = (host == NULL ? IPADDR_ANY : inet_addr(host));
    sock_addr.sin_len = sizeof(struct sockaddr_in);

    ctx->fd = socket(AF_INET,
        proto == ATINY_PROTO_TCP ? SOCK_STREAM : SOCK_DGRAM,
        proto == ATINY_PROTO_TCP ? IPPROTO_TCP : IPPROTO_UDP);

    if (ctx->fd < 0)
    {
        ret = ATINY_NET_SOCKET_FAILED;
        atiny_free(ctx);
        return NULL;
    }

	int n = 1;
	if( (ret = setsockopt( ctx->fd, SOL_SOCKET, SO_REUSEADDR,
					(const char *) &n, sizeof( n )) ) != 0 )
	{
		ret = ATINY_NET_SOCKET_FAILED;
		goto exit_failed;
	}

    ret = bind(ctx->fd, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr));
    if (ret < 0)
    {
       ret = ATINY_NET_BIND_FAILED;
       goto exit_failed;
    }

    if (proto == ATINY_PROTO_TCP)
    {
        ret = listen(ctx->fd, 20);
        if (ret < 0)
        {
            ret = ATINY_NET_LISTEN_FAILED;
            goto exit_failed;
        }
    }

    return ctx;

exit_failed:
       close(ctx->fd);
       atiny_free(ctx);
       return NULL;
       */


#elif defined(WITH_AT_FRAMEWORK)
    atiny_net_context *ctx;
    ctx = atiny_malloc(sizeof(atiny_net_context));
    if (NULL == ctx)
    {
    	SOCKET_LOG("malloc failed for socket context");
    	return NULL;
    }

    ctx->fd = at_api_bind(host, port, proto);
    if (ctx->fd < 0)
    {
    	SOCKET_LOG("unkown host or port");
    	atiny_free(ctx);
    	ctx = NULL;
    }
		return ctx;
#endif

}
int atiny_net_accept( void *bind_ctx, void *client_ctx, void *client_ip, size_t buf_size, size_t *ip_len )
{
#if defined (WITH_LWIP) || defined (WITH_LINUX)
    int bind_fd = ((atiny_net_context*)bind_ctx)->fd;
    int client_fd = ((atiny_net_context*)client_ctx)->fd;
    int type;
    int ret = ATINY_NET_ERR;
#if LWIP_IPV4 && LWIP_IPV6
	struct sockaddr_in client_addr;
#elif LWIP_IPV6
    struct sockaddr_in6 client_addr;
#else
    struct sockaddr_in client_addr;
#endif
    socklen_t type_len, client_addr_len;

    type_len = sizeof(type);
    if (getsockopt(bind_fd, SOL_SOCKET, SO_TYPE, (void*)&type, &type_len) != 0 ||
        (type != SOCK_STREAM && type != SOCK_DGRAM))
    {
        return ATINY_NET_ACCEPT_FAILED;
    }
#if LWIP_IPV4 && LWIP_IPV6
	client_addr_len = sizeof(struct sockaddr_in);
#elif LWIP_IPV6
    client_addr_len = sizeof(struct sockaddr_in6);
#else
    client_addr_len = sizeof(struct sockaddr_in);
#endif
    if (type == SOCK_STREAM)
    {
        ret = client_fd = accept(bind_fd, (struct sockaddr*)&client_addr, &client_addr_len);
    }
    else
    {
        //udp
        char buf[1] = {0};
        ret = recvfrom(bind_fd, buf, sizeof(buf), MSG_PEEK, (struct sockaddr*)&client_addr, &client_addr_len);
    }

    if (ret < 0)
        return ATINY_NET_ACCEPT_FAILED;
    if (type != SOCK_STREAM)
    {
#if LWIP_IPV4 && LWIP_IPV6
		struct sockaddr_in	local_addr;
		socklen_t n = sizeof(struct sockaddr_in);
#elif LWIP_IPV6
        struct sockaddr_in6  local_addr;
        socklen_t n = sizeof(struct sockaddr_in6);
#else
        struct sockaddr_in  local_addr;
        socklen_t n = sizeof(struct sockaddr_in);
#endif
        char port_s[6] = {0};
        int one = 1; 
        
        ((atiny_net_context*)client_ctx)->fd = client_fd = bind_fd;
        ((atiny_net_context*)bind_ctx)->fd = bind_fd = -1;

        if (connect(client_fd, (struct sockaddr *)&client_addr, client_addr_len) != 0)
            return ATINY_NET_ACCEPT_FAILED;

        ret = getsockname(client_fd, (struct sockaddr*)&local_addr, &n);
#if LWIP_IPV4 && LWIP_IPV6
		snprintf(port_s, sizeof(port_s), "%d", ntohs(local_addr.sin_port));
		((atiny_net_context*)bind_ctx)->fd = socket(local_addr.sin_family, SOCK_DGRAM, IPPROTO_UDP);
#elif LWIP_IPV6
        snprintf(port_s, sizeof(port_s), "%d", ntohs(local_addr.sin6_port));
        ((atiny_net_context*)bind_ctx)->fd = socket(local_addr.sin6_family, SOCK_DGRAM, IPPROTO_UDP);
#else
        snprintf(port_s, sizeof(port_s), "%d", ntohs(local_addr.sin_port));
        ((atiny_net_context*)bind_ctx)->fd = socket(local_addr.sin_family, SOCK_DGRAM, IPPROTO_UDP);
#endif 
        if( (ret = setsockopt( ((atiny_net_context*)bind_ctx)->fd, SOL_SOCKET, SO_REUSEADDR,
					(const char *) &one, sizeof( one )) ) != 0 )
    	{
    		ret = ATINY_NET_SOCKET_FAILED;
    	}

        if (ret != 0)
            return ret;
    }

    if (client_ip != NULL)
    {
#if LWIP_IPV4 && LWIP_IPV6
		if( client_addr.sin_family == AF_INET )
		{
			struct sockaddr_in *addr = (struct sockaddr_in *) &client_addr;
			*ip_len = sizeof( addr->sin_addr.s_addr );

			if( buf_size < *ip_len )
				return( ATINY_NET_BUF_SMALL_FAILED );

			memcpy( client_ip, &addr->sin_addr.s_addr, *ip_len );
		}
#elif LWIP_IPV6
        if( client_addr.sin6_family == AF_INET6 )
        {
            struct sockaddr_in6 *addr = (struct sockaddr_in6 *) &client_addr;
            *ip_len = sizeof( addr->sin6_addr.s6_addr );

            if( buf_size < *ip_len )
                return( ATINY_NET_BUF_SMALL_FAILED );

            memcpy( client_ip, &addr->sin6_addr.s6_addr, *ip_len );
        }
#else
        if( client_addr.sin_family == AF_INET )
        {
            struct sockaddr_in *addr = (struct sockaddr_in *) &client_addr;
            *ip_len = sizeof( addr->sin_addr.s_addr );

            if( buf_size < *ip_len )
                return( ATINY_NET_BUF_SMALL_FAILED );

            memcpy( client_ip, &addr->sin_addr.s_addr, *ip_len );
        }
#endif
    }
#else
    ((atiny_net_context*)client_ctx)->fd = ((atiny_net_context*)bind_ctx)->fd;
#endif
    return 0;
}

void *atiny_net_connect(const char *host, const char *port, int proto)
{
#if defined(WITH_LINUX) || defined(WITH_LWIP)
	if(g_cdp_session_info == NULL || 
	//CDP 主业务起时SOCKET只会创建一次，创建失败走线程退出流程，此处添加条件限制防止重复创建SOCKET
	(g_phandle != NULL && g_phandle->lwm2m_context != NULL 
	&& g_phandle->lwm2m_context->serverList != NULL
	&& g_phandle->lwm2m_context->serverList->status == XY_STATE_REG_FAILED))
	{
		return NULL;
	}

    int flags;
    struct sockaddr_in6* addr = NULL;
    atiny_net_context *ctx = atiny_malloc(sizeof(atiny_net_context));

    ip_addr_t* remote_addr = atiny_malloc(sizeof(ip_addr_t)); 
    uint16_t remote_port = strtoul(port, NULL, 10);
    uint16_t local_port = 0;
    if (g_cdp_session_info->net_info.local_port != 0 && g_cdp_config_data->cloud_server_port == remote_port)
        local_port = g_cdp_session_info->net_info.local_port;
    
    if ((ctx->fd = xy_socket_by_host(host, Sock_IPv46, (proto == ATINY_PROTO_UDP) ? IPPROTO_UDP : IPPROTO_TCP,
                                     local_port, remote_port, remote_addr)) == -1)
        goto ERR_PROC;
	
	if (proto == ATINY_PROTO_UDP)
	{
		flags = fcntl(ctx->fd, F_GETFL, 0);

		if (flags < 0 || fcntl(ctx->fd, F_SETFL, flags | O_NONBLOCK) < 0)
		{
		    close(ctx->fd);
		    ctx->fd = -1;
			xy_printf(0,XYAPP, WARN_LOG, "[atiny_net_connect] flag err");
			goto ERR_PROC;
		}
	}

    addr = xy_malloc(sizeof(struct sockaddr_in6));
    memset(addr, 0x00, sizeof(struct sockaddr_in6));

    addr->sin6_family = (remote_addr->type == IPADDR_TYPE_V6)?AF_INET6:AF_INET;

    if(remote_addr->type == IPADDR_TYPE_V6)
    {
		memcpy(&(((struct sockaddr_in6 *)addr)->sin6_addr), &remote_addr->u_addr, sizeof(remote_addr->u_addr));
    }
    else
    {
		((struct sockaddr_in *)addr)->sin_addr.s_addr = remote_addr->u_addr.ip4.addr;
    }
	addr->sin6_port = lwip_htons(remote_port);
	if(connect(ctx->fd, (struct sockaddr*)addr, sizeof(struct sockaddr_in6)) < 0)
    {
        close(ctx->fd);
        ctx->fd = -1;
        xy_printf(0, XYAPP, WARN_LOG, "[atiny_net_connect] connect err");
        goto ERR_PROC;
    }
    
    //保存远端ip和本地ip,此处获取本地ip是为了防止网口up时获取到的本地ip为空
    memcpy(&g_cdp_session_info->net_info.remote_ip, remote_addr, sizeof(ip_addr_t));
	xy_get_ipaddr(g_cdp_session_info->net_info.remote_ip.type, &g_cdp_session_info->net_info.local_ip);
	
#elif defined(WITH_AT_FRAMEWORK)
    ctx = atiny_malloc(sizeof(atiny_net_context));
    if (NULL == ctx)
    {
        SOCKET_LOG("malloc failed for socket context");
        return NULL;
    }

    ctx->fd = at_api_connect(host, port, proto);
    if (ctx->fd < 0)
    {
        SOCKET_LOG("unkown host or port");
        atiny_free(ctx);
        ctx = NULL;
    }
#elif defined(WITH_WIZNET)
    ctx = atiny_malloc(sizeof(atiny_net_context));
    if (NULL == ctx)
    {
        SOCKET_LOG("malloc failed for socket context");
        return NULL;
    }

    ctx->fd = wiznet_connect(host, port, proto);
    if (ctx->fd < 0)
    {
        SOCKET_LOG("unkown host or port");
        atiny_free(ctx);
        ctx = NULL;
    }
#else
#endif

ERR_PROC:
	if (ctx != NULL && ctx->fd < 0)
    {
        SOCKET_LOG("unkown host or port");
        atiny_free(ctx);
        ctx = NULL;
    }
    if (remote_addr != NULL)
        atiny_free(remote_addr);
    
    if (addr != NULL)
        atiny_free(addr);
    return ctx;
}

int atiny_net_recv(void *ctx, unsigned char *buf, size_t len)
{
    int ret = -1;
    int fd = ((atiny_net_context *)ctx)->fd;
#if defined(WITH_LINUX) || defined(WITH_LWIP)
    ret = recv(fd, buf, len, 0);
#elif defined(WITH_AT_FRAMEWORK)
    ret = at_api_recv(fd, buf, len);
#elif defined(WITH_WIZNET)
    ret = wiznet_recv(fd, buf, len);
#else
    (void)fd; //clear unuse warning
#endif

#if defined(WITH_LINUX) || defined(WITH_LWIP)
    if (ret < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
            SOCKET_LOG("no data available for now");
            return 0;
        }
        else
        {
            SOCKET_LOG("error accured when recv: 0x%x", errno);
            return -1;
        }
    }

    else if (ret == 0)
    {
        SOCKET_LOG("socket was closed by peer");
        return -1;
    }
#endif
    return ret;
}

int atiny_net_recv_timeout(void *ctx, unsigned char *buf, size_t len,
                           uint32_t timeout)
{
    int ret = -1;
#if defined(WITH_LINUX) || defined(WITH_LWIP)
    struct timeval tv;
    fd_set read_fds;
#endif

    int fd = ((atiny_net_context *)ctx)->fd;

#if defined(WITH_LINUX) || defined(WITH_LWIP)
    if (fd < 0)
    {
        SOCKET_LOG("ilegal socket(%d)", fd);
        return ATINY_NET_ERR;
    }

    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);

    if(timeout != 0)
    {
        tv.tv_sec  = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;

        ret = select(fd + 1, &read_fds, NULL, NULL, &tv);
    }
    else
    {
        ret = select(fd + 1, &read_fds, NULL, NULL, NULL);
    }

    if (ret == 0)
    {
       // SOCKET_LOG("recv timeout");
        return ATINY_NET_TIMEOUT;
    }

    if(ret < 0)
    {
        SOCKET_LOG("select error ret=%d,err 0x%x", ret, errno);
        return ATINY_NET_ERR;
    }

    ret = atiny_net_recv(ctx, buf, len);

#elif defined(WITH_AT_FRAMEWORK)
    ret = at_api_recv_timeout(fd, buf, len, NULL,NULL,timeout);
#elif defined(WITH_WIZNET)
    ret = wiznet_recv_timeout(fd, buf, len, timeout);
#else
    (void)fd; //clear unuse warning
#endif
    return ret;
}

int atiny_net_send(void *ctx, const unsigned char *buf, size_t len)
{
    int ret = -1;
    int fd = ((atiny_net_context *)ctx)->fd;
	uint8_t raiflag = 0;
	uint8_t seq_num = 0;
	cdp_get_seq_and_rai(&raiflag, &seq_num);

    if (fd < 0)
    {
        SOCKET_LOG("ilegal socket(%d)", fd);
        return -1;
    }

#if defined(WITH_LINUX) || defined(WITH_LWIP)
//    ret = send(fd, buf, len, 0);
    ret = send2(fd, buf, len, 0, seq_num, raiflag);

	//cdp_set_seq_and_rai(0, 0);

#elif defined(WITH_AT_FRAMEWORK)
    ret = at_api_send(fd, buf, len);
#elif defined(WITH_WIZNET)
    ret = wiznet_send(fd, buf, len);
#else
#endif

#if defined(WITH_LINUX) || defined(WITH_LWIP)
    if (ret < 0)
    {
        /* no data available for now */
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
            return 0;
        }
        else
        {
            SOCKET_LOG("error accured when send: 0x%x", errno);
            return -1;
        }
    }
#endif

    return ret;
}

void atiny_net_close(void *ctx)
{
    int fd = ((atiny_net_context *)ctx)->fd;

    if (fd >= 0)
    {
#if defined(WITH_LINUX) || defined(WITH_LWIP)
        close(fd);
#elif defined(WITH_AT_FRAMEWORK)
        at_api_close(fd);
#elif defined(WITH_WIZNET)
        wiznet_close(fd);
#endif
    }

    atiny_free(ctx);
}

#if defined(WITH_LINUX) || defined(WITH_LWIP)
static int atiny_net_write_sock(void *ctx, const unsigned char *buffer, int len, uint32_t timeout_ms)
{
    int fd;
    struct timeval interval = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};

    fd = ((atiny_net_context *)ctx)->fd;
    if (interval.tv_sec < 0 || (interval.tv_sec == 0 && interval.tv_usec <= 0))
    {
        interval.tv_sec = 0;
        interval.tv_usec = 100;
    }

    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&interval, sizeof(struct timeval));
    return write(fd, buffer, len);
}
#endif

int atiny_net_send_timeout(void *ctx, const unsigned char *buf, size_t len,
                          uint32_t timeout)
{
#if defined(WITH_LINUX) || defined(WITH_LWIP)
    return atiny_net_write_sock(ctx, buf, len, timeout);
#elif defined(WITH_AT_FRAMEWORK)
        int fd;
        fd = ((atiny_net_context *)ctx)->fd;
        return at_api_send(fd , buf, (uint32_t)len);
#endif

}
#endif
