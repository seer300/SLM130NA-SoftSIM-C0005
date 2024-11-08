/**
 * @file csp_uart_master_test.c
 * @brief 主设备测试代码
 * @version 0.2
 * @date 2022-10-08
 */

#include <stdio.h>
#include <string.h>
#include "hal_gpio.h"
#include "hal_csp.h"
#include "master.h"

#if (DRIVER_TEST == 1)

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


/******************************************************************************************/
CspUartArgStruct CspUartArgStruct_Demo = {0};
uint32_t CspUartTxPinArray[] = {GPIO_PAD_NUM_9, MaxArg};
uint32_t CspUartRxPinArray[] = {GPIO_PAD_NUM_10, MaxArg};
uint32_t CspUartInstanceArray[] = {(uint32_t)HAL_CSP_TESTSEL, /*HAL_CSP3,*/MaxArg};//CSP1用于主从机通信，CSP3用于log口（测试时可以关闭log口测试）
uint32_t CspUartBaudrateArray[] = {HAL_CSP_UART_BAUD_2400,HAL_CSP_UART_BAUD_4800, HAL_CSP_UART_BAUD_9600,\
									HAL_CSP_UART_BAUD_19200,HAL_CSP_UART_BAUD_38400,HAL_CSP_UART_BAUD_57600,\
									HAL_CSP_UART_BAUD_115200,HAL_CSP_UART_BAUD_380400,HAL_CSP_UART_BAUD_460800,\
									HAL_CSP_UART_BAUD_921600, MaxArg};									
uint32_t CspUartDatabitsArray[] = {HAL_CSP_UART_WORDLENGTH_8, HAL_CSP_UART_WORDLENGTH_6, HAL_CSP_UART_WORDLENGTH_7, MaxArg};
uint32_t CspUartStopbitsArray[] = {HAL_CSP_UART_STOPBITS_1, HAL_CSP_UART_STOPBITS_2, MaxArg};
uint32_t CspUartParityArray[] = {HAL_CSP_UART_PARITY_NONE, HAL_CSP_UART_PARITY_EVEN, HAL_CSP_UART_PARITY_ODD, MaxArg};
uint32_t CspUartTestSizeArray[] = {1, 2, 3, 127, 128, 129, 512, MaxArg};


/******************************************************************************************/
HAL_CSP_HandleTypeDef csp_uart_master_handle = {0};
volatile uint8_t csp_uart_master_rx_flag = 0;
volatile uint8_t csp_uart_master_tx_flag = 0;

__RAM_FUNC void HAL_CSP_TESTSEL_ErrorCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	hcsp->ErrorCode = HAL_CSP_ERROR_NONE;
}

__RAM_FUNC void HAL_CSP_TESTSEL_TxCpltCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	UNUSED_ARG(hcsp);
	csp_uart_master_tx_flag = 1;
}

__RAM_FUNC void HAL_CSP_TESTSEL_RxCpltCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	UNUSED_ARG(hcsp);
	csp_uart_master_rx_flag = 1;
}

__RAM_FUNC void HAL_CSP3_ErrorCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	hcsp->ErrorCode = HAL_CSP_ERROR_NONE;
}

__RAM_FUNC void HAL_CSP3_TxCpltCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	UNUSED_ARG(hcsp);
	csp_uart_master_tx_flag = 1;
}

__RAM_FUNC void HAL_CSP3_RxCpltCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	UNUSED_ARG(hcsp);
	csp_uart_master_rx_flag = 1;
}

static uint8_t csp_uart_master_init(CspUartArgStruct *pCspUartArgStruct)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};

	if( pCspUartArgStruct->CspUartInstance == (uint32_t)HAL_CSP_TESTSEL)
	{
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspUartArgStruct->CspUartTxPin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP_TESTSEL_TXD;
		HAL_GPIO_Init(&gpio_init);
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspUartArgStruct->CspUartRxPin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP_TESTSEL_RXD;
		HAL_GPIO_Init(&gpio_init);
	}
	else if( pCspUartArgStruct->CspUartInstance == (uint32_t)HAL_CSP3)
	{
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspUartArgStruct->CspUartTxPin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP3_TXD;
		HAL_GPIO_Init(&gpio_init);
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspUartArgStruct->CspUartRxPin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP3_RXD;
		HAL_GPIO_Init(&gpio_init);
	}

	csp_uart_master_handle.Instance                 = (CSP_TypeDef *)pCspUartArgStruct->CspUartInstance;
	csp_uart_master_handle.CSP_UART_Init.BaudRate   = pCspUartArgStruct->CspUartBaudrate;
	csp_uart_master_handle.CSP_UART_Init.WordLength = pCspUartArgStruct->CspUartDatabits;
	csp_uart_master_handle.CSP_UART_Init.StopBits   = pCspUartArgStruct->CspUartStopbits;
	csp_uart_master_handle.CSP_UART_Init.Parity     = pCspUartArgStruct->CspUartParitybit;
	if(HAL_CSP_UART_Init(&csp_uart_master_handle)==HAL_OK)
		return 1;
	else 
		return 0;
}

/******************************************************************************************/
static void csp_uart_test(CuTest *tc, CspUartArgStruct *pCspUartArgStruct, uint32_t TestTimes)
{
	HAL_StatusTypeDef ret = HAL_OK;
	char CmdData[100] = {0};
	char RecvConfigBuffer[10] = {0};

	for (uint16_t i = 0; i < TestTimes; i++)
	{
		Debug_Print_Str(DEBUG_LEVEL_1, "\r\ntest times:%d/%d\r\n", i+1, TestTimes);

		/*************************主机初始化并准备收发缓存区****************************/
		//主机CSP_UART初始化
		if(csp_uart_master_init(pCspUartArgStruct) == 0)
		{
			Debug_Print_Str(DEBUG_LEVEL_1, "\r\ncsp_uart_master_init fail,the baudrate is too high\r\n");
			continue;
		}

		//根据size申请内存，并初始化为全0
		char *pSendBuffer = (char *)xy_malloc(pCspUartArgStruct->CspUartTestSize + 1);
		char *pRecvBuffer = (char *)xy_malloc(pCspUartArgStruct->CspUartTestSize + 1);
		memset(pSendBuffer, 0, pCspUartArgStruct->CspUartTestSize + 1);
		memset(pRecvBuffer, 0, pCspUartArgStruct->CspUartTestSize + 1);
		//初始化pSendBuffer
		for (uint16_t i = 0; i < pCspUartArgStruct->CspUartTestSize; i++)
		{
			// i为64整数倍的时候，不要赋值'\0'
			((i % 64 == 0) ? *(pSendBuffer + i) = '\n' : (*(pSendBuffer + i) = i % 64));
		}

		/*****************************主机发送初始化参数给从机*********************************/
		GetCmdDataToSlave(CmdData, (uint32_t)test_CSP_UART, sizeof(CspUartArgStruct), InputDebugLevel, pCspUartArgStruct);
		HAL_CSP_Transmit(&MasterSlave_UART_Handle, (uint8_t *)CmdData, sizeof(CspUartArgStruct) + 12, TRANSMIT_DEFAULT_MAX_DELAY);

		/*************************主机等待从机初始化完成后回复"OK\n"************************/
		ret = WAIT_OK;
		Debug_Print_Str(DEBUG_LEVEL_2, "\r\n%s", RecvConfigBuffer);
		if (ret != HAL_OK)
		{
			CuFail(tc, "master send config failed");
		}
		if (strstr(RecvConfigBuffer, "OK") == NULL)
		{
			CuFail(tc, "slave config failed");
		}

		/*************************主机csp_uart开始向从机发送数据并接收"************************/
		if (i % 2 == 0)
		{
			/*******主机阻塞发送*******/
			Debug_Print_Str(DEBUG_LEVEL_1, "\r\ncsp_uart主机阻塞发送");
			Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pSendBuffer, pCspUartArgStruct->CspUartTestSize);
			
			HAL_Delay(5); //确保从机先等待OK
			SEND_OK;
			HAL_Delay(5); //确保从机先进入接收API

			ret = HAL_CSP_Transmit(&csp_uart_master_handle, (uint8_t *)pSendBuffer, pCspUartArgStruct->CspUartTestSize, TRANSMIT_DEFAULT_MAX_DELAY);
			if(ret != HAL_OK)
			{
				CuFail(tc, "tramsmit test failed");
			}

			/*******主机阻塞接收*******/
            Debug_Print_Str(DEBUG_LEVEL_1, "\r\ncsp_uart主机阻塞接收");

			WAIT_OK;
			
			ret = HAL_CSP_Receive(&csp_uart_master_handle, (uint8_t *)pRecvBuffer, pCspUartArgStruct->CspUartTestSize, RECEIVE_DEFAULT_MAX_DELAY);
			if(ret != HAL_OK)
			{
				CuFail(tc, "receive test failed");
			}
			Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pCspUartArgStruct->CspUartTestSize);
		}
		else
		{
			/*******主机非阻塞发送*******/
			Debug_Print_Str(DEBUG_LEVEL_1, "\r\ncsp_uart主机非阻塞发送");
			Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pSendBuffer, pCspUartArgStruct->CspUartTestSize);
	
			HAL_Delay(5); //确保从机先等待OK
			SEND_OK;
			HAL_Delay(5); //确保从机先进入接收API

			ret = HAL_CSP_Transmit_IT(&csp_uart_master_handle, (uint8_t *)pSendBuffer, pCspUartArgStruct->CspUartTestSize);
			if(ret != HAL_OK)
			{
				CuFail(tc, "tramsmit test failed");
			}
			while (!csp_uart_master_tx_flag ){;}
			csp_uart_master_tx_flag = 0;

			/*******主机非阻塞接收*******/
            Debug_Print_Str(DEBUG_LEVEL_1, "\r\ncsp_uart主机非阻塞接收");

			WAIT_OK;

			ret = HAL_CSP_Receive_IT(&csp_uart_master_handle, (uint8_t *)pRecvBuffer, pCspUartArgStruct->CspUartTestSize, 5);
			if(ret != HAL_OK)
			{
				CuFail(tc, "receive test failed");
			}
			while (!csp_uart_master_rx_flag){;}
			csp_uart_master_rx_flag = 0;
			Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pCspUartArgStruct->CspUartTestSize);
		}

		HAL_Delay(5); //确保从机先等待OK
		SEND_OK;
		HAL_Delay(5); //确保从机先进入从机接收状态

		HAL_CSP_Drivertest_DeInit(&csp_uart_master_handle);

		Debug_Print_Str(DEBUG_LEVEL_2, "\r\n%dth sendlen:%d recvlen:%d\r\n", i + 1, strlen(pSendBuffer), strlen(pRecvBuffer));

		CuAssertStrEquals(tc, pSendBuffer, pRecvBuffer);

		xy_free(pSendBuffer);
		xy_free(pRecvBuffer);
	}
}

/******************************************************************************************/
void csp_uart_master_test(CuTest *tc)
{
	Debug_Print_Str(DEBUG_LEVEL_0, "\r\n<csp uart> test begin\r\n");

	uint32_t CaseNum = 1;

	InitArgStruct(&CspUartArgStruct_Demo, CspUartTxPinArray, CspUartRxPinArray, CspUartInstanceArray, CspUartBaudrateArray,\
				  CspUartDatabitsArray, CspUartStopbitsArray,CspUartParityArray, CspUartTestSizeArray, MaxArgArray);
	while (GetArgStruct(&CspUartArgStruct_Demo) != 0)
	{
		Debug_Print_Str(DEBUG_LEVEL_1, "\r\ncsp uart case:%lu/%lu\r\n", CaseNum, GetTotalCaseNun());

		CaseNum++;

		Debug_Print_ArgStruct(DEBUG_LEVEL_1, &CspUartArgStruct_Demo);

		csp_uart_test(tc, &CspUartArgStruct_Demo, TestTimes);
	}

	DeInitArgStruct(&CspUartArgStruct_Demo);

	Debug_Print_Str(DEBUG_LEVEL_0, "\r\n<csp uart> test end\r\n");
}

#endif /* #if (DRIVER_TEST == 1) */

