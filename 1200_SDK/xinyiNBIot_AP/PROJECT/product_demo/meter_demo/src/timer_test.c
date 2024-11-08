#include <string.h>
#include "xy_system.h"
#include "xy_timer.h"
#include "hal_adc.h"
#include "xy_printf.h"
#include "basic_config.h"
#include "at_uart.h"
#include "user_config.h"
#include "xy_event.h"
#include "mcu_adapt.h"
#include "utc_watchdog.h"
#include "ap_watchdog.h"

/************************************************************************************************************
***  重点测试mcu_timer.c中的接口耗时、功耗开销、稳定性、启停CP核的健壮性等
*************************************************************************************************************/

/* 设置xy_timer的定时周期，单位ms */
uint16_t xy_timer_timing_period = 1000;
/* 设置lptimer的定时周期，单位ms */
uint16_t lptimer_timing_period  = 2000;
/* 设置timer2的定时周期，单位ms */
uint16_t timer2_timing_period   = 3000;

//timer的测试模式，0：丢中断检查，1:翻转GPIO5 200次
uint8_t g_timer_mode = 1;
/************************************************************************************
* @brief  定时器周期性工作时间/唤醒时间设置 AT+DEMOCFG=TIMER,<timer_type>,<timer_mode>,<xy_tmier period sec>,<Lptmier period sec>,<TIMER2 period sec>
* @param  val1: timer_type，定时器类型选择，bit0：xytimer,bit1:lptimer,bit2:timer2,置位某位代表初始化该定时器
* @param  val2：timer_mode，定时器测试模式选择，0：丢中断检查，1:翻转GPIO 200次
* @param  val3：xytimer工作时间设置
* @param  val4：lptimer工作时间设置
* @param  val5：timer2工作时间设置
* @return  0：成功。 -1：失败，非法参数
************************************************************************************/
int timer_test_set(char **prsp_cmd,int val1,int val2,int val3,int val4,int val5)
{
	(void)prsp_cmd;
	g_timer_mode = val2;
	
	if(val1 & (0x01 << 0))
	{
		xy_timer_timing_period = val3;
		xy_printf("\r\nxy_timer mode,Waketime cycle: %d\r\n", xy_timer_timing_period);
	}
	if(val1 & (0x01 << 1))
	{
		lptimer_timing_period = val4;
		xy_printf("\r\nlptimer mode,Worktime cycle: %d\r\n", lptimer_timing_period);
	}
	if(val1 & (0x01 << 2))
	{
		timer2_timing_period = val5;
		xy_printf("\r\ntimer2_timing_period mode,Worktime cycle: %d\r\n", timer2_timing_period);
	}
	return XY_OK;
}

/************************************************************************************
* @brief  xy_timer中断检查，中断触发时间大于定时周期xy_timer_timing_period的1.5倍进断言
************************************************************************************/
void check_xy_timer_int(void)
{
	static uint32_t xy_timer_last_time_tick = 0;

	//此处判断xy_timer是否会出现漏中断的情况，如2次运行到该处的时间差和设置的周期有明显的差异，认为丢失中断，这里取xy_timer周期的1.5倍作为判断条件；
	//第一次触发xy_timer中断时，xy_timer_last_time_tick未记录到上一次的时间点，故跳过第一次；
	if(xy_timer_last_time_tick != 0 && Check_Ms_Timeout(xy_timer_last_time_tick, (1.5 * xy_timer_timing_period)))
	{
		xy_printf("xy timer lose int\r\n");
		xy_assert(0);//lptimer_timing_period ms唤醒周期，若2次进中断时间大于中断周期的1.5倍，判定为xy_timer丢中断；
	}

	//记录最近一次触发中断的时间，用于中断丢失的判断
	xy_timer_last_time_tick = Get_Tick();
}

/************************************************************************************
* @brief  Lptimer中断检查，中断触发时间大于定时周期lptimer_timing_period的1.5倍进断言
************************************************************************************/
void check_lptimer_int(void)
{
	static uint32_t lptimer_last_time_tick = 0;

	//此处判断lptimer是否会出现漏中断的情况，如2次运行到该处的时间差和设置的周期有明显的差异，认为丢失中断，这里取lptimer周期的1.5倍作为判断条件；
	//第一次触发lptimer中断时，lptimer_last_time_tick未记录到上一次的时间点，故跳过第一次；
	if(lptimer_last_time_tick != 0 && Check_Ms_Timeout(lptimer_last_time_tick, (1.5 * lptimer_timing_period)))
	{
		xy_printf("lptimer lose int\r\n");
		xy_assert(0);//lptimer_timing_period ms唤醒周期，若2次进中断时间大于中断周期的1.5倍，判定为lptimer丢中断；
	}

	//记录最近一次触发中断的时间，用于中断丢失的判断
	lptimer_last_time_tick = Get_Tick();
}

/************************************************************************************
* @brief  timer2中断检查，中断触发时间大于定时周期timer2_timing_period的1.5倍进断言
************************************************************************************/
void check_timer_int(void)
{
	static uint32_t timer2_last_time_tick = 0;

	//此处判断timer2是否会出现漏中断的情况，如2次运行到该处的时间差和设置的周期有明显的差异，认为丢失中断，这里取timer2周期的1.5倍作为判断条件；
	//第一次触发timer2中断时，timer2_last_time_tick未记录到上一次的时间点，故跳过第一次；
	if(timer2_last_time_tick != 0 && Check_Ms_Timeout(timer2_last_time_tick, (1.5 * timer2_timing_period)))
	{
		xy_printf("timer2 lose int\r\n");
		xy_assert(0);//timer2_timing_period ms唤醒周期，若2次进中断时间大于中断周期的1.5倍，判定为timer2丢中断；
	}

	//记录最近一次触发中断的时间，用于中断丢失的判断
	timer2_last_time_tick = Get_Tick();
}

uint8_t xytimer_count = 0;
uint8_t lptimer_count = 0;
uint8_t timer2_count = 0;
__RAM_FUNC void XY_TIMER_Callback(void)
{
    set_event(EVENT_XY_TIMER);	
}

__RAM_FUNC void LPTIMER1_Callback(void)
{
    set_event(EVENT_LPTIMER);	
}

__RAM_FUNC void TIMER2_Callback(void)
{
    set_event(EVENT_TIMER2);
}

void GPIO_Output_Init(uint8_t GPIO_PAD_NUM)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin = GPIO_PAD_NUM;
	gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
	gpio_init.Pull = GPIO_PULL_DOWN;
    HAL_GPIO_Init(&gpio_init);
}

/************************************************************************************
* @brief  xy_timer工作函数
* @param  NA
* @return  NA
************************************************************************************/
void XY_TIMER_Work()
{
	static int set_xy_timer = 0;

	if(set_xy_timer == 0)
	{
		set_xy_timer = 1;
		Timer_AddEvent(XY_TIMER_TEST, xy_timer_timing_period, XY_TIMER_Callback, 1);
		if(1 == g_timer_mode)
		{
			//配置GPIO5为输出模式
			GPIO_Output_Init(GPIO_PAD_NUM_5);
			HAL_GPIO_WritePin(GPIO_PAD_NUM_5,GPIO_PIN_SET);
		}
		UTC_WDT_Init(10+(xy_timer_timing_period/1000));
	}
	else if(is_event_set(EVENT_XY_TIMER))
	{
		clear_event(EVENT_XY_TIMER);
		UTC_WDT_Refresh(10+(xy_timer_timing_period/1000));
		Send_AT_to_Ext("xytimer arrived\r\n"); 

		switch (g_timer_mode)
		{
			case 0:
				check_xy_timer_int();
				break;
			case 1:
				HAL_GPIO_TogglePin(GPIO_PAD_NUM_5);
				// xytimer_count++;
				// if(xytimer_count == 200)
				// {
				// 	McuLptimerDis();
				// 	xytimer_count = 0;
				// }
				break;
			default:break;
		}
	}
}

/************************************************************************************
* @brief  Lptimer工作函数
* @param  NA
* @return  NA
************************************************************************************/
void Lptimer_Work()
{
	static int set_Lptimer = 0;

	if(set_Lptimer == 0)
	{
		set_Lptimer = 1;
		McuLptimerSetPeriod(lptimer_timing_period);
		McuLptimerIrqReg(LPTIMER1_Callback);
		McuLptimerEn();
		if(1 == g_timer_mode)
		{
			GPIO_Output_Init(GPIO_PAD_NUM_6);
			HAL_GPIO_WritePin(GPIO_PAD_NUM_6,GPIO_PIN_SET);
		}
		AP_WDT_Init(AP_WDT_WORKMODE_INT,10+(lptimer_timing_period/1000));
	}
	else if(is_event_set(EVENT_LPTIMER))
	{
		clear_event(EVENT_LPTIMER);
		AP_WDT_Refresh(10+(lptimer_timing_period/1000));
		Send_AT_to_Ext("LPtimer arrived\r\n"); 

		switch (g_timer_mode)
		{
			case 0:
				check_lptimer_int();
				break;
			case 1:
				HAL_GPIO_TogglePin(GPIO_PAD_NUM_6);
				// lptimer_count++;
				// if(lptimer_count == 200)
				// {
				// 	McuLptimerDis();
				// 	lptimer_count = 0;
				// }
				break;
			default:break;
		}
		
	}
}
/************************************************************************************
* @brief  timer2工作函数
* @param  NA
* @return  NA
************************************************************************************/
void TIMER2_Work()
{	
	static int set_timer2 = 0;
	
	if(set_timer2 == 0)
	{
		set_timer2 = 1;
		McuTimerSetPeriod(2, timer2_timing_period);
        McuTimerIrqReg(2, TIMER2_Callback);
		McuTimerEn(2);
		if(1 == g_timer_mode)
		{
			GPIO_Output_Init(GPIO_PAD_NUM_9);
			HAL_GPIO_WritePin(GPIO_PAD_NUM_9,GPIO_PIN_SET);
		}
	}
	else if(is_event_set(EVENT_TIMER2))
	{
		clear_event(EVENT_TIMER2);
		Send_AT_to_Ext("Timer2 arrived\r\n"); 

		switch (g_timer_mode)
		{
			case 0:
				check_timer_int();
				break;
			case 1:
				HAL_GPIO_TogglePin(GPIO_PAD_NUM_9);
				// timer2_count++;
				// if(timer2_count == 200)
				// {
				// 	timer2_count = 0;
				// 	McuTimerDis(2);
				// }
				break;
			default:break;
		}
		
	}
}

