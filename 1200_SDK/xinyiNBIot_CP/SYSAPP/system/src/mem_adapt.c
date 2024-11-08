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

#include "mem_adapt.h"
#include "xy_utils.h"
#include "xy_memmap.h"
#include "hw_memmap.h"
#include "task.h"
#include "cmsis_os2.h"


uint32_t g_ps_mem_used;
uint32_t g_ps_mem_used_max;

uint32_t g_xy_mem_alloc_loose_cnt = 0;
uint32_t g_xy_mem_alloc_align_loose_cnt = 0;

uint32_t gMallocFailNum = 0;

#if( XY_HEAP_MEMORY_MALLOC_WITH_RECORD )
void* xy_MemAllocDebug(size_t size, char *file, int line, uint8_t loose, uint8_t clean)
#else
void* xy_MemAlloc(size_t size, uint8_t loose, uint8_t clean)
#endif
{
	if(size > 0x1000 && osKernelGetState() >= osKernelRunning && osKernelIsRunningIdle()!= osOK && HWREGB(BAK_MEM_XY_DUMP) == 1)
	{
#if( XY_HEAP_MEMORY_MALLOC_WITH_RECORD )
		PrintLog(0,PLATFORM,WARN_LOG,"[XY_MALLOC] %s %s %d %d  ", osThreadGetName(osThreadGetId()), file, line, size);
#else
		PrintLog(0,PLATFORM,WARN_LOG,"[XY_MALLOC] %s %d  ", osThreadGetName(osThreadGetId()), size);
#endif
	}
	
	void *mem = NULL;
#if( XY_HEAP_MEMORY_MALLOC_WITH_RECORD )
	mem = osMemoryAllocDebug(size, file, line);
#else
	mem = osMemoryAlloc(size);
#endif
	if(mem == NULL)
	{
		if(!loose)
		{
			Sys_Assert(0);
		}
		else
			gMallocFailNum++;

		osCoreEnterCritical();
		g_xy_mem_alloc_loose_cnt++;
		osCoreExitCritical();
	}
	else
	{
		if(clean)
		{	
			memset(mem, 0, size);
		}
	}

	return mem;
}

/** @brief  内存分配接口，可单独统计内存使用量
  */
#if( XY_HEAP_MEMORY_MALLOC_WITH_RECORD )
void* xy_MemAllocDebug_Ps(size_t size, char *file, int line, uint8_t loose, uint8_t clean)
#else
void* xy_MemAlloc_Ps(size_t size, uint8_t loose, uint8_t clean)
#endif
{
	void *mem = NULL;
#if( XY_HEAP_MEMORY_MALLOC_WITH_RECORD )
	mem = osMemoryAllocDebug(size, file, line);
#else
	mem = osMemoryAlloc(size);
#endif
	if(mem == NULL)
	{
		if(!loose)
		{
			Sys_Assert(0);
		}
	}
	else
	{
		if(clean)
		{	 
			memset(mem, 0, size);
		}
		vTaskSuspendAll();
		g_ps_mem_used += CalRealSize(mem);
		if(g_ps_mem_used > g_ps_mem_used_max)
		{
			g_ps_mem_used_max = g_ps_mem_used;
		}
		( void ) xTaskResumeAll();
	}

	return mem;
}


#if( XY_HEAP_MEMORY_MALLOC_WITH_RECORD )
void* xy_MemAllocAlignDebug(size_t size, char *file, int line, uint8_t loose, uint8_t clean)
#else
void* xy_MemAllocAlign(size_t size, uint8_t loose, uint8_t clean)
#endif
{
	void *mem = NULL;
#if( XY_HEAP_MEMORY_MALLOC_WITH_RECORD )
	mem = osMemoryAllocAlignDebug(size, file, line);
#else
	mem = osMemoryAllocAlign(size);
#endif
	if(mem == NULL)
	{
		if(!loose)
		{
			Sys_Assert(0);
		}
		else
			gMallocFailNum++;

		osCoreEnterCritical();
		g_xy_mem_alloc_align_loose_cnt++;
		osCoreExitCritical();
	}
	else
	{
		if(clean)
		{
			memset(mem, 0, size);
		}
	}

	return mem;
}


#if( XY_HEAP_MEMORY_FREE_WITH_RECORD )
void xy_MemFreeDebug(void *mem, char *file, int line)
#else
void xy_MemFree(void *mem)
#endif
{
#if( XY_HEAP_MEMORY_FREE_WITH_RECORD )
    (void)osMemoryFreeDebug(mem, file, line);
#else
    (void)osMemoryFree(mem);
#endif
}


#if( XY_HEAP_MEMORY_FREE_WITH_RECORD )
void xy_MemFreeDebug_Ps(void *mem, char *file, int line)
#else
void xy_MemFree_Ps(void *mem)
#endif
{
	vTaskSuspendAll();
	g_ps_mem_used -= CalRealSize(mem);
	( void ) xTaskResumeAll();

#if( XY_HEAP_MEMORY_FREE_WITH_RECORD )
    (void)osMemoryFreeDebug(mem, file, line);
#else
    (void)osMemoryFree(mem);
#endif
}

void  xy_cache_invalidate(void *addr,unsigned long len)
{
	(void)addr;
	(void)len;
}
void  xy_cache_writeback(void *addr,unsigned long len)
{
	(void)addr;
	(void)len;
}
void xy_cache_line_invalidate(void *addr)
{
	(void)addr;
}
void xy_cache_line_writeback(void *addr)
{
	(void)addr;
}


/** @brief  将AP核的内存地址转化为CP核的内存地址
  * @param  addr是需要转化的原内存地址
  * @retval 转化后的内存地址
  */
unsigned int Address_Translation_AP_To_CP(unsigned int addr)
{
	unsigned int CPAddr = addr;

	/* flash*/
   if((HWREGB(AONPRCM_BASE + 0x01) & 0x01) ) //CP Remap
   {
		if(addr >= FLASH_CP_BASE && addr < FLASH_CP_LIMITED)
		{
			CPAddr = addr - FLASH_CP_BASE;
		}
		// /* sram*/因CP cache问题暂时不应有0拷贝场景
		else if(addr >= TCM_AP_BASE && addr < TCM_AP_LIMITED)
		{
			CPAddr = addr + AP_TO_DMA_OFFSET;
		}
		else if(addr >= RAM1_DMA_BASE && addr < RAM1_DMA_BASE + TCM_CP_SIZE)
		{
			CPAddr = addr - CP_TO_DMA_REMAP_OFFSET;
		}
   }
   else
   {
	   if(addr >= TCM_AP_BASE && addr < TCM_AP_LIMITED)
	   {
			CPAddr = addr + AP_TO_DMA_OFFSET;
	   }
	   else if(addr >= RAM1_DMA_BASE && addr < RAM1_DMA_BASE + TCM_CP_SIZE)
	   {
			CPAddr = addr - CP_TO_DMA_OFFSET;
	   }
   }

	return CPAddr;
}

/** @brief  将CP核的内存地址转化为CP核的内存地址
  * @param  addr是需要转化的原内存地址
  * @retval 转化后的内存地址
  */
__RAM_FUNC  unsigned int Address_Translation_CP_To_AP(unsigned int addr)
{
    unsigned int CPAddr = addr;

	/* flash*/
    if((HWREGB(AONPRCM_BASE + 0x01) & 0x01) ) //CP Remap
	{
		if(addr >= FLASH_CP_REMAP_BASE && addr < FLASH_CP_REMAP_LIMITED)
		{
			CPAddr = addr + FLASH_CP_BASE;
		}
		else if(addr >= RAM0_DMA_BASE && addr < RAM0_DMA_BASE + TCM_AP_SIZE)
		{
			 CPAddr = addr - AP_TO_DMA_OFFSET;
		}
		// /* sram*/因CP cache问题暂时不应有0拷贝场景
		else if(addr >= TCM_CP_REMAP_BASE && addr < TCM_CP_REMAP_LIMITED)
		{
			 CPAddr = addr + CP_TO_DMA_REMAP_OFFSET;
		}
	}
    else
    {
    	if(addr >= RAM0_DMA_BASE && addr < RAM0_DMA_BASE + TCM_AP_SIZE)
		{
			 CPAddr = addr - AP_TO_DMA_OFFSET;
		}
    	else if(addr >= TCM_CP_BASE && addr < TCM_CP_LIMITED )
		{
    		CPAddr += CP_TO_DMA_OFFSET;
		}

    }
	
	return CPAddr;
}

/*RegionTwo空间为retention memory空间，可用来保存出厂NV或非易变NV*/
#if( XY_HEAP_MEMORY_MALLOC_WITH_RECORD )
void* xy_NV_Malloc_Debug(size_t size, char *file, int line)
#else
void* xy_NV_Malloc(size_t size)
#endif
{
	void *mem = NULL;

#if( XY_HEAP_MEMORY_MALLOC_WITH_RECORD )
	mem = pvPortMallocWithAlignDebug( size, RegionTwo, eHeapAlign_4, file, line );
#else
	mem = pvPortMallocWithAlign( size, RegionTwo, eHeapAlign_4);
#endif

	return mem;
}


#if( XY_HEAP_MEMORY_MALLOC_WITH_RECORD )
void* pvPortPhy_Malloc_Debug(size_t size, char *file, int line)
#else
void* pvPortPhy_Malloc(size_t size)
#endif
{
	void *mem = NULL;

#if( XY_HEAP_MEMORY_MALLOC_WITH_RECORD )
	mem = pvPortMallocWithAlignDebug( size, RegionTwo, eHeapAlign_4, file, line );
#else
	mem = pvPortMallocWithAlign( size, RegionTwo, eHeapAlign_4);
#endif

	return mem;
}

#if( XY_HEAP_MEMORY_FREE_WITH_RECORD )
void pvPortPhy_Free_Debug(void *mem, char *file, int line)
#else
void pvPortPhy_Free(void *mem)
#endif
{


#if( XY_HEAP_MEMORY_FREE_WITH_RECORD )
	(void)osMemoryFreeDebug(mem, file, line);
#else
	(void)osMemoryFree(mem);
#endif

}
uint32_t ShareMemMallocOrNot()
{
   return  vPortAllocatedBlocksOrNot(RegionTwo);
}


uint32_t *g_MultiMemArray = NULL;
uint32_t  g_MultiMemNum = 0;
uint32_t  g_MultiMemEnable = 0;


/*供物理层申请连续固定大小的内存块，最终的内存地址放在xData数组中*/
#if( XY_HEAP_MEMORY_MALLOC_WITH_RECORD )
__RAM_FUNC void xy_MemAllocDebug_Multi(size_t xWantedSize, size_t xWantedNum, uint32_t *xData, char *file, int line, uint8_t loose)
#else
__RAM_FUNC void xy_MemAlloc_Phy(size_t xWantedSize, size_t xWantedNum, uint32_t *xData, uint8_t loose)
#endif
{
	vTaskSuspendAll();

	if(xData && xWantedNum)
	{
		/*此处断言是为了防止物理层内存越界，查看gaMemAddrInfo*/
		Sys_Assert(xWantedNum <= 100);
		g_MultiMemArray = xData;
		g_MultiMemNum = xWantedNum;
		(*(g_MultiMemArray + g_MultiMemNum - 1)) = 0;
	}
	else
	{
		Sys_Assert(0);
	}

	g_MultiMemEnable = 1;

#if( XY_HEAP_MEMORY_MALLOC_WITH_RECORD )
	pvPortMallocWithAlignDebug(xWantedSize, RegionTwo, eHeapAlign_4, file, line );
#else
	pvPortMallocWithAlign( xWantedSize, RegionTwo, eHeapAlign_4);
#endif

	if((*(g_MultiMemArray + g_MultiMemNum - 1)) == 0) //判断最后一个节点是否申请
	{
		if(!loose)
		{
			Sys_Assert(0);
		}
	}

	g_MultiMemEnable = 0;

	( void ) xTaskResumeAll();
}
