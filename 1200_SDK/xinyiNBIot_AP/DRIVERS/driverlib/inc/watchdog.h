#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

#include "hw_ints.h"
#include "xinyi2100.h"

#include "hw_types.h"
#include "hw_watchdog.h"
#include "debug.h"
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
// Prototypes for the APIs.
//
//*****************************************************************************
extern uint8_t WDT_IsRunning(WDT_TypeDef* WDTx);
extern void WDT_Enable(WDT_TypeDef* WDTx);
extern void WDT_Disable(WDT_TypeDef* WDTx);
extern void WDT_ResetEnable(WDT_TypeDef* WDTx);
extern void WDT_ResetDisable(WDT_TypeDef* WDTx);
extern void WDT_TimerRepeatEnable(WDT_TypeDef* WDTx);
extern void WDT_TimerRepeatDisable(WDT_TypeDef* WDTx);
extern void WDT_ReloadSet(WDT_TypeDef* WDTx, uint32_t LoadVal);
extern uint32_t WDT_ValueGet(WDT_TypeDef* WDTx);
extern void WDT_IntRegister(uint32_t *g_pRAMVectors, void (*pfnHandler)(void));
extern void WDT_IntUnregister(uint32_t *g_pRAMVectors);
extern void WDT_IntEnable(WDT_TypeDef* WDTx);
extern void WDT_IntDisable(WDT_TypeDef* WDTx);
extern uint32_t WDT_ReadClearInt(WDT_TypeDef* WDTx);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __WATCHDOG_H__
