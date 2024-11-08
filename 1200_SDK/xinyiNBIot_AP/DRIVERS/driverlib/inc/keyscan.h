#ifndef __KEYSCAN_H__
#define __KEYSCAN_H__

#include "hw_keyscan.h"
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

#define KEY_COORD_VALUE_WIDE	  0x1F	
	
//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************
extern void KeyScanStart(void);
extern void KeyScanStop(void);
extern void KeyScanPadEn(unsigned long ulPadMsk);	
extern void KeyScanPadDis(unsigned long ulPadMsk);	
extern void KeyColValidEn(unsigned long ulKeyColValid);
extern void KeyColValidDis(unsigned long ulKeyColValid);	
extern unsigned char KeyRowAndColNumSet(unsigned char ucRowNum, unsigned char ucColNum);
extern void KeyScanConfig(unsigned char ucHoldCount, unsigned char ucScanRowCount, unsigned char ucScanCount, unsigned char ucVolState, unsigned char ucClkConfig);
extern void KeyScanLongPressEnable(unsigned char ucLongPressCount);
extern void KeyScanLongPressDisable(void);
extern unsigned char KeyScanValidStatus(void);		
extern unsigned char KeyScanValidCountGet(void);	
extern unsigned long KeyScanValidKeyCoordinate(void);
extern void KeyCoordinateConvert(const unsigned long ulKeyCoordinate, unsigned char* const ucKeyValue);	
extern void KeyScanIntRegister(unsigned long *g_pRAMVectors, void (*pfnHandler)(void));	
extern void KeyScanIntUnregister(unsigned long *g_pRAMVectors);
extern void KeyScanIntEnable(void);
extern void KeyScanIntDisable(void);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif


#endif  // __KEYSCAN_H__

