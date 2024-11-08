#include <string.h>
#include "at_uart.h"
#include "xy_timer.h"
#include "at_cmd_regist.h"
#include "xy_memmap.h"
#include "system.h"
#include "xy_system.h"
#include "xy_utils.h"
#include "tick.h"


__RAM_FUNC void Time_Test_Timeout(void)
{
	Send_AT_to_Ext("\r\nTime timeout!\r\n");

}

uint32_t xy_timer_overflow_test(uint32_t overflowsec)
{
    uint32_t timeroffset = 0XFFFFFFFF - overflowsec*1000; //设置overflowsec秒后clktick_cnt寄存器计满并归零继续计数
    Tick_TimerDisable();
    Tick_CounterSet(timeroffset);
    Tick_TimerEnable();

    return timeroffset;
}
/*距离新的一年的倒计时，newyear只能取2001或2002;countdown为分钟数，取值范围1---(24*60-1)*/
void rtc_init_test(uint32_t newyear,uint32_t countdown)
{
	uint32_t hour;
	uint32_t min;
	
	UTC_CalStop();
	UTC_TimerStop();
	UTC_DivStop();

	UTC_IntDisable(UTC_INT_ALARM);
	UTC_AlarmDisable(UTC_ALARM_ALL);
	// clk_cnt_alarm_ena is set to default zero
	UTC_AlarmCntCheckDisable();

	hour = (24*60-countdown)/60;
	min  = 60-(24*60-countdown)%60;
	/*元旦前一天的倒计时*/	
	UTC_TimerSet(0,hour,min, 0, 0);
	for (volatile uint32_t i = 0; i < 100; i++);
	
	/*2001到2002的跨年*/
	if(newyear == 2002)
		UTC_CalSet(20, 01, 12, 31, 1); //应该为星期一
	/*2000到2001的跨年*/
	else
		UTC_CalSet(20, 00, 12, 31, 7); //应该为星期天
	for (volatile uint32_t i = 0; i < 100; i++);

	UTC_TimerRun();
	UTC_CalRun();
	UTC_DivEn();
}

int at_timer_test(uint32_t val1,uint32_t val2,uint32_t val3,uint32_t val4,uint32_t val5,uint32_t val6)
{
	char rsp[50] = {0};
	/*AT+APTEST=TIMER,0,0,0,<val4>.其中val4指示多少秒后clktick_cnt寄存器计满并归零继续计数，以构造翻转的压力测试*/
	/*注意，需要与mode=1搭配使用，如：
		AT+APTEST=TIMER,0,0,0,10   //指示10s后clktick_cnt寄存器计满并归零继续计数
		AT+APTEST=TIMER,1,1,1,4000 //指示开启一个4s的周期性定时器
		若后续周期性定时正常则测试通过
	*/
	if (val1 == 0)	
	{
		uint32_t offset = xy_timer_overflow_test(val4);

		char *ret = xy_malloc(40);
		snprintf(ret, 40, "\r\n+val4 %lu,offset 0x%lx\r\n", val4, offset);
		Send_AT_to_Ext(ret);
		xy_free(ret);
	}
	/*AT+APTEST=TIMER,1,<timer id>,<period or not>,<time ms offset>. 开启某ID timer*/
	else if (val1 == 1)  //设置定长的单个定时器
	{
		Timer_AddEvent(val2,val4, Time_Test_Timeout, val3);
	}
	/*AT+APTEST=TIMER,2,<timer id>,<period or not>,<time ms max>. 开启某ID timer，时长为随机值*/
	else if (val1 == 2)  //设置随机时长的单个定时器
	{
		srand(xy_seed());
		val4 = rand()%val4;

		snprintf(rsp, 40, "\r\n+TIME%ld rand offset:%ld\r\n", val2, val4);
		Send_AT_to_Ext(rsp);	

		Timer_AddEvent(val2, val4, Time_Test_Timeout, val3);
	}
	/*AT+APTEST=TIMER,3,<timer id> 查询定时器当前的剩余时间*/
	else if (val1 == 3) 
	{
		snprintf(rsp, 40, "\r\n+TIME%ld left time:%d\r\n", val2, Timer_GetEventLeftTime(val2));
		Send_AT_to_Ext(rsp);	
	}
	/*AT+APTEST=TIMER,4  查询世界时间*/
	else if (val1 == 4)  
	{
		RTC_TimeTypeDef rtctime = {0};

		if (Get_Current_UT_Time(&rtctime))
		{
			snprintf(rsp, 48, "\r\n+WALLTIME:%02ld/%02lu/%02lu,%02lu:%02lu:%02lu,week:%02lu\r\n", rtctime.tm_year, rtctime.tm_mon, rtctime.tm_mday, rtctime.tm_hour, rtctime.tm_min, rtctime.tm_sec,rtctime.tm_wday);
			Send_AT_to_Ext(rsp);
		}
		else
		{
			Send_AT_to_Ext("\r\n+TIME:not attach\r\n");
		}
	}
	/*AT+APTEST=TIMER,5,<timer id> 杀定时器*/
	else if (val1 == 5)	
	{
		Timer_DeleteEvent(val2);
	}	
	/*AT+APTEST=TIMER,6,<timer id>,0,<sec_start>,<sec_span>  天定时器*/
	else if (val1 == 6)	
	{
		Timer_Set_By_Day(val2, Time_Test_Timeout, val4, val5); 
	}
	/*AT+APTEST=TIMER,7,<timer id>,0,<day_week>,<sec_start>,<sec_span>  周定时器*/
	else if (val1 == 7)	
	{
		Timer_Set_By_Week(val2, Time_Test_Timeout, val4, val5, val6);
	}
	/*AT+APTEST=TIMER,8,<newyear>,<countdown>  设置元旦前倒计时*/
	else if(val1 == 8)
	{
		rtc_init_test(val2,val3);
	}
	/*AT+APTEST=TIMER,9,<tm_year>,<tm_mon>,<tm_mday>,<tm_hour>,<tm_min>,<tm_sec>  设置世界时间的墙上时间，只能在attach之前设置*/
	else if(val1 == 9)
	{
		RTC_TimeTypeDef rtctime={0};
		
		rtctime.tm_year = val2;
	    rtctime.tm_mon  = val3;
	    rtctime.tm_mday = val4;
	    rtctime.tm_hour = val5;
	    rtctime.tm_min  = val6;
		
		Set_UT_Time(&rtctime);
			
		sprintf(rsp,"\r\n+WALLTIME:%02ld/%02lu/%02lu,%02lu:%02lu:%02lu,week:%02lu\r\n", rtctime.tm_year, rtctime.tm_mon, rtctime.tm_mday, rtctime.tm_hour, rtctime.tm_min, rtctime.tm_sec,rtctime.tm_wday);
		Send_AT_to_Ext(rsp);		
	}
	return XY_OK;
}


