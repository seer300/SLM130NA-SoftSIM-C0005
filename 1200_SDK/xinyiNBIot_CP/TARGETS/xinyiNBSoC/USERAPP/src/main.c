#include "xinyi_hardware.h"
#include "mpu_protect.h"
#include "cmsis_os2.h"
#include "at_ctl.h"
#include "main_proxy.h"
#include "xy_log.h"
#include "at_uart.h"
//#include "PhyL1cFlash.h"
#include "dfe.h"
#include "flash_adapt.h"
#include "xy_flash.h"
#include "oss_nv.h"
#include "ipc_msg.h"
#include "prcm.h"
#include "gpio.h"
#include "dma.h"
#include "xy_utils.h"
#include "xy_wdt.h"
#include "cloud_utils.h"
#include "common.h"
#include "low_power.h"
#include "sys_config.h"
#include "fs_al.h"
#include "NBPhyL1cExportInterface.h"
#include "hw_utc.h"
#include "utc.h"
#include "osAssitantUtils.h"

#define _PHY_INIT_
#define _PS_INIT_


rf_normal_mode_t g_rf_mt;
#ifdef _PS_INIT_
extern void PsInitReq();
extern void PsStartReq();
extern  void PsInitAsn1GlobalValueAndFuncAddr();
extern unsigned char PsRdInVarFrmFlash();
extern void WAKEUP_Handler(void);
#include "xy_atc_interface.h"

extern void rf_sido_init(void);
extern void IpcMsg_init();
extern void adc_init(void);
extern void net_led_init(void);
extern int LPM_Init(void);
extern void Freq32k_Init_Process(void);
extern void DeepSleep_Recovery(void);
extern uint8_t RF_isManufactryMode(void);

void PsInit(void)
{
    PRCM_ClockEnable(CORE_CKG_CTL_AES_EN);
	PsInitAsn1GlobalValueAndFuncAddr();
    PsInitReq();
    PsStartReq();
}
#endif


extern unsigned char   gPowerOnMode;
extern unsigned char   g_PsRdInVarFrmFlash_Result;
extern void rtc_init(void);

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
//#define TICK_COUNTER_REG			( * ( ( volatile uint32_t * ) 0x4000e014 ) )
#define UTC_RECORD_TIMER            ( * ( ( volatile uint32_t * ) 0x40001008 ) )
#define UTC_RECORD_CNT              ( * ( ( volatile uint32_t * ) 0x40001034 ) )
volatile uint32_t g_db_cp_main_timer;
volatile uint32_t g_db_cp_main_cnt;
volatile uint32_t g_db_rf_sido_timer;
volatile uint32_t g_db_rf_sido_cnt;
volatile uint32_t g_db_mpu_protect_init_timer;
volatile uint32_t g_db_mpu_protect_init_cnt;
volatile uint32_t g_db_rtc_init_timer;
volatile uint32_t g_db_rtc_init_cnt;
volatile uint32_t g_db_IpcMsg_init_timer;
volatile uint32_t g_db_IpcMsg_init_cnt;
volatile uint32_t g_db_nv_restore_timer;
volatile uint32_t g_db_nv_restore_cnt;
volatile uint32_t g_db_diag_communication_init_timer;
volatile uint32_t g_db_diag_communication_init_cnt;
volatile uint32_t g_db_at_uart_init_timer;
volatile uint32_t g_db_at_uart_init_cnt;
volatile uint32_t g_db_fs_lock_init_timer;
volatile uint32_t g_db_fs_lock_init_cnt;
volatile uint32_t g_db_rf_uart_init_timer;
volatile uint32_t g_db_rf_uart_init_cnt;
volatile uint32_t g_db_nvRfInit_timer;
volatile uint32_t g_db_nvRfInit_cnt;
volatile uint32_t g_db_adc_init_timer;
volatile uint32_t g_db_adc_init_cnt;
volatile uint32_t g_db_PhyInit_timer;
volatile uint32_t g_db_PhyInit_cnt;
volatile uint32_t g_db_worklock_init_timer;
volatile uint32_t g_db_worklock_init_cnt;
volatile uint32_t g_db_atc_ap_task_init_timer;
volatile uint32_t g_db_atc_ap_task_init_cnt;
volatile uint32_t g_db_at_init_timer;
volatile uint32_t g_db_at_init_cnt;
volatile uint32_t g_db_xy_srand_timer;
volatile uint32_t g_db_xy_srand_cnt;
volatile uint32_t g_db_net_led_init_timer;
volatile uint32_t g_db_net_led_init_cnt;
volatile uint32_t g_db_xy_proxy_init_timer;
volatile uint32_t g_db_xy_proxy_init_cnt;
volatile uint32_t g_db_icm_task_init_timer;
volatile uint32_t g_db_icm_task_init_cnt;
volatile uint32_t g_db_flash_task_init_timer;
volatile uint32_t g_db_flash_task_init_cnt;
volatile uint32_t g_db_wdt_init_timer;
volatile uint32_t g_db_wdt_init_cnt;
volatile uint32_t g_db_PsInit_timer;
volatile uint32_t g_db_PsInit_cnt;
volatile uint32_t g_db_cloud_init_timer;
volatile uint32_t g_db_cloud_init_cnt;
volatile uint32_t g_db_LPM_Init_timer;
volatile uint32_t g_db_LPM_Init_cnt;
volatile uint32_t g_db_Freq32k_Init_Process_timer;
volatile uint32_t g_db_Freq32k_Init_Process_cnt;
volatile uint32_t g_db_DeepSleep_Recovery_timer;
volatile uint32_t g_db_DeepSleep_Recovery_cnt;
#endif

extern void AutoBaudStart(void);
extern void malloc_factory_nv_mem();
extern void malloc_InVar_nv_mem();

void system_init(void)
{
	COREPRCM->RST_CTRL |= 0x1000;//release dfe

	delay_func_init();

	rf_sido_init();

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_rf_sido_timer = UTC_RECORD_TIMER;
	g_db_rf_sido_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif

	mpu_protect_init();

#if RUNTIME_DEBUG
	extern void xy_runtime_init(void);
	xy_runtime_init();
#endif

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_mpu_protect_init_timer = UTC_RECORD_TIMER;
	g_db_mpu_protect_init_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif
	
	rtc_init();

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_rtc_init_timer = UTC_RECORD_TIMER;
	g_db_rtc_init_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif

	g_sys_start_time = get_utc_tick();


	IpcMsg_init();

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_IpcMsg_init_timer = UTC_RECORD_TIMER;
	g_db_IpcMsg_init_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif

	nv_restore();

	extern void app_section_init_start();
	app_section_init_start();

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_nv_restore_timer = UTC_RECORD_TIMER;
	g_db_nv_restore_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif

    // log的初始化必须靠前，以保证能尽早使用log
    // 由于log中会根据nv判断是否需要启动log，所以需要在nv恢复后再进行初始化
    diag_communication_init();

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_diag_communication_init_timer = UTC_RECORD_TIMER;
	g_db_diag_communication_init_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif

	at_uart_init();

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_at_uart_init_timer = UTC_RECORD_TIMER;
	g_db_at_uart_init_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif

	fs_lock_init();

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_fs_lock_init_timer = UTC_RECORD_TIMER;
	g_db_fs_lock_init_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif

	PhyTimerInit();//rf_uart_init();

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_rf_uart_init_timer = UTC_RECORD_TIMER;
	g_db_rf_uart_init_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif

	nvRfInit();

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_nvRfInit_timer = UTC_RECORD_TIMER;
	g_db_nvRfInit_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif

	adc_init();

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_adc_init_timer = UTC_RECORD_TIMER;
	g_db_adc_init_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif
#ifdef _PHY_INIT_
	PhyInit();
#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_PhyInit_timer = UTC_RECORD_TIMER;
	g_db_PhyInit_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif
#endif 

	NVIC_IntRegister(WAKEUP_IRQn, WAKEUP_Handler, 1);
//	inter_buffer_msg_init();

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_worklock_init_timer = UTC_RECORD_TIMER;
	g_db_worklock_init_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif

	atc_ap_task_init();

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_atc_ap_task_init_timer = UTC_RECORD_TIMER;
	g_db_atc_ap_task_init_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif

	at_init();

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_at_init_timer = UTC_RECORD_TIMER;
	g_db_at_init_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif

	xy_srand(xy_seed());
#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_xy_srand_timer = UTC_RECORD_TIMER;
	g_db_xy_srand_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif

	if((g_softap_fac_nv != NULL) && (g_softap_fac_nv->led_pin <= GPIO_PAD_NUM_63))
	{
		net_led_init();
	}
#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_net_led_init_timer = UTC_RECORD_TIMER;
	g_db_net_led_init_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif

	xy_proxy_init();
#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_xy_proxy_init_timer = UTC_RECORD_TIMER;
	g_db_xy_proxy_init_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif

	icm_task_init();
#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_icm_task_init_timer = UTC_RECORD_TIMER;
	g_db_icm_task_init_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif

	flash_task_init();
#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_flash_task_init_timer = UTC_RECORD_TIMER;
	g_db_flash_task_init_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif

	wdt_init();
#if (configGENERATE_RUN_TIME_STATS == 1)
	osTimer_init();
#endif
#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_wdt_init_timer = UTC_RECORD_TIMER;
	g_db_wdt_init_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif

#ifdef _PS_INIT_
	if (!RF_isManufactryMode())
	{
		PsInit();
	#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
		g_db_PsInit_timer = UTC_RECORD_TIMER;
		g_db_PsInit_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
	#endif
	}
#endif

    PrintLog(0,PLATFORM,WARN_LOG,"Nbiot-1200, PowerOnReason %d, PowerOnSubReason:%d", Get_Boot_Reason(), Get_Boot_Sub_Reason());

	cloud_init();

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_cloud_init_timer = UTC_RECORD_TIMER;
	g_db_cloud_init_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif

	LPM_Init();

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_LPM_Init_timer = UTC_RECORD_TIMER;
	g_db_LPM_Init_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif

//统计内存时使用，从NV中获取内存剩余限制的大小
#if configHEAP_TEST
	extern uint32_t LimitedFreeHeap;
	LimitedFreeHeap = g_softap_fac_nv->LimitedRemainingHeap;
#endif
	// 必须放在nvRfInit之后！ 否则g_osc_cal值不准可能造成温度读取异常！
	Freq32k_Init_Process();
#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_Freq32k_Init_Process_timer = UTC_RECORD_TIMER;
	g_db_Freq32k_Init_Process_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif

	DeepSleep_Recovery();

#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	g_db_DeepSleep_Recovery_timer = UTC_RECORD_TIMER;
	g_db_DeepSleep_Recovery_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
	PrintLog(0,PLATFORM,WARN_LOG,"LPM_DEEPSLEEP cp_main:(%x, %d) rf_sido:(%x, %d) mpu_protect_init:(%x, %d) rtc_init:(%x, %d) IpcMsg_init:(%x, %d) nv_restore:(%x, %d)", g_db_cp_main_timer,g_db_cp_main_cnt,g_db_rf_sido_timer,g_db_rf_sido_cnt,g_db_mpu_protect_init_timer,g_db_mpu_protect_init_cnt,g_db_rtc_init_timer,g_db_rtc_init_cnt,g_db_IpcMsg_init_timer,g_db_IpcMsg_init_cnt,g_db_nv_restore_timer,g_db_nv_restore_cnt);
    PrintLog(0,PLATFORM,WARN_LOG,"LPM_DEEPSLEEP diag_communication_init:(%x, %d) at_uart_init:(%x, %d) fs_lock_init:(%x, %d) rf_uart_init:(%x, %d) nvRfInit:(%x, %d) adc_init:(%x, %d) PhyInit:(%x, %d)", g_db_diag_communication_init_timer,g_db_diag_communication_init_cnt,g_db_at_uart_init_timer,g_db_at_uart_init_cnt,g_db_fs_lock_init_timer,g_db_fs_lock_init_cnt,g_db_rf_uart_init_timer,g_db_rf_uart_init_cnt,g_db_nvRfInit_timer,g_db_nvRfInit_cnt,g_db_adc_init_timer,g_db_adc_init_cnt,g_db_PhyInit_timer,g_db_PhyInit_cnt);
    PrintLog(0,PLATFORM,WARN_LOG,"LPM_DEEPSLEEP worklock_init:(%x, %d) atc_ap_task_init:(%x, %d) at_init:(%x, %d) net_led_init:(%x, %d) xy_srand:(%x, %d) xy_proxy_init:(%x, %d) icm_task_init:(%x, %d)", g_db_worklock_init_timer,g_db_worklock_init_cnt,g_db_atc_ap_task_init_timer,g_db_atc_ap_task_init_cnt,g_db_at_init_timer,g_db_at_init_cnt,g_db_net_led_init_timer,g_db_net_led_init_cnt,g_db_xy_srand_timer,g_db_xy_srand_cnt,g_db_xy_proxy_init_timer,g_db_xy_proxy_init_cnt,g_db_icm_task_init_timer,g_db_icm_task_init_cnt);
	PrintLog(0,PLATFORM,WARN_LOG,"LPM_DEEPSLEEP flash_task_init:(%x, %d) wdt_init:(%x, %d) PsInit:(%x, %d) cloud_init:(%x, %d) LPM_Init:(%x, %d) Freq32k_Init_Process:(%x, %d) DeepSleep_Recovery:(%x, %d)", g_db_flash_task_init_timer,g_db_flash_task_init_cnt,g_db_wdt_init_timer,g_db_wdt_init_cnt,g_db_PsInit_timer,g_db_PsInit_cnt,g_db_cloud_init_timer,g_db_cloud_init_cnt,g_db_LPM_Init_timer,g_db_LPM_Init_cnt,g_db_Freq32k_Init_Process_timer,g_db_Freq32k_Init_Process_cnt,g_db_DeepSleep_Recovery_timer,g_db_DeepSleep_Recovery_cnt);
#endif


}

int main(void)
{
#if DEEPSLEEP_ADVANCE_TIME_CALCULTATE
	//g_db_cp_main = TICK_COUNTER_REG;
	g_db_cp_main_timer = UTC_RECORD_TIMER;
	g_db_cp_main_cnt = UTCClkCntConvert(UTC_RECORD_CNT);
#endif
	if(Is_WakeUp_From_Dsleep())
	{
		xy_assert(*(uint32_t *)BAK_MEM_CP_RETMEM_CSM == RetMem_Checksum());
	}

	osKernelInitialize();
	malloc_factory_nv_mem();
	malloc_InVar_nv_mem();

	system_init();

//	sys_global_flag_set(OS_START_FLAG, OS_START_FLAG_LEN, 1);

	osKernelStart();

	return 0;
}
