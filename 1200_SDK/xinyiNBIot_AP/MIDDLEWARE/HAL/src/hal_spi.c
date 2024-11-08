/**
 ******************************************************************************
 * @file    hal_spi.c
 * @brief   HAL库SPI.
 ******************************************************************************
 */

#include "hal_spi.h"
#include "hal_def.h"
#include "nvic.h"
#include <string.h>
#include "xy_gpio_rcr.h"

/**
 * @brief	SPI中断类型枚举
 */
typedef enum
{
    HAL_SPI_INTTYPE_HARDWARE_FAULT = SPI_INT_MODE_FAIL,        /*!< SPI硬件故障中断，主机或从机模式下CS引脚意外拉高 */
    HAL_SPI_INTTYPE_RX_FIFO_OVF = SPI_INT_RX_OVERFLOW,         /*!< SPI RXFIFO溢出中断 */
    HAL_SPI_INTTYPE_TX_FIFO_THD = SPI_INT_TX_FIFO_NFULL,       /*!< SPI TXFIFO阈值中断，当TXFIFO现存量小于阈值时触发 */
    HAL_SPI_INTTYPE_RX_FIFO_THD = SPI_INT_RX_FIFO_NEMPTY       /*!< SPI RXFIFO阈值中断，当RXFIFO现存量大于等于阈值时触发 */
}HAL_SPI_IntTypeDef;

HAL_SPI_HandleTypeDef *g_spi_handle = NULL; //指向当前句柄

static void HAL_SPI_IRQHandler(void);

/**
 * @brief SPI私有保护宏
 *
 */
#define IS_SPI_INSTANCE(__INSTANCE__)   ((__INSTANCE__) == HAL_SPI)

/**
 * @brief SPI主从收发中断过程中，单次中断传输数据个数，该值必须小于等于128
 *
 */
#define HAL_SPI_MAX_XFERSIZE    (64)

/**
 * @brief SPI主机片选引脚拉高
 *
 * @param slave_select_pin 主机片选引脚选择，详见HAL_SPI_ChipSelTypeDef.
 */
inline void HAL_SPI_SetCS(HAL_SPI_ChipSelTypeDef slave_select_pin)
{
    UNUSED_ARG(slave_select_pin);
    SPI_ChipSelect(SPI_CONFIG_SS_LINES_NONE);
}

/**
 * @brief SPI主机片选引脚拉低
 *
 * @param slave_select_pin 主机片选引脚选择，详见HAL_SPI_ChipSelTypeDef.
 */
inline void HAL_SPI_ResetCS(HAL_SPI_ChipSelTypeDef slave_select_pin)
{
	SPI_ChipSelect(slave_select_pin);
}

/**
 * @brief   获取 SPI 工作状态.
 *
 * @param   hspi. 详见结构体定义 HAL_SPI_HandleTypeDef.
 * @return  返回HAL状态，详见 HAL_SPI_StateTypeDef.
 */
inline HAL_SPI_StateTypeDef HAL_SPI_GetState(HAL_SPI_HandleTypeDef *hspi)
{
	return hspi->State;
}

/**
 * @brief  获取 SPI 错误状态.
 *
 * @param  hspi. 详见结构体定义 HAL_SPI_HandleTypeDef.
 * @return 返回SPI错误码，详见 HAL_SPI_ErrorCodeTypeDef.
 */
inline HAL_SPI_ErrorCodeTypeDef HAL_SPI_GetError(HAL_SPI_HandleTypeDef *hspi)
{
	return hspi->ErrorCode;
}

/**
 * @brief  获取SPI中断源使能状态.
 *
 * @param  hspi 详情参考 @ref HAL_SPI_HandleTypeDef.
 * @return 返回SPI中断类型，详见 HAL_SPI_IntTypeDef.
 */
static HAL_SPI_IntTypeDef HAL_SPI_GetITEnable(HAL_SPI_HandleTypeDef *hspi)
{
    return (hspi->Instance->IMASK);
}

/**
 * @brief  获取SPI中断标志.
 *
 * @param  hspi 详情参考 @ref HAL_SPI_HandleTypeDef.
 * @return 返回SPI中断类型，详见 HAL_SPI_IntTypeDef.
 */
static HAL_SPI_IntTypeDef HAL_SPI_GetITState(HAL_SPI_HandleTypeDef *hspi)
{
    return (hspi->Instance->INT_STATUS);
}

/**
 * @brief  清除SPI中断标志.
 *
 * @param  hspi 详情参考 @ref HAL_SPI_HandleTypeDef.
 * @param  IntFlag 详情参考 @ref HAL_SPI_IntTypeDef.
 */
static void HAL_SPI_Clear_IT(HAL_SPI_HandleTypeDef *hspi, HAL_SPI_IntTypeDef IntFlag)
{
    (hspi->Instance->INT_STATUS) |= IntFlag;
}

/**
 * @brief  使能SPI中断源.
 *
 * @param  hspi 详情参考 @ref HAL_SPI_HandleTypeDef.
 * @param  IntFlag 详情参考 @ref HAL_SPI_IntTypeDef.
 */
static void HAL_SPI_Enable_IT(HAL_SPI_HandleTypeDef *hspi, HAL_SPI_IntTypeDef IntFlag)
{
    (hspi->Instance->IEN) |= IntFlag;
}

/**
 * @brief  关闭SPI中断源.
 *
 * @param  hspi 详情参考 @ref HAL_SPI_HandleTypeDef.
 * @param  IntFlag 详情参考 @ref HAL_SPI_IntTypeDef.
 */
static void HAL_SPI_Disable_IT(HAL_SPI_HandleTypeDef *hspi, HAL_SPI_IntTypeDef IntFlag)
{
    (hspi->Instance->IDIS) |= IntFlag;
}

/**
 * @brief  初始化SPI.耗时71us
 *
 * @param  hspi. 详见结构体定义   @ref HAL_SPI_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 */
HAL_StatusTypeDef HAL_SPI_Init(HAL_SPI_HandleTypeDef *hspi)
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
    HAL_SPI_Disable_IT(hspi,SPI_INT_ALL);

	if (hspi->Init.MasterSlave == HAL_SPI_MODE_MASTER)
	{
        //失能SPI
		SPI_Disable();

		//配置SPI
		SPI_ConfigSetExpClk(hspi->Init.Clock_Prescaler, hspi->Init.WorkMode, hspi->Init.MasterSlave, SPI_CONFIG_WORD_SIZE_BITS_8);//默认使用8位传输
		SPI_SetDelay(0x02,0x04,0x04,0x04);            //MASTER时的输出时序延迟
		SPI_ChipSelectMode(SPI_CONFIG_SS_MODE_NORMAL);//同一时刻仅SS_0或SS_1可以低有效
        SPI_DisManualTransmit();                      //关闭手动TXFIFO发送
		SPI_ManualCsSet();                            //MASTER时切换CS为手动外设选择模式
        SPI_TxFifoReset();                            //清FIFO
        SPI_RxFifoReset();
        SPI_TxFifoEnable();                           //使能FIFO
        SPI_RxFifoEnable();
		SPI_Enable();                                 //使能SPI
		HAL_SPI_SetCS(HAL_SPI_CS0);                   //默认拉高所有CS
	}

	//失能pad为防倒灌模式
	//RCR_LCD_IO_All_Disable();

    //记录句柄
    g_spi_handle = hspi;

    hspi->ErrorCode = HAL_SPI_ERROR_NONE;
	hspi->State = HAL_SPI_STATE_READY;

	return HAL_OK;
}


/**
 * @brief  去初始化SPI.
 *
 * @param  hspi. 详见结构体定义  @ref HAL_SPI_HandleTypeDef。
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 */
HAL_StatusTypeDef HAL_SPI_DeInit(HAL_SPI_HandleTypeDef *hspi)
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

    //释放句柄
    g_spi_handle = NULL;

	hspi->ErrorCode = HAL_SPI_ERROR_NONE;
	hspi->State = HAL_SPI_STATE_RESET;

	__HAL_UNLOCK(hspi);

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
HAL_StatusTypeDef HAL_SPI_Transmit(HAL_SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0;
	HAL_StatusTypeDef errorcode = HAL_OK;

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

				//SPI主机CLK较低时需要等待发送完成，否则倒数第二个数据会被最后一个数据吃掉，导致数据丢失
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
 * @brief  SPI阻塞接收API.
 *
 * @param  hspi. 详见结构体定义  @ref HAL_SPI_HandleTypeDef.
 * @param  pData:接收数据的存储地址.
 * @param  Size:接收数据的大小.
 * @param  Timeout：超时时间（单位ms）;
 *         如果Timeout为0，则是不阻塞，检测到接收FIFO空后，则程序立刻退出;
 *         如果Timeout为HAL_MAX_DELAY，则是一直阻塞，直到接收到指定数量的字节后才退出;
 * 		   如果Timeout为非0非HAL_MAX_DELAY，表示函数等待数据接收完成的最长时间，超时则退出函数.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval HAL_OK      ：超时时间不为0时，表示指定时间内成功接收指定数量的数据；超时时间为0时，也会返回，但实际接收数据的数量通过hspi结构体中的RxXferCount来确定
 *         @retval HAL_ERROR   ：入参错误
 *         @retval HAL_BUSY    ：外设正在使用中
 *         @retval HAL_TIMEOUT ：超时时间不为0时会返回，表示指定时间内未能成功接收指定数量的数据
 * @note   hspi结构体中的RxXferCount表示实际接收的字节数.
 */
HAL_StatusTypeDef HAL_SPI_Receive(HAL_SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0;
	HAL_StatusTypeDef errorcode = HAL_OK;

    //主机模式直接使用收发接口,发送dummy以接收数据
    if(hspi->Init.MasterSlave == HAL_SPI_MODE_MASTER)
    {
        return HAL_SPI_Master_TransmitReceive(hspi, pData, pData, Size, Timeout);
    }

	if (hspi->State == HAL_SPI_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hspi->State != HAL_SPI_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK(hspi);

	tickstart = Get_Tick();

	hspi->State = HAL_SPI_STATE_BUSY;
	hspi->ErrorCode = HAL_SPI_ERROR_NONE;
	hspi->pRxBuffPtr = (uint8_t *)pData;
	hspi->RxXferSize = Size;
	hspi->RxXferCount = 0;
	hspi->pTxBuffPtr = (uint8_t *)NULL;
	hspi->TxXferSize = 0U;
	hspi->TxXferCount = 0U;

	//接收数据
	while (hspi->RxXferSize > 0U)
	{
		if (!SPI_RxFifoStatusGet(SPI_FIFO_EMPTY))
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
			if ((Timeout != 0U) && ((Timeout != HAL_MAX_DELAY) && Check_Ms_Timeout(tickstart,Timeout)))
			{
				errorcode = HAL_TIMEOUT;
				goto error;
			}
			else if (Timeout == 0U)
			{
				errorcode = HAL_OK;
				goto error;
			}
		}
	}

error:
    SPI_TxFifoReset();

	hspi->State = HAL_SPI_STATE_READY;

	__HAL_UNLOCK(hspi);

	return errorcode;
}

/**
 * @brief  SPI主机阻塞发送接收API.
 *
 * @param  hspi. 详见结构体定义  @ref HAL_SPI_HandleTypeDef.
 * @param  pTxData:发送数据的存储地址.
 * @param  pRxData:接收数据的存储地址.
 * @param  Size:收发数据的大小.
 * @param  Timeout：超时时间（单位ms）;
 *         如果Timeout为HAL_MAX_DELAY，则是一直阻塞，直到发送和接收到指定数量的字节后才退出;
 * 		   如果Timeout为非0非HAL_MAX_DELAY时，表示函数等待数据发送接收完成的最长时间，超时则退出函数.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：发送和接收数据成功，表示指定时间内成功发送和接收指定数量的数据
 *         @retval  HAL_ERROR    ：入参错误
 *         @retval  HAL_BUSY     ：外设正在使用中
 *         @retval  HAL_TIMEOUT  ：表示指定时间内未能成功发送和接收指定数量的数据
 * @note   hspi结构体中的TxXferCount表示实际发送的字节数，RxXferCount表示实际接收的字节数.
 */
HAL_StatusTypeDef HAL_SPI_Master_TransmitReceive(HAL_SPI_HandleTypeDef *hspi, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0;
	HAL_StatusTypeDef errorcode = HAL_OK;

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
	uint32_t tx_allow = 1; //主机需要使用该标志控制读写顺序，否则容易丢接收数据
	while ((hspi->TxXferSize > 0U) || (hspi->RxXferSize > 0U))
	{
		//发送数据
        if ((!SPI_TxFifoStatusGet(SPI_FIFO_FULL)) && (hspi->TxXferSize > 0U) && (tx_allow == 1))
		{
            uint8_t txfifo_len = SPI_TxFifoStatusGet(SPI_FIFO_DATA_LEN);
			if ((hspi->TxXferSize >= 4) && (txfifo_len <= 124))
			{
				hspi->Instance->TXD = *((uint32_t *)(hspi->pTxBuffPtr + hspi->TxXferCount));
                hspi->TxXferSize -= 4;
                hspi->TxXferCount += 4;
				tx_allow = 0;
			}
			else if ((hspi->TxXferSize >= 2 && hspi->TxXferSize < 4) && (txfifo_len <= 126))
			{
				hspi->Instance->TXD_16 = *((uint16_t *)(hspi->pTxBuffPtr + hspi->TxXferCount));
                hspi->TxXferSize -= 2;
                hspi->TxXferCount += 2;

				//SPI主机CLK较低时需要等待发送完成，否则倒数第二个数据会被最后一个数据吃掉，导致数据丢失
				if(hspi->Init.MasterSlave == HAL_SPI_MODE_MASTER) 
				{
					while (!SPI_TxFifoStatusGet(SPI_FIFO_EMPTY));
				}
				tx_allow = 0;
			}
			else if ((hspi->TxXferSize == 1) && (txfifo_len <= 127))
			{
				hspi->Instance->TXD_8 = *((uint8_t *)(hspi->pTxBuffPtr + hspi->TxXferCount));
                hspi->TxXferSize--;
                hspi->TxXferCount++;
				tx_allow = 0;
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

        //接收数据
        if (!SPI_RxFifoStatusGet(SPI_FIFO_EMPTY) && (hspi->RxXferSize > 0U) && (tx_allow == 0))
		{
            uint8_t rxfifo_len = SPI_RxFifoStatusGet(SPI_FIFO_DATA_LEN);
            uint8_t full = SPI_RxFifoStatusGet(SPI_FIFO_FULL);
			if ((hspi->RxXferSize >= 4) && (rxfifo_len >= 4 || full == 1))
			{
				(*(uint32_t *)hspi->pRxBuffPtr) = hspi->Instance->RXD;
				hspi->pRxBuffPtr += 4;
				hspi->RxXferSize -= 4;
                hspi->RxXferCount += 4;
				tx_allow = 1;
			}
			else if ((hspi->RxXferSize >= 2 && hspi->RxXferSize < 4) && (rxfifo_len >= 2 || full == 1))
			{
				(*(uint16_t *)hspi->pRxBuffPtr) = hspi->Instance->RXD_16;
				hspi->pRxBuffPtr += 2;
                hspi->RxXferSize -= 2;
                hspi->RxXferCount += 2;
				tx_allow = 1;
			}
			else if ((hspi->RxXferSize == 1) && (rxfifo_len >= 1 || full == 1))
			{
				(*(uint8_t *)hspi->pRxBuffPtr) = hspi->Instance->RXD_8;
				hspi->pRxBuffPtr++;
				hspi->RxXferSize--;
                hspi->RxXferCount++;
				tx_allow = 1;
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

/**
 * @brief  SPI非阻塞发送API.
 *
 * @param  hspi. 详见结构体定义  @ref HAL_SPI_HandleTypeDef.
 * @param  pData:发送数据的存储地址.
 * @param  Size:发送数据的大小.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示中断发送需要的参数配置、外设配置成功，等待进入中断发送数据
 *         @retval  HAL_ERROR    ：表示入参错误
 *         @retval  HAL_BUSY     ：表示外设正在使用中
 */
HAL_StatusTypeDef HAL_SPI_Transmit_IT(HAL_SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size)
{
	if (hspi->State == HAL_SPI_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hspi->State != HAL_SPI_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK(hspi);

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
	hspi->RxXferSize = 0;
	hspi->RxXferCount = 0;

	//清除所有中断标志位
	HAL_SPI_Clear_IT(hspi, SPI_INT_ALL);

    //设置TXFIFO阈值为1，当TXFIFO数据存量小于1时触发TXFIFO阈值中断
    hspi->Instance->TX_THRESH = 1;

    //注册中断
	NVIC_IntRegister(SPI1_IRQn, HAL_SPI_IRQHandler, 1);

	//使能中断源
    HAL_SPI_Disable_IT(hspi, HAL_SPI_INTTYPE_RX_FIFO_OVF);//SPI的TX/RXFIO是环形的，为了避免只发数据时出现RXFIFO溢出，需要关闭RXFIFO溢出中断
	HAL_SPI_Enable_IT(hspi, HAL_SPI_INTTYPE_TX_FIFO_THD);
    HAL_SPI_Enable_IT(hspi, HAL_SPI_INTTYPE_HARDWARE_FAULT);

	__HAL_UNLOCK(hspi);

	return HAL_OK;
}

/**
 * @brief  SPI非阻塞接收API.
 *
 * @param  hspi. 详见结构体定义  @ref HAL_SPI_HandleTypeDef.
 * @param  pData:接收数据的存储地址.
 * @param  Size:接收数据的大小.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示中断接收需要的参数配置、外设配置成功，等待进入中断接收数据
 *         @retval  HAL_ERROR    ：表示入参错误
 *         @retval  HAL_BUSY     ：外设正在使用中
 */
HAL_StatusTypeDef HAL_SPI_Receive_IT(HAL_SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size)
{
	if (hspi->State == HAL_SPI_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hspi->State != HAL_SPI_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK(hspi);

	hspi->State = HAL_SPI_STATE_BUSY;
	hspi->ErrorCode = HAL_SPI_ERROR_NONE;
	hspi->pRxBuffPtr = (uint8_t *)pData;
	hspi->RxXferSize = Size;
	hspi->RxXferCount = 0;
	hspi->pTxBuffPtr = (uint8_t *)NULL;
    hspi->TxXferSize = 0;
    hspi->TxXferCount = 0;

	//清除所有中断标志位
	HAL_SPI_Clear_IT(hspi, SPI_INT_ALL);

    //根据想接收数据长度设置RXFIFO阈值，当RXFIFO数据存量大于等于阈值时触发RXFIFO阈值中断
    hspi->Instance->RX_THRESH = (hspi->RxXferSize > HAL_SPI_MAX_XFERSIZE) ? HAL_SPI_MAX_XFERSIZE : hspi->RxXferSize;

    //注册中断
	NVIC_IntRegister(SPI1_IRQn, HAL_SPI_IRQHandler, 1);

	//使能中断源
	HAL_SPI_Enable_IT(hspi, HAL_SPI_INTTYPE_RX_FIFO_THD);
	HAL_SPI_Enable_IT(hspi, HAL_SPI_INTTYPE_RX_FIFO_OVF);
    HAL_SPI_Enable_IT(hspi, HAL_SPI_INTTYPE_HARDWARE_FAULT);

	//如果SPI是主机则发送dummy以接收数据
	if(hspi->Init.MasterSlave == HAL_SPI_MODE_MASTER)
	{
        uint8_t dummy_write_size = (hspi->RxXferSize > HAL_SPI_MAX_XFERSIZE) ? HAL_SPI_MAX_XFERSIZE : hspi->RxXferSize;
		while(dummy_write_size > 0U)
		{
			if (!SPI_TxFifoStatusGet(SPI_FIFO_FULL))
			{
				uint8_t txfifo_len = SPI_TxFifoStatusGet(SPI_FIFO_DATA_LEN);
				if ((dummy_write_size >= 4) && (txfifo_len <= 124))
				{
					hspi->Instance->TXD = (uint32_t )0U;
					dummy_write_size -= 4;
				}
				else if ((dummy_write_size >= 2 && dummy_write_size < 4) && (txfifo_len <= 126))
				{
					hspi->Instance->TXD_16 = (uint16_t )0U;
					dummy_write_size -= 2;

					//SPI主机CLK较低时需要等待发送完成，否则倒数第二个数据会被最后一个数据吃掉，导致数据丢失
					if(hspi->Init.MasterSlave == HAL_SPI_MODE_MASTER) 
					{
						while (!SPI_TxFifoStatusGet(SPI_FIFO_EMPTY));
					}
				}
				else if ((dummy_write_size == 1) && (txfifo_len <= 127))
				{
					hspi->Instance->TXD_8 = (uint8_t )0U;
					dummy_write_size--;
				}
			}
		}
	}

	__HAL_UNLOCK(hspi);

	return HAL_OK;
}


/**
 * @brief  本地SPI中断服务函数.
 *
 */
static void HAL_SPI_IRQHandler(void)
{
	HAL_SPI_IntTypeDef ITState = HAL_SPI_GetITState(g_spi_handle);
	HAL_SPI_IntTypeDef ITEnable = HAL_SPI_GetITEnable(g_spi_handle);

	//错误处理流程：发生错误中断
	if ((HAL_RESET != (ITEnable & HAL_SPI_INTTYPE_HARDWARE_FAULT)) && (HAL_RESET != (ITState & HAL_SPI_INTTYPE_HARDWARE_FAULT)))
	{
		g_spi_handle->ErrorCode |= HAL_SPI_ERROR_HARDWARE_FAULT;
        HAL_SPI_Disable_IT(g_spi_handle, HAL_SPI_INTTYPE_HARDWARE_FAULT);
		HAL_SPI_Clear_IT(g_spi_handle, HAL_SPI_INTTYPE_HARDWARE_FAULT);
	}
	if ((HAL_RESET != (ITEnable & HAL_SPI_INTTYPE_RX_FIFO_OVF)) && (HAL_RESET != (ITState & HAL_SPI_INTTYPE_RX_FIFO_OVF)))
	{
		g_spi_handle->ErrorCode |= HAL_SPI_ERROR_RX_FIFO_OVF;
        HAL_SPI_Disable_IT(g_spi_handle, HAL_SPI_INTTYPE_RX_FIFO_OVF);
		HAL_SPI_Clear_IT(g_spi_handle, HAL_SPI_INTTYPE_RX_FIFO_OVF);
	}
	if (g_spi_handle->ErrorCode != HAL_SPI_ERROR_NONE)
	{
		HAL_SPI_ErrorCallback(g_spi_handle);

		goto END;
	}

	//接收数据：rxfifo阈值中断
	if ((HAL_RESET != (ITEnable & HAL_SPI_INTTYPE_RX_FIFO_THD)) && (HAL_RESET != (ITState & HAL_SPI_INTTYPE_RX_FIFO_THD)))
	{
        while (!SPI_RxFifoStatusGet(SPI_FIFO_EMPTY))
        {
            uint8_t rxfifo_len = SPI_RxFifoStatusGet(SPI_FIFO_DATA_LEN);
            uint8_t full = SPI_RxFifoStatusGet(SPI_FIFO_FULL);
            if ((g_spi_handle->RxXferSize >= 4) && (rxfifo_len >= 4 || full == 1))
            {
                (*(uint32_t *)g_spi_handle->pRxBuffPtr) = g_spi_handle->Instance->RXD;
                g_spi_handle->pRxBuffPtr += 4;
                g_spi_handle->RxXferSize -= 4;
                g_spi_handle->RxXferCount += 4;
            }
            else if ((g_spi_handle->RxXferSize >= 2 && g_spi_handle->RxXferSize < 4) && (rxfifo_len >= 2 || full == 1))
            {
                (*(uint16_t *)g_spi_handle->pRxBuffPtr) = g_spi_handle->Instance->RXD_16;
                g_spi_handle->pRxBuffPtr += 2;
                g_spi_handle->RxXferSize -= 2;
                g_spi_handle->RxXferCount += 2;
            }
            else if ((g_spi_handle->RxXferSize == 1) && (rxfifo_len >= 1 || full == 1))
            {
                (*(uint8_t *)g_spi_handle->pRxBuffPtr) = g_spi_handle->Instance->RXD_8;
                g_spi_handle->pRxBuffPtr++;
                g_spi_handle->RxXferSize--;
                g_spi_handle->RxXferCount++;
            }
            if (g_spi_handle->RxXferSize == 0U)
		    {
                HAL_SPI_Disable_IT(g_spi_handle, HAL_SPI_INTTYPE_RX_FIFO_THD);
                HAL_SPI_Clear_IT(g_spi_handle, HAL_SPI_INTTYPE_RX_FIFO_THD);
                g_spi_handle->State = HAL_SPI_STATE_READY;
                SPI_TxFifoReset();
				
			    HAL_SPI_RxCpltCallback(g_spi_handle);

				goto END;
            }
        }
        //根据想接收数据长度设置RXFIFO阈值，当RXFIFO数据存量大于等于阈值时触发RXFIFO阈值中断
        g_spi_handle->Instance->RX_THRESH = (g_spi_handle->RxXferSize > HAL_SPI_MAX_XFERSIZE) ? HAL_SPI_MAX_XFERSIZE : g_spi_handle->RxXferSize;
        //如果SPI是主机则发送dummy以接收数据
        if(g_spi_handle->Init.MasterSlave == HAL_SPI_MODE_MASTER)
        {
            uint8_t dummy_write_size = (g_spi_handle->RxXferSize > HAL_SPI_MAX_XFERSIZE) ? HAL_SPI_MAX_XFERSIZE : g_spi_handle->RxXferSize;
            while(dummy_write_size > 0U)
            {
                if (!SPI_TxFifoStatusGet(SPI_FIFO_FULL))
                {
                    uint8_t txfifo_len = SPI_TxFifoStatusGet(SPI_FIFO_DATA_LEN);
                    if ((dummy_write_size >= 4) && (txfifo_len <= 124))
                    {
                        g_spi_handle->Instance->TXD = (uint32_t )0U;
                        dummy_write_size -= 4;
                    }
                    else if ((dummy_write_size >= 2 && dummy_write_size < 4) && (txfifo_len <= 126))
                    {
                        g_spi_handle->Instance->TXD_16 = (uint16_t )0U;
                        dummy_write_size -= 2;

						//SPI主机CLK较低时需要等待发送完成，否则倒数第二个数据会被最后一个数据吃掉，导致数据丢失
						if(g_spi_handle->Init.MasterSlave == HAL_SPI_MODE_MASTER) 
						{
							while (!SPI_TxFifoStatusGet(SPI_FIFO_EMPTY));
						}
						
                    }
                    else if ((dummy_write_size == 1) && (txfifo_len <= 127))
                    {
                        g_spi_handle->Instance->TXD_8 = (uint8_t )0U;
                        dummy_write_size--;
                    }
                }
            }
        }
        HAL_SPI_Clear_IT(g_spi_handle, HAL_SPI_INTTYPE_RX_FIFO_THD);
	}

	//发送数据：txfifo阈值中断
	if ((HAL_RESET != (ITEnable & HAL_SPI_INTTYPE_TX_FIFO_THD)) && (HAL_RESET != (ITState & HAL_SPI_INTTYPE_TX_FIFO_THD)))
	{
        if(g_spi_handle->TxXferSize == 0U)
        {
            HAL_SPI_Disable_IT(g_spi_handle, HAL_SPI_INTTYPE_TX_FIFO_THD);
            HAL_SPI_Clear_IT(g_spi_handle, HAL_SPI_INTTYPE_TX_FIFO_THD);
            g_spi_handle->State = HAL_SPI_STATE_READY;
            xy_free(g_spi_handle->pTxBuffPtr);
            SPI_RxFifoReset();
			
			HAL_SPI_TxCpltCallback(g_spi_handle);

			goto END;
        }
        else
        {
            //发送数据
            uint8_t real_write_size = (g_spi_handle->TxXferSize > HAL_SPI_MAX_XFERSIZE) ? HAL_SPI_MAX_XFERSIZE : g_spi_handle->TxXferSize;
            while (real_write_size > 0U)
            {
                if (!SPI_TxFifoStatusGet(SPI_FIFO_FULL))
                {
                    uint8_t txfifo_len = SPI_TxFifoStatusGet(SPI_FIFO_DATA_LEN);
                    if ((real_write_size >= 4) && (txfifo_len <= 124) )
                    {
                        g_spi_handle->Instance->TXD = *((uint32_t *)(g_spi_handle->pTxBuffPtr + g_spi_handle->TxXferCount));
                        real_write_size -= 4;
                        g_spi_handle->TxXferSize -= 4;
                        g_spi_handle->TxXferCount += 4;
                    }
                    else if ((real_write_size >= 2 && real_write_size < 4) && (txfifo_len <= 126))
                    {
                        g_spi_handle->Instance->TXD_16 = *((uint16_t *)(g_spi_handle->pTxBuffPtr + g_spi_handle->TxXferCount));
                        real_write_size -= 2;
                        g_spi_handle->TxXferSize -= 2;
                        g_spi_handle->TxXferCount += 2;

						//SPI主机CLK较低时需要等待发送完成，否则倒数第二个数据会被最后一个数据吃掉，导致数据丢失
						if(g_spi_handle->Init.MasterSlave == HAL_SPI_MODE_MASTER) 
						{
							while (!SPI_TxFifoStatusGet(SPI_FIFO_EMPTY));
						}
                    }
                    else if ((real_write_size == 1) && (txfifo_len <= 127))
                    {
                        g_spi_handle->Instance->TXD_8 = *((uint8_t *)(g_spi_handle->pTxBuffPtr + g_spi_handle->TxXferCount));
                        real_write_size--;
                        g_spi_handle->TxXferSize--;
                        g_spi_handle->TxXferCount++;
                    }
                }
            }
            HAL_SPI_Clear_IT(g_spi_handle, HAL_SPI_INTTYPE_TX_FIFO_THD);
        }
	}
END:
	return;
}

