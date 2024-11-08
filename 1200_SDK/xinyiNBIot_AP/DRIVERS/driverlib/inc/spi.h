#ifndef __SPI_H__
#define __SPI_H__

#include "hw_spi.h"

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

//*****************************************************************************
//
// Values that can be passed to SPI_ConfigSetExpClk.
//
//*****************************************************************************
#define SPI_FRF_MOTO_MODE_0     0x00000000  // Moto fmt, polarity 0, phase 0
#define SPI_FRF_MOTO_MODE_1     0x00000004  // Moto fmt, polarity 0, phase 1
#define SPI_FRF_MOTO_MODE_2     0x00000002  // Moto fmt, polarity 1, phase 0
#define SPI_FRF_MOTO_MODE_3     0x00000006  // Moto fmt, polarity 1, phase 1

#define SPI_FIFO_BYTES_RX       (128)
#define SPI_FIFO_BYTES_TX       (128)
	
	
#define	SPI_FIFO_DATA_LEN    0x01
#define	SPI_FIFO_FULL        0x02
#define	SPI_FIFO_EMPTY       0x03	

//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************
extern void SPI_ConfigSetExpClk(uint32_t ulDivide,uint32_t ulProtocol,uint32_t ulMode,uint32_t ulDataWidth);
extern void SPI_Enable(void);
extern void SPI_Disable(void);
extern void SPI_ChipSelectMode(uint32_t ulMode);
extern void SPI_ChipSelect(uint32_t ulChipSelect);
extern void SPI_SlaveBurstEnable(void);
extern void SPI_SlaveBurstDisable(void);
extern void SPI_SetTxFifoThreshold(uint8_t ucThreshold);
extern void SPI_SetRxFifoThreshold(uint8_t ucThreshold);
extern void SPI_TxFifoReset(void);
extern void SPI_TxFifoEnable(void);
extern void SPI_TxFifoDisable(void);
extern void SPI_RxFifoReset(void);
extern void SPI_RxFifoEnable(void);
extern void SPI_RxFifoDisable(void);
extern void SPI_SetDelay(uint8_t ucTransNActiveDelay, uint8_t ucTransBetweenDelay, 
	                    uint8_t ucTransAfterDelay, uint8_t ucTransInitDelay);
extern void SPI_ManualCsSet(void);
extern void SPI_ManualCsClear(void);
extern void SPI_EnManualTransmit(void);
extern void SPI_DisManualTransmit(void);
extern void SPI_StartManualTransmit(void);
extern void SPI_ModeFailEnable(void);
extern void SPI_ModeFailDisable(void);
extern void SPI_IdleCountSet(uint8_t ucCount);
extern uint8_t SPI_ModeGet(void);
extern uint8_t SPI_ClockPhaseGet(void);
extern uint8_t SPI_ClockPolarityGet(void);
extern uint8_t SPI_ClockDivGet(void);
extern uint8_t SPI_DataWidthGet(void);
extern uint8_t SPI_RxFifoStatusGet(uint8_t ucFlagType);
extern uint8_t SPI_TxFifoStatusGet(uint8_t ucFlagType);
extern uint8_t SPI_IsEnable(void);
extern void SPI_IntRegister(uint32_t *g_pRAMVectors, void(*pfnHandler)(void));
extern void SPI_IntUnregister(uint32_t *g_pRAMVectors);
extern void SPI_IntEnable(uint32_t ulIntFlags);
extern void SPI_IntDisable(uint32_t ulIntFlags);
extern void SPI_IntClear(uint32_t ulIntFlags);
extern uint32_t SPI_IntStatus(void);
extern uint32_t SPI_IntMasked(void);
//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __SPI_H__




