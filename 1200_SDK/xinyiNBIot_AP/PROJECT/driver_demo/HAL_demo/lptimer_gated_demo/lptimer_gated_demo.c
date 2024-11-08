/**
 * @file 	lptimer_gated_demo.c
 * @brief 	Lptimer定时器Demo, 工作在gated模式,此模式通常用于计算单脉冲宽度.
 * 		  	在此Demo中，先LpTimer gated模式初始化，然后外部输入信号为高电平时计数，当遇到下降沿时产生中断，
 * 		  	中断后则stop LpTimer并打印计数值.
 * 
 * @note 	gated模式需要配置输入引脚，并且确定极性，如果Polarity配置成Low，则gated模式定时器高电平使能，下降沿产生中断.
			以Polarity为Low举例，示意图如下：
					___________________                   
				   |                   |                  
				 __|                   |______________________________________________
				|  |-----count计数-----|---count停止计数---------------------
 
 * @attention 	1、LpTimer默认使用utc时钟，使用xtal 32k校准后的值为32778,使用rc32k的值为32000.
				2、xy1200的低功耗定时器为16位计数范围，对于输入信号其捕获值要保证在支持的范围内，否则会造成溢出，导致测量结果不准确.
***********************************************************************************/
#include "hal_gpio.h"
#include "hal_lptimer.h"
#include "at_uart.h"
#include "xy_printf.h"

HAL_LPTIM_HandleTypeDef Lptim1GatedHandle = {0};

volatile uint16_t counter_value =0 ;
volatile uint8_t g_lptimer_gated_flag = 0;
/**
 * @brief 	LpTimer中断回调函数
 * @note  	可以在定时器中断回调函数中做时间敏感业务相关处理
 * @warning 用户注意LpTimer中断回调函数与LpTimer的匹配.HAL_LPTIM1与HAL_LPTIM1_Callback匹配，依此类推.
 */

__RAM_FUNC void HAL_LPTIM1_Callback(void)
{
	counter_value = HAL_LPTIM_GetCount(&Lptim1GatedHandle);
	g_lptimer_gated_flag = 1;
}
/**
 * @brief 	TIMER gated模式初始化函数
 *    		这个函数描述了LpTimer初始化为gated模式时需要的相关步骤。
 * 			在初始化函数内部需要设置gated模式的输入引脚与GPIO引脚的对应关系、复用方式、上下拉状态， 
 * 			以及定时器的编号、工作模式、时钟分频、时钟极性等,然后打开定时器。
 *
*/
void Lptimer_Gated_Init(void)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};
	gpio_init.Pin = GPIO_PAD_NUM_6;
	gpio_init.PinRemap = GPIO_LPTMR1Gate;
	gpio_init.Mode = GPIO_MODE_HW_PER;
	HAL_GPIO_Init(&gpio_init);

	Lptim1GatedHandle.Instance = HAL_LPTIM1;
	Lptim1GatedHandle.Init.Mode = HAL_LPTIM_MODE_GATED;
	Lptim1GatedHandle.Init.Division = HAL_LPTIM_CLK_DIV_128;
	//Polarity_Low表示在输入为高电平时Count寄存器计数,下降沿触发中断
	Lptim1GatedHandle.Init.Polarity = HAL_LPTIM_Polarity_Low;
	HAL_LPTIM_Init(&Lptim1GatedHandle);
    
	HAL_LPTIM_Start(&Lptim1GatedHandle);
}
/**
 * @brief 	demo主函数,在此Demo中，先LpTimer gated模式初始化，然后外部输入信号为高电平时计数，当遇到下降沿时产生中断，
 * 		  	中断后则stop LpTimer并打印计数值. *
 * @note	用户使用xy_printf需要将XY_DEBUG宏置为1,详情参考xy_printf.c.  
 */
__RAM_FUNC int main(void)
{

	SystemInit();

	xy_printf("Demo Start\n");
	Lptimer_Gated_Init();

	while(1)
	{	
		if(g_lptimer_gated_flag)
		{
			g_lptimer_gated_flag = 0;				
			xy_printf("\r\nThe counter_value in lptimer_gated_mode is : %d\r\n", counter_value);
		}	
	}

	return 0;
}
