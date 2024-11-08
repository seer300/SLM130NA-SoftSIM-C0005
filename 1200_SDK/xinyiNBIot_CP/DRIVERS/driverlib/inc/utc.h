#ifndef __UTC_H__
#define __UTC_H__

#include "hw_ints.h"
#include "hw_memmap.h"
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

extern void UTCTimerStop(void);
extern void UTCTimerRun(void);
extern void UTCCalStop(void);
extern void UTCCalRun(void);
extern void UTCDivStop(void);
extern void UTCDivEn(void);
extern void UTCHourModeSet(unsigned long ulMode);
extern unsigned long UTCHourModeGet(void);
extern void UTCTimerSet(unsigned long ulAMPM, unsigned long ulHour, unsigned long ulMin, unsigned long ulSec, unsigned long ulMinSec);
extern unsigned long UTCTimerChangeGet(void);
extern void UTCTimerGet(unsigned char *ulAMPM, unsigned char *ulHour, unsigned char *ulMin, unsigned char *ulSec, unsigned char *ulMinSec, unsigned long ulRegData);
extern unsigned long UTCTimerGetBy10ms(void);
extern void UTCTimerAlarmSet(unsigned long ulAMPM, unsigned long ulHour, unsigned long ulMin, unsigned long ulSec, unsigned long ulMS);
extern void UTCTimerAlarmGet(unsigned long *ulAMPM, unsigned long *ulHour, unsigned long *ulMin, unsigned long *ulSec, unsigned long *ulMS);
extern void UTCTimerAlarmSetBy10ms(unsigned long ulMinSec);
extern void UTCCalSet(unsigned long ulCentury, unsigned long ulYear, unsigned long ulMonth, unsigned long ulDate, unsigned long ulDay);
extern unsigned long UTCCalChangeGet(void);
extern void UTCCalGet(unsigned char *ulCentury, unsigned char *ulYear, unsigned char *ulMonth, unsigned char *ulDate, unsigned char *ulDay, unsigned long ulRegData);
extern void UTCCalAlarmSet(unsigned long ulMonth, unsigned long ulDate);
extern void UTCCalAlarmGet(unsigned char *ulMonth, unsigned long *ulDate);
extern void UTCAlarmEnable(unsigned long ulAlarmFlags);
extern void UTCAlarmDisable(unsigned long ulAlarmFlags);
extern void UTCIntEnable(unsigned long ulIntFlags);
extern void UTCIntDisable(unsigned long ulIntFlags);
extern void UTCIntMaskSet(unsigned long ulIntMask);
extern unsigned long UTCIntMaskGet(void);
extern void UTCIntStatusSet(unsigned long ulIntFlags);
extern unsigned long UTCIntStatusGet(void);
extern unsigned long UTCValidStatusGet(void);
extern void UTCKeepRun(unsigned long ulKeepUTC);
extern void UTCClkCntSet(unsigned long ulClkCnt);
extern unsigned long UTCClkCntGet(unsigned long ulRegData);
extern unsigned long UTCClkCntConvert(unsigned long ulRegData);
extern void UTCIntRegister(unsigned long *g_pRAMVectors, void (*pfnHandler)(void));
extern void UTCIntUnregister(unsigned long *g_pRAMVectors);
extern void UTC32768Sel(unsigned char ucMode);
extern unsigned char UTC32768Get(void);
extern void UTCAlarmCntCheckEnable(void);
extern void UTCAlarmCntCheckDisable(void);
extern void UTCAlarmCntCFG(unsigned long ulCnt);
extern unsigned long UTCAlarmCntGet(void);

extern void UTCWDTCtrlConfig(unsigned long ulConfig);
extern void UTCWDTEnable(void);
extern void UTCWDTDisable(void);
extern void UTCWDTLongTimerDataValid(void);
extern void UTCWDTLongTimerDataInvalid(void);
extern void UTCWDTTickConfig(unsigned char ucAccuracy);
extern void UTCWDTLongTimerDataSet(unsigned long ulTime);
extern void UTCWDTShortTimerDataSet(unsigned char ucStartValue);
extern void UTCWDTStartAfterWakeupSet(unsigned char ucActive);
extern void UTCWDTCalendarDataSet(unsigned long ulCalendar);
extern unsigned long UTCWDTIntStatusGet(void);
extern void UTCWDTClearInt(unsigned char ucInt);
extern void UTCWDT_Long_Timer_CalSet(uint32_t ulCentury, uint32_t ulYear, uint32_t ulMonth, uint32_t ulDate, uint32_t ulDay);
extern void UTCWDT_Long_Timer_TimerSet(uint32_t ulAMPM, uint32_t ulHour, uint32_t ulMin, uint32_t ulSec, uint32_t ulMinSec);
extern void utc_cnt_delay(uint32_t delay_cnt);

#define UTC_WDT_MASK_SEC_Pos          8
#define UTC_WDT_MASK_SEC_Msk       	  (1UL << UTC_WDT_MASK_SEC_Pos)
#define UTC_WDT_MASK_MIN_Pos          9
#define UTC_WDT_MASK_MIN_Msk       	  (1UL << UTC_WDT_MASK_MIN_Pos)
#define UTC_WDT_MASK_HOUR_Pos         10
#define UTC_WDT_MASK_HOUR_Msk         (1UL << UTC_WDT_MASK_HOUR_Pos)
#define UTC_WDT_MASK_MON_Pos          11
#define UTC_WDT_MASK_MON_Msk       	  (1UL << UTC_WDT_MASK_MON_Pos)
#define UTC_WDT_MASK_DATE_Pos         12
#define UTC_WDT_MASK_DATE_Msk         (1UL << UTC_WDT_MASK_DATE_Pos)
#define UTC_WDT_MASK_YEAR_Pos         13
#define UTC_WDT_MASK_YEAR_Msk         (1UL << UTC_WDT_MASK_YEAR_Pos)
//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __UTC_H__

