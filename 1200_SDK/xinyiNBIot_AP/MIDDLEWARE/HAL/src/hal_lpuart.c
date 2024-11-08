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

#include "hal_lpuart.h"
#include "xy_system.h"

extern uint8_t g_lpuart_used; // 1：LPUART在芯片低功耗模式下供电打开，且为低功耗模式唤醒源；0：LPUART在芯片低功耗模式下供电关闭，且不为低功耗模式唤醒源

HAL_LPUART_HandleTypeDef *g_lpuart_handle = NULL; //指向当前句柄
static void HAL_LPUART_IRQHandler(HAL_LPUART_HandleTypeDef *hlpuart);

/**
 * @brief LPUART0中断服务函数.
 */
static void HAL_LPUART0_IRQHandler(void)
{
    if(g_lpuart_handle != NULL)
    {
        HAL_LPUART_IRQHandler(g_lpuart_handle);
    }
}

/**
 * @brief  LPUART中断类型枚举
 */
typedef enum
{
    LPUART_INTTYPE_FRAME_ERR = UART_INT_RX_FRAME_ERR,      //LPUART帧错误中断
    LPUART_INTTYPE_PARITY_ERR = UART_INT_RX_PAR_ERR,       //LPUART校验错误中断
    LPUART_INTTYPE_RX_FIFO_OVF = UART_INT_RXFIFO_OVFLW,    //LPUART接收溢出中断
    LPUART_INTTYPE_RX_TIMEOUT = UART_INT_TIMEOUT,          //LPUART接收超时中断
    LPUART_INTTYPE_RX_FIFO_THD = UART_INT_RXFIFO_TRIGGER,  //LPUART接收阈值中断，当RXFIFO现存量处于接收阈值区间时触发，接收阈值区间范围是小于上限、大于等于下限
	LPUART_INTTYPE_TX_FIFO_EMPTY = UART_INT_TXFIFO_EMPTY   //LPUARTTXFIFO空中断
} HAL_LPUART_IntTypeDef;

/**
 * @brief  使能LPUART硬件超时功能，并设置超时时间，超时倒计时触发条件为RXD处于IDLE且RXFIFO非空.
 * @param  hlpuart 详情参考 @ref HAL_LPUART_HandleTypeDef.
 * @param  timeout(ms) 超时时间，用户可根据实际接收需求设置超时时间.
 */
static void HAL_LPUART_EnableTimeout_When_RxIdle_RxfifoNE(HAL_LPUART_HandleTypeDef *hlpuart, uint32_t timeout)
{
	uint32_t reg_val = timeout * hlpuart->Init.BaudRate / 1000;
	if ((reg_val <= 0x00) || (reg_val > 0x1F))
	{
		reg_val = 0x1F;
	}
    UARTTimeOutDisable((uint32_t)hlpuart->Instance);
	UARTTimeOutConfig((uint32_t)hlpuart->Instance, UART_RX_TIMEOUT_START_FIFO_NEMPTY, reg_val);
	UARTTimeOutEnable((uint32_t)hlpuart->Instance);
}

/**
 * @brief  使能LPUART硬件超时功能，超时倒计时触发条件为RXD处于IDLE.
 * @param  hlpuart 详情参考 @ref HAL_LPUART_HandleTypeDef.
 * @note   触发后必须再次调用该接口才能再次生效.
 */
static void HAL_LPUART_EnableTimeout_When_RxIdle(HAL_LPUART_HandleTypeDef *hlpuart)
{
    UARTTimeOutDisable((uint32_t)hlpuart->Instance);
    UARTTimeOutCondition_Set((uint32_t)hlpuart->Instance, UART_RX_TIMEOUT_START_NO_MATTER);
    UARTTimeOutEnable((uint32_t)hlpuart->Instance);
}

/**
 * @brief 根据RxXferSize设置rxfifo trigger level
 * @param hlpuart 详情参考 @ref HAL_LPUART_HandleTypeDef.
 */
static void HAL_LPUART_SetRxFifoTrgLv_by_RxXferSize(HAL_LPUART_HandleTypeDef *hlpuart)
{
	#define RXFIFO_1_4_DEPTH (8) //RXFIFO 1/4深度（字节数）

	uint32_t trglv = 0;
	if(hlpuart->RxXferSize < RXFIFO_1_4_DEPTH)
	{
		trglv = UART_FIFO_LEVEL_RX1_4;
	}
	else if((hlpuart->RxXferSize >= RXFIFO_1_4_DEPTH) && (hlpuart->RxXferSize < (2*RXFIFO_1_4_DEPTH)))
	{
		trglv = UART_FIFO_LEVEL_RX2_4;
	}
	else if((hlpuart->RxXferSize >= (2*RXFIFO_1_4_DEPTH)) && (hlpuart->RxXferSize < (3*RXFIFO_1_4_DEPTH)))
	{
		trglv = UART_FIFO_LEVEL_RX3_4;
	}
	else if(hlpuart->RxXferSize >= (3*RXFIFO_1_4_DEPTH))
	{
		trglv = UART_FIFO_LEVEL_RX4_4;
	}

    if(hlpuart->RxXferSize)
    {
        //设置RXFIFO触发阈值：当前fifo空为level 0，若想使得UART_FIFO_LEVEL_TX1_4(level 0)有效触发，
        //必须先让fifo level != trigger level，再让fifo level == trigger level
        UART_RXFIFO_LevelSet((uint32_t)hlpuart->Instance, UART_FIFO_LEVEL_RX4_4);
        UART_RXFIFO_LevelSet((uint32_t)hlpuart->Instance, trglv);
    }
}

/**
 * @brief 根据波特率增加1个字符的延时，以保证数据发送完成
 * @param hlpuart 详情参考 @ref HAL_LPUART_HandleTypeDef.
 */
static void HAL_LPUART_TxFifoEmptyDelay(HAL_LPUART_HandleTypeDef *hlpuart)
{
	uint32_t onechar_timeout = (uint32_t)((15 * 1000 * 1000 / hlpuart->Init.BaudRate) + 1); //按15bit计算，单位us
    if(onechar_timeout < 50)
    {
        onechar_timeout = 50;
    }
	HAL_Delay_US(onechar_timeout);
}

/**
 * @brief  初始化 LPUART.
 * @param  hlpuart 详情参考 @ref HAL_LPUART_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 */
__FLASH_FUNC HAL_StatusTypeDef HAL_LPUART_Init(HAL_LPUART_HandleTypeDef *hlpuart)
{
	if ((hlpuart == NULL) || (hlpuart->Instance != LPUART))
	{
		return HAL_ERROR;
	}

    if (hlpuart->gState == LPUART_STATE_RESET) 
    {
        hlpuart->Lock = HAL_UNLOCKED;
        hlpuart->RxLock = HAL_UNLOCKED;
    }
    else
    {
        return HAL_ERROR;
    }

    hlpuart->gState = LPUART_STATE_BUSY;

    //配置LPUART时钟源与时钟分频系数
    PRCM_LPUA1_ClkSet_by_BaudRate(hlpuart->Init.BaudRate);

    //配置LPUART的引脚
    PRCM_LPUA1_PadSel(hlpuart->Init.PadSel);

    //开LPUART时钟
    PRCM_ClockEnable(CORE_CKG_CTL_LPUART_EN);

    //如果LPUART没有使能，则配置LPUART
    if(UART_Enable_Get((uint32_t)hlpuart->Instance) == 0)
    {
    	UARTDisable((uint32_t)hlpuart->Instance); //关闭LPUART
        UARTIntDisable((uint32_t)hlpuart->Instance, UART_INT_ALL); //关闭所有中断源
        UART_IntClear((uint32_t)hlpuart->Instance, UART_INT_ALL);  // 清所有中断标志位
        UARTFIFOFlush((uint32_t)hlpuart->Instance, UART_FIFO_ALL); // 清空TXFIFO、RXFIFO
        UARTConfigSetExpClk((uint32_t)hlpuart->Instance, GetLpuartClockFreq(), hlpuart->Init.BaudRate, hlpuart->Init.WordLength | hlpuart->Init.Parity | UART_CTL_ENDIAN_LITTLE);
        UARTTxWaitSet((uint32_t)hlpuart->Instance, 0); //设置发送间隔为0
		//PRCM_LPUA_PWR_Ctl(LPUA_ANY_MODE_ON);        //lpuart32k 需要force on
    }
	
    //如果LPUART使能，且发送没使能，则表示LPUART的供电设置为 LPUA_DEEPSLEEP_MODE_OFF 。
    //在该供电模式下，一旦芯片进入睡眠，LPUART的发送和其他部分配置就不再保持，因此唤醒后需要重配这部分设置。
    /* 
    在芯片进入睡眠后，LPUART可以保持的配置如下：
	(1) LPUART->CTRL = 0x03; //LPUART使能位、RX使能位保持，TX使能位不保持
    (2) LPUART->ABDEN = 0x00;
    (3) LPUART->BAUDRATE_DIV = 0x0F1FFF;
    (4) LPUART->WAKEUP = 0xFFFF3F;
    (5) LPUART->RX_TIMEOUT_CFG = 0x1F03;
    (6) LPUART->START_OFFSET_CFG = 0x3F01;
    */
	else if(UART_TxEnable_Get((uint32_t)hlpuart->Instance) == 0)
	{
        UARTTxWaitSet((uint32_t)hlpuart->Instance, 0); //设置发送间隔为0
        UART_ConfigRegister_Set((uint32_t)hlpuart->Instance, hlpuart->Init.WordLength | hlpuart->Init.Parity | UART_CTL_ENDIAN_LITTLE);
        UARTFIFOEnable((uint32_t)hlpuart->Instance, UART_FIFO_TX); //使能TX
	}

    //硬件流控功能配置
    UARTFlowCtrlDisable((uint32_t)hlpuart->Instance);
    UARTIntDisable((uint32_t)hlpuart->Instance, UART_INT_FLOW_CTL);
    if (hlpuart->Init.HfcLevel != LPUART_HFC_LEVEL_NONE)
    {
        if (hlpuart->Init.HfcLevel == LPUART_HFC_LEVEL_LOW) //设置有效电平
        {
            UARTFlowCtrlConfig((uint32_t)hlpuart->Instance, UART_FLOW_CTL_AUTO | UART_FLOW_CTL_VALID_LEVEL_HIGH | UART_FLOW_CTL_RTS_TRIGGER_LEVEL_3_4); //RXFIFO现存量大于24字节时触发RTS无效电平
        }
        else
        {
            UARTFlowCtrlConfig((uint32_t)hlpuart->Instance, UART_FLOW_CTL_AUTO | UART_FLOW_CTL_VALID_LEVEL_LOW | UART_FLOW_CTL_RTS_TRIGGER_LEVEL_3_4);  //RXFIFO现存量大于24字节时触发RTS无效电平
        }
        UARTFlowCtrlEnable((uint32_t)hlpuart->Instance);
    }

#if (MODULE_VER == 0x0)	// opencpu,避免深睡前配置耗时
    PRCM_LPUA_PWR_Ctl(LPUA_ANY_MODE_ON); // force on in deesleep,lpuart和lptim中有一个维持工作，则需要维持外设寄存器不掉电。
#endif

    //注册中断向量
    NVIC_IntRegister(LPUART_IRQn, HAL_LPUART0_IRQHandler, 1);

    //记录句柄
    g_lpuart_handle = hlpuart;

	hlpuart->ErrorCode = LPUART_ERROR_NONE;
	hlpuart->gState = LPUART_STATE_READY;
	hlpuart->RxState = LPUART_STATE_READY;

	g_lpuart_used = 1;

	return HAL_OK;
}

/**
 * @brief  去初始化LPUART.
 * @param  hlpuart 详情参考 @ref HAL_LPUART_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 */
HAL_StatusTypeDef HAL_LPUART_DeInit(HAL_LPUART_HandleTypeDef *hlpuart)
{
	if ((hlpuart == NULL) || (hlpuart->Instance != LPUART))
	{
		return HAL_ERROR;
	}

	if (hlpuart->gState == LPUART_STATE_RESET)
	{
		return HAL_ERROR;
	}

	hlpuart->gState = LPUART_STATE_BUSY;

    //配置LPUART时钟源与时钟分频系数
    PRCM_LPUA1_ClkSet_by_BaudRate(hlpuart->Init.BaudRate);

    //配置LPUART的引脚
    PRCM_LPUA1_PadSel(hlpuart->Init.PadSel);

    //开LPUART时钟
    PRCM_ClockEnable(CORE_CKG_CTL_LPUART_EN);

    //禁能TX、RX，禁能UART
    UARTFIFODisable((uint32_t)hlpuart->Instance, UART_FIFO_ALL);
    UARTDisable((uint32_t)hlpuart->Instance);

    //去注册中断向量
    NVIC_IntUnregister(LPUART_IRQn);

    //释放句柄
    g_lpuart_handle = NULL;

    hlpuart->ErrorCode = LPUART_ERROR_NONE;
	hlpuart->gState = LPUART_STATE_RESET;
    hlpuart->RxState = LPUART_STATE_RESET;

	__HAL_UNLOCK(hlpuart);
	__HAL_UNLOCK_RX(hlpuart);

	g_lpuart_used = 0;

	return HAL_OK;
}

/**
 * @brief  LPUART阻塞发送API.
 * @param  hlpuart 详情参考 @ref HAL_LPUART_HandleTypeDef.
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
 * @note   hlpuart结构体中的TxXferCount表示实际发送的字节数.
 */
HAL_StatusTypeDef HAL_LPUART_Transmit(HAL_LPUART_HandleTypeDef *hlpuart, uint8_t *pData, uint32_t Size, uint32_t Timeout)
{
	HAL_StatusTypeDef errorcode = HAL_OK;

	if (hlpuart->gState == LPUART_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hlpuart->gState != LPUART_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U) || (Timeout == 0))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK(hlpuart);

	uint32_t tickstart = Get_Tick();

	hlpuart->gState = LPUART_STATE_BUSY;
	hlpuart->ErrorCode = LPUART_ERROR_NONE;
	hlpuart->pTxBuffPtr = pData;
	hlpuart->TxXferSize = Size;
	hlpuart->TxXferCount = 0;

	__HAL_UNLOCK(hlpuart);

	//发送数据
	while (hlpuart->TxXferSize > 0U)
	{
        if(!LPUART_IS_TXFIFO_FULL())
        // if(!UARTTxFifoStatusGet((uint32_t)hlpuart->Instance, UART_FIFO_FULL))
		{
            UARTWriteData((uint32_t)hlpuart->Instance, *(hlpuart->pTxBuffPtr + hlpuart->TxXferCount));
			hlpuart->TxXferSize--;
			hlpuart->TxXferCount++;
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
    while (!LPUART_IS_TXFIFO_EMPTY())
	// while (!UARTTxFifoStatusGet((uint32_t)hlpuart->Instance, UART_FIFO_EMPTY))
	{
		if ((Timeout != HAL_MAX_DELAY) && Check_Ms_Timeout(tickstart,Timeout))
		{
			errorcode = HAL_TIMEOUT;
			goto error;
		}
	}

	//根据波特率增加1个字符的延时，以保证数据发送完成
	HAL_LPUART_TxFifoEmptyDelay(hlpuart);   

error:
	hlpuart->gState = LPUART_STATE_READY;

	return errorcode;
}

/**
 * @brief  LPUART阻塞接收API.
 * @param  hlpuart 详情参考 @ref HAL_LPUART_HandleTypeDef.
 * @param  pData 接收数据缓冲区指针.
 * @param  Size  接收数据字节长度.
 * @param  Timeout 超时时间（单位ms）;
 *         如果Timeout为0，则是不阻塞，检测到接收FIFO空后，则立刻退出;
 *         如果Timeout为HAL_MAX_DELAY，则是一直阻塞，直到接收到指定数量的字节后才退出;
 *         如果Timeout为非0非HAL_MAX_DELAY时，表示函数等待数据接收完成的最长时间，超时则退出函数.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：当Timeout未置为0时，表示指定时间内成功接收指定数量的数据；当Timeout置为0时，也会返回OK，但实际接收数据的数量通过hlpuart结构体中的RxXferCount来确定
 *         @retval  HAL_ERROR    ：入参错误
 *         @retval  HAL_BUSY     ：外设正在使用中
 *         @retval  HAL_TIMEOUT  ：针对Timeout未置为0时，表示指定时间内未能成功接收指定数量的数据
 */
HAL_StatusTypeDef HAL_LPUART_Receive(HAL_LPUART_HandleTypeDef *hlpuart, uint8_t *pData, uint32_t Size, uint32_t Timeout)
{
	HAL_StatusTypeDef errorcode = HAL_OK;

	if (hlpuart->RxState == LPUART_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hlpuart->RxState != LPUART_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK_RX(hlpuart);

	uint32_t tickstart = Get_Tick();

	hlpuart->RxState = LPUART_STATE_BUSY;
	hlpuart->ErrorCode = LPUART_ERROR_NONE;
	hlpuart->pRxBuffPtr = pData;
	hlpuart->RxXferSize = Size;
	hlpuart->RxXferCount = 0;

	__HAL_UNLOCK_RX(hlpuart);

	//接收数据
	while (hlpuart->RxXferSize > 0U)
	{
        if(!UARTRxFifoStatusGet((uint32_t)hlpuart->Instance, UART_FIFO_EMPTY))
        {
			*(hlpuart->pRxBuffPtr) = UARTReadData((uint32_t)hlpuart->Instance);
			hlpuart->pRxBuffPtr++;
			hlpuart->RxXferSize--;
			hlpuart->RxXferCount++;
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
	hlpuart->RxState = LPUART_STATE_READY;

	return errorcode;
}

/**
 * @brief  UART非阻塞发送API.
 * @param  hlpuart 详情参考 @ref HAL_LPUART_HandleTypeDef.
 * @param  pData 发送数据字节长度. 长度不得超过512字节，超过则断言.
 * @param  Size  发送数据字节长度.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示中断发送需要的参数配置、外设配置成功，等待进入中断发送数据
 *         @retval  HAL_ERROR    ：入参错误
 *         @retval  HAL_BUSY     ：外设正在使用中
 */
HAL_StatusTypeDef HAL_LPUART_Transmit_IT(HAL_LPUART_HandleTypeDef *hlpuart, uint8_t *pData, uint32_t Size)
{
	if (hlpuart->gState == LPUART_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hlpuart->gState != LPUART_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U))
	{
		return HAL_ERROR;
	}

    //malloc不允许过大，这里对长度做出限制，超过则断言
    xy_assert(Size <= 512);

	__HAL_LOCK(hlpuart);

	hlpuart->gState = LPUART_STATE_BUSY;
	hlpuart->ErrorCode = LPUART_ERROR_NONE;
	hlpuart->pTxBuffPtr = xy_malloc(sizeof(uint8_t) * Size);
	if (hlpuart->pTxBuffPtr == NULL)
	{
		return HAL_ERROR;
	}
	memcpy(hlpuart->pTxBuffPtr, pData, Size);
	hlpuart->TxXferSize = Size;
	hlpuart->TxXferCount = 0;

	__HAL_UNLOCK(hlpuart);

    //确保发送该字节期间不要处理中断，若期间产生接收中断，则可能将发送中断标志位清除，导致后续无法继续中断发送
    //注意中断里会一起判断中断使能状态，所以需要一起锁。
    DisablePrimask();

    //发送1个数据以触发TXFIFO空中断
    UARTWriteData((uint32_t)hlpuart->Instance, *hlpuart->pTxBuffPtr);
    hlpuart->TxXferSize--;
    hlpuart->TxXferCount++;

	//使能中断源
    UARTIntEnable((uint32_t)hlpuart->Instance, LPUART_INTTYPE_TX_FIFO_EMPTY);

    EnablePrimask();

	return HAL_OK;
}

/**
 * @brief  UART非阻塞接收API.
 * @param  hlpuart 详情参考 @ref HAL_LPUART_HandleTypeDef.
 * @param  pData 接收数据缓冲区指针.
 * @param  Size  接收数据字节长度.
 * @param  HardTimeout 超时时间（单位ms），XY2100 LPUART提供硬件超时功能，在LPUART接收过程中，如果超过HardTimeout时间后没有新的数据到来，则触发超时中断.
 *         如果HardTimeout为HAL_MAX_DELAY，则表示不使用LPUART模块自身的硬件超时功能，只有在用户指定长度的数据全部收完时，才会触发接收完成回调函数的调用;
 *         如果Timeout为非0非HAL_MAX_DELAY，则表示设定并使用LPUART模块自身的硬件超时功能，当用户指定长度的数据没有全部收完时，若出现一段数据接收间隔大于设定的超时时间后就会触发接收完成回调函数的调用。
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示中断接收需要的参数配置、外设配置成功，等待进入中断接收数据
 *         @retval  HAL_ERROR    ：入参错误
 *         @retval  HAL_BUSY     ：外设正在使用中
 */
HAL_StatusTypeDef HAL_LPUART_Receive_IT(HAL_LPUART_HandleTypeDef *hlpuart, uint8_t *pData, uint32_t Size, uint32_t HardTimeout)
{
    if (hlpuart->RxState == LPUART_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hlpuart->RxState != LPUART_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U) || (HardTimeout == 0))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK_RX(hlpuart);

	hlpuart->RxState = LPUART_STATE_BUSY;
	hlpuart->ErrorCode = LPUART_ERROR_NONE;
	hlpuart->pRxBuffPtr = pData;
	hlpuart->RxXferSize = Size;
	hlpuart->RxXferCount = 0;

	__HAL_UNLOCK_RX(hlpuart);

    //HardTimeout为非0非HAL_MAX_DELAY，开启接收阈值中断和接收超时中断
    if(HardTimeout != (uint32_t)HAL_MAX_DELAY)
    {
        HAL_LPUART_EnableTimeout_When_RxIdle_RxfifoNE(hlpuart, HardTimeout);   
        UART_RXFIFO_LevelSet((uint32_t)hlpuart->Instance, UART_FIFO_LEVEL_RX3_4); //固定rxfifo trigger level为 >= 16字节，RXFIFO 1/2深度
        UARTIntEnable((uint32_t)hlpuart->Instance, LPUART_INTTYPE_RX_FIFO_THD | LPUART_INTTYPE_RX_TIMEOUT);
    }
    //HardTimeout为HAL_MAX_DELAY，只开启接收阈值中断，关闭接收超时中断
    else
    {
        UARTTimeOutDisable((uint32_t)hlpuart->Instance);
        HAL_LPUART_SetRxFifoTrgLv_by_RxXferSize(hlpuart); //根据RxXferSize设置rxfifo trigger level        
        UARTIntDisable((uint32_t)hlpuart->Instance, LPUART_INTTYPE_RX_TIMEOUT);
        UARTIntEnable((uint32_t)hlpuart->Instance, LPUART_INTTYPE_RX_FIFO_THD);
    }

	//使能其他异常中断
    UARTIntEnable((uint32_t)hlpuart->Instance, LPUART_INTTYPE_FRAME_ERR | LPUART_INTTYPE_PARITY_ERR | LPUART_INTTYPE_RX_FIFO_OVF);

	return HAL_OK;
}

/**
 * @brief 死等UART中断标志位清除完成
 * @param hlpuart 详情参考 @ref HAL_LPUART_HandleTypeDef.
 */
static void HAL_LPUART_Wait_IT_Cleared(HAL_LPUART_HandleTypeDef *hlpuart)
{
    do {
        UART_IntClear((uint32_t)hlpuart->Instance, UART_INT_ALL);
    } while (UARTIntRead((uint32_t)hlpuart->Instance));
}

/**
 * @brief  LPUART中断服务函数.
 * @param  hlpuart 详情参考 @ref HAL_LPUART_HandleTypeDef.
 */
#if (AT_LPUART == 1) 
extern volatile uint32_t g_at_last_recv_tick;
#endif
static void HAL_LPUART_IRQHandler(HAL_LPUART_HandleTypeDef *hlpuart)
{
    HAL_LPUART_IntTypeDef ITState = UARTIntRead((uint32_t)hlpuart->Instance);
	HAL_LPUART_IntTypeDef ITEnable = UARTIntMasked((uint32_t)hlpuart->Instance);

    //发送流程
	if ((ITState & LPUART_INTTYPE_TX_FIFO_EMPTY) && (ITEnable & LPUART_INTTYPE_TX_FIFO_EMPTY))
	{
        UART_IntClear((uint32_t)hlpuart->Instance, LPUART_INTTYPE_TX_FIFO_EMPTY);
		//写发送数据至tx_fifo，注意：全部写入tx_fifo不代表数据都发出去了
		if (hlpuart->TxXferSize != 0)
		{
            while (!LPUART_IS_TXFIFO_FULL())
            // while(!UARTTxFifoStatusGet((uint32_t)hlpuart->Instance, UART_FIFO_FULL))
            {
                UARTWriteData((uint32_t)hlpuart->Instance, *(hlpuart->pTxBuffPtr + hlpuart->TxXferCount));
                hlpuart->TxXferSize--;
                hlpuart->TxXferCount++;
                if (hlpuart->TxXferSize == 0)
                {
					break;
                }
            }
        }
        //数据全部写入tx_fifo且tx_fifo空，此时调用TxCpltCallback
		else
		{
            xy_free(hlpuart->pTxBuffPtr);
			UARTIntDisable((uint32_t)hlpuart->Instance, LPUART_INTTYPE_TX_FIFO_EMPTY);
            HAL_LPUART_TxFifoEmptyDelay(hlpuart);	
			hlpuart->gState = LPUART_STATE_READY;
            HAL_LPUART_TxCpltCallback(hlpuart);
		}
    }

    //接收异常流程
    if ((ITState & LPUART_INTTYPE_FRAME_ERR) && (ITEnable & LPUART_INTTYPE_FRAME_ERR))
	{
		hlpuart->ErrorCode |= LPUART_ERROR_FRAME_ERR;
        UART_IntClear((uint32_t)hlpuart->Instance, LPUART_INTTYPE_FRAME_ERR);
        UARTIntDisable((uint32_t)hlpuart->Instance, LPUART_INTTYPE_FRAME_ERR);
    }
    if ((ITState & LPUART_INTTYPE_PARITY_ERR) && (ITEnable & LPUART_INTTYPE_PARITY_ERR))
	{
		hlpuart->ErrorCode |= LPUART_ERROR_PARITY_ERR;
        UART_IntClear((uint32_t)hlpuart->Instance, LPUART_INTTYPE_PARITY_ERR);
        UARTIntDisable((uint32_t)hlpuart->Instance, LPUART_INTTYPE_PARITY_ERR);
	}
    if ((ITState & LPUART_INTTYPE_RX_FIFO_OVF) && (ITEnable & LPUART_INTTYPE_RX_FIFO_OVF))
	{
		hlpuart->ErrorCode |= LPUART_ERROR_RX_FIFO_OVF;
        UART_IntClear((uint32_t)hlpuart->Instance, LPUART_INTTYPE_RX_FIFO_OVF);
        UARTIntDisable((uint32_t)hlpuart->Instance, LPUART_INTTYPE_RX_FIFO_OVF);
	}
    //产生错误后不用再去执行正常的接收流程，因此需要退出中断服务函数
    if(hlpuart->ErrorCode != LPUART_ERROR_NONE)
    {
        hlpuart->RxState = LPUART_STATE_ERROR;
        HAL_LPUART_ErrorCallback(hlpuart);
        return;
    }

    //接收流程：HardTimeout为非0非HAL_MAX_DELAY，开启接收阈值中断和接收超时中断
    if (ITEnable & LPUART_INTTYPE_RX_TIMEOUT)
    {
        if (((ITState & LPUART_INTTYPE_RX_FIFO_THD) && (ITEnable & LPUART_INTTYPE_RX_FIFO_THD)) || \
            ((ITState & LPUART_INTTYPE_RX_TIMEOUT) && (ITEnable & LPUART_INTTYPE_RX_TIMEOUT)))
        {
            UART_IntClear((uint32_t)hlpuart->Instance, LPUART_INTTYPE_RX_FIFO_THD | LPUART_INTTYPE_RX_TIMEOUT);
            while(!UARTRxFifoStatusGet((uint32_t)hlpuart->Instance, UART_FIFO_EMPTY))
            {
				//RXFIFO非空且原硬件超时是RXFIFO非空超时时，则切换硬件超时为RXIDLE超时
                if(UART_TimeoutCondition_Get((uint32_t)hlpuart->Instance))
				{
					HAL_LPUART_EnableTimeout_When_RxIdle(hlpuart);
				}

#if (AT_LPUART == 1) 
                g_at_last_recv_tick = Get_Tick();//连续数据时只会触发阈值中断，此时及时更新tick
#endif

				*(hlpuart->pRxBuffPtr) = UARTReadData((uint32_t)hlpuart->Instance);
                hlpuart->pRxBuffPtr++;
                hlpuart->RxXferSize--;
                hlpuart->RxXferCount++;
				if (hlpuart->RxXferSize == 0)
                {
                    break;
                }
            }
            //接收超时或者用户指定数据长度接收完成时，则执行接收完成回调
            if ((ITState & LPUART_INTTYPE_RX_TIMEOUT) || (hlpuart->RxXferSize == 0))
            {
				UARTTimeOutDisable((uint32_t)(hlpuart->Instance));
                UARTIntDisable((uint32_t)hlpuart->Instance, LPUART_INTTYPE_RX_FIFO_THD | LPUART_INTTYPE_RX_TIMEOUT);
                HAL_LPUART_Wait_IT_Cleared(hlpuart); //解决问题：当波特率很高(921600)且收到的串口数据较长时，进入接收完成回调前会带一个超时中断标志位，当再次调用HAL_Receive_IT时会立即再次进入接收完成回调，此时实际接收数据长度为0
                hlpuart->RxState = LPUART_STATE_READY;
                HAL_LPUART_RxCpltCallback(hlpuart);
            }
        }
    }
    
    //接收流程：HardTimeout为HAL_MAX_DELAY，只开启接收阈值中断，关闭接收超时中断
    else if ((ITState & LPUART_INTTYPE_RX_FIFO_THD ) && (ITEnable & LPUART_INTTYPE_RX_FIFO_THD))
    {
        UART_IntClear((uint32_t)hlpuart->Instance, LPUART_INTTYPE_RX_FIFO_THD);
        while(!UARTRxFifoStatusGet((uint32_t)hlpuart->Instance, UART_FIFO_EMPTY))
        {

#if (AT_LPUART == 1)
            g_at_last_recv_tick = Get_Tick();//连续数据时只会触发阈值中断，此时及时更新tick
#endif

            *(hlpuart->pRxBuffPtr) = UARTReadData((uint32_t)hlpuart->Instance);
            hlpuart->pRxBuffPtr++;
            hlpuart->RxXferSize--;
            hlpuart->RxXferCount++;
            if (hlpuart->RxXferSize == 0U)
            {
                UARTIntDisable((uint32_t)hlpuart->Instance, LPUART_INTTYPE_RX_FIFO_THD);
                hlpuart->RxState = LPUART_STATE_READY;
                HAL_LPUART_RxCpltCallback(hlpuart);
                break;
            }
        }
        HAL_LPUART_SetRxFifoTrgLv_by_RxXferSize(hlpuart); //根据RxXferSize设置rxfifo trigger level
    }
}
