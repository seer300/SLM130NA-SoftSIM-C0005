/**
 * @file uart_poll_demo.c
 * @author pancq
 * @brief UART轮询收发Demo
 * 		  在这个Demo中，UART先进行阻塞接收MAX_DATA_LEN个字节的数据，阻塞的时间为500ms，
 *        如果在500ms内收到MAX_DATA_LEN个字节的数据则将接收到的数据进行回写，
 *        如果500ms未成功接收指定字节数则将实际收到的数据进行回写.
 * @note  除非是特别简单的应用，一般不会使用轮询模式。如果要使用轮询模式，一定要有个良好的程序架构或实现机制，避免程序无限挂起.
 * @version 0.1
 * @date 2023-12-09
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "hal_gpio.h"
#include "hal_uart.h"

//当Timeout不是HAL_MAX_DELAY时，最大接收数据长度为UART的硬件RXFIFO深度（64字节），
//若外部发过来的数据大于64字节则会溢出，导致接收数据丢失。
#define MAX_DATA_LEN 512
uint8_t data[MAX_DATA_LEN] = {0x00};

HAL_UART_HandleTypeDef g_uart_poll_handle = {0};

/**
 * @brief UART初始化函数，主要完成以下配置：
 * 		  1. UART的TXD、RXD、CTS、RTS引脚的GPIO初始化配置
 * 		  2. UART的波特率、位宽、奇偶检验模式、硬件流控的配置
 */
void uart_poll_init(void)
{
	//gpio初始化
	HAL_GPIO_InitTypeDef uart_gpio_init = {0};

    uart_gpio_init.Pin = GPIO_PAD_NUM_9;
    uart_gpio_init.PinRemap = GPIO_UART2_TXD;
    uart_gpio_init.Mode = GPIO_MODE_HW_PER;
    HAL_GPIO_Init(&uart_gpio_init);

    uart_gpio_init.Pin = GPIO_PAD_NUM_10;
    uart_gpio_init.PinRemap = GPIO_UART2_RXD;
    uart_gpio_init.Mode = GPIO_MODE_HW_PER;
    HAL_GPIO_Init(&uart_gpio_init);

    uart_gpio_init.Pin = GPIO_PAD_NUM_5;
    uart_gpio_init.PinRemap = GPIO_UART2_CTS;
    uart_gpio_init.Mode = GPIO_MODE_HW_PER;
    HAL_GPIO_Init(&uart_gpio_init);

    uart_gpio_init.Pin = GPIO_PAD_NUM_6;
    uart_gpio_init.PinRemap = GPIO_UART2_RTS;
    uart_gpio_init.Mode = GPIO_MODE_HW_PER;
    HAL_GPIO_Init(&uart_gpio_init);

	//UART初始化
    g_uart_poll_handle.Instance = UART;
    g_uart_poll_handle.Init.BaudRate = 9600;
    g_uart_poll_handle.Init.WordLength = UART_WORDLENGTH_8;
    g_uart_poll_handle.Init.Parity = UART_PARITY_NONE;
    HAL_UART_Init(&g_uart_poll_handle);
}

/** 
 * @brief UART轮询收发Demo
 * 		  在这个Demo中，UART先进行阻塞接收MAX_DATA_LEN个字节的数据，阻塞的时间为500ms，
 *        如果在500ms内收到MAX_DATA_LEN个字节的数据则将接收到的数据进行回写，
 *        如果500ms未成功接收指定字节数则将实际收到的数据进行回写.
 * @note  除非是特别简单的应用，一般不会使用轮询模式。如果要使用轮询模式，一定要有个良好的程序架构或实现机制，避免程序无限挂起.
 */
__RAM_FUNC int main(void)
{
	SystemInit();

    uart_poll_init();

	while(1)
	{
		//接收DATA_LEN个字节，设置超时时间为500ms
		HAL_StatusTypeDef ret = HAL_UART_Receive(&g_uart_poll_handle, data, MAX_DATA_LEN, HAL_MAX_DELAY);
		switch(ret)
		{
			case HAL_OK: //成功接收指定长度数据
			{
				//此处demo将接收到的数据回写，用户可自行处理
				ret = HAL_UART_Transmit(&g_uart_poll_handle, data, MAX_DATA_LEN, 1000);
				while(ret != HAL_OK){;}
				break;
			}
			case HAL_TIMEOUT: //超时未成功接收指定字节数，此时RxXferCount为实际接收字节数
			{
				if(g_uart_poll_handle.RxXferCount > 0)
				{
					//此处demo将接收到的数据回写，用户可自行处理
					ret = HAL_UART_Transmit(&g_uart_poll_handle, data, g_uart_poll_handle.RxXferCount, 1000);
					while(ret != HAL_OK){;}
				}
				break;
			}
			case HAL_ERROR: //入参错误
			case HAL_BUSY:  //外设正在使用中，即本轮接收还未结束又调用HAL_UART_Receive接口
			{
				while(1){;}
			}
			default:;
		}
	}

	return 0;
}

