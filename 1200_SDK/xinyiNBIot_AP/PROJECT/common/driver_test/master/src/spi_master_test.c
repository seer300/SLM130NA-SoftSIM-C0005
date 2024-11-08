/**
 * @file spi_master_test.c
 * @brief 主设备测试代码
 * @version 0.2
 * @date 2022-10-17
 */

#include <stdio.h>
#include <string.h>
#include "hal_gpio.h"
#include "hal_spi.h"
#include "master.h"

#if (DRIVER_TEST == 1)

/******************************************************************************************/
SpiArgStruct SpiArgStruct_Demo = {0};
uint32_t SpiMosiPinArray[] = {GPIO_PAD_NUM_9, MaxArg};
uint32_t SpiMisoPinArray[] = {GPIO_PAD_NUM_10, MaxArg};
uint32_t SpiSclkPinArray[] = {GPIO_PAD_NUM_5, MaxArg};
uint32_t SpiNss1PinArray[] = {GPIO_PAD_NUM_6, MaxArg};
uint32_t SpiInstanceArray[] = {(uint32_t)HAL_SPI, MaxArg};
uint32_t SpiWorkModeArray[] = {HAL_SPI_WORKMODE_0 , HAL_SPI_WORKMODE_1, HAL_SPI_WORKMODE_2, HAL_SPI_WORKMODE_3, MaxArg};
uint32_t SpiClkDivArray[] = {HAL_SPI_CLKDIV_256, HAL_SPI_CLKDIV_128, SPI_CONFIG_CLK_DIV_64,\
							 HAL_SPI_CLKDIV_32, HAL_SPI_CLKDIV_16, /*SPI_CONFIG_CLK_DIV_8,\
							 HAL_SPI_CLKDIV_4,HAL_SPI_CLKDIV_2,*/MaxArg}; //SPI参考时钟为peri,当前软件版本为92.16M,注意1、主机速度理论上pclk/2，实际上最高只能到14M；从机速度理论上为pclk/4,但实际测试，最高只达到6M左右
uint32_t SpiTestSizeArray[] = {1, 2, 3, 4, 125, 126, 127, 128, 512, 513, 514, 515, MaxArg};

/******************************************************************************************/
HAL_SPI_HandleTypeDef spi_master_handle = {0};
volatile uint8_t spi_master_tx_flag = 0;
volatile uint8_t spi_master_rx_flag = 0;

__RAM_FUNC void HAL_SPI_ErrorCallback(HAL_SPI_HandleTypeDef *hspi)
{
	hspi->ErrorCode = HAL_SPI_ERROR_NONE;
}

__RAM_FUNC void HAL_SPI_RxCpltCallback(HAL_SPI_HandleTypeDef *hspi)
{
	UNUSED_ARG(hspi);
	spi_master_rx_flag = 1;
}

__RAM_FUNC void HAL_SPI_TxCpltCallback(HAL_SPI_HandleTypeDef *hspi)
{
	UNUSED_ARG(hspi);
	spi_master_tx_flag = 1;
}

static void spi_master_init(SpiArgStruct *pSpiArgStruct)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};

    if(pSpiArgStruct->SpiInstance == (uint32_t)HAL_SPI)
    {
		gpio_init.Pin		= (HAL_GPIO_PinTypeDef)pSpiArgStruct -> SpiMosiPin;
		gpio_init.Mode		= GPIO_MODE_HW_PER;
		gpio_init.PinRemap  = GPIO_SPI_MOSI;
		HAL_GPIO_Init(&gpio_init);
		gpio_init.Pin		= (HAL_GPIO_PinTypeDef)pSpiArgStruct -> SpiMisoPin;
		gpio_init.Mode		= GPIO_MODE_HW_PER;
		gpio_init.PinRemap  = GPIO_SPI_MISO;
		HAL_GPIO_Init(&gpio_init);
		gpio_init.Pin		= (HAL_GPIO_PinTypeDef)pSpiArgStruct -> SpiSclkPin;
		gpio_init.Mode		= GPIO_MODE_HW_PER;
		gpio_init.PinRemap  = GPIO_SPI_SCLK;
		HAL_GPIO_Init(&gpio_init);
		gpio_init.Pin		= (HAL_GPIO_PinTypeDef)pSpiArgStruct -> SpiNss1Pin;
		gpio_init.Mode		= GPIO_MODE_HW_PER;
		gpio_init.PinRemap  = GPIO_SPI_SS_N;
		HAL_GPIO_Init(&gpio_init);
	}

	spi_master_handle.Instance				= (SPI_TypeDef *)pSpiArgStruct -> SpiInstance;
	spi_master_handle.Init.MasterSlave		= HAL_SPI_MODE_MASTER;
	spi_master_handle.Init.WorkMode			= pSpiArgStruct -> SpiWorkMode;
	spi_master_handle.Init.Clock_Prescaler	= pSpiArgStruct -> SpiClkDiv;
	HAL_SPI_Init(&spi_master_handle);
}

/******************************************************************************************/
static void spi_test(CuTest* tc, SpiArgStruct *pSpiArgStruct, uint32_t TestTimes)
{
	HAL_StatusTypeDef ret = HAL_OK;
	char CmdData[100] = {0};
    char RecvConfigBuffer[10] = {0};

	for(uint16_t i = 0; i < TestTimes; i++)
	{
		Debug_Print_Str(DEBUG_LEVEL_1, "\r\ntest times:%d/%d\r\n", i+1, TestTimes);

		if((GetPCLK2Freq()/pSpiArgStruct->SpiClkDiv) > 14000000)//SPI测试时，spi支持的最大速度为14M
		{
			Debug_Print_Str(DEBUG_LEVEL_1, "\r\nThese parameters are not supported:\r\n");
			continue;
		}
        /*************************主机初始化并准备收发缓存区****************************/
		//主机SPI初始化
		spi_master_init(pSpiArgStruct);
		//根据size申请内存，并初始化为全0
		char *pSendBuffer = (char *)xy_malloc(pSpiArgStruct -> SpiTestSize + 1);
		char *pRecvBuffer = (char *)xy_malloc(pSpiArgStruct -> SpiTestSize + 1);
        memset(pSendBuffer, 0, pSpiArgStruct -> SpiTestSize + 1);
        memset(pRecvBuffer, 0, pSpiArgStruct -> SpiTestSize + 1);
		//初始化pSendBuffer
		for(uint16_t i = 0; i < pSpiArgStruct -> SpiTestSize; i++)
		{
			//i为64整数倍的时候，不要赋值'\0'
			((i % 64 == 0) ? *(pSendBuffer + i) = 0x01 : (*(pSendBuffer + i) = i % 64));
		}

		/*****************************主机发送初始化参数给从机*********************************/
		GetCmdDataToSlave(CmdData, (uint32_t)test_SPI, sizeof(SpiArgStruct), InputDebugLevel, pSpiArgStruct);
		HAL_CSP_Transmit(&MasterSlave_UART_Handle, (uint8_t *)CmdData, sizeof(SpiArgStruct) + 12, TRANSMIT_DEFAULT_MAX_DELAY);

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

    	/*************************主机spi开始向从机发送数据并接收"************************/
		if(pSpiArgStruct -> SpiTestSize <= 128)
		{
			if(i % 2 == 0)
			{
				/*******主机阻塞发送*******/
				Debug_Print_Str(DEBUG_LEVEL_1, "\r\nspi主机阻塞发送");
				Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pSendBuffer, pSpiArgStruct->SpiTestSize);
				
				HAL_Delay(5); //确保从机先等待OK
				SEND_OK;
				HAL_Delay(5); //确保从机先进入接收API

				HAL_SPI_ResetCS(HAL_SPI_CS0);
				ret = HAL_SPI_Transmit(&spi_master_handle, (uint8_t *)pSendBuffer, pSpiArgStruct->SpiTestSize, TRANSMIT_DEFAULT_MAX_DELAY);
				HAL_SPI_SetCS(HAL_SPI_CS0);
				if(ret != HAL_OK)
				{
					CuFail(tc, "tramsmit test failed");
				}

				/*******主机阻塞接收*******/
            	Debug_Print_Str(DEBUG_LEVEL_1, "\r\nspi主机阻塞接收");

				WAIT_OK;
				HAL_Delay(10); //确保从机先进入发送API

				HAL_SPI_ResetCS(HAL_SPI_CS0);
				ret = HAL_SPI_Receive(&spi_master_handle, (uint8_t *)pRecvBuffer, pSpiArgStruct->SpiTestSize, RECEIVE_DEFAULT_MAX_DELAY);
				HAL_SPI_SetCS(HAL_SPI_CS0);
				if(ret != HAL_OK)
				{
					CuFail(tc, "receive test failed");
				}
				Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pSpiArgStruct -> SpiTestSize);
			}
			else
			{
				/*******主机非阻塞发送*******/
				Debug_Print_Str(DEBUG_LEVEL_1, "\r\nspi主机非阻塞发送");
				Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pSendBuffer, pSpiArgStruct->SpiTestSize);

				HAL_Delay(5); //确保从机先等待OK
				SEND_OK;
				HAL_Delay(5); //确保从机先进入接收API

				HAL_SPI_ResetCS(HAL_SPI_CS0);
				ret = HAL_SPI_Transmit_IT(&spi_master_handle, (uint8_t *)pSendBuffer, pSpiArgStruct->SpiTestSize);
				while(spi_master_tx_flag != 1){;}
				spi_master_tx_flag = 0;
				HAL_SPI_SetCS(HAL_SPI_CS0);
				if(ret != HAL_OK)
				{
					CuFail(tc, "tramsmit test failed");
				}

				/*******主机非阻塞接收*******/
				Debug_Print_Str(DEBUG_LEVEL_1, "\r\nspi主机非阻塞接收");

				WAIT_OK;
				HAL_Delay(10); //确保从机先进入发送API

				HAL_SPI_ResetCS(HAL_SPI_CS0);
				ret = HAL_SPI_Receive_IT(&spi_master_handle, (uint8_t *)pRecvBuffer, pSpiArgStruct->SpiTestSize);
				while(spi_master_rx_flag != 1){;}
				spi_master_rx_flag = 0;
				HAL_SPI_SetCS(HAL_SPI_CS0);
				if(ret != HAL_OK)
				{
					CuFail(tc, "receive test failed");
				}
				Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pSpiArgStruct->SpiTestSize);
			}
		}
		else
		{
			/**********主机阻塞发送接收**********/
			Debug_Print_Str(DEBUG_LEVEL_1, "\r\nspi主机阻塞发送接收");

			HAL_Delay(5); //确保从机先等待OK
			SEND_OK;
			HAL_Delay(30); //确保从机先进入发送接收API

			HAL_SPI_ResetCS(HAL_SPI_CS0);
			ret = HAL_SPI_Master_TransmitReceive(&spi_master_handle, (uint8_t *)pSendBuffer, (uint8_t *)pRecvBuffer, pSpiArgStruct -> SpiTestSize, TRANSMIT_DEFAULT_MAX_DELAY);
			HAL_SPI_SetCS(HAL_SPI_CS0);
			if(ret != HAL_OK)
			{
				CuFail(tc, "tramsmit & receive failed");
			}
			Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pSendBuffer, pSpiArgStruct->SpiTestSize);
			Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pSpiArgStruct->SpiTestSize);
		}

//		HAL_Delay(5); //确保从机先等待OK
		Debug_Delay(DEBUG_LEVEL_2, 10, 1000);
		SEND_OK;
		HAL_Delay(5); //确保从机先进入从机接收状态

		HAL_SPI_Drivertest_DeInit(&spi_master_handle);

		Debug_Print_Str(DEBUG_LEVEL_2, "\r\n%dth sendlen:%d recvlen:%d\r\n", i + 1, strlen(pSendBuffer), strlen(pRecvBuffer));

		CuAssertStrEquals(tc, pSendBuffer, pRecvBuffer);

        xy_free(pSendBuffer);
        xy_free(pRecvBuffer);
	}
}

/******************************************************************************************/
void spi_master_test(CuTest* tc)
{	
	Debug_Print_Str(DEBUG_LEVEL_0, "\r\n<spi> test begin\r\n");

	uint32_t CaseNum = 1;

	InitArgStruct(&SpiArgStruct_Demo, SpiMosiPinArray, SpiMisoPinArray, SpiSclkPinArray, SpiNss1PinArray, SpiInstanceArray, \
									  SpiWorkModeArray, SpiClkDivArray, SpiTestSizeArray, MaxArgArray);
	while(GetArgStruct(&SpiArgStruct_Demo) != 0)
	{
		Debug_Print_Str(DEBUG_LEVEL_1, "\r\n->spi case:%lu/%lu\r\n", CaseNum, GetTotalCaseNun());

		CaseNum++;

		Debug_Print_ArgStruct(DEBUG_LEVEL_1, &SpiArgStruct_Demo);

		spi_test(tc, &SpiArgStruct_Demo, TestTimes);
	}

	DeInitArgStruct(&SpiArgStruct_Demo);

	Debug_Print_Str(DEBUG_LEVEL_0, "\r\n<spi> test end\r\n");
}

#endif /* #if (DRIVER_TEST == 1) */
