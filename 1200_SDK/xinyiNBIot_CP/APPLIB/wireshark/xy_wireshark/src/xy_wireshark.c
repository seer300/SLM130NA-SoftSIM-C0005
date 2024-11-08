#include "xy_utils.h"
#include "xy_system.h"
#include "xy_wireshark.h"
#include "ipc_msg.h"
#include "ipc_msg.h"


int g_wireshark_set = 0;
osThreadId_t g_wireshark_TskHandle = NULL;
osMessageQueueId_t wireshark_msg_q = NULL;

void wireshark_forward_format_print(void *data, unsigned short len, char in)
{
	if(g_wireshark_set == 1)
	{
		struct common_msg *msg = NULL;
		
		msg = xy_malloc(sizeof(struct common_msg) + len);
		msg->size = len;

		if (data != NULL)
			memcpy(msg->data, data, len);
		
		if(in == 0)
			msg->msg_id = WIRESHARK_UP_DATA;
		else if(in == 1)
			msg->msg_id = WIRESHARK_DOWN_DATA;
		
		osMessageQueuePut(wireshark_msg_q, &msg, 0, osWaitForever);
	}
}

void wireshark_packet_proc(void *data, unsigned short len, char in)
{
	diag_wireshark_dataAP((char *)data, len, in, osKernelGetTickCount());
}

void wireshark_task()
{
	struct common_msg *msg = NULL;
	wireshark_msg_q = osMessageQueueNew(50, sizeof(void *), NULL);

	while(1) 
	{	
		osMessageQueueGet(wireshark_msg_q, (void *)(&msg), NULL, osWaitForever);
		if(msg == NULL)
		{
            xy_assert(0);
            continue;
		}
		switch(msg->msg_id)
		{
			case WIRESHARK_DOWN_DATA:
				wireshark_packet_proc(msg->data,msg->size,1);
				break;
			case WIRESHARK_UP_DATA:
				wireshark_packet_proc(msg->data,msg->size,0);
				break;
			case WIRESHARK_END:
				goto END;
			default:
				break;
		}
		xy_free(msg);
	}
END:
	xy_free(msg);
	if(wireshark_msg_q != NULL)
	{
    	void *elem = NULL;
        while (osMessageQueueGet(wireshark_msg_q, &elem, NULL, 0) == osOK)
        {
        	xy_free(elem);
        }
    	osMessageQueueDelete(wireshark_msg_q);
		wireshark_msg_q = NULL;
	}
	g_wireshark_TskHandle = NULL;
	osThreadExit();
	return;
}

