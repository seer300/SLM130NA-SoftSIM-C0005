#ifndef __UTC_H__
#define __UTC_H__

#include "hw_ints.h"
#include "xinyi2100.h"

#include "hw_spi.h"
#include "hw_types.h"
#include "hw_utc.h"
#include "debug.h"
#include "interrupt.h"

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

extern void UTC_TimerStop(void);
extern void UTC_TimerRun(void);
extern void UTC_CalStop(void);
extern void UTC_CalRun(void);
extern void UTC_DivStop(void);
extern void UTC_DivEn(void);
extern void UTC_HourModeSet(uint32_t ulMode);
extern uint32_t UTC_HourModeGet(void);
extern void UTC_TimerSet(uint32_t ulAMPM, uint32_t ulHour, uint32_t ulMin, uint32_t ulSec, uint32_t ulMinSec);
extern uint32_t UTC_TimerChangeGet(void);
extern void UTC_TimerGet(uint8_t *ulAMPM, uint8_t *ulHour, uint8_t *ulMin, uint8_t *ulSec, uint8_t *ulMinSec, uint32_t ulRegData);
extern void UTC_TimerAlarmSet(uint32_t ulAMPM, uint32_t ulHour, uint32_t ulMin, uint32_t ulSec, uint32_t ulMS);
extern void UTC_CalSet(uint32_t ulCentury, uint32_t ulYear, uint32_t ulMonth, uint32_t ulDate, uint32_t ulDay);
extern uint32_t UTC_CalChangeGet(void);
extern void UTC_CalGet(uint8_t *ulCentury, uint8_t *ulYear, uint8_t *ulMonth, uint8_t *ulDate, uint8_t *ulDay, uint32_t ulRegData);
extern void UTC_CalAlarmSet(uint32_t ulMonth, uint32_t ulDate);
extern void UTC_CalAlarmGet(uint8_t *ulMonth, uint32_t *ulDate);
extern void UTC_AlarmEnable(uint32_t ulAlarmFlags);
extern void UTC_AlarmDisable(uint32_t ulAlarmFlags);
extern void UTC_IntEnable(uint32_t ulIntFlags);
extern void UTC_IntDisable(uint32_t ulIntFlags);
extern void UTC_IntMaskSet(uint32_t ulIntMask);
extern uint32_t UTC_IntMaskGet(void);
extern void UTC_IntStatusSet(uint32_t ulIntFlags);
extern uint32_t UTC_IntStatusGet(void);
extern uint32_t UTC_ValidStatusGet(void);
extern void UTC_KeepRun(uint32_t ulKeepUTC);
extern void UTC_ClkCntSet(uint32_t ulClkCnt);
extern uint32_t UTC_ClkCntGet(uint32_t ulRegData);
extern void UTC_IntRegister(uint32_t *g_pRAMVectors, void (*pfnHandler)(void));
extern void UTC_IntUnregister(uint32_t *g_pRAMVectors);
extern uint8_t UTC_32768Get(void);
extern void UTC_AlarmCntCheckEnable(void);
extern void UTC_AlarmCntCheckDisable(void);
extern void UTC_AlarmCntCFG(uint32_t ulCnt);
extern uint32_t UTC_AlarmCntGet(void);
extern uint32_t UTC_ClkCntConvert(uint32_t ulRegData);
extern void UTC_WDTCtrlConfig(uint32_t ulConfig);
extern void UTC_WDTEnable(void);
extern void UTC_WDTDisable(void);
extern void UTC_WDTLongTimerDataValid(void);
extern void UTC_WDTLongTimerDataInvalid(void);
extern void UTC_WDTTickConfig(uint8_t ucAccuracy);
extern void UTC_WDTLongTimerDataSet(uint32_t ulTime);
extern void UTC_WDTShortTimerDataSet(uint8_t ucStartValue);
extern void UTC_WDTStartAfterWakeupSet(uint8_t ucActive);
extern void UTC_WDTCalendarDataSet(uint32_t ulCalendar);
extern uint32_t UTC_WDTIntStatusGet(void);
extern void UTC_WDTClearInt(uint8_t ucInt);
extern void UTC_WDTSetWatchdogIntMask(uint32_t WATCHDOG_INT_MASK);
extern void UTCWDT_Long_Timer_CalSet(uint32_t ulCentury, uint32_t ulYear, uint32_t ulMonth, uint32_t ulDate, uint32_t ulDay);
extern void UTCWDT_Long_Timer_TimerSet(uint32_t ulAMPM, uint32_t ulHour, uint32_t ulMin, uint32_t ulSec, uint32_t ulMinSec);
extern void UTC_WDTLongTimerIncrementFeedSecond(uint32_t IncSec);

/*低精度的延时函数，以utc cnt为计数单位（约30us），即32K计数周期，适用于对延时准确度不太高的场景。
*只依赖硬件utc cnt计数变化，不依赖任何全局变量参数，不受APCore频率变化影响，更适合驱动层面使用（内部硬件状态变化有些也是基于32K时钟频率）。
*传入参数为32K时钟周期数量，延时时间为delay_cnt/32K频率~（delay_cnt+1）/32K频率。如 utc_cnt_delay(10)，实际延时10~11个utccnt。
*另外该延时函数功耗相对较低，在功耗敏感的场景且精确度要求不太高的情况下，可以使用该函数延时。
*如果需要高精度的延时，请使用delay_func_us*/
extern void utc_cnt_delay(uint32_t delay_cnt);
int clock_days_before_month(int month, int leapyear);
int clock_is_leapyear(int year);
//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

/*********************************************************以下是基础平台层接口********************************************************/
#include "sys_clk.h"

#define SEC_PER_MIN                     ((uint64_t)60)
#define SEC_PER_HOUR                    ((uint64_t)60 * SEC_PER_MIN)
#define SEC_PER_DAY                     ((uint64_t)24 * SEC_PER_HOUR)

#define RTC_ALARM_INVALID               0xFFFFFFFFFFFFFFFF
#define RTC_NEXT_OFFSET_INVAILD         ((uint64_t)0xFFFFFFFFFFFFFFFF)        //RTC最近超时事件的剩余毫秒偏移量为无效值

#define UTC_WDT_MASK_SEC_Pos                8
#define UTC_WDT_MASK_SEC_Msk               (1UL << UTC_WDT_MASK_SEC_Pos)
#define UTC_WDT_MASK_MIN_Pos                9
#define UTC_WDT_MASK_MIN_Msk               (1UL << UTC_WDT_MASK_MIN_Pos)
#define UTC_WDT_MASK_HOUR_Pos               10
#define UTC_WDT_MASK_HOUR_Msk              (1UL << UTC_WDT_MASK_HOUR_Pos)
#define UTC_WDT_MASK_MON_Pos                11
#define UTC_WDT_MASK_MON_Msk               (1UL << UTC_WDT_MASK_MON_Pos)
#define UTC_WDT_MASK_DATE_Pos               12
#define UTC_WDT_MASK_DATE_Msk              (1UL << UTC_WDT_MASK_DATE_Pos)
#define UTC_WDT_MASK_YEAR_Pos               13
#define UTC_WDT_MASK_YEAR_Msk              (1UL << UTC_WDT_MASK_YEAR_Pos)

/**
 * @brief	RTC中断回调函数，RTC定时到期被调用.
 * @param	para 参数
 */
typedef void (*RTC_AlarmEventCallback)(void);


/**
  * @brief  通用时间结构体
  */
typedef struct
{
	uint32_t tm_msec;  /*!< 毫秒（忽略） */
	uint32_t tm_sec;	 /*!< 秒 (0-59) */
	uint32_t tm_min;	 /*!< 分 (0-59) */
	uint32_t tm_hour;	 /*!< 时 (0-23) */
	uint32_t tm_mday;	 /*!< 日 (1-31) */
	uint32_t tm_mon;	 /*!< 月 (1-12) */
	uint32_t tm_year;	 /*!< 年，如2024 */
	uint32_t tm_wday;	 /*!< 星期(1-7)	*/
	uint32_t tm_yday;	 /*!< 日期(0-365) */
	uint32_t tm_isdst; /*!< 夏令时（忽略）*/
} RTC_TimeTypeDef;

/**
 * @brief RTC事件结构体.
 */
typedef struct
{
    uint8_t timer_id;			  /*!< RTC事件ID，当为-1表示无效 */
    uint8_t is_period_rtc;        /*!< 周期性RTC标志位，为1时表示当前RTC事件为周期性RTC事件，为0时表示非周期性RTC事件*/
    uint8_t padding[2];           /*!< unused*/
	
    uint32_t rtc_alarm64_low;     /*!< CAL/TIMER/CNT时间寄存器合并后的64位数的低32位*/
    uint32_t rtc_alarm64_high;    /*!< CAL/TIMER/CNT时间寄存器合并后的64位数的高32位*/

    uint32_t rtc_period_msoffset; /*!< 周期性RTC的毫秒偏移量，非周期性RTC可设置该值为0*/
    RTC_AlarmEventCallback callback;   /*!< RTC回调函数，RTC定时到期被调用 */
} RTC_EVENT_CB;

typedef struct
{
    uint32_t tm_cal;   /*! 保证仅记录一次RTC寄存器值，深睡仍有效 */
	uint32_t tm_timer; /*!< 时 (0-23) */
	uint32_t tm_cnt;	 /*!< 分 (0-59) */
} RTC_CalTimerCntTypeDef;

extern uint64_t xy_mktime(RTC_TimeTypeDef *tp);
extern void xy_gmtime(const uint64_t msec, RTC_TimeTypeDef *result);
extern int RTC_GetTime(RTC_TimeTypeDef *rtctime);
extern uint64_t RTC_Get_Global_Byoffset(uint64_t msec_offset);
#endif // __UTC_H__
