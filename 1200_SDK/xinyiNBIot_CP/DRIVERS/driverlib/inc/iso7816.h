#ifndef __ISO7816_H__
#define __ISO7816_H__

#include "hw_iso7816.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_types.h"
#include "debug.h"
#include "interrupt.h"


#define CLOCK_LOW		0
#define CLOCK_HIGH		1

extern void ISO7816_ColdReset(void);

extern void ISO7816_WarmReset(void);

extern void ISO7816_Deactivation(void);

extern void ISO7816_ClockStopEn(unsigned char ucClkLevel);

extern void ISO7816_ClockStopDis(void);

extern void ISO7816_TRxRetrySet(unsigned char ucRetryNum);

extern void ISO7816_ClockDiVSet(unsigned long ulRefClock, unsigned long ulExpectClock);

extern void ISO7816_ETUCycleSet(unsigned short int usiCyclesPerETU);

extern void ISO7816_IdleETUSet(unsigned short int usiIdleCnt);

extern unsigned char ISO7816_GetFifoByteNum(void);

extern unsigned char ISO7816_GetFifoFullFlag(void);

extern unsigned char ISO7816_GetFifoEmptyFlag(void);

extern unsigned char ISO7816_ByteGet(void);

extern void ISO7816_BytePut(unsigned char ucByte);

extern void ISO7816_SwitchToTxFromRx(void);

extern void ISO7816_SwitchToRxFromTx(void);

extern unsigned char ISO7816_IntStatGet(unsigned long ulIntFlags);

extern void ISO7816_IntStatClr(unsigned long ulIntFlags);

extern void ISO7816_IntEnable(unsigned long ulIntFlags);

extern void ISO7816_IntDisable(unsigned long ulIntFlags);

#endif // __ISO7816_H__
