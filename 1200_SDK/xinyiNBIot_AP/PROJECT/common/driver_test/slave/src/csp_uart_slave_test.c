/**
 * @file csp_uart_slave_test.c
 * @brief 从测试设备代码
 * @version 0.2
 * @date 2022-06-20
 */
#include <string.h>
#include "slave.h"
#include "hal_gpio.h"
#include "hal_csp.h"

#if (DRIVER_TEST == 2)

#if (MasterSlave_UART == 1) //若主从通道选择CSP1,则CSP2参与驱动测试

#define GPIO_CSP_TESTSEL_TXD	GPIO_CSP2_TXD
#define GPIO_CSP_TESTSEL_RXD	GPIO_CSP2_RXD 
#define HAL_CSP_TESTSEL			HAL_CSP2

#define HAL_CSP_TESTSEL_ErrorCallback(__HANDLE__)  HAL_CSP2_ErrorCallback(__HANDLE__)
#define HAL_CSP_TESTSEL_TxCpltCallback(__HANDLE__)	HAL_CSP2_TxCpltCallback(__HANDLE__)
#define HAL_CSP_TESTSEL_RxCpltCallback(__HANDLE__)	HAL_CSP2_RxCpltCallback(__HANDLE__)

#elif (MasterSlave_UART == 2) //若主从通道选择CSP2,则CSP1参与驱动测试

#define GPIO_CSP_TESTSEL_TXD	GPIO_CSP1_TXD
#define GPIO_CSP_TESTSEL_RXD	GPIO_CSP1_RXD 
#define HAL_CSP_TESTSEL			HAL_CSP1

#define HAL_CSP_TESTSEL_ErrorCallback(__HANDLE__)  HAL_CSP1_ErrorCallback(__HANDLE__)
#define HAL_CSP_TESTSEL_TxCpltCallback(__HANDLE__)	HAL_CSP1_TxCpltCallback(__HANDLE__)
#define HAL_CSP_TESTSEL_RxCpltCallback(__HANDLE__)	HAL_CSP1_RxCpltCallback(__HANDLE__)

#endif

#define CSP_UART_Slave_Delay(ms)	HAL_Delay(ms)

/******************************************************************************************/
HAL_CSP_HandleTypeDef csp_uart_slave_handle = {0};
volatile uint8_t csp_uart_slave_rx_flag = 0;
volatile uint8_t csp_uart_slave_tx_flag = 0;

__RAM_FUNC void HAL_CSP_TESTSEL_ErrorCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	hcsp->ErrorCode = HAL_CSP_ERROR_NONE;
}

__RAM_FUNC void HAL_CSP_TESTSEL_TxCpltCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	UNUSED_ARG(hcsp);
	csp_uart_slave_tx_flag = 1;
}

__RAM_FUNC void HAL_CSP_TESTSEL_RxCpltCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	UNUSED_ARG(hcsp);
	csp_uart_slave_rx_flag = 1;
}

__RAM_FUNC void HAL_CSP3_ErrorCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	hcsp->ErrorCode = HAL_CSP_ERROR_NONE;
}

__RAM_FUNC void HAL_CSP3_TxCpltCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	UNUSED_ARG(hcsp);
	csp_uart_slave_tx_flag = 1;
}

__RAM_FUNC void HAL_CSP3_RxCpltCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	UNUSED_ARG(hcsp);
	csp_uart_slave_rx_flag = 1;
}

static void csp_uart_slave_init(CspUartArgStruct *pCspUartArgStruct)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};

	if( pCspUartArgStruct->CspUartInstance == (uint32_t)HAL_CSP_TESTSEL)
	{
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspUartArgStruct->CspUartRxPin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP_TESTSEL_TXD;
		HAL_GPIO_Init(&gpio_init);
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspUartArgStruct->CspUartTxPin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP_TESTSEL_RXD;
		HAL_GPIO_Init(&gpio_init);
	}
	else if( pCspUartArgStruct->CspUartInstance == (uint32_t)HAL_CSP3)
	{
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspUartArgStruct->CspUartRxPin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP3_TXD;
		HAL_GPIO_Init(&gpio_init);
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspUartArgStruct->CspUartTxPin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP3_RXD;
		HAL_GPIO_Init(&gpio_init);
	}

	csp_uart_slave_handle.Instance                 = (CSP_TypeDef *)pCspUartArgStruct->CspUartInstance;
	csp_uart_slave_handle.CSP_UART_Init.BaudRate   = pCspUartArgStruct->CspUartBaudrate;
	csp_uart_slave_handle.CSP_UART_Init.WordLength = pCspUartArgStruct->CspUartDatabits;
	csp_uart_slave_handle.CSP_UART_Init.StopBits   = pCspUartArgStruct->CspUartStopbits;
	csp_uart_slave_handle.CSP_UART_Init.Parity     = pCspUartArgStruct->CspUartParitybit;
	HAL_CSP_UART_Init(&csp_uart_slave_handle);
}

/******************************************************************************************/
void csp_uart_slave_test(CspUartArgStruct *pCspUartArgStruct)
{
	HAL_StatusTypeDef ret = HAL_OK;
	char RecvConfigBuffer[10] = {0};

    Debug_Print_Str(DEBUG_LEVEL_2, "\r\n<csp_uart> test begin\r\n");
	Debug_Print_ArgStruct(DEBUG_LEVEL_1, pCspUartArgStruct);

    /*************************从机初始化并准备接收缓存区****************************/
    //csp_uart从机初始化
	csp_uart_slave_init(pCspUartArgStruct);
    //根据size申请内存，并初始化为全0
	char *pRecvBuffer = (char *)xy_malloc(pCspUartArgStruct->CspUartTestSize);
	memset(pRecvBuffer, 0, pCspUartArgStruct->CspUartTestSize);

    /***********************回应"OK\n"************************/
	SEND_OK;

	/***********************csp_uart从机接收"************************/
	Debug_Print_Str(DEBUG_LEVEL_1, "\r\ncsp_uart从机非阻塞接收");

	WAIT_OK;

	ret = HAL_CSP_Receive_IT(&csp_uart_slave_handle, (uint8_t *)pRecvBuffer, pCspUartArgStruct->CspUartTestSize, 5);
	if(ret != HAL_OK)
	{
		return;
	}
	while(!csp_uart_slave_rx_flag);
	csp_uart_slave_rx_flag = 0;
	Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pCspUartArgStruct->CspUartTestSize);

	/***********************csp_uart从机发送"************************/
    Debug_Print_Str(DEBUG_LEVEL_1, "\r\ncsp_uart从机非阻塞发送");
	Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pCspUartArgStruct->CspUartTestSize);

	HAL_Delay(5); //确保主机先等待OK
	SEND_OK;
	HAL_Delay(5); //确保主机先进入接收API

	ret = HAL_CSP_Transmit_IT(&csp_uart_slave_handle, (uint8_t *)pRecvBuffer, pCspUartArgStruct->CspUartTestSize);
	if(ret != HAL_OK)
	{
		return;
	}
	while (!csp_uart_slave_tx_flag);
	csp_uart_slave_tx_flag = 0;
	
	WAIT_OK;

	HAL_CSP_Drivertest_DeInit(&csp_uart_slave_handle);

	xy_free(pRecvBuffer);

	Debug_Print_Str(DEBUG_LEVEL_2, "\r\n<csp_uart> test end\r\n");
}

#endif	/* #if (DRIVER_TEST == 2) */
