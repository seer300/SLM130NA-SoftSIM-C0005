/**
* @file			timer_counter_demo.c
* @brief		定时器counter模式Demo，此模式主要用于对外部输入信号的上升沿（Low）或下降沿（High）计数.
*				在此Demo中，先对Timer Counter模式初始化后，然后对外部输入信号的上升沿开始计数，到达Reload值时会产生中断
				并且清零Timer count值，继续计数，while循环中将打印每隔1s延时后的中断次数与计数值.
	
***********************************************************************************/
#include "hal_gpio.h"
#include "hal_timer.h"
#include "at_uart.h"
#include "xy_printf.h"

HAL_TIM_HandleTypeDef TimCounterHandle = {0};

uint32_t counter_value = 0;
volatile uint16_t interrupt_times = 0;

/**
 * @brief TIMER中断处理函数v
 * @note 可以在定时器中断函数中做时间敏感业务相关处理
 * @warning 用户注意Timer中断函数与Timer的匹配,HAL_TIM2与HAL_TIM2_Callback匹配，依此类推.
 */
__RAM_FUNC void HAL_TIM2_Callback(void)
{
	interrupt_times++;
}

/**
 * @brief 	TIMER counter模式初始化函数
 *  		这个函数描述了timer初始化为counter模式时需要的相关步骤.
 * 			在初始化函数内部需要设置counter模式的输入引脚与GPIO引脚的对应关系、复用方式、上下拉状态， 
 * 			以及定时器的编号、工作模式、重载值、时钟极性等，然后打开定时器.
 */
void Timer_Counter_Init(void)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};
	//待计数的输入信号端
	gpio_init.Pin = GPIO_PAD_NUM_6;
	gpio_init.Mode = GPIO_MODE_SW_PER_INPUT;
	gpio_init.PinRemap = GPIO_TMR2Gate;
	HAL_GPIO_Init(&gpio_init);
	//用户仅限使用HAL_TIM1和HAL_TIM2
	TimCounterHandle.Instance = HAL_TIM2;		
	//计数模式，外部输入信号有效边沿进行计数
	TimCounterHandle.Init.Mode = HAL_TIM_MODE_COUNTER;
	//对上升沿计数
	TimCounterHandle.Init.Polarity = HAL_TIM_Polarity_Low;
	//预定的计数值， 当对外部输入信号的上升沿开始计数达到Reload值后，产生中断
	TimCounterHandle.Init.Reload = 100;

	HAL_TIM_Init(&TimCounterHandle);
	HAL_TIM_Start(&TimCounterHandle);
}

/*
* @brief	demo主函数，在此Demo中，先对Timer Counter模式初始化后，然后对外部输入信号的上升沿开始计数，
*			到达Reload值时会产生中断，然后while循环中打印中断次数.
* @note		用户使用xy_printf需要将XY_DEBUG宏置为1,详情参考xy_printf.c. 
*/
__RAM_FUNC int main(void)
{
	SystemInit();

	xy_printf("Demo Start\n");
	Timer_Counter_Init();


	while (1)
	{
       	counter_value = HAL_TIM_GetCount(&TimCounterHandle);

		xy_printf("\r\nCounter_timer_demo Counter Value: %d\r\n"
				"Interrupt Times: %d\r\n",
				counter_value, interrupt_times);

		HAL_Delay(1000);
	}
	return 0;
}
