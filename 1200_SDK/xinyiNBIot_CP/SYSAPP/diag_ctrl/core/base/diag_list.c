#include "diag_options.h"
#include "diag_list.h"
#include "diag_packet.h"

// 当前正在发送的数据的链表，使用头指针用于设置数据开始发送，使用尾指针用于快速插入待释放链表
static diag_list_t * diag_send_list_head = NULL;
static diag_list_t * diag_send_list_tail = NULL;
// 有效链表的头尾指针，使用头指针能够快速的取到第一个链表成员，使用尾指针用于快速插入新成员到链表尾部
static diag_list_t * diag_valid_list_head = NULL;
static diag_list_t * diag_valid_list_tail = NULL;
// 待释放内存链表的头指针，由于都是等待释放的内存，没有释放顺序的要求，新成员插入到链表头部，也从头部成员开始释放内存
static diag_list_t * diag_free_list = NULL;


__RAM_FUNC void diag_list_insert_valid(diag_list_t * diag_item)
{
    DIAG_ASSERT(diag_item != NULL);
    
    // 在插入链表时设置sequence_num，防止sequence_num乱序
    diag_packet_set_sequence_num(diag_item);

    if(diag_valid_list_tail == NULL)
    {
        // 尾节点为NULL时，说明当前链表为空，头尾指针均指向要插入的成员
        diag_valid_list_head = diag_item;
        diag_item->next = NULL;
        diag_valid_list_tail = diag_item;
    }
    else
    {
        // 尾节点不为NULL时，把该成员加入到链表尾部，更新尾指针
        diag_valid_list_tail->next = diag_item;
        diag_item->next = NULL;
        diag_valid_list_tail = diag_item;
    }
}
/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC diag_list_t * diag_list_get_send_list(void)
{
    diag_list_t *diag_head, *diag_iter;

    DIAG_ASSERT(diag_send_list_head == NULL);
    DIAG_ASSERT(diag_send_list_tail == NULL);

    diag_head = diag_valid_list_head;
    diag_iter = diag_head;

    // 如果待发送链表为空，则返回NULL，退出函数
    if (diag_head == NULL)
    {
        return NULL;
    }

    // 如果使用了DMA的链表模式，则遍历指定链表长度，找到列表
    #if ((DIAG_TRANSMIT_WITH_DMA == 1) && (DIAG_TRANSMIT_IN_DMA_LIST_MODE == 1))
    {
        int iter_num;
        diag_list_t *diag_prev = diag_iter;
        // 从有效链表中的第一个开始遍历，直到链表结束或链表遍历达到指定个数
        for (iter_num = 1; (iter_num < DIAG_DMA_LIST_ONCE_SEND_NUM) && (diag_iter->next != NULL); iter_num++)
        {
            // 首先获取下一节点的地址，接下来要对前一节点进行地址转换
            diag_iter = diag_iter->next;
            // 转换地址为DMA访问的地址
            diag_prev->next = (void *) CORE_ADDR_TO_DMA_ADDR(diag_prev->next);
            // 指向下一个节点，下次循环继续转换
            diag_prev = diag_iter;
        }
    }
    #endif

    // 如果迭代器的下一个成员为NULL，则认为有效链表已经为空，清空两个链表指针
    if (diag_iter->next == NULL)
    {
        diag_valid_list_head = NULL;
        diag_valid_list_tail = NULL;
    }
    // 否则认为有效链表还有成员，把头指针往后移，并截断链表
    else
    {
        diag_valid_list_head = diag_iter->next;
        diag_iter->next = NULL;
    }

    // 把截断的链表放入发送的链表，发送完成后，把该链表放入待释放链表
    diag_send_list_head = diag_head;
    diag_send_list_tail = diag_iter;

    return diag_head;
}
/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC void diag_list_insert_send_to_free(void)
{
    DIAG_ASSERT(diag_send_list_head != NULL);
    DIAG_ASSERT(diag_send_list_tail != NULL);

    // 如果使用了DMA的list模式，则需要把DMA地址转换会CPU的地址
    #if ((DIAG_TRANSMIT_WITH_DMA == 1) && (DIAG_TRANSMIT_IN_DMA_LIST_MODE == 1))
    {
        diag_list_t *diag_iter = diag_send_list_head;
        
        // 遍历链表，把所有的节点都转换成CPU可以访问的地址
        while (diag_iter->next != NULL)
        {
            // 把下一节点数据的地址，转换成CPU可以访问的地址
            diag_iter->next = (void *) DMA_ADDR_TO_CORE_ADDR(diag_iter->next);
            // 指向下一节点，继续循环
            diag_iter = diag_iter->next;
        }
    }
    #endif

    // 把当前发送完成的链表加入到free链表的头部，free链表重新指向链表头部
    diag_send_list_tail->next = diag_free_list;
    diag_free_list = diag_send_list_head;
    
    // 清除发送链表，等待下次继续发送
    diag_send_list_head = NULL;
    diag_send_list_tail = NULL;
}
/*----------------------------------------------------------------------------------------------------*/


__FLASH_FUNC void diag_list_move_valid_to_free(void)
{
    // 如果当前待发送列表有数据
    if ((diag_valid_list_head != NULL) && (diag_valid_list_tail != NULL))
    {
        // 把待发送列表的所有成员加入到待释放列表
        diag_valid_list_tail->next = diag_free_list;
        diag_free_list = diag_valid_list_head;

        // 清空待发送列表
        diag_valid_list_head = NULL;
        diag_valid_list_tail = NULL;
    }
}
/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC void diag_list_remove_free_head(void)
{
    // 要移除的链表成员必须为链表的头指针指向的成员，当前的链表必须先入先出
    if(diag_free_list != NULL)
    {
        // 更新链表的头指针，指向下一成员
        diag_free_list = diag_free_list->next;
    }
    else
    {
        DIAG_ASSERT(0);
    }
}
/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC diag_list_t * diag_list_get_free_head(void)
{
    diag_list_t * diag_head; 

    diag_head = diag_free_list;

    return diag_head;
}
/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC int diag_list_is_all_list_empty(void)
{
    int ret;

    ret = ((diag_send_list_head == NULL) && (diag_valid_list_head == NULL)) ? 1 : 0;

    return ret;
}
/*----------------------------------------------------------------------------------------------------*/
