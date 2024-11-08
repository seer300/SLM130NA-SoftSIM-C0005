#ifndef __TRNG_H__
#define __TRNG_H__

#include "hw_trng.h"
#include "hw_ints.h"
#include "xinyi2100.h"

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

extern int TRNG_CheckInput(uint32_t TRNGMode,uint32_t roscLength,uint32_t sampleCount );
	
extern void TRNG_InitializingHardware(uint32_t TRNGMode,uint32_t roscLength,uint32_t sampleCount );

extern void TRNG_StopHardware(void);

extern uint8_t TRNG_LoadEHR(uint8_t* dataU8Array);

extern void TRNG_CollectU8Array(uint32_t p, uint8_t* u8Array, uint32_t* u32Array);

extern void TRNG_IntRegister(unsigned long *g_pRAMVectors, void (*pfnHandler)(void));

extern void TRNG_IntUnregister(unsigned long *g_pRAMVectors);

extern void TRNG_ConfigSmpcntRndsel(uint8_t sample_cnt_indx,uint8_t rnd_sel_indx);

#endif // __TRNG_H__
