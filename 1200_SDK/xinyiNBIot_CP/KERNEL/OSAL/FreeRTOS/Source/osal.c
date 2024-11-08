#include "cmsis_os2.h"
#include "xy_memmap.h"
#include "cmsis_device.h"
#include "sys_debug.h"
#include "oss_nv.h"

#define OSAL_ASSERT(x)    Sys_Assert(x)
//#define OSAL_ASSERT(x)    (void)(x)


#define IS_IRQ_MODE()             (__get_IPSR() != 0U)
#define IS_IRQ_MASKED()           ((__get_PRIMASK() != 0U) || (__get_BASEPRI() != 0U))

osStatus_t osErrorState = osOK;


/*
  Determine if CPU executes from interrupt context or if interrupts are masked.
*/
__RAM_FUNC uint32_t IRQ_Context (void)
{
    uint32_t irq;
    BaseType_t state;

    irq = 0U;

    if (IS_IRQ_MODE())
    {
        /* Called from interrupt context */
        irq = 1U;
    }
    else
    {
        /* Get FreeRTOS scheduler state */
        state = xTaskGetSchedulerState();

        if (state != taskSCHEDULER_NOT_STARTED)
        {
            /* Scheduler was started */
            if (IS_IRQ_MASKED())
            {
                /* Interrupts are masked */
                irq = 2U;
            }
        }
    }

    /* Return context, 0: thread context, 1: IRQ context , 2: critical context */
    return (irq);
}


/* ==== Core Management Functions ==== */

uint16_t osCriticalNesting = 0;
uint16_t osSaveBasepri = 0;

#if( INCLUDE_osCoreEnterCritical == 1 )
/*
  Core enter critical
 */
__RAM_FUNC void osCoreEnterCritical (void)
{
    uint32_t saveBasepri;

    if(IRQ_Context() == 1U)
    {
        saveBasepri = taskENTER_CRITICAL_FROM_ISR();

        if(osCriticalNesting == 0)
        {
            osSaveBasepri = saveBasepri;
        }

        osCriticalNesting++;
    }
    else
    {
        taskENTER_CRITICAL();
    }
}
#endif

#if( INCLUDE_osCoreExitCritical == 1 )
/*
  Core exit critical
 */
__RAM_FUNC void osCoreExitCritical (void)
{
    if(IRQ_Context() == 1U)
    {
        if (osCriticalNesting > 0)
        {
            osCriticalNesting--;

            if (osCriticalNesting == 0)
            {
                taskEXIT_CRITICAL_FROM_ISR(osSaveBasepri);
            }
        }
    }
    else
    {
        taskEXIT_CRITICAL();
    }
}
#endif

#if( INCLUDE_osCoreGetState == 1 )
/*
  Core state get
 */
__RAM_FUNC osCoreState_t osCoreGetState (void)
{
    osCoreState_t stat;
    uint32_t irq_stat;

    if (IS_IRQ_MODE())
    {
        irq_stat = 1;
    }
    else if (IS_IRQ_MASKED())
    {
        irq_stat = 2;
    }
    else
    {
        irq_stat = 0;
    }

    switch (irq_stat)
    {
        case 0:  stat = osCoreNormal;      break;
        case 1:  stat = osCoreInInterrupt; break;
        case 2:  stat = osCoreInCritical;  break;
        default: stat = osCoreError;       break;
    }

    return stat;
}
#endif


/* ==== Kernel Management Functions ==== */

#if( INCLUDE_osKernelInitialize == 1 )
/*
  Initialize the RTOS Kernel.
*/


HeapRegion_t HeapRegions[HEAP_REGION_MAX + 1];
volatile uint8_t g_mem_debug;  /*内存跟踪调试信息开关*/


osStatus_t osKernelInitialize (void)
{
    osStatus_t stat;
    BaseType_t state;

    if(IRQ_Context() != 0U)
    {
        stat = osErrorISR;
    }
    else
    {
        state = xTaskGetSchedulerState();

        /* Initialize if scheduler not started and not initialized before */
        if(state == taskSCHEDULER_NOT_STARTED)
        {
            #if defined(USE_TRACE_EVENT_RECORDER)
                /* Initialize the trace macro debugging output channel */
                EvrFreeRTOSSetup(0U);
            #endif

            #if defined(USE_FreeRTOS_HEAP_5) && (HEAP_5_REGION_SETUP == 1)
                /* Initialize the memory regions when using heap_5 variant */
                vPortDefineHeapRegions (configHEAP_5_REGIONS);
            #endif

            #if defined(USE_FreeRTOS_HEAP_6) && (USE_FreeRTOS_HEAP_6 == 1)
            {
                extern uint32_t _Heap_Begin;
                extern uint32_t _Heap_Limit;
                uint8_t *heap_addr = (uint8_t *)&_Heap_Begin;
                size_t heap_size = (size_t)&_Heap_Limit - (size_t)&_Heap_Begin;

                const HeapRegion_t xHeapRegions[] =
                {
                    {heap_addr, heap_size},
                    {(uint8_t *)(0x60000000 + 0x8C00), (size_t)(0x7400)},
                    {NULL, 0}
                };

                vPortDefineHeapRegions(xHeapRegions);
            }
            #endif

            #if defined(USE_FreeRTOS_HEAP_7) && (USE_FreeRTOS_HEAP_7 == 1)
            {
                extern uint32_t __heap_start__;
                extern uint32_t _Heap_Limit;
                uint8_t *heap_addr;
                size_t heap_size;

                heap_addr = (uint8_t *)&__heap_start__;
                heap_size = (size_t)&_Heap_Limit - (size_t)heap_addr;
                
                HeapMallocSequnce_t xMallocSequnce =
                {
                    {
                        { RegionOne,   RegionTwo,   RegionThree},
                        { RegionTwo,   RegionOne,   RegionThree},
                        { RegionThree, RegionTwo,   RegionOne  },
                    }
                };

                // heap addr init start
                HeapRegions[RegionOne] = (HeapRegion_t) {heap_addr, heap_size};
                HeapRegions[RegionTwo] = (HeapRegion_t) {(uint8_t *)(CP_HEAP_START), (size_t)(CP_HEAP_LEN-32)};

                if(HWREG(BAK_MEM_CP_USED_APRAM_ADDR) > 0 && HWREGB(BAK_MEM_CP_USED_APRAM_SIZE) > 0)
                {
                    HeapRegions[RegionThree] = (HeapRegion_t) {(uint8_t *)HWREG(BAK_MEM_CP_USED_APRAM_ADDR), (size_t)(HWREGB(BAK_MEM_CP_USED_APRAM_SIZE))* 0x400};
                }
                else
                {
                    HeapRegions[RegionThree] = (HeapRegion_t) {(uint8_t *)(CP_HEAP_START+CP_HEAP_LEN-32), (size_t)(32)};
                }

                HeapRegions[InvalidRegion] = (HeapRegion_t) {NULL, 0};
                // heap addr init end

                g_mem_debug = READ_FAC_NV(uint8_t,softap_fac_nv.mem_debug);
                vPortDefineHeapRegions(HeapRegions, &xMallocSequnce);
                if(g_mem_debug == 1)
                	vTaskSwitchInit();
  
            }
            #endif

            stat = osOK;
        }
        else
        {
            stat = osError;
        }
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* trace debuger start */
    traceSTART();

    /* Return execution status */
    return (stat);
}
#endif

#if( INCLUDE_osKernelGetInfo == 1 )
/*
  Get RTOS Kernel Information.
*/
osStatus_t osKernelGetInfo (osVersion_t *version, char *id_buf, uint32_t id_size)
{
    if (version != NULL)
    {
        /* Version encoding is major.minor.rev: mmnnnrrrr dec */
        version->api    = KERNEL_VERSION;
        version->kernel = KERNEL_VERSION;
    }

    if ((id_buf != NULL) && (id_size != 0U))
    {
        /* Buffer for retrieving identification string is provided */
        if (id_size > sizeof(KERNEL_ID))
        {
            id_size = sizeof(KERNEL_ID);
        }
        /* Copy kernel identification string into provided buffer */
        memcpy(id_buf, KERNEL_ID, id_size);
    }

    /* Return execution status */
    return (osOK);
}
#endif

#if( INCLUDE_osKernelGetState == 1 )
/*
  Get the current RTOS Kernel state.
*/
osKernelState_t osKernelGetState (void)
{
    osKernelState_t state;

    switch(xTaskGetSchedulerState())
    {
        case taskSCHEDULER_RUNNING:
            state = osKernelRunning;
            break;

        case taskSCHEDULER_SUSPENDED:
            state = osKernelLocked;
            break;

        case taskSCHEDULER_NOT_STARTED:
        default:
            /* Not initialized */
            state = osKernelInactive;
            break;
    }

    /* Return current state */
    return (state);
}
#endif

#if( INCLUDE_osKernelIsRunningIdle == 1 )
/*
  Start the RTOS Kernel scheduler.
*/
osStatus_t osKernelIsRunningIdle (void)
{
    osStatus_t state;

    if(xTaskGetCurrentTaskHandle() == xTaskGetIdleTaskHandle())
    {
        state = osOK;
    }
    else
    {
        state = osError;
    }

    return state;
}
#endif

#if( INCLUDE_osKernelStart == 1 )
/*
  Start the RTOS Kernel scheduler.
*/
osStatus_t osKernelStart (void)
{
    osStatus_t stat;
    BaseType_t state;

    if (IRQ_Context() != 0U)
    {
        stat = osErrorISR;
    }
    else
    {
        state = xTaskGetSchedulerState();

        /* Start scheduler if initialized and not started before */
        if (state == taskSCHEDULER_NOT_STARTED)
        {
            /* Ensure SVC priority is at the reset value */
            NVIC_SetPriority (SVCall_IRQn, 0U);
            /* Start the kernel scheduler */
            vTaskStartScheduler();
            stat = osErrorRTOSStartFailed;
        }
        else
        {
            stat = osError;
        }
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return execution status */
    return (stat);
}
#endif

#if( INCLUDE_osKernelLock == 1 )
/*
  Lock the RTOS Kernel scheduler.
*/
int32_t osKernelLock (void)
{
    int32_t lock = -1;
    osStatus_t stat = osOK;

    if (IRQ_Context() == 1U)
    {
        stat = osErrorISR;
    }
    else
    {
        switch (xTaskGetSchedulerState())
        {
            case taskSCHEDULER_SUSPENDED:
                lock = 1;
                break;

            case taskSCHEDULER_RUNNING:
                vTaskSuspendAll();
                lock = 0;
                break;

            case taskSCHEDULER_NOT_STARTED:
            default:
                stat = osError;
                break;
        }
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return previous lock state */
    return (lock);
}
#endif

#if( INCLUDE_osKernelUnlock == 1 )
/*
  Unlock the RTOS Kernel scheduler.
*/
int32_t osKernelUnlock (void)
{
    int32_t lock = -1;
    osStatus_t stat = osOK;

    if (IRQ_Context() == 1U)
    {
        stat = osErrorISR;
    }
    else
    {
        switch (xTaskGetSchedulerState())
        {
            case taskSCHEDULER_SUSPENDED:
                lock = 1;
                if (xTaskResumeAll() != pdTRUE)
                {
                    if (xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED)
                    {
                        stat = osError;
                    }
                }
                break;

            case taskSCHEDULER_RUNNING:
                lock = 0;
                break;

            case taskSCHEDULER_NOT_STARTED:
            default:
                stat = osError;
                break;
        }
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return previous lock state */
    return (lock);
}
#endif

#if( INCLUDE_osKernelRestoreLock == 1 )
/*
  Restore the RTOS Kernel scheduler lock state.
*/
int32_t osKernelRestoreLock (int32_t lock)
{
    osStatus_t stat = osOK;

    if (IRQ_Context() == 1U)
    {
        stat = osErrorISR;
    }
    else
    {
        switch (xTaskGetSchedulerState())
        {
            case taskSCHEDULER_SUSPENDED:
                if (lock == 0)
                {
                    if (xTaskResumeAll() != pdTRUE)
                    {
                        if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING)
                        {
                            stat = osError;
                        }
                    }
                }
                else if (lock == 1)
                {
                }
                else
                {
                    stat = osError;
                }
                break;

            case taskSCHEDULER_RUNNING:
                if (lock == 0)
                {
                }
                else if (lock == 1)
                {
                    vTaskSuspendAll();
                }
                else
                {
                    stat = osError;
                }
                break;

            case taskSCHEDULER_NOT_STARTED:
            default:
                stat = osError;
                break;
        }
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return new lock state */
    return ((int32_t)stat);
}
#endif

#if( INCLUDE_osKernelGetTickCount == 1 )
/*
  Get the RTOS kernel tick count.
*/
__RAM_FUNC uint32_t osKernelGetTickCount (void)
{
    TickType_t ticks;

    if (IRQ_Context() != 0U)
    {
        ticks = xTaskGetTickCountFromISR();
    }
    else
    {
        ticks = xTaskGetTickCount();
    }

    /* Return kernel tick count */
    return (ticks);
}
#endif

#if( INCLUDE_osKernelGetTickFreq == 1 )
/*
  Get the RTOS kernel tick frequency.
*/
uint32_t osKernelGetTickFreq (void)
{
    /* Return frequency in hertz */
    return (configTICK_RATE_HZ);
}
#endif

#if( INCLUDE_osKernelGetSysTimerCount == 1 )
/*
  Get the RTOS kernel system timer count.
*/
uint32_t osKernelGetSysTimerCount (void)
{
    uint32_t irqmask = IS_IRQ_MASKED();
    TickType_t ticks;
    uint32_t val;

    __disable_irq();

    ticks = xTaskGetTickCount();
    val   = OS_Tick_GetCount();

    /* Update tick count and timer value when timer overflows */
    if (OS_Tick_GetOverflow() != 0U)
    {
        val = OS_Tick_GetCount();
        ticks++;
    }
    val += ticks * OS_Tick_GetInterval();

    if (irqmask == 0U)
    {
        __enable_irq();
    }

    /* Return system timer count */
    return (val);
}
#endif

#if( INCLUDE_osKernelGetSysTimerFreq == 1 )
/*
  Get the RTOS kernel system timer frequency.
*/
uint32_t osKernelGetSysTimerFreq (void)
{
    /* Return frequency in hertz */
    return (configCPU_CLOCK_HZ);
}
#endif


/* ==== Thread Management Functions ==== */

#if( INCLUDE_osThreadNew == 1 )
/*
  Create a thread and add it to Active Threads.

  Limitations:
  - The memory for control block and stack must be provided in the osThreadAttr_t
    structure in order to allocate object statically.
  - Attribute osThreadJoinable is not supported, NULL is returned if used.
*/
osThreadId_t osThreadNew (osThreadFunc_t func, void *argument, const osThreadAttr_t *attr)
{
    uint32_t stack;
    TaskHandle_t hTask = NULL;
    UBaseType_t prio;
    osStatus_t stat = osOK;

    if (IRQ_Context() == 1U)
    {
        stat = osErrorISR;
    }
    else if ((attr == NULL) || (func == NULL))
    {
        stat = osErrorParameter;
    }
    else
    {
        stack = attr->stack_size / sizeof(StackType_t);
        prio = (UBaseType_t) (attr->priority);

        if((stack < configMINIMAL_STACK_SIZE) && (stack != tskCREATE_WITH_SHARED_STACK))
        {
            stat = osErrorParameter;
        }
        else if((attr->attr_bits != osThreadDetached) || (prio >= configMAX_PRIORITIES))
        {
            stat = osErrorParameter;
        }
        else
        {
            if ((attr->cb_mem != NULL) && (attr->cb_size >= sizeof(StaticTask_t)) && (attr->stack_mem != NULL) && (attr->stack_size > 0U))
            {
                /* The memory for control block and stack is provided, use static object */
                #if (configSUPPORT_STATIC_ALLOCATION == 1)
                {
                    hTask = xTaskCreateStatic ((TaskFunction_t)func, attr->name, stack, argument, prio, (StackType_t  *)attr->stack_mem, (StaticTask_t *)attr->cb_mem);
                }
                #endif

                if (hTask == NULL)
                {
                    stat = osErrorResource;
                }
            }
            else if ((attr->cb_mem == NULL) && (attr->cb_size == 0U) && (attr->stack_mem == NULL))
            {
                /* Control block and stack memory will be allocated from the dynamic pool */
                #if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
                {
                    if (xTaskCreate ((TaskFunction_t)func, attr->name, (uint16_t)stack, argument, prio, &hTask) != pdPASS)
                    {
                        hTask = NULL;
                    }
                }
                #endif

                if (hTask == NULL)
                {
                    stat = osErrorResource;
                }
            }
            else
            {
                stat = osErrorParameter;
            }

            if (hTask != NULL)
            {
                #if (configUSE_LOW_POWER_FLAG == 1)
                {
                    vTaskSetLowPowerFlag(hTask, ePermitSleepStatistics);
                }
                #endif
            }
        }
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return thread ID */
    return ((osThreadId_t)hTask);
}
#endif

#if( INCLUDE_osThreadGetName == 1 )
/*
  Get name of a thread.
*/
__RAM_FUNC  const char *osThreadGetName (osThreadId_t thread_id)
{
    TaskHandle_t hTask = (TaskHandle_t)thread_id;
    const char *name;

    name = pcTaskGetName (hTask);

    /* Return name as null-terminated string */
    return (name);
}
#endif

#if( INCLUDE_osThreadGetId == 1 )
/*
  Return the thread ID of the current running thread.
*/
__RAM_FUNC osThreadId_t osThreadGetId (void)
{
    osThreadId_t id;

    osStatus_t stat = osOK;

    id = (osThreadId_t)xTaskGetCurrentTaskHandle();

    if (id == NULL)
    {
        stat = osErrorResource;
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return thread ID */
    return (id);
}
#endif

#if( INCLUDE_osThreadGetState == 1 )
/*
  Get current thread state of a thread.
*/
osThreadState_t osThreadGetState (osThreadId_t thread_id)
{
    TaskHandle_t hTask = (TaskHandle_t)thread_id;
    osThreadState_t state;

    if ((IRQ_Context() != 0U) || (hTask == NULL))
    {
        state = osThreadError;
    }
    else
    {
        switch (eTaskGetState (hTask))
        {
            case eRunning:   state = osThreadRunning;    break;
            case eReady:     state = osThreadReady;      break;
            case eBlocked:
            case eSuspended: state = osThreadBlocked;    break;
            case eDeleted:   state = osThreadTerminated; break;
            case eInvalid:
            default:         state = osThreadError;      break;
        }
    }

    /* Return current thread state */
    return (state);
}
#endif

#if( INCLUDE_osThreadGetStackSpace == 1 )
/*
  Get available stack space of a thread based on stack watermark recording during execution.
*/
uint32_t osThreadGetStackSpace (osThreadId_t thread_id)
{
    TaskHandle_t hTask = (TaskHandle_t)thread_id;
    uint32_t sz;

    if ((IRQ_Context() != 0U) || (hTask == NULL))
    {
        sz = 0U;
    }
    else
    {
        sz = (uint32_t)(uxTaskGetStackHighWaterMark(hTask) * sizeof(StackType_t));
    }

    /* Return remaining stack space in bytes */
    return (sz);
}
#endif

#if( INCLUDE_osThreadSetPriority == 1 )
/*
  Change priority of a thread.
*/
osStatus_t osThreadSetPriority (osThreadId_t thread_id, osPriority_t priority)
{
    TaskHandle_t hTask = (TaskHandle_t)thread_id;
    osStatus_t stat;

    if (IRQ_Context() != 0U)
    {
        stat = osErrorISR;
    }
    else if ((hTask == NULL) || (priority < osPriorityIdle) || (priority > osPriorityISR))
    {
        stat = osErrorParameter;
    }
    else
    {
        stat = osOK;
        vTaskPrioritySet (hTask, (UBaseType_t)priority);
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return execution status */
    return (stat);
}
#endif

#if( INCLUDE_osThreadGetPriority == 1 )
/*
  Get current priority of a thread.
*/
osPriority_t osThreadGetPriority (osThreadId_t thread_id)
{
    TaskHandle_t hTask = (TaskHandle_t)thread_id;
    osPriority_t prio = osPriorityError;
    osStatus_t stat = osOK;

    if (IRQ_Context() != 0U)
    {
        stat = osErrorISR;
    }
    else
    {
        prio = (osPriority_t)((int32_t)uxTaskPriorityGet (hTask));

        if ((prio < 0) || (prio >= configMAX_PRIORITIES))
        {
            stat = osErrorResource;
        }
    }
    
    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return current thread priority */
    return (prio);
}
#endif

#if( INCLUDE_osThreadYield == 1 )
/*
  Pass control to next thread that is in state READY.
*/
osStatus_t osThreadYield (void)
{
    osStatus_t stat;

    if (IRQ_Context() != 0U)
    {
        stat = osErrorISR;
    }
    else
    {
        stat = osOK;
        taskYIELD();
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return execution status */
    return (stat);
}
#endif


#if( INCLUDE_osThreadSuspend == 1 )
/*
  Suspend execution of a thread.
*/
osStatus_t osThreadSuspend (osThreadId_t thread_id)
{
    TaskHandle_t hTask = (TaskHandle_t)thread_id;
    osStatus_t stat;

    if (IRQ_Context() != 0U)
    {
        stat = osErrorISR;
    }
    else if (hTask == NULL)
    {
        stat = osErrorParameter;
    }
    else
    {
        stat = osOK;
        vTaskSuspend (hTask);
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return execution status */
    return (stat);
}
#endif

#if( INCLUDE_osThreadResume == 1 )
/*
  Resume execution of a thread.
*/
osStatus_t osThreadResume (osThreadId_t thread_id)
{
    TaskHandle_t hTask = (TaskHandle_t)thread_id;
    osStatus_t stat;

    if (IRQ_Context() != 0U)
    {
        stat = osErrorISR;
    }
    else if (hTask == NULL)
    {
        stat = osErrorParameter;
    }
    else
    {
        stat = osOK;
        vTaskResume (hTask);
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return execution status */
    return (stat);
}
#endif /* (configUSE_OS2_THREAD_SUSPEND_RESUME == 1) */

#if( INCLUDE_osThreadExit == 1 )
/*
  Terminate execution of current running thread.
*/
void osThreadExit (void)
{
    vTaskDelete (NULL);
    OSAL_ASSERT (0);
    for (;;);
}
#endif

#if( INCLUDE_osThreadTerminate == 1 )
/*
  Terminate execution of a thread.
*/
osStatus_t osThreadTerminate (osThreadId_t thread_id)
{
    TaskHandle_t hTask = (TaskHandle_t)thread_id;
    osStatus_t stat;
    eTaskState tstate;

    if (IRQ_Context() != 0U)
    {
        stat = osErrorISR;
    }
    else if (hTask == NULL)
    {
        stat = osErrorParameter;
    }
    else
    {
        tstate = eTaskGetState (hTask);

        if (tstate != eDeleted)
        {
            stat = osOK;
            vTaskDelete (hTask);
        }
        else
        {
            stat = osErrorResource;
        }
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return execution status */
    return (stat);
}
#endif

#if( INCLUDE_osThreadGetCount == 1 )
/*
  Get number of active threads.
*/
uint32_t osThreadGetCount (void)
{
    uint32_t count = 0U;
    osStatus_t stat;

    if (IRQ_Context() != 0U)
    {
        stat = osErrorISR;
    }
    else
    {
        stat = osOK;
        count = uxTaskGetNumberOfTasks();
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return number of active threads */
    return (count);
}
#endif

#if( INCLUDE_osThreadEnumerate == 1 )
/*
  Enumerate active threads.
*/
uint32_t osThreadEnumerate (osThreadId_t *thread_array, uint32_t array_items)
{
    uint32_t i, count;
    TaskStatus_t *task;

    if ((IRQ_Context() != 0U) || (thread_array == NULL) || (array_items == 0U))
    {
        count = 0U;
    }
    else
    {
        vTaskSuspendAll();

        /* Allocate memory on heap to temporarily store TaskStatus_t information */
        count = uxTaskGetNumberOfTasks();
        task  = pvPortMalloc (count * sizeof(TaskStatus_t));

        if (task != NULL)
        {
            /* Retrieve task status information */
            count = uxTaskGetSystemState (task, count, NULL);

            /* Copy handles from task status array into provided thread array */
            for (i = 0U; (i < count) && (i < array_items); i++)
            {
                thread_array[i] = (osThreadId_t)task[i].xHandle;
            }

            count = i;
        }

        (void)xTaskResumeAll();

        vPortFree (task);
    }

    /* Return number of enumerated threads */
    return (count);
}
#endif


/* ==== Thread Flags Functions ==== */

#if( INCLUDE_osThreadFlagsSet == 1 )
/*
  Set the specified Thread Flags of a thread.
*/
__RAM_FUNC uint32_t osThreadFlagsSet (osThreadId_t thread_id, uint32_t flags)
{
    TaskHandle_t hTask = (TaskHandle_t)thread_id;
    StaticTask_t *static_task = (StaticTask_t *)thread_id;
    uint32_t rflags;
    BaseType_t yield;
    uint32_t isrm;
    osStatus_t stat;

    if ((hTask == NULL) || ((flags & THREAD_FLAGS_INVALID_BITS) != 0U))
    {
        stat = osErrorParameter;
        rflags = (uint32_t)osErrorParameter;
    }
    else
    {
        stat = osOK;

        if (IRQ_Context() != 0U)
        {
            yield = pdFALSE;

            (void)xTaskNotifyAndQueryFromISR (hTask, flags, eSetBits, NULL, &yield);
            
            isrm = taskENTER_CRITICAL_FROM_ISR();

            /* rflags = pxCurrentTCB->ulNotifiedValue; */
            rflags = static_task->ulDummy18;

            taskEXIT_CRITICAL_FROM_ISR(isrm);

            portYIELD_FROM_ISR (yield);
        }
        else
        {
            /* Notify and get the flags value before setting. */
            (void)xTaskNotifyAndQuery (hTask, flags, eSetBits, NULL);
            
            taskENTER_CRITICAL();

            /* rflags = pxCurrentTCB->ulNotifiedValue; */
            rflags = static_task->ulDummy18;

            taskEXIT_CRITICAL();
        }
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return flags after setting */
    return (rflags);
}
#endif

#if( INCLUDE_osThreadFlagsClear == 1 )
/*
  Clear the specified Thread Flags of current running thread.
*/
uint32_t osThreadFlagsClear (uint32_t flags)
{
    uint32_t rflags;
    StaticTask_t *static_task;
    osStatus_t stat;

    if (IRQ_Context() != 0U)
    {
        stat = osErrorISR;
        rflags = (uint32_t)osErrorISR;
    }
    else if ((flags & THREAD_FLAGS_INVALID_BITS) != 0U)
    {
        stat = osErrorParameter;
        rflags = (uint32_t)osErrorParameter;
    }
    else
    {
        stat = osOK;
        static_task = (StaticTask_t *) xTaskGetCurrentTaskHandle();

        taskENTER_CRITICAL();

        /* rflags = pxCurrentTCB->ulNotifiedValue; */
        rflags = static_task->ulDummy18;
        /* pxCurrentTCB->ulNotifiedValue = rflags & (~flags); */
        static_task->ulDummy18 = rflags & (~flags);

        taskEXIT_CRITICAL();
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return flags before clearing */
    return (rflags);
}
#endif

#if( INCLUDE_osThreadFlagsGet == 1 )
/*
  Get the current Thread Flags of current running thread.
*/
uint32_t osThreadFlagsGet (void)
{
    StaticTask_t *static_task;
    uint32_t rflags;
    osStatus_t stat;

    if (IRQ_Context() != 0U)
    {
        stat = osErrorISR;
        rflags = (uint32_t)osErrorISR;
    }
    else
    {
		stat = osOK;
        static_task = (StaticTask_t *) xTaskGetCurrentTaskHandle();

        taskENTER_CRITICAL();

        /* rflags = pxCurrentTCB->ulNotifiedValue; */
        rflags = static_task->ulDummy18;
        
        taskEXIT_CRITICAL();
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return current flags */
    return (rflags);
}
#endif

#if( INCLUDE_osThreadFlagsWait == 1 )
/*
  Wait for one or more Thread Flags of the current running thread to become signaled.
*/
uint32_t osThreadFlagsWait (uint32_t flags, uint32_t options, uint32_t timeout)
{
    uint32_t clear, rflags;
    TickType_t t_start, t_spend, tout;
    BaseType_t rval;
    TaskHandle_t hTask;
    StaticTask_t *static_task;
    BaseType_t can_get_now;
    osStatus_t stat;

    if (IRQ_Context() != 0U)
    {
        stat = osErrorISR;
    }
    else if ((flags & THREAD_FLAGS_INVALID_BITS) != 0U)
    {
        stat = osErrorParameter;
    }
    else
    {
        hTask = xTaskGetCurrentTaskHandle();
        static_task = (StaticTask_t *) hTask;

        taskENTER_CRITICAL();

        /* pxCurrentTCB->ucNotifyState != taskNOTIFICATION_RECEIVED */
        if (static_task->ucDummy19 != 2)
        {
            can_get_now = pdFALSE;
        }
        else
        {
            can_get_now = pdTRUE;
        }

        taskEXIT_CRITICAL();

        if((xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) && (can_get_now == pdFALSE) && (timeout != osNoWait))
        {
            stat = osErrorKernelBlocked;
        }
        else if ((hTask == xTaskGetIdleTaskHandle()) && (can_get_now == pdFALSE) && (timeout != osNoWait))
        {
            stat = osErrorIdleBlocked;
        }
        else if ((hTask == xTimerGetTimerDaemonTaskHandle()) && (can_get_now == pdFALSE) && (timeout != osNoWait))
        {
            stat = osErrorTimerBlocked;
        }
        else
        {
            stat = osOK;

            if ((options & osFlagsNoClear) == osFlagsNoClear)
            {
                clear = 0U;
            }
            else
            {
                clear = flags;
            }

            tout = timeout;
            t_start = xTaskGetTickCount();

            do
            {
                rval = xTaskNotifyWait (0, clear, &rflags, tout);

                if (rval == pdPASS)
                {
                    if ((options & osFlagsWaitAll) == osFlagsWaitAll)
                    {
                        if ((flags & rflags) == flags)
                        {
                            break;
                        }
                    }
                    else
                    {
                        if ((flags & rflags) != 0)
                        {
                            break;
                        }
                    }

                    /* Update timeout */
                    t_spend = xTaskGetTickCount() - t_start;

                    if (tout == osWaitForever)
                    {

                    }
                    else if (t_spend > tout)
                    {
                        tout  = 0;
                    }
                    else
                    {
                        tout -= t_spend;
                    }
                }
                else
                {
                    if (timeout == 0)
                    {
                        stat = osErrorResource;
                    }
                    else
                    {
                        stat = osErrorTimeout;
                    }
                }
            }
            while (rval != pdFAIL);
        }
    }

    if (stat != osOK)
    {
        rflags = (uint32_t)stat;
    }

    osErrorState = stat;

    OSAL_ASSERT ((stat == osOK) || (osErrorResource) || (stat == osErrorTimeout));

    /* Return flags before clearing */
    return (rflags);
}
#endif


/* ==== Generic Wait Functions ==== */

#if( INCLUDE_osDelay == 1 )
/*
  Wait for Timeout (Time Delay).
*/
osStatus_t osDelay (uint32_t ticks)
{
    osStatus_t stat;



    if (IRQ_Context() != 0U)
    {
        stat = osErrorISR;
    }
    else if (ticks == 0)
    {
        stat = osErrorParameter;
    }
    else if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
    {
        stat = osErrorKernelNotStarted;
    }
    else if (xTaskGetCurrentTaskHandle() == xTaskGetIdleTaskHandle())
    {
        stat = osErrorIdleBlocked;
    }
    else if (xTaskGetCurrentTaskHandle() == xTimerGetTimerDaemonTaskHandle())
    {
        stat = osErrorTimerBlocked;
    }
    else
    {
        stat = osOK;

        if (ticks != 0U)
        {
            vTaskDelay(ticks);
        }
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return execution status */
    return (stat);
}
#endif

#if( INCLUDE_osDelayUntil == 1 )
/*
  Wait until specified time.
*/
osStatus_t osDelayUntil (uint32_t ticks)
{
    TickType_t tcnt, delay;
    osStatus_t stat;

    if (IRQ_Context() != 0U)
    {
        stat = osErrorISR;
    }
    else if (ticks == 0)
    {
        stat = osErrorParameter;
    }
    else if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
    {
        stat = osErrorKernelNotStarted;
    }
    else if (xTaskGetCurrentTaskHandle() == xTaskGetIdleTaskHandle())
    {
        stat = osErrorIdleBlocked;
    }
    else if (xTaskGetCurrentTaskHandle() == xTimerGetTimerDaemonTaskHandle())
    {
        stat = osErrorTimerBlocked;
    }
    else
    {
        stat = osOK;
        tcnt = xTaskGetTickCount();

        /* Determine remaining number of ticks to delay */
        delay = (TickType_t)ticks - tcnt;

        /* Check if target tick has not expired */
        if((delay != 0U) && (0 == (delay >> (8 * sizeof(TickType_t) - 1))))
        {
            vTaskDelayUntil (&tcnt, delay);
        }
        else
        {
            /* No delay or already expired */
            stat = osErrorParameter;
        }
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return execution status */
    return (stat);
}
#endif


/* ==== Timer Management Functions ==== */

#if( INCLUDE_osTimerNew == 1 )
/*
  Create and Initialize a timer.
*/
osTimerId_t osTimerNew (osTimerFunc_t func, osTimerType_t type, void *argument, const osTimerAttr_t *attr)
{
    const char *name;
    TimerHandle_t hTimer = NULL;
    UBaseType_t reload;
    int32_t mem;
    osStatus_t stat = osOK;

    (void) argument;

    if (IRQ_Context() == 1U)
    {
        stat = osErrorISR;
    }
    else if (func == NULL)
    {
        stat = osErrorParameter;
    }
    else
    {
        if (attr != NULL)
        {
            if ((attr->cb_mem != NULL) && (attr->cb_size >= sizeof(StaticTimer_t)))
            {
                /* The memory for control block is provided, use static object */
                mem = 1;
            }
            else if ((attr->cb_mem == NULL) && (attr->cb_size == 0U))
            {
                /* Control block will be allocated from the dynamic pool */
                mem = 0;
            }
            else
            {
                mem = -1;
            }

            name = attr->name;
        }
        else
        {
            mem = 0;

            name = NULL;
        }

        reload = (type == osTimerOnce) ? pdFALSE : pdTRUE;

        if (mem == 1)
        {
            /* The memory for control block is provided, use static object */
            #if (configSUPPORT_STATIC_ALLOCATION == 1)
            {
                hTimer = xTimerCreateStatic (name, 1, reload, argument, (TimerCallbackFunction_t) func, (StaticTimer_t *)attr->cb_mem);
            }
            #endif

            if (hTimer == NULL)
            {
                stat = osErrorResource;
            }
        }
        else if (mem == 0)
        {
            /* Control block will be allocated from the dynamic pool */
            #if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
            {
                hTimer = xTimerCreate (name, 1, reload, argument, (TimerCallbackFunction_t)func);
            }
            #endif

            if (hTimer == NULL)
            {
                stat = osErrorResource;
            }
        }
        else
        {
            stat = osErrorParameter;
        }

        #if (configUSE_LOW_POWER_FLAG == 1)
        {
            if(hTimer != NULL)
            {
                xTimerSetLowPowerFlag(hTimer, ePermitSleepStatistics);
            }
        }
        #endif
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return timer ID */
    return ((osTimerId_t)hTimer);
}
#endif

#if( INCLUDE_osTimerGetName == 1 )
/*
  Get name of a timer.
*/
const char *osTimerGetName (osTimerId_t timer_id)
{
    TimerHandle_t hTimer = (TimerHandle_t)timer_id;
    const char *p;

    if ((IRQ_Context() != 0U) || (hTimer == NULL))
    {
        p = NULL;
    }
    else
    {
        p = pcTimerGetName (hTimer);
    }

    /* Return name as null-terminated string */
    return (p);
}
#endif

#if( INCLUDE_osTimerStart == 1 )
/*
  Start or restart a timer.
*/
osStatus_t osTimerStart (osTimerId_t timer_id, uint32_t ticks)
{
    TimerHandle_t hTimer = (TimerHandle_t)timer_id;
    osStatus_t stat;

    if (IRQ_Context() == 1U)
    {
        stat = osErrorISR;
    }
    else if (hTimer == NULL)
    {
        stat = osErrorParameter;
    }
    else
    {
        if (xTimerChangePeriod (hTimer, ticks, 0) == pdPASS)
        {
            stat = osOK;
        }
        else
        {
            stat = osErrorResource;
        }
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return execution status */
    return (stat);
}
#endif

#if( INCLUDE_osTimerRestart == 1 )
/*
  Restart a timer.
*/
osStatus_t osTimerRestart (osTimerId_t timer_id)
{
    TimerHandle_t hTimer = (TimerHandle_t)timer_id;
    osStatus_t stat;

    if (IRQ_Context() != 0U)
    {
        stat = osErrorISR;
    }
    else if (hTimer == NULL)
    {
        stat = osErrorParameter;
    }
    else
    {
        if(xTimerStart(hTimer, 0) == pdPASS)
        {
            stat = osOK;
        }
        else
        {
            stat = osErrorResource;
        }
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return execution status */
    return (stat);
}
#endif

#if( INCLUDE_osTimerStop == 1 )
/*
  Stop a timer.
*/
osStatus_t osTimerStop (osTimerId_t timer_id)
{
    TimerHandle_t hTimer = (TimerHandle_t)timer_id;
    osStatus_t stat;

    if (IRQ_Context() != 0U)
    {
        stat = osErrorISR;
    }
    else if (hTimer == NULL)
    {
        stat = osErrorParameter;
    }
    else
    {
        if (xTimerIsTimerActive (hTimer) == pdFALSE)
        {
            stat = osErrorResource;
        }
        else
        {
            if (xTimerStop (hTimer, 0) == pdPASS)
            {
                stat = osOK;
            }
            else
            {
                stat = osError;
            }
        }
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK || stat == osErrorResource);

    /* Return execution status */
    return (stat);
}
#endif

#if( INCLUDE_osTimerIsRunning == 1 )
/*
  Check if a timer is running.
*/
uint32_t osTimerIsRunning (osTimerId_t timer_id)
{
    TimerHandle_t hTimer = (TimerHandle_t)timer_id;
    uint32_t running;

    if ((IRQ_Context() != 0U) || (hTimer == NULL))
    {
        running = 0U;
    }
    else
    {
        running = (uint32_t)xTimerIsTimerActive (hTimer);
    }

    /* Return 0: not running, 1: running */
    return (running);
}
#endif

#if( INCLUDE_osTimerDelete == 1 )
/*
  Delete a timer.
*/
osStatus_t osTimerDelete (osTimerId_t timer_id)
{
    TimerHandle_t hTimer = (TimerHandle_t)timer_id;
    osStatus_t stat;

    if (IRQ_Context() != 0U)
    {
        stat = osErrorISR;
    }
    else if (hTimer == NULL)
    {
        stat = osErrorParameter;
    }
    else
    {
        if (xTimerDelete (hTimer, 0) == pdPASS)
        {
            stat = osOK;
        }
        else
        {
            stat = osErrorResource;
        }
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return execution status */
    return (stat);
}
#endif /* (configUSE_OS2_TIMER == 1) */


/* ==== Mutex Management Functions ==== */

#if( INCLUDE_osMutexNew == 1 )
/*
  Create and Initialize a Mutex object.

  Limitations:
  - Priority inherit protocol is used by default, osMutexPrioInherit attribute is ignored.
  - Robust mutex is not supported, NULL is returned if used.
*/
osMutexId_t osMutexNew (const osMutexAttr_t *attr) 
{
    SemaphoreHandle_t hMutex;
    uint32_t type;
    uint32_t isRecursive;
    int32_t  mem;
    osStatus_t stat = osOK;

    hMutex = NULL;

    if (IRQ_Context() == 1U)
    {
        stat = osErrorISR;
    }
    else
    {
        if (attr != NULL)
        {
            type = attr->attr_bits;
        }
        else
        {
            type = 0U;
        }

        if ((type & osMutexRecursive) == osMutexRecursive)
        {
            isRecursive = 1U;
        }
        else
        {
            isRecursive = 0U;
        }

        if ((type & osMutexRobust) != osMutexRobust)
        {
            if (attr != NULL)
            {
                if ((attr->cb_mem != NULL) && (attr->cb_size >= sizeof(StaticSemaphore_t)))
                {
                    /* The memory for control block is provided, use static object */
                    mem = 1;
                }
                else if ((attr->cb_mem == NULL) && (attr->cb_size == 0U))
                {
                    /* Control block will be allocated from the dynamic pool */
                    mem = 0;
                }
                else
                {
                    mem = -1;
                }
            }
            else
            {
                mem = 0;
            }

            if (mem == 1)
            {
                /* The memory for control block is provided, use static object */
                #if (configSUPPORT_STATIC_ALLOCATION == 1)
                {
                    if (isRecursive != 0U)
                    {
                        #if (configUSE_RECURSIVE_MUTEXES == 1)
                        {
                            hMutex = xSemaphoreCreateRecursiveMutexStatic (attr->cb_mem);
                        }
                        #endif
                    }
                    else
                    {
                        hMutex = xSemaphoreCreateMutexStatic (attr->cb_mem);
                    }
                }
                #endif

                if (hMutex == NULL)
                {
                    stat = osErrorResource;
                }
            }
            else if (mem == 0)
            {
                #if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
                {
                    if (isRecursive != 0U)
                    {
                        #if (configUSE_RECURSIVE_MUTEXES == 1)
                        {
                            hMutex = xSemaphoreCreateRecursiveMutex ();
                        }
                        #endif
                    }
                    else
                    {
                        hMutex = xSemaphoreCreateMutex ();
                    }
                }
                #endif

                if (hMutex == NULL)
                {
                    stat = osErrorResource;
                }
            }
            else
            {
                stat = osErrorParameter;
            }

            #if (configQUEUE_REGISTRY_SIZE > 0)
            {
                const char *name;

                if (attr != NULL)
                {
                    name = attr->name;
                }
                else
                {
                    name = NULL;
                }

                if (hMutex != NULL)
                {
                    vQueueAddToRegistry (hMutex, name);
                }
            }
            #endif

            if ((hMutex != NULL) && (isRecursive != 0U))
            {
                /* Set LSB as 'recursive mutex flag' */
                hMutex = (SemaphoreHandle_t)((uint32_t)hMutex | 1U);
            }
        }
        else
        {
            stat = osErrorParameter;
        }
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return mutex ID */
    return ((osMutexId_t)hMutex);
}
#endif

#if( INCLUDE_osMutexAcquire == 1 )
/*
  Acquire a Mutex or timeout if it is locked.
*/
osStatus_t osMutexAcquire (osMutexId_t mutex_id, uint32_t timeout)
{
    SemaphoreHandle_t hMutex;
    osStatus_t stat;
    uint32_t isRecursive;
    TaskHandle_t hMutexHolder;
    TaskHandle_t xCurentThread;
    uint32_t core_stat;
    BaseType_t kernel_stat;
    BaseType_t can_take_now;

    hMutex = (SemaphoreHandle_t)((uint32_t)mutex_id & ~1U);

    /* Extract recursive mutex flag */
    isRecursive = (uint32_t)mutex_id & 1U;

    stat = osOK;

    /* Get core and kernel state */
    core_stat = IRQ_Context();
    kernel_stat = xTaskGetSchedulerState();

    if (core_stat == 1U)
    {
        stat = osErrorISR;
    }
    else if (hMutex == NULL)
    {
        stat = osErrorParameter;
    }
    // else if (kernel_stat == taskSCHEDULER_NOT_STARTED)
    // {
    //     stat = osErrorKernelNotStarted;
    // }
    else
    {
        hMutexHolder = xQueueGetMutexHolder (hMutex);
        xCurentThread = xTaskGetCurrentTaskHandle();

        if (isRecursive != 0U)
        {
            /* Current acquire can take mutex or not */
            if ((hMutexHolder != xCurentThread) && (hMutexHolder != NULL))
            {
                can_take_now = pdFALSE;
            }
            else
            {
                can_take_now = pdTRUE;
            }
        }
        else
        {
            /* Current acquire can take mutex or not */
            if (hMutexHolder != NULL)
            {
                can_take_now = pdFALSE;
            }
            else
            {
                can_take_now = pdTRUE;
            }
        }

        if((core_stat != 0U) && (can_take_now == pdFALSE) && (timeout != osNoWait))
        {
            stat = osErrorIsrBlocked;
        }
        else if((kernel_stat != taskSCHEDULER_RUNNING) && (can_take_now == pdFALSE) && (timeout != osNoWait))
        {
            stat = osErrorKernelBlocked;
        }
        else if ((xCurentThread == xTaskGetIdleTaskHandle()) && (can_take_now == pdFALSE) && (timeout != osNoWait))
        {
            stat = osErrorIdleBlocked;
        }
        // else if ((xCurentThread == xTimerGetTimerDaemonTaskHandle()) && (can_take_now == pdFALSE) && (timeout != osNoWait))
        // {
        //     stat = osErrorTimerBlocked;
        // }
        else
        {
            if (isRecursive != 0U)
            {
                if (xSemaphoreTakeRecursive (hMutex, timeout) != pdPASS)
                {
                    if (timeout != 0U)
                    {
                        stat = osErrorTimeout;
                    }
                    else
                    {
                        stat = osErrorResource;
                    }
                }
            }
            else
            {
                if (xSemaphoreTake (hMutex, timeout) != pdPASS)
                {
                    if (timeout != 0U)
                    {
                        stat = osErrorTimeout;
                    }
                    else
                    {
                        stat = osErrorResource;
                    }
                }
            }
        }
    }
    
    osErrorState = stat;

    OSAL_ASSERT ((stat == osOK) || (stat == osErrorTimeout) || (stat == osErrorResource));

    /* Return execution status */
    return (stat);
}
#endif

#if( INCLUDE_osMutexRelease == 1 )
/*
  Release a Mutex that was acquired by osMutexAcquire.
*/
osStatus_t osMutexRelease (osMutexId_t mutex_id)
{
    SemaphoreHandle_t hMutex;
    osStatus_t stat;
    uint32_t isRecursive;

    hMutex = (SemaphoreHandle_t)((uint32_t)mutex_id & ~1U);

    /* Extract recursive mutex flag */
    isRecursive = (uint32_t)mutex_id & 1U;

    stat = osOK;

    if (IRQ_Context() == 1U)
    {
        stat = osErrorISR;
    }
    else if (hMutex == NULL)
    {
        stat = osErrorParameter;
    }
    // else if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
    // {
    //     stat = osErrorKernelNotStarted;
    // }
    else
    {
        if (isRecursive != 0U)
        {
            #if (configUSE_RECURSIVE_MUTEXES == 1)
            {
                if (xSemaphoreGiveRecursive (hMutex) != pdPASS)
                {
                    stat = osErrorResource;
                }
            }
            #endif
        }
        else
        {
            if (xSemaphoreGive (hMutex) != pdPASS)
            {
                stat = osErrorResource;
            }
        }
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return execution status */
    return (stat);
}
#endif

#if( INCLUDE_osMutexGetOwner == 1 )
/*
  Get Thread which owns a Mutex object.
*/
osThreadId_t osMutexGetOwner (osMutexId_t mutex_id)
{
    SemaphoreHandle_t hMutex;
    osThreadId_t owner;

    hMutex = (SemaphoreHandle_t)((uint32_t)mutex_id & ~1U);

    if ((IRQ_Context() != 0U) || (hMutex == NULL))
    {
        owner = NULL;
    }
    else
    {
        owner = (osThreadId_t)xSemaphoreGetMutexHolder (hMutex);
    }

    /* Return owner thread ID */
    return (owner);
}
#endif

#if( INCLUDE_osMutexDelete == 1 )
/*
  Delete a Mutex object.
*/
osStatus_t osMutexDelete (osMutexId_t mutex_id)
{
    osStatus_t stat;

    SemaphoreHandle_t hMutex;

    hMutex = (SemaphoreHandle_t)((uint32_t)mutex_id & ~1U);

    if (IRQ_Context() != 0U)
    {
        stat = osErrorISR;
    }
    else if (hMutex == NULL)
    {
        stat = osErrorParameter;
    }
    else
    {
        #if (configQUEUE_REGISTRY_SIZE > 0)
        {
            vQueueUnregisterQueue (hMutex);
        }
        #endif

        stat = osOK;

        vSemaphoreDelete (hMutex);
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return execution status */
    return (stat);
}
#endif /* (configUSE_OS2_MUTEX == 1) */


/* ==== Semaphore Management Functions ==== */

#if( INCLUDE_osSemaphoreNew == 1 )
/*
  Create and Initialize a Semaphore object.
*/
osSemaphoreId_t osSemaphoreNew (uint32_t max_count, uint32_t initial_count, const osSemaphoreAttr_t *attr)
{
    SemaphoreHandle_t hSemaphore = NULL;
    int32_t mem;
    osStatus_t stat = osOK;

    if (IRQ_Context() == 1U)
    {
        stat = osErrorISR;
    }
    else if ((max_count == 0) || (max_count < initial_count))
    {
        stat = osErrorParameter;
    }
    else
    {
        if (attr != NULL)
        {
            if ((attr->cb_mem != NULL) && (attr->cb_size >= sizeof(StaticSemaphore_t)))
            {
                /* The memory for control block is provided, use static object */
                mem = 1;
            }
            else if ((attr->cb_mem == NULL) && (attr->cb_size == 0U))
            {
                /* Control block will be allocated from the dynamic pool */
                mem = 0;
            }
            else
            {
                mem = -1;
            }
        }
        else
        {
            mem = 0;
        }

        if (mem == 1)
        {
            /* The memory for control block is provided, use static object */
            #if (configSUPPORT_STATIC_ALLOCATION == 1)
            {
                if (max_count == 1U)
                {
                    hSemaphore = xSemaphoreCreateBinaryStatic ((StaticSemaphore_t *)attr->cb_mem);

                    if ((hSemaphore != NULL) && (initial_count != 0U))
                    {
                        if (xSemaphoreGive (hSemaphore) != pdPASS)
                        {
                            vSemaphoreDelete (hSemaphore);
                            hSemaphore = NULL;
                        }
                    }
                }
                else
                {
                    hSemaphore = xSemaphoreCreateCountingStatic (max_count, initial_count, (StaticSemaphore_t *)attr->cb_mem);
                }
            }
            #endif

            if (hSemaphore == NULL)
            {
                stat = osErrorResource;
            }
        }
        else if (mem == 0)
        {
            /* Control block will be allocated from the dynamic pool */
            #if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
            {
                if (max_count == 1U)
                {
                    hSemaphore = xSemaphoreCreateBinary();

                    if ((hSemaphore != NULL) && (initial_count != 0U))
                    {
                        if (xSemaphoreGive (hSemaphore) != pdPASS)
                        {
                            vSemaphoreDelete (hSemaphore);
                            hSemaphore = NULL;
                        }
                    }
                }
                else
                {
                    hSemaphore = xSemaphoreCreateCounting (max_count, initial_count);
                }
            }
            #endif

            if (hSemaphore == NULL)
            {
                stat = osErrorResource;
            }
        }
        else
        {
            stat = osErrorParameter;
        }

        #if (configQUEUE_REGISTRY_SIZE > 0)
        {
            const char *name;

            if (attr != NULL)
            {
                name = attr->name;
            }
            else
            {
                name = NULL;
            }

            if (hSemaphore != NULL)
            {
                vQueueAddToRegistry (hSemaphore, name);
            }
        }
        #endif
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return semaphore ID */
    return ((osSemaphoreId_t)hSemaphore);
}
#endif

#if( INCLUDE_osSemaphoreAcquire == 1 )
/*
  Acquire a Semaphore token or timeout if no tokens are available.
*/
osStatus_t osSemaphoreAcquire (osSemaphoreId_t semaphore_id, uint32_t timeout)
{
    SemaphoreHandle_t hSemaphore = (SemaphoreHandle_t)semaphore_id;
    osStatus_t stat;
    BaseType_t yield;
    TaskHandle_t xCurentThread;
    uint32_t core_stat;
    uint32_t count;
    BaseType_t kernel_stat;

    stat = osOK;

    if (hSemaphore == NULL)
    {
        stat = osErrorParameter;
    }
    else
    {
        xCurentThread = xTaskGetCurrentTaskHandle();
        kernel_stat = xTaskGetSchedulerState();
        core_stat = IRQ_Context();

        if (core_stat != 0U)
        {
            count = (uint32_t)uxQueueMessagesWaitingFromISR (hSemaphore);
        }
        else
        {
            count = (uint32_t)uxSemaphoreGetCount (hSemaphore);
        }
        
        if((core_stat == 1U) && (timeout != osNoWait))
        {
            stat = osErrorIsrWait;
        }
        else if((core_stat != 0U) && (count == 0) && (timeout != osNoWait))
        {
            stat = osErrorIsrBlocked;
        }
        else if((kernel_stat != taskSCHEDULER_RUNNING) && (count == 0) && (timeout != osNoWait))
        {
            stat = osErrorKernelBlocked;
        }
        else if ((xCurentThread == xTaskGetIdleTaskHandle()) && (count == 0) && (timeout != osNoWait))
        {
            stat = osErrorIdleBlocked;
        }
        else if ((xCurentThread == xTimerGetTimerDaemonTaskHandle()) && (count == 0) && (timeout != osNoWait))
        {
            stat = osErrorTimerBlocked;
        }
        else
        {
            if (core_stat != 0U)
            {
                yield = pdFALSE;

                if (xSemaphoreTakeFromISR (hSemaphore, &yield) != pdPASS)
                {
                    stat = osErrorResource;
                }
                else
                {
                    portYIELD_FROM_ISR (yield);
                }
            }
            else
            {
                if (xSemaphoreTake (hSemaphore, (TickType_t)timeout) != pdPASS)
                {
                    if (timeout != 0U)
                    {
                        stat = osErrorTimeout;
                    }
                    else
                    {
                        stat = osErrorResource;
                    }
                }
            }
        }
    }

    osErrorState = stat;

    OSAL_ASSERT ((stat == osOK) || (stat == osErrorTimeout) || (stat == osErrorResource));

    /* Return execution status */
    return (stat);
}
#endif

#if( INCLUDE_osSemaphoreRelease == 1 )
/*
  Release a Semaphore token up to the initial maximum count.
*/
__RAM_FUNC osStatus_t osSemaphoreRelease (osSemaphoreId_t semaphore_id)
{
    SemaphoreHandle_t hSemaphore = (SemaphoreHandle_t)semaphore_id;
    osStatus_t stat;
    BaseType_t yield;

    stat = osOK;

    if (hSemaphore == NULL)
    {
        stat = osErrorParameter;
    }
    else if (IRQ_Context() != 0U)
    {
        yield = pdFALSE;

        if (xSemaphoreGiveFromISR (hSemaphore, &yield) != pdTRUE)
        {
            stat = osErrorResource;
        }
        else
        {
            portYIELD_FROM_ISR (yield);
        }
    }
    else
    {
        if (xSemaphoreGive (hSemaphore) != pdPASS)
        {
            stat = osErrorResource;
        }
    }

    osErrorState = stat;

    OSAL_ASSERT ((stat == osOK) || (stat == osErrorResource));

    /* Return execution status */
    return (stat);
}
#endif

#if( INCLUDE_osSemaphoreGetCount == 1 )
/*
  Get current Semaphore token count.
*/
uint32_t osSemaphoreGetCount (osSemaphoreId_t semaphore_id)
{
    SemaphoreHandle_t hSemaphore = (SemaphoreHandle_t)semaphore_id;
    uint32_t count;

    if (hSemaphore == NULL)
    {
        count = 0U;
    }
    else if (IRQ_Context() != 0U)
    {
        count = uxQueueMessagesWaitingFromISR (hSemaphore);
    }
    else
    {
        count = (uint32_t)uxSemaphoreGetCount (hSemaphore);
    }

    OSAL_ASSERT (hSemaphore != NULL);

    /* Return number of tokens */
    return (count);
}
#endif

#if( INCLUDE_osSemaphoreDelete == 1 )
/*
  Delete a Semaphore object.
*/
osStatus_t osSemaphoreDelete (osSemaphoreId_t semaphore_id)
{
    SemaphoreHandle_t hSemaphore = (SemaphoreHandle_t)semaphore_id;
    osStatus_t stat;

    if (IRQ_Context() != 0U)
    {
        stat = osErrorISR;
    }
    else if (hSemaphore == NULL)
    {
        stat = osErrorParameter;
    }
    else
    {
        #if (configQUEUE_REGISTRY_SIZE > 0)
        {
            vQueueUnregisterQueue (hSemaphore);
        }
        #endif

        stat = osOK;

        vSemaphoreDelete (hSemaphore);
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return execution status */
    return (stat);
}
#endif


/* ==== Message Queue Management Functions ==== */

#if( INCLUDE_osMessageQueueNew == 1 )
/*
  Create and Initialize a Message Queue object.

  Limitations:
  - The memory for control block and and message data must be provided in the
    osThreadAttr_t structure in order to allocate object statically.
*/
osMessageQueueId_t osMessageQueueNew (uint32_t msg_count, uint32_t msg_size, const osMessageQueueAttr_t *attr)
{
    QueueHandle_t hQueue = NULL;
    int32_t mem;
    osStatus_t stat = osOK;

    if (IRQ_Context() == 1U)
    {
        stat = osErrorISR;
    }
    else
    {
        if (attr != NULL)
        {
            if ((attr->cb_mem != NULL) && (attr->cb_size >= sizeof(StaticQueue_t)) &&
            (attr->mq_mem != NULL) && (attr->mq_size >= (msg_count * msg_size)))
            {
                /* The memory for control block and message data is provided, use static object */
                mem = 1;
            }
            else if ((attr->cb_mem == NULL) && (attr->cb_size == 0U) &&
                (attr->mq_mem == NULL) && (attr->mq_size == 0U))
            {
                /* Control block will be allocated from the dynamic pool */
                mem = 0;
            }
            else
            {
                mem = -1;
            }
        }
        else
        {
            mem = 0;
        }

        if (mem == 1)
        {
            /* The memory for control block is provided, use static object */
            #if (configSUPPORT_STATIC_ALLOCATION == 1)
            {
                hQueue = xQueueCreateStatic (msg_count, msg_size, attr->mq_mem, attr->cb_mem);
            }
            #endif

            if (hQueue == NULL)
            {
                stat = osErrorResource;
            }
        }
        else if (mem == 0)
        {
            /* Control block will be allocated from the dynamic pool */
            #if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
            {
                hQueue = xQueueCreate (msg_count, msg_size);
            }
            #endif

            if (hQueue == NULL)
            {
                stat = osErrorResource;
            }
        }
        else
        {
            stat = osErrorParameter;
        }

        #if (configQUEUE_REGISTRY_SIZE > 0)
        {
            const char *name;

            if (attr != NULL)
            {
                name = attr->name;
            }
            else
            {
                name = NULL;
            }

            if (hQueue != NULL)
            {
                vQueueAddToRegistry (hQueue, attr->name);
            }
        }
        #endif
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return message queue ID */
    return ((osMessageQueueId_t)hQueue);
}
#endif

#if( INCLUDE_osMessageQueuePut == 1 )
/*
  Put a Message into a Queue or timeout if Queue is full.

  Limitations:
  - Message priority is ignored
*/
__RAM_FUNC osStatus_t osMessageQueuePut (osMessageQueueId_t mq_id, const void *msg_ptr, uint8_t msg_prio, uint32_t timeout)
{
    QueueHandle_t hQueue = (QueueHandle_t)mq_id;
    StaticQueue_t *mq = (StaticQueue_t *)mq_id;
    osStatus_t stat;
    BaseType_t yield;
    TaskHandle_t xCurentThread;
    uint32_t core_stat;
    BaseType_t kernel_stat;
    uint32_t space;
    uint32_t isrm;

    (void)msg_prio; /* Message priority is ignored */

    stat = osOK;

    if ((hQueue == NULL) || (msg_ptr == NULL))
    {
        stat = osErrorParameter;
    }
    else
    {
        xCurentThread = xTaskGetCurrentTaskHandle();
        core_stat = IRQ_Context();
        kernel_stat = xTaskGetSchedulerState();

        if (core_stat != 0U)
        {
            isrm = taskENTER_CRITICAL_FROM_ISR();
            /* space = pxQueue->uxLength - pxQueue->uxMessagesWaiting; */
            space = mq->uxDummy4[1] - mq->uxDummy4[0];
            taskEXIT_CRITICAL_FROM_ISR(isrm);
        }
        else
        {
            space = (uint32_t)uxQueueSpacesAvailable ((QueueHandle_t)mq);
        }

        if((core_stat == 1U) && (timeout != osNoWait))
        {
            stat = osErrorIsrWait;
        }
        else if((core_stat != 0U) && (space == 0) && (timeout != osNoWait))
        {
            stat = osErrorIsrBlocked;
        }
        else if((kernel_stat != taskSCHEDULER_RUNNING) && (space == 0) && (timeout != osNoWait))
        {
            stat = osErrorKernelBlocked;
        }
        else if ((xCurentThread == xTaskGetIdleTaskHandle()) && (space == 0) && (timeout != osNoWait))
        {
            stat = osErrorIdleBlocked;
        }
        else if ((xCurentThread == xTimerGetTimerDaemonTaskHandle()) && (space == 0) && (timeout != osNoWait))
        {
            stat = osErrorTimerBlocked;
        }
        else
        {
            if (core_stat != 0U)
            {
                yield = pdFALSE;

                if (xQueueSendToBackFromISR (hQueue, msg_ptr, &yield) != pdTRUE)
                {
                    stat = osErrorResource;
                }
                else
                {
                    portYIELD_FROM_ISR (yield);
                }
            }
            else
            {
                if (xQueueSendToBack (hQueue, msg_ptr, (TickType_t)timeout) != pdPASS)
                {
                    if (timeout != 0U)
                    {
                        stat = osErrorTimeout;
                    }
                    else
                    {
                        stat = osErrorResource;
                    }
                }
            }
        }
    }

    osErrorState = stat;

    OSAL_ASSERT ((stat == osOK) || (stat == osErrorTimeout) || (stat == osErrorResource));

    /* Return execution status */
    return (stat);
}
#endif

#if( INCLUDE_osMessageQueueGet == 1 )
/*
  Get a Message from a Queue or timeout if Queue is empty.

  Limitations:
  - Message priority is ignored
*/
osStatus_t osMessageQueueGet (osMessageQueueId_t mq_id, void *msg_ptr, uint8_t *msg_prio, uint32_t timeout)
{
    QueueHandle_t hQueue = (QueueHandle_t)mq_id;
    osStatus_t stat;
    BaseType_t yield;
    TaskHandle_t xCurentThread;
    uint32_t core_stat;
    BaseType_t kernel_stat;
    uint32_t count;

    (void)msg_prio; /* Message priority is ignored */

    stat = osOK;

    if ((hQueue == NULL) || (msg_ptr == NULL))
    {
        stat = osErrorParameter;
    }
    else
    {
        xCurentThread = xTaskGetCurrentTaskHandle();
        core_stat = IRQ_Context();
        kernel_stat = xTaskGetSchedulerState();

        if (core_stat != 0U)
        {
            count = (uint32_t)uxQueueMessagesWaitingFromISR (hQueue);
        }
        else
        {
            count = (uint32_t)uxQueueMessagesWaiting (hQueue);
        }

        if((core_stat == 1U) && (timeout != osNoWait))
        {
            stat = osErrorIsrWait;
        }
        else if((core_stat != 0U) && (count == 0) && (timeout != osNoWait))
        {
            stat = osErrorIsrBlocked;
        }
        else if((kernel_stat != taskSCHEDULER_RUNNING) && (count == 0) && (timeout != osNoWait))
        {
            stat = osErrorKernelBlocked;
        }
        else if ((xCurentThread == xTaskGetIdleTaskHandle()) && (count == 0) && (timeout != osNoWait))
        {
            stat = osErrorIdleBlocked;
        }
        else if ((xCurentThread == xTimerGetTimerDaemonTaskHandle()) && (count == 0) && (timeout != osNoWait))
        {
            stat = osErrorTimerBlocked;
        }
        else
        {
            if (core_stat != 0U)
            {
                yield = pdFALSE;

                if (xQueueReceiveFromISR (hQueue, msg_ptr, &yield) != pdPASS)
                {
                    stat = osErrorResource;
                }
                else
                {
                    portYIELD_FROM_ISR (yield);
                }
            }
            else
            {
                if (xQueueReceive (hQueue, msg_ptr, (TickType_t)timeout) != pdPASS)
                {
                    if (timeout != 0U)
                    {
                        stat = osErrorTimeout;
                    }
                    else
                    {
                        stat = osErrorResource;
                    }
                }
            }
        }
    }

    osErrorState = stat;

    OSAL_ASSERT ((stat == osOK) || (stat == osErrorTimeout) || (stat == osErrorResource));

    /* Return execution status */
    return (stat);
}
#endif

#if( INCLUDE_osMessageQueueGetCapacity == 1 )
/*
  Get maximum number of messages in a Message Queue.
*/
uint32_t osMessageQueueGetCapacity (osMessageQueueId_t mq_id)
{
    StaticQueue_t *mq = (StaticQueue_t *)mq_id;
    uint32_t capacity;

    if (mq == NULL)
    {
        capacity = 0U;
    }
    else
    {
        /* capacity = pxQueue->uxLength */
        capacity = mq->uxDummy4[1];
    }

    OSAL_ASSERT (mq != NULL);

    /* Return maximum number of messages */
    return (capacity);
}
#endif

#if( INCLUDE_osMessageQueueGetMsgSize == 1 )
/*
  Get maximum message size in a Message Queue.
*/
uint32_t osMessageQueueGetMsgSize (osMessageQueueId_t mq_id)
{
    StaticQueue_t *mq = (StaticQueue_t *)mq_id;
    uint32_t size;

    if (mq == NULL)
    {
        size = 0U;
    }
    else
    {
        /* size = pxQueue->uxItemSize */
        size = mq->uxDummy4[2];
    }

    OSAL_ASSERT (mq != NULL);

    /* Return maximum message size */
    return (size);
}
#endif

#if( INCLUDE_osMessageQueueGetCount == 1 )
/*
  Get number of queued messages in a Message Queue.
*/
uint32_t osMessageQueueGetCount (osMessageQueueId_t mq_id)
{
    QueueHandle_t hQueue = (QueueHandle_t)mq_id;
    UBaseType_t count;

    if (hQueue == NULL)
    {
        count = 0U;
    }
    else if (IRQ_Context() != 0U)
    {
        count = uxQueueMessagesWaitingFromISR (hQueue);
    }
    else
    {
        count = uxQueueMessagesWaiting (hQueue);
    }

    OSAL_ASSERT (hQueue != NULL);

    /* Return number of queued messages */
    return ((uint32_t)count);
}
#endif

#if( INCLUDE_osMessageQueueGetSpace == 1 )
/*
  Get number of available slots for messages in a Message Queue.
*/
uint32_t osMessageQueueGetSpace (osMessageQueueId_t mq_id)
{
    StaticQueue_t *mq = (StaticQueue_t *)mq_id;
    uint32_t space;
    uint32_t isrm;

    if (mq == NULL)
    {
        space = 0U;
    }
    else if (IRQ_Context() != 0U)
    {
        isrm = taskENTER_CRITICAL_FROM_ISR();

        /* space = pxQueue->uxLength - pxQueue->uxMessagesWaiting; */
        space = mq->uxDummy4[1] - mq->uxDummy4[0];

        taskEXIT_CRITICAL_FROM_ISR(isrm);
    }
    else
    {
        space = (uint32_t)uxQueueSpacesAvailable ((QueueHandle_t)mq);
    }

    OSAL_ASSERT (mq != NULL);

    /* Return number of available slots */
    return (space);
}
#endif

#if( INCLUDE_osMessageQueueReset == 1 )
/*
  Reset a Message Queue to initial empty state.
*/
osStatus_t osMessageQueueReset (osMessageQueueId_t mq_id)
{
    QueueHandle_t hQueue = (QueueHandle_t)mq_id;
    osStatus_t stat;

    if (IRQ_Context() != 0U)
    {
        stat = osErrorISR;
    }
    else if (hQueue == NULL)
    {
        stat = osErrorParameter;
    }
    else
    {
        stat = osOK;
        (void)xQueueReset (hQueue);
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return execution status */
    return (stat);
}
#endif

#if( INCLUDE_osMessageQueueDelete == 1 )
/*
  Delete a Message Queue object.
*/
osStatus_t osMessageQueueDelete (osMessageQueueId_t mq_id)
{
    QueueHandle_t hQueue = (QueueHandle_t)mq_id;
    osStatus_t stat;

    if (IRQ_Context() != 0U)
    {
        stat = osErrorISR;
    }
    else if (hQueue == NULL)
    {
        stat = osErrorParameter;
    }
    else
    {
        #if (configQUEUE_REGISTRY_SIZE > 0)
        {
            vQueueUnregisterQueue (hQueue);
        }
        #endif

        stat = osOK;
        vQueueDelete (hQueue);
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    /* Return execution status */
    return (stat);
}
#endif


/* ==== Memory Management Functions ==== */

#if( INCLUDE_osMemoryAlloc == 1 )
/*
  Allocate a block of memory
 */
#if( FreeRTOS_HEAP_MALLOC_WITH_RECORD == 1 )
void* osMemoryAllocDebug(size_t size, char *file, int line)
#else
void* osMemoryAlloc(size_t size)
#endif
{
    void *mem = NULL;
    osStatus_t stat = osOK;

    if (IRQ_Context() == 1U)
    {
        stat = osErrorISR;
    }
    else if(size == 0)
    {
        stat = osErrorParameter;
    }
    else
    {
        #if( FreeRTOS_HEAP_MALLOC_WITH_RECORD == 1 )
        {
            mem = pvPortMallocNormal(size, file, line);

            if ((uint32_t)mem % 4 != 0)
            {
                stat = osErrorNoAlignMemory;
            }
        }
        #else
        {
            mem = pvPortMallocNormal(size);

            if ((uint32_t)mem % 4 != 0)
            {
                stat = osErrorNoAlignMemory;
            }
        }
        #endif

        if(mem == NULL)
        {
            stat = osErrorNoMemory;
        }
    }

    osErrorState = stat;

    OSAL_ASSERT (stat != osErrorISR);

    return mem;
}
#endif

#if( INCLUDE_osMemoryAllocAlign == 1 )
/*
  Allocate a block of memory
 */
#if( FreeRTOS_HEAP_MALLOC_WITH_RECORD == 1 )
void* osMemoryAllocAlignDebug(size_t size, char *file, int line)
#else
void* osMemoryAllocAlign(size_t size)
#endif
{
    void *mem = NULL;
    osStatus_t stat = osOK;

    if (IRQ_Context() == 1U)
    {
        stat = osErrorISR;
    }
    else if(size == 0)
    {
        stat = osErrorParameter;
    }
    else
    {
        #if( FreeRTOS_HEAP_MALLOC_WITH_RECORD == 1 )
        {
            mem = pvPortMallocAlign(size, file, line);

            if ((uint32_t)mem % 32 != 0)
            {
                stat = osErrorNoAlignMemory;
            }
        }
        #else
        {
            mem = pvPortMallocAlign(size);

            if ((uint32_t)mem % 32 != 0)
            {
                stat = osErrorNoAlignMemory;
            }
        }
        #endif

        if(mem == NULL)
        {
            stat = osErrorNoMemory;
        }
    }
    
    osErrorState = stat;

    OSAL_ASSERT (stat != osErrorISR);

    return mem;
}
#endif

#if( INCLUDE_osMemoryRealloc == 1 )
/*
  Reallocate a block of memory
 */
#if( FreeRTOS_HEAP_MALLOC_WITH_RECORD == 1 )
void* osMemoryReallocDebug(void *mem, size_t size, char *file, int line)
#else
void* osMemoryRealloc(void *mem, size_t size)
#endif
{
    void *mem_ret = NULL;
    osStatus_t stat = osOK;

    if (IRQ_Context() == 1U)
    {
        stat = osErrorISR;
    }
    else if(size == 0)
    {
        stat = osErrorParameter;
    }
    else
    {
        #if( FreeRTOS_HEAP_MALLOC_WITH_RECORD == 1 )
        {
            mem_ret = pvPortReallocNormal(mem, size, file, line);
        }
        #else
        {
            mem_ret = pvPortReallocNormal(mem, size);
        }
        #endif

        if(mem_ret == NULL)
        {
            stat = osErrorNoMemory;
        }
    }

    osErrorState = stat;

    OSAL_ASSERT (stat != osErrorISR);

    return mem_ret;
}
#endif

#if( INCLUDE_osMemoryReallocAlign == 1 )
/*
  Reallocate a block of memory
 */
#if( FreeRTOS_HEAP_MALLOC_WITH_RECORD == 1 )
void* osMemoryReallocAlignDebug(void *mem, size_t size, char *file, int line)
#else
void* osMemoryReallocAlign(void *mem, size_t size)
#endif
{
    void *mem_ret = NULL;
    osStatus_t stat = osOK;

    if (IRQ_Context() == 1U)
    {
        stat = osErrorISR;
    }
    else if(size == 0)
    {
        stat = osErrorParameter;
    }
    else
    {
        #if( FreeRTOS_HEAP_MALLOC_WITH_RECORD == 1 )
        {
            mem_ret = pvPortReallocAlign(mem, size, file, line);
        }
        #else
        {
            mem_ret = pvPortReallocAlign(mem, size);
        }
        #endif

        if(mem_ret == NULL)
        {
            stat = osErrorNoMemory;
        }
    }

    osErrorState = stat;

    OSAL_ASSERT (stat != osErrorISR);

    return mem_ret;
}
#endif

#if( INCLUDE_osMemoryFree == 1 )
/*
  Free a block of memory
 */
#if( FreeRTOS_HEAP_FREE_WITH_RECORD == 1 )
osStatus_t osMemoryFreeDebug(void *mem, char *file, int line)
#else
osStatus_t osMemoryFree(void *mem)
#endif
{
    osStatus_t stat = osOK;

    if (IRQ_Context() == 1U)
    {
        stat = osErrorISR;
    }
    else if(mem == NULL)
    {
        stat = osErrorParameter;
    }
    else
    {
        #if( FreeRTOS_HEAP_FREE_WITH_RECORD == 1 )
        {
            vPortFreeWithDebug(mem, file, line);
        }
        #else
        {
            vPortFree(mem);
        }
        #endif
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    return stat;
}
#endif


/* ==== Sleep Management Functions ==== */
#if( INCLUDE_osThreadSetLowPowerFlag == 1 )

extern BaseType_t IsNodeUSED(void* node);
extern void* getTaskStackAddr(TaskHandle_t taskId);
extern uint32_t getTaskTopStack(TaskHandle_t taskId);
/*
  Set thread low power flag
*/
osStatus_t osThreadSetLowPowerFlag(osThreadId_t thread_id, osLpmRealtime_t lpm_flag)
{
    osStatus_t stat = osOK;

    TaskHandle_t hTask = (TaskHandle_t)thread_id;

    eLowPowerFlag eFlag = (lpm_flag == osLpmNoRealtime) ? eProhibitSleepStatistics : ePermitSleepStatistics;

    osCoreEnterCritical();

    uint32_t* temp_addr = getTaskStackAddr(hTask);
    uint8_t value1 = IsNodeUSED(temp_addr);

    osCoreExitCritical();
    // uint32_t value2 = *temp_addr;

    OSAL_ASSERT(value1 == 1); //&& (value2 == 0xa5a5a5a5));
   
    vTaskSetLowPowerFlag(hTask, eFlag);

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    return (stat);
}
#endif

#if( INCLUDE_osThreadGetLowPowerFlag == 1 )
/*
  Set thread low power flag
*/
osLpmRealtime_t osThreadGetLowPowerFlag(osThreadId_t thread_id)
{
    eLowPowerFlag eflag;

    osLpmRealtime_t lpm_flag;

    eflag = eTaskGetLowPowerFlag((TaskHandle_t)thread_id);

    lpm_flag = (eflag == eProhibitSleepStatistics) ? osLpmNoRealtime : osLpmRealtime;

    return (lpm_flag);
}
#endif

#if( INCLUDE_osThreadGetLowPowerTime == 1 )
/*
  Get thread low power time
*/
uint32_t osThreadGetLowPowerTime(osSleepStatisticsType_t sleep_type)
{
    osStatus_t stat;
    uint32_t ticks = 0;

    eSleepStatistics type = (sleep_type == osIgnorLowPowerFlag) ? eIgnorLowPowerFlag : eAttentionLowPowerFlag;

    if (IRQ_Context() == 1U)
    {
        stat = osErrorISR;
    }
    else if (xTaskGetCurrentTaskHandle() != xTaskGetIdleTaskHandle())
    {
        stat = osErrorResource;
    }
    else
    {
        stat = osOK;
        ticks = xTaskGetExpectedIdleTime(type);
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    return (ticks);
}
#endif

#if( INCLUDE_osTimerSetLowPowerFlag == 1 )
/*
  Set timer low power flag
*/
osStatus_t osTimerSetLowPowerFlag(osTimerId_t timer_id, osLpmRealtime_t lpm_flag)
{
    osStatus_t stat;

    eLowPowerFlag eFlag;

    TimerHandle_t hTimer = (TimerHandle_t)timer_id;

    if(hTimer == NULL)
    {
        stat = osErrorParameter;
    }
    else
    {
        stat = osOK;

        eFlag = (lpm_flag == osLpmNoRealtime) ? eProhibitSleepStatistics : ePermitSleepStatistics;

        xTimerSetLowPowerFlag(hTimer, eFlag);
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    return (stat);
}
#endif

#if( INCLUDE_osTimerGetLowPowerFlag == 1 )
/*
  Get timer low power flag
*/
osLpmRealtime_t osTimerGetLowPowerFlag(osTimerId_t timer_id)
{
    osStatus_t stat;

    eLowPowerFlag eflag;

    osLpmRealtime_t lpm_flag = osLpmRealtime;

    TimerHandle_t hTimer = (TimerHandle_t)timer_id;

    if(hTimer == NULL)
    {
        stat = osErrorParameter;
    }
    else
    {
        stat = osOK;

        eflag = eTimerGetLowPowerFlag(hTimer);

        lpm_flag = (eflag == eProhibitSleepStatistics) ? osLpmNoRealtime : osLpmRealtime;
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    return (lpm_flag);
}
#endif

#if( INCLUDE_osTimerGetLowPowerTime == 1 )
/*
  Get thread low power time
*/
uint32_t osTimerGetLowPowerTime(osSleepStatisticsType_t sleep_type)
{
    osStatus_t stat;
    uint32_t ticks = 0;

    eSleepStatistics type = (sleep_type == osIgnorLowPowerFlag) ? eIgnorLowPowerFlag : eAttentionLowPowerFlag;

    if (IRQ_Context() == 1U)
    {
        stat = osErrorISR;
    }
    else if (xTaskGetCurrentTaskHandle() != xTaskGetIdleTaskHandle())
    {
        stat = osErrorResource;
    }
    else
    {
        stat = osOK;
        ticks = xTimerGetExpectedUnblockTime(type);
    }

    osErrorState = stat;

    OSAL_ASSERT (stat == osOK);

    return (ticks);
}
#endif
