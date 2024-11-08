/**
 **************************************************************************************************************
 * @file   hal_csp.h
 * @brief  此文件包含CSP外设的变量，枚举，结构体定义，函数声明等.
 * @note   XY1200共有3个CSP外设，使用情况如下.
 *         1. CSP1使用情况：CSP1可供用户自由使用，不会受到切换系统时钟影响;
 *         2. CSP2使用情况：CSP2可供用户自由使用，会受到切换系统时钟影响;
 *         3. CSP3使用情况，CSP3为AP/CP的log通道，用户不可自由使用，如需使用，需要关闭log功能，
 *            具体请参考《芯翼XY1200&XY2100产品外设使用说明》.
 *         4. 模组形态与OPENCPU形态时，CSP支持的最大速率不同,详情参考《芯翼XY1200S及XY2100S产品OPENCPU软件开发指导》
 *************************************************************************************************************
 * @attention
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
 *************************************************************************************************************
 */

#pragma once
#include "hal_def.h"
#include "xinyi2100.h"
#include "csp.h"
#include "prcm.h"
#include "xy_timer.h"
#include "xy_system.h"

/**
 * @brief CSP工作状态枚举
 */
typedef enum
{
    HAL_CSP_STATE_RESET = 0x00U,      /*!< CSP未配置 */
    HAL_CSP_STATE_READY = 0x01U,      /*!< CSP已初始化完成 */
    HAL_CSP_STATE_BUSY = 0x02U,       /*!< CSP正被占用 */
    HAL_CSP_STATE_TIMEOUT = 0x04U,    /*!< CSP超时 */
    HAL_CSP_STATE_ERROR = 0x08U       /*!< CSP错误 */
} HAL_CSP_StateTypeDef;

/**
 * @brief	CSP错误类型枚举.
 */
typedef enum
{
    HAL_CSP_ERROR_NONE = 0x00U,         /*!< 无错误 */
    HAL_CSP_ERROR_UART_FRM_ERR = 0x01U, /*!< 帧错误 */
    HAL_CSP_ERROR_RX_FIFO_OVF = 0x02U,  /*!< 接收溢出错误 */
    HAL_CSP_ERROR_UART_TIMEOUT = 0x04U  /*!< 接收超时错误 */
} HAL_CSP_ErrorCodeTypeDef;

/**
 * @brief  CSP_UART波特率枚举
 */
typedef enum
{
    HAL_CSP_UART_BAUD_2400 = 2400,     /*!< CSP_UART波特率2400 */
    HAL_CSP_UART_BAUD_4800 = 4800,     /*!< CSP_UART波特率4800 */
    HAL_CSP_UART_BAUD_9600 = 9600,     /*!< CSP_UART波特率9600 */
    HAL_CSP_UART_BAUD_19200 = 19200,   /*!< CSP_UART波特率19200 */
    HAL_CSP_UART_BAUD_38400 = 38400,   /*!< CSP_UART波特率38400 */
    HAL_CSP_UART_BAUD_57600 = 57600,   /*!< CSP_UART波特率57600 */
    HAL_CSP_UART_BAUD_115200 = 115200, /*!< CSP_UART波特率115200 */
    HAL_CSP_UART_BAUD_380400 = 380400, /*!< CSP_UART波特率380400 */
    HAL_CSP_UART_BAUD_460800 = 460800, /*!< CSP_UART波特率460800 */
    HAL_CSP_UART_BAUD_921600 = 921600  /*!< CSP_UART波特率921600 */
} HAL_CSP_UART_BaudrateTypeDef;

/**
 * @brief  CSP_UART数据位枚举.
 */
typedef enum
{
    HAL_CSP_UART_WORDLENGTH_6 = 6, /*!< CSP_UART 6个数据位 */
    HAL_CSP_UART_WORDLENGTH_7 = 7, /*!< CSP_UART 7个数据位 */
    HAL_CSP_UART_WORDLENGTH_8 = 8  /*!< CSP_UART 8个数据位 */
} HAL_CSP_UART_WordLengthTypeDef;

/**
 * @brief  CSP_UART校验位枚举
 */
typedef enum
{
    HAL_CSP_UART_PARITY_NONE = CSP_UART_PARITYCHECK_None, /*!< CSP_UART无校验 */
    HAL_CSP_UART_PARITY_EVEN = CSP_UART_PARITYCHECK_Even, /*!< CSP_UART偶校验 */
    HAL_CSP_UART_PARITY_ODD = CSP_UART_PARITYCHECK_Odd    /*!< CSP_UART奇校验 */
} HAL_CSP_UART_ParityModeTypeDef;

/**
 * @brief  CSP_UART停止位枚举
 */
typedef enum
{
    HAL_CSP_UART_STOPBITS_1 = 1, /*!< CSP_UART 1个停止位 */
    HAL_CSP_UART_STOPBITS_2 = 2  /*!< CSP_UART 2个停止位 */
} HAL_CSP_UART_StopBitsTypeDef;

/**
 * @brief	CSP_UART初始化结构体
 */
typedef struct
{
    HAL_CSP_UART_BaudrateTypeDef BaudRate;     /*!< CSP_UART波特率 */
    HAL_CSP_UART_WordLengthTypeDef WordLength; /*!< CSP_UART数据位 */
    HAL_CSP_UART_ParityModeTypeDef Parity;     /*!< CSP_UART校验位 */
    HAL_CSP_UART_StopBitsTypeDef StopBits;     /*!< CSP_UART停止位 */
} HAL_CSP_UART_InitTypeDef;

/**
 * @brief  CSP_SPI主从模式枚举
 */
typedef enum
{
    HAL_CSP_SPI_MODE_MASTER = CSP_MODE1_CLOCK_MODE_Master, /*!< CSP_SPI主机模式 */
} HAL_CSP_SPI_MasterSlaveTypeDef;

/**
 * @brief  CSP_SPI工作模式枚举
 * SPI 一共有四种通讯模式，主要取决于CPOL(Clock Polarity,时钟极性)和CPHA（Clock Phase，时钟相位）配置.
 * CPOL指的是设备空闲时（SPI通讯开始前、NSS信号为高电平时）的SCK的电平状态.
 * CPOL=0时，SCK在空闲状态时为低电平.CPOL=1 时，SCK在空闲状态时为高电平.
 * CPHA指的是MOSI或MISO数据线上的信号采样时刻.
 * CPHA=0时，数据线在SCK的“第一个跳变沿”被采样.CPHA=1时，数据线在SCK的“第二个跳变沿”被采样.
 * 工作模式0： CPOL = 0 CPHA = 0 
 * 工作模式1： CPOL = 0 CPHA = 1
 * 工作模式2： CPOL = 1 CPHA = 0
 * 工作模式3： CPOL = 1 CPHA = 1
 */
typedef enum
{
    HAL_CSP_SPI_WORKMODE_0 = 0, /*!< CSP_SPI工作模式0 */
    HAL_CSP_SPI_WORKMODE_1 = 1, /*!< CSP_SPI工作模式1 */
    HAL_CSP_SPI_WORKMODE_2 = 2, /*!< CSP_SPI工作模式2 */
    HAL_CSP_SPI_WORKMODE_3 = 3  /*!< CSP_SPI工作模式3 */
} HAL_CSP_SPI_WorkModeTypeDef;

/**
 * @brief CSP_SPI的速率选择枚举
 *
 * @note  注意CSP2、CSP3时钟源来自pclk2，CSP1时钟源来自lsio，
 *        CSP_SPI Master支持最大速率为1/6的外设时钟频率，Slave支持最大速率为1/16的外设时钟频率.
 */
typedef enum
{
    HAL_CSP_SPI_SPEED_100K = 100000, /*!< CSP_SPI速率100k */
    HAL_CSP_SPI_SPEED_500K = 500000, /*!< CSP_SPI速率500k */
    HAL_CSP_SPI_SPEED_1M = 1000000,  /*!< CSP_SPI速率1M */
    HAL_CSP_SPI_SPEED_2M = 2000000,  /*!< CSP_SPI速率2M */
    HAL_CSP_SPI_SPEED_3M = 3000000,  /*!< CSP_SPI速率3M */
    HAL_CSP_SPI_SPEED_4M = 4000000,  /*!< CSP_SPI速率4M */
    HAL_CSP_SPI_SPEED_6M = 6000000   /*!< CSP_SPI速率6M */
} HAL_CSP_SPI_SpeedTypeDef;

/**
 * @brief	CSP_SPI初始化结构体
 */
typedef struct
{
  HAL_CSP_SPI_MasterSlaveTypeDef MasterSlave; /*!< CSP_SPI主从模式 */
  HAL_CSP_SPI_WorkModeTypeDef WorkMode;       /*!< CSP_SPI工作模式 */
  HAL_CSP_SPI_SpeedTypeDef Speed;             /*!< CSP_SPI速率 */
} HAL_CSP_SPI_InitTypeDef;

/**
 * @brief  CSP 控制结构体定义.
 */
typedef struct
{
  CSP_TypeDef *Instance;                  /*!< CSP寄存器基地址 */

  HAL_CSP_UART_InitTypeDef CSP_UART_Init; /*!< CSP_UART初始化结构体 */

  HAL_CSP_SPI_InitTypeDef CSP_SPI_Init;   /*!< CSP_SPI初始化结构体 */

  uint8_t *pTxBuffPtr;                    /*!< 发送数据存储地址 */
  volatile uint16_t TxXferSize;           /*!< 用户指定发送数据长度 */
  volatile uint16_t TxXferCount;          /*!< CSP实际发送数据长度 */
  uint8_t *pRxBuffPtr;                    /*!< 接收数据存储地址 */
  volatile uint16_t RxXferSize;           /*!< 用户指定接收数据长度 */
  volatile uint16_t RxXferCount;          /*!< CSP实际接收数据长度 */
        
  HAL_LockTypeDef Lock;                   /*!< CSP设备锁 */
  HAL_LockTypeDef RxLock;                 /*!< CSP设备锁 */

  volatile HAL_CSP_StateTypeDef gState;        /*!< CSP TX及全局工作状态 */
  volatile HAL_CSP_StateTypeDef RxState;       /*!< CSP RX工作状态 */
  volatile HAL_CSP_ErrorCodeTypeDef ErrorCode; /*!< CSP错误码 */
} HAL_CSP_HandleTypeDef;

/**
 * @brief  CSP外设.
 */
#define HAL_CSP1 ((CSP_TypeDef *) CSP1_BASE) /*!< CSP1 基地址 */
#define HAL_CSP2 ((CSP_TypeDef *) CSP2_BASE) /*!< CSP2 基地址 */
#define HAL_CSP3 ((CSP_TypeDef *) CSP3_BASE) /*!< CSP3 基地址 */

/**
 * @brief CSP_SPI主机片选引脚拉高。耗时11us
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
void HAL_CSP_SPI_SetCS(HAL_CSP_HandleTypeDef *hcsp);

/**
 * @brief CSP_SPI主机片选引脚拉低。耗时11us
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
void HAL_CSP_SPI_ResetCS(HAL_CSP_HandleTypeDef *hcsp);

/**
 * @brief  获取 CSP 工作状态.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @return 返回工作状态，详情参考 @ref HAL_CSP_StateTypeDef
 *         返回值可能是以下类型：
 *         @retval HAL_CSP_STATE_RESET   : CSP未配置
 *         @retval HAL_CSP_STATE_READY   : CSP已初始化完成
 *         @retval HAL_CSP_STATE_BUSY    : CSP正被占用
 *         @retval HAL_CSP_STATE_TIMEOUT : CSP超时
 *         @retval HAL_CSP_STATE_ERROR   : CSP错误
 */
HAL_CSP_StateTypeDef HAL_CSP_GetState(HAL_CSP_HandleTypeDef *hcsp);

/**
 * @brief  获取 CSP 错误状态.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @return 返回CSP错误码 详情参考 @ref HAL_CSP_ErrorCodeTypeDef.
 *         返回值可能是以下类型：
 *         @retval HAL_CSP_ERROR_NONE : 无错误
 *         @retval HAL_CSP_ERROR_UART_FRM_ERR : 帧错误
 *         @retval HAL_CSP_ERROR_RX_FIFO_OVF : 接收溢出错误
 *         @retval HAL_CSP_ERROR_UART_TIMEOUT : 接收超时错误
 */
HAL_CSP_ErrorCodeTypeDef HAL_CSP_GetError(HAL_CSP_HandleTypeDef *hcsp);

/**
 * @brief  初始化CSP为UART.498us
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示初始化外设成功
 *         @retval  HAL_ERROR    ：表示入参错误，如下：
 *         即hcsp为NULL，或hcsp->State不为HAL_CSP_STATE_RESET，或hcsp->CSP_Periph不为HAL_CSP_PERIPH_UART
 */
HAL_StatusTypeDef HAL_CSP_UART_Init(HAL_CSP_HandleTypeDef *hcsp);

/**
 * @brief  初始化CSP为SPI.耗时156us
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示初始化外设成功
 *         @retval  HAL_ERROR    ：表示入参错误，如下：
 *         即hcsp为NULL，或hcsp->State不为HAL_CSP_STATE_RESET，或hcsp->CSP_SPI_Init.Speed不正确，
 *         或hcsp->CSP_SPI_Init.WorkMode不正确，或hcsp->CSP_Periph不为HAL_CSP_PERIPH_SPI
 */
HAL_StatusTypeDef HAL_CSP_SPI_Init(HAL_CSP_HandleTypeDef *hcsp);

/**
 * @brief  去初始化CSP.
 *
 * @param  hcsp. 耗时37us。
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示去初始化外设成功
 *         @retval  HAL_ERROR    ：表示入参错误
 */
HAL_StatusTypeDef HAL_CSP_DeInit(HAL_CSP_HandleTypeDef *hcsp);

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
HAL_StatusTypeDef HAL_CSP_Transmit(HAL_CSP_HandleTypeDef *hcsp, uint8_t *pData, uint16_t Size, uint32_t Timeout);

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
HAL_StatusTypeDef HAL_CSP_Receive(HAL_CSP_HandleTypeDef *hcsp, uint8_t *pData, uint16_t Size, uint32_t Timeout);

/**
 * @brief  CSP_SPI主机阻塞发送接收API. 注意！！！该接口只能用于CSP_SPI主机.
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
 */
HAL_StatusTypeDef HAL_CSP_SPI_TransmitReceive(HAL_CSP_HandleTypeDef *hcsp, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout);

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
HAL_StatusTypeDef HAL_CSP_Transmit_IT(HAL_CSP_HandleTypeDef *hcsp, uint8_t *pData, uint16_t Size);

/**
 * @brief  CSP_UART非阻塞接收API.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 * @param  pData 接收数据缓冲区指针.
 * @param  Size  接收数据字节长度.
 * @param  HardTimeout 超时时间（单位ms），XY2100 CSP提供硬件超时功能，在CSP接收过程中，如果超过HardTimeout时间后没有新的数据到来，则触发超时中断.
 *         如果HardTimeout为HAL_MAX_DELAY，则表示不使用CSP模块自身的硬件超时功能，只有在用户指定长度的数据全部收完时，才会触发接收完成回调函数的调用;
 *         如果Timeout为非0非HAL_MAX_DELAY，则表示设定并使用CSP模块自身的硬件超时功能，当用户指定长度的数据没有全部收完时，若出现一段数据接收间隔大于设定的超时时间后就会触发接收完成回调函数的调用。
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：中断接收函数执行成功，等待在中断中接收指定数量的的数据
 *         @retval  HAL_ERROR    ：入参错误
 *         @retval  HAL_BUSY     ：外设正在使用中
 */
HAL_StatusTypeDef HAL_CSP_Receive_IT(HAL_CSP_HandleTypeDef *hcsp, uint8_t *pData, uint16_t Size, uint16_t HardTimeout);

/**
 * @brief  CSP1错误中断回调函数.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
void HAL_CSP1_ErrorCallback(HAL_CSP_HandleTypeDef *hcsp);

/**
 * @brief  CSP1接收完成回调函数.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
void HAL_CSP1_RxCpltCallback(HAL_CSP_HandleTypeDef *hcsp);

/**
 * @brief  CSP1发送完成回调函数.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
void HAL_CSP1_TxCpltCallback(HAL_CSP_HandleTypeDef *hcsp);

/**
 * @brief  CSP2错误中断回调函数.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
void HAL_CSP2_ErrorCallback(HAL_CSP_HandleTypeDef *hcsp);

/**
 * @brief  CSP2接收完成回调函数.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
void HAL_CSP2_RxCpltCallback(HAL_CSP_HandleTypeDef *hcsp);

/**
 * @brief  CSP2发送完成回调函数.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
void HAL_CSP2_TxCpltCallback(HAL_CSP_HandleTypeDef *hcsp);

/**
 * @brief  CSP3错误中断回调函数.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
void HAL_CSP3_ErrorCallback(HAL_CSP_HandleTypeDef *hcsp);

/**
 * @brief  CSP3接收完成回调函数.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
void HAL_CSP3_RxCpltCallback(HAL_CSP_HandleTypeDef *hcsp);

/**
 * @brief  CSP3发送完成回调函数.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
void HAL_CSP3_TxCpltCallback(HAL_CSP_HandleTypeDef *hcsp);
