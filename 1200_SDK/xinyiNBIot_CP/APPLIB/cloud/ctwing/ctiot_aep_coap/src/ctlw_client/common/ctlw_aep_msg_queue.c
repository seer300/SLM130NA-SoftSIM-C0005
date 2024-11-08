#include "ctlw_aep_msg_queue.h"
#include "ctlw_lwm2m_sdk.h"

ctiot_msg_list_head *ctiot_coap_queue_init(uint32_t maxMsgCount)
{
	ctiot_msg_list_head *list = (ctiot_msg_list_head *)ctlw_lwm2m_malloc(sizeof(ctiot_msg_list_head));
	if (list == NULL)
	{
		return NULL;
	}
	static uint16_t index = 0;
	char queueMutexName[20]={0};
	sprintf(queueMutexName,"QUE_MUT_%u\0",index);

	if (thread_mutex_init(&list->mut, queueMutexName))
	{
		ctlw_lwm2m_free(list);
		return NULL;
	}
	index++;

	list->max_msg_num = maxMsgCount;
	list->msg_count = 0;
	list->head = NULL;
	list->tail = NULL;

	return list;
}

int16_t ctiot_coap_queue_add(ctiot_msg_list_head *list, void *ptr)
{
	ctiot_list_t *node = (ctiot_list_t *)ptr;
	if (list == NULL || node == NULL)
	{
		return CTIOT_OTHER_ERROR;
	}
	if (list->msg_count >= list->max_msg_num)
	{
		return CTIOT_OTHER_ERROR;
	}

	thread_mutex_lock(&list->mut);
	node->next = NULL;
	if (list->head == NULL)
	{
		list->head = node;
		list->tail = node;
	}
	else
	{
		list->tail->next = node;
		list->tail = node;
	}
	list->msg_count++;
	thread_mutex_unlock(&list->mut);

	return CTIOT_NB_SUCCESS;
}

int16_t ctiot_coap_queue_add_msg(ctiot_msg_list_head *list, void *ptr, void *remove)
{
	ctiot_list_t *node = (ctiot_list_t *)ptr;
	ctiot_list_t **remove_node = (ctiot_list_t **)remove;
	if (list == NULL || node == NULL)
	{
		return CTIOT_OTHER_ERROR;
	}

	if (list->msg_count >= list->max_msg_num)
	{
		ctiot_remove_first_finished_msg(list, remove);
		if ((*remove_node) == NULL)
		{
			return CTIOT_OTHER_ERROR;
		}
	}

	return ctiot_coap_queue_add(list, ptr);
}

ctiot_list_t *ctiot_coap_queue_find(ctiot_msg_list_head *list, uint16_t msgId)
{
	if (list == NULL)
	{
		return NULL;
	}
	ctiot_list_t *pTmp = list->head;

	while (pTmp != NULL)
	{
		if (pTmp->msgId == msgId)
		{
			return pTmp;
		}
		pTmp = pTmp->next;
	}

	return NULL;
}

ctiot_list_t *ctiot_coap_queue_get(ctiot_msg_list_head *list)
{
	if (list == NULL)
	{
		return NULL;
	}
	ctiot_list_t *pTmp = list->head;

	thread_mutex_lock(&list->mut);

	if (pTmp != NULL)
	{
		list->head = pTmp->next;
		if (list->head == NULL)
		{
			list->tail = NULL;
		}
		list->msg_count--;
	}
	thread_mutex_unlock(&list->mut);

	return pTmp;
}

int16_t ctiot_coap_queue_remove(ctiot_msg_list_head *list, uint16_t msgId, void *ptr)
{
	ctiot_list_t **node = (ctiot_list_t **)ptr;
	if (list == NULL || list->head == NULL)
	{
		return CTIOT_OTHER_ERROR;
	}
	ctiot_list_t *prev;
	ctiot_list_t *next;
	ctiot_list_t *curr;
	thread_mutex_lock(&list->mut);

	if (list->head->msgId == msgId)
	{

		curr = list->head;
		list->head = list->head->next;
		(*node) = curr;
		//ctlw_lwm2m_free(curr);
		if (list->head == NULL)
		{
			list->tail = NULL;
		}
		list->msg_count--;
	}
	else
	{
		curr = list->head->next;
		prev = list->head;
		next = curr->next;
		while (curr != NULL)
		{
			if (curr->msgId == msgId)
			{
				prev->next = next;
				if (curr == list->tail)
				{
					list->tail = prev;
				}
				(*node) = curr;
				list->msg_count--;
				break;
			}
			prev = curr;
			curr = next;
			if (next != NULL)
			{
				next = next->next;
			}
		}
	}
	thread_mutex_unlock(&list->mut);

	return CTIOT_NB_SUCCESS;
}

int16_t ctiot_change_msg_status(ctiot_msg_list_head *list, uint16_t msgId, uint16_t status)
{
	thread_mutex_lock(&list->mut);

	ctiot_list_t *node = ctiot_coap_queue_find(list, msgId);
	if (node == NULL)
	{
		thread_mutex_unlock(&list->mut);
		return CTIOT_OTHER_ERROR;
	}
	node->msgStatus = status;

	thread_mutex_unlock(&list->mut);

	return CTIOT_NB_SUCCESS;
}

int16_t ctiot_remove_first_finished_msg(ctiot_msg_list_head *list, void *ptr)
{
	ctiot_list_t **node = (ctiot_list_t **)ptr;
	if (list == NULL || list->head == NULL)
	{
		return CTIOT_OTHER_ERROR;
	}
	ctiot_list_t *prev;
	ctiot_list_t *next;
	ctiot_list_t *curr;
	thread_mutex_lock(&list->mut);

	if (list->head->msgStatus != QUEUE_SEND_DATA_SENDOUT && list->head->msgStatus != QUEUE_SEND_DATA_CACHEING) // 细化哪些状态可以删除
	{

		curr = list->head;
		list->head = list->head->next;
		(*node) = curr;
		if (list->head == NULL)
		{
			list->tail = NULL;
		}
		list->msg_count--;
	}
	else
	{
		curr = list->head->next;
		prev = list->head;
		while (curr != NULL)
		{
			next = curr->next;

			if (curr->msgStatus != QUEUE_SEND_DATA_SENDOUT && curr->msgStatus != QUEUE_SEND_DATA_CACHEING)
			{
				prev->next = next;
				if (curr == list->tail)
				{
					list->tail = prev;
				}
				(*node) = curr;
				list->msg_count--;
				break;
			}
			prev = curr;
			curr = next;
		}
	}
	thread_mutex_unlock(&list->mut);

	return CTIOT_NB_SUCCESS;
}

ctiot_list_t *ctiot_coap_queue_get_msg(ctiot_msg_list_head *list, uint16_t msgStatus)
{
	if (list == NULL)
	{
		return NULL;
	}

	ctiot_list_t *pTmp = list->head;
	thread_mutex_lock(&list->mut);

	while (pTmp != NULL)
	{
		if (pTmp->msgStatus == msgStatus)
		{
			thread_mutex_unlock(&list->mut);
			return pTmp;
		}
		pTmp = pTmp->next;
	}

	thread_mutex_unlock(&list->mut);
	return NULL;
}

uint16_t ctiot_get_available_len(ctiot_msg_list_head *list)
{
	uint16_t len = 0;
	thread_mutex_lock(&list->mut);

	if (list == NULL)
	{
		return 0;
	}

	ctiot_list_t *pTmp = list->head;

	while (pTmp != NULL)
	{
		if (pTmp->msgStatus != QUEUE_SEND_DATA_SENDOUT && pTmp->msgStatus != QUEUE_SEND_DATA_CACHEING)
		{
			len++;
		}
		pTmp = pTmp->next;
	}

	len += (list->max_msg_num - list->msg_count);

	thread_mutex_unlock(&list->mut);

	return len;
}
