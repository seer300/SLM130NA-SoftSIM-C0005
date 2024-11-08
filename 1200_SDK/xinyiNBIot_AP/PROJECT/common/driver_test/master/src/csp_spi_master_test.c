/**
 * @file csp_spi_master_test.c
 * @brief 主设备测试代码
 * @version 0.1
 * @version 0.2
 * @date 2022-10-24
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
#define GPIO_CSP_TESTSEL_SCLK	GPIO_CSP2_SCLK
#define GPIO_CSP_TESTSEL_TFS	GPIO_CSP2_TFS
#define HAL_CSP_TESTSEL			HAL_CSP2

#elif (MasterSlave_UART == 2)	//若主从通道选择CSP2,则CSP1参与驱动测试

#define GPIO_CSP_TESTSEL_TXD	GPIO_CSP1_TXD
#define GPIO_CSP_TESTSEL_RXD	GPIO_CSP1_RXD 
#define GPIO_CSP_TESTSEL_SCLK	GPIO_CSP1_SCLK
#define GPIO_CSP_TESTSEL_TFS	GPIO_CSP1_TFS
#define HAL_CSP_TESTSEL			HAL_CSP1

#endif

/******************************************************************************************/
CspSpiArgStruct CspSpiArgStruct_Demo = {0};
uint32_t CspSpiMosiPinArray[] = {GPIO_PAD_NUM_9, MaxArg};
uint32_t CspSpiMisoPinArray[] = {GPIO_PAD_NUM_10, MaxArg};
uint32_t CspSpiSclkPinArray[] = {GPIO_PAD_NUM_5, MaxArg};
uint32_t CspSpiNss1PinArray[] = {GPIO_PAD_NUM_6, MaxArg};
uint32_t  CspSpiInstanceArray[] = {(uint32_t)HAL_CSP_TESTSEL,/*HAL_CSP3,*/MaxArg};//主从机通信会占用掉一个CSP,HAL_CSP3用于CP侧log口，测试时需要保证CP侧log未开启
uint32_t CspSpiWorkModeArray[] = {HAL_CSP_SPI_WORKMODE_0, HAL_CSP_SPI_WORKMODE_1, HAL_CSP_SPI_WORKMODE_2, HAL_CSP_SPI_WORKMODE_3, MaxArg};
uint32_t CspSpiClockArray[] = {HAL_CSP_SPI_SPEED_100K,HAL_CSP_SPI_SPEED_500K, HAL_CSP_SPI_SPEED_1M, HAL_CSP_SPI_SPEED_2M, HAL_CSP_SPI_SPEED_3M,HAL_CSP_SPI_SPEED_4M,5000000,HAL_CSP_SPI_SPEED_6M,MaxArg}; //注意从机速率不得超过pclk/16
uint32_t CspSpiTestSizeArray[] = {1, 2, 3, 4, 125, 126, 127, 128, 512, 513, 514, 515, MaxArg};

/**
 * @brief  初始化CSP为SPI.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 */
HAL_StatusTypeDef HAL_CSP_SPI_Master_Init(HAL_CSP_HandleTypeDef *hcsp)
{
	uint32_t pclk = 0;

	assert_param(IS_CSP_INSTANCE(hcsp->Instance));
	assert_param(IS_CSP_SPI_MASTERSLAVE(hcsp->CSP_SPI_Init.MasterSlave));
	assert_param(IS_CSP_SPI_WORKMODE(hcsp->CSP_SPI_Init.WorkMode));

	if (hcsp == NULL)
	{
		return HAL_ERROR;
	}

	if (hcsp->gState == HAL_CSP_STATE_RESET)
	{
		hcsp->Lock = HAL_UNLOCKED;

        //使能CSP时钟
		if (hcsp->Instance == HAL_CSP1)
		{
			PRCM_ClockEnable(CORE_CKG_CTL_CSP1_EN);
		}
		else if (hcsp->Instance == HAL_CSP2)
		{
			PRCM_ClockEnable(CORE_CKG_CTL_CSP2_EN);
		}
		else if (hcsp->Instance == HAL_CSP3)
		{
			PRCM_ClockEnable(CORE_CKG_CTL_CSP3_EN);
		}
	}
	else
	{
		return HAL_ERROR;
	}
	//CSP外设时钟源选择
	if (hcsp->Instance == HAL_CSP1)
	{
		pclk = GetlsioFreq();
	}
	else if ((hcsp->Instance == HAL_CSP2) || (hcsp->Instance == HAL_CSP3))
	{
		pclk = GetPCLK2Freq();
	}

    //入参要保证速率不超过上限，超过则返回
	if((hcsp->CSP_SPI_Init.MasterSlave == HAL_CSP_SPI_MODE_MASTER) && (hcsp->CSP_SPI_Init.Speed > pclk/2))
	{
		return HAL_ERROR;
	}

	hcsp->gState = HAL_CSP_STATE_BUSY;

    //关闭所有中断源
	hcsp->Instance->INT_ENABLE &= ~CSP_INT_ALL;

	//配置CSP_SPI，默认字宽为8bit
	CSP_Disable(hcsp->Instance);
	switch (hcsp->CSP_SPI_Init.WorkMode)
	{
        case HAL_CSP_SPI_WORKMODE_0:
        {
	        CSP_SPIConfigSetExpClk(hcsp->Instance, pclk, hcsp->CSP_SPI_Init.Speed, hcsp->CSP_SPI_Init.MasterSlave, 0, 0, 8);
            break;
        }
        case HAL_CSP_SPI_WORKMODE_1:
        {
            CSP_SPIConfigSetExpClk(hcsp->Instance, pclk, hcsp->CSP_SPI_Init.Speed, hcsp->CSP_SPI_Init.MasterSlave, 0, 1, 8);
            break;
        }
        case HAL_CSP_SPI_WORKMODE_2:
        {
	        CSP_SPIConfigSetExpClk(hcsp->Instance, pclk, hcsp->CSP_SPI_Init.Speed, hcsp->CSP_SPI_Init.MasterSlave, 1, 0, 8);
            break;
        }
        case HAL_CSP_SPI_WORKMODE_3:
        {
            CSP_SPIConfigSetExpClk(hcsp->Instance, pclk, hcsp->CSP_SPI_Init.Speed, hcsp->CSP_SPI_Init.MasterSlave, 1, 1, 8);
            break;
        }
        default: return HAL_ERROR;
	}
    // if(hcsp->CSP_SPI_Init.MasterSlave == HAL_CSP_SPI_MODE_MASTER)
    // {
    //     CSP_TFS_PIN_MODE(hcsp->Instance, 1); //TFS引脚为IO模式
    //     CSP_TFS_ACT_LEVEL(hcsp->Instance, 0);//TFS引脚有效电平为低电平
    //     CSP_TFS_IO_MODE(hcsp->Instance, 0);  //TFS引脚为输出
    //     HAL_CSP_SPI_SetCS(hcsp);             //TFS引脚设置为高电平，即处于无效状态
    // }
	CSP_RxEnable(hcsp->Instance);
	CSP_TxEnable(hcsp->Instance);
	CSP_FifoReset(hcsp->Instance);
	CSP_FifoStart(hcsp->Instance);
	CSP_Enable(hcsp->Instance);

	hcsp->ErrorCode = HAL_CSP_ERROR_NONE;
	hcsp->gState = HAL_CSP_STATE_READY;
	hcsp->RxState = HAL_CSP_STATE_READY;

	return HAL_OK;
}

/******************************************************************************************/
HAL_CSP_HandleTypeDef csp_spi_master_handle = {0};

static uint8_t csp_spi_master_init(CspSpiArgStruct *pCspSpiArgStruct)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};

	if( pCspSpiArgStruct->CspSpiInstance == (uint32_t)HAL_CSP_TESTSEL)
	{
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspSpiArgStruct->CspSpiMosiPin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP_TESTSEL_TXD;//映射SPI的MOSI引脚
		HAL_GPIO_Init(&gpio_init);
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspSpiArgStruct->CspSpiMisoPin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP_TESTSEL_RXD;//映射SPI的MISO引脚
		HAL_GPIO_Init(&gpio_init);
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspSpiArgStruct->CspSpiSclkPin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP_TESTSEL_SCLK;//映射SPI的SCLK引脚
		HAL_GPIO_Init(&gpio_init);
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspSpiArgStruct->CspSpiNss1Pin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP_TESTSEL_TFS;//映射SPI的NSS引脚
		HAL_GPIO_Init(&gpio_init);
	}
	else if( pCspSpiArgStruct->CspSpiInstance == (uint32_t)HAL_CSP3)
	{
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspSpiArgStruct->CspSpiMosiPin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP3_TXD;//映射SPI的MOSI引脚
		HAL_GPIO_Init(&gpio_init);
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspSpiArgStruct->CspSpiMisoPin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP3_RXD;//映射SPI的MISO引脚
		HAL_GPIO_Init(&gpio_init);
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspSpiArgStruct->CspSpiSclkPin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP3_SCLK;//映射SPI的SCLK引脚
		HAL_GPIO_Init(&gpio_init);
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspSpiArgStruct->CspSpiNss1Pin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP3_TFS;//映射SPI的NSS引脚
		HAL_GPIO_Init(&gpio_init);
	}

	csp_spi_master_handle.Instance                 = (CSP_TypeDef *)pCspSpiArgStruct->CspSpiInstance;
    csp_spi_master_handle.CSP_SPI_Init.MasterSlave = HAL_CSP_SPI_MODE_MASTER;
	csp_spi_master_handle.CSP_SPI_Init.WorkMode    = pCspSpiArgStruct->CspSpiWorkMode;
	csp_spi_master_handle.CSP_SPI_Init.Speed       = pCspSpiArgStruct->CspSpiClock;
	if(HAL_CSP_SPI_Master_Init(&csp_spi_master_handle) == HAL_OK)
		return 1;
	else 
		return 0;

}

/******************************************************************************************/
static void csp_spi_test(CuTest *tc, CspSpiArgStruct *pCspSpiArgStruct, uint32_t TestTimes)
{
	HAL_StatusTypeDef ret = HAL_OK;
	char CmdData[100] = {0};
	char RecvConfigBuffer[10] = {0};

	for (uint16_t i = 0; i < TestTimes; i++)
	{
		Debug_Print_Str(DEBUG_LEVEL_1, "\r\ntest times:%d/%d\r\n", i+1, TestTimes);

			//CSP外设时钟源选择
		uint32_t pclk=0;
		if (pCspSpiArgStruct->CspSpiInstance == (uint32_t)HAL_CSP1)
		{
			pclk = GetlsioFreq();
		}
		else if ((pCspSpiArgStruct->CspSpiInstance == (uint32_t)HAL_CSP2) || (pCspSpiArgStruct->CspSpiInstance == (uint32_t)HAL_CSP3))
		{
			pclk = GetPCLK2Freq();
		}

		if((pCspSpiArgStruct->CspSpiClock > pclk/16)) //SPI测试时，主机速度不可超过pclk/2,从机速度不可超过pclk/16
		{
			Debug_Print_Str(DEBUG_LEVEL_1, "\r\nThese parameters are not supported:\r\n");
			continue;
		}
        /*************************主机初始化并准备收发缓存区****************************/
		//主机CSP_SPI初始化
		if(csp_spi_master_init(pCspSpiArgStruct) == 0)
		{
			Debug_Print_Str(DEBUG_LEVEL_1, "\r\ncsp_spi_master_init fail,the speed is too high\r\n");
			continue;
		}

		//根据size申请内存，并初始化为全0
		char *pSendBuffer = (char *)xy_malloc(pCspSpiArgStruct->CspSpiTestSize + 1);
		char *pRecvBuffer = (char *)xy_malloc(pCspSpiArgStruct->CspSpiTestSize + 1);
		memset(pSendBuffer, 0, pCspSpiArgStruct->CspSpiTestSize + 1);
		memset(pRecvBuffer, 0, pCspSpiArgStruct->CspSpiTestSize + 1);
		//初始化pSendBuffer
		for (uint16_t i = 0; i < pCspSpiArgStruct->CspSpiTestSize; i++)
		{
			// i为64整数倍的时候，不要赋值'\0'
			((i % 64 == 0) ? *(pSendBuffer + i) = 0x01 : (*(pSendBuffer + i) = i % 64));
		}

		/*****************************主机发送初始化参数给从机*********************************/
		GetCmdDataToSlave(CmdData, (uint32_t)test_CSP_SPI, sizeof(CspSpiArgStruct), InputDebugLevel, pCspSpiArgStruct);
		HAL_CSP_Transmit(&MasterSlave_UART_Handle, (uint8_t *)CmdData, sizeof(CspSpiArgStruct) + 12, TRANSMIT_DEFAULT_MAX_DELAY);

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

    	/*************************主机csp_spi开始向从机发送数据并接收"************************/
		if(pCspSpiArgStruct->CspSpiTestSize <= 128)
		{
			/*******主机阻塞发送*******/
			Debug_Print_Str(DEBUG_LEVEL_1, "\r\ncsp_spi主机阻塞发送");
			Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pSendBuffer, pCspSpiArgStruct->CspSpiTestSize);
			
			HAL_Delay(5); //确保从机先等待OK
			SEND_OK;
			HAL_Delay(10); //确保从机先进入接收API

			ret = HAL_CSP_Transmit(&csp_spi_master_handle, (uint8_t *)pSendBuffer, pCspSpiArgStruct->CspSpiTestSize, TRANSMIT_DEFAULT_MAX_DELAY);
			if(ret != HAL_OK)
			{
				CuFail(tc, "tramsmit test failed");
			}

			/*******主机阻塞接收*******/
			Debug_Print_Str(DEBUG_LEVEL_1, "\r\ncsp_spi主机阻塞接收");

			WAIT_OK;
			HAL_Delay(10); //确保从机先进入发送API

			ret = HAL_CSP_Receive(&csp_spi_master_handle, (uint8_t *)pRecvBuffer, pCspSpiArgStruct->CspSpiTestSize, RECEIVE_DEFAULT_MAX_DELAY);
			if(ret != HAL_OK)
			{
				CuFail(tc, "receive test failed");
			}
			Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pCspSpiArgStruct->CspSpiTestSize);
		}
		else
		{
			/**********主机阻塞发送接收**********/
			Debug_Print_Str(DEBUG_LEVEL_1, "\r\ncsp_spi主机阻塞发送接收");

			HAL_Delay(5); //确保从机先等待OK
			SEND_OK;
			HAL_Delay(10); //确保从机先进入发送接收API

			ret = HAL_CSP_SPI_TransmitReceive(&csp_spi_master_handle, (uint8_t *)pSendBuffer, (uint8_t *)pRecvBuffer, pCspSpiArgStruct->CspSpiTestSize, TRANSMIT_DEFAULT_MAX_DELAY);
			if(ret != HAL_OK)
			{
				CuFail(tc, "tramsmit & receive failed");
			}
			Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pSendBuffer, pCspSpiArgStruct->CspSpiTestSize);
			Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pCspSpiArgStruct->CspSpiTestSize);
		}

//		HAL_Delay(5); //确保从机先等待OK
		Debug_Delay(DEBUG_LEVEL_2, 10, 1000);
		SEND_OK;
		HAL_Delay(5); //确保从机先进入从机接收状态

		HAL_CSP_Drivertest_DeInit(&csp_spi_master_handle);

		Debug_Print_Str(DEBUG_LEVEL_2, "\r\n%dth sendlen:%d recvlen:%d\r\n", i, strlen(pSendBuffer), strlen(pRecvBuffer));

		CuAssertStrEquals(tc, pSendBuffer, pRecvBuffer);

		xy_free(pSendBuffer);
		xy_free(pRecvBuffer);
	}
}

/******************************************************************************************/
void csp_spi_master_test(CuTest *tc)
{
	Debug_Print_Str(DEBUG_LEVEL_0, "\r\n<csp spi> test begin\r\n");

	uint32_t CaseNum = 1;

	InitArgStruct(&CspSpiArgStruct_Demo, CspSpiMosiPinArray, CspSpiMisoPinArray, CspSpiSclkPinArray, CspSpiNss1PinArray, CspSpiInstanceArray,\
				                         CspSpiWorkModeArray, CspSpiClockArray, CspSpiTestSizeArray, MaxArgArray);
	while (GetArgStruct(&CspSpiArgStruct_Demo) != 0)
	{
		Debug_Print_Str(DEBUG_LEVEL_1, "\r\ncsp spi case:%lu/%lu\r\n", CaseNum, GetTotalCaseNun());

		CaseNum++;

		Debug_Print_ArgStruct(DEBUG_LEVEL_1, &CspSpiArgStruct_Demo);

		csp_spi_test(tc, &CspSpiArgStruct_Demo, TestTimes);
	}

	DeInitArgStruct(&CspSpiArgStruct_Demo);

	Debug_Print_Str(DEBUG_LEVEL_0, "\r\n<csp spi> test end\r\n");
}

#endif /* #if (DRIVER_TEST == 1) */

