/**
 * @file 		lptimer_compare_demo.c
 * @brief 		LpTimer定时器Demo, 工作在compare模式.
 * @note  		在此Demo中，先对LpTimer compare功能初始化后，然后到达Reload值时会产生中断，
 * 		  		中断后打印中断提示信息，并清零LpTimer count值.
 * 
 * @attention 	1、LpTimer默认使用utc时钟，使用xtal 32k校准后的值为32778,使用rc32k的值为32000.
 * 				2、compare模式在定时器到reload值产生中断以后不会自动重载，会继续计时，
 * 				直到0xFFFF溢出重置为0，溢出不产生中断.

***********************************************************************************/
#include "hal_gpio.h"
#include "hal_lptimer.h"
#include "at_uart.h"
#include "xy_printf.h"

HAL_LPTIM_HandleTypeDef Lptim1CompareHandle = {0};

volatile uint16_t g_lptimer_compare_flag = 0;
/**
 * @brief 	LpTimer中断回调函数
 * @note  	可以在定时器中断回调函数中做时间敏感业务相关处理
 * @warning 用户注意LpTimer中断回调函数与LpTimer的匹配.HAL_LPTIM1与HAL_LPTIM1_Callback匹配，依此类推.
 */
__RAM_FUNC void HAL_LPTIM1_Callback(void)
{
	g_lptimer_compare_flag = 1;
}
/**
 * @brief 	LpTimer counter模式初始化函数
 *  		这个函数描述了timer初始化为compare模式时需要的相关步骤。
 * 			以及定时器的编号、工作模式、重载值等, 然后打开定时器。
 */
void Lptimer_Compare_Init(void)
{
	Lptim1CompareHandle.Instance = HAL_LPTIM1;
	Lptim1CompareHandle.Init.Mode = HAL_LPTIM_MODE_COMPARE;
	Lptim1CompareHandle.Init.Division = HAL_LPTIM_CLK_DIV_2;
	//设置重载值，重载值=总计数时间/单位时间,其中：
	//单位时间为Lptimer每计数一次的时间，用dt表示，dt=1/(f/m)，其中f为Lptimer的时钟源频率，m为分频
	//总时间用t (单位：s)表示，则重载值计算公式为Reload = t /dt = t*f/m ;
	//重载值最大不可超过2的16次方减1
	Lptim1CompareHandle.Init.Reload = 32000;
	HAL_LPTIM_Init(&Lptim1CompareHandle);

	HAL_LPTIM_Start(&Lptim1CompareHandle);
}

/**
 * @brief 	demo主函数,在此Demo中，先对LpTimer compare功能初始化后，然后到达Reload值时会产生中断，
 * 		  	中断后打印中断提示信息，并清零LpTimer count值.
 * @note	用户使用xy_printf需要将XY_DEBUG宏置为1,详情参考xy_printf.c. 		 	
 */
__RAM_FUNC int main(void)
{
	SystemInit();

	xy_printf("Demo Start\n");
	Lptimer_Compare_Init();

	while (1)
	{
		if(g_lptimer_compare_flag)
		{
			g_lptimer_compare_flag=0;
			xy_printf("\r\nCompare_lptimer_demo\r\n");
		}

	}
	return 0;
}