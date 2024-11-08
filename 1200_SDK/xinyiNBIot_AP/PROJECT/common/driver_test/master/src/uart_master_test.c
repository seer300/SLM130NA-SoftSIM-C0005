/**
 * @file uart_master_test.c
 * @brief 主设备测试代码
 * @version 0.2
 * @date 2022-10-08
 */

#include <stdio.h>
#include <string.h>
#include "hal_gpio.h"
#include "hal_uart.h"
#include "master.h"

#if (DRIVER_TEST == 1)

/******************************************************************************************/
UartArgStruct UartArgStruct_Demo = {0};
uint32_t UartTxPinArray[] = {GPIO_PAD_NUM_9, MaxArg};
uint32_t UartRxPinArray[] = {GPIO_PAD_NUM_10, MaxArg};
uint32_t UartInstanceArray[] = {(uint32_t)UART, MaxArg};
uint32_t UartBaudrateArray[] = {UART_BAUD_2400, UART_BAUD_4800, UART_BAUD_9600, \
								UART_BAUD_19200, UART_BAUD_38400, UART_BAUD_57600, \
								UART_BAUD_115200, UART_BAUD_380400, UART_BAUD_460800,
								#if ((AP_HCLK_DIV > 4))  //HRC时钟下，时钟分频大于4时实测921600通信出错
								UART_BAUD_921600,
								#endif
								MaxArg}; 
uint32_t UartDatabitsArray[] = {UART_WORDLENGTH_8, UART_WORDLENGTH_6, UART_WORDLENGTH_7, MaxArg};
uint32_t UartStopbitsArray[] = {2, MaxArg};	//停止位默认2位，该值无效
uint32_t UartParityArray[] = {UART_PARITY_NONE, UART_PARITY_EVEN, UART_PARITY_ODD, MaxArg};
uint32_t UartTestSizeArray[] = {1, 2, 3, 63, 64, 65, 128, 512, MaxArg};

/******************************************************************************************/
HAL_UART_HandleTypeDef uart_master_handle = {0};
volatile uint8_t uart_master_tx_flag = 0;
volatile uint8_t uart_master_rx_flag = 0;

__RAM_FUNC void HAL_UART_ErrorCallback(HAL_UART_HandleTypeDef* huart)
{
	huart->ErrorCode = UART_ERROR_NONE;
}

__RAM_FUNC void HAL_UART_RxCpltCallback(HAL_UART_HandleTypeDef *huart)
{
	UNUSED_ARG(huart);
	uart_master_rx_flag = 1;
}

__RAM_FUNC void HAL_UART_TxCpltCallback(HAL_UART_HandleTypeDef *huart)
{
	UNUSED_ARG(huart);
	uart_master_tx_flag = 1;
}

static uint8_t uart_master_init(UartArgStruct *pUartArgStruct)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};

    if(pUartArgStruct->UartInstance == (uint32_t)UART)
    {
	    gpio_init.Pin 		= (HAL_GPIO_PinTypeDef)pUartArgStruct -> UartTxPin;
	    gpio_init.Mode 		= GPIO_MODE_HW_PER;
	    gpio_init.PinRemap  = GPIO_UART2_TXD;
	    HAL_GPIO_Init(&gpio_init);
	    gpio_init.Pin		= (HAL_GPIO_PinTypeDef)pUartArgStruct -> UartRxPin;
	    gpio_init.Mode		= GPIO_MODE_HW_PER;
	    gpio_init.PinRemap  = GPIO_UART2_RXD;
		HAL_GPIO_Init(&gpio_init);
    }

	uart_master_handle.Instance 		= (UART_TypeDef *)pUartArgStruct -> UartInstance;
    uart_master_handle.Init.BaudRate 	= pUartArgStruct -> UartBaudrate;
	uart_master_handle.Init.WordLength 	= pUartArgStruct -> UartDatabits;
	uart_master_handle.Init.Parity 		= pUartArgStruct -> UartParity;
	if(HAL_UART_Init(&uart_master_handle) == HAL_OK)
		return 1;
	else 
		return 0;
}

/******************************************************************************************/
static void uart_test(CuTest* tc, UartArgStruct *pUartArgStruct, uint32_t TestTimes)
{
	HAL_StatusTypeDef ret = HAL_OK;
    char CmdData[100] = {0};
    char RecvConfigBuffer[10] = {0};

    for(uint16_t i = 0; i < TestTimes; i++)
	{
        Debug_Print_Str(DEBUG_LEVEL_1, "\r\ntest times:%d/%d\r\n", i+1, TestTimes);

        /*************************主机初始化并准备收发缓存区****************************/
        //主机uart初始化
		if(uart_master_init(pUartArgStruct) == 0)
		{
			Debug_Print_Str(DEBUG_LEVEL_1, "\r\nuart_master_init fail\r\n");
			continue;
		}
        //根据size申请内存，并初始化为全0
		char *pSendBuffer = (char *)xy_malloc(pUartArgStruct -> UartTestSize + 1);
		char *pRecvBuffer = (char *)xy_malloc(pUartArgStruct -> UartTestSize + 1);
        memset(pSendBuffer, 0, pUartArgStruct -> UartTestSize + 1);
        memset(pRecvBuffer, 0, pUartArgStruct -> UartTestSize + 1);
        //初始化pSendBuffer
		for(uint16_t i = 0; i < pUartArgStruct -> UartTestSize; i++)
		{
			//i为64整数倍的时候，不要赋值'\0'
			((i % 64 == 0) ? *(pSendBuffer + i) = '\n' : (*(pSendBuffer + i) = i % 64));
		}

        /*****************************主机发送初始化参数给从机*********************************/
		GetCmdDataToSlave(CmdData, (uint32_t)test_UART, sizeof(UartArgStruct), InputDebugLevel, pUartArgStruct);
		HAL_CSP_Transmit(&MasterSlave_UART_Handle, (uint8_t *)CmdData, sizeof(UartArgStruct) + 12, TRANSMIT_DEFAULT_MAX_DELAY);

        /*************************主机等待从机初始化完成后回复"OK\n"************************/
		ret = WAIT_OK;
        Debug_Print_Str(DEBUG_LEVEL_2, "\r\n%s", RecvConfigBuffer);
        if(ret != HAL_OK)
		{
			CuFail(tc, "master send config failed");
		}
        if(strstr(RecvConfigBuffer, "OK") == NULL)
		{
			CuFail(tc, "slave config failed");
		}

    	/*************************主机uart开始向从机发送数据并接收"************************/
		if(i % 2 == 0)
		{
			/*******主机阻塞发送*******/
            Debug_Print_Str(DEBUG_LEVEL_1, "\r\nuart主机阻塞发送");
            Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pSendBuffer, pUartArgStruct -> UartTestSize);

			HAL_Delay(5); //确保从机先等待OK
			SEND_OK;
			HAL_Delay(5); //确保从机先进入接收API

            ret = HAL_UART_Transmit(&uart_master_handle, (uint8_t *)pSendBuffer, pUartArgStruct -> UartTestSize, TRANSMIT_DEFAULT_MAX_DELAY);
			if(ret != HAL_OK)
			{
				CuFail(tc, "tramsmit test failed");
			}

			/*******主机阻塞接收*******/
            Debug_Print_Str(DEBUG_LEVEL_1, "\r\nuart主机阻塞接收");

			WAIT_OK;

			//从机初始化， 会导致主机误接收，FIFO中有脏数据，此时需要先清FIFO再接收
			uart_master_handle.Instance->FLUSH |= 0x01;
			uart_master_handle.Instance->FLUSH = 0x00;

			ret = HAL_UART_Receive(&uart_master_handle, (uint8_t *)pRecvBuffer, pUartArgStruct -> UartTestSize, RECEIVE_DEFAULT_MAX_DELAY);
			if(ret != HAL_OK)
			{
				CuFail(tc, "receive test failed");
			}
            Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pUartArgStruct -> UartTestSize);
		}
		else
		{
			/*******主机非阻塞发送*******/
            Debug_Print_Str(DEBUG_LEVEL_1, "\r\nuart主机非阻塞发送");
            Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pSendBuffer, pUartArgStruct -> UartTestSize);

			HAL_Delay(5); //确保从机先等待OK
			SEND_OK;
			HAL_Delay(5); //确保从机先进入接收API

			ret = HAL_UART_Transmit_IT(&uart_master_handle, (uint8_t *)pSendBuffer, pUartArgStruct -> UartTestSize);
            if(ret != HAL_OK)
			{
				CuFail(tc, "tramsmit test failed");
			}
			while(uart_master_tx_flag != 1){;}
			uart_master_tx_flag = 0;

			/*******主机非阻塞接收*******/
            Debug_Print_Str(DEBUG_LEVEL_1, "\r\nuart主机非阻塞接收");

			WAIT_OK;

			ret = HAL_UART_Receive_IT(&uart_master_handle, (uint8_t *)pRecvBuffer, pUartArgStruct -> UartTestSize,3);
			if(ret != HAL_OK)
			{
				CuFail(tc, "receive test failed");
			}
            while(uart_master_rx_flag != 1){;}
			uart_master_rx_flag = 0;
			Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pUartArgStruct -> UartTestSize);
		}
		
		HAL_Delay(5); //确保从机先等待OK
		SEND_OK;
		HAL_Delay(5); //确保从机先进入从机接收状态

		HAL_UART_DeInit(&uart_master_handle);
		UART_GPIO_Remove();

		Debug_Print_Str(DEBUG_LEVEL_2, "\r\n%dth sendlen:%d recvlen:%d\r\n", i+1, strlen(pSendBuffer), strlen(pRecvBuffer));

		CuAssertStrEquals(tc, pSendBuffer, pRecvBuffer);

        xy_free(pSendBuffer);
        xy_free(pRecvBuffer);
	}
}

/******************************************************************************************/
void uart_master_test(CuTest* tc)
{
	Debug_Print_Str(DEBUG_LEVEL_0, "\r\n<uart> test begin\r\n");

	uint32_t CaseNum = 1;

	InitArgStruct(&UartArgStruct_Demo, UartTxPinArray, UartRxPinArray, UartInstanceArray, UartBaudrateArray, \
									   UartDatabitsArray, UartStopbitsArray, UartParityArray, UartTestSizeArray, MaxArgArray);

    while(GetArgStruct(&UartArgStruct_Demo) != 0)
	{
		Debug_Print_Str(DEBUG_LEVEL_1, "\r\n->uart case:%lu/%lu\r\n", CaseNum, GetTotalCaseNun());

		CaseNum++;

		Debug_Print_ArgStruct(DEBUG_LEVEL_1, &UartArgStruct_Demo);

		uart_test(tc, &UartArgStruct_Demo, TestTimes);
	}

	DeInitArgStruct(&UartArgStruct_Demo);

	Debug_Print_Str(DEBUG_LEVEL_0, "\r\n<uart> test end\r\n");
}

#endif /* #if (DRIVER_TEST == 1) */

