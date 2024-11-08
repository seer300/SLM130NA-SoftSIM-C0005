#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>
#include "zero_copy.h"
#include "xy_list.h"
#include "sys_ipc.h"
#include "xy_system.h"
#include "xy_event.h"


/* 发给CP的零拷贝地址链表*/
ListHeader_t zero_copy_list = {0};

//通过核间向CP发送零拷贝地址时，先记录下该指针
__OPENCPU_FUNC void Insert_ZeroCopy_Buf(void *buf_addr)
{
	ZeroCopy_Buf_t *pxlist;
	pxlist = xy_malloc(sizeof(ZeroCopy_Buf_t));

	pxlist->next = NULL;
	pxlist->bufaddr = buf_addr;

	ListInsert((List_t *)pxlist, &zero_copy_list);
}

//收到核间释放零拷贝地址的消息时，删除相应地址对应的结点
__OPENCPU_FUNC void Delet_ZeroCopy_Buf(void *buf_addr)
{
	List_t *current_node = zero_copy_list.head;
	List_t *pre_node = zero_copy_list.head;

	DisablePrimask();
	while(current_node != NULL)
	{
		if(((ZeroCopy_Buf_t *)current_node)->bufaddr == buf_addr)
		{
			if(current_node == zero_copy_list.head)
			{
				zero_copy_list.head = (List_t *)(current_node->next);
				if(current_node->next == NULL)
					zero_copy_list.tail = NULL;
			}
			else
			{
				pre_node->next = current_node->next;
				if(current_node->next == NULL)
					zero_copy_list.tail = pre_node;
			}
			zero_copy_list.node_count -= 1;
			xy_free(current_node);
			xy_free(buf_addr);
			EnablePrimask();
			return;
		}
		else
		{
			pre_node = current_node;
			current_node = (List_t *)(current_node->next);
		}
	}
	EnablePrimask();
}

__OPENCPU_FUNC void Free_ZeroCopy_Buf()
{
	DisablePrimask();
	List_t *p_node;
	while((p_node = ListRemove(&zero_copy_list)) != NULL)
	{
		xy_free(((ZeroCopy_Buf_t *)p_node)->bufaddr);
		xy_free(p_node);
	}
	EnablePrimask();
}

__OPENCPU_FUNC bool Is_ZeroCopy_Buf_Freed()
{
	if (GetListNum(&zero_copy_list) == 0)
		return true;
	else
		return false;
}

