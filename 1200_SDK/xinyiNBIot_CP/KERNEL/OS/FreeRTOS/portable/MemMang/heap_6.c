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

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
	#error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif


#define heapMEMORY_BLOCK_TAIL_VALUE        ( 0xCACACACAUL )

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE	( ( size_t ) ( xHeapStructSize << 1 ) )

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE		( ( size_t ) 8 )

/* The mask of xBlockSize to get xActualBlockSize */
#define heapGET_ACTUAL_BLOCK_SIZE_MASK     ( ( size_t ) 0x00FFFFFFUL )
#define heapSET_ACTUAL_BLOCK_SIZE_MASK     ( ( size_t ) 0xFF000000UL )

/* The mask of xBlockSize to get xAlignWastedSize */
#define heapGET_ALIGN_WASTED_SIZE_MASK     ( ( size_t ) 0x7F000000UL )

/* The mask of xBlockSize to get xAlignWastedSize */
#define heapGET_USED_FLAG_MASK             ( ( size_t ) 0x80000000UL )

/* Get alignment wasted size */
#define heapGET_ACTUAL_BLOCK_SIZE( pxBlock )           ( ( ( ( pxBlock )->xBlockSize ) & heapGET_ACTUAL_BLOCK_SIZE_MASK ) >> 0 )

/* Set alignment wasted size */
#define heapSET_ACTUAL_BLOCK_SIZE( pxBlock, xSize )    ( ( ( pxBlock )->xActualBlockSize ) = ( xSize ) )

/* Get alignment wasted size */
#define heapGET_ALIGN_WASTED_SIZE( pxBlock )           ( ( ( ( pxBlock )->xBlockSize ) & heapGET_ALIGN_WASTED_SIZE_MASK ) >> 24 )

/* Set alignment wasted size */
#define heapSET_ALIGN_WASTED_SIZE( pxBlock, xSize )    ( ( ( pxBlock )->xAlignWastedSize ) = ( xSize ) )

/* Get used flag */
#define heapGET_USED_FLAG( pxBlock )                   ( ( ( ( pxBlock )->xBlockSize ) & heapGET_USED_FLAG_MASK ) >> 31 )

/* Set used flag */
#define heapSET_USED_FLAG( pxBlock, xSize )            ( ( ( pxBlock )->xUsedFlag ) = ( xSize ) )

/* Define the linked list structure.  This is used to link free blocks in order
of their memory address. */
typedef struct A_BLOCK_LINK
{
	struct A_BLOCK_LINK *pxNextBlock;		/*<< The next free block in the list, it is must be the first member. */
	union
	{
		size_t xBlockSize;					/*<< The size of the free block. */
		struct
		{
			size_t xActualBlockSize:24;		/*<< The actual block size of the block  */
			size_t xAlignWastedSize:7;		/*<< The alignment wasted bytes */
			size_t xUsedFlag:1;				/*<< The block used or not. */
		};
	};
#if( configUSE_HEAP_MALLOC_DEBUG == 1)
	char  *pcFile;							/*<< The name of file which malloc from heap . */
	int   xLine;							/*<< The line of file which malloc from heap . */
#endif
} BlockLink_t;

/* just use for xHeapUsed */
typedef struct A_BLOCK_LINK_POINT
{
	BlockLink_t *pxNextBlock;
} BlockLinkPoint_t;

/*-----------------------------------------------------------*/

#if( configUSE_HEAP_TRACE_RECORD == 1 )

#define HEAP_TRACE_MSG_RECORD_NUM    100

/* Use for "usFlag" which the member of TraceMsg_t. */
#define HEAP_TRACE_FLAG_MALLOC       1
#define HEAP_TRACE_FLAG_FREE         0

typedef struct A_MALLOC_FREE_MSG
{
	void     *pvAddr;
	TaskHandle_t  xThread;
	uint32_t  uxTickCount;
	char     *pcFile;
	uint16_t  usLine;
	uint16_t  usFlag;  /* 1: malloc  0: free */
} TraceMsg_t;

typedef struct A_HEAP_TRACE
{
	TraceMsg_t  xTraceMsg[ HEAP_TRACE_MSG_RECORD_NUM ];
	uint32_t    uxRecordCnt;
} HeapTrace_t;

#endif

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert );

/*-----------------------------------------------------------*/

#if( configUSE_HEAP_USED_LINKED_LIST == 1 )
	/* Insert current memory node to xUsedList when malloc */
	static void prvInsertBlockIntoUsedList( BlockLink_t *pxBlockToInsert );
	/* Remove current memory node from xUsedList when free */
	static void prvRemoveBlockFromUsedList( BlockLink_t *pxBlockToRemove );
#endif

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
block must by correctly byte aligned. */
static const size_t xHeapStructSize	= ( sizeof( BlockLink_t ) + ( ( size_t ) ( portBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) portBYTE_ALIGNMENT_MASK );

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart, *pxEnd = NULL;

#if( configUSE_HEAP_USED_LINKED_LIST )
/* Crate a list link to mark the used heap. */
static BlockLinkPoint_t xHeapUsed = { NULL };
#endif

#if( configUSE_HEAP_TRACE_RECORD == 1 )
/* Record heap memory trace. */
static HeapTrace_t xHeapMemoryTrace = { 0 };
#endif

/* Keeps track of the number of calls to allocate and free memory as well as the
number of free bytes remaining, but says nothing about fragmentation. */
static size_t xFreeBytesRemaining = 0U;
static size_t xMinimumEverFreeBytesRemaining = 0U;
static size_t xNumberOfSuccessfulAllocations = 0;
static size_t xNumberOfSuccessfulFrees = 0;

/*-----------------------------------------------------------*/
/*供物理层申请连续固定大小的内存块，最终的内存地址放在g_PhyMalloc_Buff数组中*/
#if( ( configUSE_HEAP_MALLOC_DEBUG == 1 ) || ( configUSE_HEAP_TRACE_RECORD == 1 ) )
void *pvPortMallocWithAlignDebug( size_t xWantedSize, eHeapAlign eAlignment, char *pcFile, int xLine )
#else
void *pvPortMallocWithAlign( size_t xWantedSize, eHeapAlign eAlignment )
#endif
{
BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
void *pvReturn = NULL;
UBaseType_t uxAlignmentSize, uxAlignmentMask;
UBaseType_t uxAlignAddr;
size_t xActualBlockSize, uxAlignWasted;
size_t xWantedSizeAlign = 0;
BlockLink_t *pxBlockAlign;

	/* The heap must be initialised before the first call to
	prvPortMalloc(). */
	configASSERT( pxEnd );

	vTaskSuspendAll();
	{
		/* Check the requested block size is not so large that the top bit is
		set.  The top bit of the block size member of the BlockLink_t structure
		is used to determine who owns the block - the application or the
		kernel, so it must be free. */
		if( ( xWantedSize & heapSET_ACTUAL_BLOCK_SIZE_MASK ) == 0 )
		{
			/* Calculate alignment size and mask */
			uxAlignmentSize = ( UBaseType_t ) eAlignment;
			uxAlignmentMask = ( UBaseType_t ) eAlignment - 1;

			/* The wanted size is increased so it can contain a BlockLink_t
			structure in addition to the requested amount of bytes. */
			if( xWantedSize > 0 )
			{
				/* When memory overflow check is enabled, the tail of four bytes is added
				to the wanted size, and will initial the tail as a known value. */
				#if( configHEAP_MEMORY_OVERFLOW_CHECK == 1 )
				{
					xWantedSize += 4;
				}
				#endif

				/* Ensure that blocks are always aligned to the required number
				of bytes. */
				if( ( xWantedSize & uxAlignmentMask ) != 0x00 )
				{
					/* Byte alignment required. */
					xWantedSize += ( uxAlignmentSize - ( xWantedSize & uxAlignmentMask ) );
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

			if( ( xWantedSizeAlign > 0 ) && ( xWantedSizeAlign <= xFreeBytesRemaining ) )
			{
				/* Traverse the list from the start	(lowest address) block until
				one	of adequate size is found. */
				pxPreviousBlock = &xStart;
				pxBlock = xStart.pxNextBlock;
				while( ( pxBlock->xBlockSize < xWantedSizeAlign ) && ( pxBlock->pxNextBlock != NULL ) )
				{
					pxPreviousBlock = pxBlock;
					pxBlock = pxBlock->pxNextBlock;
				}

				/* If the end marker was reached then a block of adequate size
				was	not found. */
				if( pxBlock != pxEnd )
				{
					/* Return the memory space pointed to - jumping over the
					BlockLink_t structure at its start. */
					pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextBlock ) + xHeapStructSize );

					/* This block is being returned for use so must be taken out
					of the list of free blocks. */
					pxPreviousBlock->pxNextBlock = pxBlock->pxNextBlock;

					/* Set the return address is satisfy alignment */
					uxAlignAddr = ( UBaseType_t ) pvReturn;

					if( ( uxAlignAddr & uxAlignmentMask ) != 0x00 )
					{
						/* Refresh return address, to satisfy alignment */
						pvReturn = ( void * ) ( ( uxAlignmentSize - ( uxAlignAddr & uxAlignmentMask ) ) + uxAlignAddr );

						/* Get the wasted bytes in alignment */
						uxAlignWasted = ( UBaseType_t ) pvReturn - uxAlignAddr;

						pxBlockAlign = pxBlock;

						/* Refresh pxBlock, move block to high address */
						pxBlock = ( BlockLink_t * ) ( ( uint8_t * ) pxBlock + uxAlignWasted );

						/* Get the actual block size */
						xActualBlockSize = heapGET_ACTUAL_BLOCK_SIZE( pxBlockAlign ) - uxAlignWasted;

						/* Add alignment wasted size to free list */
						if( uxAlignWasted > heapMINIMUM_BLOCK_SIZE )
						{
							pxBlockAlign->xBlockSize = uxAlignWasted;

							/* No alignment waste, add to free list */
							uxAlignWasted = 0;

							/* Insert the new block into the list of free blocks. */
							prvInsertBlockIntoFreeList( pxBlockAlign );
						}

						/* Set actual block size and align wasted size */
						heapSET_ACTUAL_BLOCK_SIZE( pxBlock, xActualBlockSize );
						heapSET_ALIGN_WASTED_SIZE( pxBlock, uxAlignWasted );
					}
					else
					{
						/* No align wasted */
						uxAlignWasted = 0;

						/* Get the actual block size */
						xActualBlockSize = heapGET_ACTUAL_BLOCK_SIZE( pxBlock );

						/* Set align wasted size */
						heapSET_ALIGN_WASTED_SIZE( pxBlock, 0 );
					}

					/* If the block is larger than required it can be split into
					two. */
					if( ( xActualBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
					{
						/* This block is to be split into two.  Create a new
						block following the number of bytes requested. The void
						cast is used to prevent byte alignment warnings from the
						compiler. */
						pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );

						/* Calculate the sizes of two blocks split from the
						single block. */
						pxNewBlockLink->xBlockSize = xActualBlockSize - xWantedSize;
						xActualBlockSize = xWantedSize;
						heapSET_ACTUAL_BLOCK_SIZE( pxBlock, xActualBlockSize );

						/* Insert the new block into the list of free blocks. */
						prvInsertBlockIntoFreeList( pxNewBlockLink );
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}
					
					/* Initial the tail as a known value, it will be checked when free this memory. */
					#if( configHEAP_MEMORY_OVERFLOW_CHECK == 1 )
					{
						uint32_t *puxMemTail = ( uint32_t * ) pxBlock + ( xActualBlockSize / sizeof( uint32_t ) - 1 );
						*puxMemTail = heapMEMORY_BLOCK_TAIL_VALUE;
					}
					#endif

					/* Record the allocated file and line to BlockLink_t, use for debug. */
					#if( configUSE_HEAP_MALLOC_DEBUG == 1 )
					{
						pxBlock->pcFile = pcFile;
						pxBlock->xLine = xLine;
					}
					#endif

					/* Record heap allocated memory message. */
					#if( configUSE_HEAP_TRACE_RECORD == 1 )
					{
						/* Enter critical protect. */
						taskENTER_CRITICAL();
						/* Temporary variable. */
						HeapTrace_t *pxHeapMemoryTrace = &xHeapMemoryTrace;
						uint32_t uxRecordCnt = pxHeapMemoryTrace->uxRecordCnt;
						/* Record message. */
						pxHeapMemoryTrace->xTraceMsg[ uxRecordCnt ].pvAddr = pvReturn;
						pxHeapMemoryTrace->xTraceMsg[ uxRecordCnt ].xThread = xTaskGetCurrentTaskHandle();
						pxHeapMemoryTrace->xTraceMsg[ uxRecordCnt ].uxTickCount = xTaskGetTickCount();
						pxHeapMemoryTrace->xTraceMsg[ uxRecordCnt ].pcFile = pcFile;
						pxHeapMemoryTrace->xTraceMsg[ uxRecordCnt ].usLine = ( uint16_t ) xLine;
						pxHeapMemoryTrace->xTraceMsg[ uxRecordCnt ].usFlag = HEAP_TRACE_FLAG_MALLOC;
						/* Refresh Record count. */
						pxHeapMemoryTrace->uxRecordCnt = ( uxRecordCnt + 1 ) % HEAP_TRACE_MSG_RECORD_NUM;
						/* Exit critical protect. */
						taskEXIT_CRITICAL();
					}
					#endif

					//xFreeBytesRemaining -= pxBlock->xBlockSize;
					xFreeBytesRemaining -= xActualBlockSize + uxAlignWasted;

					if( xFreeBytesRemaining < xMinimumEverFreeBytesRemaining )
					{
						xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					/* The block is being returned - it is allocated and owned
					by the application and has no "next" block. */
					heapSET_USED_FLAG( pxBlock, 1 );
					
					#if( configUSE_HEAP_USED_LINKED_LIST != 1 )
					{
						pxBlock->pxNextBlock = NULL;
					}
					#endif
					
					xNumberOfSuccessfulAllocations++;

					#if( configUSE_HEAP_USED_LINKED_LIST == 1 )
					{
						prvInsertBlockIntoUsedList( pxBlock );
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

		traceMALLOC( pvReturn, xWantedSize );
	}
	( void ) xTaskResumeAll();

	#if( configUSE_MALLOC_FAILED_HOOK == 1 )
	{
		if( pvReturn == NULL )
		{
			extern void vApplicationMallocFailedHook( void );
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

#if( configUSE_HEAP_TRACE_RECORD == 1 )
void vPortFreeWithDebug( void *pv, char *pcFile, int xLine )
#else
void vPortFree( void *pv )
#endif
{
uint8_t *puc = ( uint8_t * ) pv;
BlockLink_t *pxLink;
size_t xActualBlockSize, uxAlignWastedSize;

	if( pv != NULL )
	{
		/* Record heap allocated memory message. */
		#if( configUSE_HEAP_TRACE_RECORD == 1 )
		{
			/* Enter critical protect. */
			taskENTER_CRITICAL();
			/* Temporary variable. */
			HeapTrace_t *pxHeapMemoryTrace = &xHeapMemoryTrace;
			uint32_t uxRecordCnt = pxHeapMemoryTrace->uxRecordCnt;
			/* Record message. */
			pxHeapMemoryTrace->xTraceMsg[ uxRecordCnt ].pvAddr = pv;
			pxHeapMemoryTrace->xTraceMsg[ uxRecordCnt ].xThread = xTaskGetCurrentTaskHandle();
			pxHeapMemoryTrace->xTraceMsg[ uxRecordCnt ].uxTickCount = xTaskGetTickCount();
			pxHeapMemoryTrace->xTraceMsg[ uxRecordCnt ].pcFile = pcFile;
			pxHeapMemoryTrace->xTraceMsg[ uxRecordCnt ].usLine = ( uint16_t ) xLine;
			pxHeapMemoryTrace->xTraceMsg[ uxRecordCnt ].usFlag = HEAP_TRACE_FLAG_FREE;
			/* Refresh Record count. */
			pxHeapMemoryTrace->uxRecordCnt = ( uxRecordCnt + 1 ) % HEAP_TRACE_MSG_RECORD_NUM;
			/* Exit critical protect. */
			taskEXIT_CRITICAL();
		}
		#endif

		/* The memory being freed will have an BlockLink_t structure immediately
		before it. */
		puc -= xHeapStructSize;

		/* This casting is to keep the compiler from issuing warnings. */
		pxLink = ( void * ) puc;

		/* Check the block is actually allocated. */
		configASSERT( heapGET_USED_FLAG( pxLink ) != 0 );

		#if( configUSE_HEAP_USED_LINKED_LIST != 1 )
		{
			configASSERT( pxLink->pxNextBlock == NULL );
		}
		#endif

		/* Checked the tail value of memory block, assert if the tail value
		is wrong. */
		#if( configHEAP_MEMORY_OVERFLOW_CHECK == 1 )
		{
			xActualBlockSize = heapGET_ACTUAL_BLOCK_SIZE( pxLink );
			uint32_t *puxMemTail = ( uint32_t * ) pxLink + ( xActualBlockSize / sizeof( uint32_t ) - 1 );
			configASSERT( *puxMemTail == heapMEMORY_BLOCK_TAIL_VALUE );
		}
		#endif

		if( heapGET_USED_FLAG( pxLink ) != 0 )
		{
			#if( configUSE_HEAP_USED_LINKED_LIST != 1 )
				if( pxLink->pxNextBlock == NULL )
			#endif
				{
					/* The block is being returned to the heap - it is no longer
					allocated. */
					heapSET_USED_FLAG( pxLink, 0 );

					vTaskSuspendAll();
					{
						#if( configUSE_HEAP_USED_LINKED_LIST )
						{
							prvRemoveBlockFromUsedList( ( ( BlockLink_t * ) pxLink ) );
						}
						#endif

						if( heapGET_ALIGN_WASTED_SIZE( pxLink ) != 0 )
						{
							/* Get actual block size and align wasted size */
							xActualBlockSize = heapGET_ACTUAL_BLOCK_SIZE( pxLink );
							uxAlignWastedSize = heapGET_ALIGN_WASTED_SIZE( pxLink );

							/* Refresh pxLink to insert to free list */
							pxLink = ( BlockLink_t * ) ( ( uint8_t * ) pxLink - uxAlignWastedSize );
							pxLink->xBlockSize = xActualBlockSize + uxAlignWastedSize;
						}

						/* Add this block to the list of free blocks. */
						xFreeBytesRemaining += pxLink->xBlockSize;
						traceFREE( pv, pxLink->xBlockSize );
						prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
						xNumberOfSuccessfulFrees++;
					}
					( void ) xTaskResumeAll();
				}
			#if( configUSE_HEAP_USED_LINKED_LIST != 1 )
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

size_t xPortGetFreeHeapSize( void )
{
	return xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

size_t xPortGetMinimumEverFreeHeapSize( void )
{
	return xMinimumEverFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert )
{
BlockLink_t *pxIterator;
uint8_t *puc;

	/* Iterate through the list until a block is found that has a higher address
	than the block being inserted. */
	for( pxIterator = &xStart; pxIterator->pxNextBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextBlock )
	{
		/* Nothing to do here, just iterate to the right position. */
	}

	/* Do the block being inserted, and the block it is being inserted after
	make a contiguous block of memory? */
	puc = ( uint8_t * ) pxIterator;
	if( ( puc + pxIterator->xBlockSize ) == ( uint8_t * ) pxBlockToInsert )
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
	puc = ( uint8_t * ) pxBlockToInsert;
	if( ( puc + pxBlockToInsert->xBlockSize ) == ( uint8_t * ) pxIterator->pxNextBlock )
	{
		if( pxIterator->pxNextBlock != pxEnd )
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
	if( pxIterator != pxBlockToInsert )
	{
		pxIterator->pxNextBlock = pxBlockToInsert;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}
}
/*-----------------------------------------------------------*/

void vPortDefineHeapRegions( const HeapRegion_t * const pxHeapRegions )
{
BlockLink_t *pxFirstFreeBlockInRegion = NULL, *pxPreviousFreeBlock;
size_t xAlignedHeap;
size_t xTotalRegionSize, xTotalHeapSize = 0;
BaseType_t xDefinedRegions = 0;
size_t xAddress;
const HeapRegion_t *pxHeapRegion;

	/* Can only call once! */
	configASSERT( pxEnd == NULL );

	pxHeapRegion = &( pxHeapRegions[ xDefinedRegions ] );

	while( pxHeapRegion->xSizeInBytes > 0 )
	{
		xTotalRegionSize = pxHeapRegion->xSizeInBytes;

		/* Ensure the heap region starts on a correctly aligned boundary. */
		xAddress = ( size_t ) pxHeapRegion->pucStartAddress;
		if( ( xAddress & portBYTE_ALIGNMENT_MASK ) != 0 )
		{
			xAddress += ( portBYTE_ALIGNMENT - 1 );
			xAddress &= ~portBYTE_ALIGNMENT_MASK;

			/* Adjust the size for the bytes lost to alignment. */
			xTotalRegionSize -= xAddress - ( size_t ) pxHeapRegion->pucStartAddress;
		}

		xAlignedHeap = xAddress;

		/* Set xStart if it has not already been set. */
		if( xDefinedRegions == 0 )
		{
			/* xStart is used to hold a pointer to the first item in the list of
			free blocks.  The void cast is used to prevent compiler warnings. */
			xStart.pxNextBlock = ( BlockLink_t * ) xAlignedHeap;
			xStart.xBlockSize = ( size_t ) 0;
		}
		else
		{
			/* Should only get here if one region has already been added to the
			heap. */
			configASSERT( pxEnd != NULL );

			/* Check blocks are passed in with increasing start addresses. */
			configASSERT( xAddress > ( size_t ) pxEnd );
		}

		/* Remember the location of the end marker in the previous region, if
		any. */
		pxPreviousFreeBlock = pxEnd;

		/* pxEnd is used to mark the end of the list of free blocks and is
		inserted at the end of the region space. */
		xAddress = xAlignedHeap + xTotalRegionSize;
		xAddress -= xHeapStructSize;
		xAddress &= ~portBYTE_ALIGNMENT_MASK;
		pxEnd = ( BlockLink_t * ) xAddress;
		pxEnd->xBlockSize = 0;
		pxEnd->pxNextBlock = NULL;

		#if( configUSE_HEAP_MALLOC_DEBUG == 1 )
		{
			pxEnd->pcFile = __FILE__;
			pxEnd->xLine = __LINE__;
		}
		#endif

		/* To start with there is a single free block in this region that is
		sized to take up the entire heap region minus the space taken by the
		free block structure. */
		pxFirstFreeBlockInRegion = ( BlockLink_t * ) xAlignedHeap;
		pxFirstFreeBlockInRegion->xBlockSize = xAddress - ( size_t ) pxFirstFreeBlockInRegion;
		pxFirstFreeBlockInRegion->pxNextBlock = pxEnd;

		/* If this is not the first region that makes up the entire heap space
		then link the previous region to this region. */
		if( pxPreviousFreeBlock != NULL )
		{
			pxPreviousFreeBlock->pxNextBlock = pxFirstFreeBlockInRegion;
		}

		xTotalHeapSize += pxFirstFreeBlockInRegion->xBlockSize;

		/* Move onto the next HeapRegion_t structure. */
		xDefinedRegions++;
		pxHeapRegion = &( pxHeapRegions[ xDefinedRegions ] );
	}

	xMinimumEverFreeBytesRemaining = xTotalHeapSize;
	xFreeBytesRemaining = xTotalHeapSize;

	/* Check something was actually defined before it is accessed. */
	configASSERT( xTotalHeapSize );
}
/*-----------------------------------------------------------*/

void vPortGetHeapStats( HeapStats_t *pxHeapStats )
{
BlockLink_t *pxBlock;
size_t xBlocks = 0, xMaxSize = 0, xMinSize = portMAX_DELAY; /* portMAX_DELAY used as a portable way of getting the maximum value. */

	vTaskSuspendAll();
	{
		pxBlock = xStart.pxNextBlock;

		/* pxBlock will be NULL if the heap has not been initialised.  The heap
		is initialised automatically when the first allocation is made. */
		if( pxBlock != NULL )
		{
			do
			{
				/* Increment the number of blocks and record the largest block seen
				so far. */
				xBlocks++;

				if( pxBlock->xBlockSize > xMaxSize )
				{
					xMaxSize = pxBlock->xBlockSize;
				}

				/* Heap five will have a zero sized block at the end of each
				each region - the block is only used to link to the next
				heap region so it not a real block. */
				if( pxBlock->xBlockSize != 0 )
				{
					if( pxBlock->xBlockSize < xMinSize )
					{
						xMinSize = pxBlock->xBlockSize;
					}
				}

				/* Move to the next block in the chain until the last block is
				reached. */
				pxBlock = pxBlock->pxNextBlock;
			} while( pxBlock != pxEnd );
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

#if( configUSE_HEAP_MALLOC_DEBUG == 1 )
void *pvPortReallocWithAlignDebug( void *pvBlock, size_t size, eHeapAlign eAlignment, char *pcFile, int xLine )
#else
void *pvPortReallocWithAlign( void *pvBlock, size_t size, eHeapAlign eAlignment )
#endif
{
BlockLink_t *pxLink;
void *pvReturn = NULL;
size_t xCopySize = 0;

	if( pvBlock == NULL )
	{
		#if( configUSE_HEAP_MALLOC_DEBUG == 1 )
		{
			pvReturn = pvPortMallocWithAlignDebug( size, eAlignment, pcFile, xLine ); /*lint !e64*/
		}
		#else
		{
			pvReturn = pvPortMallocWithAlign( size, eAlignment ); /*lint !e64*/
		}
		#endif

		return pvReturn;
	}

	if( size == 0 )
	{
		vPortFree(pvBlock);
		return NULL;
	}

	pxLink = ( BlockLink_t* ) ( ( size_t ) pvBlock - xHeapStructSize );
	xCopySize = heapGET_ACTUAL_BLOCK_SIZE( pxLink ) - xHeapStructSize;

	if( size < xCopySize )
	{
		xCopySize = size;
	}

	#if( configUSE_HEAP_MALLOC_DEBUG == 1 )
	{
		pvReturn = pvPortMallocWithAlignDebug( size, eAlignment, pcFile, xLine ); /*lint !e64*/
	}
	#else
	{
		pvReturn = pvPortMallocWithAlign( size, eAlignment ); /*lint !e64*/
	}
	#endif

	if( pvReturn != NULL )
	{
		configMEMCPY( pvReturn, pvBlock, xCopySize );
		vPortFree(pvBlock);
	}

	return pvReturn;
}
/*-----------------------------------------------------------*/

#if( configUSE_HEAP_USED_LINKED_LIST == 1 )
	static void prvInsertBlockIntoUsedList( BlockLink_t *pxBlockToInsert )
	{
	BlockLink_t *pxCurBlockLink;

		configASSERT( heapGET_USED_FLAG( pxBlockToInsert ) != 0U );

		pxCurBlockLink = ( BlockLink_t * ) &xHeapUsed;

		while( ( ( size_t )( pxCurBlockLink->pxNextBlock ) < ( size_t )( pxBlockToInsert ) ) && ( pxCurBlockLink->pxNextBlock != NULL ))
		{
			pxCurBlockLink = pxCurBlockLink->pxNextBlock;
		}

		pxBlockToInsert->pxNextBlock = pxCurBlockLink->pxNextBlock;

		pxCurBlockLink->pxNextBlock = pxBlockToInsert;
	}
/*-----------------------------------------------------------*/

	static void prvRemoveBlockFromUsedList( BlockLink_t *pxBlockToRemove )
	{
	BlockLink_t *pxCurBlockLink;

		configASSERT( heapGET_USED_FLAG( pxBlockToRemove ) == 0U );

		pxCurBlockLink = ( BlockLink_t * ) &xHeapUsed;

		while( ( pxCurBlockLink->pxNextBlock != pxBlockToRemove ) && ( pxCurBlockLink->pxNextBlock != NULL ) )
		{
			pxCurBlockLink = pxCurBlockLink->pxNextBlock;
		}

		if( pxCurBlockLink->pxNextBlock != NULL )
		{
			pxCurBlockLink->pxNextBlock = pxBlockToRemove->pxNextBlock;
		}
		else
		{
			configASSERT( 0 );
		}
	}
#endif

