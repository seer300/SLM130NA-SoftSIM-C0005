/**
  ******************************************************************************
  * @file    oss_nv.h
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

#pragma once

#include "xy_utils.h"
#include "xy_flash.h"
#include "softap_nv.h"
#include "xy_memmap.h"
#include "cmsis_os2.h"
#include "rc32k_cali.h"


#define OFFSET_FAC_PARAM(param)     ((uint32_t) & (((softap_fac_nv_t *)0)->param))
#define FAC_PARAM_LEN(param)        (sizeof(((softap_fac_nv_t *)0)->param))
#define CP_FAC_LEN                  ((uint32_t) & (((factory_nv_t *)0)->softap_fac_nv))
//#define CHECK_SDK_TYPE(cust_type)   (g_factory_nv->softap_fac_nv.ver_type & (1 << cust_type))


/*实时保存softap平台出厂NV参数*/
#define SAVE_FAC_PARAM(param)	xy_ftl_write(NV_FLASH_FACTORY_BASE+CP_FAC_LEN + OFFSET_FAC_PARAM(param), (uint8_t *)&g_softap_fac_nv->param, FAC_PARAM_LEN(param))

/*实时保存softap平台出厂NV*/
#define SAVE_SOFTAP_FAC()		xy_ftl_write(NV_FLASH_FACTORY_BASE+CP_FAC_LEN, (uint8_t *)g_softap_fac_nv, sizeof(softap_fac_nv_t ))

/*获取有效的出厂NV的flash首地址，识别不严格，仅限调试功能参数使用，如调试功能PIN脚*/
#define GET_FAC_FLASH_BASE()	((*((uint8_t *)(NV_FLASH_FACTORY_BASE))==0x5a) ? (NV_FLASH_FACTORY_BASE+4) : (NV_MAIN_FACTORY_BASE))

/*获取平台出厂NV某参数值，识别不严格，仅限调试功能参数使用，如调试功能PIN脚*/
#define READ_FAC_NV(type,param) (*(type *)(GET_FAC_FLASH_BASE() + (uint32_t)(&((factory_nv_t *)0)->param)))


#define OFFSET_CALI_PRAM(param)        ((uint32_t)&(((Cali_Ftl_t *)0)->param))
#define LEN_CALI_PRAM(param)           (sizeof(((Cali_Ftl_t *)0)->param))

/*读写CALIB_FREQ_BASE校准相关的参数*/
#define READ_CALI_PARAM(param,val_addr)   (xy_ftl_read(CALIB_FREQ_BASE+OFFSET_CALI_PRAM(param),(void *)val_addr,LEN_CALI_PRAM(param)))
#define WRITE_CALI_PARAM(param,val_addr)   (xy_ftl_write(CALIB_FREQ_BASE+OFFSET_CALI_PRAM(param),(void *)val_addr,LEN_CALI_PRAM(param)))



/*出厂NV与非易变NV的堆空间来自于AP核借的内存，深睡保持供电*/
#if( XY_HEAP_MEMORY_MALLOC_WITH_RECORD )
	#define NV_MALLOC(size)           xy_NV_Malloc_Debug(size, __FILE__, __LINE__)
#else
	#define NV_MALLOC(size)           xy_NV_Malloc(size)
#endif


typedef enum XY_SDK_VER_T
{
	OPERATION_VER = 0, //ru ku ru ku version
	SDK_VER,		   //normal XY SDK,AT REQ is synchronous
} XY_SDK_VER;

extern int g_fast_off;
extern uint8_t g_orig_RF_mode;

extern uint32_t g_save_invar_nv; 

void nv_restore(void);
void dslp_sav_fac_nv();
void set_PS_work_mode();
void dslp_sav_invar_nv(uint8_t is_phy_nv_write_allow);
void set_invarnv_save_flag();

bool is_htol_test();


