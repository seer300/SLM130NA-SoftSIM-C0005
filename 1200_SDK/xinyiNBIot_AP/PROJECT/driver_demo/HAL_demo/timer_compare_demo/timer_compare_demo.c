/**
* @file			timer_compare_demo.c
* @brief		定时器compare模式,主要用于单次定时.
* @note			在此Demo中，先对Timer compare功能初始化后，然后到达Reload值时会产生中断，
*				中断后打印中断提示信息，并清零Timer count值.

* @attention 	compare模式在定时器到reload值产生中断以后不会自动重载，会继续计时，
				直到0xFFFFFFFF溢出重置为0，溢出不产生中断.
***********************************************************************************/
#include "hal_gpio.h"
#include "hal_timer.h"
#include "at_uart.h"
#include "xy_printf.h"

HAL_TIM_HandleTypeDef TimCompareHandle = {0};

volatile uint8_t g_timer_compare_flag = 0;
/**
 * @brief TIMER中断回调函数
 * @note 可以在定时器中断函数中做时间敏感业务相关处理
 * @warning 用户注意Timer中断函数与Timer的匹配,HAL_TIM2与HAL_TIM2_Callback匹配，依此类推.
 */
__RAM_FUNC void HAL_TIM2_Callback(void)
{
	g_timer_compare_flag = 1;	
}

/**
 * @brief	TIMER compare模式初始化函数
 *  		这个函数描述了timer初始化为compare模式时需要的相关步骤.
 * 			在初始化函数内部需要设置定时器的编号、工作模式、时钟分频等，然后打开定时器.
 */
void Timer_Compare_Init(void)
{
	//用户仅限使用HAL_TIM1和HAL_TIM2
	TimCompareHandle.Instance = HAL_TIM2;
	//比较模式，此模式下会产生比较(compare)中断
	TimCompareHandle.Init.Mode = HAL_TIM_MODE_COMPARE;
	//设置timer时钟源分频
	TimCompareHandle.Init.ClockDivision = HAL_TIM_CLK_DIV_128;	
	//设置重载值，重载值=总计数时间/单位时间,其中：
	//单位时间为timer每计数一次的时间，用dt表示，dt=1/(f/m)，其中f为timer的时钟源频率，m为分频
	//总时间用t (单位：s)表示，则重载值计算公式为Reload = t /dt = t*f/m ;
	//重载值最大不可超过2的32次方减1
	TimCompareHandle.Init.Reload = 100000;  
	
	HAL_TIM_Init(&TimCompareHandle);
	HAL_TIM_Start(&TimCompareHandle);
}

/**
*@brief	    demo主函数，在此Demo中，先对Timer compare功能初始化后，然后到达Reload值时会产生中断，
*			中断后打印中断提示信息，并清零Timer count值.
*@note		用户使用xy_printf需要将XY_DEBUG宏置为1,详情参考xy_printf.c. 	
*/
__RAM_FUNC int main(void)
{
	SystemInit();

	xy_printf("Demo Start\n");	
	Timer_Compare_Init();

	while(1)
	{
		if(g_timer_compare_flag)
		{
			g_timer_compare_flag = 0;
			xy_printf("\r\n Compare_timer_demo\r\n");
			HAL_TIM_SetCount(&TimCompareHandle, 0);  		
		}
	}

	return 0;
}