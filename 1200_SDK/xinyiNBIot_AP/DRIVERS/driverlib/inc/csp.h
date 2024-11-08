#ifndef __CSP_H__
#define __CSP_H__

#include "hw_csp.h"
#include "interrupt.h"

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

#define CSP_MODE_UART           1
#define CSP_MODE_IRDA           2
#define CSP_MODE_SPI_Master     3
#define CSP_MODE_SPI_Slave      4
#define CSP_MODE_SIM            5

#define CSP_PIN_CLK             CSP_MODE1_CLK_PIN_MODE_Pos
#define CSP_PIN_RFS             CSP_MODE1_RFS_PIN_MODE_Pos
#define CSP_PIN_TFS             CSP_MODE1_TFS_PIN_MODE_Pos
#define CSP_PIN_RXD             CSP_MODE1_RXD_PIN_MODE_Pos
#define CSP_PIN_TXD             CSP_MODE1_TXD_PIN_MODE_Pos

#define CSP_PIN_MODE_HW         0
#define CSP_PIN_MODE_IO         1

#define CSP_PIN_OUTPUT          0
#define CSP_PIN_INPUT           1

#define CSP_FIFO_EMPTY     0x01
#define CSP_FIFO_FULL      0x02
#define CSP_FIFO_LEVEL     0x03

extern void CSP_UARTModeSet(CSP_TypeDef* CSPx, uint32_t ulPclk, uint32_t ulBaudrate, uint8_t ucDatabits, uint32_t ulParityCheck, uint8_t ucStopbits);
extern void CSP_UARTModeSet2(CSP_TypeDef* CSPx, uint32_t ulPclk, uint32_t ulBaudrate, uint8_t ucDatabits, uint32_t ulParityCheck, uint8_t ucStopbits);
extern void CSP_UARTAutoBaudrate(CSP_TypeDef* CSPx, uint32_t ulCount, uint8_t ucDatabits,  uint32_t ulParityCheck, uint8_t ucStopbits);
//extern void CSPIrdaMode(CSP_TypeDef* CSPx, uint32_t ulPclk, uint32_t ulBaudrate, uint8_t ucDatabits, uint8_t ucStopbits, uint32_t ulIdleLevel, uint32_t ulWidth);
extern void CSP_SPIMode(CSP_TypeDef* CSPx, uint32_t ulMode1, uint32_t ulMode2, uint32_t ulTxFrameCtl, uint32_t ulRxFrameCtl);
extern void CSP_FIFOLevelSet(CSP_TypeDef* CSPx, uint32_t ulTxLevel, uint32_t ulRxLevel);
extern void CSP_RXFIFO_LevelSet(CSP_TypeDef *CSPx, uint32_t ulRxFifo);
extern void CSP_TXFIFO_LevelSet(CSP_TypeDef *CSPx, uint32_t ulTxFifo);
extern uint32_t CSP_CharGet(CSP_TypeDef* CSPx);
extern void CSP_CharPut(CSP_TypeDef* CSPx, uint32_t ucData);
extern uint8_t CSP_RxFifoStatusGet(CSP_TypeDef* CSPx, uint8_t ucFlagType);
extern uint8_t CSP_TxFifoStatusGet(CSP_TypeDef* CSPx, uint8_t ucFlagType);
extern int32_t CSP_CharGetNonBlocking(CSP_TypeDef* CSPx);
extern int32_t CSP_CharPutNonBlocking(CSP_TypeDef* CSPx, uint8_t ucData);
extern void CSP_OperationModeSet(CSP_TypeDef* CSPx, uint32_t ulOperationMode);
extern void CSP_ClockModeSet(CSP_TypeDef* CSPx, uint32_t ulClockMode);
extern void CSP_HwDetectClockSet(CSP_TypeDef* CSPx, uint32_t ulPclk, uint32_t ulClock);
extern void CSP_IrdaEnable(CSP_TypeDef* CSPx, uint32_t ulIrdaEn);
extern void CSP_EndianModeSet(CSP_TypeDef* CSPx, uint32_t ulEndianMode);
extern void CSP_Enable(CSP_TypeDef* CSPx);
extern void CSP_Disable(CSP_TypeDef* CSPx);
extern void CSP_DrivenEdgeSet(CSP_TypeDef* CSPx, uint32_t ulRxEdge, uint32_t ulTxEdge);
extern void CSP_SyncValidLevelSet(CSP_TypeDef* CSPx, uint32_t ulRFS, uint32_t ulTFS);
extern void CSP_ClockIdleModeSet(CSP_TypeDef* CSPx, uint32_t ulClkIdleMode);
extern void CSP_ClockIdleLevelSet(CSP_TypeDef* CSPx, uint32_t ulClkIdleLevel);
extern void CSP_PinModeSet(CSP_TypeDef* CSPx, uint32_t ulCSPPin, uint32_t ulPinMode, uint32_t ulPinDirection);
extern void CSP_IrdaPulseWidthSet(CSP_TypeDef* CSPx, uint32_t ulWidth);
extern void CSP_IrdaIdleLevelSet(CSP_TypeDef* CSPx, uint32_t ulIrdaIdleLevel);
extern void CSP_ClockSet(CSP_TypeDef* CSPx, uint32_t ulCSPMode, uint32_t ulPclk, uint32_t ulClock);
extern void CSP_TxFrameSet(CSP_TypeDef* CSPx, uint32_t ulDelay, uint32_t ulDataLen, uint32_t ulSyncLen, uint32_t ulFrameLen);
extern void CSP_RxFrameSet(CSP_TypeDef* CSPx, uint32_t ulDelay, uint32_t ulDataLen, uint32_t ulFrameLen);
extern void CSP_RxEnable(CSP_TypeDef* CSPx);
extern void CSP_RxDisable(CSP_TypeDef* CSPx);
extern uint8_t CSP_RxStatusGet(CSP_TypeDef* CSPx);
extern void CSP_TxEnable(CSP_TypeDef* CSPx);
extern void CSP_TxDisable(CSP_TypeDef* CSPx);
extern void CSP_IntRegister(uint32_t ulIntChannel, uint32_t *g_pRAMVectors, void(*pfnHandler)(void));
extern void CSP_IntUnregister(uint32_t ulIntChannel, uint32_t *g_pRAMVectors);
extern void CSP_IntEnable(CSP_TypeDef* CSPx, uint32_t ulIntFlags);
extern void CSP_IntDisable(CSP_TypeDef* CSPx, uint32_t ulIntFlags);
extern uint32_t CSP_IntStatus(CSP_TypeDef* CSPx);
extern void CSP_IntClear(CSP_TypeDef* CSPx, uint32_t ulIntFlags);
extern void CSP_DMAConfigRX(CSP_TypeDef* CSPx, uint8_t ucLevelCheck, uint32_t ulDataLen);
extern void CSP_DMAConfigTX(CSP_TypeDef* CSPx, uint8_t ucLevelCheck, uint32_t ulDataLen);
extern void CSP_UARTRxTimeoutConfig(CSP_TypeDef* CSPx, uint32_t relevantFifoEmpty, uint16_t timeout_num);


/**
 * @brief 获取UART超时类型
 * @param CSPx 基地址
 * @return 0：超时倒计时触发条件为RXD处于IDLE
 *         1：超时倒计时触发条件为RXD处于IDLE且RXFIFO非空
 */
extern uint8_t CSP_UART_TimeoutCondition_Get(CSP_TypeDef *CSPx);

/**
 * @brief 设置UART超时类型
 * @param CSPx 基地址
 * @param relevantFifoEmpty 
 *         0：超时倒计时触发条件为RXD处于IDLE
 *         1：超时倒计时触发条件为RXD处于IDLE且RXFIFO非空
 */
extern void CSP_UART_TimeoutCondition_Set(CSP_TypeDef *CSPx, uint32_t relevantFifoEmpty);


extern void CSP_RXFifoClear(CSP_TypeDef* CSPx);
extern void CSP_TXFifoClear(CSP_TypeDef* CSPx);
extern void CSP_TFS_PIN_MODE(CSP_TypeDef *CSPx, uint8_t NewState);
extern void CSP_TFS_ACT_LEVEL(CSP_TypeDef *CSPx, uint8_t NewState);
extern void CSP_TFS_IO_MODE(CSP_TypeDef *CSPx, uint8_t NewState);
extern void CSP_TFS_PIN_VALUE(CSP_TypeDef *CSPx, uint8_t NewState);
extern void CSP_RFS_PIN_MODE(CSP_TypeDef *CSPx, uint8_t NewState);
extern void CSP_RFS_ACT_LEVEL(CSP_TypeDef *CSPx, uint8_t NewState);
extern void CSP_RFS_IO_MODE(CSP_TypeDef *CSPx, uint8_t NewState);
extern void CSP_RFS_PIN_VALUE(CSP_TypeDef *CSPx, uint8_t NewState);

extern void CSP_SPIConfigSetExpClk(CSP_TypeDef* CSPx, uint32_t ulPclk, uint32_t ulClock, uint8_t ulMode, uint8_t ucClkPol, uint8_t ucClkPha, uint8_t dataWidth);
//extern void CSP_SPIConfigSetExpClk(CSP_TypeDef* CSPx, uint8_t ulMode, uint32_t ulPclk, uint32_t ulClock, uint8_t CPOL, uint8_t CPHA);
extern void CSP_SetRxFifoThreshold(CSP_TypeDef* CSPx, uint32_t ulThreshold);
extern void CSP_SetTxFifoThreshold(CSP_TypeDef* CSPx, uint32_t ulThreshold);
extern void CSP_FifoReset(CSP_TypeDef* CSPx);
extern void CSP_FifoStart(CSP_TypeDef* CSPx);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __CSP_H__


