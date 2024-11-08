/**
 * @file uart_slave_test.c
 * @brief 从测试设备代码
 * @version 0.2
 * @date 2022-06-20
 */
#include <string.h>
#include "slave.h"
#include "hal_gpio.h"
#include "hal_uart.h"

#if (DRIVER_TEST == 2)

/******************************************************************************************/
HAL_UART_HandleTypeDef uart_slave_handle = {0};
volatile uint8_t uart_slave_rx_flag = 0;
volatile uint8_t uart_slave_tx_flag = 0;

__RAM_FUNC void HAL_UART_ErrorCallback(HAL_UART_HandleTypeDef* huart)
{
	huart->ErrorCode = UART_ERROR_NONE;
}

__RAM_FUNC void HAL_UART_RxCpltCallback(HAL_UART_HandleTypeDef *huart)
{
	UNUSED_ARG(huart);
	uart_slave_rx_flag = 1;
}

__RAM_FUNC void HAL_UART_TxCpltCallback(HAL_UART_HandleTypeDef *huart)
{
	UNUSED_ARG(huart);
	uart_slave_tx_flag = 1;
}

static void uart_slave_init(UartArgStruct *pUartArgStruct)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};

    if(pUartArgStruct->UartInstance == (uint32_t)UART)
    {
        gpio_init.Pin	   = (HAL_GPIO_PinTypeDef)pUartArgStruct -> UartRxPin;
        gpio_init.Mode	   = GPIO_MODE_HW_PER;
        gpio_init.PinRemap = GPIO_UART2_TXD;
        HAL_GPIO_Init(&gpio_init);
        gpio_init.Pin	   = (HAL_GPIO_PinTypeDef)pUartArgStruct -> UartTxPin;
	    gpio_init.Mode	   = GPIO_MODE_HW_PER;
        gpio_init.PinRemap = GPIO_UART2_RXD;
        HAL_GPIO_Init(&gpio_init);
    }

	uart_slave_handle.Instance		   = (UART_TypeDef *)pUartArgStruct -> UartInstance;
	uart_slave_handle.Init.BaudRate	   = pUartArgStruct -> UartBaudrate;
	uart_slave_handle.Init.WordLength  = pUartArgStruct -> UartDatabits;
	uart_slave_handle.Init.Parity      = pUartArgStruct -> UartParity;
	HAL_UART_Init(&uart_slave_handle);
}

/******************************************************************************************/
void uart_slave_test(UartArgStruct *pUartArgStruct)
{
	HAL_StatusTypeDef ret = HAL_OK;
	char RecvConfigBuffer[10] = {0};

    Debug_Print_Str(DEBUG_LEVEL_2, "\r\n<uart> test begin\r\n");
    Debug_Print_ArgStruct(DEBUG_LEVEL_1, pUartArgStruct);

    /*************************从机初始化并准备接收缓存区****************************/
    //uart从机初始化
    uart_slave_init(pUartArgStruct);
    //根据size申请内存，并初始化为全0
	char *pRecvBuffer = (char *)xy_malloc(pUartArgStruct -> UartTestSize);
    memset(pRecvBuffer, 0, pUartArgStruct -> UartTestSize);

    /***********************回应"OK\n"************************/
	SEND_OK;

    /***********************uart从机接收"************************/
    Debug_Print_Str(DEBUG_LEVEL_1, "\r\nuart从机非阻塞接收");

	WAIT_OK;

    ret = HAL_UART_Receive_IT(&uart_slave_handle, (uint8_t *)pRecvBuffer, pUartArgStruct->UartTestSize, 3);
    if(ret != HAL_OK)
    {
        return;
    }
	while(uart_slave_rx_flag != 1);
	uart_slave_rx_flag = 0;
    Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pUartArgStruct->UartTestSize);

    /***********************uart从机发送"************************/
    Debug_Print_Str(DEBUG_LEVEL_1, "\r\nuart从机非阻塞发送");
    Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pUartArgStruct -> UartTestSize);

	HAL_Delay(5); //确保主机先等待OK
	SEND_OK;
	HAL_Delay(5); //确保主机先进入接收API

    ret = HAL_UART_Transmit_IT(&uart_slave_handle, (uint8_t *)pRecvBuffer, pUartArgStruct->UartTestSize);
    if(ret != HAL_OK)
    {
        return;
    }
	while(uart_slave_tx_flag != 1);
	uart_slave_tx_flag = 0;

	WAIT_OK;

    HAL_UART_DeInit(&uart_slave_handle);
    UART_GPIO_Remove();

	xy_free(pRecvBuffer);

	Debug_Print_Str(DEBUG_LEVEL_2, "\r\n<uart> test end\r\n");
}

#endif /* #if (DRIVER_TEST == 2) */

