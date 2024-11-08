/**
  ******************************************************************************
  * @brief   This file contains memory defines, enumeration and
  *          structures definitions.
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

#ifndef _XY_MEM_H
#define _XY_MEM_H

#include "cmsis_os2.h"



/* 内存申请时，参数是否增加 file, line 的参数 */
#define XY_HEAP_MEMORY_MALLOC_WITH_RECORD    FreeRTOS_HEAP_MALLOC_WITH_RECORD
/* 内存释放时，参数是否增加 file, line 的参数 */
#define XY_HEAP_MEMORY_FREE_WITH_RECORD      FreeRTOS_HEAP_FREE_WITH_RECORD


/** @brief  重新分配内存
  * @param  mem是原来内存的地址
  * @param  size是需要重新分配的内存的大小
  * @note 如果需要32字节cache行对齐要用xy_realloc_Align
  */
#define xy_realloc(mem, size)   		osMemoryRealloc(mem, size)
#define xy_realloc_Align(mem, size)   	osMemoryReallocAlign(mem, size)

#if( XY_HEAP_MEMORY_FREE_WITH_RECORD )
  void xy_MemFreeDebug(void *mem, char *file, int line);
  #define xy_MemFree(mem)          xy_MemFreeDebug(mem, __FILE__, __LINE__)
  
  void xy_MemFreeDebug_Ps(void *mem, char *file, int line);
  #define xy_MemFree_Ps(mem)       xy_MemFreeDebug_Ps(mem, __FILE__, __LINE__)

  void pvPortPhy_Free_Debug(void *mem, char *file, int line);
  #define pvPortPhyFree( mem )     pvPortPhy_Free_Debug(mem, __FILE__, __LINE__)
#else
  void xy_MemFree(void *mem);
  void xy_MemFree_Ps(void *mem);
  void pvPortPhy_Free(void *mem);
  #define pvPortPhyFree( mem )   pvPortPhy_Free( mem )
#endif

#if( XY_HEAP_MEMORY_MALLOC_WITH_RECORD )
  void* xy_MemAllocDebug(size_t size, char *file, int line, uint8_t loose, uint8_t clean);
  void* xy_MemAllocDebug_Ps(size_t size, char *file, int line, uint8_t loose, uint8_t clean);
  void* pvPortPhy_Malloc_Debug(size_t size, char *file, int line);
  void* xy_NV_Malloc_Debug(size_t size, char *file, int line);
  void  xy_MemAllocDebug_Multi(size_t xWantedSize, size_t xWantedNum, uint32_t *xData, char *file, int line, uint8_t loose);
  
  #define XY_MALLOC(size)          xy_MemAllocDebug(size, __FILE__, __LINE__, 0, 0)
  #define XY_ZALLOC(size)          xy_MemAllocDebug(size, __FILE__, __LINE__, 0, 1)
  #define XY_MALLOC_LOOSE(size)    xy_MemAllocDebug(size, __FILE__, __LINE__, 1, 0)
  #define XY_ZALLOC_LOOSE(size)    xy_MemAllocDebug(size, __FILE__, __LINE__, 1, 1)

  #define XY_MALLOC_PS(size)       xy_MemAllocDebug_Ps(size, __FILE__, __LINE__, 0, 0)
  #define XY_ZALLOC_PS(size)       xy_MemAllocDebug_Ps(size, __FILE__, __LINE__, 0, 1)


  #define XY_MALLOC_MULTI(size, Num, xData)  xy_MemAllocDebug_Multi(size, Num, xData, __FILE__, __LINE__, 0)

  #define ShareMemMalloc( xSize,file,line) pvPortPhy_Malloc_Debug(xSize,file,line)
#else
  void* xy_MemAlloc(size_t size, uint8_t loose, uint8_t clean);
  void* xy_MemAlloc_Ps(size_t size, uint8_t loose, uint8_t clean);
  void* pvPortPhy_Malloc(size_t size);
  void* xy_NV_Malloc(size_t size);
  void  xy_MemAlloc_Phy(size_t xWantedSize, size_t xWantedNum, uint32_t *xData, uint8_t loose);

  #define XY_MALLOC(size)          xy_MemAlloc(size, 0, 0)
  #define XY_ZALLOC(size)          xy_MemAlloc(size, 0, 1)
  #define XY_MALLOC_LOOSE(size)    xy_MemAlloc(size, 1, 0)
  #define XY_ZALLOC_LOOSE(size)    xy_MemAlloc(size, 1, 1)

  #define XY_MALLOC_PS(size)       xy_MemAlloc_Ps(size, 0, 0)
  #define XY_ZALLOC_PS(size)       xy_MemAlloc_Ps(size, 0, 1)

  #define XY_MALLOC_MULTI(size, Num, xData)  xy_MemAlloc_Phy(size, Num, xData, 0)

  #define ShareMemMalloc( xSize,file,line) pvPortPhy_Malloc(xSize)
#endif

#if( XY_HEAP_MEMORY_MALLOC_WITH_RECORD )
  void* xy_MemAllocAlignDebug(size_t size, char *file, int line, uint8_t loose, uint8_t clean);

  #define XY_MALLOC_ALIGN(size)          xy_MemAllocAlignDebug(size, __FILE__, __LINE__, 0, 0)
  #define XY_ZALLOC_ALIGN(size)          xy_MemAllocAlignDebug(size, __FILE__, __LINE__, 0, 1)
  #define XY_MALLOC_ALIGN_LOOSE(size)    xy_MemAllocAlignDebug(size, __FILE__, __LINE__, 1, 0)
  #define XY_ZALLOC_ALIGN_LOOSE(size)    xy_MemAllocAlignDebug(size, __FILE__, __LINE__, 1, 1)
#else
  void* xy_MemAllocAlign(size_t size, uint8_t loose, uint8_t clean);

  #define XY_MALLOC_ALIGN(size)          xy_MemAllocAlign(size, 0, 0)
  #define XY_ZALLOC_ALIGN(size)          xy_MemAllocAlign(size, 0, 1)
  #define XY_MALLOC_ALIGN_LOOSE(size)    xy_MemAllocAlign(size, 1, 0)
  #define XY_ZALLOC_ALIGN_LOOSE(size)    xy_MemAllocAlign(size, 1, 1)
#endif


/** @brief  宽松的内存申请接口，返回NULL表示申请失败
  */
#define xy_malloc_loose(size)   		XY_MALLOC_LOOSE(size)
#define xy_malloc_Align_loose(size)   	XY_MALLOC_ALIGN_LOOSE(size)
#define xy_zalloc_loose(size)   		XY_ZALLOC_LOOSE(size)
#define xy_zalloc_Align_loose(size)   	XY_ZALLOC_ALIGN_LOOSE(size)

#define xy_free_ps(mem)                 xy_MemFree_Ps(mem)
#define xy_malloc_ps(ulSize)            XY_MALLOC_PS(ulSize)
#define xy_zalloc_ps(ulSize)            XY_ZALLOC_PS(ulSize)

extern uint32_t g_ps_mem_used;
extern uint32_t g_ps_mem_used_max;

int  is_CP_heap_addr(unsigned int addr);
int  is_ARM_heap_addr(unsigned int addr);

int  is_CP_flash_addr(unsigned int addr);
int  is_ARM_flash_addr(unsigned int addr);

int  is_SRAM_addr(unsigned int addr);

/** @brief  将AP核的内存地址转化为CP核的内存地址
  * @param  addr是需要转化的原内存地址
  * @retval 转化后的内存地址
  */
unsigned int Address_Translation_AP_To_CP(unsigned int addr);

/** @brief  将CP核的内存地址转化为AP核的内存地址
  * @param  addr是需要转化的原内存地址
  * @retval 转化后的内存地址
  */
unsigned int Address_Translation_CP_To_AP(unsigned int addr);

void *shm_mem_malloc(int ulSize);
void  xy_cache_invalidate(void *addr,unsigned long len);
void  xy_cache_writeback(void *addr,unsigned long len);
void xy_cache_line_invalidate(void *addr);
void xy_cache_line_writeback(void *addr);


size_t CalRealSize(void * mem);
uint32_t ShareMemMallocOrNot();

#endif
