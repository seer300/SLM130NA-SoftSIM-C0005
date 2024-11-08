/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/*-----------------------------------------------------------
 * Implementation of functions defined in portable.h for the ARM CM3 port.
 *----------------------------------------------------------*/

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "xy_utils.h"
#include "sys_config.h"
#if( configUSE_CLKTICK_PROVIDE_TICK == 1)

#include "tick.h"
#include "prcm.h"
#include "xinyi_hardware.h"

#define CORE_HARDWARE_IN_CP

#define configCLKTICK_CLOCK_HZ 				(CLK32K_FREQ / CLKTICK_FREQ32K_DIV)

#define portCLK_TICK_PRI					( ( uint8_t ) configKERNEL_INTERRUPT_PRIORITY )

//  the margin of computation error about 0.708% per ms
#define portCLK_TICK_ONE_TICK_COUNTS		( configCLKTICK_CLOCK_HZ / configTICK_RATE_HZ + 1 )


#ifdef CORE_HARDWARE_IN_AP

#define portCLK_TICK_CTRL_REG				( * ( ( volatile uint8_t  * ) 0x4000e000 ) )
#define portCLK_TICK_PRESCALE_REG			( * ( ( volatile uint8_t  * ) 0x4000e001 ) )
#define portCLK_TICK_INT_EN_REG				( * ( ( volatile uint8_t  * ) 0x4000e002 ) )
#define portCLK_TICK_RELOAD_REG				( * ( ( volatile uint32_t * ) 0x4000e004 ) )
#define portCLK_TICK_INT_STA_REG			( * ( ( volatile uint8_t  * ) 0x4000e00C ) )
#define portCLK_TICK_COUNTER_REG			( * ( ( volatile uint32_t * ) 0x4000e014 ) )
#define portCLK_TICK_COMPARE_REG			( * ( ( volatile uint32_t * ) 0x4000e018 ) )
#define portCLK_TICK_RELOAD_COUNTER_REG		( * ( ( volatile uint32_t * ) 0x4000e020 ) )
#define portCLK_WRITE_DONE_REG				( * ( ( volatile uint32_t * ) 0x4000e028 ) )
#define portCLK_WRITE_DONE_MASK				( 1 << 0 )
#define portNVIC_CLKPRI_REG					( * ( ( volatile uint8_t  * ) 0xe000e400 ) )
#define portNVIC_CLKINTEN_REG				( * ( ( volatile uint32_t * ) 0xe000e100 ) )

#define portCLK_TICK_INT_ENABLE_PERIOD		( 0x01U )
#define portCLK_TICK_INT_ENABLE_COMPARE		( 0x02U )
#define portCLK_TICK_INT_ENABLE_OVERFLOW	( 0x04U )

#define portCLK_TICK_INT_STATUS_PERIOD		( 0x01U )
#define portCLK_TICK_INT_STATUS_COMPARE		( 0x02U )
#define portCLK_TICK_INT_STATUS_OVERFLOW	( 0x04U )

#endif

#ifdef CORE_HARDWARE_IN_CP

#define portCLK_TICK_CTRL_REG				( * ( ( volatile uint8_t  * ) 0x4000e000 ) )
#define portCLK_TICK_PRESCALE_REG			( * ( ( volatile uint8_t  * ) 0x4000e001 ) )
#define portCLK_TICK_INT_EN_REG				( * ( ( volatile uint8_t  * ) 0x4000e003 ) )
#define portCLK_TICK_RELOAD_REG				( * ( ( volatile uint32_t * ) 0x4000e008 ) )
#define portCLK_TICK_INT_STA_REG			( * ( ( volatile uint8_t  * ) 0x4000e010 ) )
#define portCLK_TICK_COUNTER_REG			( * ( ( volatile uint32_t * ) 0x4000e014 ) )
#define portCLK_TICK_COMPARE_REG			( * ( ( volatile uint32_t * ) 0x4000e01C ) )
#define portCLK_TICK_RELOAD_COUNTER_REG		( * ( ( volatile uint32_t * ) 0x4000e024 ) )
#define portCLK_WRITE_DONE_REG				( * ( ( volatile uint32_t * ) 0x4000e028 ) )
#define portCLK_WRITE_DONE_MASK				( 1 << 0 )
#define portNVIC_CLKPRI_REG					( * ( ( volatile uint8_t  * ) 0xe000e400 ) )
#define portNVIC_CLKINTEN_REG				( * ( ( volatile uint32_t * ) 0xe000e100 ) )

#define portCLK_TICK_INT_ENABLE_PERIOD		( 0x01U )
#define portCLK_TICK_INT_ENABLE_COMPARE		( 0x02U )
#define portCLK_TICK_INT_ENABLE_OVERFLOW	( 0x04U )

#define portCLK_TICK_INT_STATUS_PERIOD		( 0x01U )
#define portCLK_TICK_INT_STATUS_COMPARE		( 0x02U )
#define portCLK_TICK_INT_STATUS_OVERFLOW	( 0x04U )

#endif

	#if( configDYNAMIC_OPTION_SYSTEM_TICK == 1 )
	volatile uint32_t xAbsoluteTickCountLast;
		static UBaseType_t uxTickTimerOverflow = 0;
		extern volatile TickType_t xNextTaskUnblockTime;

		__FLASH_FUNC void vPortSetupTimerInterrupt( void )
		{
		TickType_t xNextInterruptTicks;
		uint32_t xCompareValue;
		const TickType_t xTimerMaxTick = 0xFFFFFFFFUL / ( configCLKTICK_CLOCK_HZ * 1000 / configTICK_RATE_HZ ) * 1000;

            PRCM_ClockEnable(CORE_CKG_CTL_TICK_EN | CORE_CKG_CTL_UTC_EN);
			portCLK_TICK_INT_EN_REG |= portCLK_TICK_INT_ENABLE_COMPARE | portCLK_TICK_INT_ENABLE_OVERFLOW;
			xAbsoluteTickCountLast = 0;
			while( ( portCLK_WRITE_DONE_REG & portCLK_WRITE_DONE_MASK ) != portCLK_WRITE_DONE_MASK );

			xNextInterruptTicks = xNextTaskUnblockTime - vPortGetAbsoluteTick();

			if( xNextInterruptTicks > xTimerMaxTick )
			{
				xCompareValue = 0xFFFFFFFF;
			}
			else
			{
				xCompareValue = ( uint64_t ) xNextTaskUnblockTime * ( configCLKTICK_CLOCK_HZ * 1000 / configTICK_RATE_HZ ) / 1000 + 1;
			}

			/* Configure ClkTick to interrupt at the requested rate. */
			portCLK_TICK_COMPARE_REG = xCompareValue;
			portNVIC_CLKPRI_REG = portCLK_TICK_PRI;
			portNVIC_CLKINTEN_REG |= 0x1U;

			TickCPReadAndClearInt();
		}
extern unsigned char uxSuspendCheck();
		volatile uint32_t AbsoluteTickOverflowCount =0;
		void vPortSetupNextTimerInterrupt( void )
		{
		uint32_t xCompareValue;
		const TickType_t xTimerMaxTick = 0xFFFFFFFFUL / ( configCLKTICK_CLOCK_HZ * 1000 / configTICK_RATE_HZ ) * 1000;
		TickType_t xConstTickCount = vPortGetAbsoluteTick();
		uint32_t xCountValue = portCLK_TICK_COUNTER_REG;

			if( xNextTaskUnblockTime <= xConstTickCount )
			{
				//当两次中断间隔不足1ms时，立即设pending进行调度,否则会导致下一次tick中断不来
				if(uxSuspendCheck() == ( UBaseType_t ) pdFALSE )
				{
					NVIC_SetPendingIRQ(CLKTIM_IRQn);
					return;
				}
				//当调度器被挂起，设置pending,线程无法调度，中断处理中会不停地设pending,所以设置1ms以后的tick中断
				else
					xCompareValue = xCountValue + portCLK_TICK_ONE_TICK_COUNTS;
			}
			else if( ( xNextTaskUnblockTime - xConstTickCount ) > xTimerMaxTick )
			{
				xCompareValue = xCountValue - 1;
			}
			else
			{
				// + ( uint64_t )AbsoluteTickOverflowCount<<32

				xCompareValue = (( uint64_t )xNextTaskUnblockTime+ (( uint64_t )AbsoluteTickOverflowCount<<32))* ( configCLKTICK_CLOCK_HZ * 1000 / configTICK_RATE_HZ ) / 1000 + 1;


				if( ( xCompareValue - xCountValue ) < portCLK_TICK_ONE_TICK_COUNTS )
				{
					xCompareValue = xCountValue + portCLK_TICK_ONE_TICK_COUNTS;
				}
			}

			/* Set Compare */
			portCLK_TICK_COMPARE_REG = xCompareValue;
		}

		TickType_t vPortGetAbsoluteTick( void )
		{
		TickType_t xAbsoluteTickCount;
		uint32_t xSurplusCount;
		volatile uint32_t ulbasepri;
		const uint32_t xOnceOverflowTicks = 0x100000000ULL / configCLKTICK_CLOCK_HZ;
		const uint32_t xOnceOverflowSurplusCount = 0x100000000ULL - ( uint64_t ) xOnceOverflowTicks * configCLKTICK_CLOCK_HZ;
		    ulbasepri = portSET_INTERRUPT_MASK_FROM_ISR();
			xSurplusCount = xOnceOverflowSurplusCount * uxTickTimerOverflow + portCLK_TICK_COUNTER_REG;

			xAbsoluteTickCount = uxTickTimerOverflow * xOnceOverflowTicks * configTICK_RATE_HZ + \
					( uint64_t )xSurplusCount * 1000 / ( configCLKTICK_CLOCK_HZ * 1000 / configTICK_RATE_HZ );


			if (xAbsoluteTickCount < xAbsoluteTickCountLast)
			{
				AbsoluteTickOverflowCount++;
				//产生pending,更新compare值
				NVIC_SetPendingIRQ(CLKTIM_IRQn);
				//SwitchDelaylistPortClktick();
			}
			xAbsoluteTickCountLast = xAbsoluteTickCount;

			portCLEAR_INTERRUPT_MASK_FROM_ISR(ulbasepri);
			return xAbsoluteTickCount;
		}

	#else

		__FLASH_FUNC void vPortSetupTimerInterrupt( void )
		{
			portCLK_TICK_CTRL_REG = 0U;
			portCLK_TICK_PRESCALE_REG = 0U;
			portCLK_TICK_INT_EN_REG |= portCLK_TICK_INT_ENABLE_PERIOD;
			portCLK_TICK_COUNTER_REG = 0U;

			/* Configure ClkTick to interrupt at the requested rate. */
			portCLK_TICK_RELOAD_REG = ( configCLKTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
			portNVIC_CLKPRI_REG = portCLK_TICK_PRI;
			portNVIC_CLKINTEN_REG |= 0x1U;

			TickCPReadAndClearInt();

			portCLK_TICK_CTRL_REG = 1U;
		}

	#endif /* configDYNAMIC_OPTION_SYSTEM_TICK */

	void CLKTIM_Handler(void)
	{
		uint8_t ucIntStatus = portCLK_TICK_INT_STA_REG;

		#if( configDYNAMIC_OPTION_SYSTEM_TICK == 1 )
		{
			if( portCLK_TICK_INT_STATUS_OVERFLOW & ucIntStatus )
			{
				uxTickTimerOverflow++;
			}
			else //if( portCLK_TICK_INT_STATUS_COMPARE & ucIntStatus )
			{
				extern void xPortSysTickHandler( void );
				xPortSysTickHandler();
			}
		}
		#else
		{
			if( portCLK_TICK_INT_STATUS_PERIOD & ucIntStatus )
			{
				extern void xPortSysTickHandler( void );
				xPortSysTickHandler();
			}
		}
		#endif /* configDYNAMIC_OPTION_SYSTEM_TICK */
	}

	void systick_recovery()
	{
		/*uxTickTimerOverflow
		xAbsoluteTickCountLast
		AbsoluteTickOverflowCount*/
	}



#endif /* configUSE_CLKTICK_PROVIDE_TICK */
