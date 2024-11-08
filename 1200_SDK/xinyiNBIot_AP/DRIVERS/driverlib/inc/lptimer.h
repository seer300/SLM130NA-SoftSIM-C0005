#ifndef __LPTIMER_H__
#define __LPTIMER_H__

#include "hw_lptimer.h"

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
extern void LPTimerEnable(unsigned long ulBase);
extern void LPTimerDisable(unsigned long ulBase);
extern void LPTimerDualEnable(void);
extern void LPTimerConfigure(unsigned long ulBase, unsigned long ulConfig);
extern void LPTimerInitCountValueSet(unsigned long ulBase, unsigned short ulValue);
extern void LPTimerReloadValueSet(unsigned long ulBase, unsigned short ulValue);
extern void LPTimerPWMValueSet(unsigned long ulBase, unsigned short ulValue);
extern void LPTimerPolaritySet(unsigned long ulBase, unsigned short ulInvert);
extern void LPTimerPrescaleSet(unsigned long ulBase, unsigned short usValue);
extern void LPTimerPWMDelaySet(unsigned long ulBase, unsigned short usValue);
extern unsigned short LPTimerCountValueGet(unsigned long ulBase);
extern unsigned short LPTimerReloadValueGet(unsigned long ulBase);
extern unsigned short LPTimerCaptureValueGet(unsigned long ulBase);		
extern unsigned char LPTimerPolarityGet(unsigned long ulBase);
extern void LPTimerClockSrcMux(unsigned long ulBase, unsigned char ucClkConfig);
extern void LPTimerClockSwitchEnable(unsigned long ulBase);
extern void LPTimerClockSwitchDisable(unsigned long ulBase);
extern void LPTimerClockSwitch(unsigned long ulBase, unsigned short usAction);
extern void LPTimerClockPolarity(unsigned long ulBase, unsigned short usEdge);
extern void LPTimerEXTClockFilterDelay(unsigned long ulBase, unsigned char ucValue);
extern void LPTimerClockGateFilterDelay(unsigned long ulBase, unsigned char ucValue);
extern unsigned char LPTimerClockStateGet(unsigned long ulBase, unsigned char ulClkFlags); 
extern void LPTimerExtPhaseDetectEnable(unsigned long ulBase);
extern void LPTimerExtPhaseDetectDisable(unsigned long ulBase);
extern void LPTimerExtPhaseDetectMode(unsigned long ulBase, unsigned short usMode);
extern void LPTimerIntRegister(unsigned long ulIntChannel, unsigned long *g_pRAMVectors, void (*pfnHandler)(void));
extern void LPTimerIntUnregister(unsigned long ulIntChannel, unsigned long *g_pRAMVectors);
extern void LPTimerIntEnable(unsigned long ulBase, unsigned char ucIntFlags);
extern void LPTimerIntDisable(unsigned long ulBase);
extern unsigned char LPTimerIntStatus(void);
extern unsigned char LPTimerIntEventGet(unsigned long ulBase);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __LPTIMER_H__


