/**
 * @file 	lptimer_pwm_single_demo.c
 * @brief 	定时器单PWM模式Demo,此模式主要用于产生单路PWM波.
 * 			在此Demo中， 对LpTimer pwm single mode模式功能初始化后，开启LpTimer,然后LpTimer关联的输出引脚可输出一路PWM波形.
 * @note	1、LpTimer count达到PWMCmp翻转输出电平，达到Reload后产生中断并再次翻转输出电平，LpTimer count清零并自动继续计数
			以LpTimer reload值为15000、PWMCmp为5000、Polarity为Low举例，正极性输出(GPIO_LPTMR1PWM_OUTP)，single PWM波形示意如下：
						   __________________           __________________
						  |                  |         |                  |
				 _________|                  |_________|                  |________
				|---5000--|-------10000------|---5000--|------10000-------|--------
				
			2、可通过配置Polarity来选择此模式下初始时电平状态为高电平（High）或低电平（Low）.
			3、注意Reload值影响PWM波的周期， PWMCmp值与Polarity则影响占空比（占空比：指高电平在一个周期之内所占的时间比率）.
			4、负极性输出(GPIO_LPTMR1PWM_OUTN)时的示意图，高低电平时长与正极性输出时相反.
 * @attention 1、LpTimer默认使用utc时钟，使用xtal 32k校准后的值为32778,使用rc32k的值为32000.
			  2、XY1200 B系列芯片，支持Lptimer1在深睡情况下输出PWM，但仅限于GPIO7(正极性输出)与GPIO2(负极性输出).
***********************************************************************************/
#include "hal_gpio.h"
#include "hal_lptimer.h"
#include "at_uart.h"
#include "xy_printf.h"

/**
 * @brief 	对LpTimer 单PWM模式初始化函数
 * 			这个函数描述了LpTimer初始化为双PWM模式需要的相关步骤。
 * 			在初始化函数内部需要设置单路PWM的输出引脚与GPIO引脚的对应关系、引脚复用方式， 
 * 			以及定时器的编号、工作模式、重载值、时钟分频、PWM比较值、时钟极性等,
 * 			然后打开定时器。
 */
void Lptimer_SinglePWM_Init(void)
{


	HAL_LPTIM_HandleTypeDef Lptim1PWMSingleHandle = {0};
	Lptim1PWMSingleHandle.Instance = HAL_LPTIM1;
	Lptim1PWMSingleHandle.Init.Mode = HAL_LPTIM_MODE_PWM_SINGLE;
	Lptim1PWMSingleHandle.Init.Division = HAL_LPTIM_CLK_DIV_1;
	//重载值，具体含义请参看本文件的@file相关注释
	Lptim1PWMSingleHandle.Init.Reload = 15000;
	//PWM翻转比较值，具体含义请参看本文件的@file相关注释
	Lptim1PWMSingleHandle.Init.PWMCmp = 5000;
	//Polarity_Low表示定时器关闭时输出为低电平，开启定时器Count寄存器计数值达到PWM比较值后翻转为高电平
	Lptim1PWMSingleHandle.Init.Polarity = HAL_LPTIM_Polarity_Low;
	//仅用于定时器单PWM模式下，选择正极性输出PWM
	Lptim1PWMSingleHandle.Init.SingPWMOUTPolinDSleep = GPIO_LPTMR1PWM_OUTP;
	HAL_LPTIM_Init(&Lptim1PWMSingleHandle);
    
	HAL_LPTIM_Start(&Lptim1PWMSingleHandle);
}

/**
* @brief	demo主函数,在此Demo中,对LpTimer pwm single mode模式功能初始化后，开启LpTimer,然后LpTimer关联的输出引脚可输出一路PWM波形.
*/

__RAM_FUNC int main(void)
{
	SystemInit();

	Lptimer_SinglePWM_Init();

	while (1)
	{
		;
	}

	return 0;
}
