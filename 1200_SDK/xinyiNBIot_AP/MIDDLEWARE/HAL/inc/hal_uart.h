/**
 ******************************************************************************
 * @file	hal_uart.h
 * @brief   此文件包含UART外设的变量，枚举，结构体定义，函数声明等.
 * @note    none
 ******************************************************************************
 */

#pragma once

#include "hal_def.h"
#include "uart.h"
#include "nvic.h"
#include "prcm.h"
#include "xy_timer.h"
#include "xy_system.h"
#include "sys_clk.h"

/**
 * @brief  UART工作状态枚举
 */
typedef enum
{
    UART_STATE_RESET = 0x00U,  //UART未配
    UART_STATE_READY = 0x01U,  //UART已初始化完成
    UART_STATE_BUSY = 0x02U,   //UART正被占用
    UART_STATE_ERROR = 0x04U   //UART错误
} HAL_UART_StateTypeDef;

/**
 * @brief  UART错误码类型枚举
 */
typedef enum
{
    UART_ERROR_NONE = 0x00U,        //无错误
    UART_ERROR_FRAME_ERR = 0x01U,   //帧错误
    UART_ERROR_PARITY_ERR = 0x02U,  //校验错误
    UART_ERROR_RX_FIFO_OVF = 0x04U  //接收溢出错误
} HAL_UART_ErrorCodeTypeDef;

/**
 * @brief  UART波特率枚举
 * @note   若lsio_clk是HRC26M，uart参考时钟为26M，此时uart最低支持4800pbs。
 *         目前HRC26M时钟源自身会2分频或4分频，即HRC26M实际频率为13M或6.5M，
 *         在这个参考时钟频率下uart可以支持到2400pbs。
 */
typedef enum
{
    UART_BAUD_2400 = 2400,     //UART波特率2400
    UART_BAUD_4800 = 4800,     //UART波特率4800
    UART_BAUD_9600 = 9600,     //UART波特率9600
    UART_BAUD_14400 = 14400,   //UART波特率14400
    UART_BAUD_19200 = 19200,   //UART波特率19200
    UART_BAUD_38400 = 38400,   //UART波特率38400
    UART_BAUD_57600 = 57600,   //UART波特率57600
    UART_BAUD_115200 = 115200, //UART波特率115200
    UART_BAUD_230400 = 230400, //UART波特率230400
    UART_BAUD_380400 = 380400, //UART波特率380400
    UART_BAUD_460800 = 460800, //UART波特率460800
    UART_BAUD_921600 = 921600  //UART波特率921600
} HAL_UART_BaudrateTypeDef;

/**
 * @brief  UART数据位长度枚举
 */
typedef enum
{
    UART_WORDLENGTH_8 = UART_CTL_CHAR_LEN_8, //UART数据位长度为8位
    UART_WORDLENGTH_7 = UART_CTL_CHAR_LEN_7, //UART数据位长度为7位
    UART_WORDLENGTH_6 = UART_CTL_CHAR_LEN_6  //UART数据位长度为6位
} HAL_UART_WordLengthTypeDef;

/**
 * @brief  UART校验模式枚举
 */
typedef enum
{
    UART_PARITY_NONE = UART_CTL_PARITY_NONE, //UART无校验
    UART_PARITY_EVEN = UART_CTL_PARITY_EVEN, //UART偶校验
    UART_PARITY_ODD = UART_CTL_PARITY_ODD    //UART奇校验
} HAL_UART_ParityModeTypeDef;

/**
 * @brief  UART硬流控有效电平选择枚举
 */
typedef enum
{
    UART_HFC_LEVEL_NONE = 0, //硬件流控功能关闭
    UART_HFC_LEVEL_HIGH = 1, //RTS/CTS有效电平为高电平
    UART_HFC_LEVEL_LOW = 2   //RTS/CTS有效电平为低电平
} HAL_UART_HFCLevelTypeDef;

/**
 * @brief UART初始化结构体
 */
typedef struct
{
    HAL_UART_BaudrateTypeDef BaudRate;     //波特率值，单位Hz
    HAL_UART_WordLengthTypeDef WordLength; //数据位长度
    HAL_UART_ParityModeTypeDef Parity;     //校验模式
    HAL_UART_HFCLevelTypeDef HfcLevel;     //硬件流控有效电平
} HAL_UART_InitTypeDef;

/**
 * @brief  UART控制结构体
 */
typedef struct
{
    UART_TypeDef *Instance;        //UART寄存器基地址

    HAL_UART_InitTypeDef Init;     //UART初始化结构体

    uint8_t *pTxBuffPtr;           //发送数据存储地址
    volatile uint32_t TxXferSize;  //用户指定发送数据长度
    volatile uint32_t TxXferCount; //UART实际发送数据长度
    uint8_t *pRxBuffPtr;           //接收数据存储地址
    volatile uint32_t RxXferSize;  //用户指定接收数据长度
    volatile uint32_t RxXferCount; //UART实际接收数据长度

    HAL_LockTypeDef Lock;          //UART TX设备锁
    HAL_LockTypeDef RxLock;        //UART RX设备锁

    volatile HAL_UART_StateTypeDef gState;        //UART TX及全局工作状态
    volatile HAL_UART_StateTypeDef RxState;       //UART RX工作状态
    volatile HAL_UART_ErrorCodeTypeDef ErrorCode; //UART错误码
} HAL_UART_HandleTypeDef;

/**
 * @brief  初始化UART.
 * @param  huart 详情参考 @ref HAL_UART_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 */
HAL_StatusTypeDef HAL_UART_Init(HAL_UART_HandleTypeDef *huart);

/**
 * @brief  去初始化UART.
 * @param  huart 详情参考 @ref HAL_UART_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 */
HAL_StatusTypeDef HAL_UART_DeInit(HAL_UART_HandleTypeDef *huart);

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
HAL_StatusTypeDef HAL_UART_Transmit(HAL_UART_HandleTypeDef *huart, uint8_t *pData, uint32_t Size, uint32_t Timeout);

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
HAL_StatusTypeDef HAL_UART_Receive(HAL_UART_HandleTypeDef *huart, uint8_t *pData, uint32_t Size, uint32_t Timeout);

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
HAL_StatusTypeDef HAL_UART_Transmit_IT(HAL_UART_HandleTypeDef *huart, uint8_t *pData, uint32_t Size);

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
HAL_StatusTypeDef HAL_UART_Receive_IT(HAL_UART_HandleTypeDef *huart, uint8_t *pData, uint32_t Size, uint32_t HardTimeout);

/**
 * @brief  UART错误回调函数
 * @param  huart. 详见结构体定义 HAL_UART_HandleTypeDef.
 */
void HAL_UART_ErrorCallback(HAL_UART_HandleTypeDef *huart);

/**
 * @brief  UART发送完成回调函数
 * @param  huart. 详见结构体定义 HAL_UART_HandleTypeDef.
 */
void HAL_UART_TxCpltCallback(HAL_UART_HandleTypeDef *huart);

/**
 * @brief  UART接收完成回调函数
 * @param  huart. 详见结构体定义 HAL_UART_HandleTypeDef.
 * @note   该回调函数会在指定长度(RxXferSize)数据接收完成后被调用，或者LPUART模块接收超时后被调用.
 */
void HAL_UART_RxCpltCallback(HAL_UART_HandleTypeDef *huart);
