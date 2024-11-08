#ifndef DIAG_MEM_H
#define DIAG_MEM_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "diag_options.h"
#include "diag_config.h"

#define DIAG_PHY_MEMPOOL_NODE_NUM              20   //物理层使用内存池节点数量
#define DIAG_PHY_MEMPOOL_NODE_SIZE             100   //物理层使用内存池节点大小


 typedef struct diag_phy_mempool_list
 {
      uint32_t  pdCurAddr;             // 当前节点的内存地址
      uint32_t  pdLastAddr;            //上一个节点的内存地址
      uint32_t  current_node_num;      //当前内存池节点数量
      size_t    current_mempool_size;  //当前内存池大小

 } diag_phy_mempool_list_t;

typedef enum diag_alloc
{
    NONUSE_RESERED_AREA = 0,
    CAN_USE_RESERED_AREA = 1
} diag_alloc_t;

typedef struct 
{

    uint32_t alloc_mem_succ_cnt;           // 最近一段时间内，成功申请到内存的log节点总数
    uint32_t send_time_interval;           // 最近一段时间内，某log输出的最大时延，单位：ms
    uint32_t occupy_node_full_cnt;         // 最近一段时间内，log缓存节点数达到上限时造成丢log的累加值
    uint32_t occupy_memory_full_cnt;       // 最近一段时间内，log缓存节点总内存达到上限时造成丢log的累加值
    uint32_t length_error_cnt;             // 最近一段时间内，由于数据长度问题造成丢log的累加值
    uint32_t format_fail_cnt;              // 最近一段时间内，由于格式化失败造成丢log的累加值

}diag_debug_info_t;

extern diag_debug_info_t g_diag_debug;

typedef struct 
{
    uint32_t heart_end_count;           // 心跳包有效时间，该时间过期表明SOC与PC工具的硬件连接断开
    uint32_t heart_recv_tick;           // 记录最近一次收到心跳包的时刻点
    uint32_t data_recv_tick;            // 记录最近一次从硬件通道接收数据的时刻点
    uint32_t data_send_tick;            // 记录最近一次从硬件通道发送数据的时刻点
    uint32_t recv_valid_packet_tick;    // 记录最近一次接收到有效数据包的时刻点
}diag_status_info_t;

extern diag_status_info_t g_log_status;


#if (DIAG_MALLOC_RECORD_FILE_LINE == 1)
void *diag_mem_alloc_record(uint32_t size, diag_alloc_t area, char *file, int line);
#define diag_mem_alloc(size, area)    diag_mem_alloc_record(size, area, __FILE__, __LINE__)
#else
void *diag_mem_alloc(uint32_t size, diag_alloc_t area);
#endif

void diag_mem_free(void * mem);
#if(DIAG_PHY_MEMPOOL_USE)
void diag_phy_mempool_init(void);
void *diag_phy_mempool_malloc(void);
void diag_phy_mempool_check(void);
#endif

#ifdef __cplusplus
 }
#endif

#endif  /* DIAG_MEM_H */
