/**
  ******************************************************************************
  * @file      startup_cm3.s
  * @author    xinyi platform software team
  * @version   V1.0
  * @date      15-June-2020
  * @brief     xinyi 1100plus vector table for RIDE7 toolchain.
  *            This module performs:
  *                - Set the initial SP
  *                - Set the initial PC == Reset_Handler,
  *                - Set the vector table entries with the exceptions ISR address
  *                - Branches to main in the C library (which eventually
  *                  calls main()).
  *            After Reset the Cortex-M3 processor is in Thread mode,
  *            priority is Privileged, and the Stack is set to Main.
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */

  .syntax unified
  .cpu cortex-m3
  .fpu softvfp
  .thumb

.global  g_pfnVectors
.global  Default_Handler

/* start address for the initialization values of the .data section.
defined in linker script */
.word  _sidata
/* start address for the .data section. defined in linker script */
.word  __data_start__
/* end address for the .data section. defined in linker script */
.word  __data_end__
/* start address for the .bss section. defined in linker script */
.word  __bss_start__
/* end address for the .bss section. defined in linker script */
.word  __bss_end__
/* stack used for SystemInit_ExtMemCtl; always internal RAM used */

    .section  .text.Reset_Handler
  .weak  Reset_Handler
  .type  Reset_Handler, %function
Reset_Handler:

/* Disable all interrupt before os start */
  cpsid i
/* flash init if fastboot */
  bl  first_excute_in_reset_handler

/* Copy the data segment initializers from flash to SRAM */
/* bug15130 the running time of moving data from flash to ram is very different(1.5ms to 5.5ms), modify code to circumvent this problem */
  ldr  r0, =__data_start__
  ldr  r1, =__data_end__
  ldr  r2, =_sidata
  b  LoopCopyDataInit

CopyDataInit:
  ldr  r3, [r2]
  str  r3, [r0]
  adds  r0, r0, #4
  adds  r2, r2, #4

LoopCopyDataInit:
  cmp  r0, r1
  bcc  CopyDataInit
  ldr  r2, =__bss_start__
  b  LoopFillZerobss
/* Zero fill the bss segment. */
FillZerobss:
  movs  r3, #0
  str  r3, [r2], #4

LoopFillZerobss:
  ldr  r3, =__bss_end__
  cmp  r2, r3
  bcc  FillZerobss

/* Special word fill the main stack. */
  ldr  r2, =__Main_Stack_Limit
  b  LoopFillMainStack
FillMainStack:
  ldr  r3, =0xA5A5A5A5
  str  r3, [r2], #4

LoopFillMainStack:
  ldr  r3, = __Main_Stack_Limit
  add  r3, 32
  cmp  r2, r3
  bcc  FillMainStack
  
/* Set the SP to use secondary boot's stack area */
  ldr sp, =__stack
/* Call the clock system intitialization function.*/
//为了兼容keil，统一放在main函数中去执行
/* bl  SystemInit */

/* Call the application's entry point.*/
  bl  main
  bx  lr
.size  Reset_Handler, .-Reset_Handler

/**
 * @brief  This is the code that gets called when the processor receives an
 *         unexpected interrupt.  This simply enters an infinite loop, preserving
 *         the system state for examination by a debugger.
 * @param  None
 * @retval None
*/
  .section .text.Default_Handler
  .weak Default_Handler
  .type Default_Handler, %function
Default_Handler:
  b  Default_Handler
  .size  Default_Handler, .-Default_Handler
/******************************************************************************
*
* The minimal vector table for a Cortex M3. Note that the proper constructs
* must be placed on this to ensure that it ends up at physical address
* 0x0000.0000.
*
*******************************************************************************/
   .section  .isr_vector,"a",%progbits
  .type  g_pfnVectors, %object
  .size  g_pfnVectors, .-g_pfnVectors


g_pfnVectors:
  .word  __stack
  .word  Reset_Handler
  .word  NMI_Handler
  .word  HardFault_Handler
  .word  MemManage_Handler
  .word  BusFault_Handler
  .word  UsageFault_Handler
  .word  0
  .word  0
  .word  0
  .word  0
  .word  SVC_Handler
  .word  DebugMon_Handler
  .word  0
  .word  PendSV_Handler
  .word  SysTick_Handler
  .word  CLKTIM_Handler
  .word  CSP1_Handler
  .word  I2C1_Handler
  .word  SPI1_Handler
  .word  TIM1_Handler
  .word  UART2_Handler
  .word  UTC_Handler
  .word  LPUART_Handler
  .word  LPTIM_Handler
  .word  MCNT_Handler
  .word  TIM2_Handler
  .word  CSP2_Handler
  .word  LPMODE_Handler
  .word  HRC_MISS_Handler
  .word  ROOTCLK_XTAL_Handler
  .word  ROOTCLK_PLL_Handler
  .word  QSPI_Handler
  .word  SEMAPHORE_Handler
  .word  TIM3_Handler
  .word  TIM4_Handler
  .word  DMAC0_Handler
  .word  DMAC1_Handler
  .word  DMAC2_Handler
  .word  DMAC3_Handler
  .word  WAKEUP_Handler
  .word  GPIO_Handler
  .word  I2C2_Handler
  .word  CSP3_Handler
  .word  CSP4_Handler
  .word  DMAC4_Handler
  .word  DMAC5_Handler
  .word  DMAC6_Handler
  .word  DMAC7_Handler
  .word  WDT_Handler
  .word  CACHE_Handler 
  .word  DFEBUF_Handler
  .word  PHYTMR_Handler
  .word  ADC_Handler
  .word  TANG_Hanlder
  .word  SHA_Hanlder
  .word  BB_Hanlder
  .word  CMP_Hanlder
  .word  KEYSCAN_Hanlder
  .word  ISO7816_Hanlder
  .word  SSPI_Hanlder
  .word  RC32K_Hanlder
  .word  IRQ46_Hanlder
  .word  IRQ47_Hanlder
/*******************************************************************************
*
* Provide weak aliases for each Exception handler to the Default_Handler.
* As they are weak aliases, any function with the same name will override
* this definition.
*
*******************************************************************************/

  .weak  NMI_Handler
  .thumb_set NMI_Handler,Default_Handler

//  .weak  HardFault_Handler
//  .thumb_set HardFault_Handler,Default_Handler

  .weak  MemManage_Handler
  .thumb_set MemManage_Handler,Default_Handler

  .weak  BusFault_Handler
  .thumb_set BusFault_Handler,Default_Handler

  .weak  UsageFault_Handler
  .thumb_set UsageFault_Handler,Default_Handler

  .weak  SVC_Handler
  .thumb_set SVC_Handler,Default_Handler

  .weak  DebugMon_Handler
  .thumb_set DebugMon_Handler,Default_Handler

//  .weak  PendSV_Handler
//  .thumb_set PendSV_Handler,Default_Handler

//  .weak  SysTick_Handler
//  .thumb_set SysTick_Handler,Default_Handler

  .weak  CSP1_Handler
  .thumb_set CSP1_Handler,Default_Handler

  .weak  I2C1_Handler
  .thumb_set I2C1_Handler,Default_Handler

  .weak  SPI1_Handler
  .thumb_set SPI1_Handler,Default_Handler

  .weak  TIM1_Handler
  .thumb_set TIM1_Handler,Default_Handler

  .weak  UART2_Handler
  .thumb_set UART2_Handler,Default_Handler

  .weak  LPUART_Handler
  .thumb_set LPUART_Handler,Default_Handler

  .weak  LPTIM_Handler
  .thumb_set LPTIM_Handler,Default_Handler

  .weak  LPMODE_Handler
  .thumb_set LPMODE_Handler,Default_Handler

  .weak  HRC_MISS_Handler
  .thumb_set HRC_MISS_Handler,Default_Handler

  .weak  ROOTCLK_XTAL_Handler
  .thumb_set ROOTCLK_XTAL_Handler,Default_Handler

  .weak  ROOTCLK_PLL_Handler
  .thumb_set ROOTCLK_PLL_Handler,Default_Handler

  .weak  UART1_Handler
  .thumb_set UART1_Handler,Default_Handler

//  .weak  UTC_Handler
//  .thumb_set UTC_Handler,Default_Handler

//  .weak  WDT_Handler
//  .thumb_set WDT_Handler,Default_Handler

//  .weak  MCNT_Handler
//  .thumb_set MCNT_Handler,Default_Handler

  .weak  TIM2_Handler
  .thumb_set TIM2_Handler,Default_Handler

//  .weak  CLKTIM_Handler
//  .thumb_set CLKTIM_Handler,Default_Handler

  .weak  CSP2_Handler
  .thumb_set CSP2_Handler,Default_Handler

  .weak  XTAL_Handler
  .thumb_set XTAL_Handler,Default_Handler

  .weak  PLL_Handler
  .thumb_set PLL_Handler,Default_Handler

  .weak  QSPI_Handler
  .thumb_set QSPI_Handler,Default_Handler

  .weak  SEMAPHORE_Handler
  .thumb_set SEMAPHORE_Handler,Default_Handler

  .weak  TIM3_Handler
  .thumb_set TIM3_Handler,Default_Handler

  .weak  TIM4_Handler
  .thumb_set TIM4_Handler,Default_Handler

  .weak  DMAC0_Handler
  .thumb_set DMAC0_Handler,Default_Handler

  .weak  DMAC1_Handler
  .thumb_set DMAC1_Handler,Default_Handler

  .weak  DMAC2_Handler
  .thumb_set DMAC2_Handler,Default_Handler

  .weak  DMAC3_Handler
  .thumb_set DMAC3_Handler,Default_Handler

//  .weak  WAKEUP_Handler
//  .thumb_set WAKEUP_Handler,Default_Handler

  .weak  GPIO_Handler
  .thumb_set GPIO_Handler,Default_Handler

  .weak  I2C2_Handler
  .thumb_set I2C2_Handler,Default_Handler

  .weak  CSP3_Handler
  .thumb_set CSP3_Handler,Default_Handler

  .weak  CSP4_Handler
  .thumb_set CSP4_Handler,Default_Handler

  .weak  DFEBUF_Handler
  .thumb_set DFEBUF_Handler,Default_Handler

  .weak  PHYTMR_Handler
  .thumb_set PHYTMR_Handler,Default_Handler

  .weak  AHB_CACHE_Handler
  .thumb_set AHB_CACHE_Handler,Default_Handler

  .weak  DMAC4_Handler
  .thumb_set DMAC4_Handler,Default_Handler

  .weak  DMAC5_Handler
  .thumb_set DMAC5_Handler,Default_Handler

  .weak  DMAC6_Handler
  .thumb_set DMAC6_Handler,Default_Handler

  .weak  DMAC7_Handler
  .thumb_set DMAC7_Handler,Default_Handler

  .weak   CACHE_Handler 
  .thumb_set CACHE_Handler,Default_Handler

  .weak   DFEBUF_Handler
  .thumb_set DFEBUF_Handler,Default_Handler

  .weak   PHYTMR_Handler
  .thumb_set PHYTMR_Handler,Default_Handler

  .weak   ADC_Handler
  .thumb_set ADC_Handler,Default_Handler

  .weak   TANG_Hanlder
  .thumb_set TANG_Hanlder,Default_Handler

  .weak   SHA_Hanlder
  .thumb_set SHA_Hanlder,Default_Handler

  .weak   BB_Hanlder
  .thumb_set BB_Hanlder,Default_Handler

  .weak   CMP_Hanlder
  .thumb_set CMP_Hanlder,Default_Handler

  .weak   KEYSCAN_Hanlder
  .thumb_set KEYSCAN_Hanlder,Default_Handler

  .weak   ISO7816_Hanlder
  .thumb_set ISO7816_Hanlder,Default_Handler

  .weak   SSPI_Hanlder
  .thumb_set SSPI_Hanlder,Default_Handler

  .weak   RC32K_Hanlder
  .thumb_set RC32K_Hanlder,Default_Handler

  .weak   IRQ46_Hanlder
  .thumb_set IRQ46_Hanlder,Default_Handler

  .weak   IRQ47_Hanlder
  .thumb_set IRQ47_Hanlder,Default_Handler
