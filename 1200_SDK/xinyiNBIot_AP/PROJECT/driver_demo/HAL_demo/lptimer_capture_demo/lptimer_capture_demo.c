/**
 * @file 		lptimer_capture_demo.c
 * @brief 		低功耗定时器capture模式Demo,主要用于捕捉输入信号的有效边沿.
 * @note  		在此Demo中先对Timer捕获功能初始化后，然后等待捕捉输入信号的下降沿，连续中断两次后，
 * 		  		停止计数，计算两次下降沿的时间间隔，并打印出来.
 * 
 * @attention	1、LpTimer默认使用utc时钟，使用xtal 32k校准后的值为32778,使用rc32k的值为32000.
 * 				2、xy1200的低功耗定时器为16位计数范围，对于输入信号其捕获值要保证在支持的范围内，否则会造成溢出，导致测量结果不准确.
***********************************************************************************/
#include "hal_gpio.h"
#include "hal_lptimer.h"
#include "at_uart.h"
#include "xy_printf.h"

HAL_LPTIM_HandleTypeDef Lptim1CaptureHandle = {0};

uint32_t capture_time = 0;
volatile uint16_t capture_value[2] = {0};
volatile uint16_t interrupt_times = 0;
volatile uint16_t g_lptimer_capture_flag = 0;

/**
 * @brief 	LpTimer中断回调函数
 * @note  	可以在定时器中断回调函数中做时间敏感业务相关处理
 * @warning 用户注意LpTimer中断回调函数与LpTimer的匹配.HAL_LPTIM1与HAL_LPTIM1_Callback匹配，依此类推.
 */
__RAM_FUNC void HAL_LPTIM1_Callback(void)
{
	capture_value[interrupt_times] = HAL_LPTIM_GetCaptureCount(&Lptim1CaptureHandle);		
	interrupt_times++;

	if(interrupt_times == 2)
	{
		g_lptimer_capture_flag = 1;
		interrupt_times = 0;		
	}
}
/**
 * @brief	LpTimer capture模式初始化函数
 *    		这个函数描述了LpTimer初始化为capture模式时需要的相关步骤。
 * 			在初始化函数内部需要设置capture模式的输入引脚与GPIO引脚的对应关系、复用方式， 
 * 			以及定时器的编号、工作模式、时钟分频、时钟极性等,然后打开定时器。
 *
 */
void Lptimer_Capture_Init(void)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};
	gpio_init.Pin = GPIO_PAD_NUM_6;
	gpio_init.PinRemap = GPIO_LPTMR1Gate;
	gpio_init.Mode = GPIO_MODE_HW_PER;
	HAL_GPIO_Init(&gpio_init);
    
	Lptim1CaptureHandle.Instance = HAL_LPTIM1;
	Lptim1CaptureHandle.Init.Division = HAL_LPTIM_CLK_DIV_2;
	Lptim1CaptureHandle.Init.Mode = HAL_LPTIM_MODE_CAPTURE;
	Lptim1CaptureHandle.Init.Polarity = HAL_LPTIM_Polarity_High;
	HAL_LPTIM_Init(&Lptim1CaptureHandle);

	HAL_LPTIM_Start(&Lptim1CaptureHandle);
}

/**
* @brief	demo主函数，在此Demo中先对LpTimer捕获功能初始化后，然后在while循环过程中根据是否捕获到下降沿，计算两次下降沿的时间间隔，并打印出来
* @note		用户使用xy_printf需要将XY_DEBUG宏置为1,详情参考xy_printf.c. 
*/

__RAM_FUNC int main(void)
{
	SystemInit();

	xy_printf("Demo Start\n");

	Lptimer_Capture_Init();

	while (1)
	{
		if(g_lptimer_capture_flag)
		{
			HAL_LPTIM_Stop(&Lptim1CaptureHandle);

			g_lptimer_capture_flag = 0;

			//获取lptimer在输入捕获模式下两次有效边沿间的时长
			if (HAL_LPTIM_GetTime(&Lptim1CaptureHandle, capture_value[0], capture_value[1], &capture_time) == HAL_OK)
			{
				xy_printf("\r\nThe period of the input signal is: %d\r\n", capture_time);	
			}
		}
	}

	return 0;
}
