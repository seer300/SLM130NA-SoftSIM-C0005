/*******************************************************************************
 *
 * Copyright (c) 2019-2021 天翼物联科技有限公司
 *
 *******************************************************************************/

#ifndef __CTLW_ABSTRACT_OS_H_
#define __CTLW_ABSTRACT_OS_H_

#include "ctlw_config.h"


#ifdef __cplusplus
extern "C"
{
#endif

/********************************************************
*	   assert api
*
*********************************************************/


#ifdef PLATFORM_XYZ

//#define CTIOT_ASSERT_ENABLE
#ifdef CTIOT_ASSERT_ENABLE
#define CTIOT_ASSERT(x) configASSERT(x)
#else
#define CTIOT_ASSERT(x)
#endif

#endif

#ifdef PLATFORM_LINUX

#define CTIOT_ASSERT_ENABLE
#ifdef CTIOT_ASSERT_ENABLE
#define CTIOT_ASSERT(x)                                                                  \
	do                                                                                   \
	{                                                                                    \
		if (!(x))                                                                        \
		{                                                                                \
			heap_printf("CTIOT ASSERT:[%s->%s.c:%d]\r\n", __FILE__, __func__, __LINE__); \
			while (1)                                                                    \
			{                                                                            \
			};                                                                           \
		}                                                                                \
	} while (0);
#else
#define CTIOT_ASSERT(x)
#endif

#endif

/********************************************************
*	   log api
*
*********************************************************/
#ifdef PLATFORM_LINUX
	void lwm2m_printf(const char *format, ...);
#endif

#ifdef PLATFORM_XYZ

#include "debug_trace.h"
#ifdef FEATURE_RTT_ENABLED
#include "rtt_log.h"
#define lwm2m_printf(...)                                      \
	{                                                          \
		SEGGER_RTT_printf(LOG_TERMINAL_NORMAL, ##__VA_ARGS__); \
	}
#else
#define lwm2m_printf(...)
#endif

#endif

#ifdef PLATFORM_XINYI
#include "xy_utils.h"
#define lwm2m_printf(...)  xy_printf(0,XYAPP, WARN_LOG, __VA_ARGS__)
#endif
/********************************************************
*	   malloc and trace api
*
*********************************************************/
#ifndef CTIOT_HEAP_TRACE

#define ctlw_lwm2m_malloc(ulSize)   		XY_MALLOC(ulSize)
	//void *ctlw_lwm2m_malloc(size_t s);
	void ctlw_lwm2m_free(void *p);
	char *ctlw_lwm2m_strdup(const char *str);

#endif

#ifdef CTIOT_HEAP_TRACE

#include "ctlw_heap_trace.h"
#define ctlw_lwm2m_malloc(size) ctiot_malloc_trace(size, __FILE__, __FUNCTION__, __LINE__);
#define ctlw_lwm2m_free(pmem) ctiot_free_trace(pmem, __FILE__, __FUNCTION__, __LINE__);
#define ctlw_lwm2m_strdup(pmem) ctiot_strdup_trace(pmem, __FILE__, __FUNCTION__, __LINE__);

#endif

/********************************************************
*	   time api
*
*********************************************************/
#ifdef PLATFORM_LINUX
#include <sys/time.h>
#endif

#ifdef PLATFORM_XYZ
#include <time.h>
#endif

#ifdef PLATFORM_XINYI
#include <time.h>
#endif

	time_t ctlw_lwm2m_gettime(void);
	int ctlw_lwm2m_gettimeofday(void *tv, void *);

/********************************************************
*	   thread api
*
*********************************************************/
#ifdef PLATFORM_XYZ
#include "osasys.h"
#include "ps_event_callback.h"
#include "ps_lib_api.h"
#include "slpman_ec616.h"
#ifdef FEATURE_AT_ENABLE
#include "at_def.h"
#endif
#include "cmisim.h"

	typedef osThreadId_t thread_handle_t;
	typedef osThreadAttr_t thread_attr_t;
	typedef osMutexId_t  thread_mutex_t;
	typedef osMessageQueueId_t thread_msg_queue_handle_t;
	void ctlw_usleep(uint32_t usec);
#endif

#ifdef PLATFORM_LINUX
#include <pthread.h>
#include <unistd.h> //read,write,ctlw_usleep
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

	//#define THREAD_MUTEX_INITIALIZER  PTHREAD_MUTEX_INITIALIZER
	typedef pthread_t thread_handle_t;
	typedef pthread_attr_t thread_attr_t;
	typedef pthread_mutex_t thread_mutex_t;
	typedef int thread_msg_queue_handle_t;
#endif

#ifdef PLATFORM_XINYI
#include "cmsis_os2.h"
typedef osThreadId_t thread_handle_t;
typedef osThreadAttr_t thread_attr_t;
typedef osMutexId_t  thread_mutex_t;
typedef osMessageQueueId_t thread_msg_queue_handle_t;
void ctlw_usleep(uint32_t usec);
#endif

typedef void *(*PTHREAD_ROUTINE)(void *);

	int thread_create(thread_handle_t *thread, const thread_attr_t *attr, void *(*start_routine)(void *), void *arg);

	int thread_exit(void *retval);
	int thread_cancel(thread_handle_t thread_id);
	int thread_join(thread_handle_t thread_id, void **retval);
	int thread_mutex_init(thread_mutex_t *mutex, const char *name);
	int thread_mutex_lock(thread_mutex_t *mutex);
	int thread_mutex_unlock(thread_mutex_t *mutex);
	int thread_mutex_destroy(thread_mutex_t *mutex);


#ifdef __cplusplus
}
#endif

#endif //PLATFORM_H_
