/**
 ******************************************************************************
 * @file      hal_i2c.h
 * @brief     此文件包含i2c外设的变量，枚举，结构体定义，函数声明等.
 * @attention I2C支持100K与400K两种速率模式.
 ******************************************************************************
 * @attention
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 * http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

#pragma once
#include "hal_def.h"
#include "i2c.h"
#include "prcm.h"
#include "xy_timer.h"
#include "xy_system.h"

/**
 * @brief  I2C中断类型枚举
 */
typedef enum
{
    HAL_I2C_INTTYPE_ARB_LOST = I2C_INT_ARB_LOST,    /*!< I2C仲裁丢失中断 */
    HAL_I2C_INTTYPE_SCL_TIMEOUT = I2C_INT_TIMEOUT,  /*!< I2C SCL超时中断 */
    HAL_I2C_INTTYPE_RX_FIFO_OVF = I2C_INT_RX_OVF,   /*!< I2C RXFIFO溢出中断 */
    HAL_I2C_INTTYPE_RX_FIFO_THD = I2C_INT_RX_AF,    /*!< I2C RXFIFO阈值中断，当RXFIFO现存量大于等于阈值时触发 */
    HAL_I2C_INTTYPE_TX_FIFO_THD = I2C_INT_TX_AE     /*!< I2C TXFIFO阈值中断，当TXFIFO现存量小于等于阈值时触发 */
} HAL_I2C_IntTypeDef;

#define I2C_NUM		2 //指示外设个数

/**
 * @brief I2C私有保护宏
 *
 */
#define IS_I2C_INSTANCE(__INSTANCE__)   (((__INSTANCE__) == HAL_I2C1) || \
                                         ((__INSTANCE__) == HAL_I2C2))

#define IS_I2C_MEMADD_SIZE(SIZE)        (((SIZE) == HAL_I2C_MEMADD_SIZE_8BIT) || \
                                         ((SIZE) == HAL_I2C_MEMADD_SIZE_16BIT))

/**
 * @brief I2C主从收发中断过程中，单次中断传输数据个数，该值必须小于等于32
 *
 */
#define HAL_I2C_MAX_XFERSIZE    (24)

/**
 * @brief  I2C工作状态枚举
 */
typedef enum
{
    HAL_I2C_STATE_RESET = 0x00U,    /*!< I2C未配置 */
    HAL_I2C_STATE_READY = 0x01U,    /*!< I2C已初始化完成 */
    HAL_I2C_STATE_BUSY = 0x02U,     /*!< I2C正被占用 */
    HAL_I2C_STATE_TIMEOUT = 0x04U, /*!< I2C接收或发送超时 */
    HAL_I2C_STATE_ERROR = 0x08U    /*!< I2C错误 */
} HAL_I2C_StateTypeDef;

/**
 * @brief  I2C错误码类型枚举
 */
typedef enum
{
    HAL_I2C_ERROR_NONE = 0x00U,         /*!<  I2C无错误 */
    HAL_I2C_ERROR_ARB_LOST = 0x01U,     /*!<  I2C仲裁丢失错误 */
    HAL_I2C_ERROR_SCL_TIMEOUT = 0x02U,  /*!<  I2C超时错误 */
    HAL_I2C_ERROR_RX_FIFO_OVF = 0x04U   /*!<  I2C接收溢出错误 */
} HAL_I2C_ErrorCodeTypeDef;

/**
 * @brief  I2C传输模式枚举
 */
typedef enum
{
    HAL_I2C_MODE_SLAVE = I2C_slave,                     /*!< I2C从模式 */
    HAL_I2C_MODE_MASTER = I2C_master_normal_noStartByte /*!< I2C主模式 */
} HAL_I2C_ModeTypeDef;

/**
 * @brief  I2C传输速度枚举
 */
typedef enum
{
    HAL_I2C_SPEED_100K = I2C_standard_mode,    /*!< I2C传输速度100K  */
    HAL_I2C_SPEED_400K = I2C_fast_mode         /*!< I2C传输速度400K  */
} HAL_I2C_CLKSpeedTypeDef;

/**
 * @brief  I2C传输地址模式枚举
 */
typedef enum
{
    HAL_I2C_ADDRESS_7BITS = I2C_addr_7b,  /*!< I2C从机地址为7位 */
    HAL_I2C_ADDRESS_10BITS = I2C_addr_10b /*!< I2C从机地址为10位 */
} HAL_I2C_AddressModeTypeDef;

/**
 * @brief  I2C传输方向枚举
 */
typedef enum
{
    HAL_I2C_DIR_NONE = 0x00U,  /*!< 初始化时使用，此时没有调用发送、接收接口，无需指定方向 */
    HAL_I2C_DIR_WRITE = 0x01U, /*!< I2C写/I2C发送 */
    HAL_I2C_DIR_READ = 0x02U   /*!< I2C读/I2C接收 */
} HAL_I2C_DirTypeDef;

/**
 * @brief  I2C MEM类设备内部偏移地址宽度枚举
 */
typedef enum
{
    HAL_I2C_MEMADD_SIZE_8BIT = 0x01U, /*!< I2C MEM类设备内部偏移地址宽度为8位 */
    HAL_I2C_MEMADD_SIZE_16BIT = 0x10U /*!< I2C MEM类设备内部偏移地址宽度为16位 */
} HAL_I2C_MemAddSizeTypeDef;

/**
 * @brief  I2C初始化结构体
 */
typedef struct
{
    HAL_I2C_ModeTypeDef Mode;                  /*!< I2C传输模式，详见 @brf  HAL_I2CModeTypeDef */
    HAL_I2C_CLKSpeedTypeDef ClockSpeed;        /*!< I2C传输速度，详见 @brf  HAL_I2C_CLK_SpeedTypeDef */
    HAL_I2C_AddressModeTypeDef AddressingMode; /*!< I2C地址长度，详见 @brf HAL_I2C_AddressModeTypeDef */
    uint16_t OwnAddress;                       /*!< I2C为从机时，自身设备地址（填写该地址时请不要包含读写控制位）*/
} HAL_I2C_InitTypeDef;

/**
 * @brief  I2C外设.
 */
#define  HAL_I2C1 ((I2C_TypeDef *) I2C1_BASE) /*!< I2C1外设基地址 */
#define  HAL_I2C2 ((I2C_TypeDef *) I2C2_BASE) /*!< I2C2外设基地址 */

/**
 * @brief  I2C控制结构体
 */
typedef struct
{
    I2C_TypeDef *Instance; /*!< I2C寄存器基地址 */

    HAL_I2C_InitTypeDef Init; /*!< I2C初始化配置参数 */

    uint8_t *pBuffPtr;                   /*!< （发送或者接收）数据存储地址 */
    volatile uint16_t XferSize;          /*!< 用户指定传输（发送或者接收）数据长度 */
    volatile uint16_t XferCount;         /*!< I2C实际传输（发送或者接收）数据长度 */
    volatile HAL_I2C_DirTypeDef XferDir; /*!< I2C传输方向，用户无须配置和关心 */

    HAL_LockTypeDef Lock; /*!< I2C设备锁 */

    volatile HAL_I2C_StateTypeDef State; /*!< I2C工作状态 */
    volatile HAL_I2C_ErrorCodeTypeDef ErrorCode; /*!< I2C错误码 */
} HAL_I2C_HandleTypeDef;

/**
 * @brief  获取I2C工作状态.
 *
 * @param  hi2c. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
 * @return 返回I2C工作状态. 详见  @ref HAL_I2C_StateTypeDef.
 *         返回值可能是以下类型：
 *         @retval HAL_I2C_STATE_RESET : I2C未配置
 *         @retval HAL_I2C_STATE_READY : I2C已初始化完成
 *         @retval HAL_I2C_STATE_BUSY : I2C正被占用
 *         @retval HAL_I2C_STATE_TIMEOUT : I2C接收或发送超时
 *         @retval HAL_I2C_STATE_ERROR : I2C错误
 */
HAL_I2C_StateTypeDef HAL_I2C_GetState(HAL_I2C_HandleTypeDef *hi2c);

/**
 * @brief  获取I2C错误状态.
 *
 * @param  hi2c. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
 * @return 返回I2C错误码.详见  @ref HAL_I2C_ErrorCodeTypeDef.
 *         返回值可能是以下类型：
 *         @retval  HAL_I2C_ERROR_NONE ：I2C无错误
 *         @retval  HAL_I2C_ERROR_ARB_LOST ：I2C仲裁丢失错误
 *         @retval  HAL_I2C_ERROR_SCL_TIMEOUT : I2C超时错误
 *         @retval  HAL_I2C_ERROR_RX_FIFO_OVF : I2C接收溢出错误
 */
HAL_I2C_ErrorCodeTypeDef HAL_I2C_GetError(HAL_I2C_HandleTypeDef *hi2c);

/**
 * @brief  软件复位I2C.
 *
 * @param  hi2c 详情参考 @ref HAL_I2C_HandleTypeDef.
 */
void HAL_I2C_SoftReset(HAL_I2C_HandleTypeDef *hi2c);

/**
 * @brief  初始化I2C.耗时99us
 *
 * @param  hi2c. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示初始化外设成功
 *         @retval  HAL_ERROR    ：表示入参错误，即hi2c为NULL，或hi2c->State不为HAL_SPI_STATE_RESET
 */
HAL_StatusTypeDef HAL_I2C_Init(HAL_I2C_HandleTypeDef *hi2c);

/**
 * @brief  去初始化I2C.耗时41us
 *
 * @param  hi2c. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示去初始化外设成功
 *         @retval  HAL_ERROR    ：表示入参错误
 */
HAL_StatusTypeDef HAL_I2C_DeInit(HAL_I2C_HandleTypeDef *hi2c);

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
HAL_StatusTypeDef HAL_I2C_Master_Transmit(HAL_I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);

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
HAL_StatusTypeDef HAL_I2C_Master_Receive(HAL_I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);

/**
 * @brief  I2C从机阻塞发送API.
 *
 * @param  hi2c. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
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
HAL_StatusTypeDef HAL_I2C_Slave_Transmit(HAL_I2C_HandleTypeDef *hi2c, uint8_t *pData, uint16_t Size, uint32_t Timeout);

/**
 * @brief  I2C从机阻塞接收API.
 *
 * @param  hi2c. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
 * @param  pData：要接收的数据的存储地址
 * @param  Size：要接收的数据的长度
 * @param  Timeout：超时时间（单位ms）;
 *         如果Timeout为0，则是不阻塞，检测到接收FIFO空后，则程序立刻退出;
 *         如果Timeout为HAL_MAX_DELAY 是一直阻塞，直到接收到指定数量的字节后才退出;
 * 		   如果Timeout为非0非HAL_MAX_DELAY时，表示函数等待数据接收完成的最长时间，超时则退出函数.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：超时时间不为0时，表示指定时间内成功接收指定数量的数据；超时时间为0时，也会返回，但实际接收数据的数量通过hi2c结构体中的XferCount来确定
 *         @retval  HAL_ERROR    ：入参错误
 *         @retval  HAL_BUSY     ：外设正在使用中
 *         @retval  HAL_TIMEOUT  ：超时时间不为0时会返回，表示指定时间内未能成功接收指定数量的数据
 * @note   hi2c结构体中的XferCount表示实际接收的字节数.
 */
HAL_StatusTypeDef HAL_I2C_Slave_Receive(HAL_I2C_HandleTypeDef *hi2c, uint8_t *pData, uint16_t Size, uint32_t Timeout);

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
HAL_StatusTypeDef HAL_I2C_Mem_Write(HAL_I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, HAL_I2C_MemAddSizeTypeDef MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);

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
HAL_StatusTypeDef HAL_I2C_Mem_Read(HAL_I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, HAL_I2C_MemAddSizeTypeDef MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);

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
HAL_StatusTypeDef HAL_I2C_Master_Transmit_IT(HAL_I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size);

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
HAL_StatusTypeDef HAL_I2C_Master_Receive_IT(HAL_I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size);

/**
 * @brief  I2C从机非阻塞接收API.
 *
 * @param  hi2c. 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
 * @param  pData：要接收的数据的存储地址
 * @param  Size：要接收的数据的长度
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示中断接收需要的参数配置、外设配置成功，等待进入中断接收数据
 *         @retval  HAL_ERROR    ：表示入参错误
 *         @retval  HAL_BUSY     ：外设正在使用中
 */
HAL_StatusTypeDef HAL_I2C_Slave_Receive_IT(HAL_I2C_HandleTypeDef *hi2c, uint8_t *pData, uint16_t Size);

/**
 * @brief  I2C MEM类设备的非阻塞写数据API.
 *
 * @param  hi2c 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
 * @param  DevAddress 目标从机地址，7位地址时bit[7:0]有效，10位地址时bit[9:0]有效，必须将从机设备数据手册中的7位设备地址值左移一位再传入.
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
HAL_StatusTypeDef HAL_I2C_Mem_Write_IT(HAL_I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, HAL_I2C_MemAddSizeTypeDef MemAddSize, uint8_t *pData, uint16_t Size);

/**
 * @brief  I2C MEM类设备的非阻塞读数据API.
 *
 * @param  hi2c 详见结构体定义  @ref HAL_I2C_HandleTypeDef.
 * @param  DevAddress 目标从机地址，7位地址时bit[7:0]有效，10位地址时bit[9:0]有效，必须将从机设备数据手册中的7位设备地址值左移一位再传入.
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
HAL_StatusTypeDef HAL_I2C_Mem_Read_IT(HAL_I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, HAL_I2C_MemAddSizeTypeDef MemAddSize, uint8_t *pData, uint16_t Size);

/**
 * @brief  I2C1错误回调函数
 *
 * @param  hi2c. 详见结构体定义 HAL_I2C_HandleTypeDef.Def.
 */
void HAL_I2C1_ErrorCallback(HAL_I2C_HandleTypeDef *hi2c);

/**
 * @brief  I2C1发送完成回调函数
 *
 * @param  hi2c. 详见结构体定义 HAL_I2C_HandleTypeDef.
 */
void HAL_I2C1_TxCpltCallback(HAL_I2C_HandleTypeDef *hi2c);

/**
 * @brief  I2C1接收完成回调函数
 *
 * @param  hi2c. 详见结构体定义 HAL_I2C_HandleTypeDef.
 */
void HAL_I2C1_RxCpltCallback(HAL_I2C_HandleTypeDef *hi2c);

/**
 * @brief  I2C2错误回调函数
 *
 * @param  hi2c. 详见结构体定义 HAL_I2C_HandleTypeDef.Def.
 */
void HAL_I2C2_ErrorCallback(HAL_I2C_HandleTypeDef *hi2c);

/**
 * @brief  I2C2发送完成回调函数
 *
 * @param  hi2c. 详见结构体定义 HAL_I2C_HandleTypeDef.
 */
void HAL_I2C2_TxCpltCallback(HAL_I2C_HandleTypeDef *hi2c);

/**
 * @brief  I2C2接收完成回调函数
 *
 * @param  hi2c. 详见结构体定义 HAL_I2C_HandleTypeDef.
 */
void HAL_I2C2_RxCpltCallback(HAL_I2C_HandleTypeDef *hi2c);

