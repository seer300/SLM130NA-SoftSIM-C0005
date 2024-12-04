#if STANDBY_SUPPORT

#include "hw_utc.h"
#include "phytimer.h"
#include "hw_timer.h"
#include "xy_rtc_api.h"
#include "tick.h"
#include "FreeRTOSConfig.h"
#include "PhyFRC_Time.h"
#include "timer.h"
#include "prcm.h"
#include "hw_prcm.h"
#include "dfe.h"
#include "low_power.h"
#include "xy_system.h"
#include "xy_lpm.h"
#include "sys_config.h"
#include "lwip/tcpip.h"
#include "xy_log.h"
#include "diag_list.h"
#include "at_uart.h"
#include "smartcard.h"
#include "ipc_msg.h"
#include "xy_wdt.h"
#include "diag_transmit_port.h"
#include "qspi_flash.h"

typedef enum
{
	STANDBY_FLASH_STATUS_NORMAL = 0,
	STANDBY_FLASH_STATUS_DEEP_POWER_DOWN,
	STANDBY_FLASH_STATUS_FLASH_VCC_OFF,
	STANDBY_FLASH_STATUS_VDDIO_OFF

} STANDBY_FLASH_STATUS_ENUM;

uint64_t g_standby_sleep_ms;
extern T_LPM_INFO Ps_Lpminfo;
extern volatile uint8_t g_phylowtmrstat;
extern volatile unsigned int g_freq_32k;
extern volatile uint32_t xo32k_advance_factor;
struct LPM_TIMER_INFO_RECOVERY_T g_lpm_standby_info;
extern volatile uint32_t g_nvic_0xe000e100;
extern volatile uint32_t g_nvic_0xe000e104;
extern volatile uint32_t g_nvic_0xe000e200;
extern volatile uint32_t g_nvic_0xe000e204;
#if LPM_LOG_DEBUG
uint32_t delta_phy_time_debug;
uint64_t utc_cal_debug;
extern volatile uint8_t platform_trigger_phytimer_record;
volatile uint32_t g_pending_after_sb;
volatile uint32_t g_pending_after_sb2;
volatile uint32_t g_pending_before_sb;
volatile uint32_t g_pending_before_sb2;
volatile uint32_t g_standby_wakeup_int;
volatile uint32_t g_standby_wakeup_int2;

volatile uint32_t g_standby_wakeup_status;

extern int32_t gTempera_old;
extern uint32_t g_mcnt_after_debug;
extern uint32_t g_pll_divn_old_debug;
extern uint32_t g_pll_divn_cur_debug;
extern uint32_t g_SFN_Number_after_sleep_debug;
extern uint32_t g_HFN_Number_after_sleep_debug;
extern uint32_t g_subframe_after_sleep_debug;
extern uint32_t g_countInSubFrame_after_sleep_debug;
extern uint32_t g_HFN_callback;
extern uint32_t g_SFN_callback;
extern uint32_t g_subframe_callback;
extern uint32_t g_countInSubFrame_callback;
extern uint64_t g_step_phy_cnt;
extern uint64_t g_utccnt_after_sleep_debug;
extern struct tcmcnt_info_t g_tcmcnt_info;
extern volatile FRC_TIME_t frc_temperature_debug_b;
extern volatile FRC_TIME_t frc_temperature_debug_a;

volatile uint32_t g_debug_record_tickbase = 0;
volatile uint64_t g_debug_rtc_offset = 0;
#endif 

uint32_t StandBy_Admittance_Check()
{
	if(  g_factory_nv->softap_fac_nv.lpm_standby_enable != 1 )
	{
		return NOT_ALLOW_STANDBY_NV_OFF;
	}

    if(is_sleep_locked(LPM_STANDBY))
	{
		return NOT_ALLOW_STANDBY_LOCK_UNRELEASE;
	}

	if(HWREGB(BAK_MEM_AP_LOCK_TYPE)&2)
	{
		return NOT_ALLOW_STANDBY_LOCK_UNRELEASE;
	}

	// if(LPM_Phytimer_Edge_Check() == 1)
	// {
	// 	return NOT_ALLOW_STANDBY_PHYTIMER_EDGE;
	// }

	if( !IF_DFE_TX_IDLE() || !IF_DFE_RX_IDLE())
	{
		
		return NOT_ALLOW_STANDBY_DFE_IN_WORK;
	}
	
	 if((diag_list_is_all_list_empty() == 0) || (diag_port_is_send_complete() == 0))
	 {
	 	return NOT_ALLOW_STANDBY_LOG_UNFINISHED;
	 }

	// 判断当前AT串口是否有未处理的数据，有则处理此数据，并退出睡眠
	if (at_uart_check_buffer_and_process() != 0)
	{
		return NOT_ALLOW_STANDBY_AT_UNHANDLE;
	}

    if(Smartcard_Sleep_Allow_Get() == 0)
    {
        return NOT_ALLOW_STANDBY_SMARTCARD_TRANSMIT;
    }
	
	tcpip_standby_wakeup_config();

	if(HWREGB(BAK_MEM_AP_STOP_CP_REQ) != 0)
	{
		return NOT_ALLOW_STANDBY_STOP_CP_REQ;
	}

	//判断AP核是否处于RC32K校准流程，做RC32K校准时不允许睡眠
	if(HWREGB(BAK_MEM_RC32K_CALI_FLAG) == 0xcc)
	{
		return NOT_ALLOW_STANDBY_RC32K_CALI;
	}

	HWREGB(BAK_MEM_RC32K_CALI_PERMIT) = 0;

	return ALLOW_STANDBY;
}
extern int  gu8BPLL_K_Value;
uint32_t StandBy_Context_Save(void)
{
 	rtc_reg_t utc_reg;
	FRC_TIME_t LocalFRC;
	RTC_TimeTypeDef rtctime;
	volatile uint32_t ulTimrh_cnt;
	volatile uint32_t ulTimerml_cnt;

	// 保存utc和phytimer瞬时时刻点
	LPM_Phytimer_UTC_Cnt_Save(&utc_reg , (uint32_t *)(&ulTimrh_cnt), (uint32_t *)(&ulTimerml_cnt));

	// 计算并保存phytimer时间
	FRC_GetLocalFRC_Critical(ulTimrh_cnt, ulTimerml_cnt, &LocalFRC);
	g_lpm_standby_info.SFN_Number_before_sleep = LocalFRC.FRC_Reg1.SFN_Number;
	g_lpm_standby_info.HFN_Number_before_sleep = LocalFRC.FRC_Reg1.HFN_Number;		
	g_lpm_standby_info.subframe_before_sleep = LocalFRC.FRC_Reg0.subframe;
	g_lpm_standby_info.countInSubFrame_before_sleep = LocalFRC.FRC_Reg0.countInSubFrame;	
	PhyTimerDisable();

	// 计算并保存UTC时间
	g_lpm_standby_info.utccnt_before_sleep = rtc_time_calculate_process(&utc_reg, &rtctime);

	g_lpm_standby_info.sleep_type = READY_TO_STANDBY;

	g_lpm_standby_info.frc_clk_divn = 192 + gu8BPLL_K_Value;
 
#if LPM_LOG_DEBUG
{
	g_debug_before_cal0 = utc_reg.rtc_cal;
	g_debug_before_time0 = utc_reg.rtc_timer;
	g_debug_before_cnt0 = utc_reg.rtc_cnt;
}
#endif
	/*STANDBY期间看门狗仍有效，直接deinit，这里不用refresh的原因是，refresh时间过长*/
    cp_utcwdt_deinit();

	return 0;
}


uint64_t StandBy_Cal_SleepTime_Again()
{
	uint64_t sleep_ms = TIME_SLEEP_FOREVER;
	uint64_t g_xo32k_advance_ms;
    uint64_t ullUsrAppExpSleepTime_ms = 0;

	ps_next_work_time(&Ps_Lpminfo);
	if(Ps_Lpminfo.state != READY_TO_STANDBY)
	{
		return 0;
	}

	ullUsrAppExpSleepTime_ms = LPM_GetExpectedIdleTick(READY_TO_STANDBY);
	Ps_Lpminfo.ullsleeptime_ms = ((Ps_Lpminfo.ullsleeptime_ms>ullUsrAppExpSleepTime_ms)?ullUsrAppExpSleepTime_ms:Ps_Lpminfo.ullsleeptime_ms);

	// PSM状态时主动停止phyimer的sfw1中断  此后物理层不再主动接受寻呼，只能依赖协议栈事件进行调度
	if(Ps_Lpminfo.ucTauEdrxOtherType == 1)
	{
		PhyTimerSFW1IntDisable();
	}
	// 获取phytimer sfw1定时器的状态，以此决定唤醒后是否需要重新恢复物理层搜寻呼能力
	g_phylowtmrstat = PhyTimerSFW1IntGet();

	if(Ps_Lpminfo.ullsleeptime_ms <= (uint64_t)Ps_Lpminfo.standby_advance_ms + LPM_PLATFORM_STANDBY_ADVANCE_MS )
	{
		return 0;
	}

	if(Ps_Lpminfo.ullsleeptime_ms != TIME_SLEEP_FOREVER)
	{
		sleep_ms = (uint64_t)Ps_Lpminfo.ullsleeptime_ms - Ps_Lpminfo.standby_advance_ms - LPM_PLATFORM_STANDBY_ADVANCE_MS;
		g_xo32k_advance_ms = sleep_ms * xo32k_advance_factor / 55 / 1000000;
		sleep_ms -= g_xo32k_advance_ms;
	}

	return sleep_ms;
}

uint32_t StandBy_WakeUp_Config()
{
	/* utc as wakeup source */
	AONPRCM->WAKUP_CTRL &= (~WAKEUP_CTRL_UTC_WKUP_ENA_Msk);
	AONPRCM->WAKUP_CTRL |= WAKEUP_CTRL_UTC_WKUP_ENA_Msk;
	
	if( g_standby_sleep_ms != TIME_SLEEP_FOREVER)
	{
		rtc_event_add_by_offset(RTC_TIMER_CP_LPM, g_standby_sleep_ms, NULL, RTC_NOT_RELOAD_FLAG);
	}

#if LPM_LOG_DEBUG
{
	g_debug_before_cal1 =  HWREG(UTC_CAL);
	g_debug_before_time1 = HWREG(UTC_TIMER);
	g_debug_before_cnt1 = HWREG(UTC_CLK_CNT);

	g_debug_record_tickbase = HWREG(BAK_MEM_TICK_CAL_BASE);
	g_debug_rtc_offset = *((volatile uint64_t *)BAK_MEM_RTC_NEXT_OFFSET);
}
#endif

	TickCPIntDisable(TICK_INT_CP_OVERFLOW_Msk | TICK_INT_CP_COMPARE_Msk);

	return 0;
}


uint32_t  StandBy_Power_Manage()
{
	// POWERDOWN SIM CARD
    extern void SimCard_Deactivation(void);
	SimCard_Deactivation();
/*
	// stop sido_1p8 & sido lp
	HWREGB(0x40004818) &= ~0x10;
	HWREGB(0x40000039) &= ~0x01;
	LPM_Delay(300);
*/	
	// disable jlink  for power saving.A1不再需要
	//LPM_Jlink_PowerOFF(g_softap_fac_nv->swd_swclk_pin, g_softap_fac_nv->swd_swdio_pin);

	return 0;
}

void __RAM_FUNC StandBy_Entry( void )
{
	// use lpm
	HWREG(0x40004028) |= 0x1;

	// lpm force idle
	HWREG(0x40004028) |= 0xf0000;

	//CP req standby in lpm mode
	HWREGB(0x40004030) = 0x3;

	// Cortex standby request
	SCB->SCR |= 0x4;

	// 屏蔽除wakeup中断和utc中断、32kcali外的所有中断，防止异常中断唤醒
	g_nvic_0xe000e100 = HWREG(0xe000e100) ;
	g_nvic_0xe000e104 = HWREG(0xe000e104) ;
	HWREG(0xe000e180) = ~((1 << (INT_UTC - 16)) | (1 << (INT_WAKEUP - 16)) );
	HWREG(0xe000e184) = ~(1 << (INT_RC32K - 48));

	// 将除了utc和wakeup中断、32kcali外的所有中断解悬
	g_nvic_0xe000e200 = HWREG(0xe000e200) ;
	g_nvic_0xe000e204 = HWREG(0xe000e204) ;
	HWREG(0xe000e280) |= ~((1 << (INT_UTC - 16)) | (1 << (INT_WAKEUP - 16)) );
	HWREG(0xe000e284) |= ~(1 << (INT_RC32K - 48));

#if LPM_LOG_DEBUG
	g_pending_before_sb = (HWREG(0xe000e100) & HWREG(0xe000e200));
	g_pending_before_sb2 = (HWREG(0xe000e104) & HWREG(0xe000e204));
	g_standby_wakeup_status = 0;
	g_standby_wakeup_int = 0;
#endif

	// CP更新当前状态，供AP裁决睡眠
	HWREGB(BAK_MEM_BOOT_CP_SYNC) = CP_IN_STANDBY;
	if(Ipc_SetInt() == 0)
	{
		qspi_wait_idle();

		__asm__ ("WFI");
	}

	// CP睡眠退出，及时更新自身状态
	HWREGB(BAK_MEM_BOOT_CP_SYNC) = CP_IN_WORK;

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
	g_pending_after_sb = (HWREG(0xe000e100) & HWREG(0xe000e200));
	g_pending_after_sb2 = (HWREG(0xe000e104) & HWREG(0xe000e204));
	g_standby_wakeup_status = HWREG(0x40000008);
	g_standby_wakeup_int = HWREG(0x4000000c);
    g_standby_wakeup_int2 = HWREG(0x40004020);
#endif

	SCB->SCR &= ~0x4;

	//CP req back to normal
	HWREGB(0x40004030) = 0;

}

int __RAM_FUNC StandBy_Power_Recover( void )
{
	//POWER ON FLASH
	//HWREG(0x400000A4) |= 0x2;

	//FORCE SIDO_1P4_RDY
	/*
	LPM_Delay(500);			// Must delay before sido switch!
	HWREGB(0X40000044)  |= 0x04;

	//sido_normal_ena	
	HWREGB(0X40000039) |= 0x51;	

	//START SIDO_1P8	
	HWREGB(0x40004818) = 0x16;		

	LPM_Delay(500);
    */
    // 关闭BB时钟，由协议栈自行决定时钟配置
	PRCM_ClockDisable(CORE_CKG_CTL_BB_EN| CORE_CKG_CTL_DFE_EN);
	return 0;
}

void __RAM_FUNC LPM_RTC_Event_Delete()
{
	//简化地删除cp lpm rtc事件，得益于后面补偿phytimer会重设rtcalarm；避免调用xy_rtc_timer_delete()接口，节省ram上代码和时间
	memset(cp_rtc_event_arry + RTC_TIMER_CP_LPM, 0, sizeof(rtc_event_info_t));
}

void __RAM_FUNC LPM_Wait_Flash_Recover()
{
	//wait flash ok
	while( HWREGB(BAK_MEM_LPM_FLASH_STATUS) !=  STANDBY_FLASH_STATUS_NORMAL);		
}

void __RAM_FUNC StandBy_Recover()
{
	//xy_rtc_timer_delete(RTC_TIMER_CP_LPM);
	//使用简化的lpm rtc删除，放在ram上
	LPM_RTC_Event_Delete();

 	StandBy_Power_Recover();

	//看情况A1是否可以删除，否则需要在ram上
	//LPM_Jlink_Config(g_softap_fac_nv->swd_swclk_pin, g_softap_fac_nv->swd_swdio_pin);
	
	// standby时并未停止phyimer sfw1，则恢复其定时中断
	if(g_phylowtmrstat)
	{
		PhyTimerIntEnable(PHYTIMER_SFW1MATCH_INT_EN_Msk);
	}

    #if LPM_LOG_DEBUG
    {
        g_debug_cal1 = HWREG(UTC_CAL);
        g_debug_time1 = HWREG(UTC_TIMER);
        g_debug_cnt1 = HWREG(UTC_CLK_CNT);
    }
    #endif 
	//wfi至flash恢复稳定前的代码都要在ram上，为了给flash充分的稳定时间，在pll稳定前有2ms的时间
	LPM_Wait_Flash_Recover();

	//该接口有flash代码，放在flash ready后
	Mcnt_Adjust();

	LPM_Phytimer_Compensate_Process(&g_lpm_standby_info);

	LPM_Tick_Recover();

	// 唤醒后，根据需要，刷新心跳包，让log能够正常输出
	diag_filter_refresh_heart_if_needed();

	/*standby恢复后，如果utc看门狗开启的情况下，重新初始化*/
	if(g_wdt_refresh)
		cp_utcwdt_init(UTC_WDT_TRIGGER_SECOND);

}

void StandBy_Debug_Before()
{
#if LPM_LOG_DEBUG
	uint64_t ullUsrAppExpSleepTime_ms = 0;
	ullUsrAppExpSleepTime_ms = LPM_GetExpectedIdleTick(READY_TO_STANDBY);

	xy_printf(0,PLATFORM,WARN_LOG,"LPM_STANDBY Ps_ullsleeptime_ms:%d,Ps_advance_ms:%d,ullUsrAppExpSleepTime_ms:%lld", (unsigned long)Ps_Lpminfo.ullsleeptime_ms,(unsigned long)Ps_Lpminfo.standby_advance_ms,ullUsrAppExpSleepTime_ms);
#endif
    // 阻塞等待log全部输出完成
    while ((diag_list_is_all_list_empty() == 0) || (diag_port_is_send_complete() == 0));
	diag_port_wait_send_done();
}

extern FRC_TIME_t gFrcTime_after;
int __RAM_FUNC StandBy_Process( void )
{
	int ret = ALLOW_STANDBY;

	ret = Mcnt_Get();
	if(  ret == 0 )
	{
		// MCNT校准失败，退出深睡流程
		return NOT_ALLOW_STANDBY_MCNT_FAILED;
	}

	StandBy_Debug_Before();

	vLpmEnterCritical();

	// 睡眠前更新是否需要在睡眠后手动刷新心跳包的标志位
	diag_filter_refresh_heart_flag();

	ret = StandBy_Admittance_Check();
	if(ret != ALLOW_STANDBY)
	{
		vLpmExitCritical();
		return ret;
	}

	g_standby_sleep_ms = StandBy_Cal_SleepTime_Again();
	if( g_standby_sleep_ms <= 15 )	// 时长小于15ms 可能是rtc机制本身导致的，不能进入睡眠！
	{
		HWREGB(BAK_MEM_RC32K_CALI_PERMIT) = 1;
		vLpmExitCritical();
		return NOT_ALLOW_STANDBY_TIME_LIMIT;
	}

	StandBy_Context_Save();

	StandBy_WakeUp_Config();

	StandBy_Power_Manage();
#if LPM_LOG_DEBUG
{
	g_debug_alarm_cal0 = HWREG(UTC_ALARM_CAL);
	g_debug_alarm_time0 = HWREG(UTC_ALARM_TIMER);
	g_debug_alarm_cnt0 = HWREG(UTC_ALARM_CLK_CNT);
}
#endif 

	StandBy_Entry();

#if LPM_LOG_DEBUG
{
	g_debug_cal0 = HWREG(UTC_CAL);
	g_debug_time0 = HWREG(UTC_TIMER);
	g_debug_cnt0 = HWREG(UTC_CLK_CNT);
}
#endif 

	StandBy_Recover();

	vLpmExitCritical();


#if LPM_LOG_DEBUG
	utc_cal_debug = (g_utccnt_after_sleep_debug - g_lpm_standby_info.utccnt_before_sleep) * g_mcnt_after_debug / 122880;
	xy_printf(0,PLATFORM, WARN_LOG, "LPM_STANDBY ps_sleep_type: %d, ps_ulltime_ms: %lld, ps_ullsleeptime_ms: %lld, ps_sb_advance_ms: %d, platform_advance_ms: %d, wakeup_status: %x, wakeup_int: %x, coreprcm_wkup: %x, test_rc:%d %d", Ps_Lpminfo.ucTauEdrxOtherType, Ps_Lpminfo.ulltime_ms, Ps_Lpminfo.ullsleeptime_ms, Ps_Lpminfo.standby_advance_ms, LPM_PLATFORM_STANDBY_ADVANCE_MS, g_standby_wakeup_status, g_standby_wakeup_int, g_standby_wakeup_int2, HWREGB(BAK_MEM_RC32K_CALI_FLAG),HWREGB(BAK_MEM_RC32K_CALI_PERMIT));
	xy_printf(0,PLATFORM, WARN_LOG, "LPM_STANDBY g_step_phy_cnt: %ld (ms), sleep_time_set: %lld (ms), utc_before: %lld, utc_after: %lld, MCNT: %d, PLL_old: %d, PLL_cur: %d, pb_sb: %x, pa_sb: %x, pb_sb2: %x, pa_sb2: %x", g_step_phy_cnt / 1920, g_standby_sleep_ms, g_lpm_standby_info.utccnt_before_sleep, g_utccnt_after_sleep_debug, g_mcnt_after_debug, g_pll_divn_old_debug, g_pll_divn_cur_debug,g_pending_before_sb , g_pending_after_sb, g_pending_before_sb2 , g_pending_after_sb2 );
	xy_printf(0,PLATFORM, WARN_LOG, "LPM_STANDBY frcbefore(%d,%d,%d,%d) frcafter(%d,%d,%d,%d) frc_callback(%d,%d,%d,%d),phytimer_trigger:%d,g_freq_32k: %d,frc_clk_divn: %d", g_lpm_standby_info.HFN_Number_before_sleep,g_lpm_standby_info.SFN_Number_before_sleep,g_lpm_standby_info.subframe_before_sleep,g_lpm_standby_info.countInSubFrame_before_sleep,gFrcTime_after.FRC_Reg1.HFN_Number,gFrcTime_after.FRC_Reg1.SFN_Number,gFrcTime_after.FRC_Reg0.subframe,gFrcTime_after.FRC_Reg0.countInSubFrame,g_HFN_callback,g_SFN_callback,g_subframe_callback,g_countInSubFrame_callback,platform_trigger_phytimer_record,g_freq_32k,g_lpm_standby_info.frc_clk_divn);

	if(  g_lpm_standby_info.HFN_Number_before_sleep == g_HFN_Number_after_sleep_debug )
	{
		delta_phy_time_debug = ((g_SFN_Number_after_sleep_debug * 10 + g_subframe_after_sleep_debug) * 1920 + g_countInSubFrame_after_sleep_debug) - (( g_lpm_standby_info.SFN_Number_before_sleep * 10 + g_lpm_standby_info.subframe_before_sleep ) * 1920 + g_lpm_standby_info.countInSubFrame_before_sleep);
		xy_printf(0,PLATFORM, WARN_LOG, "LPM_STANDBY delta_sleep_phy_cnt_cal :%lld, delta_phy_cnt: %d, utc_cal_debug: %lld, last_mcnt_temp: %d, temp_after_sleep: %d",g_step_phy_cnt, delta_phy_time_debug, utc_cal_debug, gTempera_old, g_tcmcnt_info.temp_after);
	}

	xy_printf(0,PLATFORM, WARN_LOG, "LPM_STANDBY utc_phytimer_snapshot utc_moment:(%x, %x, %x), wakeup_event_set utc_moment:(%x, %x, %x)", \
			g_debug_before_cal0, g_debug_before_time0, g_debug_before_cnt0, g_debug_before_cal1, g_debug_before_time1, g_debug_before_cnt1);
	xy_printf(0,PLATFORM, WARN_LOG, "LPM_STANDBY  exit_wfi utc_alarm:(%x, %x, %x), exit_wfi utc_moment:(%x, %x, %x), ap_enter_wfi utc_moment(%x, %d), record_tick:%d, offset:%lld", \
			g_debug_alarm_cal0, g_debug_alarm_time0, g_debug_alarm_cnt0, g_debug_cal0, g_debug_time0, g_debug_cnt0, *(uint32_t *)0x60011010, *(uint32_t *)0x60011014, g_debug_record_tickbase, g_debug_rtc_offset);
	xy_printf(0,PLATFORM, WARN_LOG, "LPM_STANDBY  phytimer_compensate utc_alarm:(%x, %x, %x), before_phytimer_compensate utc_moment:(%x, %x, %x)", \
			g_debug_alarm_cal1, g_debug_alarm_time1, g_debug_alarm_cnt1, g_debug_cal1, g_debug_time1, g_debug_cnt1);
	xy_printf(0,PLATFORM, WARN_LOG, "LPM_STANDBY pll_stable utc_moment:(%x, %x, %x), phytimer_trigger_event_set utc_moment:(%x, %x, %x)", \
			g_debug_cal2, g_debug_time2, g_debug_cnt2, g_debug_cal3, g_debug_time3, g_debug_cnt3);
	xy_printf(0,PLATFORM, WARN_LOG, "LPM_STANDBY  phytimer_compensate_begin utc_moment:(%x, %x, %x), phytimer_compensate_end utc_moment:(%x, %x, %x)", \
			g_debug_cal5, g_debug_time5, g_debug_cnt5, g_debug_cal4, g_debug_time4, g_debug_cnt4);

	delta_phy_time_debug = ((frc_temperature_debug_a.FRC_Reg1.SFN_Number * 10 + frc_temperature_debug_a.FRC_Reg0.subframe) * 1920 + frc_temperature_debug_a.FRC_Reg0.countInSubFrame) - ((frc_temperature_debug_b.FRC_Reg1.SFN_Number * 10 + frc_temperature_debug_b.FRC_Reg0.subframe) * 1920 + frc_temperature_debug_b.FRC_Reg0.countInSubFrame) ;
		
	xy_printf(0,PLATFORM, WARN_LOG, "LPM_STANDBY temperature debug frcbefore(%d,%d,%d,%d) frcafter(%d,%d,%d,%d) delta_frc_cnt:%d delatfrc_ms:%d", \
            frc_temperature_debug_b.FRC_Reg1.HFN_Number, frc_temperature_debug_b.FRC_Reg1.SFN_Number, frc_temperature_debug_b.FRC_Reg0.subframe, frc_temperature_debug_b.FRC_Reg0.countInSubFrame,\
			frc_temperature_debug_a.FRC_Reg1.HFN_Number, frc_temperature_debug_a.FRC_Reg1.SFN_Number, frc_temperature_debug_a.FRC_Reg0.subframe, frc_temperature_debug_a.FRC_Reg0.countInSubFrame, delta_phy_time_debug , delta_phy_time_debug/1920);

#endif
	return ALLOW_STANDBY;
}

#endif

