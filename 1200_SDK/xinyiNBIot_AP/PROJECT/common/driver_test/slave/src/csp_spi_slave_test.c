/**
 * @file  csp_spi_slave_test.c
 * @brief 从测试设备代码
 * @version 0.1
 * @date 2021-08-25
 */
#include <string.h>
#include "slave.h"
#include "hal_gpio.h"
#include "hal_csp.h"

#if (DRIVER_TEST == 2)

/******************************************************************************************/
/**
 * @brief CSP从机的宏定义
 * 
 */

#if (MasterSlave_UART == 1) //若主从通道选择CSP1,则CSP2参与驱动测试

#define GPIO_CSP_TESTSEL_TXD	GPIO_CSP2_TXD
#define GPIO_CSP_TESTSEL_RXD	GPIO_CSP2_RXD 
#define GPIO_CSP_TESTSEL_SCLK	GPIO_CSP2_SCLK
#define GPIO_CSP_TESTSEL_RFS	GPIO_CSP2_RFS
#define HAL_CSP_TESTSEL			HAL_CSP2

#elif (MasterSlave_UART == 2)	//若主从通道选择CSP2,则CSP1参与驱动测试

#define GPIO_CSP_TESTSEL_TXD	GPIO_CSP1_TXD
#define GPIO_CSP_TESTSEL_RXD	GPIO_CSP1_RXD 
#define GPIO_CSP_TESTSEL_SCLK	GPIO_CSP1_SCLK
#define GPIO_CSP_TESTSEL_RFS	GPIO_CSP1_RFS
#define HAL_CSP_TESTSEL			HAL_CSP1

#endif

#define HAL_CSP_SPI_MODE_SLAVE	CSP_MODE1_CLOCK_MODE_Slave    /*!< CSP_SPI从机模式 */

/**
 * @brief  初始化CSP为从机SPI.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 */
HAL_StatusTypeDef HAL_CSP_SPI_Slave_Init(HAL_CSP_HandleTypeDef *hcsp)
{
	uint32_t pclk = 0;

	assert_param(IS_CSP_INSTANCE(hcsp->Instance));

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
	if((hcsp->CSP_SPI_Init.MasterSlave == HAL_CSP_SPI_MODE_SLAVE) && (hcsp->CSP_SPI_Init.Speed > pclk/16))
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
	// if(hcsp->CSP_SPI_Init.MasterSlave == HAL_CSP_SPI_MODE_SLAVE)
    // {
    //     CSP_RFS_PIN_MODE(hcsp->Instance, 1); //RFS引脚为IO模式
    //     CSP_RFS_ACT_LEVEL(hcsp->Instance, 0);//RFS引脚有效电平为低电平
    //     CSP_RFS_IO_MODE(hcsp->Instance, 1);  //RFS引脚为输入
	// 	CSP_RFS_PIN_VALUE(hcsp->Instance, 1);//RFS引脚设置为高电平，即处于无效状态
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

/**
 * @brief  CSP_SPI从机阻塞发送接收API. 注意！！！该接口只能用于CSP_SPI从机.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @param  pTxData 发送缓冲区指针
 * @param  pRxData 接收缓冲区指针
 * @param  Size 发送和接收数据字节长度
 * @param  Timeout：超时时间（单位ms）;
 *         如果Timeout为HAL_MAX_DELAY，则是一直阻塞，直到发送和接收到指定数量的字节后才退出;
 * 		   如果Timeout为非0非HAL_MAX_DELAY时，表示函数等待数据发送接收完成的最长时间，超时则退出函数.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：发送和接收数据成功，表示指定时间内成功发送和接收指定数量的数据
 *         @retval  HAL_ERROR    ：入参错误
 *         @retval  HAL_BUSY     ：外设正在使用中
 *         @retval  HAL_TIMEOUT  ：表示指定时间内未能成功发送和接收指定数量的数据
 * @note   hcsp结构体中的TxXferCount表示实际发送的字节数，RxXferCount表示实际接收的字节数.
 * 
 * @attention  1、CSP_SPI从机阻塞发送接收API. 注意！！！该接口只能用于CSP_SPI从机.
 * 			   2、驱动测试时，此接口与hal_csp.c中的HAL_CSP_SPI_TransmitReceive(位于ram上)配套使用，需要将其放在ram上。
 */
__RAM_FUNC HAL_StatusTypeDef HAL_CSP_SPI_Slave_TransmitReceive(HAL_CSP_HandleTypeDef *hcsp, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0;
	HAL_StatusTypeDef errorcode = HAL_OK;

	if (hcsp->gState == HAL_CSP_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hcsp->gState != HAL_CSP_STATE_READY)
    {
		return HAL_BUSY;
	}

	if ((pTxData == NULL) || (pRxData == NULL) || (Size == 0U) || (Timeout == 0))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK(hcsp);

	tickstart = Get_Tick();

	hcsp->RxState = HAL_CSP_STATE_BUSY;
	hcsp->ErrorCode = HAL_CSP_ERROR_NONE;
    hcsp->pTxBuffPtr = xy_malloc(sizeof(uint8_t) * Size);
    if(hcsp->pTxBuffPtr == NULL)
    {
		return HAL_ERROR;
    }
    memcpy(hcsp->pTxBuffPtr, pTxData, Size);
    hcsp->TxXferSize = Size;
	hcsp->TxXferCount = 0;
	hcsp->pRxBuffPtr = (uint8_t *)pRxData;
	hcsp->RxXferSize = Size;
	hcsp->RxXferCount = 0;

	//清除TXFIFO与RXFIFO
	CSP_TXFifoClear(hcsp->Instance);
	CSP_RXFifoClear(hcsp->Instance);

    //先发送再接收数据
	while ((hcsp->TxXferSize > 0) || (hcsp->RxXferSize > 0))
	{
		//发送数据
        if ((!(hcsp->Instance->TX_FIFO_STATUS & CSP_TX_FIFO_STATUS_FULL_Msk)) && (hcsp->TxXferSize > 0U))
        {
            hcsp->Instance->TX_FIFO_DATA = *(hcsp->pTxBuffPtr + hcsp->TxXferCount);
            hcsp->TxXferSize--;
            hcsp->TxXferCount++;
        }
        else
        {
            if ((Timeout != HAL_MAX_DELAY) && Check_Ms_Timeout(tickstart,Timeout))
            {
                errorcode = HAL_TIMEOUT;
                goto error;
            }
        }

		//接收数据
        if((!(hcsp->Instance->RX_FIFO_STATUS & CSP_RX_FIFO_STATUS_EMPTY_Msk)) && (hcsp->RxXferSize > 0U))
        {
            *(hcsp->pRxBuffPtr) = (uint8_t)(hcsp->Instance->RX_FIFO_DATA);
            hcsp->pRxBuffPtr++;
            hcsp->RxXferSize--;
            hcsp->RxXferCount++;
        }
        else
        {
            if ((Timeout != HAL_MAX_DELAY) && Check_Ms_Timeout(tickstart,Timeout))
            {
                errorcode = HAL_TIMEOUT;
                goto error;
            }
        }
	}

error:
	hcsp->RxState = HAL_CSP_STATE_READY;

	xy_free(hcsp->pTxBuffPtr);

	__HAL_UNLOCK(hcsp);

	return errorcode;
}

/******************************************************************************************/
HAL_CSP_HandleTypeDef csp_spi_slave_handle = {0};

static void csp_spi_slave_init(CspSpiArgStruct *pCspSpiArgStruct)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};

	if( pCspSpiArgStruct->CspSpiInstance == (uint32_t)HAL_CSP_TESTSEL)
	{
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspSpiArgStruct->CspSpiMosiPin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP_TESTSEL_RXD;//映射SPI的MOSI引脚
		HAL_GPIO_Init(&gpio_init);
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspSpiArgStruct->CspSpiMisoPin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP_TESTSEL_TXD;//映射SPI的MISO引脚
		HAL_GPIO_Init(&gpio_init);
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspSpiArgStruct->CspSpiSclkPin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP_TESTSEL_SCLK;//映射SPI的SCLK引脚
		HAL_GPIO_Init(&gpio_init);
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspSpiArgStruct->CspSpiNss1Pin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP_TESTSEL_RFS;//映射SPI的NSS引脚
		HAL_GPIO_Init(&gpio_init);
	}
	else if( pCspSpiArgStruct->CspSpiInstance == (uint32_t)HAL_CSP3)
	{
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspSpiArgStruct->CspSpiMosiPin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP3_RXD;//映射SPI的MOSI引脚
		HAL_GPIO_Init(&gpio_init);
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspSpiArgStruct->CspSpiMisoPin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP3_TXD;//映射SPI的MISO引脚
		HAL_GPIO_Init(&gpio_init);
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspSpiArgStruct->CspSpiSclkPin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP3_SCLK;//映射SPI的SCLK引脚
		HAL_GPIO_Init(&gpio_init);
		gpio_init.Pin      = (HAL_GPIO_PinTypeDef)pCspSpiArgStruct->CspSpiNss1Pin;
		gpio_init.Mode     = GPIO_MODE_HW_PER;
		gpio_init.PinRemap = GPIO_CSP3_RFS;//映射SPI的NSS引脚
		HAL_GPIO_Init(&gpio_init);
	}

	csp_spi_slave_handle.Instance                 = (CSP_TypeDef *)pCspSpiArgStruct->CspSpiInstance;
	csp_spi_slave_handle.CSP_SPI_Init.MasterSlave = HAL_CSP_SPI_MODE_SLAVE;
	csp_spi_slave_handle.CSP_SPI_Init.WorkMode    = pCspSpiArgStruct->CspSpiWorkMode;
	csp_spi_slave_handle.CSP_SPI_Init.Speed	      = pCspSpiArgStruct -> CspSpiClock;
	HAL_CSP_SPI_Slave_Init(&csp_spi_slave_handle);
}

/******************************************************************************************/
void csp_spi_slave_test(CspSpiArgStruct *pCspSpiArgStruct)
{
	HAL_StatusTypeDef ret = HAL_OK;
	char RecvConfigBuffer[10] = {0};
	static uint32_t TestTimesFlag = 0;
	TestTimesFlag++;

	Debug_Print_Str(DEBUG_LEVEL_2, "\r\n<csp_spi> test begin\r\n");
	Debug_Print_ArgStruct(DEBUG_LEVEL_1, pCspSpiArgStruct);

    /*************************从机初始化并准备接收缓存区****************************/
	//csp_spi从机初始化
	csp_spi_slave_init(pCspSpiArgStruct);
    //根据size申请内存，并初始化为全0
	char *pRecvBuffer = (char *)xy_malloc(pCspSpiArgStruct->CspSpiTestSize);
	char *pSendBuffer = (char *)xy_malloc(pCspSpiArgStruct->CspSpiTestSize);
	memset(pRecvBuffer, 0, pCspSpiArgStruct->CspSpiTestSize);
	memset(pSendBuffer, 0, pCspSpiArgStruct->CspSpiTestSize);
	//初始化pSendBuffer
	for (uint16_t i = 0; i < pCspSpiArgStruct->CspSpiTestSize; i++)
	{
		// i为64整数倍的时候，不要赋值'\0'
		((i % 64 == 0) ? *(pSendBuffer + i) = 0x01 : (*(pSendBuffer + i) = i % 64));
	}

	/***********************回应"OK\n"************************/
	SEND_OK;

	/***********************CSP_SPI从机接收发送"************************/
	if(pCspSpiArgStruct->CspSpiTestSize <= 128)
	{
		/**********从机阻塞接收**********/
		Debug_Print_Str(DEBUG_LEVEL_1, "\r\ncsp_spi从机阻塞接收");

		WAIT_OK;

		ret = HAL_CSP_Receive(&csp_spi_slave_handle, (uint8_t *)pRecvBuffer, pCspSpiArgStruct->CspSpiTestSize, RECEIVE_DEFAULT_MAX_DELAY);
		if(ret != HAL_OK)
		{
			return;
		}
		Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pCspSpiArgStruct->CspSpiTestSize);

		/**********从机阻塞发送**********/
		Debug_Print_Str(DEBUG_LEVEL_1, "\r\ncsp_spi从机阻塞发送");
		Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pCspSpiArgStruct->CspSpiTestSize);

		HAL_Delay(5); //确保主机先等待OK
		SEND_OK;

		ret = HAL_CSP_Transmit(&csp_spi_slave_handle, (uint8_t *)pRecvBuffer, pCspSpiArgStruct->CspSpiTestSize, TRANSMIT_DEFAULT_MAX_DELAY);
		if(ret != HAL_OK)
		{
			return;
		}
	}
	else
	{
		/**********从机阻塞发送接收**********/
		Debug_Print_Str(DEBUG_LEVEL_1, "\r\ncsp_spi从机阻塞发送接收");

		WAIT_OK;

		ret = HAL_CSP_SPI_Slave_TransmitReceive(&csp_spi_slave_handle, (uint8_t *)pSendBuffer, (uint8_t *)pRecvBuffer, pCspSpiArgStruct->CspSpiTestSize, TRANSMIT_DEFAULT_MAX_DELAY);
		if(ret != HAL_OK)
		{
			return;
		}
		Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pSendBuffer, pCspSpiArgStruct->CspSpiTestSize);
		Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pCspSpiArgStruct->CspSpiTestSize);
	}

	WAIT_OK;

	HAL_CSP_Drivertest_DeInit(&csp_spi_slave_handle);

	xy_free(pSendBuffer);
	xy_free(pRecvBuffer);

	Debug_Print_Str(DEBUG_LEVEL_2, "\r\n<csp_spi> test end\r\n");
}

#endif	/* #if (DRIVER_TEST == 2) */
