/**
* @file			timer_period_demo.c
* @brief		Timer定时器Demo, 工作在continuous模式，此模式主要用于连续定时.深睡后无效
*				在此Demo中，先对Timer continuous模式初始化后，设置1s触发一次中断，每10次中断打印一次提示信息.
	
***********************************************************************************/
#include "hal_timer.h"
#include "hal_gpio.h"
#include "at_uart.h"
#include "xy_printf.h"

volatile uint8_t hal_continues_timer_counter = 0;
volatile uint8_t g_hal_continues_flag = 0;

/**
 * @brief TIMER中断回调函数
 * @note 可以在定时器中断函数中做时间敏感业务相关处理
 * @warning 用户注意Timer中断函数与Timer的匹配,HAL_TIM2与HAL_TIM2_Callback匹配，依此类推.
 */
__RAM_FUNC void HAL_TIM2_Callback(void)
{
	hal_continues_timer_counter++;
	if(hal_continues_timer_counter == 10)
	{
		hal_continues_timer_counter = 0;
		g_hal_continues_flag = 1;
	}
}

/**
 * @brief 	TIMER continuous模式初始化函数
 *  		这个函数描述了timer初始化为continuous模式时需要的相关步骤.
 * 			在初始化函数内部需要设置定时器的编号、工作模式、重载值、时钟分频等，然后打开定时器.
 */

void Timer_Continuous_Init(void)
{
	HAL_TIM_HandleTypeDef TimContinuousHandle = {0};
	//用户仅限使用HAL_TIM1和HAL_TIM2
	TimContinuousHandle.Instance = HAL_TIM2;
	//连续中断模式，此模式下每达到重置值就会产生中断
	TimContinuousHandle.Init.Mode = HAL_TIM_MODE_CONTINUOUS;

	Debug_Runtime_Add("HAL_TIM_SetTimeout  start");
	HAL_TIM_SetTimeout(&TimContinuousHandle, 1000);

	Debug_Runtime_Add("HAL_TIM_Init  start");
	HAL_TIM_Init(&TimContinuousHandle);

	Debug_Runtime_Add("HAL_TIM_Start  start");
	HAL_TIM_Start(&TimContinuousHandle);
	Debug_Runtime_Add("HAL_TIM_Start  stop");
}

/**
* @brief	demo主函数，在此Demo中，先Timer continuous模式初始化后，然后到达设置的触发时间会产生中断，在while循环过程中每10次中断打印一次提示信息.
*
* @note		用户使用xy_printf需要将XY_DEBUG宏置为1,详情参考xy_printf.c. 
*/

__RAM_FUNC int main(void)
{
	SystemInit();

	xy_printf("Demo Start\n");
	Timer_Continuous_Init();

	while(1)
	{
		if(g_hal_continues_flag)
		{
			g_hal_continues_flag = 0;
			xy_printf("\r\n Continuous_timer_demo\r\n");			
		}
	}

	return 0;
}
