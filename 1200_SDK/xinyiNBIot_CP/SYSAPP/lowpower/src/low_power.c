#include "low_power.h"
#include "hw_utc.h"
#include "phytimer.h"
#include "hw_timer.h"
#include "xy_rtc_api.h"
#include "tick.h"
#include "FreeRTOSConfig.h"
#include "deepsleep.h"
#if STANDBY_SUPPORT
#include "standby.h"
#endif
#include "xinyi_hardware.h"
#include "mcnt.h"
#include "nbiot_ps_export_interface.h"
#include "timer.h"
#include "oss_nv.h"
#include "adc.h"
#include "dfe.h"
#include "xy_log.h"
#include "diag_list.h"
#include "at_uart.h"
#include "rc32k_cali.h"
#include "xy_memmap.h"
#include "watchdog.h"
#include "xy_at_api.h"
#include "xy_wdt.h"
#include "utc.h"

#define ABS(x) ((x) > 0? (x) : (-(x)))
#define MIN(a,b) ((a)<(b)?(a):(b))

T_LPM_INFO Ps_Lpminfo;
FRC_TIME_t gFrcTime_after = {0};
struct tcmcnt_info_t g_tcmcnt_info={0};

int16_t g_dsleep_FR_high_temp_threshold;
uint32_t g_deepsleep_threshold = LPM_THRESHOLD_DEEPSLEEP_MS;
uint32_t g_dsleep_fast_recovery_threshold = LPM_FAST_RECOVERY_THRESHOLD;
uint32_t g_dsleep_bakmem_threshold = 0;
volatile uint8_t g_mcnt_finished = 0;
volatile uint32_t g_count_cumulate_standby = 0;
volatile uint32_t g_count_cumulate_deepsleep = 0;
volatile uint32_t g_pll_divn_old = 0;
volatile uint32_t g_pll_divn_cur = 0;
volatile uint32_t g_Hclk_mcnt_cnt = 0;
volatile uint8_t g_ucmcnt_feedback = 0;
volatile uint32_t xo32k_advance_factor = 0;
volatile uint8_t ucmcnt_recal_flag = 0;
volatile uint32_t g_phy_wakeup_state  = NORMAL;	
volatile uint8_t g_phylowtmrstat = 1;		// 状态标识位：判断物理层sfw1定时器是否被关闭
volatile uint32_t g_nvic_0xe000e100;
volatile uint32_t g_nvic_0xe000e104;
volatile uint32_t g_nvic_0xe000e200;
volatile uint32_t g_nvic_0xe000e204;
volatile uint8_t g_platform_trigger_phytimer = 0;
uint64_t g_wfi_sleep_ms;
volatile uint64_t g_idle_tick = 0;
volatile int g_sleep_check = 0;
uint8_t cp_hclk_div[16] = {40, 30, 35, 40, 45, 50, 60, 70, 80, 100, 120, 160, 40, 40, 40, 40};      // 考虑将其放于flash, 分频值*10
struct LPM_TIMER_INFO_RECOVERY_T *g_lpm_deepsleep_recovery_info = (struct LPM_TIMER_INFO_RECOVERY_T  *)RAM_NV_VOLATILE_LPM_START;



#if LPM_LOG_DEBUG
volatile uint32_t g_debug_before_cal0 = 0;
volatile uint32_t g_debug_before_time0 = 0;
volatile uint32_t g_debug_before_cnt0 = 0;

volatile uint32_t g_debug_before_cal1 = 0;
volatile uint32_t g_debug_before_time1 = 0;
volatile uint32_t g_debug_before_cnt1 = 0;

volatile uint32_t g_debug_alarm_cal0 = 0;
volatile uint32_t g_debug_alarm_time0 = 0;
volatile uint32_t g_debug_alarm_cnt0 = 0;

volatile uint32_t g_debug_alarm_cal1 = 0;
volatile uint32_t g_debug_alarm_time1 = 0;
volatile uint32_t g_debug_alarm_cnt1 = 0;

volatile uint32_t g_debug_cal0 = 0;
volatile uint32_t g_debug_time0 = 0;
volatile uint32_t g_debug_cnt0 = 0;

volatile uint32_t g_debug_cal1 = 0;
volatile uint32_t g_debug_time1 = 0;
volatile uint32_t g_debug_cnt1 = 0;

volatile uint32_t g_debug_cal2 = 0;
volatile uint32_t g_debug_time2 = 0;
volatile uint32_t g_debug_cnt2 = 0;

volatile uint32_t g_debug_cal3 = 0;
volatile uint32_t g_debug_time3 = 0;
volatile uint32_t g_debug_cnt3 = 0;

volatile uint32_t g_debug_cal4 = 0;
volatile uint32_t g_debug_time4 = 0;
volatile uint32_t g_debug_cnt4 = 0;

volatile uint32_t g_debug_cal5 = 0;
volatile uint32_t g_debug_time5 = 0;
volatile uint32_t g_debug_cnt5 = 0;

volatile uint32_t g_debug_utccntx = 0;
volatile uint64_t g_debug_utccnt_after_sleep;
uint32_t g_steptick_debug = 0;
uint32_t g_SFN_Number_after_sleep_debug = 0;
uint32_t g_HFN_Number_after_sleep_debug = 0;
uint32_t g_subframe_after_sleep_debug = 0;
uint32_t g_countInSubFrame_after_sleep_debug = 0;
uint32_t g_frc_aftersleep_sum = 0;
uint32_t g_HFN_callback = 0;
uint32_t g_SFN_callback = 0;
uint32_t g_subframe_callback = 0;
uint32_t g_countInSubFrame_callback = 0;
uint32_t g_frc_callback_sum = 0;
uint64_t g_utccnt_after_sleep_debug = 0;
uint32_t g_mcnt_after_debug = 0;
uint32_t g_pll_divn_old_debug = 0;
uint32_t g_pll_divn_cur_debug = 0;
uint64_t g_step_phy_cnt = 0;
uint64_t g_step_sys_cnt = 0;
volatile uint8_t platform_trigger_phytimer_record;
#endif 


typedef struct realtime_timer
{
	struct realtime_timer *next;
	osThreadId_t thread_id;
	osTimerId_t timer_id;
}realtime_timer_t;
osMutexId_t g_realtime_timer_mutex = NULL;
realtime_timer_t *g_realtime_timer_list = NULL;

extern int32_t gTempera_old;
extern volatile uint32_t g_ps_wakeup_state;
extern void send_debug_by_at_uart(char *buf);
extern void send_powerdown_urc_to_ext(void *buf, uint32_t size);

extern uint64_t get_Dsleep_time(void);
extern volatile rtc_reg_t g_debug_rtc_reg_standby_after;//UTC_ALARM_DEBUG

static uint32_t uxLpmSavePrimask = 0;
static uint32_t uxLpmCriticalNesting = 0;

void vLpmEnterCritical(void)
{
	uint32_t uxSavePrimask;

	uxSavePrimask = __get_PRIMASK();
	__disable_irq();
	__ISB();
	__DSB();

	if(uxLpmCriticalNesting == 0)
	{
		uxLpmSavePrimask = uxSavePrimask;
	}

	uxLpmCriticalNesting++;
}

void vLpmExitCritical(void)
{
	uint32_t uxSavePrimask;

	if(uxLpmCriticalNesting > 0)
	{
		uxLpmCriticalNesting--;
	}
	else
	{
		return;
	}

	if(uxLpmCriticalNesting == 0)
	{
		uxSavePrimask = uxLpmSavePrimask;

		__set_PRIMASK(uxSavePrimask);
		__ISB();
		__DSB();
	}
}

uint8_t vLpmCriticalTest( void )
{
	if( uxLpmCriticalNesting > 0 )
		return pdTRUE;

	return pdFALSE;
}

void boot_cp_sync(void)
{
	HWREGB(BAK_MEM_BOOT_CP_SYNC) = CP_IN_WORK;
}

void LPM_Jlink_PowerOFF(uint8_t swd_clk_pin, uint8_t swd_io_pin)
{
	GPIO_InitTypeDef gpio_init = {0};
	if((swd_clk_pin != 0xFF) && (swd_io_pin != 0xFF))
	{
		gpio_init.Pin = g_softap_fac_nv->swd_swclk_pin;
		gpio_init.Mode = GPIO_MODE_INPUT;
		gpio_init.Pull = GPIO_PULL_DOWN;
		GPIO_Init(&gpio_init);

		gpio_init.Pin = g_softap_fac_nv->swd_swdio_pin;
		gpio_init.Mode = GPIO_MODE_INPUT;
		gpio_init.Pull = GPIO_PULL_DOWN;
		GPIO_Init(&gpio_init);
	}
}

void LPM_Jlink_Config(uint8_t swd_clk_pin, uint8_t swd_io_pin)
{
	GPIO_InitTypeDef gpio_init = {0};

	if((swd_clk_pin != 0xFF) && (swd_io_pin != 0xFF))
	{
		GPIO_AllocateRemove(GPIO_CP_SWCLKTCK);
		GPIO_AllocateRemove(GPIO_CP_SWDIOTMS);

		gpio_init.Pin = swd_clk_pin;
		gpio_init.PinRemap = GPIO_CP_SWCLKTCK;
		gpio_init.Mode = GPIO_MODE_HW_PER;
		GPIO_Init(&gpio_init);
		GPIO_InputPeriSelect(swd_clk_pin, GPIO_CP_SWCLKTCK);
		GPIO_InputPeriSelectCmd(GPIO_CP_SWCLKTCK, ENABLE);


		gpio_init.Pin = swd_io_pin;
		gpio_init.PinRemap = GPIO_CP_SWDIOTMS;
		gpio_init.Mode = GPIO_MODE_HW_PER;
		GPIO_Init(&gpio_init);
		GPIO_InputPeriSelect(swd_io_pin, GPIO_CP_SWDIOTMS);
		GPIO_InputPeriSelectCmd(GPIO_CP_SWDIOTMS, ENABLE);

	}
}

uint32_t RetMem_Checksum()
{	
	// return 的0值表示checksum功能未开启
	uint32_t ret = 0;

	// checksum必须在开机初始化时进行，此时出厂NV未就绪，因此使用宏定义作为开启条件
#if RETENSION_MEMORY_CHECKSUM_ENABLE
	volatile uint32_t cp_checksum_result[5] = {0};

	// retension memory唤醒校验：由于双核独立深睡唤醒，为避免脏数据，此处只校验retension memory中CP侧独占的部分
	cp_checksum_result[0] = xy_chksum((void*)(RAM_NV_VOLATILE_PHY_START), RAM_NV_VOLATILE_PHY_LEN);
	cp_checksum_result[1] = xy_chksum((void*)(RAM_NV_VOLATILE_PS_START), RAM_NV_VOLATILE_PS_LEN);
	cp_checksum_result[2] = xy_chksum((void*)(RAM_NV_VOLATILE_SOFTAP_START), RAM_NV_VOLATILE_SOFTAP_LEN);
	cp_checksum_result[3] = xy_chksum((void*)(RAM_NV_VOLATILE_LPM_START), RAM_NV_VOLATILE_LPM_LEN);
	cp_checksum_result[4] = xy_chksum((void*)(BAK_MEM_CP_RTC_BASE), BAK_MEM_CP_RTC_LEN + BAK_MEM_NET_SEC_LEN + BAK_MEM_LOCAL_RTC_SEC_LEN);
	ret = xy_chksum((void*)(cp_checksum_result), sizeof(cp_checksum_result));	
#endif

	return ret;
}

/*OPENCPU形态，一次AT流程中只报一次POWERDOWN*/
int g_have_sent_downurc = 0;

/*上电URC参见Sys_Up_URC*/
void Sys_Down_URC_default(void)
{
	uint32_t rtc_msec_delta;
	uint64_t rtc_msec_cur;
	uint64_t next_rtc_ms;
	char *sys_down_str = NULL;
	rtc_msec_cur = get_utc_tick();
	rtc_msec_delta = (rtc_msec_cur - Ps_Lpminfo.ulltime_ms) * 32 / XY_UTC_CLK;

	/*OPENCPU形态，AT命令流程只触发一次URC*/
	if(Is_OpenCpu_Ver() && g_have_sent_downurc==1)
		return;
	g_have_sent_downurc = 1;
	
	if(HWREGB(BAK_MEM_CP_SET_RTC) == 0)
	{
		next_rtc_ms = RTC_ALARM_INVALID;
	}
	else
	{
		next_rtc_ms = Get_CP_ALARM_RAM();
	}


	if(HWREGB(BAK_MEM_XY_DUMP) == 1)
	{
		// 若代码运行时间过长，导致睡眠条件不满足。需要主动调整standby阈值
		xy_assert(get_Dsleep_time() > rtc_msec_delta);
		xy_assert( (g_softap_var_nv->next_PS_sec == 0) || (g_softap_var_nv->next_PS_sec >= rtc_msec_cur / configTICK_RATE_HZ));
	}

	sys_down_str = xy_malloc(40);
	if(g_softap_fac_nv->g_NPSMR_enable == 1)
	{
		sprintf(sys_down_str,"\r\n+NPSMR:1\r\n");
	}
	else 
	{
		if(next_rtc_ms != 0 && next_rtc_ms != RTC_ALARM_INVALID)
		{
			uint32_t next_rtc_sec = (uint32_t)(CONVERT_RTCTICK_TO_MS((Transform_Num64_To_Ms(next_rtc_ms) - rtc_msec_cur) / 1000));
			if(g_softap_var_nv->next_PS_sec==0xFFFFFFFF)
			{
				sprintf(sys_down_str,"+POWERDOWN:-1,%lu\r\n", next_rtc_sec);
			}
			else if(g_softap_var_nv->next_PS_sec!=0 && g_softap_var_nv->next_PS_sec > rtc_msec_cur / configTICK_RATE_HZ)
			{
				sprintf(sys_down_str,"+POWERDOWN:%lu,%lu\r\n",(uint32_t)(CONVERT_RTCTICK_TO_MS(g_softap_var_nv->next_PS_sec-rtc_msec_cur/configTICK_RATE_HZ)), next_rtc_sec);
			}
			else
			{
				sprintf(sys_down_str,"+POWERDOWN:0,%lu\r\n", next_rtc_sec);
			}
		}
		else
		{
			if(g_softap_var_nv->next_PS_sec==0xFFFFFFFF)
			{
				sprintf(sys_down_str,"+POWERDOWN:-1,-1\r\n");
			}
			else if(g_softap_var_nv->next_PS_sec!=0 && g_softap_var_nv->next_PS_sec > rtc_msec_cur / configTICK_RATE_HZ )
			{
				sprintf(sys_down_str,"+POWERDOWN:%lu,-1\r\n",(uint32_t)(CONVERT_RTCTICK_TO_MS(g_softap_var_nv->next_PS_sec-rtc_msec_cur/configTICK_RATE_HZ)));
			}
			else
			{
				sprintf(sys_down_str,"+POWERDOWN:0,-1\r\n");
			}
		}
	}

	if(sys_down_str != NULL)
	{
		xy_printf(0, PLATFORM, WARN_LOG,"URC: %s",sys_down_str);
			
		if(!is_urc_drop())
			send_powerdown_urc_to_ext(sys_down_str, strlen(sys_down_str));
		xy_free( sys_down_str);
	}
}

void LPM_Delay(uint32_t uldelay)
{
    volatile uint32_t i;
    
    for(i = 0; i < uldelay; i++)
    {
    }
}

// 低功耗初始化：全局唤醒相关配置
int LPM_Init(void)
{
	if(g_factory_nv->softap_fac_nv.FR_high_temp_threshold != 0)
	{
	    g_dsleep_FR_high_temp_threshold = g_factory_nv->softap_fac_nv.FR_high_temp_threshold;
	}
	else
	{
		g_dsleep_FR_high_temp_threshold = 50;
	}

	// deepsleep与standby临界时长,供协议栈drx\edrx睡眠规划时参考
	if(g_factory_nv->softap_fac_nv.deepsleep_threshold != 0)
		g_deepsleep_threshold = g_factory_nv->softap_fac_nv.deepsleep_threshold*1000;

	//快速恢复的时间阈值，低于此门限的深睡应执行快速恢复流程
	if(g_factory_nv->softap_fac_nv.fast_recovery_threshold != 0)
	    g_dsleep_fast_recovery_threshold = g_factory_nv->softap_fac_nv.fast_recovery_threshold*1000;

    //8k BakMem动态下电的时间阈值，模组形态下深睡时长超过此值时，bakeupmem区域回写flash后断电
	if((g_factory_nv->softap_fac_nv.bakmem_threshold != BAKMEM_8K_PWR_FORCE_ON) && (g_factory_nv->softap_fac_nv.bakmem_threshold != BAKMEM_8K_PWR_FORCE_OFF))
		g_dsleep_bakmem_threshold = g_factory_nv->softap_fac_nv.bakmem_threshold*60*1000;

	//NV设置的容错处理
#if CP_FAST_RECOVERY_FUNCTION     //CP快速恢复功能开启
	if(g_deepsleep_threshold > g_dsleep_fast_recovery_threshold)
	{
		g_dsleep_fast_recovery_threshold = g_deepsleep_threshold;   //若设置错误，强制相等，CP不走快速恢复
	}
#else    //CP快速恢复功能关闭时，上述两个阈值必须相同
	if(g_deepsleep_threshold != g_dsleep_fast_recovery_threshold)
	{
		g_dsleep_fast_recovery_threshold = g_deepsleep_threshold;
	}
#endif

	if((g_factory_nv->softap_fac_nv.bakmem_threshold != BAKMEM_8K_PWR_FORCE_ON) && (g_factory_nv->softap_fac_nv.bakmem_threshold != BAKMEM_8K_PWR_FORCE_OFF))
	{
		if(g_dsleep_bakmem_threshold < g_dsleep_fast_recovery_threshold)
		{
			//模组形态下若支持8K BAKMEM深睡动态下电，阈值bakmem_threshold必须合法有效，否则CP非快速恢复均给8K下电
			g_dsleep_bakmem_threshold = g_dsleep_fast_recovery_threshold;    
		}
	}

	return 0;
}

// phytimer子帧边界：避免在phytimer子帧跳变边沿进入睡
uint32_t  LPM_Phytimer_Edge_Check(void)
{
	if((HWREGH(PHYTIMER_TMRL_CNT)&0xFFF)>1800)
	{
		return 1;
	}
	return 0;
}


// Deepsleep、standby睡眠后 clktick 恢复
void LPM_Tick_Recover(void)
{
	// 睡眠期间发生的overflow中断并不会丢失！clktick对应的中断标记位一直有效！ 此处使能中断后，立即会触发中断pending！
	TickCPIntEnable(TICK_INT_AP_OVERFLOW_Msk | TICK_INT_AP_COMPARE_Msk);

	//  仅触发clktick中断即可，实际补偿流程由clktick机制保证
	( * ( ( volatile uint32_t * ) NVIC_PEND0 ) ) = ( 1UL << 0UL );
}

// 为了提高时间获取精度和效率，直接使用寄存器！
void __RAM_FUNC LPM_Phytimer_UTC_Cnt_Save(rtc_reg_t* utc_reg , uint32_t *ulTimrh_cnt, uint32_t *ulTimerml_cnt )
{
	volatile unsigned long utc_tmp;
	volatile unsigned long utc_tmp1;

	utc_tmp = HWREG(UTC_CLK_CNT);
	do{
		utc_tmp1 = HWREG(UTC_CLK_CNT);
	}
	while(utc_tmp1==utc_tmp);

	*ulTimrh_cnt = HWREG(PHYTIMER_TMRH_CNT);

	utc_reg->rtc_timer = HWREG( UTC_TIMER );
	utc_reg->rtc_cal = HWREG( UTC_CAL );
	utc_reg->rtc_cnt = HWREG( UTC_CLK_CNT );

	*ulTimerml_cnt = HWREG(PHYTIMER_TMRL_CNT);
}


//  Deepsleep、standby睡眠后 物理层phytimer补偿
void LPM_Phytimer_Compensate_Process(struct LPM_TIMER_INFO_RECOVERY_T* sleep_info)
{
	volatile uint8_t cp_div_index = 0;
	volatile uint32_t utc_int_clear;
	
	uint32_t steptick;
	uint32_t SFN_Number_after_sleep;
	uint32_t HFN_Number_after_sleep;
	uint32_t subframe_after_sleep;
	uint32_t countInSubFrame_after_sleep;
	uint32_t frc_aftersleep_sum;
	uint32_t frctime_aftersleep_cnt;

	uint32_t HFN_callback;
	uint32_t SFN_callback;
	uint32_t subframe_callback;
	uint32_t countInSubFrame_callback;

	uint64_t integer;
	uint64_t step_sys_cnt;
	uint64_t step_phy_cnt;
	uint64_t utccnt_after_sleep;

	(void)cp_div_index;

	// wait pll stable
	while( (HWREGB(0X40004076) & 0x10) != 0x10 ){;}	// pll lock
	while( HWREGB(0X40004010) != 0x8 ){;}				// pll as system clock

	// standby PLL稳定后强行再delay 200us，避免PLL不稳定导致的无故跑飞
	if(	sleep_info->sleep_type == READY_TO_STANDBY )
	{
		utc_cnt_delay(6);
	}

	#if LPM_LOG_DEBUG
	{
        g_debug_cal2 = HWREG(UTC_CAL);
        g_debug_time2 = HWREG(UTC_TIMER);
        g_debug_cnt2 = HWREG(UTC_CLK_CNT);
	}
    #endif 
    
	// stop phytimer： phtimer寄存器每个bit有不同功能，但必须按bit逐位操作才能保证功能正常运行！（按byte操作 可能会出错！）
	HWREGB(PHYTIMER_CTL) = 0x00;

	UTCAlarmDisable(UTC_ALARM_ALL);
	utc_int_clear = HWREG(UTC_INT_STAT);
	utc_int_clear++;						// no sense but avoid warning

	utccnt_after_sleep = rtc_get_cnt();

	//  enable utc trigger phytimer  mode
	HWREGB(PHYTIMER_CTL) = 0x02;

	// set utc alarm as phytimer restart event
	utccnt_after_sleep += PHYTIMER_UTC_TRIGGER_THREHOLD;
	rtc_set_alarm_by_cnt(utccnt_after_sleep);
    
    #if LPM_LOG_DEBUG
	{
        g_debug_utccnt_after_sleep = utccnt_after_sleep;
        g_debug_cal3 = HWREG(UTC_CAL);
    	g_debug_time3 = HWREG(UTC_TIMER);
    	g_debug_cnt3 = HWREG(UTC_CLK_CNT);
    }
    #endif 
    
	integer = (uint64_t)g_tcmcnt_info.mcnt_after  * (uint64_t)g_pll_divn_cur  / (uint64_t)g_pll_divn_old;
	// 由utc时间差首先计算出系统时间差，进而计算出phytimer时间差！
	step_sys_cnt = (utccnt_after_sleep - sleep_info->utccnt_before_sleep)*integer / MCNT_RC32K_COUNT;

	// 由于寄存器最小粒度限制（1/1920 ms的时间会被舍弃） 此处将小时间累加并更新寄存器，以消误差！
	step_phy_cnt = step_sys_cnt / ( sleep_info->frc_clk_divn );
	if(	sleep_info->sleep_type == READY_TO_STANDBY )
	{
		g_count_cumulate_standby += (step_sys_cnt % ( sleep_info->frc_clk_divn));
		if(g_count_cumulate_standby >= (sleep_info->frc_clk_divn))
		{
			step_phy_cnt    += g_count_cumulate_standby / (sleep_info->frc_clk_divn);
			g_count_cumulate_standby = g_count_cumulate_standby % (sleep_info->frc_clk_divn);
		}
	}
	if(	sleep_info->sleep_type == READY_TO_DEEPSLEEP )
	{
		g_count_cumulate_deepsleep += (step_sys_cnt % (sleep_info->frc_clk_divn));
		if(g_count_cumulate_deepsleep >= (sleep_info->frc_clk_divn))
		{
			step_phy_cnt    += g_count_cumulate_deepsleep / (sleep_info->frc_clk_divn);
			g_count_cumulate_deepsleep = g_count_cumulate_deepsleep % (sleep_info->frc_clk_divn);
		}
	}

	// calculate and compensate phytimer
	steptick = ( step_phy_cnt + sleep_info->countInSubFrame_before_sleep - 1)/SubFrameCntRld;
	countInSubFrame_after_sleep = (step_phy_cnt + sleep_info->countInSubFrame_before_sleep - 1)%SubFrameCntRld;
	subframe_after_sleep=(steptick + sleep_info->subframe_before_sleep)%10;	
	SFN_Number_after_sleep=sleep_info->SFN_Number_before_sleep + (steptick + sleep_info->subframe_before_sleep)/10 ;
	HFN_Number_after_sleep=sleep_info->HFN_Number_before_sleep + (sleep_info->SFN_Number_before_sleep + (sleep_info->subframe_before_sleep + steptick)/10)/1024;
	HWREG(PHYTIMER_TMRH_CNT) = ((SFN_Number_after_sleep%1024)|((HFN_Number_after_sleep%1024)<<10));
	HWREGB(PHYTIMER_TMRM_CNT) = subframe_after_sleep;
	HWREGH(PHYTIMER_TMRL_CNT) = countInSubFrame_after_sleep;
	frc_aftersleep_sum= ( (HFN_Number_after_sleep & 0x3F) * 10240 + SFN_Number_after_sleep * 10 + subframe_after_sleep) * 1920 + countInSubFrame_after_sleep;
	g_platform_trigger_phytimer = 0;

	if(	sleep_info->sleep_type == READY_TO_STANDBY )
	{
		SFW_CfgCnt_Get(SFW_CFGCNT1, &HFN_callback, &SFN_callback, &subframe_callback, &countInSubFrame_callback);
	}

    #if LPM_LOG_DEBUG
	{
        g_debug_alarm_cal1 = HWREG(UTC_ALARM_CAL);
    	g_debug_alarm_time1 = HWREG(UTC_ALARM_TIMER);
    	g_debug_alarm_cnt1 = HWREG(UTC_ALARM_CLK_CNT);

        g_debug_cal4 = HWREG(UTC_CAL);
    	g_debug_time4 = HWREG(UTC_TIMER);
    	g_debug_cnt4 = HWREG(UTC_CLK_CNT);

        g_debug_cal5 = g_debug_rtc_reg_standby_after.rtc_cal;
        g_debug_time5 = g_debug_rtc_reg_standby_after.rtc_timer;
        g_debug_cnt5 = g_debug_rtc_reg_standby_after.rtc_cnt;
    }
    #endif 

	// wait until phytimer back to running
	while( (HWREGH(PHYTIMER_STAT) & 0x100) != 0x100 );
	PhyTimerIntEnable(PHYTIMER_SFW1MATCH_INT_EN_Msk); //若callback时间极其接近时，所产生的中断可能被该接口清除，主动触发phytimer中断的容错机制应在使能接口后调用
	FRC_GetLocalFRC(&gFrcTime_after);
	
	frctime_aftersleep_cnt = ((gFrcTime_after.FRC_Reg1.HFN_Number & 0x3F)<<26)|((gFrcTime_after.FRC_Reg1.SFN_Number&0x3FF)<<16) | ((gFrcTime_after.FRC_Reg0.subframe&0xF)<<12)|(gFrcTime_after.FRC_Reg0.countInSubFrame&0xFFF);
	// phyimer补偿异常，主动触发phytimer中断以唤醒物理层
	if( HWREG(PHYTIMER_SFW1_CFG_CNT) <= frctime_aftersleep_cnt)
	{
		g_platform_trigger_phytimer = 1;
		( * ( ( volatile uint32_t * ) NVIC_PEND1 ) ) = ( 1UL << (PHYTMR_IRQn - 32) );
	}

	HWREGB(BAK_MEM_RC32K_CALI_PERMIT) = 1;

	// 通知物理层作临区检测
	if(	sleep_info->sleep_type == READY_TO_STANDBY )
	{
		g_phy_wakeup_state  = WAKEUP_FROM_STANDBY;	
	}
	else if(sleep_info->sleep_type == READY_TO_DEEPSLEEP )
	{
		g_phy_wakeup_state = WAKEUP_FROM_DEEPSLEEP;
		g_ps_wakeup_state = WAKEUP_FROM_DEEPSLEEP;
	}

#if LPM_LOG_DEBUG
	platform_trigger_phytimer_record = g_platform_trigger_phytimer;
	g_step_sys_cnt = step_sys_cnt;
	g_step_phy_cnt = step_phy_cnt;
	g_SFN_Number_after_sleep_debug = SFN_Number_after_sleep;
	g_HFN_Number_after_sleep_debug = HFN_Number_after_sleep;
	g_subframe_after_sleep_debug = subframe_after_sleep;
	g_countInSubFrame_after_sleep_debug = countInSubFrame_after_sleep;
	g_frc_aftersleep_sum = frc_aftersleep_sum;
	if(	sleep_info->sleep_type == READY_TO_STANDBY )
	{
		g_HFN_callback = HFN_callback;
		g_SFN_callback = SFN_callback;
		g_subframe_callback = subframe_callback;
		g_countInSubFrame_callback = countInSubFrame_callback;
	}
	g_utccnt_after_sleep_debug = utccnt_after_sleep;
	g_mcnt_after_debug = g_tcmcnt_info.mcnt_after;
	g_pll_divn_old_debug = g_pll_divn_old;
	g_pll_divn_cur_debug = g_pll_divn_cur;
#endif
}

// 获取平台standby睡眠时长
uint32_t LPM_GetExpectedIdleTick(uint8_t sleep_mode)
{
	uint32_t uwTaskSleepTicks, uwSwtmrSleepTicks, uwSleepTicks = 0;
	//BaseType_t pxListWasEmpty;
	if(sleep_mode == READY_TO_STANDBY)
	{
		uwTaskSleepTicks = osThreadGetLowPowerTime(osAttentionLowPowerFlag);
		uwSwtmrSleepTicks = osTimerGetLowPowerTime(osAttentionLowPowerFlag);
		uwSleepTicks = (uwTaskSleepTicks < uwSwtmrSleepTicks) ? uwTaskSleepTicks : uwSwtmrSleepTicks;
	}
	else if(sleep_mode == READY_TO_WFI)
	{
		uwSleepTicks = osThreadGetLowPowerTime(osIgnorLowPowerFlag);
	}
	
	return uwSleepTicks;
}


// 协议栈专用： 获取ptVar_F与当前时间的差值
void LPM_time_get(LPM_TIME_T* p_lpmtime)
{
	uint64_t ullsleeptime_ms;
	uint64_t time_cur_ms = get_utc_tick();
	uint64_t time_pre_ms = *(uint64_t*)((void*)Ps_Get_ptVar_F());
	
	ullsleeptime_ms = CONVERT_RTCTICK_TO_MS(time_cur_ms-time_pre_ms);
		
	p_lpmtime->ullpmtime_s = ullsleeptime_ms/1000;

	p_lpmtime->ullpmtime_us = (ullsleeptime_ms % 1000) * 1000;
}

// 协议栈专用：drx进深睡后通知物理层进行时间同步
uint32_t get_ps_wakeup_state(void)
{
	return g_ps_wakeup_state;
}

// 物理层专用：判断本次上电是否成功进行了深睡恢复，并重置标记位
uint32_t get_phy_wakeup_state(void)
{	
    uint32_t ulRetRslt = g_phy_wakeup_state;
    
    g_phy_wakeup_state = NORMAL;
	
	return ulRetRslt;
}

// 物理层专用： 获取睡眠NV是否开启(Deepsleep、standby)
uint32_t get_phy_lowerpower_enable_state(void)
{
    return (g_factory_nv->softap_fac_nv.deepsleep_enable | g_factory_nv->softap_fac_nv.lpm_standby_enable);
}

// 物理层专用：根据睡眠时长修正MCNT校准值
void Set32k_To_HCLK(int32_t diffval,uint32_t sleepTime)
{
	uint64_t cal_tmp;
	uint32_t old_32k_2_HCLK;
	uint32_t divn_temp;

	osCoreEnterCritical();
	old_32k_2_HCLK = g_Hclk_mcnt_cnt;
	osCoreExitCritical();

	(void)diffval;
	if( ( sleepTime < 500 ) || ( g_ucmcnt_feedback == 0 ) || (0 == old_32k_2_HCLK) )
	{
	
	}
    else
    {
		// g_Hclk_mcnt_cnt被物理层更新，因此pll分频系数保持与之同步
		divn_temp = get_pll_divn();
		if(  g_pll_divn_old != 1)
		{
			cal_tmp =  (uint64_t)old_32k_2_HCLK * (uint64_t)divn_temp / (uint64_t)g_pll_divn_old;

			osCoreEnterCritical();
			g_Hclk_mcnt_cnt = (uint32_t)cal_tmp;
			osCoreExitCritical();
		} 
		g_pll_divn_old = divn_temp;

	}
	PrintLog(0,PLATFORM,WARN_LOG,"UPDATE MCNT old:%d, new:%d", old_32k_2_HCLK , g_Hclk_mcnt_cnt );
}

// 物理层专用：开启MCNT动态反馈功能
void Mcnt_Set_Enable(uint8_t ucmcnt_en_flag)
{
	osCoreEnterCritical();
    ucmcnt_recal_flag = ucmcnt_en_flag;
	osCoreExitCritical();

}

// 物理层专用：开启MCNT动态反馈
void Mcnt_Feedback_Start()
{
	if(g_ucmcnt_feedback == 0)
		g_ucmcnt_feedback = 1;
}

// 物理层专用：关闭MCNT动态反馈功能
void Mcnt_Feedback_End()
{
	g_ucmcnt_feedback = 0;

}

// 物理层专用： 获取MCNT校准值
uint32_t Mcnt_Get_Phy()
{
    return g_Hclk_mcnt_cnt;
}

// 获取当前pll分频系数
uint32_t get_pll_divn(void)
{
	volatile uint32_t pll_divn_cur = 0;

	pll_divn_cur = (((uint32_t)HWREGH(COREPRCM_ADIF_BASE + 0x30))*0xffffff) + (HWREG(COREPRCM_ADIF_BASE + 0x34) & 0xffffff);

	return pll_divn_cur;
}

// 根据PLL变化，调整MCNT
void Mcnt_Adjust(void)
{	
	g_tcmcnt_info.us_adjust = 0;
	g_tcmcnt_info.mcnt_after = g_Hclk_mcnt_cnt;		


	if(g_ucmcnt_feedback)
	{
		g_pll_divn_old = 1;
		g_pll_divn_cur = 1;
		return;
	}
	
	if( g_pll_divn_old == 0 || g_pll_divn_old == 1 )
	{
		g_pll_divn_old = 1;
		g_pll_divn_cur = 1;
	}
	else
	{
		g_pll_divn_cur = get_pll_divn();
	}
}

// MCNT中断函数
void MCNT_Handler(void)
{
#if RUNTIME_DEBUG
	extern uint32_t xy_runtime_get_enter(void);
	uint32_t time_enter = xy_runtime_get_enter();
#endif

    __set_PRIMASK(1);

	g_Hclk_mcnt_cnt = HWREG(MCNT_CNTMEASURE) -1;
	g_mcnt_finished = 1;

     __set_PRIMASK(0);

#if RUNTIME_DEBUG
 	extern void xy_runtime_get_exit(uint32_t id, uint32_t time_enter);
	xy_runtime_get_exit(MCNT_IRQn, time_enter);
#endif

}

// MCNT频率校准流程
uint32_t process_do_mcnt(uint32_t ulutccnt)
{
	volatile uint32_t g_mcnt_delay = 0;

	NVIC_IntRegister(MCNT_IRQn, MCNT_Handler, 1);

	NVIC_ClearIntPending(MCNT_IRQn);

	// 32k as clock source
	MCNTSetClkSrc(0);

	// must delay before stop mcnt
	for( g_mcnt_delay = 0 ; g_mcnt_delay < 100 ; g_mcnt_delay++ );

	MCNTStop();

	MCNTSetCNT32K(ulutccnt);

	MCNTStart();

	return 0;
}

// 获取MCNT校准、已校准直接返回、未校准、温度剧烈变化、物理层请求则重新进行校准
#if LPM_LOG_DEBUG
volatile FRC_TIME_t frc_temperature_debug_b;
volatile FRC_TIME_t frc_temperature_debug_a;
volatile FRC_TIME_t frc_sleep_time_debug_b;
#endif

#define TEMPERATURE_CONVERT_INTO_INDEX(sctemperature)  (((sctemperature) + 50) / 2) 
extern void Set_32K_Freq(unsigned int freq_32k);
uint32_t Mcnt_Get(void)
{
	int32_t sctemperature;
	uint32_t pll_divn;
	uint32_t mcnt_cnt;
	uint32_t rc32k_freq;
	uint64_t freq_pll;
	xtal_freq_t temp_info = {0xFFFFFFFF, 0xFFFFFFFF};
	volatile uint32_t ulTimrh_cnt_debug=0;
	volatile uint32_t ulTimerml_cnt_debug=0;

	if (PRCM_32KClkSrcGet() == 1)
	{
		// 外部32k使用ADC读取温度
		sctemperature = TEMPERATURE_CONVERT_INTO_INDEX(get_adc_value_incpcore(ADC_TSENSOR));
		// 支持温度范围-40~130度，因此需要确保index处于合理范围内
		if(sctemperature > 90 )
		{
			sctemperature = 90;
		}
		else if(sctemperature < 0)
		{
			sctemperature = 0;
		}
	}
	else
	{
#if LPM_LOG_DEBUG
		ulTimrh_cnt_debug=HWREG(PHYTIMER_TMRH_CNT);
		ulTimerml_cnt_debug=HWREG(PHYTIMER_TMRL_CNT);
		frc_temperature_debug_b.FRC_Reg1.SFN_Number = ulTimrh_cnt_debug&0x3FF;
		frc_temperature_debug_b.FRC_Reg1.HFN_Number = (ulTimrh_cnt_debug>>10)&0x3FF;
		frc_temperature_debug_b.FRC_Reg0.countInSubFrame = ulTimerml_cnt_debug&0xFFF;
		frc_temperature_debug_b.FRC_Reg0.subframe=(ulTimerml_cnt_debug>>16)&0xF;
#endif

		// RC 32k使用lpts读取温度
		sctemperature = rc32k_get_last_temperature();	

#if LPM_LOG_DEBUG
		ulTimrh_cnt_debug=HWREG(PHYTIMER_TMRH_CNT);
		ulTimerml_cnt_debug=HWREG(PHYTIMER_TMRL_CNT);
		frc_temperature_debug_a.FRC_Reg1.SFN_Number = ulTimrh_cnt_debug&0x3FF;
		frc_temperature_debug_a.FRC_Reg1.HFN_Number = (ulTimrh_cnt_debug>>10)&0x3FF;
		frc_temperature_debug_a.FRC_Reg0.countInSubFrame = ulTimerml_cnt_debug&0xFFF;
		frc_temperature_debug_a.FRC_Reg0.subframe=(ulTimerml_cnt_debug>>16)&0xF;
#endif
	}

	// 从频率表中找到对应项
	temp_info.mcnt_cnt = *(uint32_t *)(CALIB_FREQ_BASE+OFFSET_CALI_PRAM(xtal_freq_tbl)+8*sctemperature + 4); //+4是跳过ftl头
	temp_info.pll_divn = *(uint32_t *)(CALIB_FREQ_BASE+OFFSET_CALI_PRAM(xtal_freq_tbl)+8*sctemperature + 4 + 4);

	if(temp_info.pll_divn == 0xFFFFFFFF || temp_info.mcnt_cnt == 0xFFFFFFFF)
	{
		// xtal32k
		if (PRCM_32KClkSrcGet() == 1)
		{
			// 外部32k晶振，测量并完善频率表
			rc32k_get_count_by_mcnt(MCNT_RC32K_COUNT, &mcnt_cnt, &rc32k_freq, &pll_divn);
			temp_info.mcnt_cnt = mcnt_cnt;
			temp_info.pll_divn = pll_divn;
			xy_ftl_write(CALIB_FREQ_BASE+OFFSET_CALI_PRAM(xtal_freq_tbl)+8*sctemperature, (void *)&temp_info, 8);

			Set_32K_Freq(rc32k_freq);
			
			osCoreEnterCritical();
			g_Hclk_mcnt_cnt = mcnt_cnt;
			g_pll_divn_old = pll_divn;
			osCoreExitCritical();

			xy_printf(0,XYAPP, WARN_LOG, "lpm xtal32k sctemperature:%d, mcnt_cnt_addr:%x mcnt_cnt:%d, pll_div:%d, freq:%d \r\n",sctemperature, 8*sctemperature, mcnt_cnt, pll_divn, rc32k_freq );

		}
		else if(PRCM_32KClkSrcGet() == 0)
		{
			osCoreEnterCritical();
			// 32000
			g_Hclk_mcnt_cnt = 36863997;
			g_pll_divn_old = 475750195;
			osCoreExitCritical();

			Set_32K_Freq(32000);
		}
	}
	else
	{
		// 根据频率表，计算频率
		freq_pll = ((uint64_t)temp_info.pll_divn >> 24) * 13000000 + ((uint64_t)temp_info.pll_divn & 0xFFFFFF) * 13000000 / 0xffffff;
		rc32k_freq = freq_pll * (uint64_t)MCNT_RC32K_COUNT / (uint64_t)temp_info.mcnt_cnt;
		
		if (PRCM_32KClkSrcGet() == 1)
		{
			// xtal32k 动态频率
			osCoreEnterCritical();
			g_Hclk_mcnt_cnt = temp_info.mcnt_cnt;
			g_pll_divn_old = temp_info.pll_divn;
			osCoreExitCritical();
			Set_32K_Freq(rc32k_freq);
		}
		else if(PRCM_32KClkSrcGet() == 0)
		{
			// RC32K 固定32000
			osCoreEnterCritical();
			g_Hclk_mcnt_cnt = 36863997;
			g_pll_divn_old = 475750195;
			osCoreExitCritical();
			Set_32K_Freq(32000);
		}

	}
	gTempera_old = sctemperature;
	Mcnt_Set_Enable(0);

#if LPM_LOG_DEBUG
	ulTimrh_cnt_debug=HWREG(PHYTIMER_TMRH_CNT);
	ulTimerml_cnt_debug=HWREG(PHYTIMER_TMRL_CNT);
	frc_sleep_time_debug_b.FRC_Reg1.SFN_Number = ulTimrh_cnt_debug&0x3FF;
	frc_sleep_time_debug_b.FRC_Reg1.HFN_Number = (ulTimrh_cnt_debug>>10)&0x3FF;
	frc_sleep_time_debug_b.FRC_Reg0.countInSubFrame = ulTimerml_cnt_debug&0xFFF;
	frc_sleep_time_debug_b.FRC_Reg0.subframe=(ulTimerml_cnt_debug>>16)&0xF;
#endif
	return g_Hclk_mcnt_cnt;
}

uint32_t WFI_Admittance_Check()
{
	if(  g_factory_nv->softap_fac_nv.wfi_enable != 1 )
	{
		return NOT_ALLOW_WFI_NV_OFF;
	}

    if(is_sleep_locked(LPM_WFI))
	{
		return NOT_ALLOW_WFI_LOCK_UNRELEASE;
	}

	if(HWREGB(BAK_MEM_AP_LOCK_TYPE)&4)
	{
		return NOT_ALLOW_WFI_LOCK_UNRELEASE;
	}
	
	if((diag_list_is_all_list_empty() == 0) || (diag_port_is_send_complete() == 0))
	{
	 	return NOT_ALLOW_WFI_LOG_UNFINISHED;
	}

    // 判断当前AT串口是否有未处理的数据，有则处理此数据，并退出睡眠
	if (at_uart_check_buffer_and_process() != 0)
	{
		return NOT_ALLOW_WFI_AT_UNHANDLE;
	}
    
	return ALLOW_WFI;
}

uint32_t WFI_Cal_SleepTime_Again()
{
    /*
	uint32_t uwSleepTicks;
	
	uwSleepTicks = LPM_GetExpectedIdleTick(READY_TO_WFI); 

	if(uwSleepTicks < LPM_WFI_TICKLESS_MIN)
	{
		return 0;
	}
	else
	    uwSleepTicks = MIN(LPM_WFI_TICKLESS_MAX, uwSleepTicks);

	return uwSleepTicks;
	*/



	ps_next_work_time(&Ps_Lpminfo);
	if((Ps_Lpminfo.state == KEEP_ACTIVE) || (Ps_Lpminfo.state == READY_TO_WFI) )
	{
		return 1;
	}

	return 0;
}

void WFI_Entry()
{
	HWREGB(0x40004030) = 0;

    SCB->SCR &= ~0x4;

	HWREGB(BAK_MEM_BOOT_CP_SYNC) = CP_IN_WORK;

	__asm__ ("WFI");
}

extern volatile int standby_degrade_flag;
int __RAM_FUNC Lpm_WFI_Process(void)  
{
	int ret = ALLOW_WFI;
    uint32_t wfi_tickcnt_before,wfi_tickcnt_after;

	vLpmEnterCritical();

	ret = WFI_Admittance_Check();
	if(ret != ALLOW_WFI)
	{
		vLpmExitCritical();
		return ret;
	}
    
    if(g_softap_fac_nv->watchdog_enable)
	{
		WatchdogDisable(CP_WDT_BASE);
	}

#if LPM_LOG_DEBUG    
    wfi_tickcnt_before = TickCounterGet();
#endif

	WFI_Entry();

#if LPM_LOG_DEBUG    
    wfi_tickcnt_after = TickCounterGet();
#endif   

    if(g_softap_fac_nv->watchdog_enable)
	{
		WatchdogEnable(CP_WDT_BASE);
	}

#if LPM_LOG_DEBUG    
    g_wfi_sleep_ms = (uint64_t)(wfi_tickcnt_after-wfi_tickcnt_before);
#endif
	vLpmExitCritical();

#if LPM_LOG_DEBUG
    if((standby_degrade_flag == 1) || (g_wfi_sleep_ms >= 15))
	{
		standby_degrade_flag = 0;
		xy_printf(0,PLATFORM, WARN_LOG, "LPM_WFI: %lld", g_wfi_sleep_ms);
	}
#endif

	return ALLOW_WFI;
}

void del_realtime_timer(osThreadId_t thread_id)
{
	osMutexAcquire(g_realtime_timer_mutex, osWaitForever);
	if (g_realtime_timer_list != NULL)
	{
		realtime_timer_t *list_prev = NULL;	
		realtime_timer_t *list = g_realtime_timer_list;

		// 节点在os timer超时回调或者云业务自行调用此接口删除
		while (list->thread_id != thread_id && list->next != NULL)
		{
			list_prev = list;
			list = list->next;
		}
		if (list->thread_id == thread_id)
		{
			if (list == g_realtime_timer_list)
				g_realtime_timer_list = list->next;
			else
				list_prev->next = list->next;
			xy_printf(0, XYAPP, WARN_LOG, "del_realtime_timer:0x%X,0x%X,0x%X", thread_id, g_realtime_timer_list, list_prev);
			// 删除定时器
			osTimerDelete(list->timer_id);
			list->timer_id = NULL;		
			xy_free(list);
		}
	}
	osMutexRelease(g_realtime_timer_mutex);
}

void realtime_timer_calback(osTimerId_t timer_id)
{	
	realtime_timer_t *list = (realtime_timer_t *)osTimerGetCallbackArgs(timer_id);
    osThreadSetLowPowerFlag(list->thread_id, osLpmNoRealtime);
}

/*对默认非实时的云业务线程，对于某些远程通信流程需要分多步才能轮询处理完毕场景，通过该接口来保证轮询周期定时器的实时性*/
void set_realtime_in_standby(int sec, osThreadId_t thread_id)
{
	realtime_timer_t *list;

	if (g_realtime_timer_mutex == NULL)
		g_realtime_timer_mutex = osMutexNew(NULL);
	osMutexAcquire(g_realtime_timer_mutex, osWaitForever);
	for (list = g_realtime_timer_list; list != NULL; list = list->next)
	{
		// 遍历链表，若已经创建过，直接刷新定时器
		if ( ((thread_id == NULL) && (list->thread_id == osThreadGetId())) || ((thread_id != NULL) && (list->thread_id == thread_id)) )
			goto OUT;				
	}

	list = (realtime_timer_t *)xy_malloc(sizeof(realtime_timer_t));
	if (thread_id == NULL)
		list->thread_id = osThreadGetId();
	else
		list->thread_id = thread_id;
		
	osTimerAttr_t timer_attr = {0};
	timer_attr.name = "realtime_timer";
	list->timer_id = osTimerNew((osTimerFunc_t)(realtime_timer_calback), osTimerOnce, list, &timer_attr);
	list->next = g_realtime_timer_list;
	g_realtime_timer_list = list; 	

OUT:
	osThreadSetLowPowerFlag(list->thread_id, osLpmRealtime);
	osMutexRelease(g_realtime_timer_mutex);
	xy_printf(0, XYAPP, WARN_LOG, "set_realtime_in_standby:0x%X,0x%X,0x%X,%d", list->thread_id, list->timer_id, g_realtime_timer_list, sec);
	xy_assert(list->timer_id != NULL);
	osTimerStart(list->timer_id, sec * 1000);		
}

//深睡查询专用，返回系统上一次进入idle的时刻点
uint64_t get_tick_enter_idle(void)
{
	return g_idle_tick;
}

//深睡查询专用，返回上一次退出idle线程时的深睡状态
int get_sleep_state(void)
{
	return g_sleep_check;
}
