
/*
 *
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* use user RTOS api */
#include "ctlw_abstract_os.h"
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE !(FALSE)
#endif

/**************************************************************************
                              THREAD LOG
***************************************************************************/

#define heap_printf(...) lwm2m_printf(__VA_ARGS__);

/**************************************************************************
                              THREAD TRACE
***************************************************************************/

#ifdef traceFREE
#undef traceFREE
#endif
#define traceFREE(a, b)

#ifdef traceMALLOC
#undef traceMALLOC
#endif
#define traceMALLOC(a, b)
#define mtCOVERAGE_TEST_MARKER()

/**************************************************************************
                              THREAD LOCK
***************************************************************************/

#define THREAD_LOCK_INIT() thread_mutex_init(&mutex, "heap mutex");
#define CTIOT_THREAD_LOCK() thread_mutex_lock(&mutex)
#define CTIOT_THREAD_UNLOCK() thread_mutex_unlock(&mutex)

thread_mutex_t mutex;

#define HEAP_DEBUG_EN 1

#define CTIOT_TOTAL_HEAP_SIZE 1024 * 40

/**************************************************************************
                              CTIOT MALLOC
***************************************************************************/
// add 8 bytes to record footprint for each allocated memory block
//the footprint value should never be modified
#define MALLOC_FOOT_PRINT_SIZE ((size_t)8)

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE ((size_t)(xHeapStructSize << 1))

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE ((size_t)8)

#ifdef PLATFORM_LINUX
#define portBYTE_ALIGNMENT 4
#if portBYTE_ALIGNMENT == 4
#define portBYTE_ALIGNMENT_MASK (0x0003)
#endif
#endif

static uint8_t ucHeap[CTIOT_TOTAL_HEAP_SIZE];

/* Define the linked list structure.  This is used to link free blocks in order
of their memory address. */
/*
if cpu word is 32bit，the sizeof malloc_list is 8 bytes;
if cpu word is 64bit, the sizeof malloc_list is 16 bytes.
*/
typedef struct A_BLOCK_LINK
{
    struct A_BLOCK_LINK *pxNextFreeBlock; /*<< The next free block in the list. */
    size_t xBlockSize;                    /*<< The size of the free block. */
} BlockLink_t;

/**************************************************************************
                              CTIOT DEBUG
***************************************************************************/

//#define HEAP_DEBUG_SUB_SIZE

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList(BlockLink_t *pxBlockToInsert);

/*
 * Called automatically to setup the required heap structures the first time
 * pvPortMalloc() is called.
 */
static void prvCtiotHeapInit(void);

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
block must by correctly byte aligned. */
static const size_t xHeapStructSize = (sizeof(BlockLink_t) + ((size_t)(portBYTE_ALIGNMENT - 1))) & ~((size_t)portBYTE_ALIGNMENT_MASK);

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart, *pxEnd = NULL;

/* Keeps track of the number of free bytes remaining, but says nothing about
fragmentation. */
static size_t xFreeBytesRemaining = 0U;
static size_t xMinimumEverFreeBytesRemaining = 0U;

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
member of an BlockLink_t structure is set then the block belongs to the
application.  When the bit is free the block is still part of the free heap
space. */
static size_t xBlockAllocatedBit = 0;

/**********************************************************************
						  functions
***********************************************************************/

static uint8_t lock_init_flag = 0;
void *pCtiotMalloc(size_t xWantedSize)
{
    BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
    void *pvReturn = NULL;
    size_t realSize = 0;

    CTIOT_ASSERT(xWantedSize > 0);

    if (lock_init_flag == 0)
    {
        lock_init_flag = 1;
        /* add thread mutex */
        THREAD_LOCK_INIT();
    }

    CTIOT_THREAD_LOCK();
    {
        /* If this is the first call to malloc then the heap will require
        initialisation to setup the list of free blocks. */
        if (pxEnd == NULL)
        {
            prvCtiotHeapInit();
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        /* Check the requested block size is not so large that the top bit is
        set.  The top bit of the block size member of the BlockLink_t structure
        is used to determine who owns the block - the application or the
        kernel, so it must be free. */
        if ((xWantedSize & xBlockAllocatedBit) == 0)
        {
            /* The wanted size is increased so it can contain a BlockLink_t
            structure in addition to the requested amount of bytes. */
            if (xWantedSize > 0)
            {
#ifdef HEAP_DEBUG_SUB_SIZE
                printf("user want(trace):%d\r\n", xWantedSize);
                printf("xHeapStructSize:%d\r\n", xHeapStructSize);
                printf("end of the deadbeaf:%d\r\n", MALLOC_FOOT_PRINT_SIZE);
#endif

                xWantedSize += xHeapStructSize;
#ifdef HEAP_DEBUG_EN
                xWantedSize += MALLOC_FOOT_PRINT_SIZE; // malloc additional 8 bytes at the end
#endif
                /* Ensure that blocks are always aligned to the required number
                of bytes. */
                if ((xWantedSize & portBYTE_ALIGNMENT_MASK) != 0x00)
                {
                    /* Byte alignment required. */
                    xWantedSize += (portBYTE_ALIGNMENT - (xWantedSize & portBYTE_ALIGNMENT_MASK));
                    CTIOT_ASSERT((xWantedSize & portBYTE_ALIGNMENT_MASK) == 0);
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }

#ifdef HEAP_DEBUG_SUB_SIZE
                printf("alignment xWantedSize:%d\r\n", xWantedSize);
#endif
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }

            if ((xWantedSize > 0) && (xWantedSize <= xFreeBytesRemaining))
            {
                /* Traverse the list from the start (lowest address) block until
                one of adequate size is found. */
                pxPreviousBlock = &xStart;
                pxBlock = xStart.pxNextFreeBlock;
                while ((pxBlock->xBlockSize < xWantedSize) && (pxBlock->pxNextFreeBlock != NULL))
                {
                    pxPreviousBlock = pxBlock;
                    pxBlock = pxBlock->pxNextFreeBlock;
                }

                /* If the end marker was reached then a block of adequate size
                was not found. */
                if (pxBlock != pxEnd)
                {
                    /* Return the memory space pointed to - jumping over the
                    BlockLink_t structure at its start. */
                    pvReturn = (void *)(((uint8_t *)pxPreviousBlock->pxNextFreeBlock) + xHeapStructSize);

                    /* This block is being returned for use so must be taken out
                    of the list of free blocks. */
                    pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

                    /* If the block is larger than required it can be split into
                    two. */
                    if (pxBlock->xBlockSize < xWantedSize)
                    {
                        CTIOT_ASSERT(FALSE);
                    }
                    if ((pxBlock->xBlockSize - xWantedSize) > heapMINIMUM_BLOCK_SIZE)
                    {
                        /* This block is to be split into two.  Create a new
                        block following the number of bytes requested. The void
                        cast is used to prevent byte alignment warnings from the
                        compiler. */
                        pxNewBlockLink = (void *)(((uint8_t *)pxBlock) + xWantedSize);
                        CTIOT_ASSERT((((size_t)pxNewBlockLink) & portBYTE_ALIGNMENT_MASK) == 0);

                        /* Calculate the sizes of two blocks split from the
                        single block. */
                        pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
                        pxBlock->xBlockSize = xWantedSize;

                        /* Insert the new block into the list of free blocks. */
                        prvInsertBlockIntoFreeList(pxNewBlockLink);
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    xFreeBytesRemaining -= pxBlock->xBlockSize;

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
                    realSize = pxBlock->xBlockSize;
                    pxBlock->xBlockSize |= xBlockAllocatedBit;
                    pxBlock->pxNextFreeBlock = NULL;
#ifdef HEAP_DEBUG_EN
                    /*set the footprint*/
                    *(uint32_t *)((uint8_t *)pxBlock + realSize - MALLOC_FOOT_PRINT_SIZE) = 0xdeadbeaf;
                    *(uint32_t *)((uint8_t *)pxBlock + realSize - MALLOC_FOOT_PRINT_SIZE + 4) = 0xdeadbeaf;
#endif

#ifdef HEAP_DEBUG_SUB_SIZE
                    printf("realSize:%d\r\n", realSize);
                    printf("top address:%p\r\n", (uint8_t *)pxBlock);
                    printf("use address:%p\r\n", pvReturn);
                    printf("end address:%p\r\n", (uint8_t *)pxBlock + realSize);
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
    CTIOT_THREAD_UNLOCK();

    CTIOT_ASSERT(pvReturn != 0);
    CTIOT_ASSERT((((size_t)pvReturn) & (size_t)portBYTE_ALIGNMENT_MASK) == 0);

    return pvReturn;
}

void *pCtiotRealloc(void *pv, size_t xWantedSize)
{
    uint8_t *puc = (uint8_t *)pv;
    void *newPuc = NULL;
    BlockLink_t *pxLink;

    if (xWantedSize == 0)
    {
        return NULL;
    }

    if (puc != NULL)
    {
        /* The memory being freed will have an BlockLink_t structure immediately before it. */
        puc -= xHeapStructSize;

        /* This casting is to keep the compiler from issuing warnings. */
        pxLink = (void *)puc;
        CTIOT_ASSERT((pxLink->xBlockSize & xBlockAllocatedBit) != 0);
        CTIOT_ASSERT(pxLink->pxNextFreeBlock == NULL);
        if ((pxLink->xBlockSize & xBlockAllocatedBit) != 0)
        {
            pxLink->xBlockSize &= ~xBlockAllocatedBit;
            if (pxLink->xBlockSize >= xWantedSize)
            {
                return pv;
            }
            else
            {
                newPuc = pCtiotMalloc(xWantedSize);
                if (newPuc != NULL)
                {
                    memset(newPuc, 0, xWantedSize);
                    memcpy(newPuc, pv, pxLink->xBlockSize);
                    return newPuc;
                }
                else
                {
                    return NULL;
                }
            }
        }
    }
	return NULL;
}

/*-----------------------------------------------------------*/
void CtiotFree(void *pv)
{
    uint8_t *puc = (uint8_t *)pv;
    BlockLink_t *pxLink;

    CTIOT_ASSERT(pv != NULL);

    //ucHeap
    CTIOT_ASSERT((uint8_t *)pv >= (uint8_t *)ucHeap && (uint8_t *)pv < ((uint8_t *)ucHeap) + CTIOT_TOTAL_HEAP_SIZE);

    if (pv != NULL)
    {
        /* The memory being freed will have an BlockLink_t structure immediately
        before it. */
        puc -= xHeapStructSize;

        /* This casting is to keep the compiler from issuing warnings. */
        pxLink = (void *)puc;

        //EC_ASSERT( ( pxLink->xBlockSize & xBlockAllocatedBit, 0, 0, 0 ) != 0 );

        /* Check the block is actually allocated. */
        CTIOT_ASSERT((pxLink->xBlockSize & xBlockAllocatedBit) != 0);
        CTIOT_ASSERT(pxLink->pxNextFreeBlock == NULL);

        if ((pxLink->xBlockSize & xBlockAllocatedBit) != 0)
        {
            if (pxLink->pxNextFreeBlock == NULL)
            {
                /* The block is being returned to the heap - it is no longer
                allocated. */
                pxLink->xBlockSize &= ~xBlockAllocatedBit;
#ifdef HEAP_DEBUG_EN
                /*check the memory block's footprint*/
                CTIOT_ASSERT(*(uint32_t *)((uint8_t *)pxLink + pxLink->xBlockSize - MALLOC_FOOT_PRINT_SIZE) == 0xdeadbeaf);
                CTIOT_ASSERT(*(uint32_t *)((uint8_t *)pxLink + pxLink->xBlockSize - MALLOC_FOOT_PRINT_SIZE + 4) == 0xdeadbeaf);
#endif

                CTIOT_THREAD_LOCK();
                {
                    /* Add this block to the list of free blocks. */
                    xFreeBytesRemaining += pxLink->xBlockSize;
                    traceFREE(pv, pxLink->xBlockSize);
                    CTIOT_ASSERT(pxLink->xBlockSize > 0 && pxLink->xBlockSize < CTIOT_TOTAL_HEAP_SIZE);

                    prvInsertBlockIntoFreeList(((BlockLink_t *)pxLink));
                }
                CTIOT_THREAD_UNLOCK();
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
}
/*-----------------------------------------------------------*/

size_t CtiotGetFreeHeapSize(void)
{
    return xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

size_t CtiotGetMinimumEverFreeHeapSize(void)
{
    return xMinimumEverFreeBytesRemaining;
}

size_t CtiotGetAllHeapSize(void)
{
    return CTIOT_TOTAL_HEAP_SIZE;
}
/*-----------------------------------------------------------*/

void CtiotInitialiseBlocks(void)
{
    /* This just exists to keep the linker quiet. */
}
/*-----------------------------------------------------------*/

static void prvCtiotHeapInit(void)
{
    BlockLink_t *pxFirstFreeBlock;
    uint8_t *pucAlignedHeap;
    size_t uxAddress;
    size_t xTotalHeapSize = CTIOT_TOTAL_HEAP_SIZE;

    /* Ensure the heap starts on a correctly aligned boundary. */
    uxAddress = (size_t)ucHeap;

    if ((uxAddress & portBYTE_ALIGNMENT_MASK) != 0)
    {
        uxAddress += (portBYTE_ALIGNMENT - 1);
        uxAddress &= ~((size_t)portBYTE_ALIGNMENT_MASK);
        xTotalHeapSize -= uxAddress - (size_t)ucHeap;
    }

    pucAlignedHeap = (uint8_t *)uxAddress;

    /* xStart is used to hold a pointer to the first item in the list of free
    blocks.  The void cast is used to prevent compiler warnings. */
    xStart.pxNextFreeBlock = (void *)pucAlignedHeap;
    xStart.xBlockSize = (size_t)0;

    /* pxEnd is used to mark the end of the list of free blocks and is inserted
    at the end of the heap space. */
    uxAddress = ((size_t)pucAlignedHeap) + xTotalHeapSize;
    uxAddress -= xHeapStructSize;
    uxAddress &= ~((size_t)portBYTE_ALIGNMENT_MASK);
    pxEnd = (void *)uxAddress;
    pxEnd->xBlockSize = 0;
    pxEnd->pxNextFreeBlock = NULL;

    /* To start with there is a single free block that is sized to take up the
    entire heap space, minus the space taken by pxEnd. */
    pxFirstFreeBlock = (void *)pucAlignedHeap;
    pxFirstFreeBlock->xBlockSize = uxAddress - (size_t)pxFirstFreeBlock;
    pxFirstFreeBlock->pxNextFreeBlock = pxEnd;

    /* Only one block exists - and it covers the entire usable heap space. */
    xMinimumEverFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
    xFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;

    /* Work out the position of the top bit in a size_t variable. */
    xBlockAllocatedBit = ((size_t)1) << ((sizeof(size_t) * heapBITS_PER_BYTE) - 1);
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList(BlockLink_t *pxBlockToInsert)
{
    BlockLink_t *pxIterator;
    uint8_t *puc;

    /* Iterate through the list until a block is found that has a higher address
    than the block being inserted. */
    for (pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock)
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
    if ((puc + pxBlockToInsert->xBlockSize) == (uint8_t *)pxIterator->pxNextFreeBlock)
    {
        if (pxIterator->pxNextFreeBlock != pxEnd)
        {
            /* Form one big block from the two blocks. */
            pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
            pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
        }
        else
        {
            pxBlockToInsert->pxNextFreeBlock = pxEnd;
        }
    }
    else
    {
        pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
    }

    /* If the block being inserted plugged a gab, so was merged with the block
    before and the block after, then it's pxNextFreeBlock pointer will have
    already been set, and should not be set here as that would make it point
    to itself. */
    if (pxIterator != pxBlockToInsert)
    {
        pxIterator->pxNextFreeBlock = pxBlockToInsert;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }
}
