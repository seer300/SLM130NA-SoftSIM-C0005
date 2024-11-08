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
#include "cloud_utils.h"
#include "xy_system.h"
#include "atiny_osdep.h"
#include <stdbool.h>
#include "xy_rtc_api.h"
#include "liblwm2m.h"

#define ATINY_CNT_MAX_WAITTIME 0xFFFFFFFF
#define LOG_BUF_SIZE (64)

#ifndef OK
#define OK 0
#endif

#ifndef ERR
#define ERR -1
#endif

//#define configTICK_RATE_HZ 1000

uint64_t atiny_gettime_ms(void)
{
    return cloud_gettime_ms();
}

time_t atiny_gettime_s(time_t *p)
{
	(void) p;

    uint32_t t = cloud_gettime_s();
    xy_printf(0,XYAPP, WARN_LOG, "[king][atiny_gettime_s]%d", t);
    return t;
}



void *atiny_malloc(size_t size)
{
    return (void *)xy_malloc(size);
}

void *atiny_calloc(size_t n, size_t size)
{
    void *p = atiny_malloc(n * size);
    if(p)
    {
        memset(p, 0, n * size);
    }

    return p;
}


void atiny_free(void *ptr)
{
    if(ptr == NULL)
        return;
    
    (void)xy_free(ptr);
}

int atiny_snprintf(char *buf, unsigned int size, const char *format, ...)
{
    int     ret;
    va_list args;

    va_start(args, format);
    ret = vsnprintf(buf, size, format, args);
    va_end(args);

    return ret;
}


int atiny_printf(const char *format, ...)
{

#ifdef LWM2M_WITH_LOGS
    int ret;
    char str_buf[LOG_BUF_SIZE] = {0};
    va_list list;

    memset(str_buf, 0, LOG_BUF_SIZE);
    va_start(list, format);
    ret = vsnprintf(str_buf, LOG_BUF_SIZE, format, list);
    va_end(list);

    //printf("%s", str_buf);

    xy_printf(0,XYAPP, WARN_LOG, "%s", str_buf);
#endif

    return 0;
}


char *atiny_strdup(const char *ch)
{
    char *copy;
    size_t length;

    if(NULL == ch)
        return NULL;

    length = strlen(ch);
    copy = (char *)atiny_malloc(length + 1);
    if(NULL == copy)
        return NULL;
    strncpy(copy, ch, length);
    copy[length] = '\0';

    return copy;
}


void atiny_reboot(void)
{
	xy_Soft_Reset(SOFT_RB_BY_FOTA);
}

void atiny_delay(uint32_t second)
{
    (void)osDelay(second*1000UL);;
	
}
void atiny_usleep(unsigned long usec)
{
    (void)osDelay(usec/1000UL);;
	
}


#ifndef LWM2M_MEMORY_TRACE
/*
void *lwm2m_malloc(size_t s)
{
    void *mem = NULL;
    mem = atiny_malloc(s);
    return mem;
}
*/
void lwm2m_free(void *p)
{
    if(NULL != p)
        atiny_free(p);
    p = NULL;
}


char *lwm2m_strdup(const char *str)
{
    int len = strlen(str) + 1;
    void *new = (void*)lwm2m_malloc(len);
    if (new == NULL)
        return NULL;
    return (char *)memcpy(new, str, len);

}

#endif

int lwm2m_strncmp(const char *s1,
                  const char *s2,
                  size_t n)
{
    return strncmp(s1, s2, n);
}

time_t lwm2m_gettime(void)
{
    return cloud_gettime_s();
}

int lwm2m_rand()
{
    return xy_rand();
}

void lwm2m_delay(uint32_t second)
{
    atiny_delay(second);
}






#endif
