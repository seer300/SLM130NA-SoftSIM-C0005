/***********************************************************************************
 * @Copyright (c)	:(C)2020, Qindao ieslab Co., Ltd
 * @FileName   	  :hc32_timer_driver.c
 * @Author       	:
 * @Version      	:V1.0
 * @Date         	:2020-7-1
 * @Description	:the function of the entity of system processor
 ************************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "U_timer1uS_driver.h"
#include "mcu_adapt.h"

#define TIM_PERIOD			(2400000)

/* Private variables ---------------------------------------------------------*/
static u32 timer_5ms_old_num[TIMER_5MS_MAX_SUM];
static u32 timer_5ms_set_num[TIMER_5MS_MAX_SUM];

static u32 timer_100ms_old_num[TIMER_100MS_MAX_SUM];
static u32 timer_100ms_set_num[TIMER_100MS_MAX_SUM];


/* Exported functions -------------------------------------------------------*/
/*************************************************
Function: void Timer1usInit(void)
Description: 微秒级定时器初始化
Input：
Return:
Others:
*************************************************/
__RAM_FUNC void Timer1usInit()
{	
	#if TIME_1uS_Timer1
		McuTimerSetPeriod(1,TIM_PERIOD);
		McuTimerEn(1);
	#endif
	
	#if TIME_1uS_Timer2
		McuTimerSetPeriod(2,TIM_PERIOD);
		McuTimerEn(2);
	#endif
}
/*************************************************
Function:  u8 TimerIfSleep()
Description: 是否可以休眠
Input：
Return:
Others:
*************************************************/
__RAM_FUNC u8 TimerIfSleep()
{
	return TRUE;
}
/*************************************************
Function:  u32 Timer1usGetTick(u8 timerNum)
Description: 获取当前计数器的数值
Input：
Return:
Others:
*************************************************/
__RAM_FUNC u32 Timer1usGetTick()
{ 
	#if TIME_1uS_Timer1
	u32 b;
	b=McuTimerGetCountUs(1);
	return b ;
	#endif
	#if TIME_1uS_Timer2
	u32 b;
	b=McuTimerGetCountUs(2);
	return b ;
	#endif
}

/*************************************************
Function:void Timer1usWakeSleep()
Description: 微秒定时器唤醒
Input:
Return:
Others:
*************************************************/
__RAM_FUNC void Timer1usWakeSleep()
{
	Timer1usInit();
}
/*************************************************
Function:void Timer1usPreSleep()
Description: 微秒定时器休眠前接口
Input:
Return:
Others:
*************************************************/
__RAM_FUNC void Timer1usPreSleep()
{
	#if TIME_1uS_Timer1
	McuTimerDis(1);
	PRCM_ClockDisable(CORE_CKG_CTL_TMR1_EN);
	#endif
	#if TIME_1uS_Timer2
	McuTimerDis(2);
	PRCM_ClockDisable(CORE_CKG_CTL_TMR2_EN);
	#endif
}


/*************************************************
Function:void Set5msTimer(u8 timer_5ms_num,u16 timespan_5ms)
Description: 设置5ms定时器
Input:  
Return: 
Others: 
*************************************************/
__RAM_FUNC void Set5msTimer(u8 timer_5ms_num,u16 timespan_5ms)
{
	if(timer_5ms_num < TIMER_5MS_MAX_SUM)
	{
		timer_5ms_set_num[timer_5ms_num] = (u32)timespan_5ms * 1000 * 5;
		timer_5ms_old_num[timer_5ms_num] = Timer1usGetTick();
	}
}

/*************************************************
Function: u16 Check5msTimer(u8 timer_5ms_num)
Description: 检查定时器当前值，如果为0则说明定时时间到
Input:  
Return: 
Others: 
*************************************************/
__RAM_FUNC u8 Check5msTimer(u8 timer_5ms_num)
{
    u32 tmp_count = 0;
	if(timer_5ms_num < TIMER_5MS_MAX_SUM)
	{
		tmp_count = Timer1usGetTick();
	
		if(tmp_count >= timer_5ms_old_num[timer_5ms_num])//定时起止无溢出
		{
			tmp_count -= timer_5ms_old_num[timer_5ms_num];
		}
		else
		{
			tmp_count += (TIM_PERIOD*1000- timer_5ms_old_num[timer_5ms_num]);     
		}

		if(tmp_count >= timer_5ms_set_num[timer_5ms_num])
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		return 0;
	}
}

/*************************************************
Function:void void Set100msTimer(u8 timer_100ms_num,u16 timespan_100ms)
Description: 设置100ms定时器
Input:  
Return: 
Others: 
*************************************************/
__RAM_FUNC void Set100msTimer(u8 timer_100ms_num,u16 timespan_100ms)
{
	if(timer_100ms_num < TIMER_100MS_MAX_SUM)
	{
		timer_100ms_set_num[timer_100ms_num] = (u32)timespan_100ms * 1000 * 100;
		timer_100ms_old_num[timer_100ms_num] = Timer1usGetTick();
	}
}

/*************************************************
Function: u16 Check100msTimer(u8 timer_100ms_num)
Description: 检查定时器当前值，如果为0则说明定时时间到
Input:  
Return: 
Others: 
*************************************************/
__RAM_FUNC u8 Check100msTimer(u8 timer_100ms_num)
{
    u32 tmp_count = 0;
//	u32 tmp_count1 = 0;
	if(timer_100ms_num < TIMER_100MS_MAX_SUM)
	{
		tmp_count = Timer1usGetTick();
//		tmp_count1 = tmp_count;
		if(tmp_count >= timer_100ms_old_num[timer_100ms_num])//定时起止无溢出
		{
			tmp_count -= timer_100ms_old_num[timer_100ms_num];
		}
		else
		{
			tmp_count += (TIM_PERIOD*1000- timer_100ms_old_num[timer_100ms_num]);     
		}

		if(tmp_count >= timer_100ms_set_num[timer_100ms_num])
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		return 0;
	}
}

/*************************************************
Function: u32 Get100msTimer(u8 timer_100ms_num)
Description: 获取定时开始到当前的时间
*************************************************/
__RAM_FUNC u32 Get100msTimer(u8 timer_100ms_num)
{
   u32 tmp_count = 0;
	if(timer_100ms_num < TIMER_100MS_MAX_SUM)
	{
		tmp_count = Timer1usGetTick();
		if(tmp_count >= timer_100ms_old_num[timer_100ms_num])//定时起止无溢出
		{
			tmp_count -= timer_100ms_old_num[timer_100ms_num];
		}
		else
		{
			tmp_count += (TIM_PERIOD*1000- timer_100ms_old_num[timer_100ms_num]);     
		}
		return tmp_count;
	}
	else
	{
		return 0;
	}
}

