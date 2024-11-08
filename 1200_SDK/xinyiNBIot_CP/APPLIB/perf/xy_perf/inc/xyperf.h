#pragma once

#include "cmsis_os2.h"

typedef struct
{
    char protocol_type;       /* 协议类型0:udp 1:tcp */
    char remote_ip[40];       /* 服务端ip地址，字符串形式 */
    int remote_port;          /* 服务端port */
    unsigned int duration;    /* 灌包持续时间 */
    unsigned int packet_size; /* 灌包携带数据大小 */
    unsigned int rate;        /* 带宽 */
    unsigned int print_interval; /* 打印间隔,目前强制客户设置为1秒 */
    int rai_val;
	int ip_type;			/* ip类型   AF_INET:ipv4 AF_INET6:ipv6 */
} xyperf_arguments_t;

/* iperf标签 */
struct xyperf_udp_datagram {
	signed long id;			// 报文ID
	unsigned long tv_sec;	// 报文发送时间戳(s)
	unsigned long tv_usec;	// 报文发送时间戳(us)
};

extern osThreadId_t g_xyperf_TskHandle;



