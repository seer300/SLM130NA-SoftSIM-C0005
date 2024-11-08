/*
 * sys_time.c
 *
 *  Created on: 2022年6月18日
 *      Author: Administrator
 */
#include <stdint.h>
#include <stdlib.h>
#include "sys_clk.h"
#include "xy_timer.h"
#include "tick.h"
#include "xy_printf.h"
#include "xy_memmap.h"
#include "xy_system.h"
#include "xy_utils.h"
#include "xy_flash.h"
#include "sys_rc32k.h"
#include "xy_ftl.h"
#include "driver_utils.h"

#define portCLK_TICK_ONE_TICK_COUNTS  		((g_32k_clock / CLK_TIMER_DIVN / 1000) + 1)

/*当两个定时器节点相距不到此阈值，认为两个软定时器同时超时*/
#define TIME_EXPIRED_THREHOLD         		(1)  //ms
#define TRANSLATE_TIMEINFO_2_MS(time_info)  (((uint64_t)time_info->tick_high << 32) | time_info->tick_low)

typedef struct
{
	uint8_t  enable;      	      /*!< 是否使能*/
	uint8_t  periodic;      	  /*!< 周期性TIME标志位，为1时表示当前TIME事件为周期性TIME事件，为0时表示非周期性TIME事件*/
	uint16_t tick_high;           /*!< 绝对时间高16位*/
	uint32_t tick_low;    		  /*!< 绝对时间低32位*/
	uint32_t period_msoffset; 	  /*!< 周期性TIME的毫秒偏移量，非周期性TIME可设置该值为0*/
	TimeEventCallback callback;   /*!< TIME回调函数，TIME定时到期被调用 */
}Time_Event_Cb;

typedef struct
{
	uint64_t xNextTimerTick;  /*!< 寄存器中配置的最近超时时刻点，-1表无效 */
	uint32_t uxTickTimerOverflow :24;	/*!< 32位tick溢出次数 */
	uint32_t xNextTimerId  :8;  /*!< 寄存器中配置的最近超时ID,-1表无效 */
	uint32_t LastCntBeforeDsleep;   /*!< 上一次cnt寄存器的值 */
	Time_Event_Cb lp_timer[0];	/*!< time事件结构体 */
}TimeTypeDef;                                                        

/*非深睡保持的定时器，深睡唤醒后需要全部设为无效*/
Time_Event_Cb g_time_nonlp_list[TIMER_NON_LP_END - TIMER_NON_LP_BASE];

/*深睡保持定时器全局，内部包含节点链表*/
TimeTypeDef*  g_Timer_Info = (TimeTypeDef*)BAK_MEM_AP_TIME_BASE;


#define FLASH_AP_TIME_BASE    (CALIB_FREQ_BASE + sizeof(Cali_Ftl_t))   //Flash中AP侧TIME起始地址,


/*获取当前精确的ms数，需要考虑补偿和翻转*/
uint64_t GetAbsoluteTick(void)  
{
	uint64_t xAbsoluteTickCount;
	uint64_t xSurplusCount;
	uint32_t current_Count;
	const uint32_t xOnceOverflowTicks = Convert_Tick_to_Ms(0xFFFFFFFFUL);
	const uint32_t xOnceOverflowSurplusCount = 0x100000000ULL - (uint64_t)Convert_Ms_to_Tick(xOnceOverflowTicks);

	DisablePrimask();

	current_Count = Tick_CounterGet();
	if(current_Count < g_Timer_Info->LastCntBeforeDsleep)    //针对tick溢出翻转所做的处理
	{
		g_Timer_Info->uxTickTimerOverflow++;
	}
	g_Timer_Info->LastCntBeforeDsleep = current_Count;

	xSurplusCount = (uint64_t)xOnceOverflowSurplusCount * (uint64_t)g_Timer_Info->uxTickTimerOverflow + current_Count;

	xAbsoluteTickCount = (uint64_t)g_Timer_Info->uxTickTimerOverflow * xOnceOverflowTicks + \
			(uint64_t)xSurplusCount * CLK_TIMER_DIVN *1000 / g_32k_clock;

	EnablePrimask();

	return xAbsoluteTickCount;
}

/*设置超时寄存器，若无效-1，则寄存器永远不会超时中断触发*/
void SetupNextTimerInterrupt(void)
{
	uint32_t xCompareValue;
	const uint32_t xTimerMaxTick = Convert_Tick_to_Ms(0xFFFFFFFFUL); //32位tick寄存器能支持的最大ms
	uint64_t xConstTickCount = GetAbsoluteTick();
	uint32_t xCountValue = Tick_CounterGet();

	if(g_Timer_Info->xNextTimerTick == NEXT_TIMER_TICK_INVAILD)  //没有事件时,直接关中断
	{
		Tick_APIntDisable(TICK_INT_AP_COMPARE_Msk);

		return;
	}
	/*若已经超时，强行修改为近时刻点*/
	else if(g_Timer_Info->xNextTimerTick <= xConstTickCount)
	{
		xCompareValue = xCountValue + portCLK_TICK_ONE_TICK_COUNTS;
	}
	/*若超时时长超过40天，则强行改为40天*/
	else if((g_Timer_Info->xNextTimerTick - xConstTickCount) > xTimerMaxTick)
	{
		xCompareValue = xCountValue - 1;
	}
	/*clock tick寄存器将发生反正超时*/
	else
	{
		xCompareValue = Convert_Ms_to_Tick(g_Timer_Info->xNextTimerTick);

		/*若即将超时，则加些冗余时间*/
		if((xCompareValue - xCountValue) < portCLK_TICK_ONE_TICK_COUNTS)
		{
			xCompareValue = xCountValue + portCLK_TICK_ONE_TICK_COUNTS;
		}
	}

	if(~(TICK->AP_INT_EN & TICK_INT_AP_COMPARE_Msk)) //compare中断关闭时,先开启compare中断,再设compare值
	{
		Tick_APIntEnable(TICK_INT_AP_COMPARE_Msk);
		Tick_APReadAndClearInt();
	}
	/* Set Compare */
	Tick_APCompareSet(xCompareValue);
}

/*设置下次超时事件并配置寄存器*/
void ResetNextTimer(uint64_t alarm_ms, uint32_t id)
{
	g_Timer_Info->xNextTimerTick = alarm_ms;
	g_Timer_Info->xNextTimerId = id;

	SetupNextTimerInterrupt();
}

/**
 * @brief 获取基于clock tick的上电至现时刻点的精确ms总计数（生命周期内无溢出风险）
 *        注：clktick为32K时钟的32分频，单位为接近1ms。接口内部会计入32K精度以及计算引起的误差，所以该接口精度高。
 *        对于模组形态，该接口使用时必须注意8K BAKMEM深睡下电的情况，8K回写前后调用所得的数值无法比较！Time_Init执行后，回写成功。
 *        因此，请尽量使用Get_Tick()接口。        
 * 
 * @return uint64_t ms总计数值
 */
uint64_t xy_get_ms(void)
{
	return GetAbsoluteTick();
}

/*基于Get_Tick实现的死循环延迟，不受深睡影响，注意收Tick翻转机制限制，Delay_MS转化成tick值不允许溢出.*/
void delay_func_ms(uint32_t Delay_MS)
{
    uint32_t tickstart = Get_Tick();
    while (!Check_Ms_Timeout(tickstart,Delay_MS));
}

Time_Event_Cb* Time_GetEventInfoById(AP_TIMER_EVENT timer_ID)
{
	if(timer_ID < TIMER_LP_END)
		return &g_Timer_Info->lp_timer[timer_ID];
	else
		return &g_time_nonlp_list[timer_ID - TIMER_NON_LP_BASE];
}

int Is_TimeAlreadyExpired(AP_TIMER_EVENT timer_ID)
{
	uint64_t cur_time, time;
	uint32_t ret = 0;
	Time_Event_Cb * time_info = Time_GetEventInfoById(timer_ID);

	DisablePrimask();

	if(time_info->enable == 0)
	{
		EnablePrimask();
		return ret;
	}

	time = TRANSLATE_TIMEINFO_2_MS(time_info);

	cur_time = GetAbsoluteTick();

	if(time < (cur_time + TIME_EXPIRED_THREHOLD))
		ret = 1;

	EnablePrimask();

	return ret;
}

// 获取最近的AP事件节点
Time_Event_Cb* Time_GetNextEventByLimit(int Limit, uint8_t *timer_id)
{
	Time_Event_Cb *time_return = NULL, *time_info = NULL;

	*timer_id = -1;
	
	// 为了防止死锁，此处锁中断必不可少
	DisablePrimask();

	for(int index = 0; index < Limit; index++)
	{
		time_info = Time_GetEventInfoById(index);

		if(time_info->enable == 0
		   || (time_return != NULL && (time_info->tick_high > time_return->tick_high || (time_info->tick_high == time_return->tick_high && time_info->tick_low >= time_return->tick_low))))
		{
			continue;
		}

		time_return = time_info;
		*timer_id = index;
	}

	EnablePrimask();

	return time_return;
}

int Have_Enable_lptimer()
{
	int i=0;
	for(;i<TIMER_LP_END;i++)
	{
		if( g_Timer_Info->lp_timer[i].enable == 1)
			return 1;
	}
	return 0;
}

/*深睡之前，遍历深睡保持节点，配置寄存器。若深睡失败，需要恢复非深睡保持ID*/
int Timer_Reset_ForDslp(int Limit)
{
	Time_Event_Cb *time_info = NULL;
	uint64_t alarm_ms = -1;
	uint8_t timer_id = -1;

	if(g_Timer_Info->xNextTimerId == 0xFF)
		return 0;

	/*深睡进入时，若当前寄存器配置的事件是深睡保持事件，则无需重设*/
	if(Limit == TIMER_LP_END)
	{
		/*若最近事件就是深睡保持，则无需重设寄存器*/
		if(g_Timer_Info->xNextTimerId < TIMER_LP_END)
			return 0;
		
		/*若无有效节点，则设置无效寄存器，以加快深睡流程处理*/
		if(Have_Enable_lptimer() == 0)
		{
			ResetNextTimer(-1, -1);
			return 0;
		}
	}

	time_info = Time_GetNextEventByLimit(Limit, &timer_id);

	if(time_info)
	{
		alarm_ms = TRANSLATE_TIMEINFO_2_MS(time_info);
	}

	ResetNextTimer(alarm_ms, timer_id);

	return 0;
}

bool Time_NewEvent(AP_TIMER_EVENT timer_ID, uint32_t msec_offset, TimeEventCallback callback, uint8_t periodic)
{
	uint64_t alarm_ms;

	Time_Event_Cb * time_info = Time_GetEventInfoById(timer_ID);

	if(msec_offset==0 || msec_offset>MAX_TIME_PERIOD_MS)
	{
		xy_assert(0);
	}

	DisablePrimask();

	alarm_ms = GetAbsoluteTick() + msec_offset;

	time_info->enable = 0;
	time_info->tick_high = (uint32_t)(alarm_ms >> 32);
	time_info->tick_low = alarm_ms & 0xFFFFFFFF;
    time_info->callback = callback;
    mark_dyn_addr(&(time_info->callback));

	time_info->period_msoffset = msec_offset;
	time_info->periodic = periodic;

	EnablePrimask();

	return 1;
}

/*仅限客户适配使用*/
__FLASH_FUNC void Timer_SetTimeVal(AP_TIMER_EVENT timer_ID, uint32_t msec)
{
	xy_printf("long time consuming, don't call in ISR");
	
	uint64_t alarm_ms;
	
	Time_Event_Cb * time_info = Time_GetEventInfoById(timer_ID);

	if(msec > MAX_TIME_PERIOD_MS)
	{
		xy_assert(0);
	}

	DisablePrimask();

	alarm_ms = GetAbsoluteTick() + msec;

	time_info->tick_high = (uint32_t)(alarm_ms >> 32);
	time_info->tick_low = alarm_ms & 0xFFFFFFFF;
	time_info->period_msoffset = msec;
	time_info->periodic = 1;

	EnablePrimask();
}


/*仅限客户适配使用*/
__FLASH_FUNC void Timer_SetCallBack(AP_TIMER_EVENT timer_ID, TimeEventCallback callback)
{
	Time_Event_Cb * time_info = Time_GetEventInfoById(timer_ID);

	DisablePrimask();

	time_info->callback = callback;

	EnablePrimask();
}

__OPENCPU_FUNC bool Time_EventStart(AP_TIMER_EVENT timer_ID)
{
	uint64_t alarm_ms;
	Time_Event_Cb * time_info = Time_GetEventInfoById(timer_ID);

	DisablePrimask();

	time_info->enable = 1;

	alarm_ms = GetAbsoluteTick() + time_info->period_msoffset;
	time_info->tick_high = (uint32_t)(alarm_ms >> 32);
	time_info->tick_low = alarm_ms & 0xFFFFFFFF;

	if(((alarm_ms + TIME_EXPIRED_THREHOLD) < g_Timer_Info->xNextTimerTick))
	{
		ResetNextTimer(alarm_ms, timer_ID);
	}

	EnablePrimask();

	return 1;
}

//返回1表示硬件中设置的超时事件就是当前事件ID
__OPENCPU_FUNC bool Time_EventStop(AP_TIMER_EVENT timer_ID)
{
	uint64_t alarm_ms = -1;
	uint8_t   timer_id = -1;
	Time_Event_Cb * time_info = Time_GetEventInfoById(timer_ID);

	DisablePrimask();

	if(time_info->enable == 1)
	{
		time_info->enable = 0;

		/*若停止的ID就是寄存器中配置的事件，则需重设寄存器*/
		if(timer_ID == g_Timer_Info->xNextTimerId)
		{
			//Timer_Reset_ForDslp(TIMER_NON_LP_END);
			
			time_info = Time_GetNextEventByLimit(TIMER_NON_LP_END, &timer_id);

			if(time_info)
			{
				alarm_ms = TRANSLATE_TIMEINFO_2_MS(time_info);
			}

			ResetNextTimer(alarm_ms, timer_id);
		}
		EnablePrimask();
		return 1;
	}

	EnablePrimask();

	return 0;
}

// 根据相对毫秒数偏移值的设置RTC事件到链表中
__OPENCPU_FUNC bool Timer_AddEvent(AP_TIMER_EVENT timer_ID, uint32_t msec_offset, TimeEventCallback callback, uint8_t is_period_rtc)
{
	xy_printf("long time consuming, don't call in ISR");

	DisablePrimask();

	//停止该事件,若硬件寄存器中设置的超时事件就是当前ID，则需要遍历链表并重设硬定时
	Time_EventStop(timer_ID); 

	Time_NewEvent(timer_ID, msec_offset, callback, is_period_rtc);

	Time_EventStart(timer_ID);

	EnablePrimask();
	
	return 1;
}

//删除TIME事件.删除的TIME事件，延时时间到期不会调用回调函数
__OPENCPU_FUNC bool Timer_DeleteEvent(AP_TIMER_EVENT timer_ID)
{
	return  Time_EventStop(timer_ID);
}

__FLASH_FUNC int Timer_GetEventLeftTime(AP_TIMER_EVENT timer_ID)
{
	int64_t cur_time, time, delta_time;
	Time_Event_Cb * time_info = Time_GetEventInfoById(timer_ID);

	DisablePrimask();

	if(time_info->enable == 0)
	{
		EnablePrimask();
		return -1;
	}

	time = TRANSLATE_TIMEINFO_2_MS(time_info);

	cur_time = GetAbsoluteTick();

	EnablePrimask();

	delta_time = time - cur_time;

	if(delta_time < 0)
	{
		return 0;
	}

	return (int)delta_time;
}

int Time_ProcessEvent(void)
{
	uint64_t alarm_ms = -1;
	uint8_t timer_id = -1;
	Time_Event_Cb *time_info = NULL;

	DisablePrimask();
	// 获取AP侧最近的alarm事件节点
	do
	{
		time_info = Time_GetNextEventByLimit(TIMER_NON_LP_END, &timer_id);

		if(time_info == NULL)
			break;

		if (Is_TimeAlreadyExpired(timer_id))
		{
			//周期性事件后续需要使用time_info->timer_id，所以不能清除
			if(!time_info->periodic)
			{
				time_info->enable = 0;
			}
			//允许在回调里
			if (time_info->callback)
			{
				time_info->callback();
			}

			// 处理周期性RTC事件
			if (time_info->periodic && time_info->enable==1)
			{
				time_info->tick_high = time_info->tick_high + (uint32_t)( ((uint64_t)time_info->tick_low + time_info->period_msoffset) >> 32);
                time_info->tick_low = time_info->tick_low + time_info->period_msoffset;
			}
		}
		else
		{
			break;
		}

	}while(1);

	if(time_info)
	{
		alarm_ms = TRANSLATE_TIMEINFO_2_MS(time_info);
	}

	/*若没有待超时事件，设置为-1*/
	ResetNextTimer(alarm_ms, timer_id);

	EnablePrimask();

	return 0;
}

/*快速恢复场景下，RAM不掉电，需设置非深睡保存节点为无效*/
void Time_Non_Lp_Init(void)
{
	for(int index = TIMER_NON_LP_BASE; index < TIMER_NON_LP_END; index++)
	{
		g_time_nonlp_list[index - TIMER_NON_LP_BASE].enable = 0;
	}
}


/*深睡前保存链表及深睡保持定时器至flash*/
__OPENCPU_FUNC void Save_ApTime_To_Flash()
{
	if(g_Timer_Info->xNextTimerId != 0xFF) //链表中有有效值则保存至flash
	{
		xy_assert(xy_ftl_write(FLASH_AP_TIME_BASE, (void *)g_Timer_Info, FLASH_AP_TIME_LEN) != false);
	}
}

__OPENCPU_FUNC static void Restore_ApTime_From_Flash()
{
	if(*(volatile uint64_t*)(FLASH_AP_TIME_BASE + 4) == NEXT_TIMER_TICK_INVAILD)
	{
		Time_Event_Cb *time_info = NULL;

		g_Timer_Info->uxTickTimerOverflow = 0;
		g_Timer_Info->xNextTimerTick = -1;
		g_Timer_Info->xNextTimerId = -1;
		g_Timer_Info->LastCntBeforeDsleep = 0;

		for(int index = TIMER_LP_BASE; index < TIMER_NON_LP_BASE; index++)
		{
			time_info = Time_GetEventInfoById(index);

			time_info->enable = 0;
		}
	}
	else
	{
		xy_assert(xy_ftl_read(FLASH_AP_TIME_BASE, (void *)(BAK_MEM_AP_TIME_BASE), BAK_MEM_AP_TIME_LEN) != false);
	}
}


__FLASH_FUNC int Time_Init(void)
{
	if(Get_Boot_Reason() != WAKEUP_DSLEEP)
	{
		Time_Event_Cb *time_info = NULL;

		g_Timer_Info->uxTickTimerOverflow = 0;
		g_Timer_Info->xNextTimerTick = -1;
		g_Timer_Info->xNextTimerId = -1;
		g_Timer_Info->LastCntBeforeDsleep = 0;

		for(int index = TIMER_LP_BASE; index < TIMER_NON_LP_BASE; index++)
		{
			time_info = Time_GetEventInfoById(index);

			time_info->enable = 0;
		}
	}
#if MODULE_VER
	else
	{
		Restore_ApTime_From_Flash();   //AP_RAM掉电的深睡唤醒（非快速恢复），从flash中恢复ap time链表
	}	
#endif

	Time_Non_Lp_Init();

	return 0;
}

// 设置每天某个时间段的随机RTC事件
__FLASH_FUNC bool  Timer_Set_By_Day(AP_TIMER_EVENT timer_ID, TimeEventCallback callback, int sec_start, int sec_span)
{
	int sec_offset;
	uint32_t msec_offset;
	RTC_TimeTypeDef now_wall_time = {0};

	xy_assert(timer_ID < TIMER_LP_END);
	
	// 若没有ATTACH成功过，now_wall_time就只是一个本地时间，不是世界北京时间
	if ( !Get_Current_UT_Time(&now_wall_time))
		return 0;

	sec_offset = (int)((uint32_t)sec_start - (now_wall_time.tm_hour * 3600 + now_wall_time.tm_min * 60 + now_wall_time.tm_sec));
	if (sec_offset <= 0)
		sec_offset += 3600 * 24;

	if(sec_span == 0)
		msec_offset = (uint32_t)((sec_offset) * 1000);
	else
	{
		srand(xy_seed());
		msec_offset = (uint32_t)((sec_offset + (rand() % sec_span)) * 1000);
	}

	Timer_AddEvent(timer_ID, msec_offset, callback, 0);
	return 1;
}

// 设置每周某个时间段的随机RTC事件
__FLASH_FUNC bool  Timer_Set_By_Week(AP_TIMER_EVENT timer_ID, TimeEventCallback callback, int day_week, int sec_start, int sec_span)
{
	int sec_offset;
	uint32_t msec_offset;
	RTC_TimeTypeDef now_wall_time = {0};

	// 若没有ATTACH成功过，now_wall_time就只是一个本地时间，不是世界北京时间
	if (!Get_Current_UT_Time(&now_wall_time))
		return 0;

	sec_offset = (int)(((uint32_t)day_week - now_wall_time.tm_wday) * 3600 * 24 + (uint32_t)sec_start - (now_wall_time.tm_hour * 3600 + now_wall_time.tm_min * 60 + now_wall_time.tm_sec));
	if (sec_offset <= 0)
		sec_offset += 7 * 3600 * 24;

	if(sec_span == 0)
		msec_offset = (uint32_t)((sec_offset) * 1000);
	else
	{
		srand(xy_seed());
		msec_offset = (uint32_t)((sec_offset + (rand() % sec_span)) * 1000);
	}

	Timer_AddEvent(timer_ID, msec_offset, callback, 0);

	return 1;
}

/**
 * @brief 使用Tick外设进行超时判断。
 *        注：考虑Tick外设计数溢出一次为49天，结合使用场景，存在芯片唤醒后Tick处于即将溢出边缘，
 *        如果立即处理某些需要超时判断的任务，tick值会出现新tick值小于旧tick值的情况，
 *        但该超时判断任务的执行时长不可能大于49天，因此Tick外设不会再次溢出，该接口无二次溢出风险。        
 * 
 * @param start_tick 进入超时判断前获取的tick个数，注意：需保证该值在一个任务的多轮超时判断中是一个定值
 * @param timeout_ms 指定超时时长，单位ms，超时判断任务的执行时长大于等于该时长判定为超时
 * @return bool        1:超时  0:未超时
 */
bool Check_Ms_Timeout(uint32_t start_tick, uint32_t timeout_ms)
{
	uint32_t timeout_tick = (uint32_t)Convert_Ms_to_Tick(timeout_ms);  //注意超时ms转成tick不允许溢出
	uint32_t new_tick = Tick_CounterGet(); // 超时判断时，实时更新的tick个数

	// 无溢出情况：新tick值大于等于旧tick值
	if(new_tick >= start_tick)
	{
		if(new_tick - start_tick >= timeout_tick)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	// 溢出情况：新tick值小于旧tick值
	else
	{
		uint32_t tick_temp = 0xFFFFFFFF - start_tick;
		if(tick_temp + new_tick >= timeout_tick)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
}
