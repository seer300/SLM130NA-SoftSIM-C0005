/**
 ******************************************************************************
 * @file    hal_i2c.c
 * @brief   HAL库I2C
 ******************************************************************************
 */

#include "prcm.h"
#include "hal_i2c.h"
#include "hal_def.h"
#include "nvic.h"
#include "sys_clk.h"
#include <string.h>
#include "xy_gpio_rcr.h"

HAL_I2C_HandleTypeDef *g_i2c_handle[I2C_NUM] = {NULL}; //指向初始化句柄

void HAL_I2C_IRQHandler(void);

/**
 * @brief 设置I2C为从机时的地址，或者I2C为主机时的目标从机地址
 *
 * @param hi2c 详情参考 @ref HAL_I2C_HandleTypeDef.
 * @param addr 从机地址
 */
void HAL_I2C_SetAddr(HAL_I2C_HandleTypeDef *hi2c, uint16_t addr)
{
	if (hi2c->Init.AddressingMode == HAL_I2C_ADDRESS_7BITS)
	{
		hi2c->Instance->CTL &= ~I2C_CTL_MST_ADD_10B_Msk;
	}
	else if(hi2c->Init.AddressingMode == HAL_I2C_ADDRESS_10BITS)
	{
		hi2c->Instance->CTL |= I2C_CTL_MST_ADD_10B_Msk;
	}
	I2C_SetAddr(hi2c->Instance, addr);
}

/**
 * @brief 注册I2C中断向量
 *
 * @param hi2c 详情参考 @ref HAL_I2C_HandleTypeDef.
 */
static void HAL_I2C_NVIC_IntRegister(HAL_I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == HAL_I2C1)
    {
		NVIC_IntRegister(I2C1_IRQn, HAL_I2C_IRQHandler, 1);
    }
    else if(hi2c->Instance == HAL_I2C2)
    {
        NVIC_IntRegister(I2C2_IRQn, HAL_I2C_IRQHandler, 1);
    }
}

/**
 * @brief  获取I2C工作状态.
 *
 * @param  hi2c. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
 * @return 返回I2C工作状态. 详见  @ref HAL_I2C_StateTypeDef.
 */
inline HAL_I2C_StateTypeDef HAL_I2C_GetState(HAL_I2C_HandleTypeDef *hi2c)
{
	return hi2c->State;
}

/**
 * @brief  获取I2C错误状态.
 *
 * @param  hi2c. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
 * @return 返回I2C错误码.详见  @ref HAL_I2C_ErrorCodeTypeDef.
 */
inline HAL_I2C_ErrorCodeTypeDef HAL_I2C_GetError(HAL_I2C_HandleTypeDef *hi2c)
{
	return hi2c->ErrorCode;
}

/**
 * @brief  获取I2C中断源使能状态.
 *
 * @param  hi2c. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
 * @return 返回I2C中断类型.详见  @ref HAL_I2C_IntTypeDef.
 */
static HAL_I2C_IntTypeDef HAL_I2C_GetITEnable(HAL_I2C_HandleTypeDef *hi2c)
{
	return (hi2c->Instance->INT_MASK);
}

/**
 * @brief  获取I2C中断标志.
 *
 * @param  hi2c. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
 * @return 返回I2C中断类型.详见  @ref HAL_I2C_IntTypeDef.
 */
static HAL_I2C_IntTypeDef HAL_I2C_GetITState(HAL_I2C_HandleTypeDef *hi2c)
{
	return (hi2c->Instance->INT_STAT);
}

/**
 * @brief  清除I2C中断标志.
 *
 * @param  hi2c 详情参考 @ref HAL_I2C_HandleTypeDef.
 * @param  IntFlag 详情参考 @ref HAL_I2C_IntTypeDef.
 */
static void HAL_I2C_Clear_IT(HAL_I2C_HandleTypeDef *hi2c, HAL_I2C_IntTypeDef IntFlag)
{
	I2C_IntClear(hi2c->Instance, IntFlag);
}

/**
 * @brief  使能I2C中断源.
 *
 * @param  hi2c 详情参考 @ref HAL_I2C_HandleTypeDef.
 * @param  IntFlag 详情参考 @ref HAL_I2C_IntTypeDef.
 */
static void HAL_I2C_Enable_IT(HAL_I2C_HandleTypeDef *hi2c, HAL_I2C_IntTypeDef IntFlag)
{
	I2C_IntEnable(hi2c->Instance, IntFlag);
}

/**
 * @brief  关闭I2C中断源.
 *
 * @param  hi2c 详情参考 @ref HAL_I2C_HandleTypeDef.
 * @param  IntFlag 详情参考 @ref HAL_I2C_IntTypeDef.
 */
static void HAL_I2C_Disable_IT(HAL_I2C_HandleTypeDef *hi2c, HAL_I2C_IntTypeDef IntFlag)
{
	I2C_IntDisable(hi2c->Instance, IntFlag);
}

/**
 * @brief  软件复位I2C.
 *
 * @param  hi2c 详情参考 @ref HAL_I2C_HandleTypeDef.
 */
inline void HAL_I2C_SoftReset(HAL_I2C_HandleTypeDef *hi2c)
{
	I2C_Reset(hi2c->Instance);
}

/**
 * @brief  初始化I2C.
 *
 * @param  hi2c. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 */ 
HAL_StatusTypeDef HAL_I2C_Init(HAL_I2C_HandleTypeDef *hi2c)
{
    assert_param(IS_I2C_INSTANCE(hi2c->Instance));

    if (hi2c == NULL)
	{
		return HAL_ERROR;
	}

	if (hi2c->State == HAL_I2C_STATE_RESET)
	{
		hi2c->Lock = HAL_UNLOCKED;

        //配置时钟源，使能I2C时钟
		if (hi2c->Instance == HAL_I2C1)
		{
            PRCM_I2C1refclkSet(I2C1_REFCLK_SEL_LSioclk);
			PRCM_ClockEnable(CORE_CKG_CTL_I2C1_EN);
		}
		else if (hi2c->Instance == HAL_I2C2)
		{
            PRCM_I2C2refclkSet(I2C2_REFCLK_SEL_LSioclk);
			PRCM_ClockEnable(CORE_CKG_CTL_I2C2_EN);
		}
	}
	else
	{
		return HAL_ERROR;
	}

	hi2c->State = HAL_I2C_STATE_BUSY;

	//软复位I2C,清CTRL，清除所有中断标志位
	I2C_DeInit(hi2c->Instance);

	//关闭所有中断源
    HAL_I2C_Disable_IT(hi2c,I2C_INT_ALL);

	//配置工作模式，设置主机寻址宽度，设置速率，以及一些默认配置
	I2C_Init(hi2c->Instance, hi2c->Init.Mode, hi2c->Init.ClockSpeed, hi2c->Init.AddressingMode, hi2c->Instance == HAL_I2C1 ? GetI2c1ClockFreq() : GetI2c2ClockFreq());

	//默认关闭字节间延时
	I2C_SetDelayBetweenByte(hi2c->Instance, 0, 0);

	//失能pad为防倒灌模式
	//RCR_LCD_IO_All_Disable();

	//I2C为从机时，设置自身设备地址(不包含读写控制位)
	if (hi2c->Init.Mode == HAL_I2C_MODE_SLAVE)
	{
        HAL_I2C_SetAddr(hi2c,hi2c->Init.OwnAddress);
	}

    //记录句柄
	switch((uint32_t)hi2c->Instance)
	{
		case (uint32_t)HAL_I2C1	:{ g_i2c_handle[0] = hi2c; break;}
		case (uint32_t)HAL_I2C2	:{ g_i2c_handle[1] = hi2c; break;}
		default :return HAL_ERROR;
	}

    hi2c->XferDir = HAL_I2C_DIR_NONE;
	hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
	hi2c->State = HAL_I2C_STATE_READY;

	return HAL_OK;
}

/**
 * @brief  去初始化I2C.
 *
 * @param  hi2c. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 */
HAL_StatusTypeDef HAL_I2C_DeInit(HAL_I2C_HandleTypeDef *hi2c)
{
    assert_param(IS_I2C_INSTANCE(hi2c->Instance));

	if (hi2c == NULL)
	{
		return HAL_ERROR;
	}

	if (hi2c->State == HAL_I2C_STATE_RESET)
	{
		return HAL_ERROR;
	}

	hi2c->State = HAL_I2C_STATE_BUSY;

	//配置时钟源，使能I2C时钟
	if (hi2c->Instance == HAL_I2C1)
	{
		PRCM_I2C1refclkSet(I2C1_REFCLK_SEL_LSioclk);
		PRCM_ClockEnable(CORE_CKG_CTL_I2C1_EN);
	}
	else if (hi2c->Instance == HAL_I2C2)
	{
		PRCM_I2C2refclkSet(I2C2_REFCLK_SEL_LSioclk);
		PRCM_ClockEnable(CORE_CKG_CTL_I2C2_EN);
	}

	//软复位I2C
	I2C_DeInit(hi2c->Instance);

    //释放句柄
	switch((uint32_t)hi2c->Instance)
	{
		case (uint32_t)HAL_I2C1	:{ g_i2c_handle[0] = NULL; break;}
		case (uint32_t)HAL_I2C2	:{ g_i2c_handle[1] = NULL; break;}
		default :return HAL_ERROR;
	}

	hi2c->XferDir = HAL_I2C_DIR_NONE;
	hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
	hi2c->State = HAL_I2C_STATE_RESET;

	__HAL_UNLOCK(hi2c);

	return HAL_OK;
}

/**
 * @brief  I2C主机阻塞发送API.
 *
 * @param  hi2c. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
 * @param  DevAddress：目标从机地址，7位地址时:从机地址bit[8:1]+读写控制bit[0]，10位地址时bit[9:0]有效.
 * @param  pData：要发送的数据的存储地址
 * @param  Size：要发送的数据的长度
 * @param  Timeout：超时时间（单位ms）;
 *         如果Timeout为HAL_MAX_DELAY，则是一直阻塞，等待发送完指定数量的字节才退出;
 * 		   如果Timeout为非0非HAL_MAX_DELAY时，表示函数等待数据发送完成的最长时间，超时则退出函数.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示发送数据成功，表示指定时间内成功发送指定数量的数据
 *         @retval  HAL_ERROR    ：表示入参错误
 *         @retval  HAL_BUSY     ：表示外设正在使用中
 *         @retval  HAL_TIMEOUT  ：表示指定时间内未能成功发送指定数量的数据
 * @note   hi2c结构体中的XferCount表示实际发送的字节数.
 */
HAL_StatusTypeDef HAL_I2C_Master_Transmit(HAL_I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0;
	HAL_StatusTypeDef errorcode = HAL_OK;

	if (hi2c->State == HAL_I2C_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hi2c->State != HAL_I2C_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U) || (Timeout == 0))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK(hi2c);

	tickstart = Get_Tick();

    hi2c->State = HAL_I2C_STATE_BUSY;
	hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
	hi2c->pBuffPtr = xy_malloc(sizeof(uint8_t) * Size);
	if (hi2c->pBuffPtr  == NULL)
	{
		return HAL_ERROR;
	}
	memcpy(hi2c->pBuffPtr , pData, Size);
	hi2c->XferSize = Size;
	hi2c->XferCount = 0;

	//设置目标从机地址，设置完成后I2C开始寻址直到寻到从机
    if(hi2c->Init.AddressingMode == HAL_I2C_ADDRESS_7BITS)
    {
        HAL_I2C_SetAddr(hi2c,DevAddress>>1);
    }
    else if(hi2c->Init.AddressingMode == HAL_I2C_ADDRESS_10BITS)
    {
        HAL_I2C_SetAddr(hi2c,DevAddress);
    }

	//发送数据
	while (hi2c->XferSize > 0U)
	{
		if ((I2C_Status(hi2c->Instance) & I2C_STAT_TFNF_Msk) == I2C_STAT_TFNF_Msk)
        {
            if (hi2c->XferSize > 1)
            {
                hi2c->Instance->CMD_DATA = *(hi2c->pBuffPtr + hi2c->XferCount);
            }
            else
            {
                hi2c->Instance->CMD_DATA = (*(hi2c->pBuffPtr + hi2c->XferCount) | 0x0200);//最后1字节数据要产生停止条件
            }
            hi2c->XferSize--;
            hi2c->XferCount++;
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

	//如果没有产生停止条件则等待
	while (!(HAL_I2C_GetITState(hi2c) & I2C_INT_COMP))
	{
        if ((Timeout != HAL_MAX_DELAY) && Check_Ms_Timeout(tickstart,Timeout))
		{
			errorcode = HAL_TIMEOUT;
			goto error;
		}
	}
	HAL_I2C_Clear_IT(hi2c, I2C_INT_COMP);

error:
	hi2c->State = HAL_I2C_STATE_READY;

    xy_free(hi2c->pBuffPtr);

	__HAL_UNLOCK(hi2c);

	return errorcode;
}

/**
 * @brief  I2C主机阻塞接收API.
 *
 * @param  hi2c. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
 * @param  DevAddress：目标从机地址，7位地址时:从机地址bit[8:1]+读写控制bit[0]，10位地址时bit[9:0]有效.
 * @param  pData：要接收的数据的存储地址
 * @param  Size：要接收的数据的长度
 * @param  Timeout：超时时间（单位ms）;
 *         如果Timeout为0，则是不阻塞，检测到接收FIFO空后，则程序立刻退出;
 *         如果Timeout为HAL_MAX_DELAY，则是一直阻塞，直到接收到指定数量的字节后才退出;
 * 		   如果Timeout为非0非HAL_MAX_DELAY时，表示函数等待数据接收完成的最长时间，超时则退出函数.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：超时时间不为0时，表示指定时间内成功接收指定数量的数据；超时时间为0时，也会返回，但实际接收数据的数量通过hi2c结构体中的XferCount来确定
 *         @retval  HAL_ERROR    ：入参错误
 *         @retval  HAL_BUSY     ：外设正在使用中
 *         @retval  HAL_TIMEOUT  ：超时时间不为0时会返回，表示指定时间内未能成功接收指定数量的数据
 * @note   hi2c结构体中的XferCount表示实际接收的字节数.
 */
HAL_StatusTypeDef HAL_I2C_Master_Receive(HAL_I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0;
	HAL_StatusTypeDef errorcode = HAL_OK;

	if (hi2c->State == HAL_I2C_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hi2c->State != HAL_I2C_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK(hi2c);

	tickstart = Get_Tick();

	hi2c->State = HAL_I2C_STATE_BUSY;
	hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
	hi2c->pBuffPtr = (uint8_t *)pData;
	hi2c->XferSize = Size;
	hi2c->XferCount = 0;

	//设置目标从机地址，设置完成后I2C开始寻址直到寻到从机
    if(hi2c->Init.AddressingMode == HAL_I2C_ADDRESS_7BITS)
    {
        HAL_I2C_SetAddr(hi2c,DevAddress>>1);
    }
    else if(hi2c->Init.AddressingMode == HAL_I2C_ADDRESS_10BITS)
    {
        HAL_I2C_SetAddr(hi2c,DevAddress);
    }

	//发送dummy以接收数据
	while (hi2c->XferSize > 0U)
	{
		if ((I2C_Status(hi2c->Instance) & I2C_STAT_TFNF_Msk) == I2C_STAT_TFNF_Msk)
        {
            if (hi2c->XferSize > 1)
            {
                hi2c->Instance->CMD_DATA = 0x0100;
            }
            else
            {
                hi2c->Instance->CMD_DATA = 0x0300;//最后1字节数据要产生停止条件
            }

		    while ((I2C_Status(hi2c->Instance) & I2C_STAT_RFNE_Msk) != I2C_STAT_RFNE_Msk)		
			{
				if ((Timeout != 0U) &&(Timeout != HAL_MAX_DELAY) && Check_Ms_Timeout(tickstart,Timeout))
				{
					errorcode = HAL_TIMEOUT;
					goto error;
				}
			}
            (*hi2c->pBuffPtr) = (uint8_t)(hi2c->Instance->CMD_DATA);
            hi2c->pBuffPtr++;
            hi2c->XferSize--;
            hi2c->XferCount++;
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

	//如果没有产生停止条件则等待
	while (!(HAL_I2C_GetITState(hi2c) & I2C_INT_COMP))
	{
        if ((Timeout != HAL_MAX_DELAY) && Check_Ms_Timeout(tickstart,Timeout))
		{
			errorcode = HAL_TIMEOUT;
			goto error;
		}
	}
	HAL_I2C_Clear_IT(hi2c, I2C_INT_COMP);

error:
	hi2c->State = HAL_I2C_STATE_READY;

	__HAL_UNLOCK(hi2c);

	return errorcode;
}


/**
 * @brief  I2C MEM类设备的阻塞写数据API.
 *
 * @param  hi2c 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
 * @param  DevAddress：目标从机地址，7位地址时:从机地址bit[8:1]+读写控制bit[0]，10位地址时bit[9:0]有效.
 * @param  MemAddress I2C存储器内部偏移地址
 * @param  MemAddSize I2C存储器内部偏移地址宽度 @ref HAL_I2C_MemAddSizeTypeDef.
 * @param  pData：要发送的数据的存储地址
 * @param  Size：要发送的数据的长度
 * @param  Timeout：超时时间（单位ms）;
 *         如果Timeout为HAL_MAX_DELAY，则是一直阻塞，等待发送完指定数量的字节才退出;
 * 		   如果Timeout为非0非HAL_MAX_DELAY时，表示函数等待数据发送完成的最长时间，超时则退出函数.
 * @return HAL_StatusTypeDef 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示发送数据成功，表示指定时间内成功发送指定数量的数据
 *         @retval  HAL_ERROR    ：表示入参错误
 *         @retval  HAL_BUSY     ：表示外设正在使用中
 *         @retval  HAL_TIMEOUT  ：表示指定时间内未能成功发送指定数量的数据
 * @note   hi2c结构体中的XferCount表示实际发送的字节数.
 */
HAL_StatusTypeDef HAL_I2C_Mem_Write(HAL_I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, HAL_I2C_MemAddSizeTypeDef MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    uint32_t tickstart = 0;
	HAL_StatusTypeDef errorcode = HAL_OK;

    assert_param(IS_I2C_MEMADD_SIZE(MemAddSize));

	if (hi2c->State == HAL_I2C_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hi2c->State != HAL_I2C_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U) || (Timeout == 0))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK(hi2c);

	tickstart = Get_Tick();

	hi2c->State = HAL_I2C_STATE_BUSY;
	hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
	hi2c->pBuffPtr = xy_malloc(sizeof(uint8_t) * Size);
	if (hi2c->pBuffPtr  == NULL)
	{
		return HAL_ERROR;
	}
	memcpy(hi2c->pBuffPtr , pData, Size);
	hi2c->XferSize = Size;
	hi2c->XferCount = 0;

	//设置目标从机地址，设置完成后I2C开始寻址直到寻到从机
    if(hi2c->Init.AddressingMode == HAL_I2C_ADDRESS_7BITS)
    {
        HAL_I2C_SetAddr(hi2c,DevAddress>>1);
    }
    else if(hi2c->Init.AddressingMode == HAL_I2C_ADDRESS_10BITS)
    {
        HAL_I2C_SetAddr(hi2c,DevAddress);
    }

    //发送I2C存储器地址
    if(MemAddSize == HAL_I2C_MEMADD_SIZE_8BIT)
    {
        hi2c->Instance->CMD_DATA = (uint8_t)((uint16_t)(MemAddress & (uint16_t)0x00FF));
    }
    else if(MemAddSize == HAL_I2C_MEMADD_SIZE_16BIT)
    {
        hi2c->Instance->CMD_DATA = (uint8_t)((uint16_t)(((uint16_t)(MemAddress & (uint16_t)0xFF00)) >> 8));
        hi2c->Instance->CMD_DATA = (uint8_t)((uint16_t)(MemAddress & (uint16_t)0x00FF));
    }

	//发送数据
	while (hi2c->XferSize > 0U)
	{
		if ((I2C_Status(hi2c->Instance) & I2C_STAT_TFNF_Msk) == I2C_STAT_TFNF_Msk)
        {
            if (hi2c->XferSize > 1)
            {
                hi2c->Instance->CMD_DATA = *(hi2c->pBuffPtr + hi2c->XferCount);
            }
            else
            {
                hi2c->Instance->CMD_DATA = (*(hi2c->pBuffPtr + hi2c->XferCount) | 0x0200);//最后1字节数据要产生停止条件
            }
            hi2c->XferSize--;
            hi2c->XferCount++;
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

	//如果没有产生停止条件则等待
	while (!(HAL_I2C_GetITState(hi2c) & I2C_INT_COMP))
	{
        if ((Timeout != HAL_MAX_DELAY) && Check_Ms_Timeout(tickstart,Timeout))
		{
			errorcode = HAL_TIMEOUT;
			goto error;
		}
	}
	HAL_I2C_Clear_IT(hi2c, I2C_INT_COMP);

error:
	hi2c->State = HAL_I2C_STATE_READY;

    xy_free(hi2c->pBuffPtr);

	__HAL_UNLOCK(hi2c);

	return errorcode;
}

/**
 * @brief  I2C MEM类设备的阻塞读数据API.
 *
 * @param  hi2c 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
 * @param  DevAddress：目标从机地址，7位地址时:从机地址bit[8:1]+读写控制bit[0]，10位地址时bit[9:0]有效.
 * @param  MemAddress I2C存储器内部偏移地址
 * @param  MemAddSize I2C存储器内部偏移地址宽度 @ref HAL_I2C_MemAddSizeTypeDef.
 * @param  pData：要接收的数据的存储地址
 * @param  Size：要接收的数据的长度
 * @param  TTimeout：超时时间（单位ms）;
 *         如果Timeout（超时）为0，则是不阻塞，检测到接收FIFO空后，则程序立刻退出;
 *         如果Timeout为HAL_MAX_DELAY 是一直阻塞，直到接收到指定数量的字节后才退出;
 * 		   如果Timeout为非0非HAL_MAX_DELAY时，表示函数等待数据接收完成的最长时间，超时则退出函数.
 * @return HAL_StatusTypeDef 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：超时时间不为0时，表示指定时间内成功接收指定数量的数据；超时时间为0时，也会返回，但实际接收数据的数量通过hi2c结构体中的XferCount来确定
 *         @retval  HAL_ERROR    ：表示入参错误
 *         @retval  HAL_BUSY     ：表示外设正在使用中
 *         @retval  HAL_TIMEOUT  ：表示指定时间内未能成功发送指定数量的数据
 * @note   hi2c结构体中的XferCount表示实际接收的字节数.
 */
HAL_StatusTypeDef HAL_I2C_Mem_Read(HAL_I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, HAL_I2C_MemAddSizeTypeDef MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0;
	HAL_StatusTypeDef errorcode = HAL_OK;

    assert_param(IS_I2C_MEMADD_SIZE(MemAddSize));

	if (hi2c->State == HAL_I2C_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hi2c->State != HAL_I2C_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK(hi2c);

	tickstart = Get_Tick();

	hi2c->State = HAL_I2C_STATE_BUSY;
	hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
	hi2c->pBuffPtr = (uint8_t *)pData;
	hi2c->XferSize = Size;
	hi2c->XferCount = 0;

	//设置目标从机地址，设置完成后I2C开始寻址直到寻到从机
    if(hi2c->Init.AddressingMode == HAL_I2C_ADDRESS_7BITS)
    {
        HAL_I2C_SetAddr(hi2c,DevAddress>>1);
    }
    else if(hi2c->Init.AddressingMode == HAL_I2C_ADDRESS_10BITS)
    {
        HAL_I2C_SetAddr(hi2c,DevAddress);
    }

    //发送I2C存储器地址
    if(MemAddSize == HAL_I2C_MEMADD_SIZE_8BIT)
    {
        hi2c->Instance->CMD_DATA = (uint8_t)((uint16_t)(MemAddress & (uint16_t)0x00FF));
    }
    else if(MemAddSize == HAL_I2C_MEMADD_SIZE_16BIT)
    {
        hi2c->Instance->CMD_DATA = (uint8_t)((uint16_t)(((uint16_t)(MemAddress & (uint16_t)0xFF00)) >> 8));
        hi2c->Instance->CMD_DATA = (uint8_t)((uint16_t)(MemAddress & (uint16_t)0x00FF));
    }

	//发送dummy以接收数据
	while (hi2c->XferSize > 0U)
	{
		if ((I2C_Status(hi2c->Instance) & I2C_STAT_TFNF_Msk) == I2C_STAT_TFNF_Msk)
        {
            if (hi2c->XferSize > 1)
            {
                hi2c->Instance->CMD_DATA = 0x0100;
            }
            else
            {
                hi2c->Instance->CMD_DATA = 0x0300;//最后1字节数据要产生停止条件
            }

		    while ((I2C_Status(hi2c->Instance) & I2C_STAT_RFNE_Msk) != I2C_STAT_RFNE_Msk)
			{
				if ((Timeout != 0U) &&(Timeout != HAL_MAX_DELAY) && Check_Ms_Timeout(tickstart,Timeout))
				{
					errorcode = HAL_TIMEOUT;
					goto error;
				}
			}
            (*hi2c->pBuffPtr) = (uint8_t)(hi2c->Instance->CMD_DATA);
            hi2c->pBuffPtr++;
            hi2c->XferSize--;
            hi2c->XferCount++;
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

	//如果没有产生停止条件则等待
	while (!(HAL_I2C_GetITState(hi2c) & I2C_INT_COMP))
	{
        if ((Timeout != HAL_MAX_DELAY) && Check_Ms_Timeout(tickstart,Timeout))
		{
			errorcode = HAL_TIMEOUT;
			goto error;
		}
	}
	HAL_I2C_Clear_IT(hi2c, I2C_INT_COMP);

error:
	hi2c->State = HAL_I2C_STATE_READY;

	__HAL_UNLOCK(hi2c);

	return errorcode;
}

/**
 * @brief  I2C主机非阻塞发送API.
 *
 * @param  hi2c. 详见结构体定义 HAL_I2C_HandleTypeDef.
 * @param  DevAddress：目标从机地址，7位地址时:从机地址bit[8:1]+读写控制bit[0]，10位地址时bit[9:0]有效.
 * @param  pData：要发送的数据的存储地址
 * @param  Size：要发送的数据的长度
 * @return 函数执行状态.详见HAL_StatusTypeDef.
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示中断发送需要的参数配置、外设配置成功，等待进入中断发送数据
 *         @retval  HAL_ERROR    ：表示入参错误
 *         @retval  HAL_BUSY     ：表示外设正在使用中
 */
HAL_StatusTypeDef HAL_I2C_Master_Transmit_IT(HAL_I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size)
{
	if (hi2c->State == HAL_I2C_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hi2c->State != HAL_I2C_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK(hi2c);

	hi2c->State = HAL_I2C_STATE_BUSY;
	hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
	hi2c->pBuffPtr  = xy_malloc(sizeof(uint8_t) * Size);
	if (hi2c->pBuffPtr  == NULL)
	{
		return HAL_ERROR;
	}
	memcpy(hi2c->pBuffPtr , pData, Size);
	hi2c->XferSize = Size;
	hi2c->XferCount = 0;
	hi2c->XferDir = HAL_I2C_DIR_WRITE;

	//清除所有中断标志位
	HAL_I2C_Clear_IT(hi2c, I2C_INT_ALL);

    //设置TXFIFO阈值为0
    hi2c->Instance->TX_FIFO_THD = 0;

    //注册中断
    HAL_I2C_NVIC_IntRegister(hi2c);

	//设置目标从机地址，设置完成后I2C开始寻址直到寻到从机
    if(hi2c->Init.AddressingMode == HAL_I2C_ADDRESS_7BITS)
    {
        HAL_I2C_SetAddr(hi2c,DevAddress>>1);
    }
    else if(hi2c->Init.AddressingMode == HAL_I2C_ADDRESS_10BITS)
    {
        HAL_I2C_SetAddr(hi2c,DevAddress);
    }

    //发送数据，根据想发送数据长度设置本次发送长度和停止条件
    uint8_t real_write_size = 0;
    uint8_t set_stop = 0;
    if(hi2c->XferSize > HAL_I2C_MAX_XFERSIZE)
    {
        set_stop = 0;
        real_write_size = HAL_I2C_MAX_XFERSIZE;
    }
    else
    {
        set_stop = 1;
        real_write_size = hi2c->XferSize;
    }
    while(real_write_size > 0U)
    {
        if ((I2C_Status(hi2c->Instance) & I2C_STAT_TFNF_Msk) == I2C_STAT_TFNF_Msk)
        {
            if(set_stop == 0)
            {
                hi2c->Instance->CMD_DATA = *(hi2c->pBuffPtr + hi2c->XferCount);
            }
            else
            {
                if (real_write_size > 1)
                {
                    hi2c->Instance->CMD_DATA = *(hi2c->pBuffPtr + hi2c->XferCount);
                }
                else
                {
                    hi2c->Instance->CMD_DATA = (*(hi2c->pBuffPtr + hi2c->XferCount) | 0x0200);//最后1字节数据要产生停止条件
                }
            }
            real_write_size--;
            hi2c->XferSize--;
            hi2c->XferCount++;
        }
    }

	//使能中断源
	HAL_I2C_Enable_IT(hi2c, HAL_I2C_INTTYPE_TX_FIFO_THD);
	HAL_I2C_Enable_IT(hi2c, HAL_I2C_INTTYPE_ARB_LOST);
	HAL_I2C_Enable_IT(hi2c, HAL_I2C_INTTYPE_SCL_TIMEOUT);

	__HAL_UNLOCK(hi2c);

	return HAL_OK;
}

/**
 * @brief  I2C主机非阻塞接收API.
 *
 * @param  hi2c. 详见结构体定义 HAL_I2C_HandleTypeDef.
 * @param  DevAddress：目标从机地址，7位地址时:从机地址bit[8:1]+读写控制bit[0]，10位地址时bit[9:0]有效.
 * @param  pData：要接收的数据的存储地址
 * @param  Size：要接收的数据的长度
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示中断接收需要的参数配置、外设配置成功，等待进入中断接收数据
 *         @retval  HAL_ERROR    ：表示入参错误
 *         @retval  HAL_BUSY     ：外设正在使用中
 */
HAL_StatusTypeDef HAL_I2C_Master_Receive_IT(HAL_I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size)
{
    if (hi2c->State == HAL_I2C_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hi2c->State != HAL_I2C_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK(hi2c);

	hi2c->State = HAL_I2C_STATE_BUSY;
	hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
	hi2c->pBuffPtr = (uint8_t *)pData;
	hi2c->XferSize = Size;
	hi2c->XferCount = 0;
	hi2c->XferDir = HAL_I2C_DIR_READ;

	//清除所有中断标志位
	HAL_I2C_Clear_IT(hi2c, I2C_INT_ALL);

    //根据想接收数据长度设置RXFIFO阈值，当RXFIFO数据存量大于等于阈值时触发RXFIFO阈值中断
    hi2c->Instance->RX_FIFO_THD = (hi2c->XferSize > HAL_I2C_MAX_XFERSIZE) ? HAL_I2C_MAX_XFERSIZE : hi2c->XferSize;

    //注册中断
    HAL_I2C_NVIC_IntRegister(hi2c);



	//设置目标从机地址，设置完成后I2C开始寻址直到寻到从机
    if(hi2c->Init.AddressingMode == HAL_I2C_ADDRESS_7BITS)
    {
        HAL_I2C_SetAddr(hi2c,DevAddress>>1);
    }
    else if(hi2c->Init.AddressingMode == HAL_I2C_ADDRESS_10BITS)
    {
        HAL_I2C_SetAddr(hi2c,DevAddress);
    }

    //发送dummy以接收数据，根据想接收数据长度设置本次发送长度和停止条件
    uint8_t dummy_write_size = 0;
    uint8_t set_stop = 0;
    if(hi2c->XferSize > HAL_I2C_MAX_XFERSIZE)
    {
        set_stop = 0;
        dummy_write_size = HAL_I2C_MAX_XFERSIZE;
    }
    else
    {
        set_stop = 1;
        dummy_write_size = hi2c->XferSize;
    }
    while(dummy_write_size > 0U)
    {
        if ((I2C_Status(hi2c->Instance) & I2C_STAT_TFNF_Msk) == I2C_STAT_TFNF_Msk)
        {
            if(set_stop == 0)
            {
                hi2c->Instance->CMD_DATA = 0x0100;
            }
            else
            {
                if (dummy_write_size > 1)
                {
                    hi2c->Instance->CMD_DATA = 0x0100;
                }
                else
                {
                    hi2c->Instance->CMD_DATA = 0x0300;//最后1字节数据要产生停止条件
                }
            }
            dummy_write_size--;
        }
    }
	//使能中断源
    HAL_I2C_Enable_IT(hi2c, HAL_I2C_INTTYPE_RX_FIFO_THD);
	HAL_I2C_Enable_IT(hi2c, HAL_I2C_INTTYPE_RX_FIFO_OVF);
	HAL_I2C_Enable_IT(hi2c, HAL_I2C_INTTYPE_ARB_LOST);
	HAL_I2C_Enable_IT(hi2c, HAL_I2C_INTTYPE_SCL_TIMEOUT);
	
    __HAL_UNLOCK(hi2c);

	return HAL_OK;
}


/**
 * @brief  I2C MEM类设备的非阻塞写数据API.
 *
 * @param  hi2c 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
 * @param  DevAddress：目标从机地址，7位地址时:从机地址bit[8:1]+读写控制bit[0]，10位地址时bit[9:0]有效.
 * @param  MemAddress I2C存储器内部偏移地址
 * @param  MemAddSize I2C存储器内部偏移地址宽度 @ref HAL_I2C_MemAddSizeTypeDef.
 * @param  pData：要发送的数据的存储地址
 * @param  Size：要发送的数据的长度
 * @return 函数执行状态.详见HAL_StatusTypeDef.
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示中断发送需要的参数配置、外设配置成功，等待进入中断发送数据
 *         @retval  HAL_ERROR    ：表示入参错误
 *         @retval  HAL_BUSY     ：表示外设正在使用中
 */
HAL_StatusTypeDef HAL_I2C_Mem_Write_IT(HAL_I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, HAL_I2C_MemAddSizeTypeDef MemAddSize, uint8_t *pData, uint16_t Size)
{
	assert_param(IS_I2C_MEMADD_SIZE(MemAddSize));

    if (hi2c->State == HAL_I2C_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hi2c->State != HAL_I2C_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK(hi2c);

	hi2c->State = HAL_I2C_STATE_BUSY;
	hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
	hi2c->pBuffPtr  = xy_malloc(sizeof(uint8_t) * Size);
	if (hi2c->pBuffPtr  == NULL)
	{
		return HAL_ERROR;
	}
	memcpy(hi2c->pBuffPtr , pData, Size);
	hi2c->XferSize = Size;
	hi2c->XferCount = 0;
	hi2c->XferDir = HAL_I2C_DIR_WRITE;

	//清除所有标志位
	HAL_I2C_Clear_IT(hi2c, I2C_INT_ALL);

    //设置TXFIFO阈值为0
    hi2c->Instance->TX_FIFO_THD = 0;

    //注册中断
    HAL_I2C_NVIC_IntRegister(hi2c);

	//使能中断源
	HAL_I2C_Enable_IT(hi2c, HAL_I2C_INTTYPE_TX_FIFO_THD);
	HAL_I2C_Enable_IT(hi2c, HAL_I2C_INTTYPE_ARB_LOST);
	HAL_I2C_Enable_IT(hi2c, HAL_I2C_INTTYPE_SCL_TIMEOUT);

	//设置目标从机地址，设置完成后I2C开始寻址直到寻到从机
    if(hi2c->Init.AddressingMode == HAL_I2C_ADDRESS_7BITS)
    {
        HAL_I2C_SetAddr(hi2c,DevAddress>>1);
    }
    else if(hi2c->Init.AddressingMode == HAL_I2C_ADDRESS_10BITS)
    {
        HAL_I2C_SetAddr(hi2c,DevAddress);
    }

    //发送I2C存储器地址
    if(MemAddSize == HAL_I2C_MEMADD_SIZE_8BIT)
    {
        hi2c->Instance->CMD_DATA = (uint8_t)((uint16_t)(MemAddress & (uint16_t)0x00FF));
    }
    else if(MemAddSize == HAL_I2C_MEMADD_SIZE_16BIT)
    {
        hi2c->Instance->CMD_DATA = (uint8_t)((uint16_t)(((uint16_t)(MemAddress & (uint16_t)0xFF00)) >> 8));
        hi2c->Instance->CMD_DATA = (uint8_t)((uint16_t)(MemAddress & (uint16_t)0x00FF));
    }

    //发送数据，根据想发送数据长度设置本次发送长度和停止条件
    uint8_t real_write_size = 0;
    uint8_t set_stop = 0;
    if(hi2c->XferSize > HAL_I2C_MAX_XFERSIZE)
    {
        set_stop = 0;
        real_write_size = HAL_I2C_MAX_XFERSIZE;
    }
    else
    {
        set_stop = 1;
        real_write_size = hi2c->XferSize;
    }
    while(real_write_size > 0U)
    {
        if ((I2C_Status(hi2c->Instance) & I2C_STAT_TFNF_Msk) == I2C_STAT_TFNF_Msk)
        {
            if(set_stop == 0)
            {
                hi2c->Instance->CMD_DATA = *(hi2c->pBuffPtr + hi2c->XferCount);
            }
            else
            {
                if (real_write_size > 1)
                {
                    hi2c->Instance->CMD_DATA = *(hi2c->pBuffPtr + hi2c->XferCount);
                }
                else
                {
                    hi2c->Instance->CMD_DATA = (*(hi2c->pBuffPtr + hi2c->XferCount) | 0x0200);//最后1字节数据要产生停止条件
                }
            }
            real_write_size--;
            hi2c->XferSize--;
            hi2c->XferCount++;
        }
    }

	__HAL_UNLOCK(hi2c);

	return HAL_OK;
}

/**
 * @brief  I2C MEM类设备的非阻塞读数据API.
 *
 * @param  hi2c 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
 * @param  DevAddress：目标从机地址，7位地址时:从机地址bit[8:1]+读写控制bit[0]，10位地址时bit[9:0]有效.
 * @param  MemAddress I2C存储器内部偏移地址
 * @param  MemAddSize I2C存储器内部偏移地址宽度 @ref HAL_I2C_MemAddSizeTypeDef.
 * @param  pData：要接收的数据的存储地址
 * @param  Size：要接收的数据的长度
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示中断接收需要的参数配置、外设配置成功，等待进入中断接收数据
 *         @retval  HAL_ERROR    ：表示入参错误
 *         @retval  HAL_BUSY     ：外设正在使用中
 */
HAL_StatusTypeDef HAL_I2C_Mem_Read_IT(HAL_I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, HAL_I2C_MemAddSizeTypeDef MemAddSize, uint8_t *pData, uint16_t Size)
{
	assert_param(IS_I2C_MEMADD_SIZE(MemAddSize));

    if (hi2c->State == HAL_I2C_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hi2c->State != HAL_I2C_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pData == NULL) || (Size == 0U))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK(hi2c);

	hi2c->State = HAL_I2C_STATE_BUSY;
	hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
	hi2c->pBuffPtr = (uint8_t *)pData;
	hi2c->XferSize = Size;
	hi2c->XferCount = 0;
	hi2c->XferDir = HAL_I2C_DIR_READ;

	//清除所有标志位
	HAL_I2C_Clear_IT(hi2c, I2C_INT_ALL);

    //根据想接收数据长度设置RXFIFO阈值，当RXFIFO数据存量大于等于阈值时触发RXFIFO阈值中断
    hi2c->Instance->RX_FIFO_THD = (hi2c->XferSize > HAL_I2C_MAX_XFERSIZE) ? HAL_I2C_MAX_XFERSIZE : hi2c->XferSize;

    //注册中断
    HAL_I2C_NVIC_IntRegister(hi2c);

	//使能中断源
    HAL_I2C_Enable_IT(hi2c, HAL_I2C_INTTYPE_RX_FIFO_THD);
	HAL_I2C_Enable_IT(hi2c, HAL_I2C_INTTYPE_RX_FIFO_OVF);
	HAL_I2C_Enable_IT(hi2c, HAL_I2C_INTTYPE_ARB_LOST);
	HAL_I2C_Enable_IT(hi2c, HAL_I2C_INTTYPE_SCL_TIMEOUT);

	//设置目标从机地址，设置完成后I2C开始寻址直到寻到从机
    if(hi2c->Init.AddressingMode == HAL_I2C_ADDRESS_7BITS)
    {
        HAL_I2C_SetAddr(hi2c,DevAddress>>1);
    }
    else if(hi2c->Init.AddressingMode == HAL_I2C_ADDRESS_10BITS)
    {
        HAL_I2C_SetAddr(hi2c,DevAddress);
    }

    //发送I2C存储器地址
    if(MemAddSize == HAL_I2C_MEMADD_SIZE_8BIT)
    {
        hi2c->Instance->CMD_DATA = (uint8_t)((uint16_t)(MemAddress & (uint16_t)0x00FF));
    }
    else if(MemAddSize == HAL_I2C_MEMADD_SIZE_16BIT)
    {
        hi2c->Instance->CMD_DATA = (uint8_t)((uint16_t)(((uint16_t)(MemAddress & (uint16_t)0xFF00)) >> 8));
        hi2c->Instance->CMD_DATA = (uint8_t)((uint16_t)(MemAddress & (uint16_t)0x00FF));
    }

    //发送dummy以接收数据，根据想接收数据长度设置本次发送长度和停止条件
    uint8_t dummy_write_size = 0;
    uint8_t set_stop = 0;
    if(hi2c->XferSize > HAL_I2C_MAX_XFERSIZE)
    {
        set_stop = 0;
        dummy_write_size = HAL_I2C_MAX_XFERSIZE;
    }
    else
    {
        set_stop = 1;
        dummy_write_size = hi2c->XferSize;
    }
    while(dummy_write_size > 0U)
    {
        if ((I2C_Status(hi2c->Instance) & I2C_STAT_TFNF_Msk) == I2C_STAT_TFNF_Msk)
        {
            if(set_stop == 0)
            {
                hi2c->Instance->CMD_DATA = 0x0100;
            }
            else
            {
                if (dummy_write_size > 1)
                {
                    hi2c->Instance->CMD_DATA = 0x0100;
                }
                else
                {
                    hi2c->Instance->CMD_DATA = 0x0300;//最后1字节数据要产生停止条件
                }
            }
            dummy_write_size--;
        }
    }

    __HAL_UNLOCK(hi2c);

	return HAL_OK;
}



/**
 * @brief  本地I2C错误回调函数
 *
 * @param  hi2c. 详见结构体定义 HAL_I2C_HandleTypeDef.
 */
static void HAL_I2C_ErrorCallback(HAL_I2C_HandleTypeDef *hi2c)
{
	if (hi2c->Instance == HAL_I2C1)
	{
		HAL_I2C1_ErrorCallback(hi2c);
	}
	else if (hi2c->Instance == HAL_I2C2)
	{
		HAL_I2C2_ErrorCallback(hi2c);
	}
}

/**
 * @brief  本地I2C发送完成回调函数
 *
 * @param  hi2c. 详见结构体定义 HAL_I2C_HandleTypeDef.
 */
static void HAL_I2C_TxCpltCallback(HAL_I2C_HandleTypeDef *hi2c)
{
	if (hi2c->Instance == HAL_I2C1)
	{
		HAL_I2C1_TxCpltCallback(hi2c);
	}
	else if (hi2c->Instance == HAL_I2C2)
	{
		HAL_I2C2_TxCpltCallback(hi2c);
	}
}

/**
 * @brief  本地I2C接收完成回调函数
 *
 * @param  hi2c. 详见结构体定义 HAL_I2C_HandleTypeDef.
 */
static void HAL_I2C_RxCpltCallback(HAL_I2C_HandleTypeDef *hi2c)
{
	if (hi2c->Instance == HAL_I2C1)
	{
		HAL_I2C1_RxCpltCallback(hi2c);
	}
	else if (hi2c->Instance == HAL_I2C2)
	{
		HAL_I2C2_RxCpltCallback(hi2c);
	}
}

/**
 * @brief  本地I2C中断服务函数
 *
 */
void HAL_I2C_IRQHandler(void)
{
	//轮询句柄指针
	for (uint8_t i = 0; i < I2C_NUM; i++)
	{
		if(g_i2c_handle[i] == NULL)
		{
			continue;
		}
	
		HAL_I2C_IntTypeDef ITState = HAL_I2C_GetITState(g_i2c_handle[i]);
		HAL_I2C_IntTypeDef ITEnable = HAL_I2C_GetITEnable(g_i2c_handle[i]);

		//错误处理流程：发生错误中断
		if ((HAL_RESET == (ITEnable & HAL_I2C_INTTYPE_RX_FIFO_OVF)) && (HAL_RESET != (ITState & HAL_I2C_INTTYPE_RX_FIFO_OVF)))
		{
			g_i2c_handle[i]->ErrorCode |= HAL_I2C_ERROR_RX_FIFO_OVF;
			HAL_I2C_Disable_IT(g_i2c_handle[i], HAL_I2C_INTTYPE_RX_FIFO_OVF);
			HAL_I2C_Clear_IT(g_i2c_handle[i], HAL_I2C_INTTYPE_RX_FIFO_OVF);
		}
		if ((HAL_RESET == (ITEnable & HAL_I2C_INTTYPE_ARB_LOST)) && (HAL_RESET != (ITState & HAL_I2C_INTTYPE_ARB_LOST)))
		{
			g_i2c_handle[i]->ErrorCode |= HAL_I2C_ERROR_ARB_LOST;
			HAL_I2C_Disable_IT(g_i2c_handle[i], HAL_I2C_INTTYPE_ARB_LOST);
			HAL_I2C_Clear_IT(g_i2c_handle[i], HAL_I2C_INTTYPE_ARB_LOST);
		}
		if ((HAL_RESET == (ITEnable & HAL_I2C_INTTYPE_SCL_TIMEOUT)) && (HAL_RESET != (ITState & HAL_I2C_INTTYPE_SCL_TIMEOUT)))
		{
			g_i2c_handle[i]->ErrorCode |= HAL_I2C_ERROR_SCL_TIMEOUT;
			HAL_I2C_Disable_IT(g_i2c_handle[i], HAL_I2C_INTTYPE_SCL_TIMEOUT);
			HAL_I2C_Clear_IT(g_i2c_handle[i], HAL_I2C_INTTYPE_SCL_TIMEOUT);
		}
		if (g_i2c_handle[i]->ErrorCode != HAL_I2C_ERROR_NONE)
		{
			HAL_I2C_ErrorCallback(g_i2c_handle[i]);

			goto END;
		}

		//主机处理流程
		if (g_i2c_handle[i]->Init.Mode == HAL_I2C_MODE_MASTER)
		{
			//主机接收流程：rxfifo阈值中断
			if (g_i2c_handle[i]->XferDir == HAL_I2C_DIR_READ)
			{
				if ((HAL_RESET == (ITEnable & HAL_I2C_INTTYPE_RX_FIFO_THD)) && (HAL_RESET != (ITState & HAL_I2C_INTTYPE_RX_FIFO_THD)))
				{
					while ((I2C_Status(g_i2c_handle[i]->Instance) & I2C_STAT_RFNE_Msk) == I2C_STAT_RFNE_Msk)
					{
						(*g_i2c_handle[i]->pBuffPtr) = (uint8_t)(g_i2c_handle[i]->Instance->CMD_DATA);
						g_i2c_handle[i]->pBuffPtr++;
						g_i2c_handle[i]->XferSize--;
						g_i2c_handle[i]->XferCount++;
						if (g_i2c_handle[i]->XferSize == 0U)
						{
							HAL_I2C_Disable_IT(g_i2c_handle[i], HAL_I2C_INTTYPE_RX_FIFO_THD);
							HAL_I2C_Clear_IT(g_i2c_handle[i], HAL_I2C_INTTYPE_RX_FIFO_THD);
							g_i2c_handle[i]->XferDir = HAL_I2C_DIR_NONE;
							g_i2c_handle[i]->State = HAL_I2C_STATE_READY;
						
							HAL_I2C_RxCpltCallback(g_i2c_handle[i]);

							goto END;
						}
					}
					//根据想接收数据长度设置RXFIFO阈值，当RXFIFO数据存量大于等于阈值时触发RXFIFO阈值中断
					g_i2c_handle[i]->Instance->RX_FIFO_THD = (g_i2c_handle[i]->XferSize > HAL_I2C_MAX_XFERSIZE) ? HAL_I2C_MAX_XFERSIZE : g_i2c_handle[i]->XferSize;
					//发送dummy以接收数据，根据想接收数据长度设置本次发送长度和停止条件
					uint8_t dummy_write_size = 0;
					uint8_t set_stop = 0;
					if(g_i2c_handle[i]->XferSize > HAL_I2C_MAX_XFERSIZE)
					{
						set_stop = 0;
						dummy_write_size = HAL_I2C_MAX_XFERSIZE;
					}
					else
					{
						set_stop = 1;
						dummy_write_size = g_i2c_handle[i]->XferSize;
					}
					while(dummy_write_size > 0U)
					{
						if ((I2C_Status(g_i2c_handle[i]->Instance) & I2C_STAT_TFNF_Msk) == I2C_STAT_TFNF_Msk)
						{
							if(set_stop == 0)
							{
								g_i2c_handle[i]->Instance->CMD_DATA = 0x0100;
							}
							else
							{
								if (dummy_write_size > 1)
								{
									g_i2c_handle[i]->Instance->CMD_DATA = 0x0100;
								}
								else
								{
									g_i2c_handle[i]->Instance->CMD_DATA = 0x0300;//最后1字节数据要产生停止条件
								}
							}
							dummy_write_size--;
						}
					}
					HAL_I2C_Clear_IT(g_i2c_handle[i], HAL_I2C_INTTYPE_RX_FIFO_THD);
				}
			}
			//主机发送流程：txfifo阈值中断
			else if (g_i2c_handle[i]->XferDir == HAL_I2C_DIR_WRITE)
			{
				if ((HAL_RESET == (ITEnable & HAL_I2C_INTTYPE_TX_FIFO_THD)) && (HAL_RESET != (ITState & HAL_I2C_INTTYPE_TX_FIFO_THD)))
				{
					if (g_i2c_handle[i]->XferSize == 0U)
					{
						HAL_I2C_Disable_IT(g_i2c_handle[i], HAL_I2C_INTTYPE_TX_FIFO_THD);
						HAL_I2C_Clear_IT(g_i2c_handle[i], HAL_I2C_INTTYPE_TX_FIFO_THD);
						g_i2c_handle[i]->XferDir = HAL_I2C_DIR_NONE;
						g_i2c_handle[i]->State = HAL_I2C_STATE_READY;
						xy_free(g_i2c_handle[i]->pBuffPtr);
						
						HAL_I2C_TxCpltCallback(g_i2c_handle[i]);

						goto END;
					}
					else
					{
						//发送数据，根据想发送数据长度设置本次发送长度和停止条件
						uint8_t real_write_size = 0;
						uint8_t set_stop = 0;
						if(g_i2c_handle[i]->XferSize > HAL_I2C_MAX_XFERSIZE)
						{
							set_stop = 0;
							real_write_size = HAL_I2C_MAX_XFERSIZE;
						}
						else
						{
							set_stop = 1;
							real_write_size = g_i2c_handle[i]->XferSize;
						}
						HAL_I2C_Clear_IT(g_i2c_handle[i], HAL_I2C_INTTYPE_TX_FIFO_THD);
						while(real_write_size > 0U)
						{
							if ((I2C_Status(g_i2c_handle[i]->Instance) & I2C_STAT_TFNF_Msk) == I2C_STAT_TFNF_Msk)
							{
								if(set_stop == 0)
								{
									g_i2c_handle[i]->Instance->CMD_DATA = *(g_i2c_handle[i]->pBuffPtr + g_i2c_handle[i]->XferCount);
								}
								else
								{
									if (real_write_size > 1)
									{
										g_i2c_handle[i]->Instance->CMD_DATA = *(g_i2c_handle[i]->pBuffPtr + g_i2c_handle[i]->XferCount);
									}
									else
									{
										g_i2c_handle[i]->Instance->CMD_DATA = (*(g_i2c_handle[i]->pBuffPtr + g_i2c_handle[i]->XferCount) | 0x0200);//最后1字节数据要产生停止条件
									}
								}
								real_write_size--;
								g_i2c_handle[i]->XferSize--;
								g_i2c_handle[i]->XferCount++;
							}
						}
						
					}
				}
			}
		}
	}
END:
	return;
}
