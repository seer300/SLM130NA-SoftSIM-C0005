/*******************************************************************************
 *
 * Copyright (c) 2019-2021 天翼物联科技有限公司. All rights reserved.
 *
 *******************************************************************************/
#include "ctlw_abstract_os.h"
#ifdef PLATFORM_XINYI
#include "ctwing_util.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "ctlw_liblwm2m.h"
#include "ctlw_internals.h"
#include "ctlw_lwm2m_sdk.h"
#include "ctlw_heap.h"

#ifdef PLATFORM_LINUX
//使用 ifconf结构体和ioctl函数时需要用到该头文件
#include <net/if.h>
#include <sys/ioctl.h>
//消息队列需要用到的头
#include <sys/ipc.h>
#include <sys/types.h>
//使用ifaddrs结构体时需要用到该头文件
#include <ifaddrs.h>
#include "chip_info.h"
#endif

#ifdef PLATFORM_XYZ
#include "ps_lib_api.h"
#endif


//---------性能测试----------------

//---------End性能测试----------------

/********************************************************
*	   log
*
*********************************************************/
#ifdef PLATFORM_LINUX
uint8_t buffer[512];
void lwm2m_printf(const char *format, ...)
{
#ifdef WITH_CTIOT_LOGS
	va_list ap;
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
#endif
}
#endif

#ifndef CTIOT_HEAP_TRACE
/********************************************************
*	   heap api
*
*********************************************************/


/*void *ctlw_lwm2m_malloc(size_t s)
{
	void *ptr = NULL;
	ptr = xy_malloc(s);//pCtiotMalloc(s);
	return ptr;
}*/

void ctlw_lwm2m_free(void *p)
{
	if (p == NULL)
	{
		return;
	}
	xy_free(p);
}

/********************************************************
*	   str lib api
*
*********************************************************/
char *ctlw_lwm2m_strdup(const char *str)
{
	char *dstr = NULL;
	size_t len;

	if (str == NULL)
		return NULL;

	len = strlen(str);
	dstr = (char *)ctlw_lwm2m_malloc(len + 1);

	if (dstr == NULL)
		return NULL;

	memcpy(dstr, str, len);
	dstr[len] = '\0';

	return dstr;
}

#endif

int ctlw_lwm2m_strncmp(const char *s1, const char *s2, size_t n)
{
	return strncmp(s1, s2, n);
}

/********************************************************
*	   time api
*
*********************************************************/

time_t ctlw_lwm2m_gettime(void)
{
#ifdef PLATFORM_XYZ
	return xTaskGetTickCount() / osKernelGetTickFreq()+1;//计时器应大于0
#endif
#ifdef PLATFORM_LINUX
	struct timeval tv;
	if (0 != gettimeofday(&tv, NULL))
	{
		return -1;
	}
	return tv.tv_sec;
#endif

#ifdef PLATFORM_XINYI
	return xy_ctlw_gettime();
#endif
}


int ctlw_lwm2m_gettimeofday(void *tv, void *tz)
{
	if (tv == NULL)
	{
		return -1;
	}
#ifdef PLATFORM_XYZ
	uint64_t ms;
	struct timeval *p;
	p = (struct timeval *)tv;
	ms = xTaskGetTickCount() * 1000 / osKernelGetTickFreq();
	p->tv_sec = ms / 1000;
	p->tv_usec = ms * 1000;
	return 0;
#endif
#ifdef PLATFORM_LINUX
	return gettimeofday((struct timeval *)tv, tz);
#endif

#ifdef PLATFORM_XINYI
	uint64_t ms;
	struct timeval *p;

	p = (struct timeval *)tv;
	p->tv_sec = ctlw_lwm2m_gettime();
	p->tv_usec = 0;
	return 0;
#endif
}

#ifdef PLATFORM_XYZ
void ctlw_usleep(uint32_t usec)
{
	uint32_t i = 1000;
	uint32_t ms, tick;

	if (i < 1000)
	{
		while (i--)
			;
	}
	else
	{
		ms = usec / 1000;
		tick = ms * osKernelGetTickFreq() / 1000;
		vTaskDelay(tick);
	}
}
#endif

#ifdef PLATFORM_XINYI
void ctlw_usleep(uint32_t usec)//us
{
	xy_ctlw_usleep(usec);
}

#endif

/********************************************************
*     rtos api
*
*********************************************************/
int thread_create(thread_handle_t *thread, const thread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{
#ifdef PLATFORM_XYZ
	thread_attr_t task_init_param;
	memset(&task_init_param, 0, sizeof(thread_attr_t));
	task_init_param.priority = osPriorityBelowNormal7;
	task_init_param.name = "lwm2m_coap_task";

	if (attr == NULL)
	{
		*thread = osThreadNew((osThreadFunc_t)start_routine, arg, &task_init_param);
	}
	else
	{
		*thread = osThreadNew((osThreadFunc_t)start_routine, arg, attr);
	}

	if (*thread != NULL)
		return 0;
	else
		return -1;
#endif

#ifdef PLATFORM_LINUX
	int result=pthread_create(thread, attr, start_routine, arg);
	ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"thread create result:%d\r\n",result);
	return result;
#endif
	
#ifdef PLATFORM_XINYI
	int result = xy_ctlw_thread_create(thread, attr, start_routine, arg);
	return result;
#endif
}
int thread_exit(void *retval)
{
#ifdef PLATFORM_XYZ
	vTaskDelete(NULL);
	return 0;
#endif

#ifdef PLATFORM_LINUX
	pthread_exit(NULL);
	return 0;
#endif

#ifdef PLATFORM_XINYI
	return xy_ctlw_thread_exit();
#endif
}

int thread_cancel(thread_handle_t thread_id)
{
#ifdef PLATFORM_XYZ
	vTaskDelete(thread_id);
	return 0;
#endif

#ifdef PLATFORM_LINUX
	pthread_cancel(thread_id);
	return 0;
#endif

#ifdef PLATFORM_XINYI
	return xy_ctlw_thread_cancel(thread_id);
#endif
}

int thread_join(thread_handle_t thread_id, void **retval)
{
#ifdef PLATFORM_XYZ
	return 0;
#endif

#ifdef PLATFORM_LINUX
	pthread_join(thread_id, retval);
	return 0;
#endif

#ifdef PLATFORM_XINYI
	return 0;
#endif
}

int thread_mutex_init(thread_mutex_t *mutex, const char *name)
{
#ifdef PLATFORM_XYZ
	*mutex = osMutexNew(NULL);
	return 0;
#endif

#ifdef PLATFORM_LINUX
	return pthread_mutex_init(mutex, NULL);
#endif

#ifdef PLATFORM_XINYI
	return xy_ctlw_mutex_init(mutex);
#endif
}

int thread_mutex_lock(thread_mutex_t *mutex)
{
#ifdef PLATFORM_XYZ
	return osMutexAcquire(*mutex, 0xffffffffUL); //block
#endif

#ifdef PLATFORM_LINUX
	return pthread_mutex_lock(mutex); //block
#endif

#ifdef PLATFORM_XINYI
	return xy_ctlw_mutex_lock(mutex);
#endif
}

int thread_mutex_unlock(thread_mutex_t *mutex)
{
#ifdef PLATFORM_XYZ
	return osMutexRelease(*mutex);
#endif

#ifdef PLATFORM_LINUX
	return pthread_mutex_unlock(mutex);
#endif

#ifdef PLATFORM_XINYI
	return xy_ctlw_mutex_unlock(mutex);
#endif
}

int thread_mutex_destroy(thread_mutex_t *mutex)
{
#ifdef PLATFORM_XYZ
	return osMutexDelete(*mutex);
#endif

#ifdef PLATFORM_LINUX
	return pthread_mutex_destroy(mutex);
#endif

#ifdef PLATFORM_XINYI
	return xy_ctlw_mutex_destroy(mutex);
#endif
}

//---------芯片性能测试----------------
//---------芯片性能测试----------------

uint16_t ctiot_get_random(void)
{
#ifdef PLATFORM_XYZ
	extern int32_t RngGenRandom(uint8_t Rand[24]);
	uint8_t Rand[24];
	uint32_t a = 0;
	uint32_t b = 0;
	uint8_t i = 0;
	RngGenRandom(Rand);
	for (i = 0; i < 24; i++)
	{
		a += Rand[i];
		b ^= Rand[i];
	}
	return (a << 8) + b;
#endif
#ifdef PLATFORM_LINUX
	srand((int)ctlw_lwm2m_gettime());
	return rand();
#endif
#ifdef PLATFORM_XINYI
	return xy_ctlw_random();
#endif
}
