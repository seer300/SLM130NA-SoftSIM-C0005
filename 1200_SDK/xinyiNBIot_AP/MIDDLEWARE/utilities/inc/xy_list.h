/*
 * @file  
 * @brief 抽象的单向链表
 */
#pragma once

#include <stdint.h>

#define List_NUM_MAX                 20

typedef struct
{
	struct List_t *next;
}List_t;

typedef struct
{
	uint32_t node_count;
	uint32_t drop_count;
	List_t *tail;  //最新
	List_t *head;  //最旧
}ListHeader_t;



/*插入一个节点到尾部，其中pxList需要进行类型强转*/
void ListInsert(List_t *pxList,ListHeader_t *p_List_header);


/*读取链表头部节点，返回的地址需要进行类型强转*/
List_t *ListRemove(ListHeader_t *p_List_header);


/*清空某链表*/
void ListFreeAll(ListHeader_t *p_List_header);


/*获取链表节点数，当为0时表示无节点，也可用来监控节点数是否超出阈值*/
uint32_t GetListNum(ListHeader_t *p_List_header);

/*记录丢节点数*/
void ListAddDropCount(ListHeader_t *p_List_header);

