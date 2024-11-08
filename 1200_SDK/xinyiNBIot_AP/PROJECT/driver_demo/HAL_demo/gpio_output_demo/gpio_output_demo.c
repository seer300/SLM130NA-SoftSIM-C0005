/**
* @file        gpio_output_demo.c
* @brief       GPIO输出Demo，配置GPIO6,GPIO5为推挽输出模式，并定时切换GPIO输出状态.
*
***********************************************************************************/
#include "hal_gpio.h"

static HAL_GPIO_PinTypeDef PinArray[] = {GPIO_PAD_NUM_6,GPIO_PAD_NUM_5};

/**
 * @brief GPIO初始化函数，主要完成以下配置：
 *        1. 配置GPIO6为推挽输出
 *        2. 配置GPIO5为推挽输出
 */
void GPIO_Output_Init(void)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};

	gpio_init.Pin = GPIO_PAD_NUM_6;
	gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
	gpio_init.Pull = GPIO_PULL_DOWN;
	Debug_Runtime_Add("HAL_GPIO_Init  start");
	HAL_GPIO_Init(&gpio_init);
	Debug_Runtime_Add("HAL_GPIO_Init  stop");

	gpio_init.Pin = GPIO_PAD_NUM_5;
	gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
	gpio_init.Pull = GPIO_PULL_DOWN;
	HAL_GPIO_Init(&gpio_init);
}

/**
 * @brief	设置引脚6的输出状态，高低电平翻转.
 * @note	用户使用xy_printf需要将XY_DEBUG宏置为1,详情参考xy_printf.c.
 */
__RAM_FUNC int main(void)
{
	SystemInit();

	GPIO_Output_Init();

	while (1)
	{
		HAL_GPIO_WritePin(GPIO_PAD_NUM_6, GPIO_PIN_RESET);//低
		HAL_GPIO_WritePin(GPIO_PAD_NUM_5, GPIO_PIN_SET);  //高
		HAL_Delay(100);

        HAL_GPIO_TogglePin(GPIO_PAD_NUM_6);//高
        HAL_GPIO_TogglePin(GPIO_PAD_NUM_5);//低
		HAL_Delay(100);

        HAL_GPIO_WritePinArray(PinArray, 2, GPIO_PIN_RESET);//低
        HAL_Delay(100);

        HAL_GPIO_WritePinArray(PinArray, 2, GPIO_PIN_SET);//高
        HAL_Delay(100);
    }

	return 0;
}
