/**
 * ******************************************************************************
 * @file    hal_spi.h
 * @brief   此文件包含spi外设的变量，枚举，结构体定义，函数声明等.
 *
 * @attention 模组形态与OPENCPU形态时，SPI支持的最大速率不同
 *            1. 模组：14MHz
 *            2. OPENCPU: Boot_CP前：3MHz; Boot_CP后：14MHz
 *********************************************************************************
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
#include "xinyi2100.h"
#include "hal_def.h"
#include "spi.h"
#include "prcm.h"
#include "xy_timer.h"
#include "xy_system.h"

/**
 * @brief  SPI工作状态枚举
 */
typedef enum
{
    HAL_SPI_STATE_RESET = 0x00U,      /*!< SPI未配置 */
    HAL_SPI_STATE_READY = 0x01U,      /*!< SPI已初始化完成 */
    HAL_SPI_STATE_BUSY = 0x02U,       /*!< SPI正被占用 */
    HAL_SPI_STATE_ERROR = 0x04U,      /*!< SPI错误 */
    HAL_SPI_STATE_ABORT = 0x08U       /*!< SPI传输中止或取消 */
} HAL_SPI_StateTypeDef;

/**
 * @brief	SPI错误码类型枚举
 */
typedef enum
{
    HAL_SPI_ERROR_NONE = 0x00U,              /*!< SPI无错误 */
    HAL_SPI_ERROR_HARDWARE_FAULT = 0x01U,    /*!< SPI硬件故障错误码 */
    HAL_SPI_ERROR_RX_FIFO_OVF = 0x02U        /*!< SPI RXFIFO溢出错误码 */
} HAL_SPI_ErrorCodeTypeDef;

/**
 * @brief SPI主从模式枚举
 */
typedef enum
{
    HAL_SPI_MODE_MASTER = SPI_CONFIG_MODE_MASTER /*!< SPI主机模式 */
} HAL_SPI_MasterSlaveModeTypeDef;

/**
 * @brief SPI工作模式枚举
 * SPI 一共有四种通讯模式，主要取决于CPOL(Clock Polarity,时钟极性)和CPHA（Clock Phase，时钟相位）配置.
 * CPOL指的是设备空闲时（SPI通讯开始前、NSS信号为高电平时）的SCK的电平状态.
 * CPOL=0时，SCK在空闲状态时为低电平.CPOL=1 时，SCK在空闲状态时为高电平.
 * CPHA指的是MOSI或MISO数据线上的信号采样时刻.
 * CPHA=0时，数据线在SCK的“第一个跳变沿”被采样.CPHA=1时，数据线在SCK的“第二个跳变沿”被采样.
 * 工作模式0： CPOL = 0 CPHA = 0 
 * 工作模式1： CPOL = 0 CPHA = 1
 * 工作模式2： CPOL = 1 CPHA = 0
 * 工作模式3： CPOL = 1 CPHA = 1
 * 
 */
typedef enum
{
    HAL_SPI_WORKMODE_0 = SPI_FRF_MOTO_MODE_0, /*!< SPI工作模式0 */
    HAL_SPI_WORKMODE_1 = SPI_FRF_MOTO_MODE_1, /*!< SPI工作模式1 */
    HAL_SPI_WORKMODE_2 = SPI_FRF_MOTO_MODE_2, /*!< SPI工作模式2 */
    HAL_SPI_WORKMODE_3 = SPI_FRF_MOTO_MODE_3  /*!< SPI工作模式3 */
} HAL_SPI_WorkModeTypeDef;

/**
 * @brief	SPI时钟分频系数枚举，SPI速率是基于pclk2时钟频率进行分频得到
 * @note    1、当前表计opencpu版本，单AP核系统时钟为HRC时钟(实际频率约为6.5M)，起CP核后会切为PLL时钟(实际频率约为36.84M)
 *             系统主频会发生变化时，导致SPI所挂载的pclk2时钟也随之变化，同一分频系数单核与双核时速度不同，下述注释列举了不同分频下的理论速率；
 *          2、轻量级opencpu单AP核时钟即为PLL时钟（36.84M），起CP后不变，SPI速率与表计opencpu双核时一致；
 *          3、下述注释的速度皆为理论速率，不同硬件时钟略有偏差，另外SPI通信的实际速率还会受到主从设备的处理能力、通信距离的影响.
 */
typedef enum
{
    HAL_SPI_CLKDIV_2 = SPI_CONFIG_CLK_DIV_2,     /*!< 2分频，表计单AP核速率约6.5M，起CP核后理论上为16.4M，内部实测最高通信速率约14M*/
    HAL_SPI_CLKDIV_4 = SPI_CONFIG_CLK_DIV_4,     /*!< 4分频，表计单AP核速率约3.2M，起CP核后约8.2M */
    HAL_SPI_CLKDIV_8 = SPI_CONFIG_CLK_DIV_8,     /*!< 8分频，表计单AP核速率约1.6M，起CP核后约4.6M */ 
    HAL_SPI_CLKDIV_16 = SPI_CONFIG_CLK_DIV_16,   /*!< 16分频，表计单AP核速率约800K，起CP核后约2.3M */
    HAL_SPI_CLKDIV_32 = SPI_CONFIG_CLK_DIV_32,   /*!< 32分频，表计单AP核速率约400K，起CP核后约1.1M */
    HAL_SPI_CLKDIV_64 = SPI_CONFIG_CLK_DIV_64,   /*!< 64分频，表计单AP核速率约200K，起CP核后约550K */
    HAL_SPI_CLKDIV_128 = SPI_CONFIG_CLK_DIV_128, /*!< 128分频，表计单AP核速率约100K，起CP核后约275K */
    HAL_SPI_CLKDIV_256 = SPI_CONFIG_CLK_DIV_256  /*!< 256分频，表计单AP核速率约50K，起CP核后约130K */
} HAL_SPI_ClkDivTypeDef;

/**
 * @brief	SPI片选类型枚举
 */
typedef enum
{
    HAL_SPI_CS0 = SPI_CONFIG_SS_LINES_SS0,     /*!< SPI片选引脚为SS0 */
    HAL_SPI_CS1 = SPI_CONFIG_SS_LINES_SS1      /*!< SPI片选引脚为SS1 */
} HAL_SPI_ChipSelTypeDef;

/**
 * @brief  SPI初始化结构体
 */
typedef struct
{
    HAL_SPI_MasterSlaveModeTypeDef MasterSlave; /*!< SPI主从模式 */
    HAL_SPI_WorkModeTypeDef WorkMode;           /*!< SPI工作模式 */
    HAL_SPI_ClkDivTypeDef Clock_Prescaler;      /*!< SPI时钟分频系数，体现为SPI速率 */
} HAL_SPI_InitTypeDef;

/**
 * @brief  SPI控制结构体
 */
typedef struct
{
    SPI_TypeDef *Instance; /*!< SPI寄存器基地址 */

    HAL_SPI_InitTypeDef Init; /*!< SPI初始化配置参数 */

    uint8_t *pTxBuffPtr;           /*!< 发送数据存储地址 */
    volatile uint16_t TxXferSize;  /*!< 用户指定发送数据长度 */
    volatile uint16_t TxXferCount; /*!< SPI实际接收数据长度 */
    uint8_t *pRxBuffPtr;           /*!< 接收数据存储地址 */
    volatile uint16_t RxXferSize;  /*!< 用户指定接收数据长度 */
    volatile uint16_t RxXferCount; /*!< SPI实际接收数据长度 */

    HAL_LockTypeDef Lock; /*!< SPI设备锁 */

    volatile HAL_SPI_StateTypeDef State; /*!< SPI工作状态 */
    volatile HAL_SPI_ErrorCodeTypeDef ErrorCode; /*!< SPI错误码 */
} HAL_SPI_HandleTypeDef;

/**
 * @brief  SPI 外设.
 */
#define HAL_SPI ((SPI_TypeDef *)SPI_BASE) /*!< SPI 外设基地址   */

/**
 * @brief SPI主机片选引脚拉高。耗时11us
 *
 * @param slave_select_pin 主机片选引脚选择，详见HAL_SPI_ChipSelTypeDef.
 */
void HAL_SPI_SetCS(HAL_SPI_ChipSelTypeDef slave_select_pin);

/**
 * @brief SPI主机片选引脚拉低。耗时11us
 *
 * @param slave_select_pin 主机片选引脚选择，详见HAL_SPI_ChipSelTypeDef.
 */
void HAL_SPI_ResetCS(HAL_SPI_ChipSelTypeDef slave_select_pin);

/**
 * @brief   获取 SPI 工作状态.
 *
 * @param   hspi. 详见结构体定义 HAL_SPI_HandleTypeDef.
 * @return  返回工作状态，详见 HAL_SPI_StateTypeDef.
 *          返回值可能是以下类型：
 *          @retval HAL_SPI_STATE_RESET : SPI未配置
 *          @retval HAL_SPI_STATE_READY : SPI已初始化完成
 *          @retval HAL_SPI_STATE_BUSY  : SPI正被占用
 *          @retval HAL_SPI_STATE_ERROR : SPI错误
 *          @retval HAL_SPI_STATE_ABORT : SPI传输中止或取消
 */
HAL_SPI_StateTypeDef HAL_SPI_GetState(HAL_SPI_HandleTypeDef *hspi);

/**
 * @brief  获取 SPI 错误状态.
 *
 * @param  hspi. 详见结构体定义 HAL_SPI_HandleTypeDef.
 * @return 返回SPI错误码，详见 HAL_SPI_ErrorCodeTypeDef.
 *         返回值可能是以下类型：
 *         @retval HAL_SPI_ERROR_NONE           ：SPI无错误
 *         @retval HAL_SPI_ERROR_HARDWARE_FAULT : SPI硬件故障错误码
 *         @retval HAL_SPI_ERROR_RX_FIFO_OVF    : SPI RXFIFO溢出错误码
 */
HAL_SPI_ErrorCodeTypeDef HAL_SPI_GetError(HAL_SPI_HandleTypeDef *hspi);

/**
 * @brief  初始化SPI.
 *
 * @param  hspi. 详见结构体定义   @ref HAL_SPI_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示初始化外设成功
 *         @retval  HAL_ERROR    ：表示入参错误，即hspi为NULL，或hspi->State不为HAL_SPI_STATE_RESET
 */
HAL_StatusTypeDef HAL_SPI_Init(HAL_SPI_HandleTypeDef *hspi);

/**
 * @brief  去初始化SPI.耗时26us
 *
 * @param  hspi. 详见结构体定义  @ref HAL_SPI_HandleTypeDef。
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval HAL_OK      ：表示外设去初始化成功
 *         @retval HAL_ERROR   ：表示入参错误
 */
HAL_StatusTypeDef HAL_SPI_DeInit(HAL_SPI_HandleTypeDef *hspi);

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
HAL_StatusTypeDef HAL_SPI_Transmit(HAL_SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout);

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
HAL_StatusTypeDef HAL_SPI_Receive(HAL_SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout);

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
HAL_StatusTypeDef HAL_SPI_Master_TransmitReceive(HAL_SPI_HandleTypeDef *hspi, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout);

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
HAL_StatusTypeDef HAL_SPI_Transmit_IT(HAL_SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size);

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
HAL_StatusTypeDef HAL_SPI_Receive_IT(HAL_SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size);

/**
 * @brief  SPI错误中断回调函数
 *
 * @param  hspi. 详见结构体定义  @ref HAL_SPI_HandleTypeDef.
 */
void HAL_SPI_ErrorCallback(HAL_SPI_HandleTypeDef *hspi);

/**
 * @brief  SPI发送完成回调函数
 *
 * @param  hspi. 详见结构体定义  @ref HAL_SPI_HandleTypeDef.
 */
void HAL_SPI_TxCpltCallback(HAL_SPI_HandleTypeDef *hspi);

/**
 * @brief  SPI接收完成回调函数
 *
 * @param  hspi. 详见结构体定义  @ref HAL_SPI_HandleTypeDef.
 */
void HAL_SPI_RxCpltCallback(HAL_SPI_HandleTypeDef *hspi);
