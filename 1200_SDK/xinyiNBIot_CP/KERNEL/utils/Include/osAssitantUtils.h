#ifndef OSASSITANTUTILS_H
#define OSASSITANTUTILS_H

#include "FreeRTOS.h"
#include "task.h"
#include "xy_memmap.h"
#define configHEAP_TEST                            1       // 1: enable only used for memeory statistics

typedef struct xSTACK_INFO
{
	uint8_t xyStackType;		/*< stack type, use shared stack or not, 0 to use */
	uint32_t xyStackSpace;      /*<the maximum amount of stack space that have used for the task
	                             since the task was created */
	uint32_t xyInitStackSpace;  /*< the size of task stack when the task create*/
}StackInfo_t;

typedef struct xSTACK_STATUS
{
	struct xSTACK_STATUS *pxNext; /*<Pinter to the next StackStatus_t in the list*/
	struct xSTACK_STATUS *pxPrevious;/*Pointer to the previous StackStatus_t in the list*/
	TaskHandle_t xTask;             /*<the Handle of task */
	char   xyTaskName[ 16 ];       /* descriptive name for the task */
    StackInfo_t xyStackInfo;      /* StackInfo_t structure with information about task stack */
    UBaseType_t	xyPriority;
    eTaskState xyState;

}StackStatus_t;


typedef struct xSATUS_LIST
{
	StackStatus_t * pxIndex;
	StackStatus_t  xyListEnd;

}xySatusList_t;

typedef struct xHEAP_INFO
{

	size_t xyFreeHeapSize;
	size_t FreeHeapSize1;
	size_t FreeHeapSize2;
	size_t FreeHeapSize3;
	size_t DynamicHeapSize;
	size_t xyMinimumEverFreeBytesRemaining;	/* The minimum amount of total free memory (sum of all free blocks) there has been in the heap since the system booted. */
	size_t NumAllocatedBlocks1;/* The number of allocated memory blocks */
	size_t NumAllocatedBlocks2;/* The number of allocated memory blocks */
	size_t NumAllocatedBlocks3;/* The number of allocated memory blocks */
	uint32_t pucStartAddress1;


}HeapInfo_t;

typedef struct xAPPSMEM_STATS
{
	size_t HeapSize;
    size_t AllocatedHeapSize;
    size_t AvailableHeapSize;
    size_t MaxSizeFreeBlock;
    size_t AllocatedBlockNum;
    size_t FreeBlockNum;
	size_t MinimumEverFreeBytesRemaining;
}AppsMemStats_t;

extern xySatusList_t xStackSpaceList;
extern xySatusList_t * volatile pxStackSpaceList;
extern xySatusList_t *taskinfohead;

uint32_t uxTaskGetSharedStackHighWaterMark()__FLASH_FUNC;
uint32_t xyTaskGetIndStackHighWaterMark( TaskHandle_t xTask )__FLASH_FUNC;


/*  xyStatusList_t *xyTaskGetStackInfo(void);
 *
 * @return point of taskinfo list. Node Struct : StackStatus_t
 *
 * Find all task, populate StackStatus_t structure with information about task stack,
 * and store it in the list;
 *
 * Example usage:
 *
 * void TaskFunction(void)
{
xySatusList_t *pxStatusxy;
StackStatus_t *tmpNode;
	while(1)
	{
	osDelay(1000);
	PrintLog(1468, USER_LOG, WARN_LOG, "xyTaskGetStackInfo start ");
	pxStatusxy = xyTaskGetStackInfo();
	tmpNode = pxStatusxy->xyListEnd.pxNext;
	while(tmpNode != &(pxStatusxy->xyListEnd))
	{
        PrintLog(1469, PLATFORM, WARN_LOG, "task:%s stat:%d pri:%d stack:%d usedmax:%d",\
		tmpNode->xyTaskName,tmpNode->xyStackInfo.xyStackType,tmpNode->xyPriority, tmpNode->xyStackInfo.xyInitStackSpace,tmpNode->xyStackInfo.xyStackSpace);
		tmpNode = tmpNode->pxNext;
	}
	PrintLog(1470, USER_LOG, WARN_LOG, "xyTaskGetStackInfo end ");
	vTaskFreeList(pxStatusxy);

	}
}
 *
 */
xySatusList_t *xyTaskGetStackInfo(void)__FLASH_FUNC;

void xyListInsert( xySatusList_t * const pxSatusList, StackStatus_t * const pxNewStackStatus )__FLASH_FUNC;

void xyListInitialise(xySatusList_t * const pxSatusList)__FLASH_FUNC;

StackStatus_t *xyTaskSetStatus(StackStatus_t *const pxStackStatus)__FLASH_FUNC;

/*void vTaskFreeList(xySatusList_t * pxSatusList);
 *
 * @param pxSatusList��oreturn from API xyTaskGetStackInfo
 *
 * @return NULL
 *
 * Free all nodes in pxSatusList
 */

void vTaskFreeList(xySatusList_t * pxSatusList)__FLASH_FUNC;

/* HeapInfo_t xyTaskGetHeapInfo(void);
 *
 * @return struct: HeapInfo_t
 *
 * populate HeapInfo in the struct
 *
 *
 * Example usage:
 *
 * void TaskFunction(void)
   {
    HeapInfo_t xyHeapInfo = {0};
	PrintLog(1468, USER_LOG, WARN_LOG, "xyTaskGetHeapInfo start ");
    while(1)
    {
    	xyHeapInfo = xyTaskGetHeapInfo();
    	 PrintLog(1469, PLATFORM, WARN_LOG, "DynamicHeapSize:%d Region1FreeSize:%d Region2FreeSize:%d Region3FreeSize:%d Region4FreeSize:%d ",\
    			xyHeapInfo.DynamicHeapSize,xyHeapInfo.FreeHeapSize1,xyHeapInfo.FreeHeapSize2, xyHeapInfo.FreeHeapSize3,xyHeapInfo.FreeHeapSize4);
    	osDelay(1000);
    }
    PrintLog(1470, USER_LOG, WARN_LOG, "xyTaskGetHeapInfo end ");

    }
 *
 */


 void xyTaskGetHeapInfo(HeapInfo_t *xyHeapInfo)__FLASH_FUNC;

 void vTaskGetAppsMem(AppsMemStats_t * pxHeapStats);

#endif /*OSASSITANTUTILS_H*/
