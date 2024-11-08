/**
 * @file 		lptimer_counter_demo.c
 * @brief 		LpTimer定时器Demo, 工作在counter模式.
 * 		  		在此Demo中，先对LpTimer Counter模式初始化后，然后对外部输入信号的上升沿开始计数，到达Reload值时会产生中断,
 * 		  		并且清零LpTimer count值，继续计数，while循环中将打印每隔1s延时后的中断次数与计数值. 
 * 
 * @attention 	LpTimer默认使用utc时钟，使用xtal 32k校准后的值为32778,使用rc32k的值为32000.
***********************************************************************************/
#include "hal_gpio.h"
#include "hal_lptimer.h"
#include "at_uart.h"
#include "xy_printf.h"


HAL_LPTIM_HandleTypeDef Lptim1CounterHandle = {0};

uint16_t counter_value = 0;
volatile uint16_t interrupt_times = 0;

/**
 * @brief 	LpTimer中断回调函数
 * @note  	可以在定时器中断回调函数中做时间敏感业务相关处理
 * @warning 用户注意LpTimer中断回调函数与LpTimer的匹配.HAL_LPTIM1与HAL_LPTIM1_Callback匹配，依此类推.
 */
__RAM_FUNC void HAL_LPTIM1_Callback(void)
{
	interrupt_times++;
}
/**
 * @brief 	LpTimer counter模式初始化函数
 *  		这个函数描述了LpTimer初始化为counter模式时需要的相关步骤。
 * 			在初始化函数内部需要设置counter模式的输入引脚与GPIO引脚的对应关系、复用方式， 
 * 			以及定时器的编号、工作模式、重载值、时钟极性等，然后打开定时器。
 *
 */
void Lptimer_Counter_Init(void)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};
	gpio_init.Pin = GPIO_PAD_NUM_6;
	gpio_init.PinRemap = GPIO_LPTMR1Gate;
	gpio_init.Mode = GPIO_MODE_HW_PER;
	HAL_GPIO_Init(&gpio_init);

	Lptim1CounterHandle.Instance = HAL_LPTIM1;	
	Lptim1CounterHandle.Init.Mode = HAL_LPTIM_MODE_COUNTER;
	//预定的计数值， 当对外部输入信号的上升沿开始计数达到Reload值后，产生中断
	Lptim1CounterHandle.Init.Reload = 10;
	//对下降沿计数
	Lptim1CounterHandle.Init.Polarity = HAL_LPTIM_Polarity_High;
	HAL_LPTIM_Init(&Lptim1CounterHandle);
    
	HAL_LPTIM_Start(&Lptim1CounterHandle);
}

/**
 * @brief 		demo主函数，在此Demo中，先对LpTimer Counter模式初始化后，然后对外部输入信号的上升沿开始计数，到达Reload值时会产生中断,
 * 		  		并且清零LpTimer count值，继续计数，while循环中将打印每隔1s延时后的中断次数与计数值. 
 *  @note		用户使用xy_printf需要将XY_DEBUG宏置为1,详情参考xy_printf.c. 
 */

__RAM_FUNC int main(void)
{
	SystemInit();

	xy_printf("Demo Start\n");
	Lptimer_Counter_Init();

	while(1)
	{
       	counter_value = HAL_TIM_GetCount(&Lptim1CounterHandle);

		xy_printf("\r\nCounter_lptimer_demo Counter Value: %d\r\n"
				"Interrupt Times: %d\r\n",
				counter_value, interrupt_times);

		HAL_Delay(1000);
	}

	return 0;
}