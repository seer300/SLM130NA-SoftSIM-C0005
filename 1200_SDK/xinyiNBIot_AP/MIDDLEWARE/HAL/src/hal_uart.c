/**
 * @file hal_uart.c
 * @author pancq
 * @brief UART的HAL库实现
 * @version 0.1
 * @date 2023-12-08
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "hal_uart.h"

static HAL_UART_HandleTypeDef *g_uart_handle = NULL; //指向当前句柄
static void HAL_UART_IRQHandler(HAL_UART_HandleTypeDef *huart);

/**
 * @brief UART0中断服务函数.
 */
static void HAL_UART0_IRQHandler(void)
{
    if(g_uart_handle != NULL)
    {
        HAL_UART_IRQHandler(g_uart_handle);
    }
}

/**
 * @brief  UART中断类型枚举
 */
typedef enum
{
    UART_INTTYPE_FRAME_ERR = UART_INT_RX_FRAME_ERR,     //UART帧错误中断
    UART_INTTYPE_PARITY_ERR = UART_INT_RX_PAR_ERR,      //UART校验错误中断
    UART_INTTYPE_RX_FIFO_OVF = UART_INT_RXFIFO_OVFLW,   //UART接收溢出中断
    UART_INTTYPE_RX_TIMEOUT = UART_INT_TIMEOUT,         //UART接收超时中断
    UART_INTTYPE_RX_FIFO_THD = UART_INT_RXFIFO_TRIGGER, //UART接收阈值中断，当RXFIFO现存量处于接收阈值区间时触发，接收阈值区间范围是小于上限、大于等于下限
	UART_INTTYPE_TX_FIFO_EMPTY = UART_INT_TXFIFO_EMPTY  //UARTTXFIFO空中断
} HAL_UART_IntTypeDef;

/**
 * @brief  使能UART硬件超时功能，并设置超时时间，超时倒计时触发条件为RXD处于IDLE且RXFIFO非空.
 * @param  huart 详情参考 @ref HAL_UART_HandleTypeDef.
 * @param  timeout(ms) 超时时间，用户可根据实际接收需求设置超时时间.
 */
static void HAL_UART_EnableTimeout_When_RxIdle_RxfifoNE(HAL_UART_HandleTypeDef *huart, uint16_t timeout)
{
	uint32_t reg_val = timeout * huart->Init.BaudRate / 1000;
	if (reg_val <= 0x00 || reg_val > 0x1F)
	{
		reg_val = 0x1F;
	}
    UARTTimeOutDisable((uint32_t)huart->Instance);
	UARTTimeOutConfig((uint32_t)huart->Instance, UART_RX_TIMEOUT_START_FIFO_NEMPTY, reg_val);
	UARTTimeOutEnable((uint32_t)huart->Instance);
}

/**
 * @brief  使能UART硬件超时功能，超时倒计时触发条件为RXD处于IDLE.
 * @param  huart 详情参考 @ref HAL_UART_HandleTypeDef.
 * @note   触发后必须再次调用该接口才能再次生效.
 */
static void HAL_UART_EnableTimeout_When_RxIdle(HAL_UART_HandleTypeDef *huart)
{
    UARTTimeOutDisable((uint32_t)huart->Instance);
    UARTTimeOutCondition_Set((uint32_t)huart->Instance, UART_RX_TIMEOUT_START_NO_MATTER);
    UARTTimeOutEnable((uint32_t)huart->Instance);
}

/**
 * @brief 根据RxXferSize设置rxfifo trigger level
 * @param huart 详情参考 @ref HAL_UART_HandleTypeDef.
 */
static void HAL_UART_SetRxFifoTrgLv_by_RxXferSize(HAL_UART_HandleTypeDef *huart)
{
	#define RXFIFO_1_4_DEPTH (16) //RXFIFO 1/4深度（字节数）

	uint32_t trglv = 0;
	if(huart->RxXferSize < RXFIFO_1_4_DEPTH)
	{
		trglv = UART_FIFO_LEVEL_RX1_4;
	}
	else if((huart->RxXferSize >= RXFIFO_1_4_DEPTH) && (huart->RxXferSize < (2*RXFIFO_1_4_DEPTH)))
	{
		trglv = UART_FIFO_LEVEL_RX2_4;
	}
	else if((huart->RxXferSize >= (2*RXFIFO_1_4_DEPTH)) && (huart->RxXferSize < (3*RXFIFO_1_4_DEPTH)))
	{
		trglv = UART_FIFO_LEVEL_RX3_4;
	}
	else if(huart->RxXferSize >= (3*RXFIFO_1_4_DEPTH))
	{
		trglv = UART_FIFO_LEVEL_RX4_4;
	}

    if(huart->RxXferSize)
    {
        //设置RXFIFO触发阈值：当前fifo空为level 0，若想使得UART_FIFO_LEVEL_TX1_4(level 0)有效触发，
        //必须先让fifo level != trigger level，再让fifo level == trigger level
        UART_RXFIFO_LevelSet((uint32_t)huart->Instance, UART_FIFO_LEVEL_RX4_4);
        UART_RXFIFO_LevelSet((uint32_t)huart->Instance, trglv);
    }
}

/**
 * @brief 根据波特率增加1个字符的延时，以保证数据发送完成
 * @param huart 详情参考 @ref HAL_UART_HandleTypeDef.
 */
static void HAL_UART_TxFifoEmptyDelay(HAL_UART_HandleTypeDef *huart)
{
	uint32_t onechar_timeout = (uint32_t)((15 * 1000 * 1000 / huart->Init.BaudRate) + 1); //按15bit计算，单位us
    if(onechar_timeout < 50)
    {
        onechar_timeout = 50;
    }
	HAL_Delay_US(onechar_timeout);
}

/**
 * @brief  初始化UART.
 * @param  huart 详情参考 @ref HAL_UART_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 */
HAL_StatusTypeDef HAL_UART_Init(HAL_UART_HandleTypeDef *huart)
{
    if((huart == NULL) || (huart->Instance != UART))
	{
		return HAL_ERROR;
	}

	if(huart->gState == UART_STATE_RESET)
	{
		huart->Lock = HAL_UNLOCKED;
		huart->RxLock = HAL_UNLOCKED;
	}
	else
	{
		return HAL_ERROR;
	}

    //bug7558：UART2工作时钟和波特率关系若满足 UartClk/BaudRate>7281 时, 则收发数据出错
	if(GetlsioFreq()/huart->Init.BaudRate > 7281)
	{
		return HAL_ERROR;
	}

	huart->gState = UART_STATE_BUSY;

    //开UART2时钟
    PRCM_ClockEnable(CORE_CKG_CTL_UART2_EN);

    //配置UART
    UARTDisable((uint32_t)huart->Instance); //关闭UART
    UARTIntDisable((uint32_t)huart->Instance, UART_INT_ALL); //关闭所有中断源
    UART_IntClear((uint32_t)huart->Instance, UART_INT_ALL);  // 清所有中断标志位
    UARTFIFOFlush((uint32_t)huart->Instance, UART_FIFO_ALL); // 清空TXFIFO、RXFIFO
	UARTConfigSetExpClk((uint32_t)huart->Instance, GetlsioFreq(), huart->Init.BaudRate, huart->Init.WordLength | huart->Init.Parity | UART_CTL_ENDIAN_LITTLE);
    UARTTxWaitSet((uint32_t)huart->Instance, 0); //设置发送间隔为0

	//硬件流控功能配置
    UARTFlowCtrlDisable((uint32_t)huart->Instance);
    UARTIntDisable((uint32_t)huart->Instance, UART_INT_FLOW_CTL);
	if(huart->Init.HfcLevel != UART_HFC_LEVEL_NONE)
	{
        if (huart->Init.HfcLevel == UART_HFC_LEVEL_LOW) //设置有效电平
        {
            UARTFlowCtrlConfig((uint32_t)huart->Instance, UART_FLOW_CTL_AUTO | UART_FLOW_CTL_VALID_LEVEL_HIGH | UART_FLOW_CTL_RTS_TRIGGER_LEVEL_3_4); //RXFIFO现存量大于48字节时触发RTS无效电平
        }
        else
        {
            UARTFlowCtrlConfig((uint32_t)huart->Instance, UART_FLOW_CTL_AUTO | UART_FLOW_CTL_VALID_LEVEL_LOW | UART_FLOW_CTL_RTS_TRIGGER_LEVEL_3_4);  //RXFIFO现存量大于48字节时触发RTS无效电平
        }
        UARTFlowCtrlEnable((uint32_t)huart->Instance);
    }

    //注册中断向量
    NVIC_IntRegister(UART2_IRQn, HAL_UART0_IRQHandler, 1);

    //记录句柄
    g_uart_handle = huart;

    huart->ErrorCode = UART_ERROR_NONE;
	huart->gState = UART_STATE_READY;
	huart->RxState = UART_STATE_READY;

	return HAL_OK;
}

/**
 * @brief  去初始化UART.
 * @param  huart 详情参考 @ref HAL_UART_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 */
HAL_StatusTypeDef HAL_UART_DeInit(HAL_UART_HandleTypeDef *huart)
{
    if((huart == NULL) || (huart->Instance != UART))
	{
		return HAL_ERROR;
	}

	if (huart->gState == UART_STATE_RESET)
	{
		return HAL_ERROR;
	}

	huart->gState = UART_STATE_BUSY;

    //开UART2时钟
    PRCM_ClockEnable(CORE_CKG_CTL_UART2_EN);

    //禁能TX、RX，禁能UART
    UARTFIFODisable((uint32_t)huart->Instance, UART_FIFO_ALL);
    UARTDisable((uint32_t)huart->Instance);

    //去注册中断向量
    NVIC_IntUnregister(UART2_IRQn);

    //释放句柄
    g_uart_handle = NULL;

    huart->ErrorCode = UART_ERROR_NONE;
	huart->gState = UART_STATE_RESET;
    huart->RxState = UART_STATE_RESET;

	__HAL_UNLOCK(huart);
	__HAL_UNLOCK_RX(huart);

	return HAL_OK;
}

/**
 * @brief  UART阻塞发送API.
 * @param  huart 详情参考 @ref HAL_UART_HandleTypeDef.
 * @param  pData 发送数据缓冲区指针.
 * @param  Size  发送数据字节长度.
 * @param  Timeout 超时时间（单位ms）;
 *         如果Timeout为HAL_MAX_DELAY，则是一直阻塞，等待发送完指定数量的字节才退出;
 * 		   如果Timeout为非0非HAL_MAX_DELAY时，表示函数等待数据发送完成的最长时间，超时则退出函数.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示发送数据成功，表示指定时间内成功发送指定数量的数据
 *         @retval  HAL_ERROR    ：表示入参错误
 *         @retval  HAL_BUSY     ：表示外设正在使用中
 *         @retval  HAL_TIMEOUT  ：表示指定时间内未能成功发送指定数量的数据
 * @note   huart结构体中的TxXferCount表示实际发送的字节数.
 */
HAL_StatusTypeDef HAL_UART_Transmit(HAL_UART_HandleTypeDef *huart, uint8_t *pData, uint32_t Size, uint32_t Timeout)
{
	HAL_StatusTypeDef errorcode = HAL_OK;

	if (huart->gState == UART_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (huart->gState != UART_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U) || (Timeout == 0))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK(huart);

	uint32_t tickstart = Get_Tick();

	huart->gState = UART_STATE_BUSY;
	huart->ErrorCode = UART_ERROR_NONE;
	huart->pTxBuffPtr = pData;
	huart->TxXferSize = Size;
	huart->TxXferCount = 0;

	__HAL_UNLOCK(huart);

	//发送数据
	while (huart->TxXferSize > 0U)
	{
        if(!UARTTxFifoStatusGet((uint32_t)huart->Instance, UART_FIFO_FULL))
        {
            UARTWriteData((uint32_t)huart->Instance, *(huart->pTxBuffPtr + huart->TxXferCount));
			huart->TxXferSize--;
			huart->TxXferCount++;
        }
		else
		{
			if ((Timeout != HAL_MAX_DELAY) && Check_Ms_Timeout(tickstart, Timeout))
			{
				errorcode = HAL_TIMEOUT;
				goto error;
			}
		}
	}

	//如果没有发送完成则等待
	while (!UARTTxFifoStatusGet((uint32_t)huart->Instance, UART_FIFO_EMPTY))
	{
		if ((Timeout != HAL_MAX_DELAY) && Check_Ms_Timeout(tickstart,Timeout))
		{
			errorcode = HAL_TIMEOUT;
			goto error;
		}
	}

	//根据波特率增加1个字符的延时，以保证数据发送完成
	HAL_UART_TxFifoEmptyDelay(huart);    

error:
	huart->gState = UART_STATE_READY;

	return errorcode;
}

/**
 * @brief  UART阻塞接收API.
 * @param  huart 详情参考 @ref HAL_UART_HandleTypeDef.
 * @param  pData 接收数据缓冲区指针.
 * @param  Size  接收数据字节长度.
 * @param  Timeout 超时时间（单位ms）;
 *         如果Timeout为0，则是不阻塞，检测到接收FIFO空后，则立刻退出;
 *         如果Timeout为HAL_MAX_DELAY，则是一直阻塞，直到接收到指定数量的字节后才退出;
 *         如果Timeout为非0非HAL_MAX_DELAY时，表示函数等待数据接收完成的最长时间，超时则退出函数.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：当Timeout未置为0时，表示指定时间内成功接收指定数量的数据；当Timeout置为0时，也会返回OK，但实际接收数据的数量通过huart结构体中的RxXferCount来确定
 *         @retval  HAL_ERROR    ：入参错误
 *         @retval  HAL_BUSY     ：外设正在使用中
 *         @retval  HAL_TIMEOUT  ：针对Timeout未置为0时，表示指定时间内未能成功接收指定数量的数据
 * @note   huart结构体中的RxXferCount表示实际接收的字节数.
 */
HAL_StatusTypeDef HAL_UART_Receive(HAL_UART_HandleTypeDef *huart, uint8_t *pData, uint32_t Size, uint32_t Timeout)
{
	HAL_StatusTypeDef errorcode = HAL_OK;

	if (huart->RxState == UART_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (huart->RxState != UART_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK_RX(huart);

	uint32_t tickstart = Get_Tick();

	huart->RxState = UART_STATE_BUSY;
	huart->ErrorCode = UART_ERROR_NONE;
	huart->pRxBuffPtr = pData;
	huart->RxXferSize = Size;
	huart->RxXferCount = 0;

	__HAL_UNLOCK_RX(huart);

	//接收数据
	while (huart->RxXferSize > 0U)
	{
        if(!UARTRxFifoStatusGet((uint32_t)huart->Instance, UART_FIFO_EMPTY))
        {
            *(huart->pRxBuffPtr) = UARTReadData((uint32_t)huart->Instance);
            huart->pRxBuffPtr++;
			huart->RxXferSize--;
			huart->RxXferCount++;
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
	huart->RxState = UART_STATE_READY;

	return errorcode;
}

/**
 * @brief  UART非阻塞发送API.
 * @param  huart 详情参考 @ref HAL_UART_HandleTypeDef.
 * @param  pData 发送数据缓冲区指针.
 * @param  Size  发送数据字节长度. 长度不得超过512字节，超过则断言.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示中断发送需要的参数配置、外设配置成功，等待进入中断发送数据
 *         @retval  HAL_ERROR    ：入参错误
 *         @retval  HAL_BUSY     ：外设正在使用中
 */
HAL_StatusTypeDef HAL_UART_Transmit_IT(HAL_UART_HandleTypeDef *huart, uint8_t *pData, uint32_t Size)
{
	if (huart->gState == UART_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (huart->gState != UART_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U))
	{
		return HAL_ERROR;
	}

    //malloc不允许过大，这里对长度做出限制，超过则断言
    xy_assert(Size <= 512);

	__HAL_LOCK(huart);

	huart->gState = UART_STATE_BUSY;
	huart->ErrorCode = UART_ERROR_NONE;
	huart->pTxBuffPtr = xy_malloc(sizeof(uint8_t) * Size);
	if (huart->pTxBuffPtr == NULL)
	{
		return HAL_ERROR;
	}
	memcpy(huart->pTxBuffPtr, pData, Size);
	huart->TxXferSize = Size;
	huart->TxXferCount = 0;

	__HAL_UNLOCK(huart);

    //确保发送该字节期间不要处理中断，若期间产生接收中断，则可能将发送中断标志位清除，导致后续无法继续中断发送
    //注意中断里会一起判断中断使能状态，所以需要一起锁。
    DisablePrimask();

    //发送1个数据以触发TXFIFO空中断
    UARTWriteData((uint32_t)huart->Instance, *huart->pTxBuffPtr);
    huart->TxXferSize--;
    huart->TxXferCount++;

	//使能中断源
    UARTIntEnable((uint32_t)huart->Instance, UART_INTTYPE_TX_FIFO_EMPTY);

    EnablePrimask();

	return HAL_OK;
}

/**
 * @brief  UART非阻塞接收API.
 * @param  huart 详情参考 @ref HAL_UART_HandleTypeDef.
 * @param  pData 接收数据缓冲区指针.
 * @param  Size  接收数据字节长度.
 * @param  HardTimeout 超时时间（单位ms），XY2100 UART提供硬件超时功能，在UART接收过程中，如果超过HardTimeout时间后没有新的数据到来，则触发超时中断.
 *         如果HardTimeout为HAL_MAX_DELAY，则表示不使用UART模块自身的硬件超时功能，只有在用户指定长度的数据全部收完时，才会触发接收完成回调函数的调用;
 *         如果Timeout为非0非HAL_MAX_DELAY，则表示设定并使用UART模块自身的硬件超时功能，当用户指定长度的数据没有全部收完时，若出现一段数据接收间隔大于设定的超时时间后就会触发接收完成回调函数的调用。
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示中断接收需要的参数配置、外设配置成功，等待进入中断接收数据
 *         @retval  HAL_ERROR    ：入参错误
 *         @retval  HAL_BUSY     ：外设正在使用中
 */
HAL_StatusTypeDef HAL_UART_Receive_IT(HAL_UART_HandleTypeDef *huart, uint8_t *pData, uint32_t Size, uint32_t HardTimeout)
{
	if (huart->RxState == UART_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (huart->RxState != UART_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U) || (HardTimeout == 0))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK_RX(huart);

	huart->RxState = UART_STATE_BUSY;
	huart->ErrorCode = UART_ERROR_NONE;
	huart->pRxBuffPtr = pData;
	huart->RxXferSize = Size;
	huart->RxXferCount = 0;

	__HAL_UNLOCK_RX(huart);

    //HardTimeout为非0非HAL_MAX_DELAY，开启接收阈值中断和接收超时中断
    if(HardTimeout != (uint32_t)HAL_MAX_DELAY)
    {
        HAL_UART_EnableTimeout_When_RxIdle_RxfifoNE(huart, HardTimeout);   
        UART_RXFIFO_LevelSet((uint32_t)huart->Instance, UART_FIFO_LEVEL_RX3_4); //固定rxfifo trigger level为 >= 32字节，RXFIFO 1/2深度
        UARTIntEnable((uint32_t)huart->Instance, UART_INTTYPE_RX_FIFO_THD | UART_INTTYPE_RX_TIMEOUT);
    }
    //HardTimeout为HAL_MAX_DELAY，只开启接收阈值中断，关闭接收超时中断
    else
    {
        UARTTimeOutDisable((uint32_t)huart->Instance);
        HAL_UART_SetRxFifoTrgLv_by_RxXferSize(huart); //根据RxXferSize设置rxfifo trigger level        
        UARTIntDisable((uint32_t)huart->Instance, UART_INTTYPE_RX_TIMEOUT);
        UARTIntEnable((uint32_t)huart->Instance, UART_INTTYPE_RX_FIFO_THD);
    }

	//使能其他异常中断
    UARTIntEnable((uint32_t)huart->Instance, UART_INTTYPE_FRAME_ERR | UART_INTTYPE_PARITY_ERR | UART_INTTYPE_RX_FIFO_OVF);

	return HAL_OK;
}

/**
 * @brief 死等UART中断标志位清除完成
 * @param huart 详情参考 @ref HAL_UART_HandleTypeDef.
 */
static void HAL_UART_Wait_IT_Cleared(HAL_UART_HandleTypeDef *huart)
{
    do {
        UART_IntClear((uint32_t)huart->Instance, UART_INT_ALL);
    } while (UARTIntRead((uint32_t)huart->Instance));
}

/**
 * @brief  UART中断服务函数.
 * @param  huart 详情参考 @ref HAL_UART_HandleTypeDef.
 */
static void HAL_UART_IRQHandler(HAL_UART_HandleTypeDef *huart)
{
    HAL_UART_IntTypeDef ITState = UARTIntRead((uint32_t)huart->Instance);
    HAL_UART_IntTypeDef ITEnable = UARTIntMasked((uint32_t)huart->Instance);

    //发送流程
	if ((ITState & UART_INTTYPE_TX_FIFO_EMPTY) && (ITEnable & UART_INTTYPE_TX_FIFO_EMPTY))
	{
        UART_IntClear((uint32_t)huart->Instance, UART_INTTYPE_TX_FIFO_EMPTY);
		//写发送数据至tx_fifo，注意：全部写入tx_fifo不代表数据都发出去了
		if (huart->TxXferSize != 0)
		{
            while(!UARTTxFifoStatusGet((uint32_t)huart->Instance, UART_FIFO_FULL))
            {
                UARTWriteData((uint32_t)huart->Instance, *(huart->pTxBuffPtr + huart->TxXferCount));
                huart->TxXferSize--;
                huart->TxXferCount++;
                if (huart->TxXferSize == 0)
                {
					break;
                }
            }
        }
        //数据全部写入tx_fifo且tx_fifo空，此时调用TxCpltCallback
		else
		{
            xy_free(huart->pTxBuffPtr);
			UARTIntDisable((uint32_t)huart->Instance, UART_INTTYPE_TX_FIFO_EMPTY);
            HAL_UART_TxFifoEmptyDelay(huart);	
			huart->gState = UART_STATE_READY;
            HAL_UART_TxCpltCallback(huart);
		}
    }

    //接收异常流程
    if ((ITState & UART_INTTYPE_FRAME_ERR) && (ITEnable & UART_INTTYPE_FRAME_ERR))
	{
		huart->ErrorCode |= UART_ERROR_FRAME_ERR;
        UART_IntClear((uint32_t)huart->Instance, UART_INTTYPE_FRAME_ERR);
        UARTIntDisable((uint32_t)huart->Instance, UART_INTTYPE_FRAME_ERR);
    }
    if ((ITState & UART_INTTYPE_PARITY_ERR) && (ITEnable & UART_INTTYPE_PARITY_ERR))
	{
		huart->ErrorCode |= UART_ERROR_PARITY_ERR;
        UART_IntClear((uint32_t)huart->Instance, UART_INTTYPE_PARITY_ERR);
        UARTIntDisable((uint32_t)huart->Instance, UART_INTTYPE_PARITY_ERR);
	}
    if ((ITState & UART_INTTYPE_RX_FIFO_OVF) && (ITEnable & UART_INTTYPE_RX_FIFO_OVF))
	{
		huart->ErrorCode |= UART_ERROR_RX_FIFO_OVF;
        UART_IntClear((uint32_t)huart->Instance, UART_INTTYPE_RX_FIFO_OVF);
        UARTIntDisable((uint32_t)huart->Instance, UART_INTTYPE_RX_FIFO_OVF);
	}
    //产生错误后不用再去执行正常的接收流程，因此需要退出中断服务函数
    if(huart->ErrorCode != UART_ERROR_NONE)
    {
        huart->RxState = UART_STATE_ERROR;
        HAL_UART_ErrorCallback(huart);
        return;
    }

    //接收流程：HardTimeout为非0非HAL_MAX_DELAY，开启接收阈值中断和接收超时中断
    if (ITEnable & UART_INTTYPE_RX_TIMEOUT)
    {
        if (((ITState & UART_INTTYPE_RX_FIFO_THD) && (ITEnable & UART_INTTYPE_RX_FIFO_THD)) || \
            ((ITState & UART_INTTYPE_RX_TIMEOUT) && (ITEnable & UART_INTTYPE_RX_TIMEOUT)))
        {
            UART_IntClear((uint32_t)huart->Instance, UART_INTTYPE_RX_FIFO_THD | UART_INTTYPE_RX_TIMEOUT);
            while(!UARTRxFifoStatusGet((uint32_t)huart->Instance, UART_FIFO_EMPTY))
            {
				//RXFIFO非空且原硬件超时是RXFIFO非空超时时，则切换硬件超时为RXIDLE超时
                if(UART_TimeoutCondition_Get((uint32_t)huart->Instance))
				{
					HAL_UART_EnableTimeout_When_RxIdle(huart);
				}
                *(huart->pRxBuffPtr) = UARTReadData((uint32_t)huart->Instance);
                huart->pRxBuffPtr++;
                huart->RxXferSize--;
                huart->RxXferCount++;
                if (huart->RxXferSize == 0U)
                {
                    break;
                }
            }
            //接收超时或者用户指定数据长度接收完成时，则执行接收完成回调
            if ((ITState & UART_INTTYPE_RX_TIMEOUT) || (huart->RxXferSize == 0))
            {
				UARTTimeOutDisable((uint32_t)huart->Instance);
                UARTIntDisable((uint32_t)huart->Instance, UART_INTTYPE_RX_FIFO_THD | UART_INTTYPE_RX_TIMEOUT);
                HAL_UART_Wait_IT_Cleared(huart); //解决问题：当波特率很高(921600)且收到的串口数据较长时，进入接收完成回调前会带一个超时中断标志位，当再次调用HAL_Receive_IT时会立即再次进入接收完成回调，此时实际接收数据长度为0
                huart->RxState = UART_STATE_READY;
                HAL_UART_RxCpltCallback(huart);
            }
        }
    }

    //接收流程：HardTimeout为HAL_MAX_DELAY，只开启接收阈值中断，关闭接收超时中断
    else if ((ITState & UART_INTTYPE_RX_FIFO_THD ) && (ITEnable & UART_INTTYPE_RX_FIFO_THD))
    {
        UART_IntClear((uint32_t)huart->Instance, UART_INTTYPE_RX_FIFO_THD);
        while(!UARTRxFifoStatusGet((uint32_t)huart->Instance, UART_FIFO_EMPTY))
        {
            *(huart->pRxBuffPtr) = UARTReadData((uint32_t)huart->Instance);
            huart->pRxBuffPtr++;
            huart->RxXferSize--;
            huart->RxXferCount++;
            if (huart->RxXferSize == 0U)
            {
                UARTIntDisable((uint32_t)huart->Instance, UART_INTTYPE_RX_FIFO_THD);
                huart->RxState = UART_STATE_READY;
                HAL_UART_RxCpltCallback(huart);
                break;
            }
        }
        HAL_UART_SetRxFifoTrgLv_by_RxXferSize(huart); //根据RxXferSize设置rxfifo trigger level
    }
}