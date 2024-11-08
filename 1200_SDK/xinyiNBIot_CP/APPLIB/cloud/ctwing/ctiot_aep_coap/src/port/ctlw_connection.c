/*******************************************************************************
 *
 * Copyright (c) 2019-2021 天翼物联科技有限公司. All rights reserved.
 *
 *******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "ctlw_config.h"
#ifdef PLATFORM_XINYI
#include "ps_netif_api.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "ctwing_util.h"
#include "xy_socket_api.h"
#endif


#include "ctlw_connection.h"
#include "ctlw_liblwm2m.h"
#include "ctlw_lwm2m_sdk.h"
#include "ctlw_abstract_signal.h"

#ifdef PLATFORM_LINUX
//使用 ifconf结构体和ioctl函数时需要用到该头文件
#include <net/if.h>
#include <sys/ioctl.h>
//消息队列需要用到的头
#include <sys/ipc.h>
#include <sys/types.h>
//使用ifaddrs结构体时需要用到该头文件
#include <ifaddrs.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif
#ifdef PLATFORM_LINUX_DEMO
#include "event.h"
#endif

#if CTIOT_CHIPSUPPORT_DTLS == 1
#include "ctlw_mbedtls_interface.h"
#endif



#ifdef WITH_LOGS
// from commandline.c
void output_buffer(FILE *stream, uint8_t *buffer, int length, int indent);
#endif

int ctlw_receivefrom(int sock, uint8_t *buffer, uint16_t maxBufferLen, struct sockaddr *addr, socklen_t *addrLen)
{
    int len = 0;
    len = recvfrom(sock, buffer, maxBufferLen, 0, addr, addrLen);
    return len;
}

int16_t ctlw_sendto(connection_t *connP, uint8_t *startPos, uint16_t length, uint8_t sendOption)
{
    int nbSent;
	ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_SOCKET_CLASS,"RAIIndication:%u\r\n",sendOption);

#ifdef PLATFORM_XYZ
    nbSent = ps_sendto(connP->sock, startPos, length, 0, (struct sockaddr *)&(connP->addr), connP->addrLen, sendOption, false);
#endif

#ifdef PLATFORM_LINUX
	nbSent = sendto(connP->sock, startPos, length, 0, (struct sockaddr *)&(connP->addr), connP->addrLen);
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_SOCKET_CLASS, "send to addrLen=%d",connP->addrLen);
#endif
#ifdef PLATFORM_XINYI
	nbSent = sendto2(connP->sock, startPos, length, 0, (struct sockaddr *)&(connP->addr), connP->addrLen,0,sendOption);
#endif
    return nbSent;
}

int ctlw_create_socket(const char *portStr, int addressFamily)
{
    int s = -1;
#if PLATFORM_XINYI
	ctiot_context_t *pContext = ctiot_get_context();   
	uint16_t local_port = strtoul(portStr, NULL, 10);
	if(local_port == 0)
	{
		return s;
	}

	if (addressFamily == AF_INET)
	{
		pContext->portV4 = pContext->portV4 == 0 ? CTIOT_DEFAULT_PORT : pContext->portV4;
		s = xy_socket_by_host(pContext->serverIPV4, Sock_IPv4_Only, IPPROTO_UDP, local_port, pContext->portV4, NULL);
	}
	else if (addressFamily == AF_INET6)
	{
		pContext->portV6 = pContext->portV6 == 0 ? CTIOT_DEFAULT_PORT : pContext->portV6;
		s = xy_socket_by_host(pContext->serverIPV6, Sock_IPv6_Only, IPPROTO_UDP, local_port, pContext->portV6, NULL);
	}
	
	xy_ctlw_get_local_ip(pContext->localIP, addressFamily);

#else
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = addressFamily;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if (0 != getaddrinfo(NULL, portStr, &hints, &res))
    {
        return -1;
    }

    for (p = res; p != NULL && s == -1; p = p->ai_next)
    {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s >= 0)
        {
            if (-1 == bind(s, p->ai_addr, p->ai_addrlen))
            {
                close(s);
                s = -1;
            }
        }
    }
    freeaddrinfo(res);
#endif

    return s;
}

connection_t *ctlw_connection_find(connection_t *connList,
                              struct sockaddr *addr,
                              size_t addrLen)
{
    connection_t *connP;
    connP = connList;
    while (connP != NULL)
    {
        if ((connP->addrLen == addrLen) && (memcmp(&(connP->addr), addr, addrLen) == 0))
        {
            return connP;
        }
        connP = connP->next;
    }
    return connP;
}

connection_t *ctlw_connection_new_incoming(connection_t *connList,
                                      int sock,
                                      struct sockaddr *addr,
                                      size_t addrLen)
{
    connection_t *connP;
    ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_SOCKET_CLASS,"ctlw_connection_new_incoming,addrLen=%d\n",addrLen);
    connP = (connection_t *)ctlw_lwm2m_malloc(sizeof(connection_t));
	ctiot_context_t* pContext = ctiot_get_context();
    if (connP != NULL)
    {
        connP->sock = sock;
        memcpy(&(connP->addr), addr, addrLen);
        connP->addrLen = addrLen;

#if CTIOT_CHIPSUPPORT_DTLS == 1
		if(pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS)
		{
			connP->ssl = pContext->ssl;
		}
#endif
        connP->next = connList;
    }
    return connP;
}

connection_t *ctlw_connection_create(connection_t *connList,
                                int sock,
                                char *host,
                                char *port,
                                int addressFamily)
{
    struct addrinfo hints;
    struct addrinfo *servinfo = NULL;
    struct addrinfo *p;
    int s;
    struct sockaddr *sa;
    socklen_t sl;
    connection_t *connP = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = addressFamily;
    hints.ai_socktype = SOCK_DGRAM;
    // we test the various addresses
   	if (0 != getaddrinfo(host, port, &hints, &servinfo) || servinfo == NULL)
    {
    	ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_SOCKET_CLASS,"getaddrinfo failed");
        return NULL;
    }
    s = -1;
    for (p = servinfo; p != NULL && s == -1; p = p->ai_next)
    {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s >= 0)
        {
            sa = p->ai_addr;
			#ifdef PLATFORM_XINYI
            sl = sa->sa_len;
			#else
			sl = p->ai_addrlen;
			#endif
            if (-1 == connect(s, p->ai_addr, p->ai_addrlen))
            {
                close(s);
                s = -1;
            }
        }
		else
		{
			ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_SOCKET_CLASS,"socket create failed");
		}
    }
    if (s >= 0)
    {
    	connP = ctlw_connection_find(connList, sa, sl);
		if(connP==NULL)/*在已有的connectionList中不存在，新建一个*/
		{
        	connP = ctlw_connection_new_incoming(connList, sock, sa, sl);
		}
        close(s);
    }
    if (NULL != servinfo)
    {
        freeaddrinfo(servinfo);
    }

    return connP;
}

void ctlw_connection_free(connection_t *connList)
{
    while (connList != NULL)
    {
        connection_t *nextP;
        nextP = connList->next;
        ctlw_lwm2m_free(connList);
        connList = nextP;
    }
    connList = NULL;
}

int ctlw_connection_send(connection_t *connP,
                    uint8_t *buffer,
                    size_t length, uint8_t sendOption)
{
    int nbSent;
    size_t offset;
    ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_SOCKET_CLASS,"ctlw_connection_send:sendOption=%d\r\n", sendOption);
#ifdef WITH_LOGS
    char s[INET6_ADDRSTRLEN];
    in_port_t port;

    s[0] = 0;
    if (AF_INET == connP->addr.sin_family)
    {
        struct sockaddr_in *saddr = (struct sockaddr_in *)&connP->addr;
        inet_ntop(saddr->sin_family, &saddr->sin_addr, s, INET_ADDRSTRLEN);
        port = saddr->sin_port;
    }

    ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_SOCKET_CLASS,"Sending %d bytes to [%s]:%hu\r\n", length, s, ntohs(port));
    output_buffer(stderr, buffer, length, 0);
#endif
	ctiot_context_t* pContext = ctiot_get_context();
	if(pContext->connectionType == MODE_NO_DTLS || pContext->connectionType == MODE_ENCRYPTED_PAYLOAD)
	{
	    offset = 0;
	    while (offset != length)
	    {

	        nbSent = ctlw_sendto(connP, buffer + offset, length - offset, sendOption);
	        if (nbSent == -1)
	        {
	            return -1;
	        }
	        offset += nbSent;
	    }
	}
#if CTIOT_CHIPSUPPORT_DTLS == 1
	else if(pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS)
	{
		//DTLS write;
		nbSent = ctlw_dtls_write(pContext->ssl, buffer, length,sendOption);
		if(nbSent < 0)
		{
			pContext->lastDtlsErrCode = nbSent;
			return -1;
		}
	}
#endif
    return 0;
}

uint8_t ctlw_lwm2m_buffer_send(void *sessionH,
                          uint8_t *buffer,
                          size_t length,
                          void *userdata, uint8_t sendOption)
{
    connection_t *connP = (connection_t *)sessionH;

    if (connP == NULL)
    {
        ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_SOCKET_CLASS,"#> failed sending %lu bytes, missing connection\r\n", length);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }
    if (-1 == ctlw_connection_send(connP, buffer, length, sendOption))
    {
        ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_SOCKET_CLASS,"#> failed sending %lu bytes\r\n", length);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    return COAP_NO_ERROR;
}

bool ctlw_lwm2m_session_is_equal(void *session1,
                            void *session2,
                            void *userData)
{
    return (session1 == session2);
}

uint16_t ctlw_get_ip_type(void)
{
#ifdef PLATFORM_XYZ
	NmAtiSyncRet netInfo = {0};
#endif
#ifdef PLATFORM_LINUX_DEMO
	return test_get_ip_type();
#endif
#ifdef PLATFORM_XYZ
	appGetNetInfoSync(0 /*pContext->chipInfo.cCellID*/, &netInfo);
	return netInfo.body.netInfoRet.netifInfo.ipType;
#endif

#ifdef PLATFORM_XINYI
	return xy_ctlw_get_iptype();
#endif
}

uint16_t ctlw_get_local_ip(char *localIP,int addrFamily)
{
    uint16_t result = CTIOT_IP_NOK_ERROR;
#ifdef PLATFORM_XYZ
	NmAtiSyncRet netInfo= { 0 };
#endif
#ifdef PLATFORM_LINUX_DEMO
	uint8_t ipType = test_get_ip_type();
	ctiot_signal_set_chip_ip_type(ipType);
	if(addrFamily == AF_INET)
	{
		if(ipType == NM_NET_TYPE_IPV4 || ipType == NM_NET_TYPE_IPV4V6 || ipType == NM_NET_TYPE_IPV4_IPV6preparing)
		{
			if(localIP!=NULL)
			{
				strcpy(localIP,test_get_ip());
			}
			result = CTIOT_NB_SUCCESS;
		}
		else if(ipType == NM_NET_TYPE_INVALID)
		{
			ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_SOCKET_CLASS,"CTIOT_IP_NOK_ERROR");
			result = CTIOT_IP_NOK_ERROR;
		}
		else if(ipType == NM_NET_TYPE_IPV6 || ipType == NM_NET_TYPE_IPV6preparing)
		{
			ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_SOCKET_CLASS,"CTIOT_IP_TYPE_ERROR");
			result = CTIOT_IP_TYPE_ERROR;
		}
		goto exit;
	}
	else if(addrFamily == AF_INET6)
	{
		if(ipType == NM_NET_TYPE_IPV6 || ipType == NM_NET_TYPE_IPV4V6)
		{
			if(localIP!=NULL)
			{
				strcpy(localIP,test_get_ipV6());
			}
			result = CTIOT_NB_SUCCESS;
		}
		else if(ipType == NM_NET_TYPE_INVALID)
		{
			result = CTIOT_IP_NOK_ERROR;
		}
		else if(ipType == NM_NET_TYPE_IPV4)
		{
			result = CTIOT_IP_TYPE_ERROR;
		}
		else if(ipType == NM_NET_TYPE_IPV6preparing || ipType == NM_NET_TYPE_IPV4_IPV6preparing)
		{
			result = CTIOT_IPV6_ONGOING_ERROR;
		}
		goto exit;
	}

#endif
#ifdef PLATFORM_XYZ
    if(appGetNetInfoSync(0 /*pContext->chipInfo.cCellID*/, &netInfo)!=0)
    {
    	result = CTIOT_SYS_API_ERROR;
		goto exit;
    }
	ctiot_signal_set_chip_ip_type(netInfo.body.netInfoRet.netifInfo.ipType);
	if(addrFamily == AF_INET)
	{

	    if (NM_NET_TYPE_IPV4 == netInfo.body.netInfoRet.netifInfo.ipType || NM_NET_TYPE_IPV4V6 == netInfo.body.netInfoRet.netifInfo.ipType || NM_NET_TYPE_IPV4_IPV6preparing == netInfo.body.netInfoRet.netifInfo.ipType)
	    {
	    	if(netInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr == 0)
	    	{
	    		result = CTIOT_IP_NOK_ERROR;
	    	}
			else
			{
				if(localIP!=NULL)
				{
		        	uint8_t *ips = (uint8_t *)&netInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr;
		        	//sprintf(localIP, "%u.%u.%u.%u", ips[0], ips[1], ips[2], ips[3]);
		        	inet_ntop(AF_INET, ips, localIP, 40);
		        	ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_SOCKET_CLASS,"local ip:%s\r\n", localIP);
				}
		        result = CTIOT_NB_SUCCESS;
			}
	    }
		else if(NM_NET_TYPE_IPV6 == netInfo.body.netInfoRet.netifInfo.ipType || NM_NET_TYPE_IPV6preparing == netInfo.body.netInfoRet.netifInfo.ipType)
		{
			result = CTIOT_IP_TYPE_ERROR;
		}
		else if(NM_NET_TYPE_INVALID == netInfo.body.netInfoRet.netifInfo.ipType)
		{
			result = CTIOT_IP_NOK_ERROR;
		}
	}
	else if(addrFamily == AF_INET6)
	{
		if (NM_NET_TYPE_IPV6 == netInfo.body.netInfoRet.netifInfo.ipType || NM_NET_TYPE_IPV4V6 == netInfo.body.netInfoRet.netifInfo.ipType)
	    {
	    	if( netInfo.body.netInfoRet.netifInfo.ipv6Info.ipv6Addr.addr[0] == 0
			 && netInfo.body.netInfoRet.netifInfo.ipv6Info.ipv6Addr.addr[1] == 0
			 && netInfo.body.netInfoRet.netifInfo.ipv6Info.ipv6Addr.addr[2] == 0
			 && netInfo.body.netInfoRet.netifInfo.ipv6Info.ipv6Addr.addr[3] == 0 )
	    	{
	    		result = CTIOT_IP_NOK_ERROR;
	    	}
			else
			{
				if(localIP != NULL)
				{
		        	uint8_t *ips = (uint8_t *)&netInfo.body.netInfoRet.netifInfo.ipv6Info.ipv6Addr.addr;
		        	//sprintf(localIP, "%u.%u.%u.%u", ips[0], ips[1], ips[2], ips[3]);
		        	inet_ntop(AF_INET6, ips, localIP, 40);
		        	ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_SOCKET_CLASS,"local ip:%s\r\n", localIP);
				}
		        result = CTIOT_NB_SUCCESS;
			}
	    }
		else if(NM_NET_TYPE_IPV4 == netInfo.body.netInfoRet.netifInfo.ipType)
		{
			result = CTIOT_IP_TYPE_ERROR;
		}
		else if(NM_NET_TYPE_INVALID == netInfo.body.netInfoRet.netifInfo.ipType)
		{
			result = CTIOT_IP_NOK_ERROR;
		}
		else if(NM_NET_TYPE_IPV4_IPV6preparing == netInfo.body.netInfoRet.netifInfo.ipType || NM_NET_TYPE_IPV6preparing == netInfo.body.netInfoRet.netifInfo.ipType)
		{
			ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_SOCKET_CLASS,"ctlw_get_local_ip:ipv6 preparing");
			result = CTIOT_IPV6_ONGOING_ERROR;
		}
	}
#endif
#ifdef PLATFORM_XINYI
	result = xy_ctlw_get_local_ip(localIP, addrFamily);
#endif

exit:
    if (result != CTIOT_NB_SUCCESS)
        ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_SOCKET_CLASS,"ctiot_get_localIP result = %u\r\n",result);
    return result;
}

