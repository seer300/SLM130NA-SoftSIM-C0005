#ifndef _CTLW_AEP_MSG_QUEUE_H
#define _CTLW_AEP_MSG_QUEUE_H


#include "ctlw_config.h"
#ifdef PLATFORM_XINYI
#include "ctlw_abstract_os.h"
#include "time.h"
#include "cmsis_os2.h"
#endif


#include "ctlw_liblwm2m.h"



typedef enum
{
	QUEUE_SEND_SUCCESS,			 //发送成功
	QUEUE_SEND_DATA_CACHEING = 2,	//缓存中
	QUEUE_SEND_DATA_SENDOUT = 3,	 //已发送
	QUEUE_SEND_DATA_MSGID_ERROR = 1, //msgId无效
} ctiot_queue_msg_status;


typedef struct _ctiot_list_t
{
	struct _ctiot_list_t *next;
	uint16_t msgId;
	uint16_t msgStatus;
} ctiot_list_t;

typedef struct _ctiot_msg_list_head
{
	ctiot_list_t *head;
	ctiot_list_t *tail;
	uint16_t msg_count;
	uint16_t max_msg_num;
	thread_mutex_t mut;
	uint8_t init;
} ctiot_msg_list_head;

ctiot_msg_list_head *ctiot_coap_queue_init(uint32_t maxMsgCount);
int16_t ctiot_coap_queue_add(ctiot_msg_list_head *list, void *ptr);
ctiot_list_t *ctiot_coap_queue_find(ctiot_msg_list_head *list, uint16_t msgId);
ctiot_list_t *ctiot_coap_queue_get(ctiot_msg_list_head *list);
int16_t ctiot_coap_queue_remove(ctiot_msg_list_head *list, uint16_t msgId, void *ptr);
int16_t ctiot_change_msg_status(ctiot_msg_list_head *list, uint16_t msgId, uint16_t status);
int16_t ctiot_remove_first_finished_msg(ctiot_msg_list_head *list, void *ptr);
ctiot_list_t *ctiot_coap_queue_get_msg(ctiot_msg_list_head *list, uint16_t msgStatus);
uint16_t ctiot_get_available_len(ctiot_msg_list_head *list);
int16_t ctiot_coap_queue_add_msg(ctiot_msg_list_head *list, void *ptr, void *remove);

#endif//_CTLW_AEP_MSG_QUEUE_H
