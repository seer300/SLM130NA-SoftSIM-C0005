#include "cmsis_device.h"
#include "xy_memmap.h"
#include "gpio.h"
#include "cache.h"
#include "prcm.h"
#include "fast_recovery.h"
#include "low_power.h"
#include "hw_utc.h"

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
volatile uint32_t g_db_FR_begin_timer_alarm;
volatile uint32_t g_db_FR_begin_cnt_alarm;
volatile uint32_t g_db_FR_begin_timer;
volatile uint32_t g_db_FR_begin_cnt;
volatile uint32_t g_db_FR_end_timer;
volatile uint32_t g_db_FR_end_cnt;
#endif


volatile uint32_t g_db_fafeweret_end;

/*快速深睡唤醒后，需要对关键全局进行初始化，以防止上次深睡时没有清除*/
void Sys_Init_By_FastRecovery(void)
{

}

// 在 Reset_Handler 的第一行被调，判断是否是快速启动，执行快速恢复
void first_excute_in_reset_handler(void)
{
#if CP_FAST_RECOVERY_FUNCTION
	if (Is_WakeUp_From_Dsleep() && g_fast_startup_flag == CP_WAKEUP_FASTRECOVERY_BEFORE)
	{

		#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
			g_db_FR_begin_timer_alarm = HWREG(UTC_ALARM_TIMER);
			g_db_FR_begin_cnt_alarm = HWREG(UTC_ALARM_CLK_CNT);
			g_db_FR_begin_timer = HWREG(UTC_TIMER);
			g_db_FR_begin_cnt = HWREG(UTC_CLK_CNT);
		#endif

		g_fast_startup_flag = CP_WAKEUP_FASTRECOVERY_AFTER;

		Sys_Init_By_FastRecovery();

		restore_msp();

		FastSystemInit();

		#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
			g_db_FR_end_timer = HWREG(UTC_TIMER);
			g_db_FR_end_cnt = HWREG(UTC_CLK_CNT);
		#endif

		// 必须在当前函数的最后调用，执行下面的函数时，当前函数不会返回，会直接跳转到深睡前的地方执行
		restore_scene_and_running();
	}
#endif
}

void SystemInit(void)
{
	// enable some trigger condition of hard fault
	SCB->CCR = SCB_CCR_STKALIGN_Msk | SCB_CCR_DIV_0_TRP_Msk;

	PRCM_ClockEnable((uint64_t)0xF88EFCEE);
	// flash cache initialize
	CacheInit(CP_CACHE_BASE, CACHE_CCR0_WRITE_BACK, CP_CODE_START_ADDR, CP_CODE_START_ADDR + CP_CODE_LENGTH - 1);
	//	CacheCRESet(CORE_CACHE_BASE, (1 << 2) | (1 << 3));
	//	CacheEntryCacheDis(CORE_CACHE_BASE);

	NVIC_SetVectorTable(g_pfnVectors);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);
}
