/**
* @file			timer_gated_demo.c
* @brief		定时器gated模式Demo,此模式通常用于计算单脉冲宽度.
				在此Demo中，先Timer gated模式初始化，然后外部输入信号为高电平时计数，当遇到下降沿时产生中断，
				中断后则stop Timer并打印计数值.

* @note			gated模式需要配置输入引脚，并且确定极性，如果Polarity配置成Low，则gated模式定时器高电平使能，下降沿产生中断.
				以Polarity为Low举例，示意图如下：
					___________________                   
				   |                   |                  
				 __|                   |______________________________________________
				|  |-----count计数-----|---count停止计数---------------------

***********************************************************************************/
#include "hal_gpio.h"
#include "hal_timer.h"
#include "at_uart.h"
#include "xy_printf.h"

HAL_TIM_HandleTypeDef TimGatedHandle = {0};

volatile uint32_t counter_value =0 ;
volatile uint8_t g_timer_gated_flag = 0;

/**
 * @brief TIMER中断回调函数
 * @note 可以在定时器中断函数中做时间敏感业务相关处理
 * @warning 用户注意Timer中断函数与Timer的匹配,HAL_TIM2与HAL_TIM2_Callback匹配，依此类推.
 */
__RAM_FUNC void HAL_TIM2_Callback(void)
{
	counter_value = HAL_TIM_GetCount(&TimGatedHandle);
	g_timer_gated_flag = 1;
}

/**
 * @brief 	TIMER gated模式初始化函数
 *    		这个函数描述了timer初始化为gated模式时需要的相关步骤.
 * 			在初始化函数内部需要设置gated模式的输入引脚与GPIO引脚的对应关系、复用方式
 * 			以及定时器的编号、工作模式、时钟分频、时钟极性等，然后打开定时器.
 *
*/
void Timer_Gated_Init(void)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};
	gpio_init.Pin = GPIO_PAD_NUM_6;
	gpio_init.Mode = GPIO_MODE_HW_PER;
	gpio_init.PinRemap = GPIO_TMR2Gate;
	HAL_GPIO_Init(&gpio_init);
	//用户仅限使用HAL_TIM1和HAL_TIM2
	TimGatedHandle.Instance = HAL_TIM2;
	//门模式
	TimGatedHandle.Init.Mode = HAL_TIM_MODE_GATED;
	//设置timer时钟源分频
	TimGatedHandle.Init.ClockDivision = HAL_TIM_CLK_DIV_32;
	//Polarity_Low表示在输入为高电平时Count寄存器计数,下降沿触发中断
	TimGatedHandle.Init.Polarity = HAL_TIM_Polarity_Low;

	HAL_TIM_Init(&TimGatedHandle);
	HAL_TIM_Start(&TimGatedHandle);
}

/**
* @brief	demo主函数，在此Demo中，先Timer gated模式初始化，然后外部输入信号为高电平时计数，
*			当遇到下降沿时产生中断，最后在while循环过程中stop Timer并打印计数值.
*
* @note		用户使用xy_printf需要将XY_DEBUG宏置为1,详情参考xy_printf.c. 
*/
__RAM_FUNC int main(void)
{
	SystemInit();

	xy_printf("Demo Start\n");
	Timer_Gated_Init();

	while(1)
	{
		if(g_timer_gated_flag)
		{
			g_timer_gated_flag = 0;		
			xy_printf("\r\nThe counter_value in timer_gated_mode is : %ld\r\n", counter_value);
			HAL_TIM_SetCount(&TimGatedHandle,0);
		}	
	}

	return 0;
}
