/**
* @file			timer_pwm_single_demo.c
* @brief		定时器单PWM模式Demo,此模式主要用于产生单路PWM波.
				在此Demo中， 对Timer pwm single mode模式功能初始化后，开启Timer,然后Timer关联的输出引脚可输出一路PWM波形.

* @note			1、Timer count达到PWMCmp翻转输出电平，达到Reload后产生中断并再次翻转输出电平，Timer count清零并自动继续计数
				以timer reload值为15000、PWMCmp为5000、Polarity为Low举例，single PWM波形示意如下：
						   __________________           __________________
						  |                  |         |                  |
				 _________|                  |_________|                  |________
				|---5000--|-------10000------|---5000--|------10000-------|--------
				
				2、可通过配置Polarity来选择此模式下初始时电平状态为高电平（High）或低电平（Low）.
				3、注意Reload值影响PWM波的周期， PWMCmp值与Polarity则影响占空比（占空比：指高电平在一个周期之内所占的时间比率）.
	
***********************************************************************************/
#include "hal_gpio.h"
#include "hal_timer.h"
#include "xy_printf.h"

/**
 * @brief 	TIMER 单PWM模式初始化函数
 * 			这个函数描述了timer初始化为双PWM模式需要的相关步骤.
 * 			在初始化函数内部需要设置单路PWM的输出引脚与GPIO引脚的对应关系、引脚复用方式， 
 * 			以及定时器的编号、工作模式、重载值、时钟分频、PWM比较值、时钟极性等，
 * 			然后打开定时器.
 */
void Timer_SinglePWM_Init(void)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};
	gpio_init.Pin = GPIO_PAD_NUM_6;
	gpio_init.Mode = GPIO_MODE_HW_PER;
	gpio_init.PinRemap = GPIO_TMR2PWM_OUTP;
	HAL_GPIO_Init(&gpio_init);

	HAL_TIM_HandleTypeDef TimPWMSingleHandle = {0};
	//用户仅限使用HAL_TIM1和HAL_TIM2
	TimPWMSingleHandle.Instance = HAL_TIM2;
	//单输出脉冲宽度（PWM）调制模式
	TimPWMSingleHandle.Init.Mode = HAL_TIM_MODE_PWM_SINGLE;
	//设置时钟源分频
	TimPWMSingleHandle.Init.ClockDivision = HAL_TIM_CLK_DIV_128;
	//重载值，具体含义请参看本文件的@file相关注释
	TimPWMSingleHandle.Init.Reload = 15000;	
	//PWM翻转比较值，具体含义请参看本文件的@file相关注释
	TimPWMSingleHandle.Init.PWMCmp = 5000;
	//Polarity_Low表示定时器关闭时输出为低电平，开启定时器Count寄存器计数值达到PWM比较值后翻转为高电平
	TimPWMSingleHandle.Init.Polarity = HAL_TIM_Polarity_Low;

	HAL_TIM_Init(&TimPWMSingleHandle);
	HAL_TIM_Start(&TimPWMSingleHandle);
}
/**
 * @brief	demo主函数，在此Demo中， 对Timer pwm single mode模式功能初始化后，开启Timer,然后Timer关联的输出引脚可输出一路PWM波形.
 */
__RAM_FUNC int main(void)
{
	SystemInit();

	Timer_SinglePWM_Init();

	while (1)
	{
	}
	return 0;
}
