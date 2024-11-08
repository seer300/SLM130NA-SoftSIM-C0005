#pragma once
#include "stdint.h"
#include "xinyi2100.h"

#ifndef __ALWAYS_STATIC_INLINE
#define __ALWAYS_STATIC_INLINE  __attribute__((always_inline)) static inline
#endif

typedef enum
{
	AP_WAKEUP_NORMAL = 0,
	AP_WAKEUP_FASTBOOT,
	AP_WAKEUP_FASTRECOVERY_BEFORE,
	AP_WAKEUP_FASTRECOVERY_AFTER,
} AP_WAKEUP_MODE;

typedef struct {
    uint32_t CTRL;                         /*!< 0xE000E010, SysTick Control and Status Register                   */
    uint32_t LOAD;                         /*!< 0xE000E014, SysTick Reload Value Register                         */
    uint32_t ISER[2];                      /*!< 0xE000E100, Interrupt Set Enable Register                         */
    uint8_t  IP[8];                 	   /*!< 0xE000E400, Interrupt Priority Register (8Bit wide)               */
    uint32_t VTOR;                         /*!< 0xE000ED08, Vector Table Offset Register                          */
    uint32_t AIRCR;                        /*!< 0xE000ED0C, Application Interrupt / Reset Control Register        */
    uint32_t CCR;                          /*!< 0xE000ED14, Configuration Control Register                        */
    uint8_t  SHP[8];                       /*!< 0xE000ED18, System Handlers Priority Registers (4-7, 8-11, 12-15) */
} nvic_scene_t;

typedef struct
{
	uint32_t DOUT0;           // 0x08
	uint32_t DOUT1;           // 0x0c
	uint32_t CTRL0;           // 0x10
	uint32_t CTRL1;           // 0x14
	uint32_t PULLUP0;         // 0x18
	uint32_t PULLUP1;         // 0x1c
	uint32_t PULLDOWN0;       // 0x20
	uint32_t PULLDOWN1;       // 0x24
	uint32_t INPUTEN0;        // 0x28
	uint32_t INPUTEN1;        // 0x2c
	uint32_t OUTPUTEN0;       // 0x38
	uint32_t OUTPUTEN1;       // 0x3c
	uint32_t CFGSEL0;         // 0x40
	uint32_t PAD_CFT0;        // 0x78
	uint32_t PAD_SEL2;        // 0xB4
	uint32_t PAD_SEL3;        // 0xB8
	uint32_t PERI_IN_SEL2;    // 0x1B8
	uint32_t PERI_IN_SEL3;    // 0x1BC
	uint32_t PERI_IN_SEL_EN1; // 0x3B4
} gpio_scene_t;

typedef struct
{
	uint32_t R0;
	uint32_t R1;
	uint32_t R2;
	uint32_t R3;
	uint32_t R4;
	uint32_t R5;
	uint32_t R6;
	uint32_t R7;
	uint32_t R8;
	uint32_t R9;
	uint32_t R10;
	uint32_t R11;
	uint32_t R12;
	uint32_t MSP;
	uint32_t PSP;
	uint32_t LR;
	uint32_t PC;
	uint32_t xPSR;
	uint32_t PRIMASK;
	uint32_t BASEPRI;
	uint32_t FAULTMASK;
	uint32_t CONTROL;
} m3_fast_recover_reg_t;
extern nvic_scene_t g_nvic_scene;
extern m3_fast_recover_reg_t g_m3_fast_recover_reg;
extern volatile uint32_t g_fast_startup_flag;

void FastSystemInit(void);

#if defined ( __CC_ARM   )
__asm __ALWAYS_STATIC_INLINE void save_scene_and_wfi(void)
{
	PRESERVE8
	IMPORT save_scene_hook_before_wfi
	IMPORT restore_scene_hook_after_wfi
	IMPORT g_m3_fast_recover_reg
	
	PUSH {R4}
	LDR R4, =g_m3_fast_recover_reg
	STMIA R4!, {R0-R3}
	MOV R0, R4
	POP {R4}
	STMIA R0!, {R4-R11}
	MOV R1, R12
	MRS R2, MSP
	MRS R3, PSP
	MOV R4, LR
	STMIA R0!, {R1-R4}
	LDR R1, =restore_scene
	MRS R2, XPSR
	MRS R3, PRIMASK
	MRS R4, BASEPRI
	MRS R5, FAULTMASK
	MRS R6, CONTROL
	STMIA R0!, {R1-R6}
	BL save_scene_hook_before_wfi
	WFI
restore_scene
	BL restore_scene_hook_after_wfi
	LDR R12, =g_m3_fast_recover_reg
	LDMIA R12!, {R0-R11}
	LDR R12, [R12]
	ALIGN
}

#elif defined ( __GNUC__ )
__ALWAYS_STATIC_INLINE void save_scene_and_wfi(void)
{
    __asm__ __volatile__ (
        /* push r4, use r4 to load variable address */
        "   push   {r4}                         \n"
        /* load variable address to r4, save r0-r3 */
        "   ldr    r4, =%0                      \n"
        "   stmia  r4!, {r0-r3}                 \n"
        /* move variable address to r0, pop r4 to restore sp, then save r4-r11 */
        "   mov    r0, r4                       \n"
        "   pop    {r4}                         \n"
        "   stmia  r0!, {r4-r11}                \n"
        /* save r12, msp, psp, lr, r12 save in this palace to alignment */
        "   mov    r1, r12                      \n"
        "   mrs    r2, msp                      \n"
        "   mrs    r3, psp                      \n"
        "   mov    r4, lr                       \n"
        "   stmia  r0!, {r1-r4}                 \n"
        /* set the symbol restore_addr as the pc value */
        "   ldr    r1, =restore_scene           \n"
        /* save xpsr, primask, basepri, faultmask, control */
        "   mrs    r2, xpsr                     \n"
        "   mrs    r3, primask                  \n"
        "   mrs    r4, basepri                  \n"
        "   mrs    r5, faultmask                \n"
        "   mrs    r6, control                  \n"
        "   stmia  r0!, {r1-r6}                 \n"
        "   bl     save_scene_hook_before_wfi   \n"
        /* excute wfi to entry sleep mode */
        "restore_scene:                         \n"
        /* restore */
        "   bl     restore_scene_hook_after_wfi \n"
        /* use r12 to load variable address */
        "   ldr    r12, =%0                     \n"
        /* recovery r0-r11, and store r12 to recovery r12 */
        "   ldmia  r12!, {r0-r11}               \n"
        "   ldr    r12, [r12]                   \n"
        : : "X" (g_m3_fast_recover_reg)
    );
}
#endif

#if defined ( __CC_ARM   )
__asm __ALWAYS_STATIC_INLINE void restore_scene_and_running(void)
{
	PRESERVE8
	IMPORT g_m3_fast_recover_reg
	LDR R0, =g_m3_fast_recover_reg
	LDR R0, [R0, #52]
	LDMIA R0!, {R1-R3}
	MSR MSP, R1
	MSR PSP, R2
	MOV LR, R3
	LDMIA R0!, {R1-R6}
	MSR XPSR_NZCVQ, R2
	MSR PRIMASK, R3
	MSR BASEPRI, R4
	MSR FAULTMASK, R5
	MSR CONTROL, R6
	BX R1
	ALIGN
}
__asm __ALWAYS_STATIC_INLINE void restore_msp(void)
{
	PRESERVE8
	IMPORT g_m3_fast_recover_reg
	LDR R0, =g_m3_fast_recover_reg
	LDR R0, [R0, #52]
	LDR R1, [R0]
	MSR MSP, R1
	ALIGN
}
#elif defined ( __GNUC__ )
__ALWAYS_STATIC_INLINE void restore_scene_and_running(void) 
{
  	__asm__ __volatile__(
        /* load up the address of g_nvic_scene, and start restore from MSP. */
        "   ldr    r0, =%0                      \n"
        /* restore msp, psp, lr */
        "   ldmia  r0!, {r1-r3}                 \n"
        "   msr    msp, r1                      \n"
        "   msr    psp, r2                      \n"
        "   mov    lr, r3                       \n"
        /* restore xpsr, primask, basepri, faultmask, control, and then set pc
        to restore running. */
        "   ldmia  r0!, {r1-r6}                 \n"
        /* _nzcvq to specifying a bitmask when write apsr.*/
        "   msr    xpsr_nzcvq, r2               \n"
        "   msr    primask, r3                  \n"
        "   msr    basepri, r4                  \n"
        "   msr    faultmask, r5                \n"
        "   msr    control, r6                  \n"
        "   mov    pc, r1                       \n"
        : : "X" (g_m3_fast_recover_reg.MSP)
    );
}

__ALWAYS_STATIC_INLINE void restore_msp(void)
{
	__asm__ __volatile__(
		/* load up the address of g_nvic_scene, and start restore from MSP. */
		"   ldr    r0, =%0                      \n"
		/* restore msp, psp, lr */
		"   ldr    r1, [r0]                     \n"
		"   msr    msp, r1                      \n"
		: : "X" (g_m3_fast_recover_reg.MSP)
	);
}
#endif