/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#include "hw_prcm.h"
#include "cmsis_os2.h"
#include "softap_nv.h"
#include "xy_memmap.h"
#include "xy_rtc_api.h"
#include "prcm.h"
#include "rtc_tmr.h"
#include "main_proxy.h"
#include "xy_at_api.h"
#include "sema.h"

/*******************************************************************************
 *                        Global variable definitions                          *
 ******************************************************************************/
#define RTC_TIMEOUT_THRESHOLD (11)
extern unsigned int TickCounterGet(void);
extern uint64_t get_utc_tick();

//RAM_NV_VOLATILE_RTCLIST_START,位于var_nv底部
rtc_event_info_t *cp_rtc_event_arry = NULL;

osMutexId_t	rtc_mutex = NULL;
osSemaphoreId_t g_rtc_sem = NULL;	
osThreadId_t rtc_tmr_task_handler = NULL;
osMutexId_t g_UT_timer_mutex = NULL;

/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/

bool get_sema_hardware(uint32_t timeout_ms)
{
	uint32_t start = osKernelGetTickCount();
	uint32_t tick;

	do
    {
        SEMA_RequestNonBlocking(SEMA_IDLE_HARDWARE, SEMA_SEMA_DMAC_NO_REQ, SEMA_SEMA_REQ_PRI_0, SEMA_MASK_AP);

		/*慎用，可能存在AP和CP冲突访问*/
	    if(timeout_ms == 0)
    	{
			return (SEMA_MASTER_CP == SEMA_MasterGet(SEMA_IDLE_HARDWARE));
    	}

        if(SEMA_MASTER_CP != SEMA_MasterGet(SEMA_IDLE_HARDWARE))
        {
        	tick = osKernelGetTickCount();
        	if(tick > start && (tick-start) > timeout_ms) 
        	{
        		xy_assert(0);
        	}
			/*IDLE线程里不能执行delay*/
        	if(osCoreGetState() == osCoreNormal && osKernelGetState() >= osKernelRunning && osKernelIsRunningIdle() != osOK)
        	{
        		osDelay(100); 
        	}
        }
        else
        	return true;
    } while (1); //阻塞等待获取AP、CP核间的互斥锁
}

void put_sema_hardware()
{
	SEMA_Release(SEMA_IDLE_HARDWARE, SEMA_MASK_NULL);
}

uint64_t Get_CP_ALARM_RAM()
{
	return *(uint64_t*)BAK_MEM_CP_RTC_ALARM;
}

void Set_CP_ALARM_RAM(uint64_t rtcmsc)
{
	*(uint64_t*)BAK_MEM_CP_RTC_ALARM = rtcmsc;
}

// bug: utc_cnt进位时，utc_timer寄存器更新不及时
void __RAM_FUNC rtc_get_register_workaround(void)
{
	char carryflag = 0;
	volatile unsigned long utc_tmp;
	volatile uint32_t utc_delay;

	utc_tmp = HWREG(UTC_CLK_CNT);
	while( utc_tmp == 159 || utc_tmp == 415 )
	{
		utc_tmp = HWREG(UTC_CLK_CNT);
		carryflag = 1;
	}

	if( carryflag )
	{
		for(utc_delay = 0; utc_delay < 10; utc_delay++ );
	}
}

// 获取UTC CAL/TIMER/CNT寄存器值
int __RAM_FUNC rtc_get_register(rtc_reg_t *rtc_reg)
{
	taskENTER_CRITICAL();

	rtc_get_register_workaround();

	rtc_reg->rtc_cal = HWREG(UTC_CAL);
	rtc_reg->rtc_timer = HWREG(UTC_TIMER);
	rtc_reg->rtc_cnt = HWREG(UTC_CLK_CNT);

	taskEXIT_CRITICAL();

	return 0;
}

/* 获取当前时间(忽略年月日,只获取时分秒),仅限定制功能使用*/
int rtc_get_clock(RTC_TimeTypeDef *rtctime)
{
	unsigned char ulAMPM;
	unsigned char ulHour;
	unsigned char ulMin;
	unsigned char ulSec;
	unsigned char ulMinSec;
	unsigned long ulclkcnt;
 	rtc_reg_t rtc_reg;
  
  	rtc_get_register(&rtc_reg);

	ulclkcnt = UTCClkCntConvert(rtc_reg.rtc_cnt);
	UTCTimerGet(&ulAMPM, &ulHour, &ulMin,&ulSec, &ulMinSec,rtc_reg.rtc_timer);

	rtctime->wall_clock.tm_hour = ulHour;
	rtctime->wall_clock.tm_min  = ulMin;
	rtctime->wall_clock.tm_sec  = ulSec;
	rtctime->tm_msec = (uint32_t)(ulMinSec * 10) + ulclkcnt / 32;

	return 0;
}

/*仅限PS/PHY和省电使用，返回count值，为1/32ms粒度*/
uint64_t __RAM_FUNC rtc_time_calculate_process(rtc_reg_t *rtc_reg, RTC_TimeTypeDef *rtctime)
{
	uint8_t ulCentury=0;
	uint8_t ulYear=0;
	uint8_t ulMonth=0;
	uint8_t ulData;
	uint8_t ulDay;	

	uint8_t ulAMPM;
	uint8_t ulHour;
	uint8_t ulMin;
	uint8_t ulSec;
	uint8_t ulMinSec;

	uint32_t ulclkcnt;

	UTCTimerGet(&ulAMPM, &ulHour, &ulMin, &ulSec, &ulMinSec, rtc_reg->rtc_timer);
	UTCCalGet(&ulCentury, &ulYear, &ulMonth, &ulData, &ulDay, rtc_reg->rtc_cal);
	ulclkcnt = UTCClkCntConvert(rtc_reg->rtc_cnt);

	// 默认为24小时制，ulAMPM为0；此处需要防止被修改为AM/PM模式
	rtctime->wall_clock.tm_hour  = ulAMPM * 12 + ulHour;
	rtctime->wall_clock.tm_min   = ulMin;
	rtctime->wall_clock.tm_sec   = ulSec;
	rtctime->wall_clock.tm_mday  = ulData;
	rtctime->wall_clock.tm_mon   = ulMonth;
	rtctime->wall_clock.tm_year  = ulCentury * 100 + ulYear;
	rtctime->tm_msec  = ulMinSec * 10 + ulclkcnt / 32;
	rtctime->tm_wday  = ulDay;
	rtctime->tm_yday  = ulData + clock_days_before_month(rtctime->wall_clock.tm_mon-1, CLOCK_IS_LEAPYEAR(rtctime->wall_clock.tm_year));
	rtctime->tm_isdst = 0;

	return (xy_mktime(rtctime) * 32 + (ulclkcnt % 32));
}
volatile rtc_reg_t g_debug_rtc_reg_standby_after;//UTC_ALARM_DEBUG


/*获取RTC年月日寄存器的count值,为1/32ms粒度。仅限底层使用，不能用xy_gmtime_r进行转换*/
uint64_t __RAM_FUNC rtc_get_cnt(void)
{
  rtc_reg_t rtc_reg;
  RTC_TimeTypeDef rtctime;
  
  rtc_get_register(&rtc_reg);
  g_debug_rtc_reg_standby_after = rtc_reg;
  return rtc_time_calculate_process(&rtc_reg, &rtctime);
}

/* 获取RTC当前墙上时间,并返回1970/1/1的毫秒偏移*/
uint64_t rtc_timer_read(RTC_TimeTypeDef *rtctime)
{
	rtc_reg_t rtc_reg;
	
	uint8_t ulCentury=0;
	uint8_t ulYear=0;
	uint8_t ulMonth=0;
	uint8_t ulData;
	uint8_t ulDay;	

	uint8_t ulAMPM;
	uint8_t ulHour;
	uint8_t ulMin;
	uint8_t ulSec;
	uint8_t ulMinSec;

	uint32_t ulclkcnt;

	rtc_get_register(&rtc_reg);

	UTCTimerGet(&ulAMPM, &ulHour, &ulMin, &ulSec, &ulMinSec, rtc_reg.rtc_timer);
	UTCCalGet(&ulCentury, &ulYear, &ulMonth, &ulData, &ulDay, rtc_reg.rtc_cal);
	ulclkcnt = UTCClkCntConvert(rtc_reg.rtc_cnt);

	// 默认为24小时制，ulAMPM为0；此处需要防止被修改为AM/PM模式
	rtctime->wall_clock.tm_hour  = ulAMPM * 12 + ulHour;
	rtctime->wall_clock.tm_min   = ulMin;
	rtctime->wall_clock.tm_sec   = ulSec;
	rtctime->wall_clock.tm_mday  = ulData;
	rtctime->wall_clock.tm_mon   = ulMonth;
	rtctime->wall_clock.tm_year  = ulCentury * 100 + ulYear;
	rtctime->tm_msec  = ulMinSec * 10 + ulclkcnt / 32;
	rtctime->tm_wday  = ulDay;
	rtctime->tm_yday  = ulData + clock_days_before_month(rtctime->wall_clock.tm_mon-1, CLOCK_IS_LEAPYEAR(rtctime->wall_clock.tm_year));
	rtctime->tm_isdst = 0;

	return xy_mktime(rtctime);
}

//56-59         52-55    		51         47-50         45-46         41-44	   40 - 9      8-0
//year(tens)    year(unit)   mon(tens)   mon(units)  Date(tens)    Date(units)     timer       cnt
uint64_t Integrate_CalTimeCntReg_Into_Num64( RTC_CalTimerCntTypeDef *rtc_cal_time_cnt_reg)
{
	uint64_t utc_num64;
	uint32_t tmp_year;
	uint32_t tmp_mon;
	uint32_t tmp_day;

	// 年\月\日依次保存在64位立即数的bit41-bit59
	tmp_year = (rtc_cal_time_cnt_reg->tm_cal >> UTC_CAL_YU_Pos) & 0xFF;
	tmp_mon  = (rtc_cal_time_cnt_reg->tm_cal >> UTC_CAL_MU_Pos) & 0x1F;
	tmp_day  = (rtc_cal_time_cnt_reg->tm_cal >> UTC_CAL_DU_Pos) & 0x3F;

	utc_num64 = ((tmp_year << 11) | (tmp_mon << 6) | tmp_day);

	// 时\分\秒依次保存在64位立即数的bit9-bit40
	utc_num64 = (utc_num64 << 32) + rtc_cal_time_cnt_reg->tm_timer;

	// CNT保存在64位立即数的bit0-bit8
	utc_num64 = (utc_num64 << 9) + UTCClkCntConvert(rtc_cal_time_cnt_reg->tm_cnt);

	return utc_num64;
}

//56-59         52-55    		51         47-50         45-46         41-44	   40 - 9      8-0
//year(tens)    year(unit)   mon(tens)   mon(units)  Date(tens)    Date(units)     timer       cnt
int Split_Num64_To_CalTimeCntReg(uint64_t utc_num64, RTC_CalTimerCntTypeDef *rtc_cal_time_cnt_reg)
{
	uint64_t tmp ;
	uint32_t cnt_tmp;
	uint32_t tmp_year;
	uint32_t tmp_mon;
	uint32_t tmp_day;

	// 年\月\日转化为cal寄存器格式
	tmp = (uint32_t)(utc_num64 >> 41);

	tmp_day  =   tmp & 0x3F;
	tmp_mon  = ((tmp >> 6)  & 0x1F);
	tmp_year = ((tmp >> 11) & 0xFF);                       

	// 年\月\日转化为cal寄存器格式
	rtc_cal_time_cnt_reg->tm_cal = (tmp_day << UTC_CAL_DU_Pos) | (tmp_mon << UTC_CAL_MU_Pos) | (tmp_year << UTC_CAL_YU_Pos);

	// 时\分\秒转化为timer寄存器格式
	rtc_cal_time_cnt_reg->tm_timer = (uint32_t)(utc_num64 >> 9);

	// cnt转化为cnt寄存器格式
	cnt_tmp = (uint32_t)(utc_num64 & 0x1FF);

	if(cnt_tmp < 160)
		rtc_cal_time_cnt_reg->tm_cnt = cnt_tmp + 256;
	else
		rtc_cal_time_cnt_reg->tm_cnt = cnt_tmp - 160;

	return 0;
}

// 判断当前RTC事件是否过期
int Is_RTC_Already_Expired(uint64_t utc_alarm_num64)
{
	unsigned long long utc_cur;
	RTC_CalTimerCntTypeDef cur_rtc_cal_time_cnt_reg;

	taskENTER_CRITICAL();

	rtc_get_register_workaround();

	// 获取当前CAL/TIMER/CNT寄存器时间
	cur_rtc_cal_time_cnt_reg.tm_cal = HWREG(UTC_CAL);
	cur_rtc_cal_time_cnt_reg.tm_timer = HWREG(UTC_TIMER) & 0x7FFFFFFF;
	cur_rtc_cal_time_cnt_reg.tm_cnt = HWREG(UTC_CLK_CNT);

	taskEXIT_CRITICAL();

	// 将当前CAL/TIMER/CNT寄存器时间和alarm事件时间分别整合为64位数值，方便比较
 	utc_cur = Integrate_CalTimeCntReg_Into_Num64(&cur_rtc_cal_time_cnt_reg);

	if(osThreadGetId() == rtc_tmr_task_handler)
	{
		xy_printf(0,PLATFORM, WARN_LOG, "is_rtc_event_expired cur_rtc_msec=%lld, event_time=%lld",utc_cur,utc_alarm_num64);
	}
	// 若当前时间不小于alarm事件时间，则判断为超时
	if(utc_cur >= utc_alarm_num64)
	{
		return 1;		
	}
	else
		return 0;
}

// nv_restore调用，初始化阶段只调用一次
void restore_RTC_list(void)
{
	uint64_t cp_rtc_alarm_time;
	rtc_event_info_t* next_rtcinfo = NULL;
	//RTC_CalTimerCntTypeDef rtc_cal_time_cnt_reg;

	next_rtcinfo = xy_rtc_get_next_event();
	if(next_rtcinfo == NULL)
		return;

	rtc_event_refresh();

	//if power on for UTC wakeup
	cp_rtc_alarm_time = next_rtcinfo->rtc_alarm64_high;
	cp_rtc_alarm_time = (cp_rtc_alarm_time << 32) + next_rtcinfo->rtc_alarm64_low;

}

/*获取RTC寄存器中1970/1/1相对tick值，该值不是严格的ms粒度，与32K精度相关，不能用于世界时间的直接加减*/
uint64_t rtc_get_tick_ms()
{
	RTC_TimeTypeDef rtctime;

	return rtc_timer_read(&rtctime);
}

// 普通流程设置RTC alarm寄存器
int rtc_alarm_set(RTC_TimeTypeDef *rtctime)
{
	UTCIntDisable(UTC_INT_ALARM);
	UTCCalAlarmSet(rtctime->wall_clock.tm_mon, rtctime->wall_clock.tm_mday);
	UTCTimerAlarmSet(0, rtctime->wall_clock.tm_hour, rtctime->wall_clock.tm_min, rtctime->wall_clock.tm_sec, rtctime->tm_msec);
	UTCAlarmCntCheckEnable();
	UTCAlarmEnable(UTC_ALARM_ALL);
	UTCIntEnable(UTC_INT_ALARM);
	return 0;
}

// 快速设置RTC ALARM寄存器
int rtc_set_alarm_by_reg(RTC_CalTimerCntTypeDef *rtc_cal_time_cnt_reg)
{
	HWREG(UTC_ALARM_CAL) = rtc_cal_time_cnt_reg->tm_cal & 0x3FFC;
	HWREG(UTC_ALARM_TIMER) = rtc_cal_time_cnt_reg->tm_timer;

	if(rtc_cal_time_cnt_reg->tm_cnt >= 0x100 && rtc_cal_time_cnt_reg->tm_cnt <= 0x104)
	{
		rtc_cal_time_cnt_reg->tm_cnt = 0x105;
	}
	HWREG(UTC_ALARM_CLK_CNT) = rtc_cal_time_cnt_reg->tm_cnt;

	UTCAlarmCntCheckEnable();
	UTCAlarmEnable(UTC_ALARM_ALL);
	UTCIntEnable(UTC_INT_ALARM);

	return 0;
}
	 
// 快速设置RTC ALARM寄存器
int __RAM_FUNC rtc_set_alarm_by_cnt(uint64_t ullutccnt)
{
	RTC_TimeTypeDef rtctime = {0};
	volatile unsigned long utc_int_clear;
	unsigned long ulclkcnt_set,tmp;
	uint64_t ulltimems = ullutccnt/32;

	xy_gmtime_r(ulltimems, &rtctime);

	UTCAlarmDisable(UTC_ALARM_ALL);
	UTCAlarmCntCheckDisable();
	utc_int_clear = HWREG(UTC_INT_STAT);  //读清

	UTCCalAlarmSet(rtctime.wall_clock.tm_mon, rtctime.wall_clock.tm_mday);
	UTCTimerAlarmSet(0, rtctime.wall_clock.tm_hour, rtctime.wall_clock.tm_min, rtctime.wall_clock.tm_sec, rtctime.tm_msec);	

	tmp = ullutccnt % 320;
	if(tmp >= 160)
		ulclkcnt_set = tmp - 160;
	else
		ulclkcnt_set = tmp + 256;

    HWREG(UTC_ALARM_CLK_CNT) = ulclkcnt_set;

    UTCAlarmCntCheckEnable();
	UTCAlarmEnable(UTC_ALARM_ALL);

	UNUSED_ARG(utc_int_clear);

	return 1;
}

void refresh_rtc_alarm_timeout_info()   //BUG8986 规避代码：AP侧在睡眠前判断距离最近超时的RTC事件不足1ms时退出睡眠
{
	uint64_t next_rtc_ms;

	next_rtc_ms = Get_CP_ALARM_RAM();
	//最近超时的RTC事件距离当前时刻点的剩余毫秒偏移量
	*((volatile uint64_t *)BAK_MEM_RTC_NEXT_OFFSET) = (Transform_Num64_To_Ms(next_rtc_ms) - get_utc_tick());  //考虑存在超过49天的可能，不能强转成32位
	//记录当前时刻的TICK
	HWREG(BAK_MEM_TICK_CAL_BASE) = TickCounterGet();
}

// 获取最近RTC ALARM事件，更新到ALARM寄存器中
int rtc_alarm_reset(uint64_t cp_rtc_time)
{
	RTC_TimeTypeDef cur_rtc_time;
	RTC_CalTimerCntTypeDef rtc_cal_time_cnt_reg;

	// 为了防止死锁，此处锁中断必不可少
	taskENTER_CRITICAL();

	// 设置下一个CP ALARM时刻点
	Set_CP_ALARM_RAM(cp_rtc_time);

	if(cp_rtc_time == RTC_ALARM_INVALID)
	{
		*((volatile uint64_t *)BAK_MEM_RTC_NEXT_OFFSET) = RTC_NEXT_OFFSET_INVAILD;   //BUG8986 规避代码：剩余毫秒偏移量设置为无效值，AP由此判断CP_RTC对于睡眠的影响是否有效
		// 失能UTC Alarm中断
		UTCAlarmDisable(UTC_ALARM_ALL);
		UTCAlarmCntCheckDisable();

		taskEXIT_CRITICAL();
		return 0;
	}
	// 判断当前CP侧alarm是否已过期，若过期，重设为当前能设置的RTC硬件的最早时刻点，防止RTC alarm寄存器里面是一个过期值，导致再也不能触发中断
	else if(Is_RTC_Already_Expired(cp_rtc_time))
	{
		xy_gmtime_r(rtc_get_tick_ms()+RTC_TIMEOUT_THRESHOLD, &cur_rtc_time);
		rtc_alarm_set(&cur_rtc_time);	
	}
	else
	{
		// 分割64位alarm值为CAL/TIMER/CNT寄存器结构体格式
		Split_Num64_To_CalTimeCntReg(cp_rtc_time, &rtc_cal_time_cnt_reg);				
		// AP侧alarm事件未过期，直接快速设置RTC ALARM寄存器
		rtc_set_alarm_by_reg(&rtc_cal_time_cnt_reg);
	}
	
	//更新BAKMEM存储的RTC最近超时事件的信息
	refresh_rtc_alarm_timeout_info();   //BUG8986 规避代码：AP侧在睡眠前判断距离最近超时的RTC事件不足1ms时退出睡眠

	taskEXIT_CRITICAL();
	return 0;
}

// 将ms偏移转化为时钟(适用于ms偏移小于一天的情况)
int Transform_Ms_To_Clock(uint64_t msec, RTC_TimeTypeDef *result)
{
	uint64_t epoch;
	uint64_t jdn;
	unsigned int hour;
	unsigned int min;
	unsigned int sec;

	/* 获取时分秒 */
	epoch = msec / 1000;

	jdn    = epoch / SEC_PER_DAY;
	epoch -= SEC_PER_DAY * jdn;

	hour   = (unsigned int)(epoch / SEC_PER_HOUR);
	epoch -= SEC_PER_HOUR * hour;

	min    = (unsigned int)(epoch / SEC_PER_MIN);
	epoch -= SEC_PER_MIN * min;

	sec    = (unsigned int)epoch;

	/* Then return the struct tm contents */
	result->wall_clock.tm_hour  = (unsigned int)hour;
	result->wall_clock.tm_min   = (unsigned int)min;
	result->wall_clock.tm_sec   = (unsigned int)sec;
	result->tm_msec  = (unsigned int)((msec)%1000);

	return 0;
}

// 将拼接成64位的寄存器值转换为ms单位
uint64_t Transform_Num64_To_Ms(uint64_t rtc_cal_time_cnt_num64)
{
	uint64_t cp_rtc_msec;
	uint32_t rtc_info_cal;
	uint32_t rtc_info_timer;
	uint32_t rtc_info_cnt;
	RTC_TimeTypeDef rtc_info;

	uint8_t ulAMPM;
	uint8_t ulHour;
	uint8_t ulMin;
	uint8_t ulSec;
	uint8_t ulMinSec;

	if ((rtc_cal_time_cnt_num64 != 0) && (rtc_cal_time_cnt_num64 != RTC_ALARM_INVALID))
	{
		// 计算年月日
		rtc_info_cal = (uint32_t)(rtc_cal_time_cnt_num64 >> 41);
		rtc_info.wall_clock.tm_year = (rtc_info_cal >> 11) & 0xFF;
		rtc_info.wall_clock.tm_year = 2000 + (rtc_info.wall_clock.tm_year >> 4) * 10 + (rtc_info.wall_clock.tm_year & 0x0F); 
		rtc_info.wall_clock.tm_mon  = (rtc_info_cal >> 6) & 0x1F;
		rtc_info.wall_clock.tm_mon  = (rtc_info.wall_clock.tm_mon >> 4) * 10 + (rtc_info.wall_clock.tm_mon & 0x0F);
		rtc_info.wall_clock.tm_mday = rtc_info_cal & 0x3F;
		rtc_info.wall_clock.tm_mday = (rtc_info.wall_clock.tm_mday >> 4) * 10 + (rtc_info.wall_clock.tm_mday & 0x0F);

		// 计算时/分/秒/毫秒
		rtc_info_timer = (uint32_t)(rtc_cal_time_cnt_num64 >> 9);
		UTCTimerGet(&ulAMPM, &ulHour, &ulMin, &ulSec, &ulMinSec, rtc_info_timer);
		rtc_info.wall_clock.tm_hour = (uint32_t)(ulAMPM * 12 + ulHour);
		rtc_info.wall_clock.tm_min  = ulMin;
		rtc_info.wall_clock.tm_sec  = ulSec;

		// 计算CNT
		rtc_info_cnt = (uint32_t)(rtc_cal_time_cnt_num64 & 0x1FF);
		rtc_info.tm_msec = (uint32_t)((ulMinSec * 10UL) + (rtc_info_cnt / 32UL));

		// 转换为ms
		cp_rtc_msec = xy_mktime(&rtc_info);	
	}
	else
	{
		cp_rtc_msec = rtc_cal_time_cnt_num64;
	}
		
	return cp_rtc_msec;
}

// 将RTC时间结构体格式转为CAL/TIMER/CNT寄存器格式
int Transform_TimeStruct_To_Reg(RTC_TimeTypeDef *rtc_time, RTC_CalTimerCntTypeDef *rtc_cal_time_cnt_reg)
{
	unsigned int  clk_cnt;
    unsigned char ucHourHigh;
    unsigned char ucHourLow;
    unsigned char ucMinHigh;
    unsigned char ucMinLow;
    unsigned char ucSecHigh;
    unsigned char ucSecLow;
    unsigned char ucMinSecHigh;
    unsigned char ucMinSecLow;
    unsigned char ucDateHigh;
    unsigned char ucDateLow;
    unsigned char ucMonthHigh;
    unsigned char ucMonthLow;
   	unsigned char ucYearHigh;
    unsigned char ucYearLow;

	// cnt
	clk_cnt = (rtc_time->tm_msec % 10) * 32;
	if(clk_cnt < 160)
		rtc_cal_time_cnt_reg->tm_cnt = clk_cnt + 256;
	else
		rtc_cal_time_cnt_reg->tm_cnt = clk_cnt - 160;

	// timer
	ucMinSecHigh = (unsigned char)((rtc_time->tm_msec / 10) / 10);
	ucMinSecLow  = (unsigned char)((rtc_time->tm_msec / 10) % 10);

	ucSecHigh  = (unsigned char)(rtc_time->wall_clock.tm_sec  / 10);
	ucSecLow   = (unsigned char)(rtc_time->wall_clock.tm_sec  % 10);

	ucMinHigh  = (unsigned char)(rtc_time->wall_clock.tm_min / 10);
	ucMinLow   = (unsigned char)(rtc_time->wall_clock.tm_min % 10);

	ucHourHigh = (unsigned char)(rtc_time->wall_clock.tm_hour / 10);
	ucHourLow  = (unsigned char)(rtc_time->wall_clock.tm_hour % 10);

   	rtc_cal_time_cnt_reg->tm_timer = (uint32_t)((ucHourHigh   << UTC_ALARM_TIMER_MRT_Pos) | (ucHourLow   << UTC_ALARM_TIMER_MRU_Pos) | 
                                                (ucMinHigh    << UTC_ALARM_TIMER_MT_Pos)  | (ucMinLow    << UTC_ALARM_TIMER_MU_Pos)  | 
                                                (ucSecHigh    << UTC_ALARM_TIMER_ST_Pos)  | (ucSecLow    << UTC_ALARM_TIMER_SU_Pos)  | 
                                                (ucMinSecHigh << UTC_ALARM_TIMER_HT_Pos)  | (ucMinSecLow << UTC_ALARM_TIMER_HU_Pos));

    // cal
   	ucDateHigh = (unsigned char)(rtc_time->wall_clock.tm_mday / 10);
   	ucDateLow  = (unsigned char)(rtc_time->wall_clock.tm_mday % 10);

    ucMonthHigh = (unsigned char)(rtc_time->wall_clock.tm_mon / 10);
    ucMonthLow  = (unsigned char)(rtc_time->wall_clock.tm_mon % 10);

	xy_assert(rtc_time->wall_clock.tm_year >= 2000);
    rtc_time->wall_clock.tm_year = rtc_time->wall_clock.tm_year - 2000;
    ucYearHigh = (unsigned char)(rtc_time->wall_clock.tm_year / 10);
    ucYearLow  = (unsigned char)(rtc_time->wall_clock.tm_year % 10);
    
    rtc_cal_time_cnt_reg->tm_cal = (uint32_t)((ucYearHigh  << UTC_CAL_YT_Pos)       | (ucYearLow << UTC_CAL_YU_Pos)        |
									          (ucMonthHigh << UTC_ALARM_CAL_MT_Pos) | (ucMonthLow << UTC_ALARM_CAL_MU_Pos) | 
                                              (ucDateHigh  << UTC_ALARM_CAL_DT_Pos) | (ucDateLow  << UTC_ALARM_CAL_DU_Pos));

    return 0;
}

// 判断当前ALARM超时时间是否超过一天
int Is_Alarm_Over_Day(RTC_TimeTypeDef *cur_rtc_time, RTC_TimeTypeDef *clock_offset, RTC_CalTimerCntTypeDef *rtc_cal_time_cnt_reg)
{
	uint32_t clk_cnt;
	uint32_t tmp;
	uint32_t msec;
	uint32_t sec;
	uint32_t min;
	uint32_t hour;
	uint32_t Hundredths_sec;

	uint8_t msec_carry = 0;
	uint8_t sec_carry = 0;
	uint8_t min_carry = 0;

    uint8_t ucHourHigh;
    uint8_t ucHourLow;
    uint8_t ucMinHigh;
    uint8_t ucMinLow;
    uint8_t ucSecHigh;
    uint8_t ucSecLow;
    uint8_t ucMinSecHigh;
    uint8_t ucMinSecLow;

	// 判断ms是否有进位
	tmp = clock_offset->tm_msec + cur_rtc_time->tm_msec;
	if(tmp >= 1000)
	{
		msec = tmp - 1000;
		msec_carry = 1;
	}
	else
	{
		msec = tmp;
	}

	// 判断s是否有进位
	tmp = (uint32_t)(clock_offset->wall_clock.tm_sec + cur_rtc_time->wall_clock.tm_sec + msec_carry);
	if(tmp >= 60)
	{
		sec = tmp - 60;
		sec_carry = 1;
	}
	else
	{
		sec = tmp;
	}

	// 判断min是否有进位
	tmp = (uint32_t)(clock_offset->wall_clock.tm_min + cur_rtc_time->wall_clock.tm_min + sec_carry);
	if(tmp >= 60)
	{
		min = tmp - 60;
		min_carry = 1;
	}
	else
	{
		min = tmp;
	}

	// 判断h是否有进位
	tmp = (uint32_t)(clock_offset->wall_clock.tm_hour + cur_rtc_time->wall_clock.tm_hour + min_carry);
	
	// 跨天时或者离下一天只剩一分钟时直接返回，预留一分钟的余量,防止跨天时发生异常
	if((tmp >= 24) || (tmp == 23 && min == 59))
	{
		return 1;
	}
	else
	{
		hour = tmp;
	}	

	// 记录cnt寄存器
	clk_cnt = (msec % 10) * 32;
	if(clk_cnt < 160)
		rtc_cal_time_cnt_reg->tm_cnt = clk_cnt + 256;
	else
		rtc_cal_time_cnt_reg->tm_cnt = clk_cnt - 160;

	// 依次获取百分之一秒\秒\分钟\小时,并转化为BCD
	Hundredths_sec = msec / 10;
	ucMinSecHigh = (uint8_t)(Hundredths_sec / 10);
	ucMinSecLow  = (uint8_t)(Hundredths_sec % 10);
	ucSecHigh  = (uint8_t)(sec / 10);
	ucSecLow   = (uint8_t)(sec % 10);
	ucMinHigh  = (uint8_t)(min / 10);
	ucMinLow   = (uint8_t)(min % 10);
	ucHourHigh = (uint8_t)(hour / 10);
	ucHourLow  = (uint8_t)(hour % 10);

	// 记录timer寄存器
   	rtc_cal_time_cnt_reg->tm_timer = (uint32_t)((0 << UTC_ALARM_TIMER_PM_Pos)  | 
                                     (ucHourHigh   << UTC_ALARM_TIMER_MRT_Pos) | (ucHourLow   << UTC_ALARM_TIMER_MRU_Pos) | 
                                     (ucMinHigh    << UTC_ALARM_TIMER_MT_Pos)  | (ucMinLow    << UTC_ALARM_TIMER_MU_Pos)  | 
                                     (ucSecHigh    << UTC_ALARM_TIMER_ST_Pos)  | (ucSecLow    << UTC_ALARM_TIMER_SU_Pos)  | 
                                     (ucMinSecHigh << UTC_ALARM_TIMER_HT_Pos)  | (ucMinSecLow << UTC_ALARM_TIMER_HU_Pos));

	// cal与当前日历保持一致
	rtc_cal_time_cnt_reg->tm_cal = HWREG(UTC_CAL);				 				
	return 0;
}

// 更新某节点，并裁决是否为最近时刻点，并重设硬件RTC
static int insert_new_rtc_event(uint8_t timer_id, uint64_t rtc_cal_time_cnt_num64, rtc_timeout_cb_t callback, uint32_t rtc_period_msoffset, uint8_t rtc_reload)
{
	uint64_t cp_alarm_num64;
	rtc_event_info_t *rtc_info = NULL; 
		
	if(timer_id < RTC_TIMER_CP_BASE || timer_id >= RTC_TIMER_CP_END)
		xy_assert(0);

	xy_mutex_acquire(rtc_mutex, osWaitForever);
	cp_rtc_event_arry[timer_id].timer_id = timer_id;
	cp_rtc_event_arry[timer_id].rtc_alarm64_high = (uint32_t)(rtc_cal_time_cnt_num64 >> 32);
	cp_rtc_event_arry[timer_id].rtc_alarm64_low = rtc_cal_time_cnt_num64 & 0xFFFFFFFF;
	cp_rtc_event_arry[timer_id].callback = callback;
	cp_rtc_event_arry[timer_id].rtc_period_msoffset = rtc_period_msoffset;
	cp_rtc_event_arry[timer_id].rtc_reload = rtc_reload;
	
	rtc_info = xy_rtc_get_next_event();

	// 设置最近的RTC事件ALARM值到硬件alarm寄存器中
	if(rtc_info != NULL)
	{
		cp_alarm_num64 = rtc_info->rtc_alarm64_high;
		cp_alarm_num64 = (cp_alarm_num64 << 32) + rtc_info->rtc_alarm64_low;
		rtc_alarm_reset(cp_alarm_num64);
	}
	else
	{
		xy_assert(0);
	}

	xy_mutex_release(rtc_mutex);

	return 0;
}

// 相对时间设置RTC ALARM事件
int rtc_event_add_by_offset(uint8_t EventId, uint32_t msec_offset, rtc_timeout_cb_t callback, uint8_t rtc_reload)
{
	RTC_TimeTypeDef cur_rtc_time;
	RTC_TimeTypeDef rtc_alarm;
	RTC_TimeTypeDef clock_offset;
	RTC_CalTimerCntTypeDef rtc_cal_time_cnt_reg;
	uint32_t temp_msec_offset;
	uint64_t alarm_ms;
	uint64_t rtc_cal_time_cnt_num64;

	// 32k晶振频率转化
	temp_msec_offset = CONVERT_MS_TO_RTCTICK(msec_offset);

	// 超过一天的RTC以常规流程设置
	if(temp_msec_offset >= SEC_PER_DAY * 1000)
	{
		goto normal_process;
	}

	//获取当前时间（时\分\秒）
	rtc_get_clock(&cur_rtc_time);

	// 毫秒转化为时钟量（时\分\秒）
	Transform_Ms_To_Clock(temp_msec_offset, &clock_offset);

	// 根据当前时间与毫秒偏移,判断ALARM是否超过一天
	if(Is_Alarm_Over_Day(&cur_rtc_time, &clock_offset, &rtc_cal_time_cnt_reg ) != 1 )
	{
		// 将cal、time、cnt寄存器的值整合成64位的立即数，便于存储
		rtc_cal_time_cnt_num64 = Integrate_CalTimeCntReg_Into_Num64(&rtc_cal_time_cnt_reg);

		// 更新RTC链表
		insert_new_rtc_event(EventId, rtc_cal_time_cnt_num64, callback, msec_offset, rtc_reload);
	}
	else
	{
 normal_process:

		// 结合毫秒偏移得出alarm时间结构体
		alarm_ms = rtc_timer_read(&cur_rtc_time) + temp_msec_offset;
		xy_gmtime_r(alarm_ms, &rtc_alarm);

		// 将alarm时间结构体转化为alarm寄存器的值
		Transform_TimeStruct_To_Reg(&rtc_alarm, &rtc_cal_time_cnt_reg) ;

		// alarm寄存器的值转化为64位的立即数，便于存储
		rtc_cal_time_cnt_num64 = Integrate_CalTimeCntReg_Into_Num64(&rtc_cal_time_cnt_reg);

		// 更新RTC链表
		insert_new_rtc_event(EventId, rtc_cal_time_cnt_num64, callback, msec_offset, rtc_reload);
	}
	return 0;
}

// 绝对时间设置RTC ALARM事件
int rtc_event_add_by_global(uint8_t timer_id, uint64_t rtc_msec, rtc_timeout_cb_t callback)
{	
	uint64_t rtc_cal_time_cnt_num64;
	RTC_TimeTypeDef rtc_alarm;
	RTC_CalTimerCntTypeDef rtc_cal_time_cnt_reg;
	
	if (rtc_msec == RTC_ALARM_INVALID)
		return 0;	

	xy_gmtime_r(rtc_msec, &rtc_alarm);

	// 将alarm时间结构体转化为alarm寄存器的值
	Transform_TimeStruct_To_Reg(&rtc_alarm, &rtc_cal_time_cnt_reg) ;

	// alarm寄存器的值转化为64位的立即数，便于存储
	rtc_cal_time_cnt_num64 = Integrate_CalTimeCntReg_Into_Num64(&rtc_cal_time_cnt_reg);

	insert_new_rtc_event(timer_id, rtc_cal_time_cnt_num64, callback, 0, 0);

	return 0;
}

// 返回指定RTC事件的剩余秒偏移
int32_t xy_rtc_next_offset_by_ID(uint8_t timer_id)
{
	int32_t offset = -1;	

	uint64_t rtc_ms;	
	uint64_t cp_rtc_msec;
	uint64_t cp_rtc_alarm_time;

	if (cp_rtc_event_arry[timer_id].rtc_alarm64_high != 0)
	{
		rtc_ms = rtc_get_tick_ms();
		cp_rtc_alarm_time = cp_rtc_event_arry[timer_id].rtc_alarm64_high;
		cp_rtc_alarm_time = (cp_rtc_alarm_time << 32) + cp_rtc_event_arry[timer_id].rtc_alarm64_low;

		cp_rtc_msec = Transform_Num64_To_Ms(cp_rtc_alarm_time);

		if(cp_rtc_msec >= rtc_ms)
		{
			offset = (int32_t)(CONVERT_RTCTICK_TO_MS(cp_rtc_msec-rtc_ms) / 1000);	
		}
	}

	//if reset,rtc phy will reset to 2018/10/1
	return offset;
}

// 删除指定的RTC事件
void xy_rtc_timer_delete(char timer_id)
{
	rtc_event_info_t *rtc_info = NULL;
	uint64_t rtc_info_time;

	xy_mutex_acquire(rtc_mutex, osWaitForever);

	rtc_info = cp_rtc_event_arry + timer_id;
	if(rtc_info == xy_rtc_get_next_event())
	{
	   memset(rtc_info, 0, sizeof(rtc_event_info_t));
	   
	   rtc_info = xy_rtc_get_next_event();
	   if(rtc_info != NULL)
	   {
			rtc_info_time = rtc_info->rtc_alarm64_high;
			rtc_info_time = (rtc_info_time << 32) + rtc_info->rtc_alarm64_low;
			rtc_alarm_reset(rtc_info_time);
	   }
	   else
	   		rtc_alarm_reset(RTC_ALARM_INVALID);
	}
	else if (rtc_info->rtc_alarm64_high != 0)
	{
	   memset(rtc_info, 0, sizeof(rtc_event_info_t));
	}

	xy_mutex_release(rtc_mutex);

}

// RTC事件更新
void rtc_event_refresh()
{
	uint64_t rtc_info_time;

	xy_mutex_acquire(rtc_mutex, osWaitForever);

	rtc_info_time = Get_CP_ALARM_RAM();

	rtc_alarm_reset(rtc_info_time);

	xy_mutex_release(rtc_mutex);
}

//根据时区重置本地世界世界，更新快照信息
int reset_universal_timer(RTC_TimeTypeDef *rtctime, int zone_sec)
{
	if(rtctime->wall_clock.tm_year < 2020)
	{
		xy_printf(0,PLATFORM, WARN_LOG, "USER set universal time ERROR!year=%d",rtctime->wall_clock.tm_year);
	}
	
	set_snapshot_by_wtime(xy_mktime(rtctime)+zone_sec*1000);
	
	return 0;
}

/*软复位/OPENCPU的boot_cp等操作，该区域之前内容仍然有效，无需等待attach成功*/
uint64_t get_cur_UT_ms()
{
	uint64_t  now_rtc_ms;
	uint64_t  rtc_delta_ms;
	uint64_t  ut_ms = 0;

	xy_mutex_acquire(g_UT_timer_mutex, osWaitForever);

	if(g_softap_var_nv->wall_time_ms == 0)
	{
		goto exit;
	}
	get_sema_hardware(1000);

	now_rtc_ms = rtc_get_tick_ms();

	/*RTC计时寄存器被复位，进而可能出现当前时刻点过久的异常*/
	if(now_rtc_ms < g_softap_var_nv->rtc_ms)
	{
		g_softap_var_nv->rtc_ms = 0;
		g_softap_var_nv->wall_time_ms = 0;
		xy_assert(0);
	}
	if(HWREGB(BAK_MEM_XY_DUMP) == 1)/*Release不打印,避免打印频繁,造成刷屏*/
	{
		xy_printf(0,PLATFORM, WARN_LOG, "current rtc msec:%u, freq:%u",(uint32_t)now_rtc_ms, XY_UTC_CLK);
	}
	
	rtc_delta_ms = CONVERT_RTCTICK_TO_MS(now_rtc_ms-g_softap_var_nv->rtc_ms);
	ut_ms = g_softap_var_nv->wall_time_ms + rtc_delta_ms;
	put_sema_hardware();

exit:
	xy_mutex_release(g_UT_timer_mutex);
	return ut_ms;
}
  
//入参zone_sec为0时，获取当前世界时间；zone_sec如果填入当地时区，则获取的是当前格林尼治时间
int get_universal_timer(RTC_TimeTypeDef *rtctime, int zone_sec)
{
	uint64_t real_msec = get_cur_UT_ms();
	if(real_msec)
	{
	 	real_msec = real_msec - (zone_sec * 1000);
		xy_gmtime_r(real_msec, rtctime);
	}
	else
	{
		user_printf("NB not attach,returl local time,such as 2018/10/1!!!");
		return 0;
	}
	return 1;
}

// 获取最近超时的RTC ALARM事件
rtc_event_info_t* xy_rtc_get_next_event()
{
	int index = 0;
	rtc_event_info_t * rtc_info = NULL; 
	uint64_t tmp_rtc_time;
	uint64_t rtc_info_time;

	xy_mutex_acquire(rtc_mutex, osWaitForever);

	for(index = RTC_TIMER_CP_BASE; index < RTC_TIMER_CP_END; index++)
	{
		if(cp_rtc_event_arry[index].rtc_alarm64_high == 0)
			continue;

		tmp_rtc_time = cp_rtc_event_arry[index].rtc_alarm64_high;
		tmp_rtc_time = (tmp_rtc_time << 32) + cp_rtc_event_arry[index].rtc_alarm64_low;
		
		if(!rtc_info || tmp_rtc_time < rtc_info_time)
		{
			rtc_info = &cp_rtc_event_arry[index];
			rtc_info_time = tmp_rtc_time;
		}
	}

	xy_mutex_release(rtc_mutex);

	return rtc_info;
}

// 清空CP侧的RTC链表信息
void reset_cp_rtcinfo(void)
{
	memset((void *)BAK_MEM_CP_RTCLIST_BASE, 0, BAK_MEM_CP_RTCLIST_LEN);
	Set_CP_ALARM_RAM(RTC_ALARM_INVALID);
	rtc_alarm_reset(RTC_ALARM_INVALID);
}

// RTC事件处理流程
void rtc_event_proc()
{
	uint64_t cp_rtc_alarm_time;
	uint8_t  last_event_id = RTC_TIMER_CP_END;
	rtc_event_info_t *rtc_info = NULL;

deal_event:
	// 获取CP侧最近的alarm事件节点
	rtc_info = xy_rtc_get_next_event();

	if(rtc_info != NULL)
	{
		// 获取CP核最近的alarm超时时刻点,此时的alarm值是CAL/TIMER/CNT寄存器合并后的一个64位数
		cp_rtc_alarm_time = rtc_info->rtc_alarm64_high;
		cp_rtc_alarm_time = (cp_rtc_alarm_time << 32) + rtc_info->rtc_alarm64_low;

		// 直接用合并寄存器后的64位数值进行比较，判断当前alarm是否已经超时
		if(Is_RTC_Already_Expired(cp_rtc_alarm_time))
		{
			// 已经超时，保存已经超时的alarm事件ID
			last_event_id = rtc_info->timer_id;
			// 此处只清除alarm值的高32位，因为跨年时alarm值的低32位可能全为0，不参与RTC链表遍历
			rtc_info->rtc_alarm64_high = 0;	
			
			if(rtc_info->callback)
			{
				rtc_info->callback();
			}
			// 处理周期性RTC事件
			if (rtc_info->rtc_reload)
			{
				rtc_event_add_by_offset(rtc_info->timer_id, rtc_info->rtc_period_msoffset, rtc_info->callback, rtc_info->rtc_reload);
			}		
				
			goto deal_event;
		}
		else 
		{
			// 针对同一个alarm超时事件，rtc_event_add_by_offset接口里已经设置过RTC寄存器，此处没必要重设
			// 处理直接设alarm寄存器，但没有添加进RTC链表的情况，比如rtc_set_alarm_by_cnt接口
			if((last_event_id != rtc_info->timer_id) || (last_event_id == RTC_TIMER_CP_END))
			{
				rtc_alarm_reset(cp_rtc_alarm_time);
			}
		}
	}
	else
	{
		rtc_alarm_reset(RTC_ALARM_INVALID);
	}
}

void rtc_tmr_entry()
{
	while (1)
	{
		if (osSemaphoreAcquire(g_rtc_sem, osWaitForever) != osOK)
			continue;

		rtc_event_proc();
	}
}

void rtc_task_init()
{
	osThreadAttr_t thread_attr = {0};

	cp_rtc_event_arry =  (rtc_event_info_t *)BAK_MEM_CP_RTCLIST_BASE;

	thread_attr.name = RTC_TMR_THREAD_NAME;
	thread_attr.priority = RTC_TMR_THREAD_PRIO;
	thread_attr.stack_size = osStackShared;
	rtc_tmr_task_handler = osThreadNew(rtc_tmr_entry, NULL, &thread_attr);
}
void __RAM_FUNC rtc_handler( void )
{
#if RUNTIME_DEBUG
	extern uint32_t xy_runtime_get_enter(void);
	uint32_t time_enter = xy_runtime_get_enter();
#endif

    volatile unsigned long reg_val = HWREG(UTC_INT_STAT); // 读清

	static BaseType_t xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(g_rtc_sem, &xHigherPriorityTaskWoken );
	if(xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR(1);
    }
	UNUSED_ARG(reg_val);

#if RUNTIME_DEBUG
	extern void xy_runtime_get_exit(uint32_t id, uint32_t time_enter);
	xy_runtime_get_exit(UTC_IRQn, time_enter);
#endif
}

void rtc_init(void)
{
	osMutexAttr_t mutex_attr = {0};

	PRCM_ClockEnable(CORE_CKG_CTL_UTC_EN);

	NVIC_IntRegister(UTC_IRQn, rtc_handler, 1);

	g_rtc_sem = osSemaphoreNew(0x1, 0, NULL);
	mutex_attr.attr_bits = osMutexRecursive;
	rtc_mutex = osMutexNew(&mutex_attr);

	osMutexAttr_t ut_mutex_attr = {0};
	ut_mutex_attr.attr_bits = osMutexRecursive;
	g_UT_timer_mutex = osMutexNew(&ut_mutex_attr);

	rtc_task_init();
}

/*用于指示物理层触发帧同步，上电或换小区需要触发一次,其他时机严禁触发*/
int g_do_frame_update = 0;

/*attach成功后收到“+CTZEU:<tz>,<dst>,[<utime>]”，设置原始快照信息，后续依靠update_time_snapshot更新来维持精度*/
void set_snapshot_by_wtime(uint64_t wall_time_ms)
{
	xy_mutex_acquire(g_UT_timer_mutex, osWaitForever);

	uint64_t  rtc_ms = rtc_get_tick_ms();

	get_sema_hardware(1000);

	xy_printf(0,PLATFORM, INFO_LOG, "set_snapshot_by_wtime !");

	if(g_softap_var_nv->frame_ms != 0)
	{
		xy_assert(g_softap_var_nv->rtc_ms != 0);
		
		g_softap_var_nv->frame_ms += CONVERT_RTCTICK_TO_MS(rtc_ms-g_softap_var_nv->rtc_ms);

		xy_printf(0,PLATFORM, INFO_LOG, "update frame ms by rtc delta %d ms!",(rtc_ms-g_softap_var_nv->rtc_ms));
	}
	
	g_softap_var_nv->wall_time_ms = wall_time_ms;
	g_softap_var_nv->rtc_ms = rtc_ms;

	put_sema_hardware();

	xy_mutex_release(g_UT_timer_mutex);

}


uint32_t GetFrameMsByRtcDelta(uint32_t ulOldNetTimeMs, uint32_t ulNewNetTimeMs, uint32_t ulRTC_DeltaMs)
{
    uint32_t    ulExactIntervalMs,ulNumOfPeriod,ulTempDuration;

    ulTempDuration      = ulRTC_DeltaMs - ulNewNetTimeMs + ulOldNetTimeMs;
    ulNumOfPeriod       = (ulTempDuration + 5242880)/10485760;
    ulExactIntervalMs   = ulNumOfPeriod*10485760 + ulNewNetTimeMs - ulOldNetTimeMs;
    
    return ulExactIntervalMs;
}

/*phy在上电或小区变更时，上报帧信息，以通知主线程更新快照信息，以维持精度*/
/*换小区时，并不一定会重新attach，进而也就不会报XYIPDNS*/
void update_snapshot_by_frame(PhyFrameInfo *frame_info)
{
	RTC_TimeTypeDef wtime = {0};
	uint64_t  rtc_ms = rtc_get_tick_ms();
	
	xy_mutex_acquire(g_UT_timer_mutex, osWaitForever);

	get_sema_hardware(1000);

	/*同一个小区的帧信息更新，此处是精度保证的关键，高概率事件*/
	if(g_softap_var_nv->cell_id == frame_info->cell_id && g_softap_var_nv->freq_num == frame_info->freq_num)
	{
		uint32_t offset_rtc = (uint32_t)CONVERT_RTCTICK_TO_MS(rtc_ms-g_softap_var_nv->rtc_ms);
		uint32_t offset_ms = GetFrameMsByRtcDelta(g_softap_var_nv->frame_ms,frame_info->frame_ms,offset_rtc);
		
		xy_printf(0,PLATFORM, INFO_LOG, "update_snapshot_by_frame  same cell!");

		xy_assert(g_softap_var_nv->rtc_ms != 0);

		/*超过10秒，若物理层时间间隔异常，使用RTC差值*/
		if(offset_rtc>10000 || offset_ms>10000)
		{
			/*10分钟内误差，不得超过2秒;一小时内误差，不得超过10秒；*/
			if((offset_rtc<10*60*1000 && abs((int64_t)(offset_rtc-offset_ms))>2000) ||
				(offset_rtc < 60*60*1000 && abs((int64_t)(offset_rtc-offset_ms))>10000) ||
				(offset_rtc >= 60*60*1000 && abs((int64_t)(offset_rtc-offset_ms))>120000))
			{
				if(HWREGB(BAK_MEM_XY_DUMP) == 0)
					offset_ms = offset_rtc;
				else
					xy_printf(0,PLATFORM, INFO_LOG, "WARNING! update_snapshot_by_frame phy cell sync fail!");
			}
		}
		
		/*有可能尚未获取过世界时间*/
		if(g_softap_var_nv->wall_time_ms != 0)
			g_softap_var_nv->wall_time_ms += offset_ms;
		
		g_softap_var_nv->rtc_ms = rtc_ms;
		g_softap_var_nv->frame_ms = frame_info->frame_ms;

		/*与PC机时间应该出入一秒内，超过2秒需要找青云确认*/
		xy_gmtime_r(g_softap_var_nv->wall_time_ms,&wtime);

		xy_printf(0,PLATFORM, INFO_LOG, "update_snapshot_same_cell at %d-%d/%d/%d!",wtime.wall_clock.tm_mday,wtime.wall_clock.tm_hour,wtime.wall_clock.tm_min,wtime.wall_clock.tm_sec);
	}
	/*小区变更，或者尚未获取世界时间。该处应该为小概率事件*/
	else
	{	
		xy_printf(0,PLATFORM, INFO_LOG, "update_snapshot_by_frame fail ! new cell %d %d,old cell %d %d!",frame_info->cell_id,frame_info->freq_num,g_softap_var_nv->cell_id,g_softap_var_nv->freq_num);
		
		/*有旧的快照信息，则通过RTC时刻数来更新*/
		if(g_softap_var_nv->rtc_ms != 0 && g_softap_var_nv->wall_time_ms != 0)
			g_softap_var_nv->wall_time_ms += CONVERT_RTCTICK_TO_MS(rtc_ms-g_softap_var_nv->rtc_ms);
			
		g_softap_var_nv->rtc_ms = rtc_ms;
		g_softap_var_nv->frame_ms = frame_info->frame_ms;

		g_softap_var_nv->cell_id = frame_info->cell_id;
		g_softap_var_nv->freq_num = frame_info->freq_num;

		/*此处精度为RC，与PC机时间出入几秒属于正常现象*/
		xy_gmtime_r(g_softap_var_nv->wall_time_ms,&wtime);

		xy_printf(0,PLATFORM, INFO_LOG, "update_snapshot_new_cell at %d-%d/%d/%d!",wtime.wall_clock.tm_mday,wtime.wall_clock.tm_hour,wtime.wall_clock.tm_min,wtime.wall_clock.tm_sec);

	}

	put_sema_hardware();

	xy_mutex_release(g_UT_timer_mutex);
}

/*phy在上电或小区变更时，上报帧信息，以通知主线程更新快照信息，以维持精度*/
__RAM_FUNC void send_frame_time(uint32_t frame_ms,uint32_t freq_num,uint16_t cell_id)
{	
	/*上电(含深睡唤醒)、小区变更、重新attach收到世界时间时，执行帧信息更新*/
	if((HWREGB(BAK_MEM_32K_CLK_SRC) == 0 && g_softap_fac_nv->frame_cal==1) && 
		(g_do_frame_update == 1 || g_softap_var_nv->freq_num!=freq_num || g_softap_var_nv->cell_id!=cell_id))
	{
		PhyFrameInfo info;
		
		g_do_frame_update = 0;	
		
		info.frame_ms = frame_ms;
		info.freq_num = freq_num;
		info.cell_id = cell_id;
		send_msg_2_proxy(PROXY_MSG_FRAME_TIME,&info, sizeof(PhyFrameInfo));
		xy_printf(0,PLATFORM, INFO_LOG, "send_frame_time!");
	}
}

void set_frame_update_flag()
{
	/*若非RC32K，则不使用物理层帧信息同步快照，退化为原始的快照机制*/
	if(HWREGB(BAK_MEM_32K_CLK_SRC) == 0 && g_softap_fac_nv->frame_cal==1)
	{
		xy_printf(0,PLATFORM, INFO_LOG, "set_frame_update_flag!");
		g_do_frame_update = 1;
	}
}

/*1小时更新一次世界时间快照，深睡后自动无效，以应对永不深睡的产品*/
void set_snapshot_update_timer()
{
	osTimerId_t UT_timer = NULL;
	
	if(HWREGB(BAK_MEM_32K_CLK_SRC) == 0 && g_softap_fac_nv->frame_cal==1)
	{
		g_do_frame_update = 1;
		UT_timer = osTimerNew((osTimerFunc_t)set_frame_update_flag,osTimerPeriodic, NULL,NULL);
		osTimerStart(UT_timer,(60*60*1000));
		xy_printf(0,PLATFORM, INFO_LOG, "set_snapshot_update_timer!");
	}

}
