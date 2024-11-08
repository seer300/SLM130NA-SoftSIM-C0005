#include "xinyi_nvic.h"
#include "fast_recovery.h"
#include "hw_types.h"
#include "hw_prcm.h"
#include "prcm.h"
#include "xy_memmap.h"
#include "xy_system.h"
#include "hw_gpio.h"
#include "low_power.h"
#include "cache.h"
#include "xinyi_hardware.h"
#include "mpu_protect.h"
#include "cmsis_os2.h"
#include "at_ctl.h"
#include "main_proxy.h"
#include "xy_log.h"
#include "at_uart.h"
#include "dfe.h"
#include "flash_adapt.h"
#include "xy_flash.h"
#include "oss_nv.h"
#include "ipc_msg.h"
#include "gpio.h"
#include "dma.h"
#include "xy_utils.h"
#include "xy_wdt.h"
#include "cloud_utils.h"
#include "common.h"
#include "sys_config.h"
#include "phytimer.h"
#include "hw_utc.h"
#include "ipc_msg.h"

volatile uint32_t g_fast_startup_flag = CP_WAKEUP_NORMAL;

#if CP_FAST_RECOVERY_FUNCTION

nvic_scene_t g_nvic_scene = {0};
m3_fast_recover_reg_t g_m3_fast_recover_reg = {0};
gpio_scene_t gpio_scene = {0};

extern void lpm_time_enter_wfi(void);
// 在 save_scene_and_wfi 中调用
void save_scene_hook_before_wfi(void)
{
	// 保存NVIC部分寄存器
	g_nvic_scene.CTRL = SysTick->CTRL;
	g_nvic_scene.LOAD = SysTick->LOAD;
	g_nvic_scene.ISER[0] = NVIC->ISER[0];
	g_nvic_scene.ISER[1] = NVIC->ISER[1];

	//外部中断仅仅保存CLKTIM、UTC、LPUART、LPTIM、QSPI、WAKEUP、GPIO的中断优先级
	g_nvic_scene.IP[0] = NVIC->IP[CLKTIM_IRQn];
	g_nvic_scene.IP[1] = NVIC->IP[UTC_IRQn];
	g_nvic_scene.IP[2] = NVIC->IP[BB_IRQn];
	g_nvic_scene.IP[3] = NVIC->IP[RC32K_IRQn];
	g_nvic_scene.IP[4] = NVIC->IP[QSPI_IRQn];
	g_nvic_scene.IP[5] = NVIC->IP[WAKEUP_IRQn];
	g_nvic_scene.IP[6] = NVIC->IP[GPIO_IRQn];
	g_nvic_scene.IP[7] = NVIC->IP[PHYTMR_IRQn];
	
	g_nvic_scene.VTOR = SCB->VTOR;
	g_nvic_scene.AIRCR = (SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk) | 0x05FA;
	g_nvic_scene.CCR = SCB->CCR;

	//内部中断仅仅保存MemoryManagement、BusFault、UsageFault、SVCall、DebugMonitor、PendSV、SysTick的中断优先级
	g_nvic_scene.SHP[0] = SCB->SHP[0];//MemoryManagement
	g_nvic_scene.SHP[1] = SCB->SHP[1];//BusFault	
	g_nvic_scene.SHP[2] = SCB->SHP[2];//UsageFault
	g_nvic_scene.SHP[3] = SCB->SHP[7];//SVCall
	g_nvic_scene.SHP[4] = SCB->SHP[8];//DebugMonitor
	g_nvic_scene.SHP[5] = SCB->SHP[10];//PendSV
	g_nvic_scene.SHP[6] = SCB->SHP[11];//SysTick

	// gpio_scene.CTRL0           = GPIO->MODE[0];
	// gpio_scene.PULLUP0         = GPIO->PULL_UP[0];
	// gpio_scene.PULLDOWN0       = GPIO->PULL_DOWN[0];
	// gpio_scene.INPUTEN0        = GPIO->INPUT_EN[0];
	// gpio_scene.OUTPUTEN0       = GPIO->OUTPUT_EN[0];
	// gpio_scene.CFGSEL0         = GPIO->CTL[0];
	// gpio_scene.PAD_SEL2        = GPIO->PERI[2];
	// gpio_scene.PAD_SEL3        = GPIO->PERI[3];
	// gpio_scene.PERI_IN_SEL2    = GPIO->PERILIN[GPIO_AP_SWCLKTCK];
	// gpio_scene.PERI_IN_SEL3    = GPIO->PERILIN[GPIO_AP_SWDIOTMS];
	// gpio_scene.PERI_IN_SEL_EN1 = GPIO->PERILIN_EN[1];

	// CP更新当前状态，供AP裁决睡眠
	HWREGB(BAK_MEM_BOOT_CP_SYNC) = CP_IN_DEEPSLEEP_FAST_RECOVERY;
	if(Ipc_SetInt() == 0)
	{
#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
		lpm_time_enter_wfi();
#endif

		__WFI();
	}
}

void restore_scene_hook_after_wfi(void)
{
	// 恢复NVIC部分寄存器
	SysTick->LOAD = g_nvic_scene.LOAD;
	SysTick->CTRL = g_nvic_scene.CTRL;
	SysTick->VAL = 0;
	NVIC->ISER[0] = g_nvic_scene.ISER[0];
	NVIC->ISER[1] = g_nvic_scene.ISER[1];

	//外部中断仅仅恢复CLKTIM、UTC、LPUART、LPTIM、QSPI、WAKEUP、GPIO的中断优先级
	NVIC->IP[CLKTIM_IRQn] = g_nvic_scene.IP[0];
	NVIC->IP[UTC_IRQn] 	  = g_nvic_scene.IP[1];
	NVIC->IP[BB_IRQn] = g_nvic_scene.IP[2];
	NVIC->IP[RC32K_IRQn]  = g_nvic_scene.IP[3];
	NVIC->IP[QSPI_IRQn]   = g_nvic_scene.IP[4];
	NVIC->IP[WAKEUP_IRQn] = g_nvic_scene.IP[5];
	NVIC->IP[GPIO_IRQn]   = g_nvic_scene.IP[6];
	NVIC->IP[PHYTMR_IRQn] = g_nvic_scene.IP[7];

	SCB->VTOR = g_nvic_scene.VTOR;
	SCB->AIRCR = g_nvic_scene.AIRCR;
	SCB->CCR = g_nvic_scene.CCR;
	
	//内部中断仅仅恢复MemoryManagement、BusFault、UsageFault、SVCall、DebugMonitor、PendSV、SysTick的中断优先级
	SCB->SHP[0]  = g_nvic_scene.SHP[0];//MemoryManagement
	SCB->SHP[1]  = g_nvic_scene.SHP[1];//BusFault
	SCB->SHP[2]  = g_nvic_scene.SHP[2];//UsageFault
	SCB->SHP[7]  = g_nvic_scene.SHP[3];//SVCall
	SCB->SHP[8]  = g_nvic_scene.SHP[4];//DebugMonitor
	SCB->SHP[10] = g_nvic_scene.SHP[5];//PendSV
	SCB->SHP[11] = g_nvic_scene.SHP[6];//SysTick
}

void restore_gpio_scene_after_wfi(void)
{
	GPIO->PCFT[0]                  |= 0xFFFFFFFF;
	GPIO->PERI[2] 					= gpio_scene.PAD_SEL2;
	GPIO->PERI[3] 					= gpio_scene.PAD_SEL3;
	GPIO->PERILIN[GPIO_AP_SWCLKTCK] = gpio_scene.PERI_IN_SEL2;
	GPIO->PERILIN[GPIO_AP_SWDIOTMS] = gpio_scene.PERI_IN_SEL3;
	GPIO->PERILIN_EN[1] 			= gpio_scene.PERI_IN_SEL_EN1;
	GPIO->MODE[0] 					= gpio_scene.CTRL0;
	GPIO->CTL[0] 					= gpio_scene.CFGSEL0;
	GPIO->INPUT_EN[0]				= gpio_scene.INPUTEN0;
	GPIO->OUTPUT_EN[0]				= gpio_scene.OUTPUTEN0;
	GPIO->PULL_UP[0] 				= gpio_scene.PULLUP0;
	GPIO->PULL_DOWN[0] 				= gpio_scene.PULLDOWN0;
}

extern void rf_sido_init(void);
extern void IpcMsg_init();
extern void diag_port_hardware_init(void);
extern void user_led_pin_init(void);
extern void DeepSleep_Recovery(void);
extern void PHY_EventExecuteInit_Fast(void);
extern volatile uint32_t g_sfw_cfgcnt;
extern void reset_cp_rtcinfo(void);
extern void Freq32k_Init_Process(void);

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
volatile uint32_t g_db_FR_resetore_timer;
volatile uint32_t g_db_FR_resetore_cnt;
volatile uint32_t g_db_FR_rf_sido_timer;
volatile uint32_t g_db_FR_rf_sido_cnt;
volatile uint32_t g_db_FR_mpu_protect_timer;
volatile uint32_t g_db_FR_mpu_protect_cnt;
volatile uint32_t g_db_FR_diag_port_timer;
volatile uint32_t g_db_FR_diag_port_cnt;
volatile uint32_t g_db_FR_at_uart_fast_timer;
volatile uint32_t g_db_FR_at_uart_fast_cnt;
volatile uint32_t g_db_FR_rf_uart_timer;
volatile uint32_t g_db_FR_rf_uart_cnt;
volatile uint32_t g_db_FR_Sys_Up_URC_timer;
volatile uint32_t g_db_FR_Sys_Up_URC_cnt;
volatile uint32_t g_db_FR_xy_delaylock_timer;
volatile uint32_t g_db_FR_xy_delaylock_cnt;
volatile uint32_t g_db_FR_user_led_timer;
volatile uint32_t g_db_FR_user_led_cnt;
volatile uint32_t g_db_FR_wdt_timer;
volatile uint32_t g_db_FR_wdt_cnt;
volatile uint32_t g_db_FR_SFW_CfgCnt_timer;
volatile uint32_t g_db_FR_SFW_CfgCnt_cnt;
volatile uint32_t g_db_FR_DeepSleep_Recovery_timer;
volatile uint32_t g_db_FR_DeepSleep_Recovery_cnt;
volatile uint32_t g_db_FR_PHY_Event_timer;
volatile uint32_t g_db_FR_PHY_Event_cnt;
#endif

void FastSystemInit(void)
{
	// PRCM_CKG_CTL_UTC_EN、PRCM_CKG_CTL_AES_EN
	PRCM_ClockEnable((uint64_t)0xF88EFCEE);

	CacheInit(CP_CACHE_BASE, CACHE_CCR0_WRITE_BACK, CP_CODE_START_ADDR, CP_CODE_START_ADDR + CP_CODE_LENGTH - 1);

	NVIC_SetVectorTable(g_pfnVectors);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);

	COREPRCM->RST_CTRL |= 0x1000;//release dfe

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_FR_resetore_timer = HWREG(UTC_TIMER);
	g_db_FR_resetore_cnt = HWREG(UTC_CLK_CNT);
#endif

	rf_sido_init();
#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_FR_rf_sido_timer = HWREG(UTC_TIMER);
	g_db_FR_rf_sido_cnt = HWREG(UTC_CLK_CNT);
#endif

	mpu_protect_init();
#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_FR_mpu_protect_timer = HWREG(UTC_TIMER);
	g_db_FR_mpu_protect_cnt = HWREG(UTC_CLK_CNT);
#endif


    diag_port_hardware_init();

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_FR_diag_port_timer = HWREG(UTC_TIMER);
	g_db_FR_diag_port_cnt = HWREG(UTC_CLK_CNT);
#endif

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_FR_at_uart_fast_timer = HWREG(UTC_TIMER);
	g_db_FR_at_uart_fast_cnt = HWREG(UTC_CLK_CNT);
#endif

	PhyTimerInit();//rf_uart_init();
	
#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_FR_rf_uart_timer = HWREG(UTC_TIMER);
	g_db_FR_rf_uart_cnt = HWREG(UTC_CLK_CNT);
#endif

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_FR_Sys_Up_URC_timer = HWREG(UTC_TIMER);
	g_db_FR_Sys_Up_URC_cnt = HWREG(UTC_CLK_CNT);
#endif

	/*外部MCU触发唤醒，必须启动延迟锁，否则云通信中有sleep操作会释放调度进入idle深睡*/
	if (!IS_WAKEUP_BY_SOC())
	{
		if(g_softap_fac_nv->deepsleep_delay != 0)
		{
			at_delaylock_act();
		}
	}
	
	
#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_FR_xy_delaylock_timer = HWREG(UTC_TIMER);
	g_db_FR_xy_delaylock_cnt = HWREG(UTC_CLK_CNT);
#endif

	if((g_softap_fac_nv != NULL) && (g_softap_fac_nv->led_pin <= GPIO_PAD_NUM_63))
	{
		user_led_pin_init();	
	}
#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_FR_user_led_timer = HWREG(UTC_TIMER);
	g_db_FR_user_led_cnt = HWREG(UTC_CLK_CNT);
#endif

	wdt_init();
#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_FR_wdt_timer = HWREG(UTC_TIMER);
	g_db_FR_wdt_cnt = HWREG(UTC_CLK_CNT);
#endif

	SFW_CfgCnt_Set_Reg(SFW_CFGCNT1, g_sfw_cfgcnt);
#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_FR_SFW_CfgCnt_timer = HWREG(UTC_TIMER);
	g_db_FR_SFW_CfgCnt_cnt = HWREG(UTC_CLK_CNT);
#endif

    // tau NV未使能，深睡唤醒清rtc链表
	if(HWREGB(BAK_MEM_CP_SET_RTC) == 0)
	{
		reset_cp_rtcinfo();
	}

	Freq32k_Init_Process();
	
	DeepSleep_Recovery();
#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_FR_DeepSleep_Recovery_timer = HWREG(UTC_TIMER);
	g_db_FR_DeepSleep_Recovery_cnt = HWREG(UTC_CLK_CNT);
#endif

    PHY_EventExecuteInit_Fast();
#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_FR_PHY_Event_timer = HWREG(UTC_TIMER);
	g_db_FR_PHY_Event_cnt = HWREG(UTC_CLK_CNT);
#endif
}

#endif
