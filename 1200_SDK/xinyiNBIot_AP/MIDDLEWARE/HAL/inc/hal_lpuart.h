/**
 ******************************************************************************
 * @file    hal_lpuart.h
 * @brief   此文件包含uart外设的变量，枚举，结构体定义，函数声明等.
 * @note	XY1200共有1个LPUART外设，使用情况如下.
 *          1. 若AT_UART宏为1或为2，则LPUART默认为AT通道.
 *          2. 若AT_UART宏为0，则表示无AT通道，此时LPUART可供用户自由使用.
 * 
 * @attention 模组形态与OPENCPU形态时，lpuart支持的最大速率不同
 *            1. 模组：921600
 *            2. OPENCPU: 460800
 ******************************************************************************
 */

#pragma once
#include "hal_def.h"
#include "uart.h"
#include "hw_gpio.h"
#include "prcm.h"
#include "xy_timer.h"
#include "xy_system.h"

/**
 * @brief  LPUART工作状态枚举
 */
typedef enum
{
    LPUART_STATE_RESET = 0x00U, //LPUART未配置
    LPUART_STATE_READY = 0x01U, //LPUART已初始化完成
    LPUART_STATE_BUSY = 0x02U,  //LPUART正被占用
    LPUART_STATE_ERROR = 0x04U  //LPUART错误
} HAL_LPUART_StateTypeDef;

/**
 * @brief  LPUART错误码类型枚举
 */
typedef enum
{
    LPUART_ERROR_NONE = 0x00U,         //无错误
    LPUART_ERROR_FRAME_ERR = 0x01U,    //帧错误
    LPUART_ERROR_PARITY_ERR = 0x02U,   //校验错误
    LPUART_ERROR_RX_FIFO_OVF = 0x04U   //接收溢出错误
} HAL_LPUART_ErrorCodeTypeDef;

/**
 * @brief  LPUART波特率枚举
 * @note   若lpuart对参考时钟不分频，参考时钟为26M，此时lpuart最低支持4800pbs。
 *         目前HRC26M时钟源自身会2分频或4分频，即HRC26M实际频率为13M或6.5M，且lpuart也会对HRC26M再次分频，
 *         分别是6.5M、3.25M，在这个参考时钟频率下lpuart可以支持到1200pbs。
 */
typedef enum
{
    LPUART_BAUD_1200 = 1200,     //LPUART波特率1200
    LPUART_BAUD_2400 = 2400,     //LPUART波特率2400
    LPUART_BAUD_4800 = 4800,     //LPUART波特率4800
    LPUART_BAUD_9600 = 9600,     //LPUART波特率9600
    LPUART_BAUD_14400 = 14400,   //LPUART波特率14400
    LPUART_BAUD_19200 = 19200,   //LPUART波特率19200
    LPUART_BAUD_38400 = 38400,   //LPUART波特率38400
    LPUART_BAUD_57600 = 57600,   //LPUART波特率57600
    LPUART_BAUD_115200 = 115200, //LPUART波特率115200
    LPUART_BAUD_230400 = 230400, //LPUART波特率230400
    LPUART_BAUD_380400 = 380400, //LPUART波特率380400
    LPUART_BAUD_460800 = 460800, //LPUART波特率460800
    LPUART_BAUD_921600 = 921600  //LPUART波特率921600
} HAL_LPUART_BaudrateTypeDef;

/**
 * @brief  LPUART数据位长度枚举
 */
typedef enum
{
    LPUART_WORDLENGTH_8 = UART_CTL_CHAR_LEN_8, //LPUART数据位长度为8位
    LPUART_WORDLENGTH_7 = UART_CTL_CHAR_LEN_7, //LPUART数据位长度为7位
    LPUART_WORDLENGTH_6 = UART_CTL_CHAR_LEN_6  //LPUART数据位长度为6位
} HAL_LPUART_WordLengthTypeDef;

/**
 * @brief  LPUART校验模式枚举
 */
typedef enum
{
    LPUART_PARITY_NONE = UART_CTL_PARITY_NONE, //LPUART无校验
    LPUART_PARITY_EVEN = UART_CTL_PARITY_EVEN, //LPUART偶检验
    LPUART_PARITY_ODD = UART_CTL_PARITY_ODD    //LPUART奇检验
} HAL_LPUART_ParityModeTypeDef;

/**
 * @brief  LPUART硬流控有效电平选择枚举
 */
typedef enum
{
    LPUART_HFC_LEVEL_NONE = 0, //硬件流控功能关闭
    LPUART_HFC_LEVEL_HIGH = 1, //RTS、CTS有效电平为高电平
    LPUART_HFC_LEVEL_LOW = 2   //RTS、CTS有效电平为低电平
} HAL_LPUART_HFCLevelTypeDef;

/**
 * @brief  LPUART引脚选择枚举
 */
typedef enum
{
    LPUART_PADSEL_DISABLE = AON_LPUART_PAD_DISABLE,         //GPIO关闭，LPUART无法使用，此时GPIO3/4可供自由使用
    LPUART_PADSEL_RXD_WKUPRST = AON_LPUART_PAD_RXD_WKUPRST, //GPIO3/WKUPRST分别为LPUART的TXD/RXD
    LPUART_PADSEL_RXD_GPIO4 = AON_LPUART_PAD_RXD_GPIO4,     //GPIO3/4分别为LPUART的TXD/RXD
    LPUART_PADSEL_HARDFLOWCTL = AON_LPUART_PAD_HARDFLOWCTL  //GPIO3/4/5/6分别为LPUART的TXD/RXD/CTS/RTS
} HAL_LPUART_PadSelTypeDef;

/**
 * @brief LPUART初始化结构体
 */
typedef struct
{
    HAL_LPUART_BaudrateTypeDef BaudRate;     //波特率值，单位Hz
    HAL_LPUART_WordLengthTypeDef WordLength; //数据位长度
    HAL_LPUART_ParityModeTypeDef Parity;     //校验模式
    HAL_LPUART_HFCLevelTypeDef HfcLevel;     //硬件流控有效电平
    HAL_LPUART_PadSelTypeDef PadSel;         //引脚选择
} HAL_LPUART_InitTypeDef;

/**
 * @brief  LPUART控制结构体
 */
typedef struct
{
    UART_TypeDef *Instance;        //LPUART寄存器基地址

    HAL_LPUART_InitTypeDef Init;   //LPUART初始化结构体

    uint8_t *pTxBuffPtr;           //发送数据存储地址
    volatile uint32_t TxXferSize;  //用户指定发送数据长度
    volatile uint32_t TxXferCount; //LPUART实际发送数据长度
    uint8_t *pRxBuffPtr;           //接收数据存储地址
    volatile uint32_t RxXferSize;  //用户指定接收数据长度
    volatile uint32_t RxXferCount; //LPUART实际接收数据长度

    HAL_LockTypeDef Lock;          //LPUART TX设备锁
    HAL_LockTypeDef RxLock;        //LPUART RX设备锁

    volatile HAL_LPUART_StateTypeDef gState;        //LPUART TX及全局工作状态
    volatile HAL_LPUART_StateTypeDef RxState;       //LPUART RX工作状态
    volatile HAL_LPUART_ErrorCodeTypeDef ErrorCode; //LPUART错误码
} HAL_LPUART_HandleTypeDef;

/**
 * @brief  初始化LPUART. 耗时1478us
 * @param  hlpuart 详情参考 @ref HAL_LPUART_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 */
HAL_StatusTypeDef HAL_LPUART_Init(HAL_LPUART_HandleTypeDef *hlpuart);

/**
 * @brief  去初始化LPUART. 耗时1108us
 * @param  hlpuart 详情参考 @ref HAL_LPUART_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 */
HAL_StatusTypeDef HAL_LPUART_DeInit(HAL_LPUART_HandleTypeDef *hlpuart);

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
HAL_StatusTypeDef HAL_LPUART_Transmit(HAL_LPUART_HandleTypeDef *hlpuart, uint8_t *pData, uint32_t Size, uint32_t Timeout);

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
HAL_StatusTypeDef HAL_LPUART_Receive(HAL_LPUART_HandleTypeDef *hlpuart, uint8_t *pData, uint32_t Size, uint32_t Timeout);

/**
 * @brief  LPUART非阻塞发送API.
 * @param  hlpuart 详情参考 @ref HAL_LPUART_HandleTypeDef.
 * @param  pData 发送数据缓冲区指针.
 * @param  Size  发送数据字节长度. 长度不得超过512字节，超过则断言.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示中断发送需要的参数配置、外设配置成功，等待进入中断发送数据
 *         @retval  HAL_ERROR    ：入参错误
 *         @retval  HAL_BUSY     ：外设正在使用中
 */
HAL_StatusTypeDef HAL_LPUART_Transmit_IT(HAL_LPUART_HandleTypeDef *hlpuart, uint8_t *pData, uint32_t Size);

/**
 * @brief  LPUART非阻塞接收API.
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
HAL_StatusTypeDef HAL_LPUART_Receive_IT(HAL_LPUART_HandleTypeDef *hlpuart, uint8_t *pData, uint32_t Size, uint32_t HardTimeout);

/**
 * @brief  LPUART错误回调函数
 * @param  hlpuart. 详见结构体定义 HAL_LPUART_HandleTypeDef.
 */
void HAL_LPUART_ErrorCallback(HAL_LPUART_HandleTypeDef *hlpuart);

/**
 * @brief  LPUART发送完成回调函数.
 * @param  hlpuart. 详见结构体定义 HAL_LPUART_HandleTypeDef.
 */
void HAL_LPUART_TxCpltCallback(HAL_LPUART_HandleTypeDef *hlpuart);

/**
 * @brief  LPUART接收完成回调函数.
 * @param  hlpuart. 详见结构体定义 HAL_LPUART_HandleTypeDef.
 * @note   该回调函数会在指定长度(RxXferSize)数据接收完成后被调用，或者LPUART模块接收超时后被调用.
 */
void HAL_LPUART_RxCpltCallback(HAL_LPUART_HandleTypeDef *hlpuart);
