#include "dump.h"
#include "sys_debug.h"
#include "cmsis_device.h"
#include "MPU.h"
#include "hw_types.h"
#include "xy_memmap.h"
#include "dfe.h"
#include "watchdog.h"


typedef struct {
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
    uint32_t SP;
    uint32_t LR;
    uint32_t PC;
    uint32_t xPSR;
    uint32_t MSP;
    uint32_t PSP;
    uint32_t PRIMASK;
    uint32_t BASEPRI;
    uint32_t FAULTMASK;
    uint32_t CONTROL;
} m3_common_reg_t;

typedef struct
{
    uint8_t  MFSR;    /* Addr: 0xE000ED28, Memory Manage Fault Status Register */
    uint8_t  BFSR;    /* Addr: 0xE000ED29, Bus Fault Status Register           */
    uint16_t UFSR;    /* Addr: 0xE000ED2A, Usage Fault Status Register         */
    uint32_t HFSR;    /* Addr: 0xE000ED2C, Hard Fault Status Register          */
    uint32_t DFSR;    /* Addr: 0xE000ED30, Debug Fault Status Register         */
    uint32_t MMAR;    /* Addr: 0xE000ED34, Memory Manage Address Register      */
    uint32_t BFAR;    /* Addr: 0xE000ED38, Bus Fault Address Register          */
    uint32_t AFAR;    /* Addr: 0xE000ED3C, Auxiliary Fault Address Register    */
} m3_fault_reg_t;

typedef struct
{
    uint32_t R0;
    uint32_t R1;
    uint32_t R2;
    uint32_t R3;
    uint32_t R12;
    uint32_t LR;
    uint32_t PC;
    uint32_t xPSR;
} m3_int_push_reg_t;

m3_common_reg_t    m3_reg = {0};
m3_fault_reg_t     m3_fault = {0};
m3_int_push_reg_t *m3_push_reg = NULL;


#ifdef __CC_ARM

/******************************************************************************
 * @brief  This function dump current register to memory, use trace to analyze
 * @param  None
 * @retval None
 *****************************************************************************/
__asm void DumpRegister_from_Normal(void)
{
    /* push R4, use R4 to load variable address */
    PUSH   {R4}
    /* load variable address to R4, save R0-R3 */
    LDR    R4, =m3_reg
    STMIA  R4!, {R0-R3}
    /* move variable address to R0, pop R4 to restore sp, then save R4-R12 */
    MOV    R0, R4
    POP    {R4}
    STMIA  R0!, {R4-R12}
    /* save SP, LR, PC */
    MOV    R1, SP
    MOV    R2, LR
    MOV    R3, PC
    STMIA  R0!, {R1-R3}
    /* save xPSR, MSP, PSP, PRIMASK, BASEPRI, FAULTMASK, CONTROL */
    MRS    R1, xPSR
    MRS    R2, MSP
    MRS    R3, PSP
    MRS    R4, PRIMASK
    MRS    R5, BASEPRI
    MRS    R6, FAULTMASK
    MRS    R7, CONTROL
    STMIA  R0!, {R1-R7}
    /* use R12 to load variable address */
    LDR    R12, =m3_reg
    /* recovery R0-R11, and store R12 to recovery R12 */
    LDMIA  R12!, {R0-R11}
    LDR    R12, [R12]
    BX     LR
}

/******************************************************************************
 * @brief  This function handles Hard Fault exception.
 * @param  stack_sp: the SP before trigger HardFault, MSP or PSP
 * @retval None
 *****************************************************************************/
__asm void DumpRegister_from_Fault(uint32_t *stack_sp)
{
    /* save R4-R11 */
    LDR    R1, =m3_reg          // R1 is the address to save common register
    ADD    R1, R1, #16          // R1 is the address of m3_reg.R4
    STMIA  R1!, {R4-R11}        // now R1 is the address of m3_reg.R12
    /* save r0-r3 in stack */
    LDR    R1, =m3_reg          // reload R1, R1 is the address of m3_reg.R0
    MOV    R2, R0               // SP move to R2
    LDMIA  R2!, {R3-R6}         // load R0-R3 from stack
    STMIA  R1!, {R3-R6}         // save R0-R3
    /* save R12, SP, LR, PC, xPSR in stack */
    LDR    R1, =m3_reg          // reload R1, R1 is the address of m3_reg.R0
    ADD    R1, R1, #48          // R1 is the address of m3_reg.R12
    ADD    R2, R0, #16          // R2 is the stack address of R12
    LDMIA  R2!, {R3, R5-R7}     // load R12, LR, PC, xPSR from stack
    MOV    R4, R0               // SP move to R4
    STMIA  R1!, {R3-R7}         // now R1 is the address of m3_reg.MSP
    /* save MSP, PSP, PRIMASK, BASEPRI, FAULTMASK, CONTROL */
    LDR    R1, =m3_reg          // reload R1, R1 is the address of m3_reg.R0
    ADD    R1, R1, #68          // R1 is the address of m3_reg.MSP
    MRS    R2, MSP              // move all the special register to common register
    MRS    R3, PSP
    MRS    R4, PRIMASK
    MRS    R5, BASEPRI
    MRS    R6, FAULTMASK
    MRS    R7, CONTROL
    STMIA  R1!, {R2-R7}         // now R1 is the address of m3_reg.CONTROL + 4
    BX     LR
}

#endif


#ifdef __GNUC__

/******************************************************************************
 * @brief  This function dump current register to memory, use trace to analyze
 * @param  None
 * @retval None
 *****************************************************************************/
void __attribute__ (( naked )) DumpRegister_from_Normal(void)
{
    __asm__ __volatile__ (
        /* push r4, use r4 to load variable address */
        "   push   {r4}                         \n"
        /* load variable address to r4, save r0-r3 */
        "   ldr    r4, =m3_reg                  \n"
        "   stmia  r4!, {r0-r3}                 \n"
        /* move variable address to r0, pop r4 to restore sp, then save r4-r12 */
        "   mov    r0, r4                       \n"
        "   pop    {r4}                         \n"
        "   stmia  r0!, {r4-r12}                \n"
        /* save sp, lr, pc */
        "   mov    r1, sp                       \n"
        "   mov    r2, lr                       \n"
        "   mov    r3, pc                       \n"
        "   stmia  r0!, {r1-r3}                 \n"
        /* save xpsr, msp, psp, primask, basepri, faultmask, control */
        "   mrs    r1, xpsr                     \n"
        "   mrs    r2, msp                      \n"
        "   mrs    r3, psp                      \n"
        "   mrs    r4, primask                  \n"
        "   mrs    r5, basepri                  \n"
        "   mrs    r6, faultmask                \n"
        "   mrs    r7, control                  \n"
        "   stmia  r0!, {r1-r7}                 \n"
        /* use r12 to load variable address */
        "   ldr    r12, =m3_reg                 \n"
        /* recovery r0-r11, and store r12 to recovery r12 */
        "   ldmia  r12!, {r0-r11}               \n"
        "   ldr    r12, [r12]                   \n"
        "   bx     lr                           \n"
        "   .align 4                            \n"
    );
}

/******************************************************************************
 * @brief  This function handles Hard Fault exception.
 * @param  stack_sp: the SP before trigger HardFault, MSP or PSP
 * @retval None
 *****************************************************************************/
void __attribute__ (( naked )) DumpRegister_from_Fault(uint32_t *stack_sp)
{
    (void) stack_sp;

    __asm__ __volatile__ (
        /* save r4-r11 */
        "   ldr    r1, m3_reg_addr              \n"    // r1 is the address to save common register
        "   add    r1, r1, #16                  \n"    // r1 is the address of m3_reg.R4
        "   stmia  r1!, {r4-r11}                \n"    // now r1 is the address of m3_reg.R12
        /* save r0-r3 in stack */
        "   ldr    r1, m3_reg_addr              \n"    // reload r1, r1 is the address of m3_reg.R0
        "   mov    r2, r0                       \n"    // sp move to r2
        "   ldmia  r2!, {r3-r6}                 \n"    // load r0-r3 from stack
        "   stmia  r1!, {r3-r6}                 \n"    // save r0-r3
        /* save r12, sp, lr, pc, xpsr in         stack */
        "   ldr    r1, m3_reg_addr              \n"    // reload r1, r1 is the address of m3_reg.R0
        "   add    r1, r1, #48                  \n"    // r1 is the address of m3_reg.R12
        "   add    r2, r0, #16                  \n"    // r2 is the stack address of r12
        "   ldmia  r2!, {r3,r5-r7}              \n"    // load r12, lr, pc, xpsr from stack
        "   mov    r4, r0                       \n"    // sp move to r4
        "   stmia  r1!, {r3-r7}                 \n"    // now r1 is the address of m3_reg.MSP
        /* save msp, psp, primask, basep        ri, faultmask, control */
        "   ldr    r1, m3_reg_addr              \n"    // reload r1, r1 is the address of m3_reg.R0
        "   add    r1, r1, #68                  \n"    // r1 is the address of m3_reg.MSP
        "   mrs    r2, msp                      \n"    // move all the special register to common register
        "   mrs    r3, psp                      \n"
        "   mrs    r4, primask                  \n"
        "   mrs    r5, basepri                  \n"
        "   mrs    r6, faultmask                \n"
        "   mrs    r7, control                  \n"
        "   stmia  r1!, {r2-r7}                 \n"    // now r1 is the address of m3_reg.CONTROL + 4
        "   bx     lr                           \n"
        "   .align 4                            \n"
        "m3_reg_addr: .word m3_reg              \n"
    );
}

#endif


/******************************************************************************
 * @brief  This function handles Hard Fault exception.
 * @param  stack_sp: the SP before trigger HardFault, MSP or PSP
 * @retval None
 *****************************************************************************/
void Dump_Handler_in_Fault(uint32_t *stack_sp)
{
    DumpRegister_from_Normal();

    m3_push_reg = (m3_int_push_reg_t *)stack_sp;

    uint32_t reg  = SCB->CFSR;

    m3_fault.MFSR = (reg >> 0)  & 0xFF;
    m3_fault.BFSR = (reg >> 8)  & 0xFF;
    m3_fault.UFSR = (reg >> 16) & 0xFFFF;
    m3_fault.HFSR = SCB->HFSR;
    m3_fault.DFSR = SCB->DFSR;
    m3_fault.MMAR = SCB->MMFAR;
    m3_fault.BFAR = SCB->BFAR;
    m3_fault.AFAR = SCB->AFSR;




    assert_file = __FILE__;
    assert_line = __LINE__;

    WatchdogDisable(CP_WDT_BASE);

    rf_trx_close(); //关闭射频，防止射频工作时间过长
    extern void send_assert_info_to_AP();
    send_assert_info_to_AP();//ap核死机，此时ap核处于dump流程中，调用此函数也不会异常


#if (__MPU_PRESENT == 1U)
    MPU_Cmd(DISABLE);
#endif

#if (ASSERT_DUMP_MEMORY_ENABLE == 1)
    // dump memory, all memory dumped here, use for trace32
    Dump_Memory_to_File();
#endif
    while(1)
    	dump_sync();

    while(1);
}
