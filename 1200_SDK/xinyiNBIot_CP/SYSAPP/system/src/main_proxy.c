/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "main_proxy.h"
#include "at_worklock.h"
#include "ipc_msg.h"
#include "ps_netif_api.h"
#include "xy_system.h"
#include "xy_flash.h"
#include "ps_netif_api.h"
#include "net_api_priv.h"
#include "rtc_tmr.h"
#include "xy_fota.h"

#if	AT_SOCKET
#include "at_socket_context.h"
#endif /* AT_SOCKET */
#if TELECOM_VER || MOBILE_VER
#include "cloud_utils.h"
#endif
#if TELECOM_VER
#include "cdp_backup.h"
#endif
#if MOBILE_VER
#include "onenet_utils.h"
#endif
#if CTWING_VER
#include "ctwing_resume.h"
#endif

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define XY_PROXY_THREAD_STACKSIZE 		osStackShared
#define XY_PROXY_THREAD_NAME 			"xy_proxy_ctl"
#define XY_PROXY_THREAD_PRIO 			osPriorityAboveNormal

/*******************************************************************************
 *						   Global variable definitions				           *
 ******************************************************************************/
osMessageQueueId_t xy_proxy_msg_q = NULL; //proxy msg queue
osMutexId_t xy_proxy_mutex = NULL;

typedef struct
{
	uint32_t  offset;
	uint32_t  len;
	uint8_t   param[0];
}NV_Data;


/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
void xy_proxy_ctl(void)
{
	xy_proxy_msg_t *msg = NULL;

	if(Get_Boot_Reason()==SOFT_RESET && Get_Boot_Sub_Reason()==SOFT_RB_BY_FOTA)
	{
		OTA_update_upgrade_result();
	}

#if MOBILE_VER
	onet_remove_session();
#endif
#if AT_SOCKET
	clear_socket_backup();
#endif
	while(1) 
	{
		osMessageQueueGet(xy_proxy_msg_q, (void *)(&msg), NULL, osWaitForever);
		switch (msg->msg_id)
		{			
			case PROXY_MSG_FRAME_TIME:
			{
				update_snapshot_by_frame((PhyFrameInfo *)(msg->data));
				break;
			}

			case PROXY_WRITE_AP_NV_PARAM:
			{
				NV_Data *nv_val = (NV_Data *)(msg->data);
				
				xy_assert(msg->size == sizeof(NV_Data)+nv_val->len);
				xy_ftl_write(NV_FLASH_FACTORY_BASE+nv_val->offset,nv_val->param,nv_val->len);
				
				break;
			}
			case PROXY_MSG_PS_PDP_ACT:
			{
				ps_netif_activate((PsNetifInfo *)(msg->data));
				break;
			}

			case PROXY_MSG_PS_PDP_DEACT:
			{
				ps_netif_deactivate(*((unsigned char *)msg->data));
				break;
			}

			case PROXY_MSG_IPDATA:
			{
				proc_downlink_packet((proxy_downlink_data_t *)(msg->data));
				break;
			}

			case PROXY_MSG_DNS_INIT:
			{
				dns_server_init();
				break;
			}
			case PROXY_MSG_TCP_ACK:
			{
#if AT_SOCKET
				proc_tcp_ack(((tcp_ack_info_t *)msg->data)->socket,((tcp_ack_info_t *)msg->data)->ack_no, ((tcp_ack_info_t *)msg->data)->ack_len);
#endif //AT_SOCKET
				break;
			}
			case PROXY_MSG_REMOVE_DELAY_LOCK:
			{
				at_delaylock_deact();
				break;
			}
			default:
				break;
		}
		xy_free(msg);
	}
}

__RAM_FUNC int send_msg_2_proxy(int msg_id, void *buff, int len)
{
	if( msg_id != PROXY_MSG_REMOVE_DELAY_LOCK)
	{
		xy_mutex_acquire(xy_proxy_mutex, osWaitForever);
	}

	xy_assert(xy_proxy_msg_q != NULL);
	xy_proxy_msg_t *msg = NULL;
	
	msg = xy_malloc(sizeof(xy_proxy_msg_t) + len);
	msg->msg_id = msg_id;	
	msg->size = len;

	if (buff != NULL)
		memcpy(msg->data, buff, len);

	if (msg_id == PROXY_MSG_REMOVE_DELAY_LOCK)
	{
		osMessageQueuePut(xy_proxy_msg_q, &msg, 0, 0);
		return 1;
	}
	else if (msg_id == PROXY_MSG_IPDATA)
	{
		if (osMessageQueuePut(xy_proxy_msg_q, &msg, 0, 0) != osOK)
		{
			xy_free(msg);
			xy_mutex_release(xy_proxy_mutex);
			return 0;
		}		
	}
	else
	{
		osMessageQueuePut(xy_proxy_msg_q, &msg, 0, osWaitForever);		
	}

	xy_mutex_release(xy_proxy_mutex);
	return 1;
}


void xy_proxy_init(void)
{
	xy_proxy_msg_q = osMessageQueueNew(30, sizeof(void *), NULL);
	xy_proxy_mutex = osMutexNew(NULL);
	osThreadAttr_t thread_attr = {0};
	thread_attr.name 			= XY_PROXY_THREAD_NAME;
	thread_attr.priority 		= XY_PROXY_THREAD_PRIO;
	thread_attr.stack_size 		= XY_PROXY_THREAD_STACKSIZE;
	osThreadNew((osThreadFunc_t)(xy_proxy_ctl), NULL, &thread_attr);

extern void Before_DeepSleep_Hook(void);
	DeepSleep_Before_Regist(Before_DeepSleep_Hook);
	regist_system_callback();
}

