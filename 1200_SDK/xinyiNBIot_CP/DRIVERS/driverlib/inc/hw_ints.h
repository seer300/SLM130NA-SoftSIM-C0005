#ifndef __HW_INTS_H__
#define __HW_INTS_H__

//*****************************************************************************
//
// The following are defines for the fault assignments.
//
//*****************************************************************************
#define FAULT_NMI               2       // NMI fault
#define FAULT_HARD              3       // Hard fault
#define FAULT_MPU               4       // MPU fault
#define FAULT_BUS               5       // Bus fault
#define FAULT_USAGE             6       // Usage fault
#define FAULT_SVCALL            11      // SVCall
#define FAULT_DEBUG             12      // Debug monitor
#define FAULT_PENDSV            14      // PendSV
#define FAULT_SYSTICK           15      // System Tick

//*****************************************************************************
//
// The following are defines for the interrupt assignments.
//
//*****************************************************************************
#define INT_TICK                16      //lowpower systick
#define INT_CSP1                17      // 
#define INT_I2C1                18      // 
#define INT_SPI                 19      // 
#define INT_TIMER1              20      // 
#define INT_UART2               21      //LPUART2_FD 
#define INT_UTC                 22      // 
#define INT_UART1               23      //LPUART1_FD
#define INT_LPTIMER             24      //lowpower timer 
#define INT_MEASURECNT          25      // 
#define INT_TIMER2              26      // 
#define INT_CSP2                27      // 
#define INT_BREAK_LPMODE        28      // 
#define INT_HRC_MISS            29      //
#define INT_ROOTCLK_XTAL        30      // 
#define INT_ROOTCLK_PLL         31      // 
#define INT_QSPI                32      // 
#define INT_SEMAPHORE           33      // 
#define INT_TIMER3              34      // 
#define INT_TIMER4              35      // 
#define INT_DMAC0               36      // 
#define INT_DMAC1               37      // 
#define INT_DMAC2               38      // 
#define INT_DMAC3               39      // 
#define INT_WAKEUP              40      // 
#define INT_GPIO                41      //
#define INT_I2C2                42      //
#define INT_CSP3                43      //
#define INT_CSP4                44      //
#define INT_DMAC4               45      // 
#define INT_DMAC5               46      // 
#define INT_DMAC6               47      // 
#define INT_DMAC7               48      // 
#define INT_WDT                 49      //
#define INT_CACHE               50      //
#define INT_DFEBUF              51      //
#define INT_PHYTMR              52      //phy timer
#define INT_ADC                 53      //
#define INT_TRNG                54      //
#define INT_SHA                 55      //
#define INT_BB                  56      //
#define INT_CMP                 57      //
#define INT_KEYSCAN             58      //
#define INT_ISO7816             59
#define INT_SSPI                60
#define INT_RC32K               61

//*****************************************************************************
//
// The following are defines for the total number of interrupts.
//
//*****************************************************************************
//#define NUM_INTERRUPTS          80

#define NUM_INTERRUPTS          64

//*****************************************************************************
//
// The following are defines for the total number of priority levels.
//
//*****************************************************************************
#define NUM_PRIORITY            8
#define NUM_PRIORITY_BITS       3

#endif // __HW_INTS_H__
