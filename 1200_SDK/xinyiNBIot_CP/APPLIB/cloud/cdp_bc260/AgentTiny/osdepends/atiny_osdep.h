/*----------------------------------------------------------------------------
 * Copyright (c) <2018>, <Huawei Technologies Co., Ltd>
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

/**@defgroup atiny_osdep Agenttiny OS Depends
 * @ingroup agent
 */

#ifndef _ATINY_OSDEP_H_
#define _ATINY_OSDEP_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include "xy_utils.h"
#include <time.h>

#define ATINY_INLINE static inline


/**
 *@ingroup atiny_adapter
 *@brief get time since the system was boot.
 *
 *@par Description:
 *This API is used to retrieves the number of milliseconds that have elapsed since the system was boot.
 *@attention none.
 *
 *@param none.
 *
 *@retval #uint64_t     the number of milliseconds.
 *@par Dependency: none.
 *@see atiny_usleep
 */
uint64_t atiny_gettime_ms(void);
time_t atiny_gettime_s(time_t * p);

/**
 *@ingroup atiny_adapter
 *@brief sleep thread itself.
 *
 *@par Description:
 *This API is used to sleep thread itself, it could be implemented in the user file.
 *@attention none.
 *
 *@param usec           [IN] the time interval for which execution is to be suspended, in microseconds.
 *
 *@retval none.
 *@par Dependency: none.
 *@see atiny_gettime_ms
 */
void atiny_usleep(unsigned long usec);


/**
 *@ingroup atiny_adapter
 *@brief allocates a block of size bytes of memory
 *
 *@par Description:
 *This API is used to allocates a block of size bytes of memory, returning a pointer to the beginning of the block.
 *@attention none.
 *
 *@param size           [IN] specify block size in bytes.
 *
 *@retval #pointer      pointer to the beginning of the block.
 *@par Dependency: none.
 *@see atiny_free
 */
void* atiny_malloc(size_t size);


void* atiny_calloc(size_t n, size_t size);

/**
 *@ingroup atiny_adapter
 *@brief deallocate memory block.
 *
 *@par Description:
 *This API is used to deallocate memory block.
 *@attention none.
 *
 *@param size           [IN] pointer to the beginning of the block previously allocated with atiny_malloc.
 *
 *@retval none.
 *@par Dependency: none.
 *@see atiny_malloc
 */
void atiny_free(void* ptr);

/**
 *@ingroup atiny_adapter
 *@brief writes formatted data to string.
 *
 *@par Description:
 *This API is used to writes formatted data to string.
 *@attention none.
 *
 *@param buf            [OUT] string that holds written text.
 *@param size           [IN] maximum length of character will be written.
 *@param format         [IN] format that contains the text to be written, it can optionally contain embedded
                             format specifiers that specifies how subsequent arguments are converted for output.
 *@param ...            [IN] the variable argument list, for formatted and inserted in the resulting string
                             replacing their respective specifiers.
 *
 *@retval #int          bytes of character successfully written into string.
 *@par Dependency: none.
 *@see none.
 */
int atiny_snprintf(char* buf, unsigned int size, const char* format, ...);

/**
 *@ingroup atiny_adapter
 *@brief writes formatted data to stream.
 *
 *@par Description:
 *This API is used to writes formatted data to stream.
 *@attention none.
 *
 *@param format         [IN] string that contains the text to be written, it can optionally contain embedded
                             format specifiers that specifies how subsequent arguments are converted for output.
 *@param ...            [IN] the variable argument list, for formatted and inserted in the resulting string
                             replacing their respective specifiers.
 *
 *@retval #int          bytes of character successfully written into stream.
 *@par Dependency: none.
 *@see none.
 */
int atiny_printf(const char* format, ...);

/**
 *@ingroup atiny_strdup
 *@brief returns a pointer to a new string which is a duplicate of the string ch
 *
 *@par Description:
 *This API returns a pointer to a new string which is a duplicate of the string ch. 
   Memory for the new string is obtained with atiny_malloc, and can be freed with atiny_free
 *@attention none.
 *
 *@param format         [IN] const char pointer
 *@param ch            [IN] const char pointer points to the string to be duplicated
 *
 *@retval #char pointer          a pointer to a new string which is a duplicate of the string ch
 *@par Dependency: none.
 *@see atiny_malloc atiny_free.
 */
char *atiny_strdup(const char *ch);

/**
 *@ingroup atiny_adapter
 *@brief create a mutex.
 *
 *@par Description:
 *This API is used to create a mutex.
 *@attention none.
 *
 *@param none.
 *
 *@retval #pointer      the mutex handle.
 *@retval NULL          create mutex failed.
 *@par Dependency: none.
 *@see atiny_mutex_destroy | atiny_mutex_lock | atiny_mutex_unlock
 */
long atiny_mutex_create(osMutexId_t *pMutexId);

/**
 *@ingroup atiny_adapter
 *@brief destroy the specified mutex object.
 *
 *@par Description:
 *This API is used to destroy the specified mutex object, it will release related resource.
 *@attention none.
 *
 *@param mutex          [IN] the specified mutex.
 *
 *@retval none.
 *@par Dependency: none.
 *@see atiny_mutex_create | atiny_mutex_lock | atiny_mutex_unlock
 */
long atiny_mutex_destroy(osMutexId_t *pMutexId);

/**
 *@ingroup atiny_adapter
 *@brief waits until the specified mutex is in the signaled state.
 *
 *@par Description:
 *This API is used to waits until the specified mutex is in the signaled state.
 *@attention none.
 *
 *@param mutex          [IN] the specified mutex.
 *
 *@retval none.
 *@par Dependency: none.
 *@see atiny_mutex_create | atiny_mutex_destroy | atiny_mutex_unlock
 */
long atiny_mutex_lock(osMutexId_t *pMutexId);

/**
 *@ingroup atiny_adapter
 *@brief releases ownership of the specified mutex object.
 *
 *@par Description:
 *This API is used to releases ownership of the specified mutex object.
 *@attention none.
 *
 *@param mutex          [IN] the specified mutex.
 *
 *@retval none.
 *@par Dependency: none.
 *@see atiny_mutex_create | atiny_mutex_destroy | atiny_mutex_lock
 */
long atiny_mutex_unlock(osMutexId_t *pMutexId);

/**
 *@ingroup atiny_adapter
 *@brief reboot.
 *
 *@par Description:
 *This API is used to reboot, it could be implemented in the user file.
 *@attention none.
 *
 *@retval none.
 *@par Dependency: none.
 *@see none.
 */
void atiny_reboot(void);

/**
 *@ingroup atiny_adapter
 *@brief task delay.
 *
 *@par Description:
 *This API is used to delay some seconds.
 *@attention none.
 *
 *@param second          [IN] the specified second.
 *
 *@retval none.
 *@par Dependency: none.
 *@see none.
 */
void atiny_delay(uint32_t second);


/**

 *@ingroup atiny_list

 *Structure of a node in a doubly linked list.

 */

typedef struct atiny_dl_list
{
    struct atiny_dl_list *prev;            /**< Current node's pointer to the previous node*/
    struct atiny_dl_list *next;            /**< Current node's pointer to the next node*/

} atiny_dl_list;


ATINY_INLINE void atiny_list_init(atiny_dl_list *list)
{
    list->next = list;
    list->prev = list;
}

#define ATINY_DL_LIST_FIRST(object) ((object)->next)

ATINY_INLINE void atiny_list_add(atiny_dl_list *list, atiny_dl_list *node)
{
    node->next = list->next;
    node->prev = list;
    list->next->prev = node;
    list->next = node;
}

ATINY_INLINE void atiny_list_insert_tail(atiny_dl_list *list, atiny_dl_list *node)
{
    atiny_list_add(list->prev, node);
}

ATINY_INLINE atiny_dl_list * atiny_list_get_head(atiny_dl_list *header)
{
    return header->next;
}

ATINY_INLINE void atiny_list_delete(atiny_dl_list *node)
{
    node->next->prev = node->prev;
    node->prev->next = node->next;
    node->next = (atiny_dl_list *)NULL;
    node->prev = (atiny_dl_list *)NULL;
}

ATINY_INLINE int atiny_list_empty(atiny_dl_list *node)
{
    return (node->next == node);
}

#define ATINY_OFFSET_OF_FIELD(type, field)    ((uint32_t)&(((type *)0)->field))
#define ATINY_OFF_SET_OF(type, member) ((long)&((type *)0)->member)   /*lint -e(413) */
#define ATINY_FIELD_TO_STRUCT(field_addr, type, member) \
    ((type *)((char *)(field_addr) - ATINY_OFF_SET_OF(type, member)))

#define ATINY_DL_LIST_ENTRY(item, type, member)\
    ((type *)((char *)item - ATINY_OFF_SET_OF(type, member)))\


#define ATINY_DL_LIST_FOR_EACH_ENTRY(item, list, type, member)\
    for (item = ATINY_DL_LIST_ENTRY((list)->next, type, member);\
        &item->member != (list);\
        item = ATINY_DL_LIST_ENTRY(item->member.next, type, member))


#define ATINY_DL_LIST_FOR_EACH_ENTRY_SAFE(item, next, list, type, member)\
    for (item = ATINY_DL_LIST_ENTRY((list)->next, type, member),\
        next = ATINY_DL_LIST_ENTRY(item->member->next, type, member);\
        &item->member != (list);\
        item = next, item = ATINY_DL_LIST_ENTRY(item->member.next, type, member))

ATINY_INLINE void ATINY_ListDel(atiny_dl_list *pstPrevNode, atiny_dl_list *pstNextNode)
{
    pstNextNode->prev = pstPrevNode;
    pstPrevNode->next = pstNextNode;
}

ATINY_INLINE void ATINY_ListDelInit(atiny_dl_list *pstList)
{
    ATINY_ListDel(pstList->prev, pstList->next);
    atiny_list_init(pstList);
}

#define ATINY_DL_LIST_FOR_EACH(item, list)\
    for ((item) = (list)->next;\
        (item) != (list);\
        (item) = (item)->next)

#define ATINY_DL_LIST_FOR_EACH_SAFE(item, next, list)\
    for (item = (list)->next, next = item->next; item != (list);\
        item = next, next = item->next)

#define ATINY_DL_LIST_HEAD(list)\
            atiny_dl_list list = { &(list), &(list) }



#if defined(__cplusplus)
}
#endif

#endif

