#include "low_power.h"
#include "hw_utc.h"
#include "phytimer.h"
#include "hw_timer.h"
#include "sema.h"
#include "xy_rtc_api.h"
#include "tick.h"
#include "FreeRTOSConfig.h"
#include "PhyFRC_Time.h"
#include "deepsleep.h"
#include "ipc_msg.h"
#include "dfe.h"
#include "xy_system.h"
#include "xy_lpm.h"
#include "sys_config.h"
#include "softap_nv.h"
#include "xy_log.h"
#include "diag_list.h"
#include "at_uart.h"
#include "phytimer.h"
#include "rtc_tmr.h"
#include "prcm.h"
#include "smartcard.h"
#include "at_com.h"
#include "fast_recovery.h"
#include "ipc_msg.h"
#include "xy_memmap.h"
#include "xy_wdt.h"
#include "diag_transmit_port.h"
#include "adc.h"
#include "flash_adapt.h"
#include "xy_sys_hook.h"

uint64_t g_deepsleep_ms;
uint64_t g_utccnt_after_dp;
volatile uint32_t g_ps_wakeup_state = NORMAL;

extern T_LPM_INFO Ps_Lpminfo;
extern volatile unsigned int g_freq_32k;
extern volatile uint32_t g_Hclk_mcnt_cnt;
extern volatile uint32_t g_pll_divn_old;
extern volatile uint32_t g_pll_divn_cur;
extern volatile uint32_t xo32k_advance_factor;
extern volatile uint32_t g_phy_wakeup_state;
extern softap_var_nv_t *g_softap_var_nv;
extern volatile uint32_t g_nvic_0xe000e100;
extern volatile uint32_t g_nvic_0xe000e104;
extern volatile uint32_t g_nvic_0xe000e200;
extern volatile uint32_t g_nvic_0xe000e204;
extern struct LPM_TIMER_INFO_RECOVERY_T *g_lpm_deepsleep_recovery_info;
extern volatile uint8_t g_phylowtmrstat;
extern FRC_TIME_t gFrcTime_after;
extern int32_t gTempera_old;
extern uint32_t g_deepsleep_threshold;
extern uint32_t g_dsleep_fast_recovery_threshold;
extern uint32_t g_dsleep_bakmem_threshold;
uint32_t g_if_fast_recovery = 0;   //CP快速恢复标志，在每次睡眠前计算睡眠时长时更新。1：快速恢复  
extern void Mcnt_Adjust(void);
extern void SaveNvInDeepsleep(uint32_t fastrecovery_flag);
extern uint32_t PRCM_32KClkSrcGet(void);

#if LPM_LOG_DEBUG
uint64_t utc_cal_debug;
uint32_t delta_phy_time_debug;

volatile uint32_t g_deepsleep_wakeup_status;
volatile uint32_t g_deepsleep_wakeup_int;
volatile uint32_t g_deepsleep_wakeup_int2;
volatile uint32_t g_pending_before_urc;
volatile uint32_t g_pending_before_urc2;
volatile uint32_t g_pending_before_dp;
volatile uint32_t g_pending_after_dp;
volatile uint32_t g_pending_after_dp2;
volatile uint32_t g_pending_before_dp2;
extern uint32_t g_frc_aftersleep_sum;
extern uint32_t g_HFN_callback;
extern uint32_t g_SFN_callback;
extern uint32_t g_subframe_callback;
extern uint32_t g_countInSubFrame_callback;
extern uint32_t g_frc_callback_sum;
extern volatile uint8_t platform_trigger_phytimer_record;

extern uint32_t g_mcnt_after_debug;
extern uint32_t g_pll_divn_old_debug;
extern uint32_t g_pll_divn_cur_debug;
extern uint32_t g_SFN_Number_after_sleep_debug;
extern uint32_t g_HFN_Number_after_sleep_debug;
extern uint32_t g_subframe_after_sleep_debug;;
extern uint32_t g_countInSubFrame_after_sleep_debug;
extern struct tcmcnt_info_t g_tcmcnt_info;

extern uint64_t g_step_phy_cnt;
extern uint64_t g_utccnt_after_sleep_debug;

extern volatile FRC_TIME_t frc_temperature_debug_b;
extern volatile FRC_TIME_t frc_temperature_debug_a;
extern volatile FRC_TIME_t frc_sleep_time_debug_b;

extern volatile FRC_TIME_t gLocalFRC_debug;
extern PHY_TIMER_CB_t gPhyTimerCallback;

volatile uint32_t g_debug_alarm_cal_dp;
volatile uint32_t g_debug_alarm_time_dp;
volatile uint32_t g_debug_alarm_cnt_dp;

uint32_t ggdebug_alarm_cal = 0;
uint32_t ggdebug_alarm_time = 0;
uint32_t ggdebug_alarm_cnt = 0;
#endif 

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
#if CP_FAST_RECOVERY_FUNCTION
extern volatile uint32_t g_db_FR_begin_timer_alarm;
extern volatile uint32_t g_db_FR_begin_cnt_alarm;
extern volatile uint32_t g_db_FR_begin_timer;
extern volatile uint32_t g_db_FR_begin_cnt;
extern volatile uint32_t g_db_FR_end_timer;
extern volatile uint32_t g_db_FR_end_cnt;
extern volatile uint32_t g_db_FR_resetore_timer;
extern volatile uint32_t g_db_FR_resetore_cnt;
extern volatile uint32_t g_db_FR_rf_sido_timer;
extern volatile uint32_t g_db_FR_rf_sido_cnt;
extern volatile uint32_t g_db_FR_mpu_protect_timer;
extern volatile uint32_t g_db_FR_mpu_protect_cnt;
extern volatile uint32_t g_db_FR_diag_port_timer;
extern volatile uint32_t g_db_FR_diag_port_cnt;
extern volatile uint32_t g_db_FR_at_uart_fast_timer;
extern volatile uint32_t g_db_FR_at_uart_fast_cnt;
extern volatile uint32_t g_db_FR_rf_uart_timer;
extern volatile uint32_t g_db_FR_rf_uart_cnt;
extern volatile uint32_t g_db_FR_Sys_Up_URC_timer;
extern volatile uint32_t g_db_FR_Sys_Up_URC_cnt;
extern volatile uint32_t g_db_FR_xy_delaylock_timer;
extern volatile uint32_t g_db_FR_xy_delaylock_cnt;
extern volatile uint32_t g_db_FR_user_led_timer;
extern volatile uint32_t g_db_FR_user_led_cnt;
extern volatile uint32_t g_db_FR_wdt_timer;
extern volatile uint32_t g_db_FR_wdt_cnt;
extern volatile uint32_t g_db_FR_SFW_CfgCnt_timer;
extern volatile uint32_t g_db_FR_SFW_CfgCnt_cnt;
extern volatile uint32_t g_db_FR_DeepSleep_Recovery_timer;
extern volatile uint32_t g_db_FR_DeepSleep_Recovery_cnt;
extern volatile uint32_t g_db_FR_PHY_Event_timer;
extern volatile uint32_t g_db_FR_PHY_Event_cnt;
#endif

uint32_t g_exit_critical_timer;
uint32_t g_exit_critical_cnt;

/* 复用用户私有空间，保存AP侧函数级时统计信息，CP只读取作为log信息输出*/
#define BAK_MEM_RUN_TIME_STATISTICS     USER_BAK_MEM_BASE
#define RUNTIME_INFO_MAXNUM    32

typedef struct
{
	uint32_t cur_index;    //runtime_info[]的当前下标
	uint32_t start_time;   //第一个时间戳
	uint32_t last_time;    //最后一个时间戳
	uint32_t total_time;   //总时间消耗，单位ms

	struct
	{
		char *label_name;   //仅作标示使用，方便查看，无实质意义
		uint32_t runtime;   //上一次index到当前index的阶段性运行的实际时长，单位为ms
	}runtime_info[RUNTIME_INFO_MAXNUM];
}RUNTIME_DBG_T;

RUNTIME_DBG_T *g_dp_runtime_record = (RUNTIME_DBG_T *)(BAK_MEM_RUN_TIME_STATISTICS + 0x100);

uint32_t Debug_Runtime_Get_ms(uint32_t index)
{
	if(index >= g_dp_runtime_record->cur_index)
		return 0;
	return g_dp_runtime_record->runtime_info[index].runtime;
}

#define portCLK_TICK_COUNTER_REG    	( * ( ( volatile uint32_t * ) 0x4000e014 ) )
#define LPM_TIME_ENTER_CRI				( * ( ( volatile uint32_t * ) (BAK_MEM_RUN_TIME_STATISTICS + 0x20) ) )
#define LPM_TIME_ENTER_WFI				( * ( ( volatile uint32_t * ) (BAK_MEM_RUN_TIME_STATISTICS + 0x24) ) )

volatile uint32_t g_db_cp_schedule_time;
void lpm_time_enter_cri(void)
{
	LPM_TIME_ENTER_CRI = portCLK_TICK_COUNTER_REG;
}

void lpm_time_enter_wfi(void)
{
	g_lpm_deepsleep_recovery_info->cp_enter_wfi_tick = portCLK_TICK_COUNTER_REG;
	g_lpm_deepsleep_recovery_info->ap_cp_delta_time = g_lpm_deepsleep_recovery_info->cp_enter_wfi_tick;
}

void lpm_time_debug(void)
{
	g_db_cp_schedule_time  = portCLK_TICK_COUNTER_REG;
	g_dp_runtime_record = (RUNTIME_DBG_T *)(BAK_MEM_RUN_TIME_STATISTICS + 0x100);
	
	xy_printf(0,PLATFORM, WARN_LOG, "LPM_DEEPSLEEP utc_timer:%x, utc_cnt:%d, utc_alarm_timer:%x, utc_alarm_cnt:%d",
			*(uint32_t *)0x60011000, *(uint32_t *)0x60011004, *(uint32_t *)0x60011008, *(uint32_t *)0x6001100c);
	
	xy_printf(0,PLATFORM, WARN_LOG, "LPM_DEEPSLEEP ap_reset_handler:%d, mpu:%d, restore_msp:%d, lp_init:%d, sysclk_select:%d, apwakeup:%d, gpio_scene:%d, gpio_update:%d, 32kxtal:%d, wp_restore:%d, measuer_32k:%d, fastsysteminit:%d, fastrecovery:%d",
			g_dp_runtime_record->start_time, Debug_Runtime_Get_ms(1), Debug_Runtime_Get_ms(2), Debug_Runtime_Get_ms(3), Debug_Runtime_Get_ms(4), Debug_Runtime_Get_ms(5), Debug_Runtime_Get_ms(6), Debug_Runtime_Get_ms(7), Debug_Runtime_Get_ms(8), Debug_Runtime_Get_ms(9), Debug_Runtime_Get_ms(10), Debug_Runtime_Get_ms(11), Debug_Runtime_Get_ms(12));
	
	xy_printf(0,PLATFORM, WARN_LOG, "LPM_DEEPSLEEP boot_cp_start-wkp:%d, pll_select-wkp:%d, pll_lock-wkp:%d, boot_cp_end-wkp:%d, cp_task_start-wkp:%d,CP_ENTER_WFI:%d, AP_ENTER_WFI:%d, delta_dsleep_time:%d, WAKEUP:%d,schedule_time:%d,g_freq_32k:%d",
			Debug_Runtime_Get_ms(13), Debug_Runtime_Get_ms(14), Debug_Runtime_Get_ms(15), Debug_Runtime_Get_ms(16), (uint32_t)(g_db_cp_schedule_time - g_dp_runtime_record->start_time), g_lpm_deepsleep_recovery_info->cp_enter_wfi_tick, *(uint32_t *)0x60011018, g_lpm_deepsleep_recovery_info->ap_cp_delta_time, g_dp_runtime_record->start_time, g_db_cp_schedule_time, g_freq_32k);
}
#endif 

void update_Dsleep_time(uint64_t sleep_ms)
{
	g_deepsleep_ms = sleep_ms;
}

uint64_t get_Dsleep_time(void)
{
	return g_deepsleep_ms;
}


/*仅供PS使用，实时查询系统是否容许进入深睡：包含对NV:deepsleep_enable，以及AP、CP深睡锁三者的判断*/
char deepsleep_enabled_check()
{
	if(is_sleep_locked(LPM_DEEPSLEEP)==0 && !(HWREGB(BAK_MEM_AP_LOCK_TYPE)&1) && g_factory_nv->softap_fac_nv.deepsleep_enable == 1)
		return TRUE;
	else
		return FALSE;
}

int DeepSleep_Admittance_Check()
{
	if(  g_factory_nv->softap_fac_nv.deepsleep_enable != 1 )
	{
		return NOT_ALLOW_DEEPSLEEP_NV_OFF;
	}

	if(  g_factory_nv->softap_fac_nv.close_drx_dsleep==1 && Ps_Lpminfo.ucTauEdrxOtherType==2)
	{
		return NOT_ALLOW_DEEPSLEEP_NV_OFF;
	}

	// 判断睡眠锁是否释放
    if(is_sleep_locked(LPM_DEEPSLEEP) || (HWREGB(BAK_MEM_AP_LOCK_TYPE)&1))
	{
		return NOT_ALLOW_DEEPSLEEP_LOCK_UNRELEASE;
	}

	// 判断跨核内存是否释放
	if(g_zero_copy_sum != 0)
	{
		return NOT_ALLOW_DEEPSLEEP_ICM_ZERO_COPY;
	}

	// 判断射频是否空闲
	if( !IF_DFE_TX_IDLE() || !IF_DFE_RX_IDLE())
	{
		return NOT_ALLOW_DEEPSLEEP_DFE_IN_WORK;
	}

	extern int at_uart_is_write_complete(void);
	// 判断当前AT串口是否有未处理的数据、CP是否有uart输出需求，有则处理，并退出睡眠
	if (is_at_cmd_processing() || at_uart_check_buffer_and_process() != 0 || at_uart_is_write_complete() == 0)
	{
		return NOT_ALLOW_DEEPSLEEP_AT_UNHANDLE;
	}

	// 判断log是否输出完成
	if((diag_list_is_all_list_empty() == 0) || (diag_port_is_send_complete() == 0))
	{
		return NOT_ALLOW_DEEPSLEEP_LOG_UNFINISHED;
	}

    if(Smartcard_Sleep_Allow_Get() == 0)
    {
        return NOT_ALLOW_DEEPSLEEP_SMARTCARD_TRANSMIT;
    }

    if(((volatile flash_notice_t*)BAK_MEM_FLASH_NOTICE)->cp_status == cp_status_write)
	{
    	return NOT_ALLOW_DEEPSLEEP_FLASH_WRITE;
	}
	
	if(HWREGB(BAK_MEM_AP_STOP_CP_REQ) != 0)
	{
		return NOT_ALLOW_DEEPSLEEP_STOP_CP_REQ;
	}

	//判断AP核是否处于RC32K校准流程，做RC32K校准时不允许睡眠
	if(HWREGB(BAK_MEM_RC32K_CALI_FLAG) == 0xcc)    
	{
		return NOT_ALLOW_DEEPSLEEP_RC32K_CALI;
	}

	HWREGB(BAK_MEM_RC32K_CALI_PERMIT) = 0;

	return ALLOW_DEEPSLEEP;
}

extern int  gu8BPLL_K_Value;
volatile uint32_t g_sfw_cfgcnt;
extern void SimCard_Deactivation(void);
void DeepSleep_Context_Save(void)
{
	FRC_TIME_t LocalFRC;
 	rtc_reg_t utc_reg;
	RTC_TimeTypeDef rtctime;
	volatile uint32_t ulTimrh_cnt;
	volatile uint32_t ulTimerml_cnt;

	g_lpm_deepsleep_recovery_info->sleep_type = READY_TO_DEEPSLEEP ;

	// 保存utc和phytimer瞬时时刻点
	LPM_Phytimer_UTC_Cnt_Save(&utc_reg , (uint32_t *)(&ulTimrh_cnt), (uint32_t *)(&ulTimerml_cnt));

	FRC_GetLocalFRC_Critical(ulTimrh_cnt, ulTimerml_cnt, &LocalFRC);
	g_sfw_cfgcnt = SFW_CfgCnt_Get_Reg(SFW_CFGCNT1);
	g_lpm_deepsleep_recovery_info->SFN_Number_before_sleep = LocalFRC.FRC_Reg1.SFN_Number;
	g_lpm_deepsleep_recovery_info->HFN_Number_before_sleep = LocalFRC.FRC_Reg1.HFN_Number;		
	g_lpm_deepsleep_recovery_info->subframe_before_sleep = LocalFRC.FRC_Reg0.subframe;
	g_lpm_deepsleep_recovery_info->countInSubFrame_before_sleep = LocalFRC.FRC_Reg0.countInSubFrame;
	
	// 计算utc cnt值(由于计算过程较耗时，因此将获取与计算流程分开！)
	g_lpm_deepsleep_recovery_info->utccnt_before_sleep = rtc_time_calculate_process(&utc_reg, &rtctime);

	// mcnt：唤醒后用于phytimer补偿
	g_lpm_deepsleep_recovery_info->mcnt_32k_to_HCLK = g_Hclk_mcnt_cnt;

	// recovery flag: 标识唤醒后执行深睡恢复流程
	g_lpm_deepsleep_recovery_info->frc_flag = FRC_RECOVERY_FLAG;

	g_lpm_deepsleep_recovery_info->frc_clk_divn = 192 + gu8BPLL_K_Value;

	// // PLL：记录pll分频系数以提高补偿精度
	g_lpm_deepsleep_recovery_info->pll_divn_before = g_pll_divn_old;
	g_lpm_deepsleep_recovery_info->pll_divn_after =  get_pll_divn();
	g_lpm_deepsleep_recovery_info->temp_before = gTempera_old;

#if LPM_LOG_DEBUG
	g_debug_alarm_cal_dp = utc_reg.rtc_cal;
	g_debug_alarm_time_dp = utc_reg.rtc_timer;
	g_debug_alarm_cnt_dp = utc_reg.rtc_cnt;
#endif

	SaveNvInDeepsleep(g_if_fast_recovery);

	SimCard_Deactivation();

	/*芯片深睡，看门狗不会下电，直接deinit，这里不用refresh的原因是，refresh时间过长*/
    cp_utcwdt_deinit();
#if LPM_LOG_DEBUG
	g_pending_before_urc = (HWREG(0xe000e100) & HWREG(0xe000e200));
	g_pending_before_urc2 = (HWREG(0xe000e104) & HWREG(0xe000e204));
#endif

	//若已经产生wakeup中断，必然睡眠失败，此处不再上报powerdown，减少重复上报的概率
	if((((HWREG(0xe000e100) & HWREG(0xe000e200)) >> (INT_WAKEUP - 16)) & 1UL) == 0)  
	{
		if(p_SysDown_URC_Hook != NULL)
			p_SysDown_URC_Hook();
	}

	//HWREGH(BAK_MEM_CP_VBAT_MV) = gVBatVal;
}

extern volatile uint64_t g_debug_utccnt_after_sleep;
void DeepSleep_Recovery(void)
{
	uint32_t utc_tm = 0;

	if(g_lpm_deepsleep_recovery_info->frc_flag == FRC_RECOVERY_FLAG)
	{
		g_pll_divn_old = g_lpm_deepsleep_recovery_info->pll_divn_before ;
		g_Hclk_mcnt_cnt = g_lpm_deepsleep_recovery_info->mcnt_32k_to_HCLK;
		gTempera_old = g_lpm_deepsleep_recovery_info->temp_before;

#if LPM_LOG_DEBUG
        ggdebug_alarm_cal = HWREG(UTC_ALARM_CAL);
        ggdebug_alarm_time = HWREG(UTC_ALARM_TIMER);
        ggdebug_alarm_cnt = HWREG(UTC_ALARM_CLK_CNT);
#endif

		g_utccnt_after_dp = rtc_get_cnt();
		if( (g_utccnt_after_dp - g_lpm_deepsleep_recovery_info->utccnt_before_sleep) * configTICK_RATE_HZ / g_freq_32k > LPM_THRESHOLD_DEEPSLEEP_WITHFRC_MS)
		{
			HWREGB(BAK_MEM_RC32K_CALI_PERMIT) = 1;  
#if LPM_LOG_DEBUG
			xy_printf(0,PLATFORM, WARN_LOG, "LPM_DEEPSLEEP g_lpm_deepsleep_recovery_info->frc_flag:%d, temper:%d", g_lpm_deepsleep_recovery_info->frc_flag,gTempera_old);
#endif
			// 长时间深睡的补偿会失准，因此需要协议栈重新进行cell detect
			g_lpm_deepsleep_recovery_info->frc_flag = 0;
			memset((uint8_t *)RAM_NV_VOLATILE_LPM_START,0,sizeof(struct LPM_TIMER_INFO_RECOVERY_T));
			return;
		}

		RF_BBPLL_Freq_Set_PllDivn_recovery(g_lpm_deepsleep_recovery_info->pll_divn_after);

		HWREGB(PRCM_1P92M_CLK_CNT_N) = g_lpm_deepsleep_recovery_info->frc_clk_divn - 1;
		
		Mcnt_Adjust();

		LPM_Phytimer_Compensate_Process(g_lpm_deepsleep_recovery_info);
	}

	HWREGB(BAK_MEM_RC32K_CALI_PERMIT) = 1;  //执行fastoff后，不走以上流程，需要补充置1操作

	// 唤醒后，根据需要，刷新心跳包，让log能够正常输出
	diag_filter_refresh_heart_if_needed();

#if LPM_LOG_DEBUG
	xy_printf(0,PLATFORM, WARN_LOG, "LPM_DEEPSLEEP g_step_phy_cnt: %d, cur_utcms: %lld, utccnt_before_sleep: %lld, utccnt_after_sleep: %lld,wakeup_status: %x, pll_divn_old:%d, pll_divn_cur:%d frc_divn:%d frc_flag:%d temper:%d",(uint32_t)(g_step_phy_cnt/SubFrameCntRld),g_utccnt_after_dp/32,g_lpm_deepsleep_recovery_info->utccnt_before_sleep,g_utccnt_after_dp,AONPRCM->WAKEUP_STATUS,g_pll_divn_old,g_pll_divn_cur,g_lpm_deepsleep_recovery_info->frc_clk_divn, g_lpm_deepsleep_recovery_info->frc_flag,gTempera_old);
	xy_printf(0,PLATFORM, WARN_LOG, "LPM_DEEPSLEEP frcbefore(%d,%d,%d,%d) frcafter(%d,%d,%d,%d) sum:%d frc_callback(%d,%d,%d,%d) sum:%d phytimer_trigger:%d", g_lpm_deepsleep_recovery_info->HFN_Number_before_sleep,g_lpm_deepsleep_recovery_info->SFN_Number_before_sleep,g_lpm_deepsleep_recovery_info->subframe_before_sleep,g_lpm_deepsleep_recovery_info->countInSubFrame_before_sleep,gFrcTime_after.FRC_Reg1.HFN_Number,gFrcTime_after.FRC_Reg1.SFN_Number,gFrcTime_after.FRC_Reg0.subframe,gFrcTime_after.FRC_Reg0.countInSubFrame,g_frc_aftersleep_sum,g_HFN_callback,g_SFN_callback,g_subframe_callback,g_countInSubFrame_callback,g_frc_callback_sum,platform_trigger_phytimer_record);
	xy_printf(0,PLATFORM, WARN_LOG, "LPM_DEEPSLEEP utc_alarm: %x, %x, %x,cur_utcreg:%x, %x, %x, g_debug_utccnt_after_sleep:%lld test_rc:%d %d",ggdebug_alarm_cal,ggdebug_alarm_time,ggdebug_alarm_cnt,g_debug_cal4,g_debug_time4,g_debug_cnt4,g_debug_utccnt_after_sleep ,HWREGB(BAK_MEM_RC32K_CALI_FLAG),HWREGB(BAK_MEM_RC32K_CALI_PERMIT));
#endif

}
extern int16_t g_dsleep_FR_high_temp_threshold;
uint32_t g_platform_deepsleep_advance_ms = 0;
// 重新计算睡眠时长，用于设置睡眠唤醒（实际睡眠时长 = 协议栈规划出的睡眠时长 - 平台提前量 - 物理层提前量 - 代码运行时长 * 晶振拟合系数）
// 同时更新平台易变NV(next_PS_sec、ucTauEdrxOtherType），供上层调试与打印。
uint64_t DeepSleep_Cal_SleepTime_Again()
{
	int16_t tempera;
	uint64_t sleep_ms = TIME_SLEEP_FOREVER;
	uint64_t g_xo32k_advance_ms;

	ps_next_work_time(&Ps_Lpminfo);
	if(Ps_Lpminfo.state != READY_TO_DEEPSLEEP)
	{
		goto err_process;
	}

	// PSM状态时，平台主动停止phtimer，目的是关闭物理层搜寻呼能力，使得物理层调度只依赖协议栈事件而调度
	if(Ps_Lpminfo.ucTauEdrxOtherType == 1)
	{
		PhyTimerSFW1IntDisable();
	}
	// 获取phytimer sfw1定时器的状态，以此决定唤醒后是否需要重新恢复物理层搜寻呼能力
	g_phylowtmrstat = PhyTimerSFW1IntGet();

	//AP侧根据产品形态配置：AP_FAST_RECOVERY_FUNCTION、CP_FAST_RECOVERY_FUNCTION，并通过核间消息传递至CP
	//8K BAKMEM的power方案由NV:bakmem_threshold决定
	//以下为不同产品形态下的平台提前量、快速恢复标记位、8K下电标记位的配置

#if CP_FAST_RECOVERY_FUNCTION
	// CP开启快速恢复功能时，AP必然同样开启快速恢复功能，AP是否快速恢复由CP动态决定（仅模组）
	// CP根据睡眠时长判断是否快速恢复

	if (PRCM_32KClkSrcGet() == 1)
	{
		tempera = gTempera_old;
	}
	else
	{
		tempera = (gTempera_old * 2 - 50);
	}

	if( (Ps_Lpminfo.ullsleeptime_ms - (uint64_t)Ps_Lpminfo.deepsleep_advance_ms < g_dsleep_fast_recovery_threshold) && \
			( tempera < g_dsleep_FR_high_temp_threshold ) )   
	{
		g_platform_deepsleep_advance_ms = 12;	// 双核同时快速恢复时的提前量
		g_if_fast_recovery = 1;
	}
	else
	{
		if(g_factory_nv->softap_fac_nv.bakmem_threshold == BAKMEM_8K_PWR_FORCE_ON)   //8K BAKMEM睡眠时维持供电
		{
			g_platform_deepsleep_advance_ms = 50;      //双核同时不快速恢复，且8K维持供电的提前量
		}
		else if(g_factory_nv->softap_fac_nv.bakmem_threshold == BAKMEM_8K_PWR_FORCE_OFF)   
		{
			if(g_dsleep_fast_recovery_threshold != g_deepsleep_threshold)
			{
				xy_assert(0);  //8K BEKMEM睡眠时强制断电与快速恢复冲突，因为双核快速恢复8K必维持供电
			}
			else  
			{    
				//通过阈值控制构造双核不快速恢复的场景
				g_platform_deepsleep_advance_ms = 89;   //双核同时不快速恢复，且8K下电的提前量
				// 通知AP核
				HWREGB(BAK_MEM_OFF_RETMEM_FLAG) = 1;
			}	
		}   
		else    //根据睡眠时长动态决定8K BAKMEM是否下电
		{
			if(Ps_Lpminfo.ullsleeptime_ms - (uint64_t)Ps_Lpminfo.deepsleep_advance_ms > g_dsleep_bakmem_threshold)
			{
				g_platform_deepsleep_advance_ms = 89;   //双核同时不快速恢复，且8K下电的提前量
				// 通知AP核
				HWREGB(BAK_MEM_OFF_RETMEM_FLAG) = 1;
			}
			else
			{
				g_platform_deepsleep_advance_ms = 50;   //双核同时不快速恢复，且8K维持供电的提前量
				// 通知AP核
				HWREGB(BAK_MEM_OFF_RETMEM_FLAG) = 0;
			}
		}
		g_if_fast_recovery = 0;
	}
	
#else
	//CP不支持快速恢复功能（包括opencpu形态、1200对标B0模组）
	if(AP_FAST_RECOVERY_FUNCTION ==1)  //AP永远快速恢复，CP不快速恢复，8K维持供电（Opencpu表计）
	{
		g_platform_deepsleep_advance_ms = 24;
		g_if_fast_recovery = 0;
		// if(g_factory_nv->softap_fac_nv.bakmem_threshold != BAKMEM_8K_PWR_FORCE_ON)
		// {
		// 	xy_assert(0);
		// }
	}
	else   //AP、CP均永远不快速恢复（1200对标B0模组）
	{
		if(g_factory_nv->softap_fac_nv.bakmem_threshold == BAKMEM_8K_PWR_FORCE_OFF)         //1200对标B0模组，睡眠时8K RetMem强制断电
		{
			g_platform_deepsleep_advance_ms = 89;
			// 通知AP核
			HWREGB(BAK_MEM_OFF_RETMEM_FLAG) = 1;
		}
		else if(g_factory_nv->softap_fac_nv.bakmem_threshold == BAKMEM_8K_PWR_FORCE_ON)    //支持，但暂时未配置该形态
		{
			g_platform_deepsleep_advance_ms = 50;
		}
		else   //支持，但暂时未配置该形态
		{
			if(Ps_Lpminfo.ullsleeptime_ms - (uint64_t)Ps_Lpminfo.deepsleep_advance_ms > g_dsleep_bakmem_threshold)
			{
				g_platform_deepsleep_advance_ms = 89;   //双核同时不快速恢复，且8K下电的提前量
				// 通知AP核
				HWREGB(BAK_MEM_OFF_RETMEM_FLAG) = 1;
			}
			else
			{
				g_platform_deepsleep_advance_ms = 50;   //双核同时不快速恢复，且8K维持供电的提前量
				// 通知AP核
				HWREGB(BAK_MEM_OFF_RETMEM_FLAG) = 0;
			}
		}
		g_if_fast_recovery = 0;
	}
  

#endif

	// 根据平台与物理层提前量，计算实际睡眠时长，并记录在retention memory的易变NV中
	if(Ps_Lpminfo.ullsleeptime_ms <= (uint64_t)Ps_Lpminfo.deepsleep_advance_ms + g_platform_deepsleep_advance_ms )
	{
		goto err_process;
	}
	else if(Ps_Lpminfo.ullsleeptime_ms != TIME_SLEEP_FOREVER)
	{
		sleep_ms = (uint64_t)Ps_Lpminfo.ullsleeptime_ms - Ps_Lpminfo.deepsleep_advance_ms - g_platform_deepsleep_advance_ms;
		g_xo32k_advance_ms = sleep_ms * xo32k_advance_factor / 55 / 1000000;
		sleep_ms -= g_xo32k_advance_ms;

		//next_PS_sec 记录的是utc cnt对应的时间，与真实时间间需要进行32k频率转化!
		g_softap_var_nv->next_PS_sec = (uint32_t)((Ps_Lpminfo.ulltime_ms + CONVERT_MS_TO_RTCTICK(sleep_ms)) / configTICK_RATE_HZ);
	}
	else
	{
		if(Ps_Lpminfo.ucTauEdrxOtherType == 1)
		{
			// PSM
			g_softap_var_nv->next_PS_sec = 0XFFFFFFFF;
		}
		else
		{
			g_softap_var_nv->next_PS_sec = 0;
		}
	}
	
	g_softap_var_nv->ps_deepsleep_state = Ps_Lpminfo.ucTauEdrxOtherType;
	update_Dsleep_time(sleep_ms);
	return sleep_ms;

err_process:
	g_softap_var_nv->next_PS_sec = 0;
	g_softap_var_nv->ps_deepsleep_state = Ps_Lpminfo.ucTauEdrxOtherType;
	update_Dsleep_time(0);
	return sleep_ms;
}


int DeepSleep_WakeUp_Config()
{
	// tau NV未使能，则不设置唤醒事件，默认进入无限长睡眠
	if(HWREGB(BAK_MEM_CP_SET_RTC) == 0)
	{
		*((volatile uint64_t *)BAK_MEM_RTC_NEXT_OFFSET) = RTC_NEXT_OFFSET_INVAILD;   //BUG8986 规避代码：剩余毫秒偏移量设置为无效值，AP由此判断CP_RTC对于睡眠的影响是否有效
		// 失能UTC Alarm中断
		UTCAlarmDisable(UTC_ALARM_ALL);
		UTCAlarmCntCheckDisable();
		return 0;
	}

	AONPRCM->WAKUP_CTRL &= (~WAKEUP_CTRL_UTC_WKUP_ENA_Msk);
	AONPRCM->WAKUP_CTRL |= WAKEUP_CTRL_UTC_WKUP_ENA_Msk;
	
	// 设置UTC作为CP深睡唤醒事件
	if( get_Dsleep_time() != TIME_SLEEP_FOREVER)
	{
		rtc_event_add_by_offset(RTC_TIMER_CP_LPM, get_Dsleep_time() , NULL, RTC_NOT_RELOAD_FLAG);
	}
#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	//lpm_time_enter_cri();
#endif
	// 协议栈规划睡眠条件均已满足，此时不允许普通线程阻止深睡！
	TickCPIntDisable(TICK_INT_CP_OVERFLOW_Msk | TICK_INT_CP_COMPARE_Msk);
	g_lpm_deepsleep_recovery_info->wakeup_reason = DEEPSLEEP_WAKEUP_BY_UTC;

	return 0;
}

int DeepSleep_Power_Manage()
{
	return 0;
}

extern volatile uint32_t g_fast_startup_flag;
void DeepSleep_Entry()
{
	// retention memory校验
	*(uint32_t *)BAK_MEM_CP_RETMEM_CSM = RetMem_Checksum();

	// 使用lpm仲裁睡眠仲裁机制
	HWREG(0x40004028) |= 0x1;

	// lpm force idle
	HWREG(0x40004028) |= 0xf0000;

	//CP req deepsleep in lpm mode
	HWREGB(0x40004030) = 0x2;

	// 内核深睡申请
	SCB->SCR |= 0x4;

	// 屏蔽除wakeup中断和utc中断外的所有中断，防止异常中断唤醒深睡
	g_nvic_0xe000e100 = HWREG(0xe000e100) ;
	g_nvic_0xe000e104 = HWREG(0xe000e104) ;
	HWREG(0xe000e180) = ~((1 << (INT_UTC - 16)) | (1 << (INT_WAKEUP - 16)) );
	HWREG(0xe000e184) = ~(1 << (INT_RC32K - 48));

	// 将除了utc和wakeup中断外的所有中断解悬
	g_nvic_0xe000e200 = HWREG(0xe000e200) ;
	g_nvic_0xe000e204 = HWREG(0xe000e204) ;
	HWREG(0xe000e280) |= ~((1 << (INT_UTC - 16)) | (1 << (INT_WAKEUP - 16)) );
	HWREG(0xe000e284) |= ~(1 << (INT_RC32K - 48));

#if LPM_LOG_DEBUG
	g_pending_before_dp = (HWREG(0xe000e100) & HWREG(0xe000e200));
	g_pending_before_dp2 = (HWREG(0xe000e104) & HWREG(0xe000e204));
	g_deepsleep_wakeup_status = 0;
	g_deepsleep_wakeup_int = 0;
	g_deepsleep_wakeup_int2 = 0;
#endif

	/*OPENCPU产品，CP核正常进入深睡，此处设置下次Boot_CP后的上电原因*/
	HWREGB(BAK_MEM_CP_UP_REASON) = WAKEUP_DSLEEP;
	HWREG(BAK_MEM_CP_UP_SUBREASON) = 0;

#if CP_FAST_RECOVERY_FUNCTION
	if(g_if_fast_recovery == 1)
	{
		// 快速恢复
		g_fast_startup_flag = CP_WAKEUP_FASTRECOVERY_BEFORE;
		HWREG(DEEPSLEEP_CP_FASTRECOVERY_FLAG) = 0x43;
		
		save_scene_and_wfi();
	}
	else
#endif
	{
		// CP更新当前状态，供AP裁决睡眠
		HWREGB(BAK_MEM_BOOT_CP_SYNC) = CP_IN_DEEPSLEEP;
		HWREG(DEEPSLEEP_CP_FASTRECOVERY_FLAG) = 0;
		if(Ipc_SetInt() == 0)
		{
#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
			lpm_time_enter_wfi();
#endif
			// 普通模式
			__asm__ ("WFI");
		}
	}

	// CP睡眠退出，及时更新自身状态
	HWREGB(BAK_MEM_BOOT_CP_SYNC) = CP_IN_WORK;
	g_fast_startup_flag = CP_WAKEUP_NORMAL;
	HWREG(DEEPSLEEP_CP_FASTRECOVERY_FLAG) = 0;

	// 取消中断屏蔽,重新置中断位
	HWREG(0xe000e180) = 0;
	HWREG(0xe000e184) = 0;
	HWREG(0xe000e100) = g_nvic_0xe000e100;
	HWREG(0xe000e104) = g_nvic_0xe000e104 ;

	// 重新悬起
	HWREG(0xe000e280) = 0;
	HWREG(0xe000e284) = 0;
	HWREG(0xe000e200) = g_nvic_0xe000e200;
	HWREG(0xe000e204) = g_nvic_0xe000e204 ;

#if LPM_LOG_DEBUG
	g_pending_after_dp = (HWREG(0xe000e100) & HWREG(0xe000e200));
	g_pending_after_dp2 = (HWREG(0xe000e104) & HWREG(0xe000e204));
	g_deepsleep_wakeup_status = HWREG(0x40000008);
	g_deepsleep_wakeup_int = HWREG(0x4000000c);
	g_deepsleep_wakeup_int2 = HWREG(0x40004020);
#endif
	SCB->SCR &= ~0x4;

	//CP req back to normal
	HWREGB(0x40004030) = 0 ;
}

// 深睡失败时首先进行电源恢复
void Power_Manage()
{
    /*
	LPM_Delay(500); // must delay before sido switch

	HWREGB(0X40000044)  |= 0x04;	//FORCE SIDO_1P4_RDY

	HWREGB(0X40000039) |= 0x51;		//sido_normal_ena

	HWREGB(0x40004818) = 0x16;		//START SIDO_1P8

	LPM_Delay(500);
	*/
	while( !(COREPRCM->SYSCLK_FLAG & 0x8) );				// pll as system clock
}
extern void free_ext_flash_write_node(void);
void DeepSleep_Recover()
{
	HWREGB(BAK_MEM_RC32K_CALI_PERMIT) = 1;

	Power_Manage();

	// 睡眠失败，恢复所有的RTC
	if(g_fast_startup_flag != CP_WAKEUP_FASTRECOVERY_AFTER)
	{
		if(HWREGB(BAK_MEM_CP_SET_RTC) == 0)
		{
			rtc_event_refresh();
		}
	}

	// 恢复操作系统tick，保证系统正常调度
	LPM_Tick_Recover();

	/*如果从idle深睡流程中退出，需要及时删除待AP核跨核保存NV到flash中的节点，否则内存泄漏*/
	free_ext_flash_write_node();
	
	//删除写flash链表后，恢复写PS_InVarNV的NvChgBitmap标志位，再次睡眠时根据需求更新写flash的节点
	PsRollBackucNvChgBitMap();

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_exit_critical_timer = HWREG(UTC_TIMER);
	g_exit_critical_cnt = HWREG(UTC_CLK_CNT);
#endif

	/*芯片深睡失败，如果utc看门狗开启的情况下，重新初始化*/
	if(g_wdt_refresh)
		cp_utcwdt_init(UTC_WDT_TRIGGER_SECOND);

	//深睡失败后，刷新心跳包，让log能够正常输出
	diag_filter_refresh_heart_if_needed();
}

void DeepSleep_Debug_Before()
{
#if LPM_LOG_DEBUG
	uint64_t dsleep_wakeup_time;
	uint64_t frc_cal_temp;		

	// 统计dsleep睡眠时长
	dsleep_wakeup_time = (Ps_Lpminfo.ullsleeptime_ms == TIME_SLEEP_FOREVER) ? TIME_SLEEP_FOREVER :  Ps_Lpminfo.ulltime_ms + CONVERT_MS_TO_RTCTICK((uint64_t)Ps_Lpminfo.ullsleeptime_ms - Ps_Lpminfo.deepsleep_advance_ms);
	// 计算获取温度所需时间
	frc_cal_temp = ((frc_temperature_debug_a.FRC_Reg1.SFN_Number * 10 + frc_temperature_debug_a.FRC_Reg0.subframe) * 1920 + frc_temperature_debug_a.FRC_Reg0.countInSubFrame) - ((frc_temperature_debug_b.FRC_Reg1.SFN_Number * 10 + frc_temperature_debug_b.FRC_Reg0.subframe) * 1920 + frc_temperature_debug_b.FRC_Reg0.countInSubFrame) ;
	
	xy_printf(0,PLATFORM,WARN_LOG,"LPM_DEEPSLEEP Ps_ullsleeptime_ms:%d,Ps_advance_ms:%d,wakeup_ms:%lld,set_time(%d,%d,%d,%d),callback(%d,%d,%d,%d) test_rc:%d %d RC_SRC:%d,%d", (unsigned long)Ps_Lpminfo.ullsleeptime_ms,(unsigned long)Ps_Lpminfo.deepsleep_advance_ms,dsleep_wakeup_time,gLocalFRC_debug.FRC_Reg1.HFN_Number,gLocalFRC_debug.FRC_Reg1.SFN_Number,gLocalFRC_debug.FRC_Reg0.subframe,gLocalFRC_debug.FRC_Reg0.countInSubFrame,gPhyTimerCallback.callBackFRC.FRC_Reg1.HFN_Number,gPhyTimerCallback.callBackFRC.FRC_Reg1.SFN_Number,gPhyTimerCallback.callBackFRC.FRC_Reg0.subframe,gPhyTimerCallback.callBackFRC.FRC_Reg0.countInSubFrame,  HWREGB(BAK_MEM_RC32K_CALI_FLAG),HWREGB(BAK_MEM_RC32K_CALI_PERMIT), PRCM_32KClkSrcGet(),HWREGB(BAK_MEM_32K_CLK_SRC));

	xy_printf(0,PLATFORM, WARN_LOG, "LPM_DEEPSLEEP temperature debug frcbefore(%d,%d,%d,%d) frcafter(%d,%d,%d,%d) frc3(%d,%d,%d,%d) delta_frc_cnt:%d delatfrc_ms:%d enter_dp_tick:%d", \
            frc_temperature_debug_b.FRC_Reg1.HFN_Number, frc_temperature_debug_b.FRC_Reg1.SFN_Number, frc_temperature_debug_b.FRC_Reg0.subframe, frc_temperature_debug_b.FRC_Reg0.countInSubFrame,\
			frc_temperature_debug_a.FRC_Reg1.HFN_Number, frc_temperature_debug_a.FRC_Reg1.SFN_Number, frc_temperature_debug_a.FRC_Reg0.subframe, frc_temperature_debug_a.FRC_Reg0.countInSubFrame,\
			frc_sleep_time_debug_b.FRC_Reg1.HFN_Number, frc_sleep_time_debug_b.FRC_Reg1.SFN_Number, frc_sleep_time_debug_b.FRC_Reg0.subframe, frc_sleep_time_debug_b.FRC_Reg0.countInSubFrame,\
			frc_cal_temp , frc_cal_temp/1920, *(uint32_t*)0x4000e014);
#endif
    // 阻塞等待log全部输出完成
    while ((diag_list_is_all_list_empty() == 0) || (diag_port_is_send_complete() == 0));
	diag_port_wait_send_done();
}

void DeepSleep_Debug_After()
{
#if LPM_LOG_DEBUG
	utc_cal_debug = (g_utccnt_after_sleep_debug - g_lpm_deepsleep_recovery_info->utccnt_before_sleep);
	xy_printf(0,PLATFORM, WARN_LOG, "LPM_DEEPSLEEP ps_sleep_type: %d, ps_ulltime_ms: %lld, ps_ullsleeptime_ms: %lld, ps_standby_advance_ms: %d, platform_standby_advance_ms: %d, wakeup_status: %x, wakeup_int: %x, 40004020: %x test_rc:%d %d", Ps_Lpminfo.ucTauEdrxOtherType, Ps_Lpminfo.ulltime_ms, Ps_Lpminfo.ullsleeptime_ms, Ps_Lpminfo.deepsleep_advance_ms, g_platform_deepsleep_advance_ms, g_deepsleep_wakeup_status, g_deepsleep_wakeup_int, g_deepsleep_wakeup_int2,  HWREGB(BAK_MEM_RC32K_CALI_FLAG),HWREGB(BAK_MEM_RC32K_CALI_PERMIT));
	xy_printf(0,PLATFORM, WARN_LOG, "LPM_DEEPSLEEP g_step_phy_cnt: %ld (ms), sleep_time_set: %lld (ms), utc_before: %lld, utc_after: %lld, MCNT: %d, PLL_old: %d, PLL_cur: %d, p_b: %x, p_a: %x, p_b2: %x, p_a2: %x", g_step_phy_cnt / 1920, get_Dsleep_time(), g_lpm_deepsleep_recovery_info->utccnt_before_sleep, g_utccnt_after_sleep_debug, g_mcnt_after_debug, g_pll_divn_old_debug, g_pll_divn_cur_debug, g_pending_before_dp , g_pending_after_dp, g_pending_before_dp2 , g_pending_after_dp2 );

	delta_phy_time_debug = ((g_SFN_Number_after_sleep_debug * 10 + g_subframe_after_sleep_debug) * 1920 + g_countInSubFrame_after_sleep_debug) - (( g_lpm_deepsleep_recovery_info->SFN_Number_before_sleep * 10 + g_lpm_deepsleep_recovery_info->subframe_before_sleep ) * 1920 + g_lpm_deepsleep_recovery_info->countInSubFrame_before_sleep);
	xy_printf(0,PLATFORM, WARN_LOG, "LPM_DEEPSLEEP delta_sleep_phy_cnt_cal :%lld, delta_phy_cnt: %d, utc_cal_debug: %lld, %lld,g_freq_32k: %d, last_mcnt_temp: %d, temp_after_sleep: %d, frc_clk_divn:%d",g_step_phy_cnt, delta_phy_time_debug, utc_cal_debug, utc_cal_debug/32, g_freq_32k, gTempera_old, g_tcmcnt_info.temp_after,g_lpm_deepsleep_recovery_info->frc_clk_divn);
	xy_printf(0,PLATFORM, WARN_LOG, "LPM_DEEPSLEEP g_step_phy_cnt: %d, cur_utcms: %lld, utccnt_before_sleep: %lld, utccnt_after_sleep: %lld,wakeup_status: %x",(uint32_t)(g_step_phy_cnt/SubFrameCntRld),g_utccnt_after_dp/32,g_lpm_deepsleep_recovery_info->utccnt_before_sleep,g_utccnt_after_dp,AONPRCM->WAKEUP_STATUS );
	xy_printf(0,PLATFORM, WARN_LOG, "LPM_DEEPSLEEP frcbefore(%d,%d,%d,%d) frcafter(%d,%d,%d,%d) sum:%d frc_callback(%d,%d,%d,%d) sum:%d", g_lpm_deepsleep_recovery_info->HFN_Number_before_sleep,g_lpm_deepsleep_recovery_info->SFN_Number_before_sleep,g_lpm_deepsleep_recovery_info->subframe_before_sleep,g_lpm_deepsleep_recovery_info->countInSubFrame_before_sleep,gFrcTime_after.FRC_Reg1.HFN_Number,gFrcTime_after.FRC_Reg1.SFN_Number,gFrcTime_after.FRC_Reg0.subframe,gFrcTime_after.FRC_Reg0.countInSubFrame,g_frc_aftersleep_sum,g_HFN_callback,g_SFN_callback,g_subframe_callback,g_countInSubFrame_callback,g_frc_callback_sum);
	xy_printf(0,PLATFORM, WARN_LOG, "LPM_DEEPSLEEP utc_alarm: %x, %x, %x,cur_utcreg:%x, %x, %x,p_b_urc:%x, %x",ggdebug_alarm_cal,ggdebug_alarm_time,UTCClkCntConvert(ggdebug_alarm_cnt),g_debug_cal4,g_debug_time4,UTCClkCntConvert(g_debug_cnt4),g_pending_before_urc,g_pending_before_urc2);

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE	
#if CP_FAST_RECOVERY_FUNCTION
		extern volatile uint32_t g_pad[10];
		extern volatile uint32_t g_pad_index[10];
		xy_printf(0,PLATFORM, WARN_LOG, "LPM_DEEPSLEEP FR alarm(%x, %x) FR_begin(%x, %x) FR_end(%x, %x) exit_critical(%x, %x)", g_db_FR_begin_timer_alarm, g_db_FR_begin_cnt_alarm, g_db_FR_begin_timer, g_db_FR_begin_cnt, g_db_FR_end_timer, g_db_FR_end_cnt, g_exit_critical_timer, g_exit_critical_cnt );

		xy_printf(0,PLATFORM, WARN_LOG, "LPM_DEEPSLEEP g_debug_cal_dp:%x, g_debug_time_dp:%x, g_debug_cnt_dp:%x,", g_debug_alarm_cal_dp, g_debug_alarm_time_dp, UTCClkCntConvert(g_debug_alarm_cnt_dp));
		xy_printf(0,PLATFORM, WARN_LOG, "LPM_DEEPSLEEP FR resetore(%x, %x) rf_sido(%x, %x) mpu_protect(%x, %x)",g_db_FR_resetore_timer,g_db_FR_resetore_cnt,g_db_FR_rf_sido_timer,g_db_FR_rf_sido_cnt,g_db_FR_mpu_protect_timer,g_db_FR_mpu_protect_cnt);
		xy_printf(0,PLATFORM, WARN_LOG, "LPM_DEEPSLEEP FR diag_port(%x, %x) at_uart_fast(%x, %x) rf_uart(%x, %x) Sys_Up_URC(%x, %x)", g_db_FR_diag_port_timer,g_db_FR_diag_port_cnt,g_db_FR_at_uart_fast_timer,g_db_FR_at_uart_fast_cnt,g_db_FR_rf_uart_timer,g_db_FR_rf_uart_cnt,g_db_FR_Sys_Up_URC_timer,g_db_FR_Sys_Up_URC_cnt);
		xy_printf(0,PLATFORM, WARN_LOG, "LPM_DEEPSLEEP FR delaylock(%x, %x) user_led(%x, %x) wdt(%x, %x) SFW_CfgCnt(%x, %x)",g_db_FR_xy_delaylock_timer,g_db_FR_xy_delaylock_cnt,g_db_FR_user_led_timer,g_db_FR_user_led_cnt,g_db_FR_wdt_timer,g_db_FR_wdt_cnt,g_db_FR_SFW_CfgCnt_timer,g_db_FR_SFW_CfgCnt_cnt );
		xy_printf(0,PLATFORM, WARN_LOG, "LPM_DEEPSLEEP FR DeepSleep_Recovery(%x, %x) PHY_Event(%x, %x)", g_db_FR_DeepSleep_Recovery_timer,g_db_FR_DeepSleep_Recovery_cnt,g_db_FR_PHY_Event_timer,g_db_FR_PHY_Event_cnt);
		if(g_if_fast_recovery == 1)
		{
			lpm_time_debug();
		}
#endif
#endif

#endif
}

int DeepSleep_Process(void)
{
	int ret;

	ret = Mcnt_Get();
	if( ret == 0 )
	{	
		return NOT_ALLOW_DEEPSLEEP_MCNT_FAILED;
	}

	// dsleep调试打印
	DeepSleep_Debug_Before();

    vLpmEnterCritical();

	// 睡眠前更新是否需要在睡眠后手动刷新心跳包的标志位
	diag_filter_refresh_heart_flag();

	ret = DeepSleep_Admittance_Check();
	if(ret != ALLOW_DEEPSLEEP)
	{
		vLpmExitCritical();
		return ret;
	}
	
	DeepSleep_Cal_SleepTime_Again();
	if(get_Dsleep_time() <=  15)		// 1. 对于RTC已经超时的情况，后台机制会自动+10ms设置alarm，此时深睡被拉长 2. 此外，短时间进入深睡也并不划算
	{
		HWREGB(BAK_MEM_RC32K_CALI_PERMIT) = 1;
		vLpmExitCritical();
		return NOT_ALLOW_DEEPSLEEP_TIME_LIMIT;
	}

#if LPM_LOG_DEBUG
	SFW_CfgCnt_Get(SFW_CFGCNT1, &g_HFN_callback, &g_SFN_callback, &g_subframe_callback, &g_countInSubFrame_callback);
	g_frc_callback_sum = ( g_HFN_callback * 10240 + g_SFN_callback * 10 + g_subframe_callback) * 1920 + g_countInSubFrame_callback;
#endif

	DeepSleep_WakeUp_Config();

	if(p_Into_DeepSleep_Cb != NULL);
		p_Into_DeepSleep_Cb();

	DeepSleep_Context_Save();

	DeepSleep_Power_Manage();

	DeepSleep_Entry();

	DeepSleep_Recover();
	

	vLpmExitCritical();

	// dsleep调试打印
	DeepSleep_Debug_After();

	return ALLOW_DEEPSLEEP;
}
