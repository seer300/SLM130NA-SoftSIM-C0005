/**
 ******************************************************************************
 * @file    hal_csp.c
 * @brief   此文件包含CSP外设的函数定义等.
 ******************************************************************************
 */

#include "hal_csp.h"
#include "sys_clk.h"
#include "hal_def.h"
#include "nvic.h"

/**
 * @brief  CSP 中断类型枚举.
 */
typedef enum
{
    HAL_CSP_INTTYPE_UART_FRAME_ERR = CSP_INT_UART_FRM_ERR,           /*!< CSP_UART帧错误中断 */
    HAL_CSP_INTTYPE_UART_PARITY_ERR = CSP_INT_UART_PARITYCHECK_ERR,  /*!< CSP_UART校验错误中断*/
    HAL_CSP_INTTYPE_RX_FIFO_OVF = CSP_INT_RX_OFLOW,                  /*!< CSP接收溢出中断 */
    HAL_CSP_INTTYPE_RX_ONE_BYTE = CSP_INT_RX_DONE,                   /*!< CSP 1字节接收完成中断 */
    HAL_CSP_INTTYPE_RX_TIMEOUT = CSP_INT_RX_TIMEOUT,                 /*!< CSP接收超时中断 */
    HAL_CSP_INTTYPE_RX_FIFO_THD = CSP_INT_RXFIFO_THD_REACH,          /*!< CSP接收阈值中断，当RXFIFO现存量大于阈值时触发 */
    HAL_CSP_INTTYPE_TX_FIFO_THD = CSP_INT_TXFIFO_THD_REACH,          /*!< CSP发送阈值中断，当TXFIFO现存量小于阈值时触发 */
    HAL_CSP_INTTYPE_TX_FIFO_EMPTY = CSP_INT_TXFIFO_EMPTY,            /*!< CSP TXFIFO空中断 */
} HAL_CSP_IntTypeDef;

/**
 * @brief  CSP模拟外设类型枚举
 *
 */
typedef enum
{
    HAL_CSP_PERIPH_NONE = 0x00U,  /*!< CSP无外设模拟 */
    HAL_CSP_PERIPH_UART = 0x01U,  /*!< CSP模拟UART，UART为异步协议 */
    HAL_CSP_PERIPH_SPI  = 0x02U   /*!< CSP模拟SPI，SPI为同步协议 */
} HAL_CSP_PeriphModeTypeDef;

#define CSP_NUM		3 //指示外设个数

HAL_CSP_HandleTypeDef *g_csp_handle[CSP_NUM] = {NULL}; //指向初始化句柄

static void HAL_CSP_IRQHandler(void);

/**
 * @brief  CSP私有保护宏
 */
#define IS_CSP_INSTANCE(__INSTANCE__)       		(((__INSTANCE__) == HAL_CSP1) || \
												 	 ((__INSTANCE__) == HAL_CSP2) || \
												 	 ((__INSTANCE__) == HAL_CSP3))

#define IS_CSP_SPI_MASTERSLAVE(__MASTERSLAVE__)		(((__MASTERSLAVE__) == CSP_MODE1_CLOCK_MODE_Master))

#define IS_CSP_SPI_WORKMODE(__WORKMODE__)        	(((__WORKMODE__) == HAL_CSP_SPI_MODE0) || \
												 	 ((__WORKMODE__) == HAL_CSP_SPI_MODE1) || \
                                            	     ((__WORKMODE__) == HAL_CSP_SPI_MODE2) || \
												     ((__WORKMODE__) == HAL_CSP_SPI_MODE3))

/**
 * @brief CSP_UART收发中断过程中，单次中断传输数据个数，该值必须小于128
 *
 */
#define HAL_CSP_UART_MAX_XFERSIZE    (32)

/**
 * @brief  获取CSP的外设模式.
 *
 * @param  hcsp 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @return 返回CSP的外设模式，详情参考 @ref HAL_CSP_PeriphModeTypeDef
 */
static HAL_CSP_PeriphModeTypeDef HAL_CSP_Get_PeriphMode(HAL_CSP_HandleTypeDef *hcsp)
{
    if((hcsp->Instance->MODE1 & 0x01) == CSP_MODE1_ASYN)
    {
        return HAL_CSP_PERIPH_UART;
    }
    if((hcsp->Instance->MODE1 & 0x01) == CSP_MODE1_SYNC)
    {
        return HAL_CSP_PERIPH_SPI;
    }
    return HAL_CSP_PERIPH_NONE;
}

/**
 * @brief  设置CSP_UART硬件Timeout中断的超时时间，Timeout倒计时触发条件是RXD处于IDLE且RXFIFO非空.
 *
 * @param  hcsp 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @param  timeout(ms) 超时时间值,XY21100的CSP提供硬件超时机制，用户可根据实际接收需求设置超时时间
 */
static void HAL_CSP_UART_SetTimeout_RxdIdle_RxFIFONE(HAL_CSP_HandleTypeDef *hcsp, uint16_t timeout)
{
    uint32_t reg_val = 0;
    reg_val = timeout * hcsp->CSP_UART_Init.BaudRate / 1000;
    if ((reg_val <= 0x00) || (reg_val > 0xFFFF))
    {
        reg_val = 0xFFFF;
    }
    CSP_UARTRxTimeoutConfig(hcsp->Instance, 1, reg_val);
}

/**
 * @brief 使能Timeout超时，Timeout倒计时触发条件是只要RXD处于IDLE就触发.
 *
 * @param  hcsp 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
static void HAL_CSP_UART_EnableTimeout_OnlyRxdIdle(HAL_CSP_HandleTypeDef *hcsp)
{
    hcsp->Instance->AYSNC_PARAM_REG = ((hcsp->Instance->AYSNC_PARAM_REG) & ~CSP_AYSNC_PARAM_RX_TIMEOUT_EN_Msk) | (0 << CSP_AYSNC_PARAM_RX_TIMEOUT_EN_Pos);
}

/**
 * @brief CSP_SPI主机片选引脚拉高
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
inline void HAL_CSP_SPI_SetCS(HAL_CSP_HandleTypeDef *hcsp)
{
    CSP_TFS_PIN_VALUE(hcsp->Instance, 1);
}

/**
 * @brief CSP_SPI主机片选引脚拉低
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
inline void HAL_CSP_SPI_ResetCS(HAL_CSP_HandleTypeDef *hcsp)
{
    CSP_TFS_PIN_VALUE(hcsp->Instance, 0);
}

/**
 * @brief  获取 CSP 工作状态.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @return 返回工作状态，详情参考 @ref HAL_CSP_StateTypeDef
 */
inline HAL_CSP_StateTypeDef HAL_CSP_GetState(HAL_CSP_HandleTypeDef *hcsp)
{
	uint32_t temp1 = 0x00U, temp2 = 0x00U;
	temp1 = hcsp->gState;
	temp2 = hcsp->RxState;
	return (HAL_CSP_StateTypeDef)(temp1 | temp2);
}

/**
 * @brief  获取 CSP 错误状态.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @return 返回CSP错误码 详情参考 @ref HAL_CSP_ErrorCodeTypeDef.
 */
inline HAL_CSP_ErrorCodeTypeDef HAL_CSP_GetError(HAL_CSP_HandleTypeDef *hcsp)
{
	return hcsp->ErrorCode;
}

/**
 * @brief 注册CSP中断向量
 *
 * @param hcsp 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
static void HAL_CSP_NVIC_IntRegister(HAL_CSP_HandleTypeDef *hcsp)
{
    if (hcsp->Instance == HAL_CSP1)
    {
		NVIC_IntRegister(CSP1_IRQn, HAL_CSP_IRQHandler, 1);
    }
    else if(hcsp->Instance == HAL_CSP2)
    {
        NVIC_IntRegister(CSP2_IRQn, HAL_CSP_IRQHandler, 1);
    }
    else if(hcsp->Instance == HAL_CSP3)
    {
        NVIC_IntRegister(CSP3_IRQn, HAL_CSP_IRQHandler, 1);
    }
}

/**
 * @brief  读取CSP的NVIC中断使能状态
 * @param  hcsp 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @retval HAL_ENABLE：使能，HAL_DISABLE：禁能
 */
static HAL_FunctionalState HAL_CSP_Get_EnableIRQ(HAL_CSP_HandleTypeDef *hcsp)
{
    IRQn_Type IRQn = 0;
    switch ((uint32_t)hcsp->Instance)
    {
        case (uint32_t)HAL_CSP1: { IRQn = CSP1_IRQn; break; }
        case (uint32_t)HAL_CSP2: { IRQn = CSP2_IRQn; break; }
        case (uint32_t)HAL_CSP3: { IRQn = CSP3_IRQn; break; }
        default:;
    }
    return NVIC_GetEnableIRQ(IRQn);
}

/**
 * @brief 设置CSP时钟开关状态
 *
 * @param hcsp 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
static void HAL_CSP_ClockSet(HAL_CSP_HandleTypeDef *hcsp, HAL_FunctionalState state)
{
    //使能CSP时钟
    if(state == HAL_ENABLE)
    {
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
    //失能CSP时钟
    else if(state == HAL_DISABLE)
    {
        if (hcsp->Instance == HAL_CSP1)
        {
            PRCM_ClockDisable(CORE_CKG_CTL_CSP1_EN);
        }
        else if (hcsp->Instance == HAL_CSP2)
        {
            PRCM_ClockDisable(CORE_CKG_CTL_CSP2_EN);
        }
        else if (hcsp->Instance == HAL_CSP3)
        {
            PRCM_ClockDisable(CORE_CKG_CTL_CSP3_EN);
        }
    }
}

/**
 * @brief  获取CSP中断使能状态.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @return 返回CSP中断类型.详见  @ref HAL_CSP_IntTypeDef.
 */
static HAL_CSP_IntTypeDef HAL_CSP_GetITEnable(HAL_CSP_HandleTypeDef *hcsp)
{
	return hcsp->Instance->INT_ENABLE;
}

/**
 * @brief  获取CSP中断状态.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @return 返回CSP中断类型.详见  @ref HAL_CSP_IntTypeDef.
 */
static HAL_CSP_IntTypeDef HAL_CSP_GetITState(HAL_CSP_HandleTypeDef *hcsp)
{
	return CSP_IntStatus(hcsp->Instance);
}

/**
 * @brief  清除CSP中断标志.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @param  IntFlag 详情参考 @ref HAL_CSP_IntTypeDef.
 */
static void HAL_CSP_Clear_IT(HAL_CSP_HandleTypeDef *hcsp, HAL_CSP_IntTypeDef IntFlag)
{
	CSP_IntClear(hcsp->Instance, IntFlag);
}

/**
 * @brief  使能CSP中断源.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @param  IntFlag 详情参考 @ref HAL_CSP_IntTypeDef.
 */
static void HAL_CSP_Enable_IT(HAL_CSP_HandleTypeDef *hcsp, HAL_CSP_IntTypeDef IntFlag)
{
	CSP_IntEnable(hcsp->Instance, IntFlag);
}

/**
 * @brief  关闭CSP中断源.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @param  IntFlag 详情参考 @ref HAL_CSP_IntTypeDef.
 */
static void HAL_CSP_Disable_IT(HAL_CSP_HandleTypeDef *hcsp, HAL_CSP_IntTypeDef IntFlag)
{
	CSP_IntDisable(hcsp->Instance, IntFlag);
}

/**
 * @brief  初始化CSP为UART.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 */
HAL_StatusTypeDef HAL_CSP_UART_Init(HAL_CSP_HandleTypeDef *hcsp)
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
		hcsp->RxLock = HAL_UNLOCKED;

        //使能CSP时钟
        HAL_CSP_ClockSet(hcsp, HAL_ENABLE);
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
	if(hcsp->CSP_UART_Init.BaudRate > pclk/16)
	{
		return HAL_ERROR;
	}
	hcsp->gState = HAL_CSP_STATE_BUSY;

    //关闭所有中断源
    HAL_CSP_Disable_IT(hcsp, CSP_INT_ALL);

	//配置CSP_UART
	CSP_Disable(hcsp->Instance);
	CSP_UARTModeSet(hcsp->Instance, pclk, hcsp->CSP_UART_Init.BaudRate, hcsp->CSP_UART_Init.WordLength, hcsp->CSP_UART_Init.Parity, hcsp->CSP_UART_Init.StopBits);

    //记录句柄
	switch((uint32_t)hcsp->Instance)
	{
		case (uint32_t)HAL_CSP1	:{ g_csp_handle[0] = hcsp; break;}
		case (uint32_t)HAL_CSP2	:{ g_csp_handle[1] = hcsp; break;}
		case (uint32_t)HAL_CSP3	:{ g_csp_handle[2] = hcsp; break;}
		default :return HAL_ERROR;
	}

    hcsp->ErrorCode = HAL_CSP_ERROR_NONE;
	hcsp->gState = HAL_CSP_STATE_READY;
	hcsp->RxState = HAL_CSP_STATE_READY;

	return HAL_OK;
}

/**
 * @brief  初始化CSP为SPI.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 */
HAL_StatusTypeDef HAL_CSP_SPI_Init(HAL_CSP_HandleTypeDef *hcsp)
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
        HAL_CSP_ClockSet(hcsp, HAL_ENABLE);
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
    HAL_CSP_Disable_IT(hcsp, CSP_INT_ALL);

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

    if(hcsp->CSP_SPI_Init.MasterSlave == HAL_CSP_SPI_MODE_MASTER)
    {
        CSP_TFS_PIN_MODE(hcsp->Instance, 1); //TFS引脚为IO模式
        CSP_TFS_ACT_LEVEL(hcsp->Instance, 0);//TFS引脚有效电平为低电平
        CSP_TFS_IO_MODE(hcsp->Instance, 0);  //TFS引脚为输出
        HAL_CSP_SPI_SetCS(hcsp);             //TFS引脚设置为高电平，即处于无效状态
    }
	
	CSP_RxEnable(hcsp->Instance);
	CSP_TxEnable(hcsp->Instance);
	CSP_FifoReset(hcsp->Instance);
	CSP_FifoStart(hcsp->Instance);
	CSP_Enable(hcsp->Instance);

    //记录句柄
	switch((uint32_t)hcsp->Instance)
	{
		case (uint32_t)HAL_CSP1	:{ g_csp_handle[0] = hcsp; break;}
		case (uint32_t)HAL_CSP2	:{ g_csp_handle[1] = hcsp; break;}
		case (uint32_t)HAL_CSP3	:{ g_csp_handle[2] = hcsp; break;}
		default :return HAL_ERROR;
	}

	hcsp->ErrorCode = HAL_CSP_ERROR_NONE;
	hcsp->gState = HAL_CSP_STATE_READY;
	hcsp->RxState = HAL_CSP_STATE_READY;

	return HAL_OK;
}

/**
 * @brief  去初始化CSP.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 */
HAL_StatusTypeDef HAL_CSP_DeInit(HAL_CSP_HandleTypeDef *hcsp)
{
    assert_param(IS_CSP_INSTANCE(hcsp->Instance));

    if (hcsp == NULL)
	{
		return HAL_ERROR;
	}

	if (hcsp->gState == HAL_CSP_STATE_RESET)
	{
		return HAL_ERROR;
	}

	hcsp->gState = HAL_CSP_STATE_BUSY;

	//使能CSP时钟
	HAL_CSP_ClockSet(hcsp, HAL_ENABLE);

    //关闭CSP
    CSP_Disable(hcsp->Instance);

    //失能外设时钟
    HAL_CSP_ClockSet(hcsp, HAL_DISABLE);

    //释放句柄
	switch((uint32_t)hcsp->Instance)
	{
		case (uint32_t)HAL_CSP1	:{ g_csp_handle[0] = NULL; break;}
		case (uint32_t)HAL_CSP2	:{ g_csp_handle[1] = NULL; break;}
		case (uint32_t)HAL_CSP3	:{ g_csp_handle[2] = NULL; break;}
		default :return HAL_ERROR;
	}

	hcsp->ErrorCode = HAL_CSP_ERROR_NONE;
	hcsp->gState = HAL_CSP_STATE_RESET;
	hcsp->RxState = HAL_CSP_STATE_RESET;

	__HAL_UNLOCK(hcsp);
	__HAL_UNLOCK_RX(hcsp);

	return HAL_OK;
}


/**
 * @brief  CSP_UART或者CSP_SPI的阻塞发送API.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @param  pData 发送数据缓冲区指针.
 * @param  Size  发送数据字节长度.
 * @param  Timeout 超时时间（单位ms）;
 *         如果Timeout取值为HAL_MAX_DELAY，则是一直阻塞，等待发送完指定数量的字节才退出;
 * 		   如果Timeout设置为非0非HAL_MAX_DELAY时，表示函数等待数据发送完成的最长时间，超时则退出函数.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示发送数据成功，表示指定时间内成功发送指定数量的数据
 *         @retval  HAL_ERROR    ：表示入参错误
 *         @retval  HAL_BUSY     ：表示外设正在使用中
 *         @retval  HAL_TIMEOUT  ：表示指定时间内未能成功发送指定数量的数据
 * @note   hcsp结构体中的TxXferCount表示实际发送的字节数.
 */
HAL_StatusTypeDef HAL_CSP_Transmit(HAL_CSP_HandleTypeDef *hcsp, uint8_t *pData, uint16_t Size, uint32_t Timeout)
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

	if ((pData == NULL) || (Size == 0U) || (Timeout == 0))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK(hcsp);

	tickstart = Get_Tick();

	hcsp->gState = HAL_CSP_STATE_BUSY;
	hcsp->ErrorCode = HAL_CSP_ERROR_NONE;
    hcsp->pTxBuffPtr = xy_malloc(sizeof(uint8_t) * Size);
    if(hcsp->pTxBuffPtr == NULL)
    {
		return HAL_ERROR;
    }
    memcpy(hcsp->pTxBuffPtr, pData, Size);
    hcsp->TxXferSize = Size;
	hcsp->TxXferCount = 0;

	__HAL_UNLOCK(hcsp);

	//清除TXFIFO
	CSP_TXFifoClear(hcsp->Instance);

	//发送数据
	while (hcsp->TxXferSize > 0U)
	{
        if(!(hcsp->Instance->TX_FIFO_STATUS & CSP_TX_FIFO_STATUS_FULL_Msk))
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
	}

	//如果没有发送完成则等待
    while(!(hcsp->Instance->INT_STATUS & CSP_INT_TX_ALLOUT))
	{
		if ((Timeout != HAL_MAX_DELAY) && Check_Ms_Timeout(tickstart,Timeout))
		{
			errorcode = HAL_TIMEOUT;
			goto error;
		}
	}
	HAL_CSP_Clear_IT(hcsp, CSP_INT_TX_ALLOUT);

error:
    if(HAL_CSP_Get_PeriphMode(hcsp) == HAL_CSP_PERIPH_SPI)
    {
        CSP_RXFifoClear(hcsp->Instance);
    }

	hcsp->gState = HAL_CSP_STATE_READY;

    xy_free(hcsp->pTxBuffPtr);

	return errorcode;
}

/**
 * @brief  CSP_UART或者CSP_SPI阻塞接收API.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @param  pData 接收数据缓冲区指针.
 * @param  Size  接收数据字节长度.
 * @param  Timeout 超时时间（单位ms）;
 *         如果Timeout为0，则是不阻塞，检测到接收FIFO空后，则立刻退出;
 *         如果Timeout为HAL_MAX_DELAY，则是一直阻塞，直到接收到指定数量的字节后才退出;
 * 		   如果Timeout为非0非HAL_MAX_DELAY时，表示函数等待数据接收完成的最长时间，超时则退出函数.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：超时时间不为0时，表示指定时间内成功接收指定数量的数据；超时时间为0时，也会返回，但实际接收数据的数量通过hcsp结构体中的RxXferCount来确定
 *         @retval  HAL_ERROR    ：入参错误
 *         @retval  HAL_BUSY     ：外设正在使用中
 *         @retval  HAL_TIMEOUT  ：超时时间不为0时会返回，表示指定时间内未能成功接收指定数量的数据
 * @note   hcsp结构体中的TxXferCount表示实际发送的字节数.
 */
HAL_StatusTypeDef HAL_CSP_Receive(HAL_CSP_HandleTypeDef *hcsp, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0;
	HAL_StatusTypeDef errorcode = HAL_OK;

    //CSP_SPI为主机模式时使用收发接口,发送dummy以接收数据
    if((HAL_CSP_Get_PeriphMode(hcsp) == HAL_CSP_PERIPH_SPI) && (hcsp->CSP_SPI_Init.MasterSlave == HAL_CSP_SPI_MODE_MASTER))
    {
        return HAL_CSP_SPI_TransmitReceive(hcsp, pData, pData, Size, Timeout);
    }

	if (hcsp->RxState == HAL_CSP_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hcsp->RxState != HAL_CSP_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK_RX(hcsp);

	tickstart = Get_Tick();

	hcsp->RxState = HAL_CSP_STATE_BUSY;
	hcsp->ErrorCode = HAL_CSP_ERROR_NONE;
	hcsp->pRxBuffPtr = (uint8_t *)pData;
	hcsp->RxXferSize = Size;
	hcsp->RxXferCount = 0;

	__HAL_UNLOCK_RX(hcsp);

	//清除RXFIFO
	CSP_RXFifoClear(hcsp->Instance);

	//接收数据
	while (hcsp->RxXferSize > 0U)
	{
        if(!(hcsp->Instance->RX_FIFO_STATUS & CSP_RX_FIFO_STATUS_EMPTY_Msk))
        {
			*(hcsp->pRxBuffPtr) = (uint8_t)(hcsp->Instance->RX_FIFO_DATA);
			hcsp->pRxBuffPtr++;
			hcsp->RxXferSize--;
			hcsp->RxXferCount++;
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
    if(HAL_CSP_Get_PeriphMode(hcsp) == HAL_CSP_PERIPH_UART)
    {
        //对于实际接收长度小于用户指定长度，而接收超时退出时，需要在尾部加'\0'
        if (hcsp->RxXferCount != 0 && hcsp->RxXferSize != 0)
        {
            *(hcsp->pRxBuffPtr) = '\0';
        }
    }

    if(HAL_CSP_Get_PeriphMode(hcsp) == HAL_CSP_PERIPH_SPI)
    {
        CSP_TXFifoClear(hcsp->Instance);
    }

	hcsp->RxState = HAL_CSP_STATE_READY;

	return errorcode;
}

/**
 * @brief  CSP_SPI主机阻塞发送接收API. 注意！！！该接口只能用于CSP_SPI主机.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @param  pTxData 发送缓冲区指针
 * @param  pRxData 接收缓冲区指针
 * @param  Size 发送和接收数据字节长度
 * @param  Timeout：超时时间（单位ms）;
 *         如果Timeout为HAL_MAX_DELAY 是一直阻塞，直到发送和接收到指定数量的字节后才退出;
 * 		   如果Timeout为非0非HAL_MAX_DELAY时，表示函数等待数据发送接收完成的最长时间，超时则退出函数.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：发送和接收数据成功，表示指定时间内成功发送和接收指定数量的数据
 *         @retval  HAL_ERROR    ：入参错误
 *         @retval  HAL_BUSY     ：外设正在使用中
 *         @retval  HAL_TIMEOUT  ：表示指定时间内未能成功发送和接收指定数量的数据
 * @note   hcsp结构体中的TxXferCount表示实际发送的字节数，RxXferCount表示实际接收的字节数.
 */
HAL_StatusTypeDef HAL_CSP_SPI_TransmitReceive(HAL_CSP_HandleTypeDef *hcsp, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0;
	HAL_StatusTypeDef errorcode = HAL_OK;

    if(HAL_CSP_Get_PeriphMode(hcsp) != HAL_CSP_PERIPH_SPI)
    {
        return HAL_ERROR;
    }

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
	uint32_t tx_allow = 1; //主机需要使用该标志控制读写顺序，否则容易丢接收数据
	while ((hcsp->TxXferSize > 0) || (hcsp->RxXferSize > 0))
	{
		//发送数据
		if((!(hcsp->Instance->TX_FIFO_STATUS & CSP_TX_FIFO_STATUS_FULL_Msk)) && (hcsp->TxXferSize > 0) && (tx_allow == 1))
		{
			hcsp->Instance->TX_FIFO_DATA = *(hcsp->pTxBuffPtr + hcsp->TxXferCount);
			hcsp->TxXferSize--;
			hcsp->TxXferCount++;
			tx_allow = 0;
		}
		else
		{
			if ((Timeout != HAL_MAX_DELAY) && Check_Ms_Timeout(tickstart,Timeout))
			{
				errorcode = HAL_TIMEOUT;
				goto error;
			}
		}

		//在低频时保证不进入else分支，else分支的xy_get_ms比较耗时，6.5M主频下约65us
		while(!(HAL_CSP_GetITState(hcsp) & CSP_INT_RX_DONE));
		HAL_CSP_Clear_IT(hcsp, CSP_INT_RX_DONE);

        //接收数据
		if((!(hcsp->Instance->RX_FIFO_STATUS & CSP_RX_FIFO_STATUS_EMPTY_Msk)) && (hcsp->RxXferSize > 0) && (tx_allow == 0))
		{
			*(hcsp->pRxBuffPtr) = (uint8_t)(hcsp->Instance->RX_FIFO_DATA);
			hcsp->pRxBuffPtr++;
			hcsp->RxXferSize--;
			hcsp->RxXferCount++;
			tx_allow = 1;
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

/**
 * @brief  CSP_UART非阻塞发送API.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @param  pData 发送数据缓冲区指针.
 * @param  Size  发送数据字节长度.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示中断发送需要的参数配置、外设配置成功，等待进入中断发送数据
 *         @retval  HAL_ERROR    ：表示入参错误
 *         @retval  HAL_BUSY     ：表示外设正在使用中
 */
HAL_StatusTypeDef HAL_CSP_Transmit_IT(HAL_CSP_HandleTypeDef *hcsp, uint8_t *pData, uint16_t Size)
{
    if(HAL_CSP_Get_PeriphMode(hcsp) != HAL_CSP_PERIPH_UART)
    {
        return HAL_ERROR;
    }

	if (hcsp->gState == HAL_CSP_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hcsp->gState != HAL_CSP_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK(hcsp);

	hcsp->gState = HAL_CSP_STATE_BUSY;
	hcsp->ErrorCode = HAL_CSP_ERROR_NONE;
    hcsp->pTxBuffPtr = xy_malloc(sizeof(uint8_t) * Size);
    if(hcsp->pTxBuffPtr == NULL)
    {
		return HAL_ERROR;
    }
    memcpy(hcsp->pTxBuffPtr, pData, Size);
    hcsp->TxXferSize = Size;
	hcsp->TxXferCount = 0;

    __HAL_UNLOCK(hcsp);

	//清除TXFIFO
	CSP_TXFifoClear(hcsp->Instance);

    //清除所有中断标志位
    HAL_CSP_Clear_IT(hcsp, CSP_INT_ALL);

    //注册中断
    HAL_CSP_NVIC_IntRegister(hcsp);

    //确保发送该字节期间不要处理中断，若期间产生接收中断，则可能将发送中断标志位清除，导致后续无法继续中断发送
    //注意中断里会一起判断中断使能状态，所以需要一起锁。
    DisablePrimask();

    //发送数据1个字符
	do {
		if(!(hcsp->Instance->TX_FIFO_STATUS & CSP_TX_FIFO_STATUS_FULL_Msk))
		{
			hcsp->Instance->TX_FIFO_DATA = *(hcsp->pTxBuffPtr + hcsp->TxXferCount);
			hcsp->TxXferSize--;
			hcsp->TxXferCount++;
		}
	} while ((hcsp->Instance->TX_FIFO_STATUS & CSP_TX_FIFO_STATUS_FULL_Msk));
	
	//使能中断源，保证中断触发时，全局变量更新完成
	HAL_CSP_Enable_IT(hcsp, HAL_CSP_INTTYPE_TX_FIFO_EMPTY);

    EnablePrimask();

	return HAL_OK;
}

/**
 * @brief  CSP_UART非阻塞接收API.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @param  pData 接收数据缓冲区指针.
 * @param  Size  接收数据字节长度.
 * @param  HardTimeout 超时时间（单位ms），XY1200 CSP提供硬件超时功能，在CSP接收过程中，如果超过HardTimeout时间后没有新的数据到来，则触发超时中断.
 *         如果HardTimeout为HAL_MAX_DELAY，则表示不使用CSP模块自身的硬件超时功能，只有在用户指定长度的数据全部收完时，才会触发接收完成回调函数的调用;
 *         如果Timeout为非0非HAL_MAX_DELAY，则表示设定并使用CSP模块自身的硬件超时功能，当用户指定长度的数据没有全部收完时，若出现一段数据接收间隔大于设定的超时时间后就会触发接收完成回调函数的调用。
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：中断接收函数执行成功，等待在中断中接收指定数量的的数据
 *         @retval  HAL_ERROR    ：入参错误
 *         @retval  HAL_BUSY     ：外设正在使用中
 */
HAL_StatusTypeDef HAL_CSP_Receive_IT(HAL_CSP_HandleTypeDef *hcsp, uint8_t *pData, uint16_t Size, uint16_t HardTimeout)
{
    if(HAL_CSP_Get_PeriphMode(hcsp) != HAL_CSP_PERIPH_UART)
    {
        return HAL_ERROR;
    }

	if (hcsp->RxState == HAL_CSP_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hcsp->RxState != HAL_CSP_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK_RX(hcsp);

	hcsp->RxState = HAL_CSP_STATE_BUSY;
	hcsp->ErrorCode = HAL_CSP_ERROR_NONE;
	hcsp->pRxBuffPtr = (uint8_t *)pData;
	hcsp->RxXferSize = Size;
	hcsp->RxXferCount = 0;

	__HAL_UNLOCK_RX(hcsp);

	//清除RXFIFO
	CSP_RXFifoClear(hcsp->Instance);

    //清除所有中断标志位
    HAL_CSP_Clear_IT(hcsp, CSP_INT_ALL);

    //根据HardTimeout设置RXFIFO阈值中断
    if(HardTimeout != (uint16_t)HAL_MAX_DELAY)
    {
        //RXFIFO数据存量大于31字节时触发RXFIFO阈值中断
        CSP_RXFIFO_LevelSet(hcsp->Instance, 31);
    }
    else
    {
        //根据用户指定接收数据长度设置RXFIFO阈值中断，RXFIFO数据存量大于阈值时触发RXFIFO阈值中断
        if(hcsp->RxXferSize > HAL_CSP_UART_MAX_XFERSIZE)
        {
            CSP_RXFIFO_LevelSet(hcsp->Instance, HAL_CSP_UART_MAX_XFERSIZE);
        }
        else
        {
            CSP_RXFIFO_LevelSet(hcsp->Instance, hcsp->RxXferSize-1);
        }
    }

    //注册中断
    HAL_CSP_NVIC_IntRegister(hcsp);

    //根据HardTimeout使能超时时间和超时中断
    if(HardTimeout != (uint16_t)HAL_MAX_DELAY)
    {
        HAL_CSP_UART_SetTimeout_RxdIdle_RxFIFONE(hcsp, HardTimeout);
        HAL_CSP_Enable_IT(hcsp, HAL_CSP_INTTYPE_RX_TIMEOUT);
    }
    else
    {
        HAL_CSP_Disable_IT(hcsp, HAL_CSP_INTTYPE_RX_TIMEOUT);
    }

	//使能中断源
    HAL_CSP_Enable_IT(hcsp, HAL_CSP_INTTYPE_RX_FIFO_THD);
	HAL_CSP_Enable_IT(hcsp, HAL_CSP_INTTYPE_UART_FRAME_ERR);
	HAL_CSP_Enable_IT(hcsp, HAL_CSP_INTTYPE_RX_FIFO_OVF);

	return HAL_OK;
}



/**
 * @brief  本地CSP错误回调函数
 *
 * @param  hcsp. 详见结构体定义 HAL_CSP_HandleTypeDef.
 */
static void HAL_CSP_ErrorCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	if (hcsp->Instance == HAL_CSP1)
	{
		HAL_CSP1_ErrorCallback(hcsp);
	}
	else if (hcsp->Instance == HAL_CSP2)
	{
		HAL_CSP2_ErrorCallback(hcsp);
	}
	else if (hcsp->Instance == HAL_CSP3)
	{
		HAL_CSP3_ErrorCallback(hcsp);
	}
}

/**
 * @brief  本地CSP接收完成回调函数
 *
 * @param  hcsp. 详见结构体定义 HAL_CSP_HandleTypeDef.
 */
static void HAL_CSP_RxCpltCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	if (hcsp->Instance == HAL_CSP1)
	{
		HAL_CSP1_RxCpltCallback(hcsp);
	}
	else if (hcsp->Instance == HAL_CSP2)
	{
		HAL_CSP2_RxCpltCallback(hcsp);
	}
	else if (hcsp->Instance == HAL_CSP3)
	{
		HAL_CSP3_RxCpltCallback(hcsp);
	}
}

/**
 * @brief  本地CSP发送完成回调函数
 *
 * @param  hcsp. 详见结构体定义 HAL_CSP_HandleTypeDef.
 */
static void HAL_CSP_TxCpltCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	if (hcsp->Instance == HAL_CSP1)
	{
		HAL_CSP1_TxCpltCallback(hcsp);
	}
	else if (hcsp->Instance == HAL_CSP2)
	{
		HAL_CSP2_TxCpltCallback(hcsp);
	}
	else if (hcsp->Instance == HAL_CSP3)
	{
		HAL_CSP3_TxCpltCallback(hcsp);
	}
}

/**
 * @brief  本地CSP中断服务函数
 *
 */
static void HAL_CSP_IRQHandler(void)
{
	//轮询句柄指针
	for (uint8_t i = 0; i < CSP_NUM; i++)
	{
		if((g_csp_handle[i] == NULL) || (HAL_CSP_Get_EnableIRQ(g_csp_handle[i]) == HAL_DISABLE))
		{
			continue;
		}

		HAL_CSP_IntTypeDef ITState = HAL_CSP_GetITState(g_csp_handle[i]);
		HAL_CSP_IntTypeDef ITEnable = HAL_CSP_GetITEnable(g_csp_handle[i]);
		
		//发送流程：txfifo阈值中断
		if ((HAL_RESET != (ITState & HAL_CSP_INTTYPE_TX_FIFO_EMPTY)) && (HAL_RESET != (ITEnable & HAL_CSP_INTTYPE_TX_FIFO_EMPTY)))
		{
			static uint8_t tx_write_done = 0;
			HAL_CSP_Clear_IT(g_csp_handle[i], HAL_CSP_INTTYPE_TX_FIFO_EMPTY);//速率较高时中断标志清除应放置最前

			//发送数据还没全部写入tx_fifo，注意全部写入tx_fifo不代表数据都发出去了
			if ((tx_write_done == 0) && (g_csp_handle[i]->TxXferSize !=0))
			{
				//发送数据，一次只写HAL_CSP_UART_MAX_XFERSIZE字节，减少中断执行时间
				uint8_t real_write_size = (g_csp_handle[i]->TxXferSize > HAL_CSP_UART_MAX_XFERSIZE) ? HAL_CSP_UART_MAX_XFERSIZE : g_csp_handle[i]->TxXferSize;
				while(real_write_size > 0)
				{
					if(!(g_csp_handle[i]->Instance->TX_FIFO_STATUS & CSP_TX_FIFO_STATUS_FULL_Msk))
					{
						g_csp_handle[i]->Instance->TX_FIFO_DATA = *(g_csp_handle[i]->pTxBuffPtr + g_csp_handle[i]->TxXferCount);
						real_write_size--;
						g_csp_handle[i]->TxXferSize--;
						g_csp_handle[i]->TxXferCount++;
						if(g_csp_handle[i]->TxXferSize == 0)
						{
							tx_write_done = 1;
							break;
						}
					}
				}
			}
			//数据全部写入tx_fifo且tx_fifo空，此时调用TxCpltCallback才是合理的
			else
			{
				//清零标志，为下次发送中断做准备
				tx_write_done = 0;

				HAL_CSP_Disable_IT(g_csp_handle[i], HAL_CSP_INTTYPE_TX_FIFO_EMPTY);
				xy_free(g_csp_handle[i]->pTxBuffPtr);

				//针对不同速率，增加2个字符的延时，以保证退出该接口时数据发送完成
				uint32_t onechar_timeout = (uint32_t)((20.0 * 1000.0 * 1000.0 / g_csp_handle[i]->CSP_UART_Init.BaudRate) + 0.5); //按20bit计算，单位us
				if(onechar_timeout < 100)
				{
					onechar_timeout = 100; //最少延时100us
				}
				HAL_Delay_US(onechar_timeout);
				g_csp_handle[i]->gState = HAL_CSP_STATE_READY;
				HAL_CSP_TxCpltCallback(g_csp_handle[i]);
			}
		}

		//接收异常流程
		if ((HAL_RESET != (ITEnable & HAL_CSP_INTTYPE_RX_FIFO_OVF)) && (HAL_RESET != (ITState & HAL_CSP_INTTYPE_RX_FIFO_OVF)))
		{
			g_csp_handle[i]->ErrorCode |= HAL_CSP_ERROR_RX_FIFO_OVF;
			HAL_CSP_Clear_IT(g_csp_handle[i], HAL_CSP_INTTYPE_RX_FIFO_OVF);
			HAL_CSP_Disable_IT(g_csp_handle[i], HAL_CSP_INTTYPE_RX_FIFO_OVF);
		}
		if ((HAL_RESET != (ITEnable & HAL_CSP_INTTYPE_UART_FRAME_ERR)) && (HAL_RESET != (ITState & HAL_CSP_INTTYPE_UART_FRAME_ERR)))
		{
			g_csp_handle[i]->ErrorCode |= HAL_CSP_ERROR_UART_FRM_ERR;
			HAL_CSP_Clear_IT(g_csp_handle[i], HAL_CSP_INTTYPE_UART_FRAME_ERR);
			HAL_CSP_Disable_IT(g_csp_handle[i], HAL_CSP_INTTYPE_UART_FRAME_ERR);
		}
        //产生错误后不用再去执行正常的接收流程，因此需要退出中断服务函数
		if (g_csp_handle[i]->ErrorCode != HAL_CSP_ERROR_NONE)
		{
			HAL_CSP_ErrorCallback(g_csp_handle[i]);
            return;
        }
		
		//接收流程：只开rxfifo阈值中断，即HardTimeout为HAL_MAX_DELAY
		if(HAL_RESET == (ITEnable & HAL_CSP_INTTYPE_RX_TIMEOUT))
		{
			if ((HAL_RESET != (ITEnable & HAL_CSP_INTTYPE_RX_FIFO_THD)) && (HAL_RESET != (ITState & HAL_CSP_INTTYPE_RX_FIFO_THD)))
			{
				while(!(g_csp_handle[i]->Instance->RX_FIFO_STATUS & CSP_RX_FIFO_STATUS_EMPTY_Msk))
				{
					*(g_csp_handle[i]->pRxBuffPtr) = (uint8_t)(g_csp_handle[i]->Instance->RX_FIFO_DATA);
					g_csp_handle[i]->pRxBuffPtr++;
					g_csp_handle[i]->RxXferSize--;
					g_csp_handle[i]->RxXferCount++;
					if (g_csp_handle[i]->RxXferSize == 0U)
					{
						HAL_CSP_Disable_IT(g_csp_handle[i], HAL_CSP_INTTYPE_RX_FIFO_THD);
						HAL_CSP_Clear_IT(g_csp_handle[i], HAL_CSP_INTTYPE_RX_FIFO_THD);
						g_csp_handle[i]->RxState = HAL_CSP_STATE_READY;
						HAL_CSP_RxCpltCallback(g_csp_handle[i]);
					}
				}
				//根据用户指定接收数据长度设置RXFIFO阈值中断，RXFIFO数据存量大于阈值时触发RXFIFO阈值中断
				if(g_csp_handle[i]->RxXferSize > HAL_CSP_UART_MAX_XFERSIZE)
				{
					CSP_RXFIFO_LevelSet(g_csp_handle[i]->Instance, HAL_CSP_UART_MAX_XFERSIZE);
				}
				else
				{
					CSP_RXFIFO_LevelSet(g_csp_handle[i]->Instance, g_csp_handle[i]->RxXferSize-1);
				}
				HAL_CSP_Clear_IT(g_csp_handle[i], HAL_CSP_INTTYPE_RX_FIFO_THD);
			}
		}

		//接收流程：既开rxfifo阈值中断又开rxd timeout中断，即HardTimeout不为HAL_MAX_DELAY
		else
		{
			if (((HAL_RESET != (ITEnable & HAL_CSP_INTTYPE_RX_FIFO_THD)) && (HAL_RESET != (ITState & HAL_CSP_INTTYPE_RX_FIFO_THD))) || \
				((HAL_RESET != (ITEnable & HAL_CSP_INTTYPE_RX_TIMEOUT)) && (HAL_RESET != (ITState & HAL_CSP_INTTYPE_RX_TIMEOUT))))
			{
				while(!(g_csp_handle[i]->Instance->RX_FIFO_STATUS & CSP_RX_FIFO_STATUS_EMPTY_Msk))
				{
					//确保RXSTA是非空的，才EnableTimeout_OnlyRxdIdle
					if(((g_csp_handle[i]->Instance->AYSNC_PARAM_REG) & CSP_AYSNC_PARAM_RX_TIMEOUT_EN_Msk) != (0 << CSP_AYSNC_PARAM_RX_TIMEOUT_EN_Pos))
					{
						HAL_CSP_UART_EnableTimeout_OnlyRxdIdle(g_csp_handle[i]);
					}
					*(g_csp_handle[i]->pRxBuffPtr) = (uint8_t)(g_csp_handle[i]->Instance->RX_FIFO_DATA);
					g_csp_handle[i]->pRxBuffPtr++;
					g_csp_handle[i]->RxXferSize--;
					g_csp_handle[i]->RxXferCount++;
					if (g_csp_handle[i]->RxXferSize == 0U)
					{
						break;
					}
				}
				//接收超时或者用户指定数据长度接收完成时，则调用回调函数
				if (HAL_RESET != (ITState & HAL_CSP_INTTYPE_RX_TIMEOUT) || g_csp_handle[i]->RxXferSize == 0)
				{
					//对于实际接收长度小于用户指定长度，而接收超时退出时，需要在尾部加'\0'
					if (g_csp_handle[i]->RxXferCount != 0 && g_csp_handle[i]->RxXferSize != 0)
					{
						*(g_csp_handle[i]->pRxBuffPtr) = '\0';
					}
					HAL_CSP_Disable_IT(g_csp_handle[i], HAL_CSP_INTTYPE_RX_FIFO_THD);
					HAL_CSP_Disable_IT(g_csp_handle[i], HAL_CSP_INTTYPE_RX_TIMEOUT);
					HAL_CSP_Clear_IT(g_csp_handle[i], HAL_CSP_INTTYPE_RX_FIFO_THD);
					HAL_CSP_Clear_IT(g_csp_handle[i], HAL_CSP_INTTYPE_RX_TIMEOUT);
					g_csp_handle[i]->RxState = HAL_CSP_STATE_READY;
					HAL_CSP_RxCpltCallback(g_csp_handle[i]);
				}
				HAL_CSP_Clear_IT(g_csp_handle[i], HAL_CSP_INTTYPE_RX_FIFO_THD);
			}
		}
	}
}

