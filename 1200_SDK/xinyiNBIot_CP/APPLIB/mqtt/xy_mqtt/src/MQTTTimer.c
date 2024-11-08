/****************************************************************************************************
Copyright:   2018-2020, XinYi Info Tech Co.,Ltd.
File name:   MQTTTimer.c
Description: MQTT protocol API function
Author:  gaoj
Version:
Date:    2020.7.20
History:
 ****************************************************************************************************/
#include "MQTTClient.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "xy_rtc_api.h"
#include "xy_socket_api.h"
#include "xy_net_api.h"
#include "dtls_interface.h"

uint64_t mqtt_gettime_ms(void)
{
    return get_utc_ms();
}

void xy_gettimeofday(struct timeval *time)
{
    time->tv_sec  = (uint32_t)(mqtt_gettime_ms() / 1000);
    time->tv_usec  = 0;
}

void xy_timersub(struct timeval *a, struct timeval *b, struct timeval *res)
{
    long tv_sec = 0;
    long tv_usec = 0;

    tv_usec = a->tv_usec - b->tv_usec;
    if(tv_usec < 0)
    {
        tv_usec += 1000000;
        tv_sec -= 1;
    }
    tv_sec = tv_sec + a->tv_sec - b->tv_sec;

    res->tv_sec  = tv_sec;
    res->tv_usec = tv_usec;
}

void xy_timeradd(struct timeval *a, struct timeval *b, struct timeval *res)
{
    long tv_sec = 0;
    long tv_usec = 0;

    tv_usec = a->tv_usec + b->tv_usec;
    if(tv_usec >= 1000000)
    {
        tv_usec -= 1000000;
        tv_sec += 1;
    }
    tv_sec = tv_sec + a->tv_sec + b->tv_sec;

    res->tv_sec  = tv_sec;
    res->tv_usec = tv_usec;
}

void TimerInit(Timer* timer)
{
    timer->end_time = (struct timeval){0, 0};
}

char TimerIsExpired(Timer* timer)
{
    struct timeval now, res;
    xy_gettimeofday(&now);
    xy_timersub(&timer->end_time, &now, &res);

    return (res.tv_sec < 0) || (res.tv_sec == 0 && res.tv_usec <= 0);
}


void TimerCountdownMS(Timer* timer, unsigned int timeout)
{
    struct timeval now;
    xy_gettimeofday(&now);
    struct timeval interval = {timeout / 1000, (timeout % 1000) * 1000};
    xy_timeradd(&now, &interval, &timer->end_time);
}


void TimerCountdown(Timer* timer, unsigned int timeout)
{
    struct timeval now;
    xy_gettimeofday(&now);
    struct timeval interval = {timeout, 0};
    xy_timeradd(&now, &interval, &timer->end_time);
}


int TimerLeftMS(Timer* timer)
{
    struct timeval now, res;
    xy_gettimeofday(&now);
    xy_timersub(&timer->end_time, &now, &res);
    return (res.tv_sec < 0) ? 0 : res.tv_sec * 1000 + res.tv_usec / 1000;
}


int xy_mqtt_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	char flag = 0;
	int rc = 0;
	struct timeval interval = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};

    int bytes = 0;

    //由于MQTTS开源库读取数据的限制，传入的 timeout_ms (接收数据超时时间) 极小概率可能为0，以下对该情况进行特殊处理；
	if (!timeout_ms)
	{
        //TLS开源库读取数据时如果传入0值，会导致无限阻塞；
        if (n->ssl_ctx != NULL)
        {
            timeout_ms = 1;
        }
        else
        {
            flag = MSG_DONTWAIT; 
        }    
	}

    while (bytes < len)
    {
		if (n->ssl_ctx != NULL)
		{
			rc = dtls_read(n->ssl_ctx, &buffer[bytes], (size_t)(len - bytes), timeout_ms);
			
			if (rc < 0)
	        {
	            if (rc != -2)
	            	bytes = -1;
	            break;
	        }
	        else if (rc == 0)
	        {
	            bytes = 0;
	            break;
	        }
	        else
	            bytes += rc;	
		}
		else
		{
			setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&interval, sizeof(struct timeval));

		    rc = recv(n->my_socket, &buffer[bytes], (size_t)(len - bytes), flag);
			
			if (rc <= 0)
	        {
	            if (errno != EAGAIN && errno != EWOULDBLOCK)
	            	bytes = -1;
	            break;
	        }
	        else
	            bytes += rc;
		}

    }
    return bytes;
}


int xy_mqtt_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
    struct timeval tv = {0};
	int rc = 0;

	tv.tv_sec = 0;	/* 30 Secs Timeout */
	tv.tv_usec = timeout_ms * 1000;  // Not init'ing this can cause strange errors

	setsockopt(n->my_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv,sizeof(struct timeval));

	if (n->ssl_ctx != NULL)
	{
		rc = dtls_write(n->ssl_ctx, buffer, len);
	}
	else
	{
		rc = write(n->my_socket, buffer, len);
	}

    return rc;
}


void NetworkInit(Network* n)
{
    n->my_socket = -1;
	n->ssl_ctx = NULL;
    n->mqttread = xy_mqtt_read;
    n->mqttwrite = xy_mqtt_write;
}

	
int NetworkConnect(Network* n, char* addr, unsigned short port)
{
	int rc = 0;
	/* Create TCP socket */
	if ((n->my_socket = xy_socket_by_host(addr, Sock_IPv46, IPPROTO_TCP, 0, port, NULL)) == -1)
       rc = -1; 

	return rc;
}

void NetworkDisconnect(Network* n)
{
    close(n->my_socket);
}

