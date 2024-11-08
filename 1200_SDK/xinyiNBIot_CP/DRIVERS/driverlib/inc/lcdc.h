#ifndef __LCDC_H__
#define __LCDC_H__

#include "hw_types.h"
#include "hw_lcdc.h"
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
//! \addtogroup lcdc_api
//! @{
//
//*****************************************************************************



//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************
extern void LCDC_TmuxSelSet(unsigned char ucValue);
extern void LCDC_DutySet(unsigned char ucValue);
extern void LCDC_BiasSet(unsigned char ucValue);
extern void LCDC_BypassEnSet(unsigned char ucValue);
extern void LCDC_LwaveSet(unsigned char ucValue);
extern unsigned char LCDC_LwaveGet(void);
extern void LCDC_ForceclkEnable(void);
extern void LCDC_ForceclkDisable(void);
extern void LCDC_BlinkSet(unsigned char ucValue);
extern void LCDC_DeadSet(unsigned char ucValue);
extern void LCDC_UvolVlcdCHSSet(unsigned char ucValue);
extern void LCDC_UvolLCPGainSet(unsigned char ucValue);
extern void LCDC_UvolSupplySelSet(unsigned char ucValue);
extern void LCDC_UvolClkCtrl(unsigned char ucValue);
extern void LCDC_FCRpsSet(unsigned char ucValue);
extern void LCDC_FCRLcdPuSet(unsigned char ucValue);
extern void LCDC_FCRdivSet(unsigned char ucValue);
extern void LCDC_UPDATEReqSet(unsigned char ucValue);
extern void LCDC_ComsegmuxSet(unsigned char ucValue);
extern unsigned char LCDC_ComsegmuxGet(void);
extern void LCDC_ComBasicSet(unsigned long ucValue, unsigned char comNo);
extern void LCDC_ComAdditionalSet(unsigned long ucValue,unsigned char comNo);
extern void LCDC_EN_Start(void);
extern void LCDC_EN_Stop(void);
//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __LCDC_H__
