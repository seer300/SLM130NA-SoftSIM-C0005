#ifndef __TRNG_H__
#define __TRNG_H__

#include "hw_trng.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "debug.h"
#include "hw_types.h"
#include "debug.h"
#include "interrupt.h"
extern void TRNG_Resetblock(void);

extern void TRNG_EnableHwRngClock(void);

extern void TRNG_SetSamlpeCountValue(uint32_t countValue);

extern uint32_t TRNG_ReadSamlpeCountValue(void);

extern void TRNG_SetRngRoscLength(uint32_t roscLength);

extern void TRNG_FastModeBypass(void);

extern void TRNG_FeModeBypass(void);

extern void TRNG_80090bModeBypass(void);

extern void TRNG_EnableRndSource(void);

extern void TRNG_DisableRndSource(void);

extern uint32_t TRNG_ReadValidReg(void);

extern uint32_t TRNG_ReadValidISRReg(void);

extern void TRNG_CleanUpInterruptStatus(void);

extern int TRNG_checkInput(uint32_t TRNGMode,uint32_t roscLength,uint32_t sampleCount );
	
extern void TRNG_initializingHardware(uint32_t TRNGMode,uint32_t roscLength,uint32_t sampleCount );

extern void TRNG_stopHardware(void);

extern void TRNG_IntRegister(unsigned long *g_pRAMVectors, void (*pfnHandler)(void));

extern void TRNG_IntUnregister(unsigned long *g_pRAMVectors);

extern uint32_t TRNG_HalWaitInterrupt(void);

extern void TRNG_IntEnable(unsigned long ulIntFlags);

extern void TRNG_IntDisable(unsigned long ulIntFlags);

extern void TRNG_IntClear(unsigned long ulIntFlags);

extern uint32_t TRNG_IntStatus(void);

extern void TRNG_rndWriteHeader(uint32_t TRNGMode,uint32_t roscLength,uint32_t sampleCount,uint8_t* dataU8Array);

extern int TRNG_loadEHR(uint8_t* dataU8Array);

extern void TRNG_rndWriteFooter(uint32_t Error,uint8_t* dataU8Array);

extern void TRNG_collectU8Array(uint32_t p, uint8_t* u8Array, uint32_t* u32Array);

#endif // __TRNG_H__
