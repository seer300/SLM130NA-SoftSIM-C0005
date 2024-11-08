
/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#include <string.h>
#include <stdlib.h>

#include "sema.h"
#include "xy_memmap.h"
#include "system.h"
#include "core_cm3.h"
#include "prcm.h"
#include "sys_proc.h"
#include "xy_timer.h"
#include "hal_def.h"
#include "xy_system.h"
#include "driver_utils.h"

snapshot_t *g_snapshot = (snapshot_t *)RAM_NV_VOLATILE_SOFTAP_START;

/*******************************************************************************
 *                        Global function definitions                          *
 ******************************************************************************/
 


void  get_sema_hardware()
{
	do{
	    SEMA_RequestNonBlocking(SEMA_IDLE_HARDWARE , SEMA_SEMA_DMAC_NO_REQ, SEMA_SEMA_REQ_PRI_0, SEMA_MASK_CP);
	}while(SEMA_MASTER_AP != SEMA_MasterGet(SEMA_IDLE_HARDWARE));
}

void  put_sema_hardware()
{
	SEMA_Release(SEMA_IDLE_HARDWARE, SEMA_MASK_NULL);
}

// 读取当前时间寄存器值获取year/mon/day/hour/min/sec
// 务必放RAM！！
__FLASH_FUNC int RTC_GetTime(RTC_TimeTypeDef *rtctime)
{
	uint8_t ulCentury = 0;
	uint8_t ulYear = 0;
	uint8_t ulMonth = 0;
	uint8_t ulData;
	uint8_t ulDay;

	uint8_t ulAMPM;
	uint8_t ulHour;
	uint8_t ulMin;
	uint8_t ulSec;
	uint8_t ulMinSec;
	volatile uint8_t carryflag = 0;

	uint32_t ulclkcnt;
	volatile uint32_t cnttmp;
	volatile uint32_t utc_delay;

	DisablePrimask();

	cnttmp = UTC->CLK_CNT;

	// utccnt 从159变到256后，需要几个cycle才会更新time reg
	// 当前解决方案，utccnt从159变化至256，软件delay 10次for循环，然后再读取time reg
	while(cnttmp==159||cnttmp==415)
	{
		cnttmp = UTC->CLK_CNT;
		carryflag = 1;
	}

	if(carryflag)
	{   // bug3356
		for(utc_delay=0;utc_delay<10;utc_delay++);
	}

	UTC_TimerGet(&ulAMPM, &ulHour, &ulMin, &ulSec, &ulMinSec, 0);
	UTC_CalGet(&ulCentury, &ulYear, &ulMonth, &ulData, &ulDay, 0);
	ulclkcnt = UTC_ClkCntConvert(cnttmp);

	EnablePrimask();

	rtctime->tm_hour = (uint32_t)(ulAMPM * 12 + ulHour);//默认为24小时制，ulAMPM为0；此处需要防止被修改为AM/PM模式
	rtctime->tm_min  = ulMin;
	rtctime->tm_sec  = ulSec;
	rtctime->tm_msec = (uint32_t)(ulMinSec * 10) + ulclkcnt / 32;
	rtctime->tm_mday = ulData;
	rtctime->tm_mon  = ulMonth;
	rtctime->tm_year = (uint32_t)(ulCentury * 100 + ulYear);
	rtctime->tm_wday = ulDay;
	rtctime->tm_yday = (uint32_t)(ulData + clock_days_before_month((int)(rtctime->tm_mon-1), clock_is_leapyear((int)rtctime->tm_year)));
	rtctime->tm_isdst = 0;

	return 0;
}

// 获取某毫秒偏移量对应的相距1970/1/1的毫秒数
// 务必放RAM！！
__FLASH_FUNC uint64_t RTC_Get_Global_Byoffset(uint64_t msec_offset)
{
	uint64_t rtc_msec;
	RTC_TimeTypeDef rtc_time;

	RTC_GetTime(&rtc_time);
	rtc_msec = ((uint64_t)(msec_offset)*XY_UTC_CLK/32000ULL) + xy_mktime(&rtc_time);
	return rtc_msec;
}


/*系统软复位、OPENCPU的boot_cp等操作，仍然可以获取之前的世界时间*/
// 务必放RAM！！通过硬件锁解决双核互斥
__FLASH_FUNC bool Get_Current_UT_Time(RTC_TimeTypeDef *rtctime)
{
	int32_t ret;
	int get_sema = 0;
#if XY_DEBUG
	uint32_t start = Get_Tick();
#endif
	if (CP_Is_Alive() == true)
	{
		get_sema = 1;
		get_sema_hardware();
	}
	
	if ((g_snapshot->rtc_ms != 0) && (g_snapshot->wall_time_ms != 0))
	{
		uint64_t now_ms = RTC_Get_Global_Byoffset(0);

		/*测试代码rtc_init_test被调用时，可能进该处断言*/
		xy_assert(now_ms >= g_snapshot->rtc_ms);
		xy_gmtime((g_snapshot->wall_time_ms + ((uint64_t)(now_ms - g_snapshot->rtc_ms) * 32000) / XY_UTC_CLK), rtctime);
		
		ret = 1;
	}
	else
	{
		xy_printf("Get_Current_UT_Time fail! %lx %lx\r\n",g_snapshot->wall_time_ms,g_snapshot->rtc_ms);
		RTC_GetTime(rtctime);
		ret = 0;
	}
	
	if(get_sema == 1)
	{
		get_sema = 0;
		put_sema_hardware();
	}
	
#if XY_DEBUG
	xy_printf("Get_Current_UT_Time  take time:%d\r\n",Convert_Tick_to_Ms(Get_Tick()-start));
#endif

	return ret;
}

__FLASH_FUNC bool Get_GMT_Time(RTC_TimeTypeDef *rtctime)
{
	int32_t ret;
	int get_sema = 0;
#if XY_DEBUG
	uint32_t start = Get_Tick();
#endif
	if (CP_Is_Alive() == true)
	{
		get_sema = 1;
		get_sema_hardware();
	}

	if ((g_snapshot->rtc_ms != 0) && (g_snapshot->wall_time_ms != 0))
	{
		uint64_t now_ms = RTC_Get_Global_Byoffset(0);

		/*测试代码rtc_init_test被调用时，可能进该处断言*/
		xy_assert(now_ms >= g_snapshot->rtc_ms);
		int32_t zone_sec = (int32_t)g_snapshot->g_zone * 15 * 60;
		xy_gmtime((g_snapshot->wall_time_ms + ((uint64_t)(now_ms - g_snapshot->rtc_ms) * 32000) / XY_UTC_CLK) - zone_sec * 1000, rtctime);

		ret = 1;
	}
	else
	{
		xy_printf("Get_Current_UT_Time fail! %lx %lx\r\n", g_snapshot->wall_time_ms, g_snapshot->rtc_ms);
		RTC_GetTime(rtctime);
		ret = 0;
	}

	if (get_sema == 1)
	{
		get_sema = 0;
		put_sema_hardware();
	}

#if XY_DEBUG
	xy_printf("Get_Current_UT_Time  take time:%d\r\n", Convert_Tick_to_Ms(Get_Tick() - start));
#endif

	return ret;
}

/*根据入参ms偏移，得出对应的年月日时分秒。该接口比Get_Current_UT_Time运行耗时更短。缺陷是不支持星期几*/
void Get_UT_Time_Fast(RTC_TimeTypeDef *time,uint32_t ms_diff)
{
    uint8_t DayNum;
	uint8_t const DayTab[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; //月时间常量

	xy_assert(ms_diff <= 60000);
		
	time->tm_sec += (time->tm_msec + ms_diff) / 1000;
	time->tm_msec = (time->tm_msec + ms_diff) % 1000;
	
    if(time->tm_sec> 59)
    {
        time->tm_sec -= 60;

        time->tm_min++;

        if(time->tm_min > 59)
        {
            time->tm_hour++;
            time->tm_min = 0;

            if(time->tm_hour > 23)
            {
                time->tm_hour = 0;
                time->tm_mday++;

                DayNum = DayTab[time->tm_mon - 1];

				/*润2月*/
                if(time->tm_mon == 2)
                {
                    if((time->tm_year & 0x03) == 0)
                    {
                        DayNum = 29;
                    }
                }

                if(time->tm_mday > DayNum)
                {
                    time->tm_mday = 1;
                    time->tm_mon++;
                }

                if(time->tm_mon > 12)
                {
                    time->tm_mon = 1;
                    time->tm_year++;
                }
            }
        }
    }
}


/*若有快照信息，AP核不得设置世界时间，以防止修改快照信息。通过硬件锁解决双核互斥*/
__FLASH_FUNC void Set_UT_Time(RTC_TimeTypeDef *rtctime)
{
	int get_sema = 0;
	
	if (CP_Is_Alive() == true)
	{
		get_sema = 1;
		get_sema_hardware();
	}

	/*仅容许用户在未驻留小区之前设置一次本地时间*/
	if(g_snapshot->wall_time_ms != 0)
	{
		xy_printf("Set_UT_Time WARNING! have setted %lx\r\n",g_snapshot->wall_time_ms);
	}

	g_snapshot->wall_time_ms = xy_mktime(rtctime);
	g_snapshot->rtc_ms = RTC_Get_Global_Byoffset(0);

	if(get_sema == 1)
	{
		get_sema = 0;
		put_sema_hardware();
	}
}


__FLASH_FUNC void rtc_init_intern(void)
{
	// #if (CFG_XTAL32K_EXIST == 0x01) && ((CFG_CLOCK_SRC_SELECT == 0x01) || (CFG_UTC_CLK_TYPE == 0x01))
	// 	if(g_fast_startup_flag == AP_WAKEUP_NORMAL)
	// 	{
	// 		Power_Up_Outer_32k_Xtal();
	// 	}
	// #endif

	if (Get_Boot_Reason() < SOFT_RESET)
	{
		//UTC_CalStop();
		//UTC_TimerStop();
		//UTC_DivStop();

		UTC_IntDisable(UTC_INT_ALARM);
		UTC_AlarmDisable(UTC_ALARM_ALL);
		// clk_cnt_alarm_ena is set to default zero
		UTC_AlarmCntCheckDisable();

		//UTC_TimerSet(0, 0, 0, 0, 0);
		//for (volatile uint32_t i = 0; i < 100; i++);
		//UTC_CalSet(20, 00, 1, 1, 6);
		//for (volatile uint32_t i = 0; i < 100; i++);
	}

	UTC_TimerRun();
	UTC_CalRun();
	UTC_DivEn();
}


