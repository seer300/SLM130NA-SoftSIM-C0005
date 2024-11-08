#ifndef __DMA_H__
#define __DMA_H__

#include "hw_dma.h"

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
// Values that can be passed to some functions as the ulChannelNum parameter.
//
//*****************************************************************************
#define DMA_CHANNEL_0           0x00000000
#define DMA_CHANNEL_1           0x00000001
#define DMA_CHANNEL_2           0x00000002
#define DMA_CHANNEL_3           0x00000003
#define DMA_CHANNEL_4           0x00000004
#define DMA_CHANNEL_5           0x00000005
#define DMA_CHANNEL_6           0x00000006
#define DMA_CHANNEL_7           0x00000007

//*****************************************************************************
//
// Values that can be passed to DMAChannelPeriphReq as the ucPeriSrc parameter.
//
//*****************************************************************************	
#define DMA_REQNUM_CSP2_RX      0x00000000
#define DMA_REQNUM_CSP2_TX      0x00000001
#define DMA_REQNUM_CSP3_RX      0x00000002
#define DMA_REQNUM_CSP3_TX      0x00000003
#define DMA_REQNUM_CSP4_RX      0x00000004
#define DMA_REQNUM_CSP4_TX      0x00000005
#define DMA_REQNUM_AES_IN       0x00000006
#define DMA_REQNUM_AES_OUT      0x00000007
#define DMA_REQNUM_QSPI_RD      0x00000008
#define DMA_REQNUM_QSPI_WR      0x00000009
#define DMA_REQNUM_ADC_RD       0x0000000A
#define DMA_REQNUM_SHA_WR       0x0000000B
#define DMA_REQNUM_UART2_RX     0x0000000C
#define DMA_REQNUM_UART2_WR     0x0000000D
#define DMA_REQNUM_UART1_RX     0x0000000E
#define DMA_REQNUM_UART1_WR     0x0000000F
#define DMA_REQNUM_CS_REQ0      0x00000010
#define DMA_REQNUM_CS_REQ1      0x00000011
#define DMA_REQNUM_CS_REQ2      0x00000012
#define DMA_REQNUM_CS_REQ3      0x00000013

//*****************************************************************************
//
// Values that can be passed to DMAChannelTransferSet as the ucMemType parameter.
//
//*****************************************************************************
#define MEMORY_TYPE_AP			0x01
#define MEMORY_TYPE_CP			0x02
#define MEMORY_TYPE_DMA			0x03

//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************
extern void DMAChannelConfigure(unsigned long ulChannelNum, unsigned long ulConfig);
extern void DMAPeriphReqEn(unsigned long ulChannelNum);
extern void DMAPeriphReqDis(unsigned long ulChannelNum);
extern void DMAChannelTransferStart(unsigned long ulChannelNum);
extern uint8_t DMAChannelGetStartStatus(unsigned long ulChannelNum);
extern void DMAChannelTransferStop(unsigned long ulChannelNum);
extern void DMAChannelPeriphReq(unsigned long ulChannelNum, unsigned char ucPeriSrc);
extern void DMAChannelMuxDisable(unsigned long ulChannelNum);
extern void DMAChannelTransfer(unsigned long ulChannelNum, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned char ucMemType);
extern void DMAChannelTransferSet(unsigned long ulChannelNum, void *pvSrcAddr, void *pvDstAddr, unsigned long ulTransferSize, unsigned char ucMemType);
extern void DMAChannelArbitrateEnable(unsigned char num);
extern void DMAChannelArbitrateDisable(unsigned char num);
extern void DMAChannelNextPointerSet(unsigned long ulChannelNum, void *pvNextPointer);
extern void DMAErrorStatusClear(unsigned long ulChannelNum);
extern unsigned char DMAErrorStatusGet(unsigned long ulChannelNum);
extern unsigned long DMAReqSrcPendingGet(void);
extern unsigned long DMAChannelTransferRemainCNT(unsigned long ulChannelNum);
extern void DMACMemset(unsigned long ulChannelNum, void *pvDstAddr, unsigned char ucValue,unsigned long ulSize, unsigned char ucMemType);
extern void DMAIntRegister(unsigned long ulIntChannel, unsigned long *g_pRAMVectors, void (*pfnHandler)(void));
extern void DMAIntUnregister(unsigned long ulIntChannel, unsigned long *g_pRAMVectors);
extern void DMAIntClear(unsigned long ulChannelNum);
extern void DMAChannelWaitIdle(unsigned long ulChannelNum);
extern unsigned char DMAIntStatus(unsigned long ulChannelNum);
extern unsigned char DMAIntAllStatus(void);
extern void DMAClockGateEnable(unsigned char num);
extern void DMAClockGateDisable(unsigned char num);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __DMA_H__
