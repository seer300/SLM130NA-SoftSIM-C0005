#ifndef __TIMER_H__
#define __TIMER_H__

#include "hw_timer.h"

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
// Values that can be passed to TimerConfigure as the ulConfig parameter.
//
//*****************************************************************************


//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************
extern void TimerEnable(unsigned long ulBase);
extern void TimerDisable(unsigned long ulBase);		
extern void TimerConfigure(unsigned long ulBase, unsigned long ulConfig);
extern void TimerInitCountValueSet(unsigned long ulBase, unsigned long ulValue);
extern void TimerReloadValueSet(unsigned long ulBase, unsigned long ulValue);
extern void TimerPWMValueSet(unsigned long ulBase, unsigned long ulValue);
extern void TimerPolaritySet(unsigned long ulBase, unsigned char ucInvert);	
extern void TimerPrescaleSet(unsigned long ulBase, unsigned long ulValue);
extern void TimerPWMDelaySet(unsigned long ulBase, unsigned long ulValue);
extern void TimerCountOffset(unsigned long ulBase, unsigned long ulOffsetDirect, unsigned long ulValue);
extern unsigned long TimerCountValueGet(unsigned long ulBase);
extern unsigned long TimerReloadValueGet(unsigned long ulBase);
extern unsigned long TimerCaptureValueGet(unsigned long ulBase);	
extern unsigned char TimerPolarityGet(unsigned long ulBase);	
extern void TimerIntRegister(unsigned long ulIntChannel, unsigned long *g_pRAMVectors, void (*pfnHandler)(void));
extern void TimerIntUnregister(unsigned long ulIntChannel, unsigned long *g_pRAMVectors);
extern void TimerIntEnable(unsigned long ulBase, unsigned long ulIntFlags);
extern unsigned char TimerIntEventGet(unsigned long ulBase);
//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __TIMER_H__
