/**
 * @file 		lptimer_period_demo.c
 * @author 		LpTimer外设Demo, 工作在continuous模式。深睡仍然有效
 * @note 		在此Demo中，先对LpTimer周期性定时初始化，当到达设定时长时会产生中断，然后在中断回调内翻转GPIO. 
 * 
 * @attention 	LpTimer默认使用utc时钟，使用xtal 32k校准后的值为32778,使用rc32k的值为32000.
***********************************************************************************/
#include "hal_gpio.h"
#include "hal_lptimer.h"
#include "at_uart.h"
#include "xy_printf.h"
#include "hw_types.h"
#include "xinyi2100.h"

/**
 * @brief 	LpTimer1中断回调函数
 * @note  	可以在定时器中断回调函数中做时间敏感业务相关处理
 * @warning 用户注意LpTimer中断回调函数与LpTimer的匹配.HAL_LPTIM1与HAL_LPTIM1_Callback匹配，依此类推.
 */
__RAM_FUNC void HAL_LPTIM1_Callback(void)
{
	HAL_GPIO_TogglePin(GPIO_PAD_NUM_5);
}

/**
 * @brief 	LpTimer2中断回调函数
 * @note  	可以在定时器中断回调函数中做时间敏感业务相关处理
 * @warning 用户注意LpTimer中断回调函数与LpTimer的匹配.HAL_LPTIM2与HAL_LPTIM2_Callback匹配，依此类推.
 */
__RAM_FUNC void HAL_LPTIM2_Callback(void)
{
	HAL_GPIO_TogglePin(GPIO_PAD_NUM_6);
}

/**
 * @brief 	LPTIMER1周期性定时初始化函数
 *  		这个函数描述了LpTimer初始化为continuous模式时需要的相关步骤。
 * 			在初始化函数内部需要设置定时器的编号、工作模式、重载值、时钟分频等，然后打开定时器。
 */
void Lptimer1_Period_Init(uint32_t ms)
{
	HAL_StatusTypeDef retval = HAL_OK;
    HAL_LPTIM_HandleTypeDef Lptim1ContinuousHandle = {0};
	Lptim1ContinuousHandle.Instance = HAL_LPTIM1;
	Lptim1ContinuousHandle.Init.Mode = HAL_LPTIM_MODE_CONTINUOUS;
	Debug_Runtime_Add("HAL_LPTIM1_SetTimeout  start");
	retval = HAL_LPTIM_SetTimeout(&Lptim1ContinuousHandle, ms);
	if(retval != HAL_OK)
    {
        xy_assert(0);
    }

	Debug_Runtime_Add("HAL_LPTIM1_Init  start");
	HAL_LPTIM_Init(&Lptim1ContinuousHandle);

	Debug_Runtime_Add("HAL_LPTIM1_Start  start");
	HAL_LPTIM_Start(&Lptim1ContinuousHandle);
	Debug_Runtime_Add("HAL_LPTIM1_Start  stop");
}

/**
 * @brief 	LPTIMER2周期性定时初始化函数
 *  		这个函数描述了LpTimer初始化为continuous模式时需要的相关步骤。
 * 			在初始化函数内部需要设置定时器的编号、工作模式、重载值、时钟分频等，然后打开定时器。
 */
void Lptimer2_Period_Init(uint32_t ms)
{
	HAL_StatusTypeDef retval = HAL_OK;
    HAL_LPTIM_HandleTypeDef Lptim1ContinuousHandle = {0};
	Lptim1ContinuousHandle.Instance = HAL_LPTIM2;
	Lptim1ContinuousHandle.Init.Mode = HAL_LPTIM_MODE_CONTINUOUS;
	Debug_Runtime_Add("HAL_LPTIM2_SetTimeout  start");
	retval = HAL_LPTIM_SetTimeout(&Lptim1ContinuousHandle, ms);
	if(retval != HAL_OK)
    {
        xy_assert(0);
    }

	Debug_Runtime_Add("HAL_LPTIM2_Init  start");
	HAL_LPTIM_Init(&Lptim1ContinuousHandle);

	Debug_Runtime_Add("HAL_LPTIM2_Start  start");
	HAL_LPTIM_Start(&Lptim1ContinuousHandle);
	Debug_Runtime_Add("HAL_LPTIM2_Start  stop");
}

/**
 * @brief 配置GPIO5、6为推挽输出
 */
void GPIO_Output_Init(void)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};
	gpio_init.Pin = GPIO_PAD_NUM_5;
	gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
	HAL_GPIO_Init(&gpio_init);
	gpio_init.Pin = GPIO_PAD_NUM_6;
	gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
	HAL_GPIO_Init(&gpio_init);
}

/**
 * @brief demo主函数,在此Demo中，先对LpTimer周期性定时初始化，当到达设定时长时会产生中断，然后在中断回调内翻转GPIO. 
 * @note 用户使用xy_printf需要将XY_DEBUG宏置为1,详情参考xy_printf.c.
 */
__RAM_FUNC int main(void)
{
	SystemInit();

	xy_printf("Demo Start\n");

	GPIO_Output_Init();

	Lptimer1_Period_Init(40);

    Lptimer2_Period_Init(10);

	while (1)
	{
		;
	}

	return 0;
}
