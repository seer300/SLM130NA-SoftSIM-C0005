/**
  ******************************************************************************
  * @file    xy_mem.c
  * @brief   This file contains memory function prototype.
  ******************************************************************************
  * @attention
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at

  * http://www.apache.org/licenses/LICENSE-2.0

  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "sys_mem.h"
#include "xy_memmap.h"
#include "hw_types.h"
#include "sys_ipc.h"
#include "xy_cp.h"
#include "xy_flash.h"
#include "xy_utils.h"


/*****************************************************************************
Function         :      free
Description      :      Deallocates the memory previously allocated by a call to calloc, malloc, or
				realloc. The argument ptr points to the space that was previously allocated.
				If ptr points to a memory block that was not allocated with calloc, malloc,
				or realloc, or is a space that has been deallocated, then the result is
				undefined.
Input            :      [1] void *ptr, pointed to the memory need to free.
Output           :      nothing.
Return           :      No value is returned.
*****************************************************************************/
void free(void *ptr)
{
	if (ptr == NULL)
		return;

	xy_free_r(ptr); /*lint !e534*/
}

void *malloc(size_t size) /*lint !e31 !e10*/
{
	void *ptr = NULL; /*lint !e64 !e10*/

	if (size == 0)
		return NULL; /*lint !e64*/

	ptr = pvPortMalloc((uint32_t)size);

	return ptr;
}

void *zalloc(size_t size) /*lint !e10*/
{
	void *ptr = malloc(size);

	if (ptr != NULL)
	{
		memset((void *)ptr, (int)0, size);
	}

	return ptr;
}

void *calloc(size_t nitems, size_t size) /*lint !e578*/
{
	return zalloc(nitems * size);
}

void *realloc(void *ptr, size_t size)
{
	if (ptr == NULL)
	{
		return malloc(size); /*lint !e64*/
	}

	if (size == 0)
	{
		free(ptr);
		return NULL;
	}
	xy_assert(0);
	//return pvPortRealloc((void *)ptr, (uint32_t)size);
    return NULL;
}

void *_malloc_r(struct _reent *ptr, size_t size)
{
	(void)ptr;
	return malloc(size);
}

void *_realloc_r(struct _reent *ptr, void *old, size_t newlen)
{
	(void)ptr;
	return realloc(old, newlen);
}

void *_calloc_r(struct _reent *ptr, size_t size, size_t len)
{
	(void)ptr;
	return calloc(size, len);
}

void _free_r(struct _reent *ptr, void *addr)
{
	(void)ptr;
	free(addr);
}



/*工作态出厂NV使用FTL磨损算法保存，若严格检查运算量大，为了提速，此处简化有效性检查步骤*/
__FLASH_FUNC uint32_t GET_FAC_FLASH_BASE()
{
	/*CP核一旦运行，FLASH主控变为CP核，进而AP核不准直接访问FLASH，否则内容可能异常*/
#if XY_DUMP
	xy_assert(CP_Is_Alive() == false);
#endif	
	return (((*((uint32_t *)(NV_FLASH_FACTORY_BASE))==0x5A5A5A5A)&&(*((uint32_t *)(NV_FLASH_FACTORY_BASE+NV_FLASH_FACTORY_LEN-8))!=0XFFFFFFFF))? (NV_FLASH_FACTORY_BASE + 4) : (NV_FLASH_MAIN_FACTORY_BASE));
}

__FLASH_FUNC void fac_working_nv_check() 
{
	uint8_t *worknv_tmp_mem = (void*)0x20035000; //堆还没初始化，先用下cp内存
	
	xy_Flash_Read(NV_FLASH_FACTORY_BASE, worknv_tmp_mem, NV_FLASH_FACTORY_LEN);
	
	if((*((uint32_t *)(NV_FLASH_FACTORY_BASE))==0x5A5A5A5A) && (*((uint32_t *)(NV_FLASH_FACTORY_BASE+NV_FLASH_FACTORY_LEN - 8)) != xy_chksum((void*)worknv_tmp_mem, NV_FLASH_FACTORY_LEN - 8)))
	{
#if XY_DUMP
		xy_assert(0);
#endif
		extern void flash_erase(uint32_t addr, uint32_t size);
		flash_erase(NV_FLASH_FACTORY_BASE, NV_FLASH_FACTORY_LEN);
	}
}