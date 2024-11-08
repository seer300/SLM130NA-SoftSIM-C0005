#include "xy_list.h"
#include "hal_def.h"
#include "xy_system.h"

__OPENCPU_FUNC void ListInsert(List_t *pxList,ListHeader_t *p_List_header)
{
	DisablePrimask();

	List_t *end_list = (List_t *)(p_List_header->tail);

	if(end_list != NULL)
		end_list->next = (struct List_t *)(pxList);
	else
		p_List_header->head = pxList;

	p_List_header->tail = pxList;
	p_List_header->node_count += 1;
	EnablePrimask();
}

__OPENCPU_FUNC void ListInsertHead(List_t *pxList,ListHeader_t *p_List_header)
{
	DisablePrimask();

	List_t *end_list = (List_t *)(p_List_header->head);

	if(end_list != NULL)
		pxList->next = (struct List_t *)end_list;
	else
		p_List_header->head = pxList;

	p_List_header->head = pxList;
	p_List_header->node_count += 1;
	EnablePrimask();
}

__OPENCPU_FUNC List_t *ListRemove(ListHeader_t *p_List_header)
{
	DisablePrimask();

	List_t *header_node = p_List_header->head;

	if(header_node != NULL)
	{
		p_List_header->head = (List_t *)(header_node->next);
		p_List_header->node_count -= 1;
		
		if(header_node->next == NULL)
			p_List_header->tail = NULL;
	}
	else
		xy_assert(p_List_header->node_count == 0);

	EnablePrimask();

	return header_node;
}


__OPENCPU_FUNC void ListFreeAll(ListHeader_t *p_List_header)
{
	DisablePrimask();
	List_t *p_node;
	while((p_node = ListRemove(p_List_header)) != NULL)
	{
		xy_free(p_node);
	}
	EnablePrimask();
}

__OPENCPU_FUNC uint32_t GetListNum(ListHeader_t *p_List_header)
{
	uint32_t count;
    DisablePrimask();
	count = p_List_header->node_count;
	EnablePrimask();

	return count;
}

__OPENCPU_FUNC void ListAddDropCount(ListHeader_t *p_List_header)
{	
    DisablePrimask();
	p_List_header->drop_count++;
	EnablePrimask();
}


