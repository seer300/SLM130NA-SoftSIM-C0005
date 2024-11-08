#pragma once

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "hal_csp.h"
#include "CuTest.h"
#include "hal_i2c.h"
#include "hal_uart.h"
#include "hal_spi.h"

/****************配置驱动测试时的打印信息*******************/
#define OPEN_DEBUG
// #define DEBUG_LEVEL DEBUG_LEVEL_3
#define DEBUG_LEVEL_0 0 //仅打印测试结果
#define DEBUG_LEVEL_1 1 //仅打印必要的显示
#define DEBUG_LEVEL_2 2 //打印每一次发送和接收开始和结束提示
#define DEBUG_LEVEL_3 3 //打印每一次发送和接收的数据

#define MasterSlave_UART 1//主从通信通道，1：CSP1; 2:CSP2;

extern int32_t InputDebugLevel;
extern int32_t TestTimes;
extern int32_t Switchclksys; 

/****************打印信息宏*******************/
#ifdef OPEN_DEBUG
    #ifdef DEBUG_LEVEL
        #define Debug_Print_Hex(DebugLevel, ...) DebugLevel <= DEBUG_LEVEL ? Output_Print_Hex(__VA_ARGS__) : 0
        #define Debug_Print_Reg(DebugLevel, ...) DebugLevel <= DEBUG_LEVEL ? Output_Print_Reg(__VA_ARGS__) : 0
        #define Debug_Print_Str(DebugLevel, ...) DebugLevel <= DEBUG_LEVEL ? Output_Print_Str(__VA_ARGS__) : 0
        #define Debug_Print_ArgStruct(DebugLevel, ...) DebugLevel <= DEBUG_LEVEL ? Output_Print_ArgStruct(__VA_ARGS__) : 0
        #define Debug_Delay(DebugLevel, Arg1, Arg2) DebugLevel <= DEBUG_LEVEL ? HAL_Delay(Arg1) : HAL_Delay(Arg2)
    #else
        #define Debug_Print_Hex(DebugLevel, ...) DebugLevel <= InputDebugLevel ? Output_Print_Hex(__VA_ARGS__) : 0
        #define Debug_Print_Reg(DebugLevel, ...) DebugLevel <= InputDebugLevel ? Output_Print_Reg(__VA_ARGS__) : 0
        #define Debug_Print_Str(DebugLevel, ...) DebugLevel <= InputDebugLevel ? Output_Print_Str(__VA_ARGS__) : 0
        #define Debug_Print_ArgStruct(DebugLevel, ...) DebugLevel <= InputDebugLevel ? Output_Print_ArgStruct(__VA_ARGS__) : 0
        #define Debug_Delay(DebugLevel, Arg1, Arg2) DebugLevel <= InputDebugLevel ? HAL_Delay(Arg2) : HAL_Delay(Arg1)
    #endif
#else
    #define Debug_Print_Hex(...)
    #define Debug_Print_Reg(...)
    #define Debug_Print_Str(...)
    #define Debug_Delay(...)
#endif

#define MaxArg 0xFFFFFFFF
#define MaxArgArray ((uint32_t *)0xFFFFFFFF)

#define TRANSMIT_DEFAULT_MIN_DELAY 10
#define TRANSMIT_DEFAULT_MID_DELAY 200
#define TRANSMIT_DEFAULT_MAX_DELAY 10000
#define RECEIVE_DEFAULT_MIN_DELAY 100
#define RECEIVE_DEFAULT_MID_DELAY 1000
#define RECEIVE_DEFAULT_MAX_DELAY 10000

/*驱动测试专用，等待CPboot成功的时长，默认30秒*/
#define WAIT_CP_BOOT_TICK  (30*1000)

/*驱动测试专用，等待CP主动stop成功的时长，默认10秒*/
#define WAIT_CP_STOP_TICK  (10*1000)
typedef enum
{
    test_UART = 1,
    test_I2C = 2,
    test_SPI = 3,
    test_CSP_UART = 4,
    test_CSP_SPI = 5
}ArgStructId;

typedef struct
{
    uint32_t CspUartTxPin;
    uint32_t CspUartRxPin;
    uint32_t CspUartInstance;
    uint32_t CspUartBaudrate;
    uint32_t CspUartDatabits;
    uint32_t CspUartStopbits;
    uint32_t CspUartParitybit;
    uint32_t CspUartTestSize;

    uint32_t MaxArgElement;
} CspUartArgStruct;

typedef struct
{
    uint32_t CspSpiMosiPin;
    uint32_t CspSpiMisoPin;
    uint32_t CspSpiSclkPin;
    uint32_t CspSpiNss1Pin;
    uint32_t CspSpiInstance;
    uint32_t CspSpiWorkMode;
    uint32_t CspSpiClock;
    uint32_t CspSpiTestSize;
    uint32_t MaxArgElement;
} CspSpiArgStruct;

typedef struct
{
    uint32_t UartTxPin;
    uint32_t UartRxPin;
    uint32_t UartInstance;
    uint32_t UartBaudrate;
    uint32_t UartDatabits;
    uint32_t UartStopbits;
    uint32_t UartParity;
    uint32_t UartTestSize;
    uint32_t MaxArgElement;
} UartArgStruct;

typedef struct
{
    uint32_t SpiMosiPin;
    uint32_t SpiMisoPin;
    uint32_t SpiSclkPin;
    uint32_t SpiNss1Pin;
    uint32_t SpiInstance;
    uint32_t SpiWorkMode;
    uint32_t SpiClkDiv;
    uint32_t SpiTestSize;
    uint32_t MaxArgElement;
} SpiArgStruct;

typedef struct
{
    uint32_t I2cSclPin;
    uint32_t I2cSdaPin;
    uint32_t I2cInstance;
    uint32_t I2cAddressMode;
    uint32_t I2cClockSpeed;
    uint32_t I2cSlaveAddress;
    uint32_t I2cTestSize;
    uint32_t MaxArgElement;
} I2cArgStruct;

/****************驱动测试时，主MCU与从MCU通信使用********************/
#define WAIT_OK     HAL_CSP_Receive(&MasterSlave_UART_Handle, (uint8_t *)RecvConfigBuffer, 3, RECEIVE_DEFAULT_MAX_DELAY)
#define SEND_OK     HAL_CSP_Transmit(&MasterSlave_UART_Handle, (uint8_t *)"OK\n", 3, TRANSMIT_DEFAULT_MID_DELAY)

extern HAL_CSP_HandleTypeDef MasterSlave_UART_Handle;
extern volatile uint8_t MasterSlave_RxCplt_Flag;
void MasterSlave_UART_Init(void);
uint8_t MasterSlave_UART_GetInit();

/*************主MCU与上位机通信时，输出的格式化数据处理接口************/
void Output_Print_ArgStruct(void *ArgStruct);
void Output_Print_Hex(uint8_t *pData, uint16_t length);
void Output_Print_Reg(uint32_t pRegAddr, uint32_t length);
void Output_Print_Str(char *fmt, ...);

/**********************提取字符串中的数字接口************************/
int32_t get_num_from_cmd(uint8_t *str);

/****************驱动测试时，主机处理数据的一些接口*******************/
void InitArgStruct(void *pArgStruct, ...);
void DeInitArgStruct(void *pArgStruct);
uint8_t GetArgStruct(void *pArgStruct);
uint32_t GetTotalCaseNun(void);
void GetCmdDataToSlave(void *pBuf, uint32_t id, uint32_t size, uint32_t DebugLevel, void *pArgStruct);

/****************驱动测试时，从机处理数据的一些接口*******************/
void GetArgStructFromMaster(void *pBuf, uint32_t *id, uint32_t *size, int32_t *DebugLevel, void *pArgStruct);
HAL_StatusTypeDef HAL_I2C_Drivertest_DeInit(HAL_I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef HAL_CSP_Drivertest_DeInit(HAL_CSP_HandleTypeDef *hcsp);
HAL_StatusTypeDef HAL_SPI_Drivertest_DeInit(HAL_SPI_HandleTypeDef *hspi);
void UART_GPIO_Remove(void);