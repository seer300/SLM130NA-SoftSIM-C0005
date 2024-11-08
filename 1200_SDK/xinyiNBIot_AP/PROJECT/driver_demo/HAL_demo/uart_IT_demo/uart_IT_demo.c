/**
 * @file uart_IT_demo.c
 * @author pancq
 * @brief UART中断收发Demo
 *        在这个Demo中，UART首先通过中断接收数据，接收完成后置位标志g_uart_rxdone_IT，
 *        然后调用中断发送API将收到的数据进行回写，发送完成后置位标志g_uart_txdone_IT。
 * @version 0.1
 * @date 2023-12-08
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "hal_gpio.h"
#include "hal_uart.h"

#define MAX_DATA_LEN 512
uint8_t data[MAX_DATA_LEN] = {0x00};

volatile uint8_t g_uart_rxdone_IT = 0;
volatile uint8_t g_uart_txdone_IT = 0;

HAL_UART_HandleTypeDef g_uart_IT_handle = {0};
uint32_t g_RxXferCount = 0;

/**
 * @brief UART错误回调函数
 * 		  错误码类型有：帧错误，奇偶校验错误，接收溢出错误，
 *        错误码可以为三个中任意的一个，或者为它们的组合值。
 */
__RAM_FUNC void HAL_UART_ErrorCallback(HAL_UART_HandleTypeDef *huart)
{
	//用户根据实际需求添加错误处理代码
	//错误处理完后恢复ErrorCode
    huart->ErrorCode = UART_ERROR_NONE;
}

/**
 * @brief UART接收完成回调函数
 *        程序运行至此表明外部发送来的数据均已接收至接收缓存区。
 */
__RAM_FUNC void HAL_UART_RxCpltCallback(HAL_UART_HandleTypeDef *huart)
{
	//收到有效数据置位标志
	if(huart->RxXferCount > 0)
	{
	    g_uart_rxdone_IT = 1;
        g_RxXferCount = huart->RxXferCount;

        //开启中断接收数据
        HAL_StatusTypeDef ret = HAL_UART_Receive_IT(huart, data, MAX_DATA_LEN, 1000);
        while(ret != HAL_OK){;}
	}
}

/**
 * @brief UART发送完成回调函数
 *        程序运行至此表明发送缓存区数据均已发送至外部总线上。
 */
__RAM_FUNC void HAL_UART_TxCpltCallback(HAL_UART_HandleTypeDef *huart)
{
    //数据全部发完置位标志
	if(huart->TxXferSize == 0)
	{
        g_uart_txdone_IT = 1;
	}
}

/**
 * @brief UART初始化函数，主要完成以下配置：
 * 		  1. UART的TXD、RXD、CTS、RTS引脚的GPIO初始化配置
 * 		  2. UART的波特率、位宽、奇偶检验模式、硬件流控的配置
 */
void uart_IT_init(void)
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
    g_uart_IT_handle.Instance = UART;
    g_uart_IT_handle.Init.BaudRate = 9600;
    g_uart_IT_handle.Init.WordLength = UART_WORDLENGTH_8;
    g_uart_IT_handle.Init.Parity = UART_PARITY_NONE;
    HAL_UART_Init(&g_uart_IT_handle);
}

/**
 * @brief UART中断收发Demo
 * @brief 在这个Demo中，UART首先通过中断接收数据，接收完成后置位标志g_uart_rxdone_IT，
 *        然后调用中断发送API将收到的数据进行回写，发送完成后置位标志g_uart_txdone_IT。
 */
__RAM_FUNC int main(void)
{
	SystemInit();

	uart_IT_init();

    //开启中断接收数据
    //如果HardTimeout为HAL_MAX_DELAY，则表示不使用UART模块自身的硬件超时功能，只有在用户指定长度的数据全部收完时，才会触发接收完成回调函数的调用;
    //如果HardTimeout为非0非HAL_MAX_DELAY，则表示设定并使用UART模块自身的硬件超时功能，当用户指定长度的数据没有全部收完时，若出现一段数据接收间隔大于设定的超时时间后就会触发接收完成回调函数的调用。
    HAL_StatusTypeDef ret = HAL_UART_Receive_IT(&g_uart_IT_handle, data, MAX_DATA_LEN, 1000);
    while(ret != HAL_OK){;}

	while(1)
	{
        if(g_uart_rxdone_IT == 1)
        {
            g_uart_rxdone_IT = 0;

            //此处demo将接收到的数据回写，用户可自行处理
            ret = HAL_UART_Transmit_IT(&g_uart_IT_handle, data, g_RxXferCount);
            while(ret != HAL_OK){;}

            //等待发送完成
            while(g_uart_txdone_IT != 1);
            g_uart_txdone_IT = 0;
        }
    }

    return 0;
}
