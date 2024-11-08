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
extern void Tick_TimerEnable(void);
extern void Tick_TimerDisable(void);
extern void Tick_PrescaleSet(uint8_t ucPrescale);
extern void Tick_APReloadSet(uint8_t ucReload);
extern void Tick_CPReloadSet(uint8_t ucReload);
extern uint32_t Tick_APReloadGet(void);
extern uint32_t Tick_CPReloadGet(void);
extern void Tick_CounterSet(uint32_t ulCounter);
extern uint32_t Tick_CounterGet(void);
extern void Tick_APCompareSet(uint32_t ulCompare);
extern void Tick_CPCompareSet(uint32_t ulCompare);
extern void Tick_APIntEnable(uint8_t ucConfig);
extern void Tick_CPIntEnable(uint8_t ucConfig);
extern void Tick_APIntDisable(uint8_t ucConfig);
extern void Tick_CPIntDisable(uint8_t ucConfig);
extern void Tick_IntRegister(uint32_t *g_pRAMVectors, void (*pfnHandler)(void));
extern void Tick_IntUnregister(uint32_t *g_pRAMVectors);
extern uint8_t Tick_APReadAndClearInt(void);
extern uint8_t Tick_CPReadAndClearInt(void);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __TICK_H__
