;/**
;  ******************************************************************************
;  * @file      startup_cm3.s
;  * @author    xinyi platform software team
;  * @version   V1.0
;  * @date      15-June-2020
;  * @brief     xinyi 1100plus vector table for RIDE7 toolchain.
;  *            This module performs:
;  *                - Set the initial SP
;  *                - Set the initial PC == Reset_Handler,
;  *                - Set the vector table entries with the exceptions ISR address
;  *                - Branches to main in the C library (which eventually
;  *                  calls main()).
;  *            After Reset the Cortex-M3 processor is in Thread mode,
;  *            priority is Privileged, and the Stack is set to Main.
;  ******************************************************************************
;  * @attention
;  *
;  ******************************************************************************
;  */

                PRESERVE8
                THUMB

; Vector Table Mapped to Address 0 at Reset
                AREA    RESET, DATA, READONLY
				IMPORT  ||Image$$ARM_LIB_STACK$$ZI$$Limit||
                EXPORT  g_pfnVectors
                EXPORT  __Vectors_End
                EXPORT  __Vectors_Size

g_pfnVectors    DCD     ||Image$$ARM_LIB_STACK$$ZI$$Limit||               ; Top of Stack
                DCD     Reset_Handler              ; Reset Handler
                DCD     NMI_Handler                ; NMI Handler
                DCD     HardFault_Handler          ; Hard Fault Handler
                DCD     MemManage_Handler          ; MPU Fault Handler
                DCD     BusFault_Handler           ; Bus Fault Handler
                DCD     UsageFault_Handler         ; Usage Fault Handler
                DCD     0                          ; Reserved
                DCD     0                          ; Reserved
                DCD     0                          ; Reserved
                DCD     0                          ; Reserved
                DCD     SVC_Handler                ; SVCall Handler
                DCD     DebugMon_Handler           ; Debug Monitor Handler
                DCD     0                          ; Reserved
                DCD     PendSV_Handler             ; PendSV Handler
                DCD     SysTick_Handler            ; SysTick Handler

                ; External Interrupts
                DCD     CLKTIM_Handler
                DCD     CSP1_Handler
                DCD     I2C1_Handler
                DCD     SPI1_Handler
                DCD     TIM1_Handler
                DCD     UART2_Handler
                DCD     UTC_Handler
                DCD     LPUART_Handler
                DCD     LPTIM_Handler
                DCD     MCNT_Handler
                DCD     TIM2_Handler
                DCD     CSP2_Handler
                DCD     LPMODE_Handler
                DCD     HRC_MISS_Handler
                DCD     ROOTCLK_XTAL_Handler
                DCD     ROOTCLK_PLL_Handler
                DCD     QSPI_Handler
                DCD     SEMAPHORE_Handler
                DCD     TIM3_Handler
                DCD     TIM4_Handler
                DCD     DMAC0_Handler
                DCD     DMAC1_Handler
                DCD     DMAC2_Handler
                DCD     DMAC3_Handler
                DCD     WAKEUP_Handler
                DCD     GPIO_Handler
                DCD     I2C2_Handler
                DCD     CSP3_Handler
                DCD     CSP4_Handler
                DCD     DMAC4_Handler
                DCD     DMAC5_Handler
                DCD     DMAC6_Handler
                DCD     DMAC7_Handler
                DCD     WDT_Handler
                DCD     CACHE_Handler 
                DCD     DFEBUF_Handler
                DCD     PHYTMR_Handler
                DCD     ADC_Handler
                DCD     TANG_Hanlder
                DCD     SHA_Hanlder
                DCD     BB_Hanlder
                DCD     CMP_Hanlder
                DCD     KEYSCAN_Hanlder
                DCD     ISO7816_Hanlder
                DCD     SSPI_Hanlder
                DCD     RC32K_Hanlder
                DCD     IRQ46_Hanlder
                DCD     IRQ47_Hanlder
__Vectors_End

__Vectors_Size  EQU  __Vectors_End - g_pfnVectors

                AREA    |.text|, CODE, READONLY
                ENTRY

; Reset handler routine
Reset_Handler     PROC
                  EXPORT  Reset_Handler             [WEAK]
                  IMPORT  __main
                  IMPORT  SystemInit
                  IMPORT  first_excute_in_reset_handler
				 CPSID   I
				 BL      first_excute_in_reset_handler
                 ;LDR     R0, =SystemInit
                 ;BLX     R0
                 LDR     R0, =__main
                 BX      R0
                 ENDP

; Dummy Exception Handlers (infinite loops which can be modified)

NMI_Handler     PROC
                EXPORT  NMI_Handler                [WEAK]
                B       .
                ENDP
HardFault_Handler\
                PROC
                EXPORT  HardFault_Handler          [WEAK]
                B       .
                ENDP
MemManage_Handler\
                PROC
                EXPORT  MemManage_Handler          [WEAK]
                B       .
                ENDP
BusFault_Handler\
                PROC
                EXPORT  BusFault_Handler           [WEAK]
                B       .
                ENDP
UsageFault_Handler\
                PROC
                EXPORT  UsageFault_Handler         [WEAK]
                B       .
                ENDP
SVC_Handler     PROC
                EXPORT  SVC_Handler                [WEAK]
                B       .
                ENDP
DebugMon_Handler\
                PROC
                EXPORT  DebugMon_Handler           [WEAK]
                B       .
                ENDP
PendSV_Handler  PROC
                EXPORT  PendSV_Handler             [WEAK]
                B       .
                ENDP
SysTick_Handler PROC
                EXPORT  SysTick_Handler            [WEAK]
                B       .
                ENDP

Default_Handler PROC
				
				        EXPORT  Default_Handler            [WEAK]
                EXPORT  CLKTIM_Handler             [WEAK]
                EXPORT  CSP1_Handler               [WEAK]
                EXPORT  I2C1_Handler               [WEAK]
                EXPORT  SPI1_Handler               [WEAK]
                EXPORT  TIM1_Handler               [WEAK]
                EXPORT  UART2_Handler              [WEAK]
                EXPORT  UTC_Handler                [WEAK]
                EXPORT  LPUART_Handler             [WEAK]
                EXPORT  LPTIM_Handler              [WEAK]
                EXPORT  MCNT_Handler               [WEAK]
                EXPORT  TIM2_Handler               [WEAK]
                EXPORT  CSP2_Handler               [WEAK]
                EXPORT  LPMODE_Handler             [WEAK]
                EXPORT  HRC_MISS_Handler           [WEAK]
                EXPORT  ROOTCLK_XTAL_Handler       [WEAK]
                EXPORT  ROOTCLK_PLL_Handler        [WEAK]
                EXPORT  QSPI_Handler               [WEAK]
                EXPORT  SEMAPHORE_Handler          [WEAK]
                EXPORT  TIM3_Handler               [WEAK]
                EXPORT  TIM4_Handler               [WEAK]
                EXPORT  DMAC0_Handler              [WEAK]
                EXPORT  DMAC1_Handler              [WEAK]
                EXPORT  DMAC2_Handler              [WEAK]
                EXPORT  DMAC3_Handler              [WEAK]
                EXPORT  WAKEUP_Handler             [WEAK]
                EXPORT  GPIO_Handler               [WEAK]
                EXPORT  I2C2_Handler               [WEAK]
                EXPORT  CSP3_Handler               [WEAK]
                EXPORT  CSP4_Handler               [WEAK]
                EXPORT  DMAC4_Handler              [WEAK]
                EXPORT  DMAC5_Handler              [WEAK]
                EXPORT  DMAC6_Handler              [WEAK]
                EXPORT  DMAC7_Handler              [WEAK]
                EXPORT  WDT_Handler                [WEAK]
                EXPORT  CACHE_Handler              [WEAK]
                EXPORT  DFEBUF_Handler             [WEAK]
                EXPORT  PHYTMR_Handler             [WEAK]
                EXPORT  ADC_Handler                [WEAK]
                EXPORT  TANG_Hanlder               [WEAK]
                EXPORT  SHA_Hanlder                [WEAK]
                EXPORT  BB_Hanlder                 [WEAK]
                EXPORT  CMP_Hanlder                [WEAK]
                EXPORT  KEYSCAN_Hanlder            [WEAK]
                EXPORT  ISO7816_Hanlder            [WEAK]
                EXPORT  SSPI_Hanlder               [WEAK]
                EXPORT  RC32K_Hanlder              [WEAK]
                EXPORT  IRQ46_Hanlder              [WEAK]
                EXPORT  IRQ47_Hanlder              [WEAK]

CLKTIM_Handler
CSP1_Handler
I2C1_Handler
SPI1_Handler
TIM1_Handler
UART2_Handler
UTC_Handler
LPUART_Handler
LPTIM_Handler
MCNT_Handler
TIM2_Handler
CSP2_Handler
LPMODE_Handler
HRC_MISS_Handler
ROOTCLK_XTAL_Handler
ROOTCLK_PLL_Handler
QSPI_Handler
SEMAPHORE_Handler
TIM3_Handler
TIM4_Handler
DMAC0_Handler
DMAC1_Handler
DMAC2_Handler
DMAC3_Handler
WAKEUP_Handler
GPIO_Handler
I2C2_Handler
CSP3_Handler
CSP4_Handler
DMAC4_Handler
DMAC5_Handler
DMAC6_Handler
DMAC7_Handler
WDT_Handler
CACHE_Handler 
DFEBUF_Handler
PHYTMR_Handler
ADC_Handler
TANG_Hanlder
SHA_Hanlder
BB_Hanlder
CMP_Hanlder
KEYSCAN_Hanlder
ISO7816_Hanlder
SSPI_Hanlder
RC32K_Hanlder
IRQ46_Hanlder
IRQ47_Hanlder


                B       .

                ENDP

                ALIGN

                 END
