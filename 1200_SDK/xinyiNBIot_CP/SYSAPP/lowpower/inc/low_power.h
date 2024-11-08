#ifndef __LOW_POWER_H__
#define __LOW_POWER_H__

#include "hw_memmap.h"
#include "hw_types.h"
#include "xy_system.h"
#include "xy_lpm.h"
#include "rtc_utils.h"
//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

#define     AP_FAST_RECOVERY_FUNCTION               ( * ( ( volatile uint8_t * ) BAK_MEM_AP_FAST_RECOVERY_FUNC ) )      //BAK_MEM_AP_FAST_RECOVERY_FUNC

#define     BAKMEM_8K_PWR_FORCE_ON                  (0xFFFF)    // bakmem_threshold == 0: force off ; bakmem_threshold == 0xFFFF: force on ; else: dynamic

#define     BAKMEM_8K_PWR_FORCE_OFF                 (0)         

#define 	RETENSION_MEMORY_CHECKSUM_ENABLE	  	(0)		  								// 易变NV checksum功能

#define 	XY_DFE_CLK_DIV							(12)										// DFE分频系数：默认为PLL的12分频

#define 	XY_SAMPLE_CLK_DIV						(16)										// phytiemr分频系数：默认为DFE的16分频

#define		SubFrameCntRld							(1920)								// 1ms对应的phytimer cnt值

#define	    FRC_RECOVERY_FLAG    					(0xa4a3)									// 深睡恢复魔术数字，据此判断是否进行深睡恢复

#define 	TIME_SLEEP_FOREVER						(0xFFFFFFFFFFFFFFFF)						// 表示无限长睡眠

#define 	LPM_THRESHOLD_DEEPSLEEP_MS				(6000)  			 						// 单位：ms；PSM模式下deepsleep睡眠阈值； drx\edrx 模式的阈值需要结合NV：deepsleep_threshold 共同确定阈值！

#define 	LPM_FAST_RECOVERY_THRESHOLD      		(20000)										// 单位：ms；快速恢复的时间阈值，低于此门限的深睡应执行快速恢复流程；需要结合NV：dsleep_fast_recovery_threshold 共同确定阈值！

#define		LPM_PLATFORM_DEEPSLEEP_ADVANCE_MS 		(((AONPRCM->SMEM_SLPCTRL & AP_SRAM0_SLPCTL_Msk) == (SRAM_POWER_MODE_FORCE_ON << AP_SRAM0_SLPCTL_Pos)) ? 50 : 60)	//  Deepsleep平台提前量(2022/9/16 测试唤醒至调度20ms，+5ms余量),不开快速恢复的时间未测

#define		LPM_PLATFORM_DEEPSLEEP_ADVANCE_FAST_MS 		15										//  cp快速恢复的深睡提前量

#define 	LPM_THRESHOLD_DEEPSLEEP_WITHFRC_MS      (10800000)							//  最大睡眠深睡可恢复时长（由于32k精度问题，超过此时间的深睡补偿不可靠，协议栈需要重新进行小区检测。当前默认为3小时，稍大于edrx最大睡眠时长）

#define 	LPM_PLATFORM_STANDBY_ADVANCE_MS			(5	)									//  standby平台提前量(2022/9/16 测试唤醒至调度3ms，+2ms余量)

#define 	LPM_THRESHOLD_STANDBY_MS				( LPM_PLATFORM_STANDBY_ADVANCE_MS + Ps_Lpminfo->standby_advance_ms)	//  standby睡眠阈值: phy提前量 + 平台提前量

#define 	PHYTIMER_UTC_TRIGGER_THREHOLD 			(7)  									// uphytime补偿的计算过程耗时,2022/09/20测试耗时约10cnt

#define     LPM_WFI_TICKLESS_MIN                    (5)                                      //WFI tick计数

#define     LPM_WFI_TICKLESS_MAX                    (3600000 )



#define LPM_ABNORMAL_CASE                                     (0)
#define LPM_ENTRY_DEEP_SLEEP                                  (1)
#define LPM_MTNET_NOT_ENTRY_SLEEP                             (2)
#define LPM_ATTACH_PROC_NOT_ENTRY_SLEEP                       (3)
#define LPM_PHY_CAN_NOT_ENTRY_SLEEP                           (4)
#define LPM_CAN_NOT_ENTRY_STANDBY_THRESHOLD                   (5)
#define LPM_STANDBY_RRC_CONN_STATE                            (6)
#define LPM_STANDBY_ATC_NAS_NOT_ENTRY_SLEEP                   (7)
#define LPM_STANDBY_USIM_NOT_ENTRY_SLEEP_TIMING_NOT_EXPIRY    (8)
#define LPM_STANDBY_USIM_NOT_ENTRY_SLEEP_PIN_LOCKED           (9)
#define LPM_STANDBY_RRC_NAS_NOT_STEADY_STATE                  (10)
#define LPM_STANDBY_OTHER_CASE                                (11)
#define LPM_STANDBY_NOT_SATISFY_DEEP_SLEEP_THRESHOLD          (12)
#define LPM_STANDBY_RRC_NOT_STEADY_STATE                      (13)
#define LPM_STANDBY_USIM_NOT_ENTRY_SLEEP_NV_SETTING           (14)
#define LPM_STANDBY_NOT_SATISFY_DEEP_SLEEP_CONDITION          (15)
#define LPM_STANDBY_PHY_FORCE_ENTRY                          (16)

typedef enum
{
	ALLOW_DEEPSLEEP = 0,
	NOT_ALLOW_DEEPSLEEP_MCNT_FAILED,
	NOT_ALLOW_DEEPSLEEP_TIME_LIMIT,
	NOT_ALLOW_DEEPSLEEP_AT_UNHANDLE,
	NOT_ALLOW_DEEPSLEEP_ICM_CHECK,
	NOT_ALLOW_DEEPSLEEP_PHYTIMER_EDGE,
	NOT_ALLOW_DEEPSLEEP_ICM_ZERO_COPY,
	NOT_ALLOW_DEEPSLEEP_DFE_IN_WORK,
	NOT_ALLOW_DEEPSLEEP_LOG_UNFINISHED,
	NOT_ALLOW_DEEPSLEEP_NV_OFF,
	NOT_ALLOW_DEEPSLEEP_LOCK_UNRELEASE,
	NOT_ALLOW_DEEPSLEEP_SMARTCARD_TRANSMIT,
	NOT_ALLOW_DEEPSLEEP_FLASH_WRITE,
	NOT_ALLOW_DEEPSLEEP_RC32K_CALI,
	NOT_ALLOW_DEEPSLEEP_STOP_CP_REQ,
	NOT_ALLOW_DEEPSLEEP_USER_NOT_ALLOW,
}DEEPSLEEP_STATE_ENUM;

typedef enum
{
	ALLOW_STANDBY = 0,
	NOT_ALLOW_STANDBY_MCNT_FAILED,
	NOT_ALLOW_STANDBY_TIME_LIMIT,
	NOT_ALLOW_STANDBY_AT_UNHANDLE,
	NOT_ALLOW_STANDBY_ICM_CHECK,
	NOT_ALLOW_STANDBY_PHYTIMER_EDGE,
	NOT_ALLOW_STANDBY_DFE_IN_WORK,
	NOT_ALLOW_STANDBY_LOG_UNFINISHED,
	NOT_ALLOW_STANDBY_NV_OFF,
	NOT_ALLOW_STANDBY_LOCK_UNRELEASE,
	NOT_ALLOW_STANDBY_SMARTCARD_TRANSMIT,
	NOT_ALLOW_STANDBY_RC32K_CALI,
	NOT_ALLOW_STANDBY_STOP_CP_REQ,
	NOT_ALLOW_STANDBY_USER_NOT_ALLOW,
}STANDBY_STATE_ENUM;

typedef enum
{
	ALLOW_WFI = 0,
	NOT_ALLOW_WFI_MCNT_FAILED,
	NOT_ALLOW_WFI_TIME_LIMIT,
	NOT_ALLOW_WFI_AT_UNHANDLE,
	NOT_ALLOW_WFI_ICM_CHECK,
	NOT_ALLOW_WFI_PHYTIMER_EDGE,
	NOT_ALLOW_WFI_DFE_IN_WORK,
	NOT_ALLOW_WFI_LOG_UNFINISHED,
	NOT_ALLOW_WFI_NV_OFF,
	NOT_ALLOW_WFI_LOCK_UNRELEASE,
}WFI_STATE_ENUM;

enum PHY_WAKEUP_STATE_Def
{
	NORMAL = 0,
	WAKEUP_FROM_WFI,
	WAKEUP_FROM_STANDBY,
	WAKEUP_FROM_DEEPSLEEP,
};

// 协议栈使用：规划出相应的睡眠模式
typedef enum
{
	KEEP_ACTIVE = 0,
	READY_TO_WFI,
	READY_TO_STANDBY,
	READY_TO_DEEPSLEEP,
}D_LPM_STATE;

// CP AP双核同步：供AP判断CP裁决最终睡眠模式
typedef enum{
	CP_DEFAULT_STATUS = 0,
	CP_IN_WORK,
	CP_IN_STANDBY,
	CP_IN_DEEPSLEEP,
	CP_IN_DEEPSLEEP_FAST_RECOVERY,	
}CP_WORK_MODE;

struct tcmcnt_info_t
{
	uint32_t mcnt_before;
	uint32_t mcnt_after;
	signed long 	temp_before;
	signed long 	temp_after;
	signed long  us_adjust;
};

// 协议栈睡眠结构体
typedef struct
{
	uint64_t	ulltime_ms;				//set sleep state at this point of time	
	uint64_t	ullsleeptime_ms;		//how long to sleep
	char		state;             		//enum D_LPM_STATE
	char        ucForceStandby;         //froce goto standby
	uint32_t	deepsleep_advance_ms;	//sys wake up from deepsleep in advance 
	uint32_t	standby_advance_ms;		//sys wake up from standby in advance 

	uint8_t ucTauEdrxOtherType;	//0:not sleep || abnormal sleep; 1:psm;2:drx/edrx;3:poweroff sleep ;4: detach(CGATT0\COPS or CFUN1) state sleep
}T_LPM_INFO;


typedef struct
{
	uint32_t  ullpmtime_s;
	uint32_t  ullpmtime_us;
}LPM_TIME_T;

// 保存于易变NV中的全局睡眠信息
struct LPM_TIMER_INFO_RECOVERY_T
{
	uint8_t sleep_type;
	uint8_t wakeup_reason;
	uint16_t frc_flag;
	uint32_t pll_divn_before;
	uint32_t pll_divn_after;
	int32_t temp_before;
	uint32_t mcnt_32k_to_HCLK;
	uint32_t SFN_Number_before_sleep;
	uint32_t HFN_Number_before_sleep;
	uint32_t subframe_before_sleep;
	uint32_t countInSubFrame_before_sleep;
	uint32_t cp_enter_wfi_tick;
	uint32_t frc_clk_divn;
	uint32_t ap_cp_delta_time;	
	uint64_t utccnt_before_sleep;
};

// 获取phytimer和UTC瞬时时刻点
void LPM_Phytimer_UTC_Cnt_Save(rtc_reg_t* utc_reg , uint32_t *ulTimrh_cnt, uint32_t *ulTimerml_cnt );

// lpm专用锁临界区接口
void vLpmEnterCritical(void);

// lpm专用开临界区接口
void vLpmExitCritical(void);

// lpm专用查询临界区状态接口
uint8_t vLpmCriticalTest( void );

// retension memory checksum计算，校验涵盖了phy\ps\platform\lpm\rtc
uint32_t RetMem_Checksum();

// 关闭JLINK，将其引脚输入上拉
void LPM_Jlink_PowerOFF(uint8_t swd_clk_pin, uint8_t swd_io_pin);

// 根据出厂NV配置引脚，使能JLINK
void LPM_Jlink_Config(uint8_t swd_clk_pin, uint8_t swd_io_pin);

// 获取平台standby睡眠时长
uint32_t LPM_GetExpectedIdleTick(uint8_t sleep_mode);

void LPM_Delay(uint32_t uldelay);

// 获取当前pll分频系数
uint32_t get_pll_divn(void);

// MCNT校准
uint32_t Mcnt_Get(void);

// 根据PLL变化，调整MCNT
void Mcnt_Adjust(void);

// Deepsleep、standby睡眠后 clktick 恢复
void LPM_Tick_Recover(void);

//  Deepsleep、standby睡眠后 物理层phytimer补偿
void LPM_Phytimer_Compensate_Process(struct LPM_TIMER_INFO_RECOVERY_T* sleep_info);

// phytimer临界状态判断：避免在phytimer跳变边沿进入睡
uint32_t  LPM_Phytimer_Edge_Check(void);

// PS： 获取ptVar_F与当前时间的差值
void LPM_time_get(LPM_TIME_T* p_lpmtime);

// PHY：判断本次上电是否成功进行了深睡恢复，并重置标记位
uint32_t get_phy_wakeup_state(void);

// PHY： 获取MCNT校准值
uint32_t Mcnt_Get_Phy();

//PHY：判断本次上电是否成功进行了深睡恢复
uint32_t get_ps_wakeup_state(void);

// PHY： 获取睡眠NV是否开启(Deepsleep、standby)
uint32_t get_phy_lowerpower_enable_state(void);

// PHY：开启MCNT动态反馈功能
void Mcnt_Set_Enable(uint8_t ucmcnt_en_flag);

// PHY：开启MCNT动态反馈
void Mcnt_Feedback_Start();

// PHY：关闭MCNT动态反馈功能
void Mcnt_Feedback_End();

// PHY：根据睡眠时长修正MCNT校准值
void Set32k_To_HCLK(int32_t diffval,uint32_t sleepTime);

int Lpm_WFI_Process(void);

// 对默认非实时的云业务线程，对于某些远程通信流程需要分多步才能轮询处理完毕场景，通过该接口来保证若干秒(建议不小于5秒)内，其轮询周期定时器的实时性
// thread_id为NULL时，默认为当前线程的句柄
// 注：使用此接口时，请先与唐小玲确认
void set_realtime_in_standby(int sec, osThreadId_t thread_id);

// 使用者若在业务中调用set_realtime_in_standby接口设置了软定时器，业务流程结束时，一定要在退出业务线程之前，调用此接口删除设置的软定时器，以避免访问野指针
// 注：使用此接口时，请先与唐小玲确认
void del_realtime_timer(osThreadId_t thread_id);

//深睡查询专用，返回系统上一次进入idle的时刻点
uint64_t get_tick_enter_idle(void);

//深睡查询专用，返回上一次退出idle线程时的深睡状态
int get_sleep_state(void);

#if LPM_LOG_DEBUG
extern volatile uint32_t g_debug_before_cal0;
extern volatile uint32_t g_debug_before_time0;
extern volatile uint32_t g_debug_before_cnt0;

extern volatile uint32_t g_debug_before_cal1;
extern volatile uint32_t g_debug_before_time1;
extern volatile uint32_t g_debug_before_cnt1;

extern volatile uint32_t g_debug_alarm_cal0 ;
extern volatile uint32_t g_debug_alarm_time0 ;
extern volatile uint32_t g_debug_alarm_cnt0 ;

extern volatile uint32_t g_debug_alarm_cal1;
extern volatile uint32_t g_debug_alarm_time1 ;
extern volatile uint32_t g_debug_alarm_cnt1 ;

extern volatile uint32_t g_debug_cal0 ;
extern volatile uint32_t g_debug_time0 ;
extern volatile uint32_t g_debug_cnt0;

extern volatile uint32_t g_debug_cal1 ;
extern volatile uint32_t g_debug_time1 ;
extern volatile uint32_t g_debug_cnt1 ;

extern volatile uint32_t g_debug_cal2;
extern volatile uint32_t g_debug_time2;
extern volatile uint32_t g_debug_cnt2;

extern volatile uint32_t g_debug_cal3;
extern volatile uint32_t g_debug_time3;
extern volatile uint32_t g_debug_cnt3;

extern volatile uint32_t g_debug_cal4;
extern volatile uint32_t g_debug_time4;
extern volatile uint32_t g_debug_cnt4;

extern volatile uint32_t g_debug_cal5;
extern volatile uint32_t g_debug_time5;
extern volatile uint32_t g_debug_cnt5;


extern volatile uint32_t g_debug_utccntx;
extern volatile uint64_t g_debug_utccnt_after_sleep;
#endif 

extern void ps_next_work_time(T_LPM_INFO *Ps_Lpminfo);
extern void phy_next_work_time(T_LPM_INFO *work_model, unsigned int *puiHsfn, unsigned int *puiSfn);
extern uint8_t chk_ps_in_attach_proc();
extern unsigned char chk_ps_allow_sleep_chk();
extern char deepsleep_enabled_check();
extern void PsRollBackucNvChgBitMap();
#ifdef __cplusplus
}
#endif

#endif //  __LOW_POWER_H__
