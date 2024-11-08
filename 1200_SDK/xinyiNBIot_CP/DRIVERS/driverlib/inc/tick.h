#ifndef __TICK_H__
#define __TICK_H__

#include "hw_types.h"
#include "hw_tick.h"
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
extern void TickTimerEnable(void);
extern void TickTimerDisable(void);
extern void TickPrescaleSet(unsigned char ucPrescale);
extern void TickAPReloadSet(unsigned char ucReload);
extern void TickCPReloadSet(unsigned char ucReload);
extern unsigned long TickAPReloadGet(void);
extern unsigned long TickCPReloadGet(void);
extern void TickCounterSet(unsigned long ulCounter);
extern unsigned int TickCounterGet(void);
extern void TickAPCompareSet(unsigned long ulCompare);
extern void TickCPCompareSet(unsigned long ulCompare);
extern unsigned long TickCPCompareGet(void);
extern void TickWRDone(void);
extern void TickAPIntEnable(unsigned char ucConfig);
extern void TickCPIntEnable(unsigned char ucConfig);
extern void TickAPIntDisable(unsigned char ucConfig);
extern void TickCPIntDisable(unsigned char ucConfig);
extern void TickIntRegister(unsigned long *g_pRAMVectors, void (*pfnHandler)(void));
extern void TickIntUnregister(unsigned long *g_pRAMVectors);
extern unsigned char TickAPReadAndClearInt(void);
extern unsigned char TickCPReadAndClearInt(void);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __TICK_H__
