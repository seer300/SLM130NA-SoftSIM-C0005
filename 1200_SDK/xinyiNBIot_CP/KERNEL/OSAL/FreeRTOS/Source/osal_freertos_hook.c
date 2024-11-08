#include "cmsis_os2.h"
#include "sys_debug.h"
#include "at_uart.h"
#include <stdio.h>

#if (configSUPPORT_STATIC_ALLOCATION == 1)

void vApplicationGetIdleTaskMemory (StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize)
{
	/* Idle task control block and stack */
	static StaticTask_t Idle_TCB;
	static StackType_t  Idle_Stack[configMINIMAL_STACK_SIZE];

	*ppxIdleTaskTCBBuffer   = &Idle_TCB;
	*ppxIdleTaskStackBuffer = &Idle_Stack[0];
	*pulIdleTaskStackSize   = (uint32_t)configMINIMAL_STACK_SIZE;
}

#endif


#if( configCHECK_FOR_STACK_OVERFLOW > 0 )

__RAM_FUNC void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
	(void) xTask;

	write_to_at_uart("stack overflow in task ", strlen("stack overflow in task "));
	write_to_at_uart(pcTaskName,strlen(pcTaskName));

	Sys_Assert(0);
}

#endif

#if( configUSE_TICK_HOOK == 1 )

void vApplicationTickHook( void )
{

}

#endif

#if( configUSE_MALLOC_FAILED_HOOK == 1 )

void vApplicationMallocFailedHook( void )
{

}

#endif
