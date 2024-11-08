/*******************************************************************************
 *
 * Copyright (c) 2019-2021 天翼物联科技有限公司. All rights reserved.
 *
 *******************************************************************************/

#ifndef CONNECTION_H_
#define CONNECTION_H_
#include "ctlw_config.h"

#ifdef PLATFORM_LINUX
#include <unistd.h> //read,write,ctlw_usleep
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> //posix api (gethostbyaddr,gethostbyname)
#endif

#ifdef PLATFORM_XYZ
#include "netdb.h"
#endif

#ifdef PLATFORM_XINYI
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/errno.h"

#endif

#if CTIOT_CHIPSUPPORT_DTLS == 1
//添加mbed dtls 用到的头
#include "mbedtls/ssl.h"
#endif

#define LWM2M_STANDARD_PORT_STR "5683"
#define LWM2M_STANDARD_PORT 5683
#define LWM2M_DTLS_PORT_STR "5684"
#define LWM2M_DTLS_PORT 5684
#define LWM2M_BSSERVER_PORT_STR "5685"
#define LWM2M_BSSERVER_PORT 5685

typedef struct _connection_t
{
    struct _connection_t *next;
    int sock;
    struct sockaddr_in6 addr;
    size_t addrLen;
#if CTIOT_CHIPSUPPORT_DTLS == 1
	mbedtls_ssl_context *ssl;
#endif
} connection_t;

int ctlw_create_socket(const char *portStr, int ai_family);
connection_t *ctlw_connection_find(connection_t *connList, struct sockaddr *addr, size_t addrLen);
connection_t *ctlw_connection_new_incoming(connection_t *connList, int sock, struct sockaddr *addr, size_t addrLen);
connection_t *ctlw_connection_create(connection_t *connList, int sock, char *host, char *port, int addressFamily);
void ctlw_connection_free(connection_t *connList);
int ctlw_connection_send(connection_t *connP, uint8_t *buffer, size_t length, uint8_t sendOption);
int16_t ctlw_sendto(connection_t *connP, uint8_t *startPos, uint16_t length, uint8_t sendOption);
int ctlw_receivefrom(int sock, uint8_t *buffer, uint16_t maxBufferLen, struct sockaddr *addr, socklen_t *addrLen);

//****************************************************************
//
//! @brief	ctlw_get_ip_type:获取芯片ip类型
//
//! @param	void
//! @retval 返回chip_ip_type_e：
//  CHIP_IP_TYPE_FALSE,	CHIP_IP_TYPE_V4ONLY,CHIP_IP_TYPE_V6ONLY,
//  CHIP_IP_TYPE_V4V6,CHIP_IP_TYPE_V6ONLY_V6PREPARING,CHIP_IP_TYPE_V4V6_V6PREPARING
//
//****************************************************************
uint16_t ctlw_get_ip_type(void);

//****************************************************************
//
//! @brief	ctlw_get_local_ip:查询指令类型的ip地址
//
//! @param	localIP：出参，本地ip地址；addressFamily；入参，ip地址类型-AF_INET或AF_INET6
//! @retval 详见CTIOT_NB_ERRORS
//
//****************************************************************
uint16_t ctlw_get_local_ip(char *localIP,int addressFamily);

#endif
