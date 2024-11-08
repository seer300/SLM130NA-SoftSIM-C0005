#ifndef CTIOT_LOG_H
#define CTIOT_LOG_H
#include <stdio.h>


#include "ctlw_config.h"

#define LOG_PREINIT_MODULE "preint"
#define LOG_INIT_MODULE "init"
#define LOG_AT_MODULE "at"
#define LOG_SEND_RECV_MODULE "sendrecv"
#define LOG_COMMON_MODULE "common"


#define LOG_VOTE_CLASS "vote"
#define LOG_IP_CLASS "IP"
#define LOG_SOCKET_CLASS "socket"
#define LOG_DTLS_CLASS "DTLS"
#define LOG_SESSTATUS_CLASS "sesstatus"
#define LOG_OTHER_CLASS "other"

#ifdef PLATFORM_XINYI
#include "xy_utils.h"
int xy_ctiot_log_info(uint8_t log_level, const char * module, const char *class, const char* function, int32_t line, const char *fmt,...);
#endif
typedef enum
{
    LOG_DEBUG = 1,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERR,
    LOG_FATAL,

    LOG_MAX
} ctiot_log_e;

void ctiot_set_log_level(ctiot_log_e level);

ctiot_log_e ctiot_get_log_level(void);

#define CTIOT_DEBUG
#ifdef CTIOT_DEBUG
const char* ctiot_get_log_level_name(ctiot_log_e log_level);
#define CTIOT_LOG1(level, fmt, ...) \
    do \
    { \
        if ((level) >= ctiot_get_log_level()) \
        { \
           	(void)printf("[%s][%s][%s:%d] " fmt "\r\n" , \
           		ctiot_get_log_level_name((level)), __TIME__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        } \
    } while (0)

#define CTIOT_NEW_LOG
#ifdef CTIOT_NEW_LOG
#ifdef PLATFORM_XINYI
#define CTIOT_LOG(level, module, class, fmt, ...) \
    do \
    { \
        if ((level) >= ctiot_get_log_level()) \
        { \
           	(void)xy_printf(0,XYAPP, WARN_LOG, "[%s][%s][%s:%d] " fmt "\r\n" , \
           		ctiot_get_log_level_name((level)), __TIME__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        } \
    } while (0)

#else
#define CTIOT_LOG(level, module, class, fmt, ...) \
do \
{ \
    if ((level) >= ctiot_get_log_level()) \
    { \
       	(void)printf("[%s][%u][%s][%s:%d]@@@%s@@@ " fmt "\r\n" , \
       		ctiot_get_log_level_name((level)), ctlw_lwm2m_gettime(), module, __FUNCTION__, __LINE__, class, ##__VA_ARGS__); \
    } \
} while (0)

#endif
#endif
#else
#define CTIOT_LOG(level, fmt, ...)
#endif

#define CTIOT_LOG_BINARY(level,module,class,refmsg,msgBuffer,length) \
do \
{ \
    if ((level) >= ctiot_get_log_level()) \
    { \
       	(void)printf("[%s][%u][%s][%s:%d]@@@%s@@@ " refmsg "\r\n" , \
       		ctiot_get_log_level_name((level)), ctlw_lwm2m_gettime(), module, __FUNCTION__, __LINE__, class); \
       	for (int i = 1; i <= length; i++) \
       	{ \
			printf("%02x ", msgBuffer[i]); \
			if(i % 20 == 0 && i > 0) printf("\r\n"); \
       	} \
		printf("\r\n"); \
    } \
} while (0)

#endif
