#ifndef __SEMA_H__
#define __SEMA_H__

#include "hw_sema.h"

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

#define SEMA_MASTER_AP              0
#define SEMA_MASTER_CP              1

#define SEMA_MASTER_NONE            0x0F

#define SEMA_MASK_AP              0xFE
#define SEMA_MASK_CP              0xFD
#define SEMA_MASK_NONE            0xFF
            
#define SEMA_SLAVE_AONPRCM          0
#define SEMA_SLAVE_UTC              1
#define SEMA_SLAVE_LPUART           2
#define SEMA_SLAVE_LPTIMER          3
#define SEMA_SLAVE_COREPRCM         4
#define SEMA_SLAVE_GPIO             5
#define SEMA_SLAVE_QSPI             6
#define SEMA_SLAVE_MCNT             7
#define SEMA_SLAVE_I2C1             8
#define SEMA_SLAVE_SPI              9
#define SEMA_SLAVE_CSP1             10
#define SEMA_SLAVE_CSP2             11
#define SEMA_SLAVE_SRAM0_0          12
#define SEMA_SLAVE_SRAM0_1          13
#define SEMA_SLAVE_SRAM1_0          14
#define SEMA_SLAVE_SRAM1_1          15
#define SEMA_SLAVE_SRAM_SH2         16
#define SEMA_SLAVE_SRAM_SH1         17
#define SEMA_SLAVE_TIMER1           18
#define SEMA_SLAVE_TIMER2           19
#define SEMA_SLAVE_TIMER3           20
#define SEMA_SLAVE_TIMER4           21
#define SEMA_SLAVE_DMAC             22
#define SEMA_SLAVE_AES_DATA         23
#define SEMA_SLAVE_AES_REG          24
#define SEMA_SLAVE_I2C2             25
#define SEMA_SLAVE_CSP3             26
#define SEMA_SLAVE_CSP4             27
#define SEMA_SLAVE_UART2            28
#define SEMA_SLAVE_AUXADC           29
#define SEMA_SLAVE_PHYTMR           30
#define SEMA_SLAVE_TRNG             31
#define SEMA_SLAVE_SHA              32
#define SEMA_SLAVE_LCDC             33
#define SEMA_SLAVE_TICK_CTRL        34
#define SEMA_SLAVE_BB_REG           35
#define SEMA_SLAVE_DFE              36
#define SEMA_SLAVE_ISO7816          37
#define SEMA_SLAVE_KEYSCAN          38
#define SEMA_SLAVE_AUX              39

#define SEMA_ATWR_AUX               50
#define SEMA_IDLE_HARDWARE       	55
#define SEMA_SLAVE_CKG              60

#define SEMA_SEMA_CTL1_Pos           8
#define SEMA_SEMA_CTL2_Pos          16

#define SEMA_MASK_CP                0xFD
#define SEMA_MASK_AP                0xFE
#define SEMA_MASK_NULL              0xFF

// Software needs to appy the access permission by semaphore block first before access the resources.
// In case DMA, software needs to apply for it as well before start dma operation.
extern void SEMA_Release(uint8_t SemaSlave, uint8_t MasterMask);
extern void SEMA_ResetAll(void);
extern void SEMA_Request(uint8_t SemaMaster, uint8_t SemaSlave, uint32_t DmacReq,
	                    uint16_t SemaPority, uint8_t MasterMask);

extern uint8_t SEMA_MasterGet(uint8_t SemaSlave);						
extern void SEMA_RequestNonBlocking(uint8_t SemaSlave, uint32_t DmacReq,
						           uint16_t SemaPority, uint8_t MasterMask);

extern uint8_t SEMA_ReqQueueState(void);
extern void SEMA_SlaveGet(uint8_t SemaMaster, uint32_t* pSlaveReg0, uint32_t* pSlaveReg1);
extern void SEMA_IntRegister(uint32_t *g_pRAMVectors, void (*pfnHandler)(void));
extern void SEMA_IntUnregister(uint32_t *g_pRAMVectors);
extern void SEMA_MasterIntEnable(uint32_t SemaMaster);
extern void SEMA_MasterIntDisable(uint32_t SemaMaster);
extern void SEMA_MasterIntClear(uint32_t SemaMaster);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __SEMA_H__
