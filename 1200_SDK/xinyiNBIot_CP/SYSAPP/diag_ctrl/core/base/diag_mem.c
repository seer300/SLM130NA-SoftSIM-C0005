#include "diag_options.h"
#include "diag_mem.h"
#include "diag_list.h"
#include "factory_nv.h"

static uint32_t diag_max_occupancy_size = 0;
static uint32_t diag_occupancy_size = 0;
static uint32_t diag_occupancy_node = 0;
diag_phy_mempool_list_t g_diag_phy_mempool_head;
diag_debug_info_t g_diag_debug = {0};
diag_status_info_t g_log_status = {0};

#if (DIAG_MALLOC_RECORD_FILE_LINE == 1)
__RAM_FUNC void * diag_mem_alloc_record(uint32_t size, diag_alloc_t area, char *file, int line)
#else
__RAM_FUNC void * diag_mem_alloc(uint32_t size, diag_alloc_t area)
#endif
{
    void * mem ;
    void * ret_mem;
    size_t malloc_size;

    DIAG_CRITICAL_DEF(isr);

    DIAG_ASSERT(size != 0);

    malloc_size = size + sizeof(diag_list_t);

    // 进入临界区，下面会访问到全局变量，防止多线程抢占
    DIAG_ENTER_CRITICAL(isr);

    // 内存节点数不能超过设置的最大内存节点数，防止内存碎片化严重
    if (diag_occupancy_node >= DIAG_MAX_OCCUPANCY_NODE)
    {
		g_diag_debug.occupy_node_full_cnt++;
        DIAG_EXIT_CRITICAL(isr);
        return NULL;
    }

    if(area == NONUSE_RESERED_AREA)
    {
        // 当不能使用预留区域时，申请成功后的内存不能超过（最大log占用大小 - 预留大小）
        if ((diag_occupancy_size + malloc_size) > (DIAG_MAX_OCCUPANCY_SIZE - DIAG_SIGNAL_RESERVED_SIZE))
        {
			g_diag_debug.occupy_memory_full_cnt++;
            DIAG_EXIT_CRITICAL(isr);
            return NULL;
        }
    }
    else if(area == CAN_USE_RESERED_AREA)
    {
        // 当可以使用预留区域时，申请成功后的内存不能超过最大log占用大小
        if ((diag_occupancy_size + malloc_size) > DIAG_MAX_OCCUPANCY_SIZE)
        {
			g_diag_debug.occupy_memory_full_cnt++;
            DIAG_EXIT_CRITICAL(isr);
            return NULL;
        }
    }
    else
    {
        DIAG_ASSERT(0);
    }

    // 下面申请内存的操作不需要临界区保护，在此先退出临界区
    DIAG_EXIT_CRITICAL(isr);

        #if (DIAG_MALLOC_RECORD_FILE_LINE == 1)
        {
#if( configUSE_HEAP_MALLOC_DEBUG == 1 )
            mem = DIAG_MALLOC(malloc_size, file, line);
#else
            mem = DIAG_MALLOC(malloc_size);
#endif
        }
        #else
        {
#if( configUSE_HEAP_MALLOC_DEBUG == 1 )
            mem = DIAG_MALLOC(malloc_size, __FILE__, __LINE__);
#else
            mem = DIAG_MALLOC(malloc_size);
#endif
        }
        #endif

    // 再次进入临界区，该函数会多线程访问，下面的全局变量需要保护
    DIAG_ENTER_CRITICAL(isr);

    if (mem != NULL)
    {
        // 增加log占用的内存，增加log占用的内存节点
        diag_occupancy_size += malloc_size;
        diag_occupancy_node += 1;
        g_diag_debug.alloc_mem_succ_cnt += 1;
        // 记录log最大内存消耗
        if (diag_occupancy_size > diag_max_occupancy_size)
        {
            diag_max_occupancy_size = diag_occupancy_size;
        }
        // 记录当前内存的大小，用于释放时，获取log减少占用的内存
        DIAG_LIST_SET_MALLOC_SIZE(mem, malloc_size);

    }
    else
    {
		g_diag_debug.occupy_memory_full_cnt++;
        DIAG_EXIT_CRITICAL(isr);
        return NULL;
    }

    DIAG_EXIT_CRITICAL(isr);

    // 返回调用者实际可用的地址
    ret_mem = (char *)mem + sizeof(diag_list_t);
    return ret_mem;
}
/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC void diag_mem_free(void * mem)
{
    void * free_mem;
    uint32_t malloc_size;

    DIAG_CRITICAL_DEF(isr);

    DIAG_ASSERT(mem != NULL);

    free_mem = (char *)mem - sizeof(diag_list_t);

    // 进入临界区，下面会访问到全局变量，防止多线程抢占
    DIAG_ENTER_CRITICAL(isr);

    // 减少log占用的内存，减少log占用的内存节点
    malloc_size = DIAG_LIST_GET_MALLOC_SIZE(free_mem);
    // 当前要释放的内存不会超过log占用的内存，否则就是异常情况
    if(diag_occupancy_size >= malloc_size)
    {
        diag_occupancy_size -= DIAG_LIST_GET_MALLOC_SIZE(free_mem);
        diag_occupancy_node -= 1;
    }
    else
    {
        DIAG_ASSERT(0);
    }

    DIAG_EXIT_CRITICAL(isr);

    // 这里必须放在减少log统计数据的后面，否则在恢复log内存占用时，这块内存已经释放，数据可能不正确
    DIAG_FREE(free_mem);
}


#if(DIAG_PHY_MEMPOOL_USE)
__FLASH_FUNC void diag_phy_mempool_init(void)
{
    void * mem;
    uint32_t pdCurAddr;
    uint32_t pdLastAddr ;
    int i;

//	if(HWREGB(BAK_MEM_CP_USED_APRAM_SIZE) == 0)
//		return;

	
    for(i = 0;i<DIAG_PHY_MEMPOOL_NODE_NUM;i++)
    {

       mem = DIAG_MALLOC(DIAG_PHY_MEMPOOL_NODE_SIZE,__FILE__ , __LINE__);

       if(i == 0)
       {
           *(uint32_t*)mem = 0;
       }
       else
       {
           *(uint32_t*)mem = pdLastAddr;

       }
       pdCurAddr = (uint32_t)mem;
       pdLastAddr = pdCurAddr;
    }
    g_diag_phy_mempool_head.pdCurAddr = pdCurAddr;
    g_diag_phy_mempool_head.pdLastAddr =  *(uint32_t*)mem;
    g_diag_phy_mempool_head.current_node_num = DIAG_PHY_MEMPOOL_NODE_NUM;
    g_diag_phy_mempool_head.current_mempool_size = g_diag_phy_mempool_head.current_node_num*DIAG_PHY_MEMPOOL_NODE_SIZE;
    diag_occupancy_size += g_diag_phy_mempool_head.current_mempool_size;
    diag_occupancy_node += DIAG_PHY_MEMPOOL_NODE_NUM;

}
__RAM_FUNC void *diag_phy_mempool_malloc(void)
{
    void*mem;
    void * ret_mem;
    DIAG_CRITICAL_DEF(isr);
    DIAG_ENTER_CRITICAL(isr);
    if(g_diag_phy_mempool_head.current_node_num != 0)
    {
        mem = (void*)g_diag_phy_mempool_head.pdCurAddr;
        g_diag_phy_mempool_head.pdCurAddr = g_diag_phy_mempool_head.pdLastAddr;
        if(g_diag_phy_mempool_head.pdCurAddr != 0)
        {
             g_diag_phy_mempool_head.pdLastAddr = *(uint32_t*)g_diag_phy_mempool_head.pdCurAddr;
        }
        else
        {
            g_diag_phy_mempool_head.pdLastAddr  = 0;
        }

        g_diag_phy_mempool_head.current_node_num--;
        g_diag_phy_mempool_head.current_mempool_size -= DIAG_PHY_MEMPOOL_NODE_SIZE;
        // 记录当前内存的大小，用于释放时，获取log减少占用的内存
        DIAG_LIST_SET_MALLOC_SIZE(mem, DIAG_PHY_MEMPOOL_NODE_SIZE);

    }
    else
    {
        mem = NULL;
        return mem;
    }
    DIAG_EXIT_CRITICAL(isr);
    // 返回调用者实际可用的地址
    ret_mem = (char *)mem + sizeof(diag_list_t);
    return ret_mem;

}
__RAM_FUNC void diag_phy_mempool_check(void)
{
    uint32_t i;
    uint32_t addnode = 0;
    void * mem = NULL;
    void * mem_tail = NULL;
    uint32_t pdCurAddr = 0;
    uint32_t pdLastAddr = 0;
    size_t malloc_size = 0;
    size_t diag_cur_size;
    size_t mempool_node;
    DIAG_CRITICAL_DEF(isr);


//	if(HWREGB(BAK_MEM_CP_USED_APRAM_SIZE) == 0)
//		return;

	
    diag_cur_size = diag_occupancy_size;

    mempool_node = g_diag_phy_mempool_head.current_node_num;
    if(mempool_node < DIAG_PHY_MEMPOOL_NODE_NUM)
    {
        for(i=mempool_node;i<DIAG_PHY_MEMPOOL_NODE_NUM;i++)
        {
            if(diag_cur_size + malloc_size < (DIAG_MAX_OCCUPANCY_SIZE - DIAG_SIGNAL_RESERVED_SIZE))

            {

            #if (DIAG_MALLOC_RECORD_FILE_LINE == 1)
            {
                mem = DIAG_MALLOC(DIAG_PHY_MEMPOOL_NODE_SIZE, __FILE__, __LINE__);
            }
            #else
            {
                mem = DIAG_MALLOC(DIAG_PHY_MEMPOOL_NODE_SIZE, __FILE__, __LINE__);
            }
            #endif

            if(i == mempool_node)
            {
                mem_tail = mem;
            }
            else
            {
                *(uint32_t*)mem = pdLastAddr;

            }
            pdCurAddr = (uint32_t)mem;
            pdLastAddr = pdCurAddr;
            malloc_size += DIAG_PHY_MEMPOOL_NODE_SIZE;
            addnode++;
            }

        }
           DIAG_ENTER_CRITICAL(isr);
           if(mem != NULL)
           {
            *(uint32_t*)mem_tail = g_diag_phy_mempool_head.pdCurAddr;
            g_diag_phy_mempool_head.pdCurAddr = pdCurAddr;
            g_diag_phy_mempool_head.pdLastAddr =  *(uint32_t*)mem;
            g_diag_phy_mempool_head.current_node_num += addnode;
            g_diag_phy_mempool_head.current_mempool_size += addnode*DIAG_PHY_MEMPOOL_NODE_SIZE;
            diag_occupancy_size += addnode*DIAG_PHY_MEMPOOL_NODE_SIZE;
            diag_occupancy_node += addnode;
           }
           DIAG_EXIT_CRITICAL(isr);

    }
}
#endif
/*----------------------------------------------------------------------------------------------------*/
