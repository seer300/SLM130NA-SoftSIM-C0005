/**
  ******************************************************************************
  * @file    xy_mem.h
  * @brief   此文件包含了产生地址对齐的API接口
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
#pragma once
#include <stdint.h>
#include <stdio.h>
#include "system.h"
#include "stdbool.h"
#include "xy_system.h"
#include "factory_nv.h"
#include "xy_ftl.h"



/***************************NB出厂NV相关操作***************************/
#define OFFSET_SOFTAP_PRAM(param)     ((uint32_t) & ((((factory_nv_t *)0)->softap_fac_nv).param))
#define LEN_SOFTAP_PRAM(param)        (sizeof((((factory_nv_t *)0)->softap_fac_nv).param))

/*读写平台的基础出厂NV参数*/
#define READ_FAC_PARAM(param,val_addr)   (memcpy((void *)val_addr,GET_FAC_FLASH_BASE()+OFFSET_SOFTAP_PRAM(param),LEN_SOFTAP_PRAM(param)))
/*等效于READ_FAC_PARAM*/
#define READ_FAC_NV(type,param)		    (*(type *)(GET_FAC_FLASH_BASE() + OFFSET_SOFTAP_PRAM(param)))

#define WRITE_FAC_PARAM(param,val_addr)   (xy_ftl_write(NV_FLASH_FACTORY_BASE+OFFSET_SOFTAP_PRAM(param),(void *)val_addr,LEN_SOFTAP_PRAM(param)))


/***************************BLE出厂NV相关操作***************************/
#define OFFSET_BLE_PRAM(param)        ((uint32_t) &((((factory_nv_t *)0)->softap_fac_nv).ble_cfg.param))
#define LEN_BLE_PRAM(param)           (sizeof((((factory_nv_t *)0)->softap_fac_nv).ble_cfg.param))

/*读写BLE的出厂NV参数*/
#define READ_BLE_PARAM(param,val_addr)    (memcpy((void *)val_addr,GET_FAC_FLASH_BASE()+OFFSET_BLE_PRAM(param),LEN_BLE_PRAM(param)))
#define WRITE_BLE_PARAM(param,val_addr)   (xy_ftl_write(NV_FLASH_FACTORY_BASE+OFFSET_BLE_PRAM(param),(void *)val_addr,LEN_BLE_PRAM(param)))



/***************************RC32K校准相关NV操作***************************/
#define OFFSET_CALI_PRAM(param)        ((uint32_t)&(((Cali_Ftl_t *)0)->param))
#define LEN_CALI_PRAM(param)           (sizeof(((Cali_Ftl_t *)0)->param))

/*读写CALIB_FREQ_BASE校准相关的参数。读取参数值为全F，表示无效*/
#define READ_CALI_PARAM(param,val_addr)    (memcpy((void *)val_addr,(void *)(CALIB_FREQ_BASE+4+OFFSET_CALI_PRAM(param)),LEN_CALI_PRAM(param)))
#define WRITE_CALI_PARAM(param,val_addr)   (xy_ftl_write(CALIB_FREQ_BASE+OFFSET_CALI_PRAM(param),(void *)val_addr,LEN_CALI_PRAM(param)))




#if XY_DEBUG
	#define configUSE_HEAP_MALLOC_DEBUG     1     /*内存节点带文件名和行号*/
#else
	#define configUSE_HEAP_MALLOC_DEBUG     0     /*内存节点不带文件名和行号，节省点内存*/
#endif
typedef enum
{
  eHeapAlign_1 = 1,
  eHeapAlign_2 = 2,
  eHeapAlign_4 = 4,
  eHeapAlign_8 = 8,
  eHeapAlign_16 = 16,
  eHeapAlign_32 = 32,
  eHeapAlign_64 = 64,
  eHeapAlign_128 = 128
} eHeapAlign;

typedef unsigned int size_t;

#if (configUSE_HEAP_MALLOC_DEBUG == 1)
	/**
	 * @note zalloc内部会执行memset 0的操作，进而会影响性能；对于大内存的申请，务必注意是否需要初始化为0。
	 */
	void *aligned_zalloc_heap6(unsigned int required_bytes, unsigned int alignment, uint8_t loose, char *pcFile, int xLine);
	void *aligned_malloc_heap6(unsigned int required_bytes, unsigned int alignment, uint8_t loose, char *pcFile, int xLine);
	void *aligned_r_malloc_heap6(void *pvBlock, size_t size, eHeapAlign eAlignment, uint8_t loose, char *pcFile, int xLine);
	#define xy_malloc_r(ulSize)       aligned_malloc_heap6(ulSize, eHeapAlign_4, 0, __FILE__, __LINE__)
    #define xy_malloc_r2(ulSize)      aligned_malloc_heap6(ulSize, eHeapAlign_4, 1, __FILE__, __LINE__)
	#define pvPortMalloc(xSize) 	  aligned_malloc_heap6(xSize, eHeapAlign_4, 0, __FILE__, __LINE__)
	#define pvPortRealloc(pv, xSize)  //aligned_r_malloc_heap6(pv, xSize, eHeapAlign_4, 0, __FILE__, __LINE__)
#else
	/**
	 * @note zalloc内部会执行memset 0的操作，进而会影响性能；对于大内存的申请，务必注意是否需要初始化为0。
	 */
	void *aligned_zalloc_heap6(unsigned int required_bytes, unsigned int alignment, uint8_t loose);
	void *aligned_malloc_heap6(unsigned int required_bytes, unsigned int alignment, uint8_t loose);
	void *aligned_r_malloc_heap6(void *pvBlock, size_t size, eHeapAlign eAlignment, uint8_t loose);
	#define xy_malloc_r(ulSize)       aligned_malloc_heap6(ulSize, eHeapAlign_4, 0)
    #define xy_malloc_r2(ulSize)      aligned_malloc_heap6(ulSize, eHeapAlign_4, 1)
	#define pvPortMalloc(xSize)		  aligned_malloc_heap6(xSize, eHeapAlign_4, 0)
	#define pvPortRealloc(pv, xSize)  //aligned_r_malloc_heap6(pv, xSize, eHeapAlign_4, 0)
#endif
void aligned_free_heap6(void *p2);
#define xy_free_r(mem)   aligned_free_heap6(mem)
void vPortFree(void *pv);

size_t xPortGetFreeHeapSize(void);
size_t xPortGetMinimumEverFreeHeapSize(void);
size_t xPorGetTotalHeapSize(void);

uint32_t GET_FAC_FLASH_BASE();

