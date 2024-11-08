/**
* @file			timer_capture_demo.c
* @brief		定时器capture模式Demo,主要用于捕捉输入信号的有效边沿.
* @note			在此Demo中先对Timer捕获功能初始化后，然后等待捕捉输入信号的下降沿，连续中断两次后，
				停止计数，计算两次下降沿的时间间隔，并打印出来.
				
* @attention	xy1200的硬件定时器为32位计数范围，对于输入信号其捕获值要保证在支持的范围内，否则会造成溢出，导致测量结果不准确.
*********************************************************************************************************************/
#include "hal_timer.h"
#include "hal_gpio.h"
#include "at_uart.h"
#include "xy_printf.h"

HAL_TIM_HandleTypeDef TimCaptureHandle = {0};

uint64_t capture_time = 0;
volatile uint32_t capture_value[2] = {0};
volatile uint8_t interrupt_times = 0;
volatile uint8_t g_timer_capture_flag = 0;

/**
 * @brief   TIMER中断回调函数，用户在使用时可自行添加中断处理函数.
 * @note 	可以在定时器中断回调函数中做时间敏感业务相关处理, 每当捕获到有效边沿就会进入这个中断.
 * @warning 用户注意Timer中断函数与Timer的匹配.HAL_TIM2与HAL_TIM2_Callback匹配，依此类推.
 */
__RAM_FUNC void HAL_TIM2_Callback()
{
	capture_value[interrupt_times]= HAL_TIM_GetCaptureCount(&TimCaptureHandle);
	interrupt_times ++;
		
	if(interrupt_times == 2)
	{
		g_timer_capture_flag = 1;
		interrupt_times = 0;		
	}
}

/**
 * @brief	TIMER capture模式初始化函数
 *    		这个函数描述了timer初始化为capture模式时需要的相关步骤.
 * 			在初始化函数内部需要设置capture模式的输入引脚与GPIO引脚的对应关系、复用方式，
 * 			以及定时器的编号、工作模式、时钟分频、时钟极性等，然后打开定时器.
 */
void Timer_Capture_Init(void)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};
	gpio_init.Pin = GPIO_PAD_NUM_6;
	gpio_init.Mode = GPIO_MODE_SW_PER_INPUT;
	gpio_init.PinRemap = GPIO_TMR2Gate;
	HAL_GPIO_Init(&gpio_init);
	//用户仅限使用HAL_TIM1和HAL_TIM2
	TimCaptureHandle.Instance = HAL_TIM2;
	//输入捕获模式，此模式下会产生捕获(capture)中断
	TimCaptureHandle.Init.Mode = HAL_TIM_MODE_CAPTURE;
	//捕捉下降沿，捕捉到下降沿会产生中断
	TimCaptureHandle.Init.Polarity = HAL_TIM_Polarity_High;
	//设置timer时钟源分频
	TimCaptureHandle.Init.ClockDivision = HAL_TIM_CLK_DIV_128;  //注意此处实际值为7<<12,并非128

	HAL_TIM_Init(&TimCaptureHandle);	
	HAL_TIM_Start(&TimCaptureHandle);
}

/**
* @brief	demo主函数，在此Demo中先对Timer捕获功能初始化后，然后在while循环过程中根据是否捕获到下降沿，计算两次下降沿的时间间隔，并打印出来
* @note		用户使用xy_printf需要将XY_DEBUG宏置为1,详情参考xy_printf.c. 
*/

__RAM_FUNC int main(void)
{
	SystemInit();

	xy_printf("Demo Start\n");

	Timer_Capture_Init();

	while(1)
	{
		if(g_timer_capture_flag)
		{
			HAL_TIM_Stop(&TimCaptureHandle);

			g_timer_capture_flag = 0;


			//获取timer在输入捕获模式下两次有效边沿间的时长
			if (HAL_TIM_GetTime(&TimCaptureHandle, capture_value[0], capture_value[1], &capture_time) == HAL_OK)
			{
				xy_printf("\r\nThe period of the input signal is: %d\r\n", capture_time);	
			}
		}
	}

	return 0;
}
