#ifndef DIAG_CONFIG_H
#define DIAG_CONFIG_H

#include "sys_debug.h"
#include "cmsis_os2.h"
#include <string.h>


/***************************************************************************************************************/
/*                                        dialog contral function config                                       */
/***************************************************************************************************************/
#define DIAG_ASSERT(x)                          Sys_Assert(x)                           // 主动断言函数，可以去除，但建议保留，用于出问题时的定位
#define DIAG_CRITICAL_DEF(x)                    BaseType_t x                            // 临界区定义，在需要保存中断状态时需要定义
#define DIAG_ENTER_CRITICAL(x)                  x = portSET_INTERRUPT_MASK_FROM_ISR()   // 进入临界区函数，必须实现
#define DIAG_EXIT_CRITICAL(x)                   portCLEAR_INTERRUPT_MASK_FROM_ISR(x)    // 退出临界区函数，必须实现
#if( configUSE_HEAP_MALLOC_DEBUG == 1 )
	#define DIAG_MALLOC(size, file, line)           pvPortLogMalloc(size, file, line)       // 内存申请函数，DIAG_MALLOC_RECORD_FILE_LINE 为0时可以把文件名行号参数去掉
#else
	#define DIAG_MALLOC(size)           			pvPortLogMalloc(size)       // 内存申请函数，DIAG_MALLOC_RECORD_FILE_LINE 为0时可以把文件名行号参数去掉
#endif
#define DIAG_FREE(mem)                          vPortFree(mem)                          // 内存释放函数，必须实现
#define DIAG_GET_TICK_COUNT()                   xTaskGetTickCount()                     // 获取操作系统tick的函数，必须实现
#define DIAG_MEMCPY(dst, src, size)             memcpy(dst, src, size)                  // 内存拷贝函数，必须实现
#define DIAG_MEMSET(dst, num, size)             memset(dst, num, size)                  // 内存设置函数，暂时未使用到，无需实现
#define DIAG_STRLEN(str)                        strlen((const char *) (str))            // 获得字符串长度函数，必须实现
//#define DIGA_ATTR_PRINTF(a, b)                  __attribute__ ((format(printf, a, b)))  // 设置变参函数的参数检查，可以不实现


/***************************************************************************************************************/
/*                                        dialog contral config                                                */
/***************************************************************************************************************/
#define DIAG_TRANSMIT_WITH_DMA                  1                       // log传输是否使用DMA
#define DIAG_TRANSMIT_IN_DMA_LIST_MODE          0                       // 使用DMA传输时是否使用DMA链表模式，仅在 DIAG_TRANSMIT_WITH_DMA 为1时有效
#define DIAG_DMA_LIST_ONCE_SEND_NUM             10                      // DMA的list模式，一次最多传输多少条数据，仅在 DIAG_TRANSMIT_IN_DMA_LIST_MODE 为1时有效
#define DIAG_MALLOC_RECORD_FILE_LINE            1                       // 是否使能申请内存时，记录申请的文件和行号，为1时使能
#define DIAG_MAX_OCCUPANCY_SIZE                 (1024 * 12)             // log最多占用的内存大小，不包含内存节点占用的内存，不可为0
#define DIAG_SIGNAL_RESERVED_SIZE               (1024 * 2)              // log中为信令log预留的大小，其他log只能使用 (DIAG_MAX_OCCUPANCY_SIZE - DIAG_SIGNAL_RESERVED_SIZE) 字节
#define DIAG_MAX_OCCUPANCY_NODE                 300                     // log最多占用的节点个数，不可为0
#define DIAG_ONE_LOG_MAX_SIZE_BYTE              (1024 * 4)              // 一条log最多占用的字节数，对于动态log，即参数最多占用长度，对于静态log，即参数和格式化字符串最多占用长度
#define DIAG_CALCULATE_CRC_ENABLE               0                       // 是否使能数据CRC校验，为1时使能
#define DIAG_DUMP_ONE_PACKET_SIZE               128                     // 死机内存导出时，每包导出内存的大小，会根据这个大小分配全局变量，不可小于64
#define DIAG_HEART_BEAT_PACKET                  0                       // log输出是否需要心跳包，如果需要，当前处理是只要收到一次心跳包，后续就可以一直打印
#define DIAG_HEART_VALID_TIME                   (2 * 1000)              // log收到心跳包有效的时间，单位tick，在 DIAG_HEART_BEAT_PACKET 为1时有效
#define DIAG_PHY_MEMPOOL_USE                    0                       // log节点申请内存是否使用预申请机制

#endif  /* DIAG_CONFIG_H */
