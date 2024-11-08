/**
 * @file 	lptimer_pwm_dual_demo.c
 * @brief 	定时器双PWM模式Demo，此模式可以产生2路互补带死区的PWM信号.
 * 			在此Demo中， 对LpTimer pwm dual mode模式功能初始化后，开启LpTimer,然后LpTimer关联的输出引脚可输出两路互补带死区的PWM波形.
 * 
 * @note	1、双PWM模式下，LpTimer由初始值开始计数，到配置的PWMCmp值翻转一次输出且继续计数，接着到重载值(Reload)后再翻转一次输出，然后
			计数值(LpTimer count)清零，重复以上行为。双输出模式两路输出的初始电平相反，并且可以配置死区.
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

 * @attention 1、LpTimer默认使用utc时钟，使用xtal 32k校准后的值为32778,使用rc32k的值为32000.
			  2、XY1200 B系列芯片，支持Lptimer1在深睡情况下输出PWM，但仅限于GPIO7(正极性输出)与GPIO2(负极性输出).
***********************************************************************************/
#include "hal_gpio.h"
#include "hal_lptimer.h"
#include "at_uart.h"
#include "xy_printf.h"

/**
 * @brief 	LpTimer 双PWM模式初始化函数
 * 			这个函数描述了LpTimer初始化为双PWM模式需要的相关步骤。
 * 			在初始化函数内部需要设置双路PWM的输出引脚与GPIO引脚的对应关系、引脚复用方式， 
 * 			以及定时器的编号、工作模式、重载值、时钟分频、PWM比较值、时钟极性以及死区等
 */
void Lptimer_DualPWM_Init(void)
{
	HAL_LPTIM_HandleTypeDef Lptim1PWMDualHandle = {0};
	Lptim1PWMDualHandle.Instance = HAL_LPTIM1;
	Lptim1PWMDualHandle.Init.Mode = HAL_LPTIM_MODE_PWM_DUAL;
	Lptim1PWMDualHandle.Init.Division = HAL_LPTIM_CLK_DIV_8;
	//重载值，具体含义请参看本文件的 @file相关注释
	Lptim1PWMDualHandle.Init.Reload = 1500;
	//PWM翻转比较值，具体含义请参看本文件的@file相关注释
	Lptim1PWMDualHandle.Init.PWMCmp = 500;
	//Polarity_Low表示定时器关闭时输出为低电平，开启定时器Count寄存器计数值达到PWM比较值后翻转为高电平，另一路PWM波相反
	Lptim1PWMDualHandle.Init.Polarity = HAL_LPTIM_Polarity_Low;
	//死区时间，注意是此处是timer时钟源的周期，不受timer分频系数的影响
	Lptim1PWMDualHandle.Init.DualPWMDeadtime = HAL_LPTIM_DELAY_64_CYCLES;
	HAL_LPTIM_Init(&Lptim1PWMDualHandle);
    
	HAL_LPTIM_Start(&Lptim1PWMDualHandle);
}

/**
 * @brief 	demo主函数,在此Demo中， 对LpTimer pwm dual mode模式功能初始化后，开启LpTimer,然后Timer关联的输出引脚可输出两路互补带死区的PWM波形.
 * 
 */

__RAM_FUNC int main(void)
{
	SystemInit();

	Lptimer_DualPWM_Init();
	//打开中断
	EnablePrimask();

	while(1)
	{
		;
	}
	return 0;
}
