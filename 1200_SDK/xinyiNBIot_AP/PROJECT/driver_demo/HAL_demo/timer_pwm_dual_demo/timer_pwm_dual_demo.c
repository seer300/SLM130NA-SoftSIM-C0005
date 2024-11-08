/**
* @file			timer_pwm_dual_demo.c
* @brief		定时器双PWM模式Demo，此模式可以产生2路互补带死区的PWM信号.
				在此Demo中， 对Timer pwm dual mode模式功能初始化后，开启Timer,然后Timer关联的输出引脚可输出两路互补带死区的PWM波形.

* @note			1、双PWM模式下，Timer由初始值开始计数，到配置的PWMCmp值翻转一次输出且继续计数，接着到重载值(Reload)后再翻转一次输出，然后
				计数值(timer count)清零，重复以上行为。双输出模式两路输出的初始电平相反，并且可以配置死区.
				下面以Reload值为1500、PWMCmp为500、Polarity为Low为例，两路互补PWM示意图如下：(其中D所示区域为死区)
						   __________________            __________________
						  |                  |          |                  |
				 _________|                  |__________|                  |________
				 _______                        ______                        ________
						|D                    D|      |D                    D|
						|_|__________________|_|      |_|__________________|_|

				|-------|---1000+2*deadtime----|------|----1000+2*deadtime---|--------
				500-1*deadtime					500-2*deadtime
				
				2、可通过配置Polarity来选择此模式下初始时电平状态为高电平（High）或低电平（Low）.
				3、注意Reload值影响PWM波的周期， PWMCmp值与Polarity则影响占空比（占空比：指高电平在一个周期之内所占的时间比率）.
				
***********************************************************************************/
#include "hal_gpio.h"
#include "hal_timer.h"
#include "xy_printf.h"
#include "hal_uart.h"

/**
 * @brief 	TIMER 双PWM模式初始化函数
 * 			这个函数描述了timer初始化为双PWM模式需要的相关步骤.
 * 			在初始化函数内部需要设置双路PWM的输出引脚与GPIO引脚的对应关系、引脚复用方式， 
 * 			以及定时器的编号、工作模式、重载值、时钟分频、PWM比较值、时钟极性以及死区时间等.
 *
 */
void Timer_DualPWM_Init(void)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};
	gpio_init.Pin = GPIO_PAD_NUM_6;
	gpio_init.Mode = GPIO_MODE_HW_PER;
	gpio_init.PinRemap = GPIO_TMR2PWM_OUTP;
	HAL_GPIO_Init(&gpio_init);

	gpio_init.Pin = GPIO_PAD_NUM_5;
	gpio_init.Mode = GPIO_MODE_HW_PER;
	gpio_init.PinRemap = GPIO_TMR2PWM_OUTN;
	HAL_GPIO_Init(&gpio_init);

	HAL_TIM_HandleTypeDef TimPWMDualHandle = {0};
	//用户仅限使用HAL_TIM1和HAL_TIM2
	TimPWMDualHandle.Instance = HAL_TIM2;
	//双输出脉冲宽度（PWM）调制模式
	TimPWMDualHandle.Init.Mode = HAL_TIM_MODE_PWM_DUAL;
	//设置时钟源分频
	TimPWMDualHandle.Init.ClockDivision = HAL_TIM_CLK_DIV_1;
	//重载值，具体含义请参看本文件的 @file相关注释
	TimPWMDualHandle.Init.Reload = 1500;	
	//PWM翻转比较值，具体含义请参看本文件的@file相关注释
	TimPWMDualHandle.Init.PWMCmp = 500;
	//Polarity_Low表示定时器关闭时输出为低电平，开启定时器Count寄存器计数值达到PWM比较值后翻转为高电平，另一路PWM波相反
	TimPWMDualHandle.Init.Polarity = HAL_TIM_Polarity_Low;
	//死区时间，注意是此处是timer时钟源的周期，不受timer分频系数的影响
	TimPWMDualHandle.Init.DualPWMDeadtime = HAL_TIM_DEADTIME_128_CYCLES;

	HAL_TIM_Init(&TimPWMDualHandle);
	HAL_TIM_Start(&TimPWMDualHandle);
}
/**
  * @brief   demo主函数,在此Demo中， 对Timer pwm dual mode模式功能初始化后，开启Timer,然后Timer关联的输出引脚可输出两路互补带死区的PWM波形.
  */
__RAM_FUNC int main(void)
{
	SystemInit();

	Timer_DualPWM_Init();

	while (1)
	{
	}
	return 0;
}
