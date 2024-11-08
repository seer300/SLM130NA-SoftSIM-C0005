#ifndef DIAG_LIST_H
#define DIAG_LIST_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "diag_options.h"

#if ((DIAG_TRANSMIT_WITH_DMA == 1) && (DIAG_TRANSMIT_IN_DMA_LIST_MODE == 1))

typedef struct diag_list
{
    void     *src_addr;       // 源地址
    void     *dst_addr;       // 目的地址
    uint32_t  dma_ctrl;       // dma的配置
    uint32_t  send_size;      // 记录需要传输的数据的字节数
    struct diag_list *next;   // 指向下一节点
    uint32_t  malloc_size;    // 记录当前节点的大小，包括该结构体大小和数据
    char      data[0];        // 传输数据的起始地址
} diag_list_t;

// 设置和获取结构体成员的宏定义
// 获取该链表节点申请的字节数，包含链表结构体的大小，不包含内存节点占用的大小
#define DIAG_LIST_SET_MALLOC_SIZE(list, size)    do{((diag_list_t *)list)->malloc_size = size;}while(0)
// 设置该链表接待你申请的字节数
#define DIAG_LIST_GET_MALLOC_SIZE(list)          (((diag_list_t *)list)->malloc_size)
// 获取需要发送的数据大小，即 data[0] 段的数据长度
#define DIAG_LIST_SET_SEND_SIZE(list, size)      do{((diag_list_t *)list)->send_size = size;}while(0)
// 设置需要发送的数据大小，用于发送时获取长度
#define DIAG_LIST_GET_SEND_SIZE(list)            (((diag_list_t *)list)->send_size)
// 获取下一节点的地址
#define DIAG_LIST_GET_NEXT_ITEM(list)            (((diag_list_t *)list)->next)
// 获取需要发送的数据的起始地址
#define DIAG_LIST_GET_SEND_DATA(list)            (((diag_list_t *)list)->data)

// DMA和CPU的地址转换
#define CORE_ADDR_TO_DMA_ADDR(addr)    ((((uint32_t)(addr) >= 0UL) && ((uint32_t)(addr) < 0x20000UL)) ? ((uint32_t)(addr) + 0x20020000UL) : (uint32_t)(addr))
#define DMA_ADDR_TO_CORE_ADDR(addr)    ((((uint32_t)(addr) >= 0x20020000UL) && ((uint32_t)(addr) < 0x20040000UL)) ? ((uint32_t)(addr) - 0x20020000UL) : (uint32_t)(addr))


#else   /* (DIAG_TRANSMIT_WITH_DMA != 1) || (DIAG_TRANSMIT_IN_DMA_LIST_MODE != 1) */


typedef struct diag_list
{
    struct diag_list *next;           // 指向链表下一节点
    uint16_t          malloc_size;    // 记录当前节点的大小，包括该结构体大小和数据
    uint16_t          send_size;      // 记录需要传输的大小
    char              data[0];        // 传输数据的起始地址
} diag_list_t;

// 设置和获取结构体成员的宏定义
// 获取该链表节点申请的字节数，包含链表结构体的大小，不包含内存节点占用的大小
#define DIAG_LIST_SET_MALLOC_SIZE(list, size)    do{((diag_list_t *)list)->malloc_size = size;}while(0)
// 设置该链表接待你申请的字节数
#define DIAG_LIST_GET_MALLOC_SIZE(list)          (((diag_list_t *)list)->malloc_size)
// 获取需要发送的数据大小，即 data[0] 段的数据长度
#define DIAG_LIST_SET_SEND_SIZE(list, size)      do{((diag_list_t *)list)->send_size = size;}while(0)
// 设置需要发送的数据大小，用于发送时获取长度
#define DIAG_LIST_GET_SEND_SIZE(list)            (((diag_list_t *)list)->send_size)
// 获取需要发送的数据的起始地址
#define DIAG_LIST_GET_SEND_DATA(list)            (((diag_list_t *)list)->data)


#endif  /* (DIAG_TRANSMIT_WITH_DMA == 1) && (DIAG_TRANSMIT_IN_DMA_LIST_MODE == 1) */


// 把一个列表项加入有效链表的尾部
void diag_list_insert_valid(diag_list_t * diag_item);

// 获取需要发送的列表，根据是否使用DMA的list模式，会返回一个链表成员或多个
// 如果使用了DMA的list模式，会修改每个成员的源地址，改成DMA可以访问的地址
diag_list_t * diag_list_get_send_list(void);

// 把已经发送完成的链表加入到待释放链表的头部
// 如果使用了DMA的list模式，会再把DMA访问的地址，转换到CPU可以访问的地址
void diag_list_insert_send_to_free(void);

// 把未发送的列表加入到待释放链表的头部，用以清空还未发送的log
void diag_list_move_valid_to_free(void);

// 移除待释放链表的头部，内存已经释放完成
void diag_list_remove_free_head(void);

// 获取待释放链表的头部，一般用在准备释放内存时
diag_list_t * diag_list_get_free_head(void);

// 判断当前待发送和已发送列表是否都为空
// 1: 为空  0: 非空
int diag_list_is_all_list_empty(void);


#ifdef __cplusplus
 }
#endif

#endif  /* DIAG_LIST_H */
