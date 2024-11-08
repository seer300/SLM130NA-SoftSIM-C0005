#include "osAssitantUtils.h"
#include "portmacro.h"
#include "list.h"



#if configHEAP_TEST
uint32_t  LimitedFreeHeap = 0;
#endif
typedef struct tskTaskControlBlock 			/* The old naming convention is used to prevent breaking kernel aware debuggers. */
{
	volatile StackType_t	*pxTopOfStack;	/*< Points to the location of the last item placed on the tasks stack.  THIS MUST BE THE FIRST MEMBER OF THE TCB STRUCT. */

	#if ( portUSING_MPU_WRAPPERS == 1 )
		xMPU_SETTINGS	xMPUSettings;		/*< The MPU settings are defined as part of the port layer.  THIS MUST BE THE SECOND MEMBER OF THE TCB STRUCT. */
	#endif

	ListItem_t			xStateListItem;	/*< The list that the state list item of a task is reference from denotes the state of that task (Ready, Blocked, Suspended ). */
	ListItem_t			xEventListItem;		/*< Used to reference a task from an event list. */
	UBaseType_t			uxPriority;			/*< The priority of the task.  0 is the lowest priority. */
	StackType_t			*pxStack;			/*< Points to the start of the stack. */
	char				pcTaskName[ configMAX_TASK_NAME_LEN ];/*< Descriptive name given to the task when created.  Facilitates debugging only. */ /*lint !e971 Unqualified char types are allowed for strings and single characters only. */

	#if ( ( portSTACK_GROWTH > 0 ) || ( configRECORD_STACK_HIGH_ADDRESS == 1 ) )
		StackType_t		*pxEndOfStack;		/*< Points to the highest valid address for the stack. */
	#endif

	#if ( portCRITICAL_NESTING_IN_TCB == 1 )
		UBaseType_t		uxCriticalNesting;	/*< Holds the critical section nesting depth for ports that do not maintain their own count in the port layer. */
	#endif

	#if ( configUSE_TRACE_FACILITY == 1 )
		UBaseType_t		uxTCBNumber;		/*< Stores a number that increments each time a TCB is created.  It allows debuggers to determine when a task has been deleted and then recreated. */
		UBaseType_t		uxTaskNumber;		/*< Stores a number specifically for use by third party trace code. */
	#endif

	#if ( configUSE_MUTEXES == 1 )
		UBaseType_t		uxBasePriority;		/*< The priority last assigned to the task - used by the priority inheritance mechanism. */
		UBaseType_t		uxMutexesHeld;
	#endif

	#if ( configUSE_APPLICATION_TASK_TAG == 1 )
		TaskHookFunction_t pxTaskTag;
	#endif

	#if( configNUM_THREAD_LOCAL_STORAGE_POINTERS > 0 )
		void			*pvThreadLocalStoragePointers[ configNUM_THREAD_LOCAL_STORAGE_POINTERS ];
	#endif

	#if( configGENERATE_RUN_TIME_STATS == 1 )
		uint32_t		ulRunTimeCounter;	/*< Stores the amount of time the task has spent in the Running state. */
	#endif

	#if ( configUSE_NEWLIB_REENTRANT == 1 )
		/* Allocate a Newlib reent structure that is specific to this task.
		Note Newlib support has been included by popular demand, but is not
		used by the FreeRTOS maintainers themselves.  FreeRTOS is not
		responsible for resulting newlib operation.  User must be familiar with
		newlib and must provide system-wide implementations of the necessary
		stubs. Be warned that (at the time of writing) the current newlib design
		implements a system-wide malloc() that must be provided with locks.

		See the third party link http://www.nadler.com/embedded/newlibAndFreeRTOS.html
		for additional information. */
		struct	_reent xNewLib_reent;
	#endif

	#if( configUSE_TASK_NOTIFICATIONS == 1 )
		volatile uint32_t ulNotifiedValue;
		volatile uint8_t ucNotifyState;
	#endif

	/* See the comments in FreeRTOS.h with the definition of
	tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE. */
	#if( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 ) /*lint !e731 !e9029 Macro has been consolidated for readability reasons. */
		uint8_t	ucStaticallyAllocated; 		/*< Set to pdTRUE if the task is a statically allocated to ensure no attempt is made to free the memory. */
	#endif

	#if( INCLUDE_xTaskAbortDelay == 1 )
		uint8_t ucDelayAborted;
	#endif

	#if( configUSE_POSIX_ERRNO == 1 )
		int iTaskErrno;
	#endif

	uint8_t             ucStackType;		/*< stack type, use shared stack or not, 0 to use */

	#if( configUSE_LOW_POWER_FLAG == 1 )
		uint8_t ucLowPowerFlag;
	#endif

	uint32_t             ulMaxBackup;

} tskTCB;

/* The old tskTCB name is maintained above then typedefed to the new TCB_t name
below to enable the use of older kernel aware debuggers. */
typedef tskTCB TCB_t;


//extern volatile UBaseType_t uxTopReadyPriority ;
//extern List_t pxReadyTasksLists[ configMAX_PRIORITIES ];/*< Prioritised ready tasks. */
//extern List_t xDelayedTaskList1;						/*< Delayed tasks. */
//extern List_t xDelayedTaskList2;						/*< Delayed tasks (two lists are used - one for delays that have overflowed the current tick count. */
//extern List_t * volatile pxDelayedTaskList;				/*< Points to the delayed task list currently being used. */
//extern  List_t * volatile pxOverflowDelayedTaskList;		/*< Points to the delayed task list currently being used to hold tasks that have overflowed the current tick count. */
//extern List_t xPendingReadyList;						/*< Tasks that have been readied while the scheduler was suspended.  They will be moved to the ready list when the scheduler is resumed. */
extern volatile StackType_t * pxSharedStack;
extern  TCB_t * volatile pxSharedTCB;


uint32_t uxTaskGetSharedStackHighWaterMark()
{
    TaskHandle_t xTask = (TaskHandle_t)pxSharedTCB;
    uint32_t uxReturn;


 uxReturn = (uint32_t)((configTASK_SHARED_STACK_DEPTH)*4)-(uint32_t) (uxTaskGetStackHighWaterMark(xTask))*4;
 return uxReturn;
}


uint32_t xyTaskGetIndStackHighWaterMark( TaskHandle_t xTask )
{
	TCB_t *pxTCB = (TCB_t * )xTask;

	uint32_t uxReturn;


	uxReturn =((uint32_t)(pxTCB->pxEndOfStack)-(uint32_t)(pxTCB->pxStack)- (uint32_t)(uxTaskGetStackHighWaterMark(xTask))*4);;

	return uxReturn;
}


xySatusList_t xStackSpaceList;
xySatusList_t * volatile pxStackSpaceList;
xySatusList_t *taskinfohead = NULL;


void vTaskFreeList(xySatusList_t * pxSatusList)
{

	StackStatus_t *  pxIndex = pxSatusList->xyListEnd.pxNext;
	StackStatus_t *  pxIndextmp;
	while(pxIndex != (StackStatus_t *) &(pxSatusList->xyListEnd))
	{
		pxIndextmp = pxIndex;
		pxIndex = pxIndex->pxNext;
		vPortFree(pxIndextmp);
	}


}

void xyListInitialise(xySatusList_t * const pxSatusList)
{

	pxSatusList->pxIndex = (StackStatus_t  *) & (pxSatusList->xyListEnd);
	pxSatusList->xyListEnd.pxNext = (StackStatus_t *)& (pxSatusList->xyListEnd);	/*lint !e826 !e740 !e9087 The mini list structure is used as the list end to save RAM.  This is checked and valid. */
	pxSatusList->xyListEnd.pxPrevious = (StackStatus_t  *)& (pxSatusList->xyListEnd);;

}


void xyListInsert( xySatusList_t * const pxSatusList, StackStatus_t * const pxNewStackStatus )
{
	StackStatus_t * const pxIndex = pxSatusList->pxIndex;

		/* Insert a new list item into pxList, but rather than sort the list,
		makes the new list item the last item to be removed by a call to
		listGET_OWNER_OF_NEXT_ENTRY(). */
		pxNewStackStatus->pxNext = pxIndex;
		pxNewStackStatus->pxPrevious = pxIndex->pxPrevious;

		/* Only used during decision coverage testing. */
		mtCOVERAGE_TEST_DELAY();

		pxIndex->pxPrevious->pxNext = pxNewStackStatus;
		pxIndex->pxPrevious = pxNewStackStatus;

}

StackStatus_t *xyTaskSetStatus(StackStatus_t * pxStackStatus)
{
	TCB_t *pxTCB =(TCB_t *) pxStackStatus->xTask;
	StackStatus_t *xySetStatus;
	UBaseType_t x;
    char *pcName;

        pcName = &( pxTCB->pcTaskName[ 0 ]);
	    for( x = ( UBaseType_t ) 0; x < ( UBaseType_t ) configMAX_TASK_NAME_LEN; x++ )
		{
	    	pxStackStatus->xyTaskName[ x ] = pcName[ x ];
		}

		pxStackStatus->xyPriority = uxTaskPriorityGet(pxStackStatus->xTask);
		pxStackStatus->xyState= eTaskGetState(pxStackStatus->xTask);
		pxStackStatus->xyStackInfo.xyStackType = pxTCB->ucStackType;
		if(pxStackStatus->xyStackInfo.xyStackType == 0)
		{
			pxStackStatus->xyStackInfo.xyInitStackSpace = (uint32_t)0;
			pxStackStatus->xyStackInfo.xyStackSpace = pxTCB->ulMaxBackup;
		}
		else
		{
			pxStackStatus->xyStackInfo.xyInitStackSpace = (uint32_t)(pxTCB->pxEndOfStack) - (uint32_t)(pxTCB->pxStack);
			pxStackStatus->xyStackInfo.xyStackSpace = (uint32_t)xyTaskGetIndStackHighWaterMark(pxStackStatus->xTask);

		}
		xySetStatus = pxStackStatus;

		return xySetStatus;
}
#if( configUSE_HEAP_USED_LINKED_LIST == 1 )
extern size_t xyTaskGetAllocatedBlocks(eHeapRegions eRegion);
void xyTaskGetHeapInfo(HeapInfo_t *xyHeapInfo)
{

    extern uint32_t _Heap_Begin;
    extern uint32_t _Heap_Limit;

    size_t heap_size = (size_t)&_Heap_Limit - (size_t)&_Heap_Begin;


    xyHeapInfo->NumAllocatedBlocks1= xyTaskGetAllocatedBlocks(RegionOne);
    xyHeapInfo->NumAllocatedBlocks2= xyTaskGetAllocatedBlocks(RegionTwo);
    xyHeapInfo->NumAllocatedBlocks3= xyTaskGetAllocatedBlocks(RegionThree);
    xyHeapInfo->FreeHeapSize1 = xPortGetRegionFreeHeapSize(RegionOne);
    xyHeapInfo->FreeHeapSize2 = xPortGetRegionFreeHeapSize(RegionTwo);
    xyHeapInfo->FreeHeapSize3 = xPortGetRegionFreeHeapSize(RegionThree);
    xyHeapInfo->DynamicHeapSize = heap_size;
    xyHeapInfo->pucStartAddress1= (uint32_t)&_Heap_Begin;
    xyHeapInfo->xyFreeHeapSize = xPortGetFreeHeapSize();
    xyHeapInfo->xyMinimumEverFreeBytesRemaining = xPortGetMinimumEverFreeHeapSize();

}
void vTaskGetAppsMem(AppsMemStats_t * pxHeapStats)
{
    HeapStats_t xyHeapStats;
    HeapInfo_t xyHeapInfo;
    osCoreEnterCritical();
    xyTaskGetHeapInfo(&xyHeapInfo);
    vPortGetHeapStats(&xyHeapStats);
    osCoreExitCritical();
    extern HeapRegion_t HeapRegions[HEAP_REGION_MAX + 1];

    
    pxHeapStats->HeapSize = HeapRegions[RegionOne].xSizeInBytes + HeapRegions[RegionTwo].xSizeInBytes + HeapRegions[RegionThree].xSizeInBytes;
    pxHeapStats->AllocatedBlockNum = xyHeapStats.xNumberOfSuccessfulAllocations;
    pxHeapStats->FreeBlockNum = xyHeapStats.xNumberOfSuccessfulFrees;
    pxHeapStats->MaxSizeFreeBlock = xyHeapStats.xSizeOfLargestFreeBlockInBytes;
    pxHeapStats->AvailableHeapSize = xyHeapInfo.xyFreeHeapSize;
	pxHeapStats->MinimumEverFreeBytesRemaining = xyHeapInfo.xyMinimumEverFreeBytesRemaining;

	pxHeapStats->AllocatedHeapSize = pxHeapStats->HeapSize - pxHeapStats->AvailableHeapSize;
}
#else
void xyTaskGetHeapInfo(HeapInfo_t *xyHeapInfo){}
void vTaskGetAppsMem(AppsMemStats_t * pxHeapStats){}
#endif
