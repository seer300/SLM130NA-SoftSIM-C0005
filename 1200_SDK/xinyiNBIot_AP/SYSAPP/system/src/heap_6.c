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

/*
 * A sample implementation of pvPortMalloc() that allows the heap to be defined
 * across multiple non-contigous blocks and combines (coalescences) adjacent
 * memory blocks as they are freed.
 *
 * See heap_1.c, heap_2.c, heap_3.c and heap_4.c for alternative
 * implementations, and the memory management pages of http://www.FreeRTOS.org
 * for more information.
 *
 * Usage notes:
 *
 * vPortDefineHeapRegions() ***must*** be called before pvPortMalloc().
 * pvPortMalloc() will be called if any task objects (tasks, queues, event
 * groups, etc.) are created, therefore vPortDefineHeapRegions() ***must*** be
 * called before any other objects are defined.
 *
 * vPortDefineHeapRegions() takes a single parameter.  The parameter is an array
 * of HeapRegion_t structures.  HeapRegion_t is defined in portable.h as
 *
 * typedef struct HeapRegion
 * {
 *	uint8_t *pucStartAddress; << Start address of a block of memory that will be part of the heap.
 *	size_t xSizeInBytes;	  << Size of the block of memory.
 * } HeapRegion_t;
 *
 * The array is terminated using a NULL zero sized region definition, and the
 * memory regions defined in the array ***must*** appear in address order from
 * low address to high address.  So the following is a valid example of how
 * to use the function.
 *
 * HeapRegion_t xHeapRegions[] =
 * {
 * 	{ ( uint8_t * ) 0x80000000UL, 0x10000 }, << Defines a block of 0x10000 bytes starting at address 0x80000000
 * 	{ ( uint8_t * ) 0x90000000UL, 0xa0000 }, << Defines a block of 0xa0000 bytes starting at address of 0x90000000
 * 	{ NULL, 0 }                << Terminates the array.
 * };
 *
 * vPortDefineHeapRegions( xHeapRegions ); << Pass the array into vPortDefineHeapRegions().
 *
 * Note 0x80000000 is the lower address so appears in the array first.
 *
 */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "system.h"
#include "sys_mem.h"
#include "xy_memmap.h"
#include "factory_nv.h"
#include "hw_types.h"
#include "soc_control.h"
/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

//#include "portable.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE ((size_t)(xHeapStructSize << 1))

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE ((size_t)8)

/* The mask of xBlockSize to get xActualBlockSize */
#define heapGET_ACTUAL_BLOCK_SIZE_MASK ((size_t)0x00FFFFFFUL)
#define heapSET_ACTUAL_BLOCK_SIZE_MASK ((size_t)0xFF000000UL)

/* The mask of xBlockSize to get xAlignWastedSize */
#define heapGET_ALIGN_WASTED_SIZE_MASK ((size_t)0x7F000000UL)

/* The mask of xBlockSize to get xAlignWastedSize */
#define heapGET_USED_FLAG_MASK ((size_t)0x80000000UL)

/* Get alignment wasted size */
#define heapGET_ACTUAL_BLOCK_SIZE(pxBlock) ((((pxBlock)->xBlockSize) & heapGET_ACTUAL_BLOCK_SIZE_MASK) >> 0)

/* Set alignment wasted size */
#define heapSET_ACTUAL_BLOCK_SIZE(pxBlock, xSize) (((pxBlock)->xActualBlockSize) = (xSize))

/* Get alignment wasted size */
#define heapGET_ALIGN_WASTED_SIZE(pxBlock) ((((pxBlock)->xBlockSize) & heapGET_ALIGN_WASTED_SIZE_MASK) >> 24)

/* Set alignment wasted size */
#define heapSET_ALIGN_WASTED_SIZE(pxBlock, xSize) (((pxBlock)->xAlignWastedSize) = (xSize))

/* Get used flag */
#define heapGET_USED_FLAG(pxBlock) ((((pxBlock)->xBlockSize) & heapGET_USED_FLAG_MASK) >> 31)

#define portBYTE_ALIGNMENT 4
#define portBYTE_ALIGNMENT_MASK (0x0003)

typedef unsigned long UBaseType_t;
typedef long BaseType_t;

typedef struct HeapRegion
{
	uint8_t *pucStartAddress;
	size_t xSizeInBytes;
} HeapRegion_t;

typedef struct xHeapStats
{
	size_t xAvailableHeapSpaceInBytes;		/* The total heap size currently available - this is the sum of all the free blocks, not the largest block that can be allocated. */
	size_t xSizeOfLargestFreeBlockInBytes;	/* The maximum size, in bytes, of all the free blocks within the heap at the time vPortGetHeapStats() is called. */
	size_t xSizeOfSmallestFreeBlockInBytes; /* The minimum size, in bytes, of all the free blocks within the heap at the time vPortGetHeapStats() is called. */
	size_t xNumberOfFreeBlocks;				/* The number of free memory blocks within the heap at the time vPortGetHeapStats() is called. */
	size_t xMinimumEverFreeBytesRemaining;	/* The minimum amount of total free memory (sum of all free blocks) there has been in the heap since the system booted. */
	size_t xNumberOfSuccessfulAllocations;	/* The number of calls to pvPortMalloc() that have returned a valid memory block. */
	size_t xNumberOfSuccessfulFrees;		/* The number of calls to vPortFree() that has successfully freed a block of memory. */
} HeapStats_t;

#define portMAX_DELAY                       (0xffffffffUL)
#define traceMALLOC(pvReturn, xWantedSize)
#define traceFREE(pv, xBlockSize)
#define mtCOVERAGE_TEST_MARKER()

#define configUSE_HEAP_USED_LINKED_LIST      (1)
#define configASSERT(pxEnd)                   xy_assert(pxEnd)
#define vTaskSuspendAll()                     DisablePrimask()
#define xTaskResumeAll()                      EnablePrimask()

#define taskENTER_CRITICAL()                  DisablePrimask()
#define taskEXIT_CRITICAL()                   EnablePrimask()

#define EXT_MEM_TRACE                         0
/* Define the linked list structure.  This is used to link free blocks in order
of their memory address. */
typedef struct A_BLOCK_LINK
{
	struct A_BLOCK_LINK *pxNextBlock; /*<< The next free block in the list, it is must be the first member. */
	union
	{
		size_t xBlockSize; /*<< The size of the free block. */
		struct
		{
			size_t xActualBlockSize : 24; /*<< The actual block size of the block  */
			size_t xAlignWastedSize : 7;  /*<< The alignment wasted bytes */
			size_t xUsedFlag : 1;		  /*<< The block used or not. */
		};
	};
#if (configUSE_HEAP_MALLOC_DEBUG == 1)
	char *pcFile; /*<< The name of file which malloc from heap . */
	int xLine;	  /*<< The line of file which malloc from heap . */
#endif
} BlockLink_t;

/* just use for xHeapUsed */
typedef struct A_BLOCK_LINK_POINT
{
	BlockLink_t *pxNextBlock;
} BlockLinkPoint_t;

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList(BlockLink_t *pxBlockToInsert);

/*-----------------------------------------------------------*/

#if (configUSE_HEAP_USED_LINKED_LIST == 1)
/* Insert current memory node to xUsedList when malloc */
static void prvInsertBlockIntoUsedList(BlockLink_t *pxBlockToInsert);
/* Remove current memory node from xUsedList when free */
static void prvRemoveBlockFromUsedList(BlockLink_t *pxBlockToRemove);
#endif

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
block must by correctly byte aligned. */
static const size_t xHeapStructSize = (sizeof(BlockLink_t) + ((size_t)(portBYTE_ALIGNMENT - 1))) & ~((size_t)portBYTE_ALIGNMENT_MASK);

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart, *pxEnd = NULL;

#if (configUSE_HEAP_USED_LINKED_LIST)
/* Crate a list link to mark the used heap  */
static BlockLinkPoint_t xHeapUsed = {NULL};
#endif

/* Keeps track of the number of calls to allocate and free memory as well as the
number of free bytes remaining, but says nothing about fragmentation. */
static size_t xFreeBytesRemaining = 0U;
static size_t xMinimumEverFreeBytesRemaining = 0U;
static size_t xNumberOfSuccessfulAllocations = 0;
static size_t xNumberOfSuccessfulFrees = 0;
static size_t xFreeBytesTotal = 0U;

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
member of an BlockLink_t structure is set then the block belongs to the
application.  When the bit is free the block is still part of the free heap
space. */
static size_t xBlockAllocatedBit = 0;

/*-----------------------------------------------------------*/
__OPENCPU_FUNC void *pvPortMallocWithAlign(size_t xWantedSize, eHeapAlign eAlignment
#if (configUSE_HEAP_MALLOC_DEBUG == 1)
		, char *pcFile, int xLine
#endif
	)
{
	BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
	void *pvReturn = NULL;
	UBaseType_t uxAlignmentSize, uxAlignmentMask;
	UBaseType_t uxAlignAddr;
	size_t xActualBlockSize, uxAlignWasted;
	size_t xWantedSizeAlign = 0;
	BlockLink_t *pxBlockAlign;
//	extern uint8_t *_Heap_Begin;
	/* The heap must be initialised before the first call to
	prvPortMalloc(). */
	configASSERT(pxEnd);

	vTaskSuspendAll();
	{
		/* Check the requested block size is not so large that the top bit is
		set.  The top bit of the block size member of the BlockLink_t structure
		is used to determine who owns the block - the application or the
		kernel, so it must be free. */
		if ((xWantedSize & heapSET_ACTUAL_BLOCK_SIZE_MASK) == 0)
		{
			/* Calculate alignment size and mask */
			uxAlignmentSize = (UBaseType_t)eAlignment;
			uxAlignmentMask = (UBaseType_t)eAlignment - 1;

			/* The wanted size is increased so it can contain a BlockLink_t
			structure in addition to the requested amount of bytes. */
			if (xWantedSize > 0)
			{
				/* Ensure that blocks are always aligned to the required number
				of bytes. */
				if ((xWantedSize & uxAlignmentMask) != 0x00)
				{
					/* Byte alignment required. */
					xWantedSize += (uxAlignmentSize - (xWantedSize & uxAlignmentMask));
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}

				/* Actual wanted size */
				xWantedSize += xHeapStructSize;

				/* Ensure that the return address has enough space for byte alignment. */
				xWantedSizeAlign = xWantedSize + uxAlignmentSize;
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}

			if ((xWantedSizeAlign > 0) && (xWantedSizeAlign <= xFreeBytesRemaining))
			{
				/* Traverse the list from the start	(lowest address) block until
				one	of adequate size is found. */
				pxPreviousBlock = &xStart;
				pxBlock = xStart.pxNextBlock;
				while ((pxBlock->xBlockSize < xWantedSizeAlign) && (pxBlock->pxNextBlock != NULL))
				{
					pxPreviousBlock = pxBlock;
					pxBlock = pxBlock->pxNextBlock;
				}

				/* If the end marker was reached then a block of adequate size
				was	not found. */
				if (pxBlock != pxEnd)
				{
					if(pxBlock->pxNextBlock == NULL || ((uint32_t)pxBlock & 0x03))
						xy_assert(0);
					/* Return the memory space pointed to - jumping over the
					BlockLink_t structure at its start. */
					pvReturn = (void *)(((uint8_t *)pxPreviousBlock->pxNextBlock) + xHeapStructSize);

					/* This block is being returned for use so must be taken out
					of the list of free blocks. */
					pxPreviousBlock->pxNextBlock = pxBlock->pxNextBlock;

					/* Set the return address is satisfy alignment */
					uxAlignAddr = (UBaseType_t)pvReturn;

					if ((uxAlignAddr & uxAlignmentMask) != 0x00)
					{
						/* Refresh return address, to satisfy alignment */
						pvReturn = (void *)((uxAlignmentSize - (uxAlignAddr & uxAlignmentMask)) + uxAlignAddr);

						/* Get the wasted bytes in alignment */
						uxAlignWasted = (UBaseType_t)pvReturn - uxAlignAddr;

						pxBlockAlign = pxBlock;

						/* Refresh pxBlock, move block to high address */
						pxBlock = (BlockLink_t *)((uint8_t *)pxBlock + uxAlignWasted);

						/* Get the actual block size */
						xActualBlockSize = heapGET_ACTUAL_BLOCK_SIZE(pxBlockAlign) - uxAlignWasted;

						/* Add alignment wasted size to free list */
						if (uxAlignWasted > heapMINIMUM_BLOCK_SIZE)
						{
							pxBlockAlign->xBlockSize = uxAlignWasted;

							/* No alignment waste, add to free list */
							uxAlignWasted = 0;

							/* Insert the new block into the list of free blocks. */
							prvInsertBlockIntoFreeList(pxBlockAlign);
						}

						/* Set actual block size and align wasted size */
						heapSET_ACTUAL_BLOCK_SIZE(pxBlock, xActualBlockSize);
						heapSET_ALIGN_WASTED_SIZE(pxBlock, uxAlignWasted);
					}
					else
					{
						/* No align wasted */
						uxAlignWasted = 0;

						/* Get the actual block size */
						xActualBlockSize = heapGET_ACTUAL_BLOCK_SIZE(pxBlock);

						/* Set align wasted size */
						heapSET_ALIGN_WASTED_SIZE(pxBlock, 0);
					}

					/* If the block is larger than required it can be split into
					two. */
					if ((xActualBlockSize - xWantedSize) > heapMINIMUM_BLOCK_SIZE)
					{
						/* This block is to be split into two.  Create a new
						block following the number of bytes requested. The void
						cast is used to prevent byte alignment warnings from the
						compiler. */
						pxNewBlockLink = (void *)(((uint8_t *)pxBlock) + xWantedSize);

						/* Calculate the sizes of two blocks split from the
						single block. */
						pxNewBlockLink->xBlockSize = xActualBlockSize - xWantedSize;
						xActualBlockSize = xWantedSize;
						heapSET_ACTUAL_BLOCK_SIZE(pxBlock, xActualBlockSize);

						/* Insert the new block into the list of free blocks. */
						prvInsertBlockIntoFreeList(pxNewBlockLink);
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					// xFreeBytesRemaining -= pxBlock->xBlockSize;
					xFreeBytesRemaining -= xActualBlockSize + uxAlignWasted;

					if (xFreeBytesRemaining < xMinimumEverFreeBytesRemaining)
					{
						xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					/* The block is being returned - it is allocated and owned
					by the application and has no "next" block. */
					pxBlock->xBlockSize |= xBlockAllocatedBit;

#if (configUSE_HEAP_USED_LINKED_LIST != 1)
					{
						pxBlock->pxNextBlock = NULL;
					}
#endif

					xNumberOfSuccessfulAllocations++;

#if (configUSE_HEAP_MALLOC_DEBUG == 1)
					{
						pxBlock->pcFile = pcFile;
						pxBlock->xLine = xLine;
					}
#endif

#if (configUSE_HEAP_USED_LINKED_LIST == 1)
					{
						prvInsertBlockIntoUsedList(pxBlock);
					}
#endif
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		traceMALLOC(pvReturn, xWantedSize);
	}
	(void)xTaskResumeAll();

#if (configUSE_MALLOC_FAILED_HOOK == 1)
	{
		if (pvReturn == NULL)
		{
			extern void vApplicationMallocFailedHook(void);
			vApplicationMallocFailedHook();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
#endif

	return pvReturn;
}
/*-----------------------------------------------------------*/

__OPENCPU_FUNC void vPortFree(void *pv)
{
	uint8_t *puc = (uint8_t *)pv;
	BlockLink_t *pxLink;
	size_t xActualBlockSize, uxAlignWastedSize;

	if (pv != NULL)
	{
		/* The memory being freed will have an BlockLink_t structure immediately
		before it. */
		puc -= xHeapStructSize;

		/* This casting is to keep the compiler from issuing warnings. */
		pxLink = (void *)puc;

		/* Check the block is actually allocated. */
		configASSERT((pxLink->xBlockSize & xBlockAllocatedBit) != 0);

#if (!configUSE_HEAP_USED_LINKED_LIST)
		{
			configASSERT(pxLink->pxNextBlock == NULL);
		}
#endif

		if ((pxLink->xBlockSize & xBlockAllocatedBit) != 0)
		{
#if (configUSE_HEAP_USED_LINKED_LIST != 1)
			if (pxLink->pxNextBlock == NULL)
#endif
			{
				/* The block is being returned to the heap - it is no longer
				allocated. */
				pxLink->xBlockSize &= ~xBlockAllocatedBit;

				vTaskSuspendAll();
				{
#if (configUSE_HEAP_USED_LINKED_LIST)
					{
						prvRemoveBlockFromUsedList(((BlockLink_t *)pxLink));
					}
#endif

					if (heapGET_ALIGN_WASTED_SIZE(pxLink) != 0)
					{
						/* Get actual block size and align wasted size */
						xActualBlockSize = heapGET_ACTUAL_BLOCK_SIZE(pxLink);
						uxAlignWastedSize = heapGET_ALIGN_WASTED_SIZE(pxLink);

						/* Refresh pxLink to insert to free list */
						pxLink = (BlockLink_t *)((uint8_t *)pxLink - uxAlignWastedSize);
						pxLink->xBlockSize = xActualBlockSize + uxAlignWastedSize;
					}

					/* Add this block to the list of free blocks. */
					xFreeBytesRemaining += pxLink->xBlockSize;
					traceFREE(pv, pxLink->xBlockSize);
					prvInsertBlockIntoFreeList(((BlockLink_t *)pxLink));
					xNumberOfSuccessfulFrees++;
				}
				(void)xTaskResumeAll();
			}
#if (configUSE_HEAP_USED_LINKED_LIST != 1)
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
#endif
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
}
/*-----------------------------------------------------------*/

__FLASH_FUNC size_t xPortGetFreeHeapSize(void)
{
	return xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

__FLASH_FUNC size_t xPortGetMinimumEverFreeHeapSize(void)
{
	return xMinimumEverFreeBytesRemaining;
}

__OPENCPU_FUNC void xPortResetMinimumEverFreeHeapSize(void)
{
	xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
}
__FLASH_FUNC size_t xPorGetTotalHeapSize(void)
{
	return xFreeBytesTotal;
}

/*-----------------------------------------------------------*/

__OPENCPU_FUNC static void prvInsertBlockIntoFreeList(BlockLink_t *pxBlockToInsert)
{
	BlockLink_t *pxIterator;
	uint8_t *puc;

	/* Iterate through the list until a block is found that has a higher address
	than the block being inserted. */
	for (pxIterator = &xStart; pxIterator->pxNextBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextBlock)
	{
		/* Nothing to do here, just iterate to the right position. */
	}

	/* Do the block being inserted, and the block it is being inserted after
	make a contiguous block of memory? */
	puc = (uint8_t *)pxIterator;
	if ((puc + pxIterator->xBlockSize) == (uint8_t *)pxBlockToInsert)
	{
		pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
		pxBlockToInsert = pxIterator;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}

	/* Do the block being inserted, and the block it is being inserted before
	make a contiguous block of memory? */
	puc = (uint8_t *)pxBlockToInsert;
	if ((puc + pxBlockToInsert->xBlockSize) == (uint8_t *)pxIterator->pxNextBlock)
	{
		if (pxIterator->pxNextBlock != pxEnd)
		{
			/* Form one big block from the two blocks. */
			pxBlockToInsert->xBlockSize += pxIterator->pxNextBlock->xBlockSize;
			pxBlockToInsert->pxNextBlock = pxIterator->pxNextBlock->pxNextBlock;
		}
		else
		{
			pxBlockToInsert->pxNextBlock = pxEnd;
		}
	}
	else
	{
		pxBlockToInsert->pxNextBlock = pxIterator->pxNextBlock;
	}

	/* If the block being inserted plugged a gab, so was merged with the block
	before and the block after, then it's pxNextBlock pointer will have
	already been set, and should not be set here as that would make it point
	to itself. */
	if (pxIterator != pxBlockToInsert)
	{
		pxIterator->pxNextBlock = pxBlockToInsert;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}
}
/*-----------------------------------------------------------*/

__FLASH_FUNC void vPortDefineHeapRegions(const HeapRegion_t *const pxHeapRegions)
{
	BlockLink_t *pxFirstFreeBlockInRegion = NULL, *pxPreviousFreeBlock;
	size_t xAlignedHeap;
	size_t xTotalRegionSize, xTotalHeapSize = 0;
	BaseType_t xDefinedRegions = 0;
	size_t xAddress;
	const HeapRegion_t *pxHeapRegion;

	/* Can only call once! */
	configASSERT(pxEnd == NULL);

	pxHeapRegion = &(pxHeapRegions[xDefinedRegions]);

	while (pxHeapRegion->xSizeInBytes > 0)
	{
		xTotalRegionSize = pxHeapRegion->xSizeInBytes;

		/* Ensure the heap region starts on a correctly aligned boundary. */
		xAddress = (size_t)pxHeapRegion->pucStartAddress;
		if ((xAddress & portBYTE_ALIGNMENT_MASK) != 0)
		{
			xAddress += (portBYTE_ALIGNMENT - 1);
			xAddress &= ~portBYTE_ALIGNMENT_MASK;

			/* Adjust the size for the bytes lost to alignment. */
			xTotalRegionSize -= xAddress - (size_t)pxHeapRegion->pucStartAddress;
		}

		xAlignedHeap = xAddress;

		/* Set xStart if it has not already been set. */
		if (xDefinedRegions == 0)
		{
			/* xStart is used to hold a pointer to the first item in the list of
			free blocks.  The void cast is used to prevent compiler warnings. */
			xStart.pxNextBlock = (BlockLink_t *)xAlignedHeap;
			xStart.xBlockSize = (size_t)0;
		}
		else
		{
			/* Should only get here if one region has already been added to the
			heap. */
			configASSERT(pxEnd != NULL);

			/* Check blocks are passed in with increasing start addresses. */
			configASSERT(xAddress > (size_t)pxEnd);
		}

		/* Remember the location of the end marker in the previous region, if
		any. */
		pxPreviousFreeBlock = pxEnd;

		/* pxEnd is used to mark the end of the list of free blocks and is
		inserted at the end of the region space. */
		xAddress = xAlignedHeap + xTotalRegionSize;
		xAddress -= xHeapStructSize;
		xAddress &= ~portBYTE_ALIGNMENT_MASK;
		pxEnd = (BlockLink_t *)xAddress;
		pxEnd->xBlockSize = 0;
		pxEnd->pxNextBlock = NULL;

		/* To start with there is a single free block in this region that is
		sized to take up the entire heap region minus the space taken by the
		free block structure. */
		pxFirstFreeBlockInRegion = (BlockLink_t *)xAlignedHeap;
		pxFirstFreeBlockInRegion->xBlockSize = xAddress - (size_t)pxFirstFreeBlockInRegion;
		pxFirstFreeBlockInRegion->pxNextBlock = pxEnd;

		/* If this is not the first region that makes up the entire heap space
		then link the previous region to this region. */
		if (pxPreviousFreeBlock != NULL)
		{
			pxPreviousFreeBlock->pxNextBlock = pxFirstFreeBlockInRegion;
		}

		xTotalHeapSize += pxFirstFreeBlockInRegion->xBlockSize;

		/* Move onto the next HeapRegion_t structure. */
		xDefinedRegions++;
		pxHeapRegion = &(pxHeapRegions[xDefinedRegions]);
	}

	xMinimumEverFreeBytesRemaining = xTotalHeapSize;
	xFreeBytesRemaining = xTotalHeapSize;
	xFreeBytesTotal = xTotalHeapSize;
	/* Check something was actually defined before it is accessed. */
	configASSERT(xTotalHeapSize);

	/* Work out the position of the top bit in a size_t variable. */
	xBlockAllocatedBit = ((size_t)1) << ((sizeof(size_t) * heapBITS_PER_BYTE) - 1);
}
/*-----------------------------------------------------------*/

__FLASH_FUNC void vPortGetHeapStats(HeapStats_t *pxHeapStats)
{
	BlockLink_t *pxBlock;
	size_t xBlocks = 0, xMaxSize = 0, xMinSize = portMAX_DELAY; /* portMAX_DELAY used as a portable way of getting the maximum value. */

	vTaskSuspendAll();
	{
		pxBlock = xStart.pxNextBlock;

		/* pxBlock will be NULL if the heap has not been initialised.  The heap
		is initialised automatically when the first allocation is made. */
		if (pxBlock != NULL)
		{
			do
			{
				/* Increment the number of blocks and record the largest block seen
				so far. */
				xBlocks++;

				if (pxBlock->xBlockSize > xMaxSize)
				{
					xMaxSize = pxBlock->xBlockSize;
				}

				/* Heap five will have a zero sized block at the end of each
				each region - the block is only used to link to the next
				heap region so it not a real block. */
				if (pxBlock->xBlockSize != 0)
				{
					if (pxBlock->xBlockSize < xMinSize)
					{
						xMinSize = pxBlock->xBlockSize;
					}
				}

				/* Move to the next block in the chain until the last block is
				reached. */
				pxBlock = pxBlock->pxNextBlock;
			} while (pxBlock != pxEnd);
		}
	}
	xTaskResumeAll();

	pxHeapStats->xSizeOfLargestFreeBlockInBytes = xMaxSize;
	pxHeapStats->xSizeOfSmallestFreeBlockInBytes = xMinSize;
	pxHeapStats->xNumberOfFreeBlocks = xBlocks;

	taskENTER_CRITICAL();
	{
		pxHeapStats->xAvailableHeapSpaceInBytes = xFreeBytesRemaining;
		pxHeapStats->xNumberOfSuccessfulAllocations = xNumberOfSuccessfulAllocations;
		pxHeapStats->xNumberOfSuccessfulFrees = xNumberOfSuccessfulFrees;
		pxHeapStats->xMinimumEverFreeBytesRemaining = xMinimumEverFreeBytesRemaining;
	}
	taskEXIT_CRITICAL();
}

/*-----------------------------------------------------------*/

__OPENCPU_FUNC void *pvPortReallocWithAlign(void *pvBlock, size_t size, eHeapAlign eAlignment
#if (configUSE_HEAP_MALLOC_DEBUG == 1)
		, char *pcFile, int xLine
#endif
)
{
	BlockLink_t *pxLink;
	void *pvReturn = NULL;
	size_t xCopySize = 0;

	if (pvBlock == NULL)
	{
			pvReturn = pvPortMallocWithAlign(size, eAlignment
#if (configUSE_HEAP_MALLOC_DEBUG == 1)
					, pcFile, xLine

#endif
			); /*lint !e64*/
		return pvReturn;
	}

	if (size == 0)
	{
		vPortFree(pvBlock);
		return NULL;
	}

	pxLink = (BlockLink_t *)((size_t)pvBlock - xHeapStructSize);
	xCopySize = heapGET_ACTUAL_BLOCK_SIZE(pxLink) - xHeapStructSize;

	if (size < xCopySize)
	{
		xCopySize = size;
	}


	pvReturn = pvPortMallocWithAlign(size, eAlignment
#if (configUSE_HEAP_MALLOC_DEBUG == 1)
				, pcFile, xLine
#endif
	); /*lint !e64*/

	if (pvReturn != NULL)
	{
		memcpy(pvReturn, pvBlock, xCopySize);
		vPortFree(pvBlock);
	}

	return pvReturn;
}

/*-----------------------------------------------------------*/

#if (configUSE_HEAP_USED_LINKED_LIST == 1)
__OPENCPU_FUNC static void prvInsertBlockIntoUsedList(BlockLink_t *pxBlockToInsert)
{
	BlockLink_t *pxCurBlockLink;

	configASSERT(heapGET_USED_FLAG(pxBlockToInsert) != 0U);

	pxCurBlockLink = (BlockLink_t *)&xHeapUsed;

	while (((size_t)(pxCurBlockLink->pxNextBlock) < (size_t)(pxBlockToInsert)) && (pxCurBlockLink->pxNextBlock != NULL))
	{
		pxCurBlockLink = pxCurBlockLink->pxNextBlock;
	}

	pxBlockToInsert->pxNextBlock = pxCurBlockLink->pxNextBlock;

	pxCurBlockLink->pxNextBlock = pxBlockToInsert;
}
/*-----------------------------------------------------------*/

__OPENCPU_FUNC static void prvRemoveBlockFromUsedList(BlockLink_t *pxBlockToRemove)
{
	BlockLink_t *pxCurBlockLink;

	configASSERT(heapGET_USED_FLAG(pxBlockToRemove) == 0U);

	pxCurBlockLink = (BlockLink_t *)&xHeapUsed;

	while (pxCurBlockLink->pxNextBlock != pxBlockToRemove)
	{
		pxCurBlockLink = pxCurBlockLink->pxNextBlock;
	}

	pxCurBlockLink->pxNextBlock = pxBlockToRemove->pxNextBlock;
}
#endif

#if EXT_MEM_TRACE
typedef struct _MemTrace
{
	void *mem;
	char *file;
	uint16_t line;
	uint16_t isfree;
} MemTrace_t;

/* ring buffer control */
typedef struct _RBCtrl
{
	void *startAddr;
	uint32_t uintSize;
	uint32_t pos;
	uint32_t size;
} RBCtrl_t;

void getMemTrace(MemTrace_t *mt)
{
	BlockLink_t *pxLink;
	pxLink = (BlockLink_t *)((uint8_t *)(mt->mem) - xHeapStructSize);
	mt->file = pxLink->pcFile;
	mt->line = (uint16_t)(pxLink->xLine);
}

/* init ring buffer */
void RBInit(RBCtrl_t *rb, uint32_t uintsize, uint32_t size)
{
	rb->uintSize = uintsize;
	rb->size = size;
	rb->startAddr = xy_malloc(rb->uintSize * rb->size);
	rb->pos = 0;
}
/* push item into ring buffer */
void RBPush(RBCtrl_t *rb, void *uint, uint32_t uintsize)
{
	if (rb->pos >= rb->size)
	{
		rb->pos = 0;
	}
	memcpy((uint8_t *)(rb->startAddr) + (rb->pos) * uintsize, (uint8_t *)uint, uintsize);
	rb->pos++;
}

RBCtrl_t mTraceRB;

void extMemTraceInit(void)
{
	static uint8_t init = 0;

	if (!init)
	{
		RBInit(&mTraceRB, sizeof(MemTrace_t), 100);
		init = 1;
	}
}

/* ??????EXT_MEM_TRACE_NUM?��??????????? */
void extMemTrace(void *mem, uint8_t isfree)
{
	MemTrace_t mt;
	if (mem == NULL)
	{
		return;
	}
	mt.mem = mem;
	getMemTrace(&mt);
	if (isfree)
	{
		mt.isfree = 1;
	}
	else
	{
		mt.isfree = 0;
	}
	DisablePrimask();
	RBPush(&mTraceRB, &mt, sizeof(MemTrace_t));
	EnablePrimask();
}
#else
#define extMemTraceInit()
#define extMemTrace(mem, isfree) ;
#endif


__OPENCPU_FUNC size_t CalRealSize(void *mem)
{
	size_t size = 0;
	BlockLink_t *pxLink;

	pxLink = (BlockLink_t *)((char *)mem - xHeapStructSize);
	size += heapGET_ACTUAL_BLOCK_SIZE(pxLink);
	size += heapGET_ALIGN_WASTED_SIZE(pxLink);

	return size;
}

__OPENCPU_FUNC void *aligned_zalloc_heap6(unsigned int required_bytes, unsigned int alignment, uint8_t loose
#if (configUSE_HEAP_MALLOC_DEBUG == 1)
		, char *pcFile, int xLine
#endif
)
{

	if (required_bytes == 0)
		return (void *)0; /*lint !e64*/

	void *p1 = pvPortMallocWithAlign(required_bytes, alignment
#if (configUSE_HEAP_MALLOC_DEBUG == 1)
			, pcFile, xLine
#endif
			);

	if (!p1)
	{
		if(loose)
			return (void *)0;
		else
			xy_assert(0);
	}

	memset(p1, 0x00, required_bytes);

	extMemTrace(p1, 0);
	return p1;
}

__OPENCPU_FUNC void *aligned_malloc_heap6(unsigned int required_bytes, unsigned int alignment, uint8_t loose
#if (configUSE_HEAP_MALLOC_DEBUG == 1)
		, char *pcFile, int xLine
#endif
		)
{
	if (required_bytes == 0)
		return (void *)0; /*lint !e64*/

	void *p1 = pvPortMallocWithAlign(required_bytes, alignment
#if (configUSE_HEAP_MALLOC_DEBUG == 1)
			, pcFile, xLine
#endif
			);

	if (!p1)
	{
		if(loose)
			return (void *)0;
		else
			xy_assert(0);
	}

	extMemTrace(p1, 0);
	return p1;
}

__OPENCPU_FUNC void *aligned_r_malloc_heap6(void *pvBlock, size_t size, eHeapAlign eAlignment, uint8_t loose
#if (configUSE_HEAP_MALLOC_DEBUG == 1)
		, char *pcFile, int xLine
#endif
)
{
	if (pvBlock == NULL)
	{
		return aligned_malloc_heap6(size, eAlignment, loose
#if (configUSE_HEAP_MALLOC_DEBUG == 1)
				, pcFile, xLine
#endif
				); /*lint !e64*/
	}

	if (size == 0)
		return (void *)0; /*lint !e64*/

	void *p1 = pvPortReallocWithAlign(pvBlock, size, eAlignment
#if (configUSE_HEAP_MALLOC_DEBUG == 1)
			, pcFile, xLine
#endif
	);

	if (!p1)
	{
		if(loose)
			return (void *)0;
		else
			xy_assert(0);
	}

	extMemTrace(p1, 0);
	return p1;
}

__OPENCPU_FUNC void aligned_free_heap6(void *p2)
{
	if (p2 == NULL)
		return;
	extMemTrace(p2, 1);
	vPortFree(p2);
}

__FLASH_FUNC void VPortDefineHeapRegions(void)
{
	uint32_t heap_addr;
	uint32_t heap_end;

	extern uint8_t *_Heap_Begin;
	heap_addr = (uint32_t)&_Heap_Begin;
#if DYN_LOAD
	extern const int32_t SO_AVAILABLE;
	if (SO_AVAILABLE)
	{
		extern const uint32_t HEAP_END;
		heap_end = HEAP_END;
	}
	else
	{
		extern uint32_t _Heap_Limit;
		heap_end = (uint32_t)&_Heap_Limit;
	}
#else
	extern uint32_t _Heap_Limit;
	heap_end = (uint32_t)&_Heap_Limit;
#endif
	/*CP向AP借用一段内存*/
	if (HWREG(BAK_MEM_CP_USED_APRAM_ADDR) != 0)
		heap_addr = heap_addr + 0x400 * HWREGB(BAK_MEM_CP_USED_APRAM_SIZE);
	
	xy_assert(heap_addr < heap_end);
	
	const HeapRegion_t xHeapRegions[] =
		{
			{(uint8_t *)heap_addr, heap_end - heap_addr},
			{NULL, 0}};

	pxEnd = NULL;

	vPortDefineHeapRegions(xHeapRegions);

	extMemTraceInit();
	
	soc_mem_control();
}


__FLASH_FUNC void GetApHeapInfo(char *info)
{
	uint32_t heap_size = 0;
	HeapStats_t heapStatus;
	uint16_t str_len = 0;
	uint32_t heap_addr;
	uint32_t heap_end;
	extern uint8_t *_Heap_Begin;
	extern uint32_t _Heap_Limit;
	heap_addr = (uint32_t)&_Heap_Begin;
#if DYN_LOAD
	extern const int32_t SO_AVAILABLE;
	if (SO_AVAILABLE)
	{
		extern const uint32_t HEAP_END;
		heap_end = HEAP_END;
	}
	else
	{
		extern uint32_t _Heap_Limit;
		heap_end = (uint32_t)&_Heap_Limit;
	}
#else
	extern uint32_t _Heap_Limit;
	heap_end = (uint32_t)&_Heap_Limit;
#endif
	/*CP向AP借用一段内存*/
	if (HWREG(BAK_MEM_CP_USED_APRAM_ADDR) != 0)
		heap_addr = heap_addr + 0x400 * HWREGB(BAK_MEM_CP_USED_APRAM_SIZE);
	
	heap_size = heap_end - heap_addr;

	taskENTER_CRITICAL();
	vPortGetHeapStats(&heapStatus);
	taskEXIT_CRITICAL();

	str_len += sprintf(info + str_len, "\r\n+NUESTATS:APPSMEM,Heap Size:%lu\r\n", heap_size);
	str_len += sprintf(info + str_len, "\r\n+NUESTATS:APPSMEM,Heap Peak Used:%lu\r\n", heap_size - heapStatus.xMinimumEverFreeBytesRemaining);
	str_len += sprintf(info + str_len, "\r\n+NUESTATS:APPSMEM,Current Allocated:%lu\r\n", heap_size - heapStatus.xAvailableHeapSpaceInBytes);
	str_len += sprintf(info + str_len, "\r\n+NUESTATS:APPSMEM,Total Free:%d\r\n", heapStatus.xAvailableHeapSpaceInBytes);
	str_len += sprintf(info + str_len, "\r\n+NUESTATS:APPSMEM,Max Free:%d\r\n", heapStatus.xSizeOfLargestFreeBlockInBytes);
	str_len += sprintf(info + str_len, "\r\n+NUESTATS:APPSMEM,Num Allocs:%d\r\n", heapStatus.xNumberOfSuccessfulAllocations);
	str_len += sprintf(info + str_len, "\r\n+NUESTATS:APPSMEM,Num Frees:%d\r\n", heapStatus.xNumberOfSuccessfulFrees);
	strcat((char *)info, "\r\nOK\r\n");

}

