/**
  ******************************************************************************
  * @file    oss_nv.c
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

#include "oss_nv.h"
#include "factory_nv.h"
#include "xy_memmap.h"
#include "xy_utils.h"
#include "nbiot_ps_export_interface.h"
#include "NBPhyL1cExportInterface.h"
#include "factory_nv.h"
#include "flash_adapt.h"
#include "low_power.h"
#include "xy_rtc_api.h"
#include "sys_config.h"
#include "net_app_resume.h"
#include "at_com.h"
#include "sys_clk.h"
#include "rc32k_cali.h"
#include "xy_utils_hook.h"
#include "lpm_nv_write.h"

/*系统上电时RF校准模式与否.若是，则整个工作期间不能发送任何AT给PS，包括关闭校准模式后，直至系统重新再上电*/
uint8_t g_orig_RF_mode = 0;		

factory_nv_t *g_factory_nv = NULL;			//平台NV
softap_fac_nv_t *g_softap_fac_nv = NULL;
factory_nv_t *ptPsFactory_Nv = NULL;		//协议栈出场NV
softap_var_nv_t *g_softap_var_nv = NULL;

// 打开retmem深睡下电功能时，需放在读出厂NV后面
void regist_save_bakmem_ftl(void)
{
	xy_ftl_regist(BAK_MEM_FLASH_BASE, BAK_MEM_FLASH_LEN);
	xy_ftl_regist(BAK_MEM_FLASH_BASE2, BAK_MEM_FLASH_LEN2);
}

// 预留，此块区域可不用擦除，在读之前，此块区域一定已经被写入过
void erase_save_bakmem_ftl(void)
{
	xy_Flash_Erase(BAK_MEM_FLASH_BASE, BAK_MEM_FLASH_LEN);
	xy_Flash_Erase(BAK_MEM_FLASH_BASE2, BAK_MEM_FLASH_LEN2);
}	

#define RAM_NV_VOLATILE_LEN	 (RAM_NV_VOLATILE_PHY_LEN + RAM_NV_VOLATILE_LPM_LEN + RAM_NV_VOLATILE_SOFTAP_LEN + BAK_MEM_CP_RTC_LEN)

void restore_flash_to_bakmem(void)
{
	// 深睡唤醒后，AP核通知CP核恢复深睡前的retmem内容
	if (HWREGB(BAK_MEM_OFF_RETMEM_FLAG) == 2)
	{
		HWREGB(BAK_MEM_OFF_RETMEM_FLAG) = 0;
		if(!xy_ftl_read(BAK_MEM_FLASH_BASE, (uint8_t *)RAM_NV_VOLATILE_PS_START, RAM_NV_VOLATILE_PS_LEN))
			memset((uint8_t *)RAM_NV_VOLATILE_PS_START, 0, RAM_NV_VOLATILE_PS_LEN);
		if(!xy_ftl_read(BAK_MEM_FLASH_BASE2, (uint8_t *)RAM_NV_VOLATILE_PHY_START, RAM_NV_VOLATILE_LEN))
			memset((uint8_t *)RAM_NV_VOLATILE_PHY_START, 0, RAM_NV_VOLATILE_LEN);
	}
}

extern uint64_t get_Dsleep_time(void);
void write_bakmem_to_flash(void)
{ 
	// 2100S模组形态下，深睡超过一定阈值时，retmem深睡需掉电
	// 1200对标B0模组形态下，retmem深睡强制断电，以上两种情形均需通知AP核给8K的retmem下电
	if (HWREGB(BAK_MEM_OFF_RETMEM_FLAG) == 1 )
	{
		// 用户数据、核间共享区域不进行保存
		xy_ftl_write(BAK_MEM_FLASH_BASE, (uint8_t *)RAM_NV_VOLATILE_PS_START, RAM_NV_VOLATILE_PS_LEN);
		xy_ftl_write(BAK_MEM_FLASH_BASE2, (uint8_t *)RAM_NV_VOLATILE_PHY_START, RAM_NV_VOLATILE_LEN);
	}
}

void xy_regist_nv_ftl()
{
	extern osMutexId_t ftl_mutex;
	osMutexAttr_t mutex_attr = {0};
	mutex_attr.attr_bits = osMutexRecursive;
    ftl_mutex = osMutexNew(&mutex_attr);
	xy_ftl_regist(NV_FLASH_FACTORY_BASE, NV_FLASH_FACTORY_LEN);
	xy_ftl_regist(NV_NON_VOLATILE_BASE, NV_NON_VOLATILE_LEN);
	xy_ftl_regist(NV_FLASH_RF_BASE, NV_FLASH_RF_SIZE);
	xy_ftl_regist(NV_FLASH_RF_BAKUP_BASE, NV_FLASH_RF_SIZE);
	xy_ftl_regist(CALIB_FREQ_BASE, CALIB_FREQ_LEN);
}


void malloc_factory_nv_mem()
{	
	if(NOT_ALLOWED_SAVE_FLASH())
		g_factory_nv = NV_MALLOC(sizeof(factory_nv_t));
	else
		g_factory_nv = xy_malloc(sizeof(factory_nv_t));
}

extern void reset_cp_rtcinfo(void);
void nv_restore()
{
	lpm_nv_write_init();
	//注册磨损均衡
	xy_regist_nv_ftl();
	uint8_t echo_mode;
	uint8_t cmee_mode;
	
	xy_assert(Ps_Get_InVar_F_Sizeof() <= XY_FTL_AVAILABLE_SIZE);
	xy_assert(Ps_Get_Var_F_Sizeof() <= RAM_NV_VOLATILE_PS_LEN);
	xy_assert((PhyL1cGetRetentionMemSize()) <= RAM_NV_VOLATILE_PHY_LEN);

	g_softap_fac_nv = &(g_factory_nv->softap_fac_nv);
	ptPsFactory_Nv = g_factory_nv;

	/*OPENCPU形态，出厂和非易变NV全局位于深睡保持供电的RAM区，唤醒/软复位/stop_cp时无需读FLASH*/
	xy_ftl_read(NV_FLASH_FACTORY_BASE, (uint8_t *)g_factory_nv, sizeof(factory_nv_t));

	//易变NV,其中PS相关的RTC每次深睡会重设，进而上电时清除
    Ps_Set_ptVar_F();
	g_softap_var_nv = (softap_var_nv_t *)RAM_NV_VOLATILE_SOFTAP_START;


	if(g_factory_nv->softap_fac_nv.bakmem_threshold != BAKMEM_8K_PWR_FORCE_ON) 
	{
		regist_save_bakmem_ftl();
	}

	echo_mode = g_softap_fac_nv->echo_mode;
	cmee_mode = g_softap_fac_nv->cmee_mode;
	if(Is_WakeUp_From_Dsleep())
	{
		if(g_factory_nv->softap_fac_nv.bakmem_threshold != BAKMEM_8K_PWR_FORCE_ON)
		{
			// 若retmem深睡掉电了，需从flash中恢复bakupmem里的内容
			restore_flash_to_bakmem();
		}

		g_softap_var_nv->next_PS_sec = 0;
		xy_rtc_timer_delete(RTC_TIMER_CP_LPM);

		// tau NV未使能，深睡唤醒清rtc链表
		if(HWREGB(BAK_MEM_CP_SET_RTC) == 0)
		{
			reset_cp_rtcinfo();
		}
		else
		{
			restore_RTC_list();
		}
#if 0//VER_BC95
		cmee_mode = g_softap_var_nv->cmee_mode;
#endif

#if 0//VER_BC25
		cmee_mode = g_softap_var_nv->cmee_mode;
		echo_mode = g_softap_var_nv->echo_mode;
#endif
	}
	/*SOFT_RESET软复位retention mem内容不会被清除，OPENCPU用到的BAK_MEM_NET_SEC等在AP核的BakupMemInit里执行初始化*/
	else
	{
		//断电上电或软重启时，易变NV对应的backup mem为脏数据，需清除，包括世界时间快照信息
		memset((uint8_t *)RAM_NV_VOLATILE_PS_START, 0, RAM_NV_VOLATILE_PS_LEN);	//PS的易变NV占用前4Kbackup mem

		/*OPENCPU形态，stop_cp后8Kretention仍有效，世界时间仍可获取，平台易变NV不得清零*/
		memset((uint8_t *)RAM_NV_VOLATILE_PHY_START, 0, RAM_NV_VOLATILE_PHY_LEN + RAM_NV_VOLATILE_LPM_LEN);
		/*保证时间快照信息有效*/
		memset((uint8_t *)RAM_NV_VOLATILE_SOFTAP_START+((uint32_t)&(((softap_var_nv_t *)0)->next_PS_sec)),0,RAM_NV_VOLATILE_SOFTAP_LEN-((uint32_t)&(((softap_var_nv_t *)0)->next_PS_sec)));

		//清除CP侧RTC链表
		memset((uint8_t *)BAK_MEM_CP_RTC_BASE, 0, BAK_MEM_CP_RTC_LEN);
#if 0//VER_BC95
		g_softap_var_nv->cmee_mode = g_softap_fac_nv->cmee_mode;
		g_softap_var_nv->at_parity = g_softap_fac_nv->at_parity;
#endif

#if 0//VER_BC25
		g_softap_var_nv->echo_mode = g_softap_fac_nv->echo_mode;
		g_softap_var_nv->cmee_mode = g_softap_fac_nv->cmee_mode;
		g_softap_var_nv->wakup_URC = g_softap_fac_nv->wakup_URC;
		// g_softap_var_nv->qsclk_mode = g_softap_fac_nv->qsclk_mode;
#endif

	}
	g_orig_RF_mode = HWREGB(BAK_MEM_RF_MODE);

	extern void check_HTOL_test();
	check_HTOL_test();

	/* 双核共同使用，务必在CP核调度前完成共享内存设置，否则AP核先于CP核调度NV会失效 */
	set_cmee_mode(cmee_mode);
	set_echo_mode(echo_mode);
	
	Set_32K_Freq(HWREG(BAK_MEM_RC32K_FREQ));
	READ_CALI_PARAM(default_freq,&g_32k_default_freq);
	xy_assert(g_32k_default_freq >= 31768 && g_32k_default_freq <= 33768);

	READ_CALI_PARAM(hrc_clk,&g_hrc_clock);
	xy_assert(g_hrc_clock >= 23000000 && g_hrc_clock <= 28000000);
	//set_PS_work_mode();
}

/*仅RF自校准参数才容许深睡时写*/
void dslp_sav_fac_nv()
{
	xy_ftl_write(NV_FLASH_FACTORY_BASE, (uint8_t *)g_factory_nv, sizeof(factory_nv_t));
}

extern uint8_t *Ps_GetPhyInvar(void);
/*检查NV中保存的物理层老化参数是否已经过旧*/
bool check_phy_ppm_nv_update()
{
	bool ret=false;
	uint64_t *ppm_nv_ms = (uint64_t *)Ps_GetPhyInvar();

	/*容许写FLASH*/
	if(!NOT_ALLOWED_SAVE_FLASH())
	{
		ret = true;
		goto DONE;
	}
	
	/*第一次写FLASH*/
	if(*ppm_nv_ms == 0)
	{
		ret = true;
		goto DONE;
	}
	
	if (HWREGB(BAK_MEM_XY_DUMP) == 1)
	{
		ret = true;
		goto DONE;
	}

	if(g_softap_var_nv->wall_time_ms != 0)
	{
		/*超过半年，则写一次FLASH*/
		if(g_softap_var_nv->wall_time_ms-*ppm_nv_ms > 0x39EF8B000)
			ret = true;
		else
			ret = false;
	}
DONE:
	if(ret == true)
		*ppm_nv_ms = g_softap_var_nv->wall_time_ms;
	
	return ret;
}

/*phy老化参数更新时，必须写非易变NV到FLASH*/
void set_invarnv_save_flag()
{
	g_save_invar_nv = 1;
}

void update_volatile_nv(void)
{
	// 物理层易变nv使用RAM空间，只有睡眠前刷新至retension memroy中! 原因在于：物理层易变nv结构体成员较离散，并且retension读写速度低于CP SRAM！啓閫熷害浣庝簬CP SRAM锛?
	NL1cBackupInfoSaveToRetentionMem();

	//更新协议栈易变NV的睡眠时间，为唤醒后恢复流程做准备
	*(uint64_t *)((void*)Ps_Get_ptVar_F()) = get_utc_tick();

	//强行断电异常流程，不考虑易变NV，接近于强行断电上电
	if (g_fast_off == 1)
	{
		g_softap_var_nv->next_PS_sec = 0;
		g_softap_var_nv->ps_deepsleep_state = 0;
        memset((uint8_t *)RAM_NV_VOLATILE_PS_START, 0, RAM_NV_VOLATILE_PS_LEN);
        memset((uint8_t *)RAM_NV_VOLATILE_LPM_START, 0, RAM_NV_VOLATILE_LPM_LEN);
		memset((uint8_t *)(RAM_NV_VOLATILE_PHY_START), 0, RAM_NV_VOLATILE_PHY_LEN);//PHY易变空间不足，为保证功能，暂用前4K
		//执行fastoff情况下，删除芯翼自己的RTC事件
		int i = RTC_TIMER_CP_BASE;
		for (; i < RTC_TIMER_CP_END; i++)
			xy_rtc_timer_delete(i);

		g_softap_var_nv->cdp_recovery_flag = 0;//清除云恢复标志位
	}
	//仅进行状态机更新，不影响深睡时长
	else
	{
		save_net_app_infos();
	}

}


/* 仅phy老化的非易变NV容许写FLASH */
uint32_t g_save_invar_nv = 0; 


void SaveNvInDeepsleep(uint32_t fastrecovery_flag)
{       
	/*深睡过程中，只有bit0为出厂NV(RF自校准);bit1为非易变(phy老化)容许写NV，且由AP核执行写动作*/
	if(NOT_ALLOWED_SAVE_FLASH())
	{
		// dslp_sav_fac_nv();
		
        dslp_sav_invar_nv(g_save_invar_nv);
		
		update_volatile_nv();
	}
	else
	{
		if(fastrecovery_flag == 0)
		{
			update_volatile_nv();
			
			if(g_factory_nv->softap_fac_nv.bakmem_threshold != BAKMEM_8K_PWR_FORCE_ON)
			{
				// 2100S模组形态下，深睡超过一定阈值时，retmem深睡需掉电
				// 1200对标B0模组形态下，retmem深睡强制断电，以上两种情形均需保存bakupmem里的内容至flash中
				write_bakmem_to_flash();
			}
			
			/* 非快速恢复：1. 更新易变NV 2.保存非易变NV+易变NV至flash */
			Ps_SaveInvar_forLpm();
		}
		else
		{
			/* 快速恢复：更新易变NV，但无需保存至flash */

			//更新协议栈易变NV的睡眠时间，为唤醒后恢复流程做准备
			*(uint64_t *)((void*)Ps_Get_ptVar_F()) = get_utc_tick();
		}
	}
}

xy_static_assert(sizeof(factory_nv_t) <= XY_FTL_AVAILABLE_SIZE);
xy_static_assert(sizeof(struct LPM_TIMER_INFO_RECOVERY_T) <= RAM_NV_VOLATILE_LPM_LEN);
xy_static_assert(sizeof(softap_var_nv_t) <= RAM_NV_VOLATILE_SOFTAP_LEN);


