/**
* @file   gpio_input_demo.c
* @brief  GPIO输入Demo，配置GPIO6为上拉输入模式，GPIO5为下拉输入模式，并分别读取电平状态。
* @note   在这个DEMO中，先初始化GPIO，然后再读取引脚的电平状态，读取完成后通过xy_printf打印出GPIO的电平状态。
*
***********************************************************************************/
#include "hal_gpio.h"
#include "xy_printf.h"
#include "at_uart.h"

uint8_t gpio6_value = 0;
uint8_t gpio5_value = 0;

/**
 * @brief GPIO初始化函数，主要完成以下配置：
 *        1. 配置GPIO6为上拉输入
 *        2. 配置GPIO5为下拉输入
 */
void GPIO_Input_Init(void)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};

	gpio_init.Pin = GPIO_PAD_NUM_6;
	gpio_init.Mode = GPIO_MODE_INPUT;
	gpio_init.Pull = GPIO_PULL_UP;
	HAL_GPIO_Init(&gpio_init);

	gpio_init.Pin = GPIO_PAD_NUM_5;
	gpio_init.Mode = GPIO_MODE_INPUT;
	gpio_init.Pull = GPIO_PULL_DOWN;
	HAL_GPIO_Init(&gpio_init);
}

/**
 * @brief 	读取引脚的输入状态，并打印出来。
 * @note	用户使用xy_printf需要将XY_DEBUG宏置为1,详情参考xy_printf.c。
 */
__RAM_FUNC int main(void)
{
	SystemInit();

	GPIO_Input_Init();

	while (1)
	{
		gpio6_value = HAL_GPIO_ReadPin(GPIO_PAD_NUM_6);
		gpio5_value = HAL_GPIO_ReadPin(GPIO_PAD_NUM_5);
        xy_printf("\r\nHAL_GPIO_PIN_NUM_6 Value: %d\r\nHAL_GPIO_PIN_NUM_5 Value: %d\r\n", gpio6_value, gpio5_value);
        HAL_Delay(1000);
	}

	return 0;
}
