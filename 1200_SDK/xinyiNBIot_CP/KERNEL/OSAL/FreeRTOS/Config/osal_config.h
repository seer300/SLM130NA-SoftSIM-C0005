#ifndef __OSAL_CONFIG_H
#define __OSAL_CONFIG_H


#ifndef __OSAL_H_
	#error "cannot include osal_config.h directly, please include cmsis_os2.h "
#endif


/* ==== Memory Management Select ==== */
#define USE_FreeRTOS_HEAP_5                 (configHEAP_MANAGE_TYPE == 5)
#define USE_FreeRTOS_HEAP_6                 (configHEAP_MANAGE_TYPE == 6)
#define USE_FreeRTOS_HEAP_7                 (configHEAP_MANAGE_TYPE == 7)
#define FreeRTOS_HEAP_MALLOC_WITH_RECORD    ((configUSE_HEAP_MALLOC_DEBUG == 1) || (configUSE_HEAP_TRACE_RECORD == 1))
#define FreeRTOS_HEAP_FREE_WITH_RECORD      (configUSE_HEAP_TRACE_RECORD == 1)


#if( configHEAP_MANAGE_TYPE == 6 )

  #if( ( configUSE_HEAP_MALLOC_DEBUG == 1 ) || ( configUSE_HEAP_TRACE_RECORD == 1 ) )
    #define pvPortMallocNormal( xSize, pcFile, xLine ) pvPortMallocWithAlignDebug( xSize, eHeapAlign_4, pcFile, xLine )
    #define pvPortMallocAlign( xSize, pcFile, xLine ) pvPortMallocWithAlignDebug( xSize, eHeapAlign_32, pcFile, xLine )
    #define pvPortLogMalloc( xSize, pcFile, xLine ) pvPortMallocWithAlignDebug( xSize, eHeapAlign_4, pcFile, xLine )

    #define pvPortReallocNormal( pv, xSize, pcFile, xLine ) pvPortReallocWithAlignDebug( pv, xSize, eHeapAlign_4, pcFile, xLine )
    #define pvPortReallocAlign( pv, xSize, pcFile, xLine ) pvPortReallocWithAlignDebug( pv, xSize, eHeapAlign_32, pcFile, xLine )
  #else
    #define pvPortMallocNormal( xSize ) pvPortMallocWithAlign( xSize, eHeapAlign_4 )
    #define pvPortMallocAlign( xSize ) pvPortMallocWithAlign( xSize, eHeapAlign_32 )
    #define pvPortLogMalloc( xSize ) pvPortMallocWithAlignDebug( xSize, eHeapAlign_4 )

    #define pvPortReallocNormal( pv, xSize ) pvPortReallocWithAlign( pv, xSize, eHeapAlign_4 )
    #define pvPortReallocAlign( pv, xSize ) pvPortReallocWithAlign( pv, xSize, eHeapAlign_32 )
  #endif

#elif( configHEAP_MANAGE_TYPE == 7 )

  #if( ( configUSE_HEAP_MALLOC_DEBUG == 1 ) || ( configUSE_HEAP_TRACE_RECORD == 1 ) )
    #define pvPortMallocNormal( xSize, pcFile, xLine ) pvPortMallocWithAlignDebug( xSize, RegionOne, eHeapAlign_4, pcFile, xLine )
    #define pvPortMallocAlign( xSize, pcFile, xLine ) pvPortMallocWithAlignDebug( xSize, RegionOne, eHeapAlign_32, pcFile, xLine )
    #define pvPortLogMalloc( xSize, pcFile, xLine ) pvPortMallocWithAlignDebug( xSize, RegionThree, eHeapAlign_4, pcFile, xLine )


    #define pvPortReallocNormal( pv, xSize, pcFile, xLine ) pvPortReallocWithAlignDebug( pv, xSize, RegionOne, eHeapAlign_4, pcFile, xLine )
    #define pvPortReallocAlign( pv, xSize, pcFile, xLine ) pvPortReallocWithAlignDebug( pv, xSize, RegionOne, eHeapAlign_32, pcFile, xLine )
  #else
    #define pvPortMallocNormal( xSize ) pvPortMallocWithAlign( xSize, RegionOne, eHeapAlign_4 )
    #define pvPortMallocAlign( xSize ) pvPortMallocWithAlign( xSize, RegionOne, eHeapAlign_32 )
    #define pvPortMallocRegion( xSize, eRegion ) pvPortMallocWithAlign( xSize, eRegion, eHeapAlign_4 )
    #define pvPortLogMalloc( xSize ) pvPortMallocWithAlign( xSize, RegionThree, eHeapAlign_4 )


    #define pvPortReallocNormal( pv, xSize ) pvPortReallocWithAlign( pv, xSize, RegionOne, eHeapAlign_4 )
    #define pvPortReallocAlign( pv, xSize ) pvPortReallocWithAlign( pv, xSize, RegionOne, eHeapAlign_32 )
  #endif

#endif


/* ==== Core Management Functions ==== */
#define INCLUDE_osCoreEnterCritical         1
#define INCLUDE_osCoreExitCritical          1
#define INCLUDE_osCoreGetState              1

/* ==== Kernel Management Functions ==== */
#define INCLUDE_osKernelInitialize          1
#define INCLUDE_osKernelGetInfo             0
#define INCLUDE_osKernelGetState            1
#define INCLUDE_osKernelIsRunningIdle       1
#define INCLUDE_osKernelStart               1
#define INCLUDE_osKernelLock                1
#define INCLUDE_osKernelUnlock              1
#define INCLUDE_osKernelRestoreLock         1
#define INCLUDE_osKernelGetTickCount        1
#define INCLUDE_osKernelGetTickFreq         1
#define INCLUDE_osKernelGetSysTimerCount    0
#define INCLUDE_osKernelGetSysTimerFreq     1

/* ==== Thread Management Functions ==== */
#define INCLUDE_osThreadNew                 1
#define INCLUDE_osThreadGetName             1
#define INCLUDE_osThreadGetId               1
#define INCLUDE_osThreadGetState            1
#define INCLUDE_osThreadGetStackSpace       1
#define INCLUDE_osThreadSetPriority         1
#define INCLUDE_osThreadGetPriority         1
#define INCLUDE_osThreadYield               1
#define INCLUDE_osThreadSuspend             1
#define INCLUDE_osThreadResume              1
#define INCLUDE_osThreadExit                1
#define INCLUDE_osThreadTerminate           1
#define INCLUDE_osThreadGetCount            1
#define INCLUDE_osThreadEnumerate           0

/* ==== Thread Flags Functions ==== */
#define INCLUDE_osThreadFlagsSet            1
#define INCLUDE_osThreadFlagsClear          1
#define INCLUDE_osThreadFlagsGet            1
#define INCLUDE_osThreadFlagsWait           1

/* ==== Generic Wait Functions ==== */
#define INCLUDE_osDelay                     1
#define INCLUDE_osDelayUntil                1

/* ==== Timer Management Functions ==== */
#define INCLUDE_osTimerNew                  1
#define INCLUDE_osTimerGetName              1
#define INCLUDE_osTimerStart                1
#define INCLUDE_osTimerRestart              1
#define INCLUDE_osTimerStop                 1
#define INCLUDE_osTimerIsRunning            1
#define INCLUDE_osTimerDelete               1

/* ==== Mutex Management Functions ==== */
#define INCLUDE_osMutexNew                  1
#define INCLUDE_osMutexAcquire              1
#define INCLUDE_osMutexRelease              1
#define INCLUDE_osMutexGetOwner             1
#define INCLUDE_osMutexDelete               1

/* ==== Semaphore Management Functions ==== */
#define INCLUDE_osSemaphoreNew              1
#define INCLUDE_osSemaphoreAcquire          1
#define INCLUDE_osSemaphoreRelease          1
#define INCLUDE_osSemaphoreGetCount         1
#define INCLUDE_osSemaphoreDelete           1

/* ==== Message Queue Management Functions ==== */
#define INCLUDE_osMessageQueueNew           1
#define INCLUDE_osMessageQueuePut           1
#define INCLUDE_osMessageQueueGet           1
#define INCLUDE_osMessageQueueGetCapacity   1
#define INCLUDE_osMessageQueueGetMsgSize    1
#define INCLUDE_osMessageQueueGetCount      1
#define INCLUDE_osMessageQueueGetSpace      1
#define INCLUDE_osMessageQueueReset         1
#define INCLUDE_osMessageQueueDelete        1

/* ==== Memory Management Functions ==== */
#define INCLUDE_osMemoryAlloc               1
#define INCLUDE_osMemoryAllocAlign          1
#define INCLUDE_osMemoryRealloc             1
#define INCLUDE_osMemoryReallocAlign        1
#define INCLUDE_osMemoryFree                1

/* ==== Sleep Management Functions ==== */
#ifndef configUSE_LOW_POWER_FLAG
  #define configUSE_LOW_POWER_FLAG 0
#endif
#define INCLUDE_osThreadSetLowPowerFlag     (1 && (configUSE_LOW_POWER_FLAG == 1 ))
#define INCLUDE_osThreadGetLowPowerFlag     (1 && (configUSE_LOW_POWER_FLAG == 1 ))
#define INCLUDE_osThreadGetLowPowerTime     (1 && (configUSE_LOW_POWER_FLAG == 1 ))
#define INCLUDE_osTimerSetLowPowerFlag      (1 && (configUSE_LOW_POWER_FLAG == 1 ))
#define INCLUDE_osTimerGetLowPowerFlag      (1 && (configUSE_LOW_POWER_FLAG == 1 ))
#define INCLUDE_osTimerGetLowPowerTime      (1 && (configUSE_LOW_POWER_FLAG == 1 ))


//  ==== OS config ====
#define OS_TICK_RATE_HZ      configTICK_RATE_HZ
#define OS_TICK_PERIOD_MS    (1000 / OS_TICK_RATE_HZ)


#endif  /* __OSAL_CONFIG_H */
