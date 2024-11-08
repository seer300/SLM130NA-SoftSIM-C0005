/**
* @file			xy_timer_rtc_demo.c
* @brief		定时器设置，支持深睡后仍然有效和深睡后无效的定时器2种定时器设置；支持设置指定间隔时间的周期唤醒和每天一段时间区间内随机唤醒的单次触发；
* @note			在此Demo中设置了每隔60s的周期性唤醒（深睡后有效）；设置了每隔60s的单次唤醒（深睡后无效）；设置了每天早上8点到12点间随机的单次唤醒（深睡后有效），需在其回调中重新设置达到周期性的目的；
				验证深睡唤醒流程，设置的下列事件是否可以如预期	

***********************************************************************************/
#include "hal_gpio.h"
#include "hal_timer.h"
#include "at_uart.h"
#include "xy_printf.h"
#include "xy_timer.h"



__RAM_FUNC void LpTimer_Test_Timeout(void)
{
	xy_printf("\r\nPeriod LPTIMER timeout!\r\n");
}


__RAM_FUNC void NoLpTimer_Test_Timeout(void)
{
	xy_printf("\r\nNo Period NOLPTIMER timeout!\r\n");
}


__RAM_FUNC void DayTimer_Test_Timeout(void)
{
	xy_printf("\r\nPeriod LPTIMER timeout!\r\n");

	/*再次设置第二天的8:00到12:00的随机定时器*/
	Timer_Set_By_Day(TIMER_LP_USER2,DayTimer_Test_Timeout,(8*60*60),(4*60*60));
}


/**
* @brief	demo主函数，在此Demo中，设置芯片深睡后仍然有效和芯片深睡后无效的定时器，详情参考 @ref HAL_TIM_HandleTypeDef.
*
* @note		设置TIMER_LP_USER1定时器为周期性事件，深睡唤醒后能继续保持住；
			设置TIMER_NON_LP_USER1定时器为周期性事件，深睡唤醒后不能继续保持住；
			设置TIMER_LP_USER2定时器为非周期性事件，深睡唤醒后能继续保持住，在其中断回调中需重新设置可以达到周期性的效果；		
*/
__RAM_FUNC int main(void)
{
	SystemInit();

	/*芯片深睡后仍然有效的定时器设置*/
	Timer_AddEvent(TIMER_LP_USER1,60*1000,LpTimer_Test_Timeout,1);
	

	/*芯片深睡后无效的定时器设置*/
	Timer_AddEvent(TIMER_NON_LP_USER1,60*1000,NoLpTimer_Test_Timeout,1);


	/*设置每天的8:00到12:00的随机定时器,Timer_Set_By_Day内部设置成非周期性，在中断回调中需重新设置*/
	Timer_Set_By_Day(TIMER_LP_USER2,DayTimer_Test_Timeout,(8*60*60),(4*60*60));
	

	while(1)
	{
		Enter_LowPower_Mode(LPM_DSLEEP);
		HAL_Delay(1000);
	}

	return 0;
}
