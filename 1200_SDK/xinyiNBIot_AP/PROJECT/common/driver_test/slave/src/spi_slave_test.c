/**
 * @file spi_slave_test.c
 * @brief 从测试设备代码
 * @version 0.2
 * @date 2022-10-17
 */
#include <string.h>
#include "slave.h"
#include "hal_gpio.h"
#include "hal_spi.h"

#if (DRIVER_TEST == 2)

/******************************************************************************************/
/**
 * @brief SPI从机的宏定义
 * 
 */
#define HAL_SPI_MODE_SLAVE SPI_CONFIG_MODE_SLAVE  /*!< SPI从机模式 */

/**
 * @brief  初始化SPI从机.
 *
 * @param  hspi. 详见结构体定义   @ref HAL_SPI_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 */
HAL_StatusTypeDef HAL_Slave_SPI_Init(HAL_SPI_HandleTypeDef *hspi)
{
    assert_param(IS_SPI_INSTANCE(hspi->Instance));

	if (hspi == NULL)
    {
		return HAL_ERROR;
    }

	if (hspi->State == HAL_SPI_STATE_RESET)
	{
		hspi->Lock = HAL_UNLOCKED;

        //使能SPI外设时钟
	    PRCM_ClockEnable(CORE_CKG_CTL_SPI_EN);
	}
	else
	{
		return HAL_ERROR;
	}

	hspi->State = HAL_SPI_STATE_BUSY;

    //关闭所有中断源
	hspi->Instance->IDIS |= SPI_INT_ALL;

	if(hspi->Init.MasterSlave == HAL_SPI_MODE_SLAVE)
	{
		GPIO_InputPeriInvertCmd(GPIO_SPI_CS_N, ENABLE);//Slave模式下，在使能SPI 时，CS会产生一个上升沿， 该上升沿会造成发送指针紊乱。

		//失能SPI
        SPI_Disable();

		//配置SPI
		SPI_ConfigSetExpClk(SPI_CONFIG_CLK_DIV_2, hspi->Init.WorkMode, hspi->Init.MasterSlave, SPI_CONFIG_WORD_SIZE_BITS_8);//默认使用8位传输
        SPI_DisManualTransmit();//关闭手动TXFIFO发送
        SPI_SlaveBurstEnable();//SLAVE时传输字符间的CS低有效时,SLAVE参考时钟始终运行

        //清FIFO
        SPI_TxFifoReset();
        SPI_RxFifoReset();

        //使能FIFO
        SPI_TxFifoEnable();
        SPI_RxFifoEnable();

		//使能SPI外设功能
		SPI_Enable();

		GPIO_InputPeriInvertCmd(GPIO_SPI_CS_N, DISABLE);
	}

    hspi->ErrorCode = HAL_SPI_ERROR_NONE;
	hspi->State = HAL_SPI_STATE_READY;

	return HAL_OK;
}

/**
 * @brief  SPI阻塞发送API.
 *
 * @param  hspi. 详见结构体定义  @ref HAL_SPI_HandleTypeDef.
 * @param  pData:发送数据的存储地址.
 * @param  Size:发送数据的大小.
 * @param  Timeout：超时时间（单位ms）;
 *         如果Timeout为HAL_MAX_DELAY，则是一直阻塞，等待发送完指定数量的字节才退出;
 *         如果Timeout为非0非HAL_MAX_DELAY时，表示函数等待数据发送完成的最长时间，超时则退出函数.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval HAL_OK      ：表示发送数据成功，表示指定时间内成功发送指定数量的数据
 *         @retval HAL_ERROR   ：表示入参错误
 *         @retval HAL_BUSY    ：表示外设正在使用中
 *         @retval HAL_TIMEOUT ：表示指定时间内未能成功发送指定数量的数据
 * @note   hspi结构体中的TxXferCount表示实际发送的字节数.
 */
__RAM_FUNC HAL_StatusTypeDef HAL_Slave_SPI_Transmit(HAL_SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0;
	HAL_StatusTypeDef errorcode = HAL_OK;
	uint8_t slave_first_tx = 0;

	if (hspi->State == HAL_SPI_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hspi->State != HAL_SPI_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U) || (Timeout == 0))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK(hspi);

	tickstart = Get_Tick();

	hspi->State = HAL_SPI_STATE_BUSY;
	hspi->ErrorCode = HAL_SPI_ERROR_NONE;
	hspi->pTxBuffPtr = xy_malloc(sizeof(uint8_t) * Size);
	if (hspi->pTxBuffPtr == NULL)
	{
		return HAL_ERROR;
	}
	memcpy(hspi->pTxBuffPtr, pData, Size);
	hspi->TxXferSize = Size;
	hspi->TxXferCount = 0;
	hspi->pRxBuffPtr = (uint8_t *)NULL;
	hspi->RxXferSize = 0U;
	hspi->RxXferCount = 0U;

	//发送数据
	while (hspi->TxXferSize > 0U)
	{
		if (!SPI_TxFifoStatusGet(SPI_FIFO_FULL))
		{
            uint8_t txfifo_len = SPI_TxFifoStatusGet(SPI_FIFO_DATA_LEN);
            if ((hspi->TxXferSize >= 4) && (txfifo_len <= 124))
			{
				hspi->Instance->TXD = *((uint32_t *)(hspi->pTxBuffPtr + hspi->TxXferCount));
				hspi->TxXferSize -= 4;
				hspi->TxXferCount += 4;
			}
			else if ((hspi->TxXferSize >= 2 && hspi->TxXferSize < 4) && (txfifo_len <= 126))
			{
				hspi->Instance->TXD_16 = *((uint16_t *)(hspi->pTxBuffPtr + hspi->TxXferCount));
				hspi->TxXferSize -= 2;
				hspi->TxXferCount += 2;

				//CLK较低时需要等待发送完成，否则倒数第二个数据会被最后一个数据吃掉，导致数据丢失
				if(hspi->Init.MasterSlave == HAL_SPI_MODE_MASTER) 
				{
					while (!SPI_TxFifoStatusGet(SPI_FIFO_EMPTY));
				}
			}
			else if ((hspi->TxXferSize == 1) && (txfifo_len <= 127))
			{
				hspi->Instance->TXD_8 = *((uint8_t *)(hspi->pTxBuffPtr + hspi->TxXferCount));
				hspi->TxXferSize--;
				hspi->TxXferCount++;
			}
		}
		else
		{
			if ((Timeout != HAL_MAX_DELAY) && Check_Ms_Timeout(tickstart,Timeout))
			{
				errorcode = HAL_TIMEOUT;
				goto error;
			}
		}

		//SPI从机的TXFIFO首次从空至非空需要重置SPI内部计数器以保证数据发送正确，通过使CS引脚产生下降沿可重置SPI内部计数器。
		if((hspi->Init.MasterSlave == HAL_SPI_MODE_SLAVE) && (slave_first_tx == 0))
		{
			GPIO_InputPeriInvertCmd(GPIO_SPI_CS_N, ENABLE);
			GPIO_InputPeriInvertCmd(GPIO_SPI_CS_N, DISABLE);
			slave_first_tx = 1;
		}
	}

	//如果没有发送完成则等待
	while (!SPI_TxFifoStatusGet(SPI_FIFO_EMPTY))
    {
        if ((Timeout != HAL_MAX_DELAY) && Check_Ms_Timeout(tickstart,Timeout))
		{
			errorcode = HAL_TIMEOUT;
			goto error;
		}
    }

error:
    SPI_RxFifoReset();

	hspi->State = HAL_SPI_STATE_READY;

    xy_free(hspi->pTxBuffPtr);

	__HAL_UNLOCK(hspi);

	return errorcode;
}

/**
 * @brief  SPI从机阻塞发送接收API.
 *
 * @param  hspi. 详见结构体定义  @ref HAL_SPI_HandleTypeDef.
 * @param  pTxData:发送数据的存储地址.
 * @param  pRxData:接收数据的存储地址.
 * @param  Size:收发数据的大小.
 * @param  Timeout：超时时间（单位ms）;
 *         如果Timeout为HAL_MAX_DELAY 是一直阻塞，直到发送和接收到指定数量的字节后才退出;
 * 		   如果Timeout为非0非HAL_MAX_DELAY时，表示函数等待数据发送接收完成的最长时间，超时则退出函数.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：发送和接收数据成功，表示指定时间内成功发送和接收指定数量的数据
 *         @retval  HAL_ERROR    ：入参错误
 *         @retval  HAL_BUSY     ：外设正在使用中
 *         @retval  HAL_TIMEOUT  ：表示指定时间内未能成功发送和接收指定数量的数据
 * @note   hspi结构体中的TxXferCount表示实际发送的字节数，RxXferCount表示实际接收的字节数.
 */
__RAM_FUNC HAL_StatusTypeDef HAL_SPI_Slave_TransmitReceive(HAL_SPI_HandleTypeDef *hspi, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0;
	HAL_StatusTypeDef errorcode = HAL_OK;
	uint8_t slave_first_tx = 0;

	if (hspi->State == HAL_SPI_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hspi->State != HAL_SPI_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pTxData == NULL) || (pRxData == NULL) || (Size == 0U) || (Timeout == 0))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK(hspi);

	tickstart = Get_Tick();

	hspi->State = HAL_SPI_STATE_BUSY;
	hspi->ErrorCode = HAL_SPI_ERROR_NONE;
	hspi->pTxBuffPtr = xy_malloc(sizeof(uint8_t) * Size);
	if (hspi->pTxBuffPtr == NULL)
	{
		return HAL_ERROR;
	}
	memcpy(hspi->pTxBuffPtr, pTxData, Size);
	hspi->TxXferSize = Size;
	hspi->TxXferCount = 0;
	hspi->pRxBuffPtr = (uint8_t *)pRxData;
	hspi->RxXferSize = Size;
	hspi->RxXferCount = 0;

	//先发送再接收数据
	while ((hspi->TxXferSize > 0U) || (hspi->RxXferSize > 0U))
	{
		//发送数据
		if ((!SPI_TxFifoStatusGet(SPI_FIFO_FULL)) && (hspi->TxXferSize > 0U))
		{
            uint8_t txfifo_len = SPI_TxFifoStatusGet(SPI_FIFO_DATA_LEN);
			if ((hspi->TxXferSize >= 4) && (txfifo_len <= 124))
			{
				hspi->Instance->TXD = *((uint32_t *)(hspi->pTxBuffPtr + hspi->TxXferCount));
                hspi->TxXferSize -= 4;
                hspi->TxXferCount += 4;
            }
			else if ((hspi->TxXferSize >= 2 && hspi->TxXferSize < 4) && (txfifo_len <= 126))
			{
				hspi->Instance->TXD_16 = *((uint16_t *)(hspi->pTxBuffPtr + hspi->TxXferCount));
                hspi->TxXferSize -= 2;
                hspi->TxXferCount += 2;
            }
			else if ((hspi->TxXferSize == 1) && (txfifo_len <= 127))
			{
				hspi->Instance->TXD_8 = *((uint8_t *)(hspi->pTxBuffPtr + hspi->TxXferCount));
                hspi->TxXferSize--;
                hspi->TxXferCount++;
            }
		}
		else
		{
			if ((Timeout != HAL_MAX_DELAY) && Check_Ms_Timeout(tickstart,Timeout))
			{
				errorcode = HAL_TIMEOUT;
				goto error;
			}
		}
		
		//SPI从机的TXFIFO首次从空至非空需要重置SPI内部计数器以保证数据发送正确，通过使CS引脚产生下降沿可重置SPI内部计数器。
		if(slave_first_tx == 0)
		{
			GPIO_InputPeriInvertCmd(GPIO_SPI_CS_N, ENABLE);
			GPIO_InputPeriInvertCmd(GPIO_SPI_CS_N, DISABLE);
			slave_first_tx = 1;
		}

		//接收数据
		if (!SPI_RxFifoStatusGet(SPI_FIFO_EMPTY) && (hspi->RxXferSize > 0U))
		{
            uint8_t rxfifo_len = SPI_RxFifoStatusGet(SPI_FIFO_DATA_LEN);
            uint8_t full = SPI_RxFifoStatusGet(SPI_FIFO_FULL);
			if ((hspi->RxXferSize >= 4) && (rxfifo_len >= 4 || full == 1))
			{
				(*(uint32_t *)hspi->pRxBuffPtr) = hspi->Instance->RXD;
				hspi->pRxBuffPtr += 4;
                hspi->RxXferSize -= 4;
                hspi->RxXferCount += 4;
            }
			else if ((hspi->RxXferSize >= 2 && hspi->RxXferSize < 4) && (rxfifo_len >= 2 || full == 1))
			{
				(*(uint16_t *)hspi->pRxBuffPtr) = hspi->Instance->RXD_16;
				hspi->pRxBuffPtr += 2;
                hspi->RxXferSize -= 2;
                hspi->RxXferCount += 2;
            }
			else if ((hspi->RxXferSize == 1) && (rxfifo_len >= 1 || full == 1))
			{
				(*(uint8_t *)hspi->pRxBuffPtr) = hspi->Instance->RXD_8;
				hspi->pRxBuffPtr++;
                hspi->RxXferSize--;
                hspi->RxXferCount++;
            }
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
	hspi->State = HAL_SPI_STATE_READY;

    xy_free(hspi->pTxBuffPtr);

	__HAL_UNLOCK(hspi);

	return errorcode;
}

/******************************************************************************************/
HAL_SPI_HandleTypeDef spi_slave_handle = {0};

static void spi_slave_init(SpiArgStruct *pSpiArgStruct)
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
		gpio_init.PinRemap  = GPIO_SPI_CS_N;
		HAL_GPIO_Init(&gpio_init);
	}

	spi_slave_handle.Instance		  = (SPI_TypeDef *)pSpiArgStruct -> SpiInstance;
	spi_slave_handle.Init.MasterSlave = HAL_SPI_MODE_SLAVE;
	spi_slave_handle.Init.WorkMode    = pSpiArgStruct -> SpiWorkMode;
	HAL_Slave_SPI_Init(&spi_slave_handle);
}

/******************************************************************************************/
/**
 * @brief  去初始化SPI.
 *
 * @param  hspi. 详见结构体定义  @ref HAL_SPI_HandleTypeDef。
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 * @attention SPI做从机时， 去初始化时必须进行硬件复位
 */
HAL_StatusTypeDef HAL_SPI_Slave_DeInit(HAL_SPI_HandleTypeDef *hspi)
{
    assert_param(IS_SPI_INSTANCE(hspi->Instance));

	if (hspi == NULL)
    {
		return HAL_ERROR;
    }

	if (hspi->State == HAL_SPI_STATE_RESET)
	{
		return HAL_ERROR;
	}

	hspi->State = HAL_SPI_STATE_BUSY;


	//使能SPI外设时钟
	PRCM_ClockEnable(CORE_CKG_CTL_SPI_EN);

	//关闭SPI
	SPI_Disable();

	//失能外设时钟
	PRCM_ClockDisable(CORE_CKG_CTL_SPI_EN);


	hspi->ErrorCode = HAL_SPI_ERROR_NONE;
	hspi->State = HAL_SPI_STATE_RESET;

	__HAL_UNLOCK(hspi);

	return HAL_OK;
}

/******************************************************************************************/
void spi_slave_test(SpiArgStruct *pSpiArgStruct)
{
	HAL_StatusTypeDef ret = HAL_OK;
	char RecvConfigBuffer[10] = {0};
	static uint32_t TestTimesFlag = 0;
	TestTimesFlag++;

	Debug_Print_Str(DEBUG_LEVEL_2, "\r\n<spi> test begin\r\n");
	Debug_Print_ArgStruct(DEBUG_LEVEL_1, pSpiArgStruct);

    /*************************从机初始化并准备接收缓存区****************************/
    //spi从机初始化
    spi_slave_init(pSpiArgStruct);
    //根据size申请内存，并初始化为全0
	char *pRecvBuffer = (char *)xy_malloc(pSpiArgStruct -> SpiTestSize);
	char *pSendBuffer = (char *)xy_malloc(pSpiArgStruct -> SpiTestSize);
    memset(pRecvBuffer, 0, pSpiArgStruct -> SpiTestSize);
    memset(pSendBuffer, 0, pSpiArgStruct -> SpiTestSize);
	//初始化pSendBuffer
	for(uint16_t i = 0; i < pSpiArgStruct -> SpiTestSize; i++)
	{
		//i为64整数倍的时候，不要赋值'\0'
		((i % 64 == 0) ? *(pSendBuffer + i) = 0x01 : (*(pSendBuffer + i) = i % 64));
	}

	/***********************回应"OK\n"************************/
	SEND_OK;

	/***********************SPI从机接收发送"************************/
	if(pSpiArgStruct -> SpiTestSize <= 128)
	{
		/**********从机阻塞接收**********/
        Debug_Print_Str(DEBUG_LEVEL_1, "\r\nspi从机阻塞接收");
	
		WAIT_OK;

		ret = HAL_SPI_Receive(&spi_slave_handle, (uint8_t *)pRecvBuffer, pSpiArgStruct->SpiTestSize, RECEIVE_DEFAULT_MAX_DELAY);
		if(ret != HAL_OK)
		{
			return;
		}
		Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pSpiArgStruct->SpiTestSize);

		/**********从机阻塞发送**********/
		Debug_Print_Str(DEBUG_LEVEL_1, "\r\nspi从机阻塞发送");
		Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pSpiArgStruct->SpiTestSize);

		HAL_Delay(5); //确保主机先等待OK
		SEND_OK;

		ret = HAL_Slave_SPI_Transmit(&spi_slave_handle, (uint8_t *)pRecvBuffer, pSpiArgStruct->SpiTestSize, TRANSMIT_DEFAULT_MAX_DELAY);
		if(ret != HAL_OK)
		{
			return;
		}
	}
	else
	{
		/**********从机阻塞发送接收**********/
		Debug_Print_Str(DEBUG_LEVEL_1, "\r\nspi从机阻塞发送接收");

		WAIT_OK;

		ret = HAL_SPI_Slave_TransmitReceive(&spi_slave_handle, (uint8_t *)pSendBuffer, (uint8_t *)pRecvBuffer, pSpiArgStruct -> SpiTestSize, TRANSMIT_DEFAULT_MAX_DELAY);
		if(ret != HAL_OK)
		{
			return;
		}
		Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pSendBuffer, pSpiArgStruct->SpiTestSize);
		Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pSpiArgStruct->SpiTestSize);
	}

	WAIT_OK;

	HAL_SPI_Slave_DeInit(&spi_slave_handle);

	xy_free(pRecvBuffer);
	xy_free(pSendBuffer);

	Debug_Print_Str(DEBUG_LEVEL_2, "\r\n<spi> test end\r\n");
}

#endif /* #if (DRIVER_TEST == 2) */
