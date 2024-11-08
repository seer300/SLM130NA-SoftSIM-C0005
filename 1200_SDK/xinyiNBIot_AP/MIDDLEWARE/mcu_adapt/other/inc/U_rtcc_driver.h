/*******************************************************************************
* @Copyright (c)    :(C)2020, Qingdao ieslab Co., Ltd
* @FileName         :hc32_rtcc_driver.h
* @Author           :Kv-L
* @Version          :V1.0
* @Date             :2020年7月1日 16:06:02
* @Description      :the function of the entity of GP22Gas_rtcc_driver.h
*******************************************************************************/
#ifndef __U_RTCC_DRIVER_H
#define __U_RTCC_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
//#include "ddl.h"
//typedef uint32_t  u32;
//typedef uint16_t u16;
//typedef uint8_t  u8;
#include "time.h"
#include "type.h"
#define difftime(t1, t0)	((double)((time_t)(t1)-(time_t)(t0)))
#define RTC_TIMER_EN                    1
/*RTCC定时器当量选择*/
#define	RTC_TIMER_RPT_SEC               1	//1s当量				1:选择		0：不选择
#define	RTC_TIMER_RPT_MIN               0	//1min
#define	RTC_TIMER_RPT_HOUR              0	//1hour
#define	RTC_TIMER_RPT_DAY               0	//1day
#define	RTC_TIMER_RPT_WEEK              0	//1week
#define	RTC_TIMER_RPT_MON               0	//1mon


#define RTC_TIM_MAX_NUM                  15

#define RTCC_TIMER_BAT_NUM               0  //3s
#define RTCC_TIMER_LCD_NUM               1  //60s
#define RTCC_TIMER_LCD_METSTATUES		 2  //1s
#define RTCC_TIMER_GP30_NUM              3   //小时
//#define TIMER_RTCC_DELAY_UPLOAD          4  //1s

#define TIMER_DATE_SAVE_MONITOR          5  //1min------------------------
#define TIMER_RTCC_TEST_SELF             6
#define RTCC_MIJI_WAIT_TIME              7
#define RTCC_GP30_INTERRUPT_TIME         8
#define RTCC_TOF_CALIBERATE_TIME         9 // 反算单程时间校准超时防护
#define RTCC_CARD_KEY_TIME				 10
#define RTCC_CARD_SEARCH_TIME            11
#define FLOW_DETECT_TIME                 12// 5s---流速度检测定时
#define RTCC_TIMER_CLKTRIM_TOTAL         13
#define RTCC_BLE_ACTIVE_TIME             14


typedef union
{
    struct
    {
        u8 year;
        u8 month;
        u8 day;
        u8 weekday;
        u8 hour;
        u8 minute;
        u8 second;
    } s; //日历和时间结构体
    u8 b[7]; // BYTE access
} DateTime;

typedef union
{
    struct
    {
        u8 year;
        u8 month;
        u8 day;
        u8 hour;
        u8 minute;
        u8 second;
    } s; //日历和时间结构体
    u8 b[6]; // BYTE access
} DateTime_noweek;

typedef union
{
    struct
    {
        u8 year;
        u8 month;
        u8 day;
        u8 weekday;
    } s; //日历结构体
    u8 b[4]; // BYTE access
} Date;

typedef union
{
    struct
    {
        u8 hour;
        u8 minute;
        u8 second;
    } s; //时间结构体
    u8 b[3]; // BYTE access
} rtcc_Time;

#define RTCC_TIMEOUT_US		1000	//timeout time 1000us

u8 RtccInit(void);

void RtccSetDateTime(DateTime *pDT);
u8 RtccGetDateTime(DateTime *pDT);
void RtccGetDate(Date *pD);
void RtccGetTime(rtcc_Time *pT);

void RtccAdjustDateTime(u8 adjust, u32 sec);

void RtccSetTimer(u8 rtcc_timenum, u16 rtcc_timespan);
u16 RtccCheckTimer(u8 rtcc_timenum);

void RtccSetAlrm(DateTime *pDT);
//void RtccSetAlrm(u8 count, u32 cycle);
void RtccGetAlrm(DateTime *pDT);
u8 RtccCheckMsg(void);
void RtccClearMsg(u8 bit);
//void RtccGetAlrmTimeAfter(DateTime *pDT);
u8 Rtcc_Check_DateTime(DateTime *pDT);
int32_t RtcccalculateDiffTime(const DateTime *newpD, const DateTime *oldpD);
void RtccAutoWakeupTimerEnable(u16 span_ms);
void RtccAutoWakeupTimerDisable(void);
u8 RtccWakeupCheckMsg(void);
void RtccWakeupClearMsg(void);

u8 RtccIfSleep(void);
void RtccPreSleep(void);
void RtccWakeSleep(void);
long RtccCalculateDiffTime(const DateTime *newpD, const DateTime *oldpD);
time_t get_mktime (DateTime *time_GMT);//标准时间准换为绝对秒数
time_t get_mktime_noweek (DateTime_noweek *time_GMT);//标准时间准换为绝对秒数
u8 RTC_ClkSource(void);
#if RTC_TIMER_EN
#if (1 != (RTC_TIMER_RPT_SEC + RTC_TIMER_RPT_MIN + RTC_TIMER_RPT_HOUR + RTC_TIMER_RPT_DAY + RTC_TIMER_RPT_WEEK + RTC_TIMER_RPT_MON))
	#error "Rtcc timer config error!"
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
