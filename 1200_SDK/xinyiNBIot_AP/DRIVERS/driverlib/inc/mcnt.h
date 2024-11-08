#ifndef __MCNT_H__
#define __MCNT_H__

#include "hw_types.h"
#include "hw_mcnt.h"
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

//*****************************************************************************
//
//! \addtogroup dma_api
//! @{
//
//*****************************************************************************



//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************
extern void MCNT_Start(void);
extern void MCNT_Stop(void);
extern void MCNT_SetCNT32k(uint32_t ulCounter);
extern uint32_t MCNT_GetCNT32k(void);
extern uint32_t MCNT_GetMCNT(void);
extern uint8_t MCNT_GetIntStatus(void);
extern void MCNT_SetClkSrc(uint32_t ulClkSrc);
extern void MCNT_IntRegister(uint32_t *g_pRAMVectors, void (*pfnHandler)(void));
extern void MCNT_IntUnregister(uint32_t *g_pRAMVectors);
extern void MCNT_SelectMeasureClk(uint8_t value);
extern uint8_t MCNT_GetMeasureClk(void);
extern uint8_t MCNT_GetMeasureProcessStatus(void);
//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __MCNT_H__
