/**
 * @file   gpio_interrupt_demo.c
 * @brief  GPIO外部中断Demo
 *         在这个Demo中，配置GPIO6和GPIO5为下拉输入，其中GPIO6配置为上升沿触发，GPIO5配置为下降沿触发.
 *         分别在中断服务函数中累计GPIO6和GPIO5中断触发的次数，并在main函数的while循环中调用xy_printf打印中断触发的次数.
 ***********************************************************************************/
#include "hal_gpio.h"
#include "xy_printf.h"
#include "at_uart.h"

volatile uint16_t int1_times = 0;
volatile uint16_t int2_times = 0;

/**
 * @brief 	GPIO引脚外部中断的回调函数，在此函数中，先读取GPIO的中断状态，若有中断发生，则将相应引脚的中断次数累加，
 */
__RAM_FUNC void HAL_GPIO_InterruptCallback(void)
{
	if (HAL_GPIO_ReadIntFlag(GPIO_PAD_NUM_6))	
	{
		int1_times++;
	}
	if (HAL_GPIO_ReadIntFlag(GPIO_PAD_NUM_5))
	{
	    int2_times++;
	}
	HAL_GPIO_ClearIntFlag(GPIO_PAD_NUM_6);
	HAL_GPIO_ClearIntFlag(GPIO_PAD_NUM_5);
}

/**
 * @brief GPIO初始化函数，主要完成以下配置：
 *        1. 配置GPIO6为上升沿触发中断
 *        2. 配置GPIO5为下降沿触发中断
 */
void GPIO_Interrupt_Init(void)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};

	gpio_init.Pin = GPIO_PAD_NUM_6;
	gpio_init.Mode = GPIO_MODE_INPUT;
	gpio_init.Pull = GPIO_PULL_DOWN;
	gpio_init.Int = GPIO_INT_RISE_EDGE;
	HAL_GPIO_Init(&gpio_init);

	gpio_init.Pin = GPIO_PAD_NUM_5;
	gpio_init.Mode = GPIO_MODE_INPUT;
	gpio_init.Pull = GPIO_PULL_DOWN;
	gpio_init.Int = GPIO_INT_FALL_EDGE;
	HAL_GPIO_Init(&gpio_init);
}

/**
 * @brief   打印中断次数为演示，用户可根据需要，自行开发.
 *
 * @note	用户使用xy_printf需要将XY_DEBUG宏置为1,详情参考xy_printf.c.
 */
__RAM_FUNC int main(void)
{
	SystemInit();

    GPIO_Interrupt_Init();

	while (1)
	{
		xy_printf("\r\nGPIO_PAD_NUM_6 interrupt times: %d\r\nGPIO_PAD_NUM_5 interrupt times: %d\r\n", int1_times, int2_times);
		HAL_Delay(1000);
	}

	return 0;
}
