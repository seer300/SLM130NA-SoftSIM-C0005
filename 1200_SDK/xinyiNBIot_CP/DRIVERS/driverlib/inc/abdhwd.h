#ifndef __ABDHWD_H__
#define __ABDHWD_H__

#include "hw_abdhwd.h"

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


extern void ABD_Enable(void);
extern void ABD_Disable(void);
extern uint8_t ABD_EndStatusGet(void);
extern uint32_t ABD_CountValueGet(void);
extern void HW_DetectConfig(uint8_t CSPNum, uint8_t ValidBits,uint8_t Pattern);
extern void HW_DetectEnable(void);
extern void HW_DetectDisable(void);		

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __ABDHWD_H__



