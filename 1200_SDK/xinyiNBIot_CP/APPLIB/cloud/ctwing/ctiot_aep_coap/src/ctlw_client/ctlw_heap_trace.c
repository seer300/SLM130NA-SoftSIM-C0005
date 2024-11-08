
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "ctlw_heap.h"
#include "ctlw_abstract_os.h"
#include "ctlw_platform.h"
#include "ctlw_heap_trace.h"
#ifdef PLATFORM_LINUX
#include <pthread.h>
#include <unistd.h> //read,write,ctlw_usleep
#endif
#ifdef PLATFORM_XYZ
#include "osasys.h"
#endif
static thread_mutex_t mutexLocal;



/*
if cpu word is 32bit，the sizeof malloc_list is 28 bytes;
if cpu word is 64bit, the sizeof malloc_list is 48 bytes.
*/
#pragma pack(1)
typedef struct malloc_list
{
	struct malloc_list *next;
	const char *file;
	const char *function;
	int lineno;
	size_t size;
	int time;
	uint8_t *data;
} malloc_list_t;
#pragma pack()

static malloc_list_t malloc_list = {
	.next = NULL,
};

static uint8_t trace_lock_init_flag = 0;
/***********************************

malloc_list ---->
                 O  O  O  O
          p ---->

***********************************/
static int malloc_list_add(malloc_list_t *list)
{
	malloc_list_t *p = &malloc_list;

	if (list == NULL)
		return -1;

	list->next = p->next;
	p->next = list;

	return 0;
}
//o o o o
static malloc_list_t *malloc_list_find(void *memory)
{
	malloc_list_t *plist = &malloc_list;
	malloc_list_t *retlist;

	if (memory == NULL)
	{
		return NULL;
	}

	while (plist->next)
	{
		if (plist->next->data == memory)
		{
			retlist = plist->next;
			plist->next = plist->next->next;

			retlist->next = NULL;
			return retlist;
		}
		plist = plist->next;
	}

	return NULL;
}

void ctiot_print_trace(void)
{
	malloc_list_t *plist = malloc_list.next;
	#ifndef PLATFORM_XYZ
	lwm2m_printf("*************************************************************************\r\n");
	lwm2m_printf("                    malloc list information\r\n");
	lwm2m_printf("*************************************************************************\r\n");
	#endif


	thread_mutex_lock(&mutexLocal);


	while (plist)
	{
		#ifndef PLATFORM_XYZ
		lwm2m_printf("[%s.c->%s:%u]\r\n", plist->file, plist->function, plist->lineno);
		lwm2m_printf("size:%u\n", plist->size);
		lwm2m_printf("address:%p\n", plist);
		lwm2m_printf("time:%u\n", ctlw_lwm2m_gettime() - plist->time);
		#else
		//uint8_t at_str[150]={0};
		//sprintf((char *)at_str,"file:%s,function:%s,line:%u", plist->file, plist->function, plist->lineno);
		#endif
		plist = plist->next;
	}
	thread_mutex_unlock(&mutexLocal);
	#ifndef PLATFORM_XYZ
	lwm2m_printf("********************malloc list information end************************\r\n");
	#endif
}

void *ctiot_malloc_trace(size_t size, const char *file, const char *function, uint32_t lineno)
{
#ifdef PLATFORM_XINYI
	if(size == 0)
		return NULL;
#endif
	int ret;
	#ifndef PLATFORM_XYZ
	lwm2m_printf("malloc trace:[%s.c->%s:%d]\n", file, function, lineno);
	lwm2m_printf("user real want:%d\r\n", size);
	lwm2m_printf("trace list size:%d", sizeof(malloc_list_t));
	#endif

	malloc_list_t *entry = (malloc_list_t *)malloc(size + sizeof(malloc_list_t));

	if (entry == NULL)
	{
		//lwm2m_printf("malloc fail\r\n");
		return NULL;
	}

	if (trace_lock_init_flag == 0)
	{
		trace_lock_init_flag = 1;
		/* add thread mutex */
		thread_mutex_init(&mutexLocal, "heap_trace");
	}

	entry->file = file;
	entry->function = function;
	entry->lineno = lineno;
	entry->size = size;
	entry->time = ctlw_lwm2m_gettime();
	entry->data = (uint8_t *)((uint8_t *)entry + sizeof(malloc_list_t));
	entry->next = NULL;

	thread_mutex_lock(&mutexLocal);
	ret = malloc_list_add(entry);
	thread_mutex_unlock(&mutexLocal);

	if (ret != 0)
	{
		#ifndef PLATFORM_XYZ
		lwm2m_printf("add malloclist fail\n");
		#else
		#endif
		while (1)
		{
		}; //stop
	}

	#ifndef PLATFORM_XYZ
	lwm2m_printf("malloc pointer: %p\r\n", entry);
	#endif
	return (void *)(entry->data);
}

void ctiot_free_trace(void *p, const char *file, const char *function, uint32_t lineno)
{
	malloc_list_t *plist;
	#ifndef PLATFORM_XYZ
	lwm2m_printf("free trace:[%s.c->%s:%d]\n", file, function, lineno);
	#endif
	if (p == NULL)
	{
		return;
	}

	thread_mutex_lock(&mutexLocal);
	plist = malloc_list_find(p);
	thread_mutex_unlock(&mutexLocal);
	if (plist == NULL)
	{
		//lwm2m_printf("find malloclist fail\n");
		#ifdef PLATFORM_XYZ
		return ;
		#else
		while (1)
		{

		}; //stop
		#endif
	}

	#ifndef PLATFORM_XYZ
	lwm2m_printf("free pointer: %p\r\n", plist);
	#endif
	free(plist);
}

char *ctiot_strdup_trace(const char *str, const char *file, const char *function, uint32_t lineno)
{
	char *dstr = NULL;
	size_t len;

	if (str == NULL)
		return NULL;

	len = strlen(str);

	dstr = (char *)ctiot_malloc_trace(len + 1, file, function, lineno);

	if (dstr == NULL)
		return NULL;

	memcpy(dstr, str, len);
	dstr[len] = '\0';

	return dstr;
}
