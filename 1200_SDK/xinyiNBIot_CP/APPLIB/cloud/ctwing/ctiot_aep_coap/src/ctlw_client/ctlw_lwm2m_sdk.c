/************************************************************************

            (c) Copyright 2019 by 天翼物联科技有限公司 All rights reserved.

**************************************************************************/

#include "ctlw_config.h"
#ifdef PLATFORM_XINYI
#include "ctlw_fota.h"
#include "ctwing_util.h"
#include "xy_utils.h"
#include "ctwing_resume.h"
#include "xy_ctwing_api.h"
#include "xy_socket_api.h"
#include "low_power.h"
#endif


#include "ctlw_lwm2mclient.h"
#include "ctlw_liblwm2m.h"
#include "ctlw_internals.h"
#include "ctlw_aep_msg_queue.h"
#include "ctlw_lwm2m_sdk.h"
#include "ctlw_platform.h"
#include <errno.h>
#include "ctlw_sdk_internals.h"
#include "ctlw_NV_data.h"
#include "ctlw_abstract_os.h"
#include "ctlw_abstract_signal.h"
#include "ctlw_parseuri.h"

#ifdef CTLW_APP_DEMO
/*user app*/
#include "ctlw_user_fota.h"
#endif



#include "ctlw_connection.h"

#ifdef MAX_OBJECT_COUNT
#undef MAX_OBJECT_COUNT
#endif
#ifdef PLATFORM_LINUX
#include "../../sample/linux/event.h"
#include "../../sample/linux/chip_info.h"
#include <sys/ioctl.h>
#endif

#ifdef WITH_FOTA
#define MAX_OBJECT_COUNT 7
#else
#define MAX_OBJECT_COUNT 6
#endif

#ifdef PLATFORM_XYZ
#include "ps_lib_api.h"
#endif

#if CTIOT_WITH_PAYLOAD_ENCRYPT == 1
//包含芯片加密方法的头文件
#endif

#if CTIOT_CHIPSUPPORT_DTLS == 1
#include "ctlw_mbedtls_interface.h"
#endif
/*for XYZ  app test*/
#ifdef CTLW_ANYTIME_SOCKET
bool laterRecoverNetwork = false;////延时恢复Lw层的connection
#endif


/*******************************************************************************/
thread_handle_t sendRecvThreadHandle, releaseThreadHandle;
//object_instance_array_t objInsArray[MAX_MCU_OBJECT] = {0};
system_boot_reason_e  startReason = CTIOT_UNKNOWN_STATE;
static volatile uint8_t sDataFlashNeeded = CTIOT_DATA_UNCHANGED; //session 数据是否变化
static uint8_t ctlwIsBusy = 0;
#if CTIOT_TIMER_AUTO_UPDATE == 1
static uint8_t autoUpdateFlag = 0;
#endif
#ifdef PLATFORM_XINYI
uint8_t *g_ctlw_buffer = NULL;//下行数据包处理buffer,此全局用于优化局部静态变量
#endif
/*******************************************************************************/
static void ctiotprv_set_server_ip_type(ctiot_context_t* pContext)
{
	if(pContext->serverIPV4 != NULL)
	{
		if(pContext->serverIPV6 != NULL)
		{
			pContext->serverIPType = SERVER_IP_TYPE_V4V6;
		}
		else
		{
			pContext->serverIPType = SERVER_IP_TYPE_V4ONLY;
		}
	}
	else if(pContext->serverIPV6 != NULL)
	{
		pContext->serverIPType = SERVER_IP_TYPE_V6ONLY;
	}
}
static int ctiotprv_get_port(ctiot_context_t *pContext, int port)
{
	if(port == 0)
	{
		switch(pContext->connectionType)
		{
			case MODE_DTLS:
				return CTIOT_DEFAULT_DTLS_PORT;
			case MODE_DTLS_PLUS:
				return CTIOT_DEFAULT_DTLSPLUS_PORT;
			case MODE_ENCRYPTED_PAYLOAD:
				return CTIOT_DEFAULT_ENCRYPT_PORT;
			case MODE_NO_DTLS:
			default:
				return CTIOT_DEFAULT_PORT;
		}
	}
	else
		return port;
}
bool ctiotprv_init_system_list(ctiot_context_t *pContext)
{
	if (pContext->upMsgList == NULL)
	{
		pContext->upMsgList = ctiot_coap_queue_init(CTIOT_MAX_QUEUE_SIZE);
	}
#ifdef CTLW_APP_FUNCTION
	if (pContext->appMsgList == NULL)
	{
		pContext->appMsgList = ctiot_coap_queue_init(CTIOT_MAX_QUEUE_SIZE);
	}
#endif
	if (pContext->downMsgList == NULL)
	{
		pContext->downMsgList = ctiot_coap_queue_init(CTIOT_MAX_QUEUE_SIZE);
	}
#ifdef CTLW_APP_FUNCTION
	if(pContext->upMsgList == NULL || pContext->appMsgList == NULL || pContext->downMsgList == NULL)
#else
	if(pContext->upMsgList == NULL || pContext->downMsgList == NULL)
#endif
	{
		return false;
	}
	return true;
}

uint16_t ctiotprv_get_ip_status(uint16_t serverIPType, uint8_t getIPTypeFunc)// 0 代表 从适配层获取ip状态， 1 代表 从芯片直接获取ip状态
{
	uint16_t ipStatus = 0;
	uint16_t chipIPType;
	if(getIPTypeFunc == 0)
	{
		chipIPType = ctiot_signal_get_chip_ip_type();
	}
	else
	{
		chipIPType = ctlw_get_ip_type();
	}

	switch(serverIPType)
	{
		case SERVER_IP_TYPE_V4ONLY:
		{
			switch(chipIPType)
			{
				case CHIP_IP_TYPE_V4ONLY:
				case CHIP_IP_TYPE_V4V6:
				case CHIP_IP_TYPE_V4V6_V6PREPARING:
				{
					ipStatus = SDK_IP_STATUS_TRUE_V4;
					break;
				}
				case CHIP_IP_TYPE_V6ONLY:
				case CHIP_IP_TYPE_V6ONLY_V6PREPARING:
				{
					ipStatus = SDK_IP_STATUS_DISABLE;
					break;
				}
				default:
				{
					ipStatus = SDK_IP_STATUS_FALSE;
					break;
				}
			}
			break;
		}
		case SERVER_IP_TYPE_V6ONLY:
		{
			switch(chipIPType)
			{
				case CHIP_IP_TYPE_V6ONLY:
				case CHIP_IP_TYPE_V4V6:
				{
					ipStatus = SDK_IP_STATUS_TRUE_V6;
					break;
				}
				case CHIP_IP_TYPE_V4V6_V6PREPARING:
				case CHIP_IP_TYPE_V6ONLY_V6PREPARING:
				{
					ipStatus = SDK_IP_STATUS_V6PREPARING;
					break;
				}
				case CHIP_IP_TYPE_V4ONLY:
				{
					ipStatus = SDK_IP_STATUS_DISABLE;
					break;
				}
				default:
				{
					ipStatus = SDK_IP_STATUS_FALSE;
					break;
				}
			}
			break;
		}
		case SERVER_IP_TYPE_V4V6:
		{
			switch(chipIPType)
			{
				case CHIP_IP_TYPE_V4ONLY:
				case CHIP_IP_TYPE_V4V6:
				case CHIP_IP_TYPE_V4V6_V6PREPARING:
				{
					ipStatus = SDK_IP_STATUS_TRUE_V4;
					break;
				}
				case CHIP_IP_TYPE_V6ONLY:
				{
					ipStatus = SDK_IP_STATUS_TRUE_V6;
					break;
				}
				case CHIP_IP_TYPE_V6ONLY_V6PREPARING:
				{
					ipStatus = SDK_IP_STATUS_V6PREPARING;
					break;
				}
				default:
				{
					ipStatus = SDK_IP_STATUS_FALSE;
					break;
				}
			}
			break;
		}
		default:
			ipStatus = SDK_IP_STATUS_FALSE;
			break;
	}
	return ipStatus;
}

//该方法待删除
int ctiotprv_get_socket_address_family(ctiot_context_t *pContext)
{
	int addressFamily = 0;//服务器未设置地址
	ctiotprv_set_server_ip_type(pContext);
	uint16_t ipStatus = ctiotprv_get_ip_status(pContext->serverIPType, 1);
	if(ipStatus == SDK_IP_STATUS_TRUE_V4)
	{
		addressFamily = AF_INET;
	}
	else if(ipStatus == SDK_IP_STATUS_TRUE_V6)
	{
		addressFamily = AF_INET6;
	}
	log_info("ctiotprv_get_socket_address_family:%d",addressFamily);
	return addressFamily;
}

int ctiotprv_switch_address_family_with_ip_status(uint16_t ipStatus)
{
	int addressFamily = 0;//服务器未设置地址
	if(ipStatus == SDK_IP_STATUS_TRUE_V4)
	{
		addressFamily = AF_INET;
	}
	else if(ipStatus == SDK_IP_STATUS_TRUE_V6)
	{
		addressFamily = AF_INET6;
	}
	ctiot_log_debug(LOG_COMMON_MODULE, LOG_IP_CLASS, "ctiotprv_switch_address_family_with_ip_status:%d",addressFamily);
	return addressFamily;
}

void ctiotprv_free_system_list(ctiot_context_t *pContext)
{
	thread_mutex_destroy(&(pContext->upMsgList->mut));
	ctlw_lwm2m_free(pContext->upMsgList);
#ifdef CTLW_APP_FUNCTION
	thread_mutex_destroy(&(pContext->appMsgList->mut));
	ctlw_lwm2m_free(pContext->appMsgList);
#endif
	thread_mutex_destroy(&(pContext->downMsgList->mut));
	ctlw_lwm2m_free(pContext->downMsgList);
}
/*只登录用*/
static int32_t ctiotprv_get_ep(ctiot_context_t *pContext, char *ep_name)
{
	if (ep_name == NULL)
	{
		return CTIOT_FALSE;
	}
	if (pContext->idAuthMode >= AUTHMODE_MAX)
	{
		return CTIOT_FALSE;
	}
	if (pContext->idAuthMode == AUTHMODE_EXTEND_MCU && (pContext->authTokenStr == NULL || pContext->authModeStr == NULL))
	{
		return CTIOT_FALSE;
	}
	if (pContext->idAuthMode == AUTHMODE_EXTEND_MODULE && pContext->authTokenStr == NULL)
	{
		return CTIOT_FALSE;
	}
	if (pContext->idAuthMode == AUTHMODE_STANDARD)
	{
		sprintf(ep_name, "urn:imei:%s\0", pContext->chipInfo->imei);
	}
	else if (pContext->idAuthMode == AUTHMODE_STANDARD_2)
	{
		sprintf(ep_name, "urn:imei-imsi:%s-%s", pContext->chipInfo->imei, pContext->chipInfo->imsi);
	}
	else if (pContext->idAuthMode == AUTHMODE_SIMPLIFIED)
	{
		sprintf(ep_name, "%s\0", pContext->chipInfo->imei);
	}
	else if (pContext->idAuthMode == AUTHMODE_EXTEND_MODULE)
	{
#if CTIOT_SIMID_ENABLED == 1
		if (pContext->idAuthType == AUTHTYPE_SIMID)
		{
			if (pContext->authModeStr != NULL)
			{
				ctlw_lwm2m_free(pContext->authModeStr);
				pContext->authModeStr = NULL;
			}
			pContext->authModeStr = ctlw_lwm2m_strdup("simid");
		}
#endif
#if CTIOT_SM9_ENABLED == 1
		if (pContext->idAuthType == AUTHTYPE_SM9)
		{
			if (pContext->authModeStr != NULL)
			{
				ctlw_lwm2m_free(pContext->authModeStr);
				pContext->authModeStr = NULL;
			}
			pContext->authModeStr = ctlw_lwm2m_strdup("sm9");
		}
#endif
		sprintf(ep_name, "urn:imei+%s:%s+%s", pContext->authModeStr, pContext->chipInfo->imei, pContext->authTokenStr);
	}
	else if (pContext->idAuthMode == AUTHMODE_EXTEND_MCU)
	{
		sprintf(ep_name, "urn:imei+%s:%s+%s", pContext->authModeStr, pContext->chipInfo->imei, pContext->authTokenStr);
	}
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS, "ep_name:%s\r\n", ep_name);

	return CTIOT_SUCCESS;
}

uint8_t ctiot_trace_ip_by_bindmode()
{
	ctiot_context_t* pContext = ctiot_get_context();

	uint8_t onIP = CTIOT_ONIP_NO_RECONNECT;
	if(pContext->contextBindMode == CTIOT_BINDMODE_U)
	{
		ctiot_log_debug(LOG_COMMON_MODULE, LOG_OTHER_CLASS, "ctiot_trace_ip_by_bindmode:drx");
		onIP = CTIOT_ONIP_RECONNECT;
	}
	else if(pContext->contextBindMode == CTIOT_BINDMODE_UQ)
	{
		if(pContext->connectionType == MODE_DTLS/* dtls plus 不订阅ip地址变化*/)
		{
			ctiot_log_debug(LOG_COMMON_MODULE, LOG_OTHER_CLASS, "ctiot_trace_ip_by_bindmode:psm dtls");
			onIP = CTIOT_ONIP_RECONNECT;
		}
		else
		{
			ctiot_log_debug(LOG_COMMON_MODULE, LOG_OTHER_CLASS, "ctiot_trace_ip_by_bindmode:psm no dtls/psm dtls plus");
		}
	}
	ctiot_log_debug(LOG_COMMON_MODULE, LOG_IP_CLASS, "ctiot_trace_ip_by_bindmode:%u\r\n", onIP);
	return onIP;

}

static void ctiotprv_trace_ip_by_workmode(ctiot_context_t *pContext)
{
	if(pContext->clientWorkMode == U_WORK_MODE)
	{
		pContext->onIP = CTIOT_ONIP_RECONNECT;
	}
	else if(pContext->clientWorkMode == UQ_WORK_MODE)
	{
		if(pContext->connectionType == MODE_DTLS/* dtls plus 不订阅ip地址变化*/)
		{
			pContext->onIP = CTIOT_ONIP_RECONNECT;
		}
		else
		{
			pContext->onIP = CTIOT_ONIP_NO_RECONNECT;
		}
	}
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_IP_CLASS, "ctiotprv_trace_ip_by_workmode:%u\r\n", pContext->onIP);
}

static void ctiotprv_handle_inner_update_reply(lwm2m_transaction_t *transacP,
										void *message)
{
	ctlw_coap_packet_t *packet = (ctlw_coap_packet_t *)message;
	ctiot_context_t *pContext = ctiot_get_context();
	if(pContext->lastInnerUpdateTransID != transacP->mID)
	{
		return;
	}
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS, "inner update reply arrived,msgId=%d...\r\n",transacP->mID);
	if (pContext->sessionStatus == UE_LOGINED) // 防止登出中发出通知
	{
		if(packet == NULL)
		{
			if (ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_OUTING))
			{
				ctiot_set_release_flag(RELEASE_MODE_INNER_UPDATE_TIMEOUT_L0, inupdatetimeout); //update timeout
				ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS, "update:timeout\r\n");
			}
		}
		else if(packet->code == 0x84 /*COAP_404*/) //现在产品没有涉及异步更新状态失败错误码
		{
			if (ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_OUTING))
			{
				ctiot_set_release_flag(RELEASE_MODE_INNER_UPDATE_404_L0, inupdate404); //update 404
				ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS, "update:404\r\n");
			}
		}
	}
}

static void ctiotprv_change_up_msg_status(ctiot_context_t *pContext, uint32_t msgID, uint16_t status)
{
	if (pContext->updateMsg != NULL && pContext->updateMsg->msgId == msgID)
	{
		pContext->updateMsg->msgStatus = status;
	}
	else if(ctiot_change_msg_status(pContext->upMsgList, msgID, status) != CTIOT_NB_SUCCESS)
	{
#ifdef CTLW_APP_FUNCTION
		ctiot_change_msg_status(pContext->appMsgList, msgID, status);
#endif
	}
}

static void ctiotprv_handle_outer_update_reply(lwm2m_transaction_t *transacP, void *message)
{
	ctlw_coap_packet_t *packet = (ctlw_coap_packet_t *)message;
	ctiot_context_t *pContext = ctiot_get_context();
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS, "outer update reply arrived...\r\n");
	if (pContext->sessionStatus == UE_LOGINED)
	{
		if (packet != NULL && packet->code == COAP_204_CHANGED)
		{
#ifdef PLATFORM_XINYI
			//update成功，更新注册时间
			pContext->lwm2mContext->serverList->registration = ctlw_lwm2m_gettime();
#endif
			ctiotprv_change_up_msg_status(pContext, transacP->mID, QUEUE_SEND_SUCCESS);
			ctiot_publish_sdk_notification("/19/0/0", CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_UPDATE, CTIOT_NB_SUCCESS, transacP->mID, 0, NULL);
		}
		else
		{
			if (packet == NULL)
			{
				ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS, "ctiotprv_handle_outer_update_reply:timeOut\r\n");
				ctiotprv_change_up_msg_status(pContext, transacP->mID, CTIOT_TIME_OUT_ERROR);
				ctiot_publish_sdk_notification("/19/0/0", CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_UPDATE, CTIOT_TIME_OUT_ERROR, transacP->mID, 0, NULL);
			}
			else if (packet->code == COAP_404_NOT_FOUND /*COAP_404*/)
			{
				//现在产品没有涉及异步更新状态失败错误码
				if (ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_OUTING))
				{
					ctiot_set_release_flag(RELEASE_MODE_OUTER_UPDATE_404_L0, atupdate404); //update 404
					ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS, "ctiotprv_handle_outer_update_reply:404\r\n");
#ifdef PLATFORM_XINYI
					ctiot_publish_sdk_notification(NULL,CTIOT_SYSTEM_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LSTATUS, CTIOT_LSTATUS_QUITSESSION, atupdate404, 0, NULL);
#endif
				}
			}
			else if (packet->code == COAP_400_BAD_REQUEST /*COAP_400*/)
			{
				ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS, "ctiotprv_handle_outer_update_reply:400\r\n");
				ctiotprv_change_up_msg_status(pContext, transacP->mID, CTIOT_BAD_REQ_ERROR);
				ctiot_publish_sdk_notification("/19/0/0", CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_UPDATE, CTIOT_BAD_REQ_ERROR, transacP->mID, 0, NULL);
			}
			else
			{
				ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS, "ctiotprv_handle_outer_update_reply other failed code:%d\n", packet->code);
				ctiotprv_change_up_msg_status(pContext, transacP->mID, CTIOT_PLAT_OTH_ERROR);
				ctiot_publish_sdk_notification("/19/0/0", CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_UPDATE, CTIOT_PLAT_OTH_ERROR, transacP->mID, 0, NULL);
			}
		}
	}
}

static bool ctiotprv_dataList_is_empty(ctiot_context_t *pContext)
{
	bool result = false;
	if ((pContext->upMsgList == NULL || pContext->upMsgList->max_msg_num == ctiot_get_available_len(pContext->upMsgList))
		&& (pContext->downMsgList == NULL || pContext->downMsgList->msg_count == 0)
		&& pContext->lwm2mContext->transactionList == NULL
		&& (pContext->updateMsg == NULL || (pContext->updateMsg->msgStatus != QUEUE_SEND_DATA_CACHEING && pContext->updateMsg->msgStatus != QUEUE_SEND_DATA_SENDOUT))
#ifdef CTLW_APP_FUNCTION
		&& (pContext->appMsgList == NULL || pContext->appMsgList->max_msg_num == ctiot_get_available_len(pContext->appMsgList))
#endif
		)
	{
		result = true;
	}
	return result;
}
#ifdef CTLW_APP_FUNCTION
int ctlw_add_user_object(ADDOBJFUNC addfunc)
{
	ctiot_context_t* pContext=ctiot_get_context();
	if(pContext->lwm2mContext == NULL)
		return -2;
	if(addfunc != NULL)
	{
		ctlw_lwm2m_object_t* pObject=addfunc();
		if(pObject!=NULL)
		{
			return ctlw_lwm2m_add_object(pContext->lwm2mContext,pObject);
		}
		else
		{
			return -4;
		}
	}
	else
	{
		return -3;
	}
}
#endif

#if CTIOT_CHIPSUPPORT_DTLS == 1
int ctlw_create_socket_dtls(const char *portStr, int addressFamily,char* serverAddr,int serverPort)
{
    int s = -1;
#if PLATFORM_XINYI
	ctiot_context_t * pContext = ctiot_get_context();
	uint16_t local_port = strtoul(portStr, NULL, 10);
	if(local_port == 0)
		return s;
	
	if(addressFamily == AF_INET)
	{
		pContext->portV4 = serverPort;
		s = xy_socket_by_host(serverAddr, Sock_IPv4_Only, IPPROTO_UDP, local_port, serverPort, NULL);
	}
	else if(addressFamily == AF_INET6)
	{
		pContext->portV6 = serverPort;
		s = xy_socket_by_host(serverAddr, Sock_IPv6_Only, IPPROTO_UDP, local_port, serverPort, NULL);
	}

#else
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = addressFamily;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    if (0 != getaddrinfo(NULL, portStr, &hints, &res))
    {
        return -1;
    }

    for (p = res; p != NULL && s == -1; p = p->ai_next)
    {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s >= 0)
        {
            if (-1 == bind(s, p->ai_addr, p->ai_addrlen))
            {
                close(s);
                s = -1;
            }
			/*connect to server*/
			if(s != -1)
			{
				if(addressFamily == AF_INET)
				{
					struct sockaddr_in servaddr;
				    memset(&servaddr, 0, sizeof(servaddr));
				    servaddr.sin_family = AF_INET;
				    servaddr.sin_port = htons(serverPort);  ///服务器端口
				    servaddr.sin_addr.s_addr = inet_addr(serverAddr);  ///服务器ip
				    ///连接服务器，成功返回0，错误返回-1
				    if (connect(s, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
				    {
						close(s);
						s = -1;

				    }
				}
				else
				{
					struct sockaddr_in6 servaddr;
				    memset(&servaddr, 0, sizeof(servaddr));
				    servaddr.sin6_family = AF_INET6;
				    servaddr.sin6_port = htons(serverPort);  ///服务器端口
				    inet_pton(addressFamily, serverAddr, &servaddr.sin6_addr);  ///服务器ip

				    ///连接服务器，成功返回0，错误返回-1
				    if (connect(s, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
				    {
						close(s);
						s = -1;

				    }
				}
			}
        }

    }

    freeaddrinfo(res);
#endif
	
    return s;
}
#endif

static bool ctiotprv_check_location(ctiot_context_t *pContext)
{
	lwm2m_server_t *pServer = pContext->lwm2mContext->serverList;
	while (pServer != NULL)
	{
		if (ctiot_location_path_validation(pServer->location) != CTIOT_NB_SUCCESS)
			return false;
		pServer = pServer->next;
	}
	return true;
}

static void ctiotprv_clear_lwm2mContext(ctiot_context_t *pContext)
{
	if (pContext->lwm2mContext != NULL)
	{
		if (pContext->lwm2mContext->endpointName != NULL)
		{
			ctlw_lwm2m_free(pContext->lwm2mContext->endpointName);
			pContext->lwm2mContext->endpointName = NULL;
		}
		if (pContext->lwm2mContext->msisdn != NULL)
		{
			ctlw_lwm2m_free(pContext->lwm2mContext->msisdn);
			pContext->lwm2mContext->msisdn = NULL;
		}
		if (pContext->lwm2mContext->altPath != NULL)
		{
			ctlw_lwm2m_free(pContext->lwm2mContext->altPath);
			pContext->lwm2mContext->altPath = NULL;
		}
		ctlw_lwm2m_free(pContext->lwm2mContext);
		pContext->lwm2mContext = NULL;
	}
}

static uint16_t ctiotprv_free_objects_by_array(ctlw_lwm2m_object_t *ctiotObjArray[], uint8_t count)
{
	while (count--)
	{
		ctlw_lwm2m_object_t *pTmpObject = ctiotObjArray[count];

		if (pTmpObject == NULL)
			continue;

		if (pTmpObject->objID == LWM2M_SECURITY_OBJECT_ID)
		{
			ctlw_clean_security_object(pTmpObject);
			ctlw_lwm2m_free(pTmpObject);
			ctiotObjArray[count] = NULL;
		}
		else if (pTmpObject->objID == LWM2M_SERVER_OBJECT_ID)
		{
			ctlw_clean_server_object(pTmpObject);
			ctlw_lwm2m_free(pTmpObject);
			ctiotObjArray[count] = NULL;
		}
		else if (pTmpObject->objID == LWM2M_DEVICE_OBJECT_ID)
		{
			ctlw_free_object_device(pTmpObject);
			ctiotObjArray[count] = NULL;
		}
#ifdef WITH_FOTA
		else if (pTmpObject->objID == LWM2M_FIRMWARE_UPDATE_OBJECT_ID)
		{
			ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS, "ctiotprv_free_objects_by_array:ctlw_free_object_firmware\r\n");
			ctlw_free_object_firmware(pTmpObject);
			ctiotObjArray[count] = NULL;
		}
#endif
		else if (pTmpObject->objID == LWM2M_CONN_MONITOR_OBJECT_ID)
		{
			ctlw_free_object_conn_m(pTmpObject);
			ctiotObjArray[count] = NULL;
		}
		else if (pTmpObject->objID == LWM2M_CONN_STATS_OBJECT_ID)
		{
			ctlw_free_object_conn_s(pTmpObject);
			ctiotObjArray[count] = NULL;
		}
		else if (pTmpObject->objID == DATA_REPORT_OBJECT)
		{
			ctlw_free_data_report_object(pTmpObject);
			ctiotObjArray[count] = NULL;
		}
		else
		{
			LWM2M_LIST_FREE(pTmpObject->instanceList);
			if (pTmpObject->userData != NULL)
			{
				ctlw_lwm2m_free(pTmpObject->userData);
			}
			ctlw_lwm2m_free(pTmpObject);
			ctiotObjArray[count] = NULL;
		}
	}

	return 0;
}

static uint16_t ctiotprv_free_objects_by_list(ctiot_context_t *pContext)
{
	if (pContext->lwm2mContext == NULL)
		return 1;
	ctlw_lwm2m_object_t *object = pContext->lwm2mContext->objectList;
	while (object != NULL)
	{
		ctlw_lwm2m_object_t *pTmpObject = object;
		object = object->next;
		if (pTmpObject->objID == LWM2M_SECURITY_OBJECT_ID)
		{
			ctlw_clean_security_object(pTmpObject);
			ctlw_lwm2m_free(pTmpObject);
		}
		else if (pTmpObject->objID == LWM2M_SERVER_OBJECT_ID)
		{
			ctlw_clean_server_object(pTmpObject);
			ctlw_lwm2m_free(pTmpObject);
		}
		else if (pTmpObject->objID == LWM2M_DEVICE_OBJECT_ID)
		{
			ctlw_free_object_device(pTmpObject);
		}
#ifdef WITH_FOTA
		else if (pTmpObject->objID == LWM2M_FIRMWARE_UPDATE_OBJECT_ID)
		{
			ctiot_log_info(LOG_COMMON_MODULE, LOG_OTHER_CLASS, "ctiotprv_free_objects_by_list:ctlw_free_object_firmware\r\n");
			ctlw_free_object_firmware(pTmpObject);
		}
#endif
		else if (pTmpObject->objID == LWM2M_CONN_MONITOR_OBJECT_ID)
		{
			ctlw_free_object_conn_m(pTmpObject);
		}
		else if (pTmpObject->objID == LWM2M_CONN_STATS_OBJECT_ID)
		{
			ctlw_free_object_conn_s(pTmpObject);
		}
		else if (pTmpObject->objID == DATA_REPORT_OBJECT)
		{
			ctlw_free_data_report_object(pTmpObject);
		}
		else
		{
			LWM2M_LIST_FREE(pTmpObject->instanceList);
			if (pTmpObject->userData != NULL)
			{
				ctlw_lwm2m_free(pTmpObject->userData);
			}
			ctlw_lwm2m_free(pTmpObject);
		}
	}
	return CTIOT_NB_SUCCESS;
}


static uint16_t ctiotprv_lwm2m_init(ctiot_context_t *pContext)
{
	uint16_t result = CTIOT_NB_SUCCESS;
	char serverUri[CTIOT_MAX_IP_LEN] = {0};

	int32_t ret;
	ctlw_lwm2m_object_t *ctiotObjArray[MAX_OBJECT_COUNT];
	char* serverIP=NULL;
	int32_t port=0;

	if(pContext->addressFamily == AF_INET)
	{
		serverIP = pContext->serverIPV4;
		port = pContext->portV4;
	}
	else
	{
		serverIP = pContext->serverIPV6;
		port = pContext->portV6;
	}

#if CTIOT_CHIPSUPPORT_DTLS == 1
	if(pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS)
	{

		sprintf(serverUri, "coaps://[%s]:%d", serverIP, ctiotprv_get_port(pContext,port));
	}
	else
	{
#endif
		sprintf(serverUri, "coap://[%s]:%d", serverIP, ctiotprv_get_port(pContext,port));
#if CTIOT_CHIPSUPPORT_DTLS == 1
	}
#endif

	ctiot_log_debug(LOG_INIT_MODULE, LOG_OTHER_CLASS,"enter ctiot_session_init,%s!!!\r\n",serverUri);
	ctiotprv_free_objects_by_list(pContext);

	uint16_t i = 0;
#if CTIOT_CHIPSUPPORT_DTLS == 1
	if(pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS)
	{
		ctiotObjArray[i] = ctlw_get_security_object(CTIOT_DEFAULT_SERVER_ID, serverUri, pContext->pskID, /*initInfo.u.pskInfo.pskStr*/ pContext->psk, pContext->pskLen /*initInfo.u.pskInfo.pskLen*/, false);
	}
else
	{
#endif
		ctiotObjArray[i] = ctlw_get_security_object(CTIOT_DEFAULT_SERVER_ID, serverUri, NULL, NULL, 0, false);
#if CTIOT_CHIPSUPPORT_DTLS == 1
	}
#endif

	//OBJ = 0
	uint32_t tmpLifetime = pContext->lifetime;
	char *bindMode = "U";
	char epName[150] = {0};

	if (ctiotObjArray[i] == NULL)
	{
		result = CTIOT_SYS_API_ERROR;
		ctiot_log_info(LOG_INIT_MODULE, LOG_OTHER_CLASS,"ctiotObjArray[%d]==NULL\r\n", i);
		goto exit;
	}
	pContext->clientInfo.securityObjP = ctiotObjArray[i];

	if (pContext->bootFlag == BOOT_LOCAL_BOOTUP)
	{
		tmpLifetime = pContext->contextLifetime;
	}
	else
	{
		pContext->contextLifetime = pContext->lifetime; //[checktag]需要check？？？？？？
	}

	pContext->contextBindMode = CTIOT_BINDMODE_U;
	if (pContext->clientWorkMode == UQ_WORK_MODE)
	{
		bindMode = "UQ";
		pContext->contextBindMode = CTIOT_BINDMODE_UQ;
	}

	ctiotObjArray[++i] = ctlw_get_server_object(CTIOT_DEFAULT_SERVER_ID, bindMode /*default binding mode*/, tmpLifetime, false);
	if (NULL == ctiotObjArray[i])
	{
		result = CTIOT_SYS_API_ERROR;
		ctiot_log_info(LOG_INIT_MODULE, LOG_OTHER_CLASS,"ctiotObjArray[%d]==NULL\r\n", i);
		goto exit;
	}
	pContext->clientInfo.serverObject = ctiotObjArray[i];

	//OBJ = 3
	ctiotObjArray[++i] = ctlw_get_object_device();
	if (NULL == ctiotObjArray[i])
	{
		result = CTIOT_SYS_API_ERROR;
		ctiot_log_info(LOG_INIT_MODULE, LOG_OTHER_CLASS,"ctiotObjArray[%d]==NULL\r\n", i);
		goto exit;
	}

	//OBJ = 4
	ctiotObjArray[++i] = ctlw_get_object_conn_m();
	if (NULL == ctiotObjArray[i])
	{
		result = CTIOT_SYS_API_ERROR;
		ctiot_log_info(LOG_INIT_MODULE, LOG_OTHER_CLASS,"ctiotObjArray[%d]==NULL\r\n", i);
		goto exit;
	}
    
#ifdef WITH_FOTA
	//OBJ = 5
	ctiotObjArray[++i] = ctlw_get_object_firmware();
	if (NULL == ctiotObjArray[i])
	{
		result = CTIOT_SYS_API_ERROR;
		ctiot_log_info(LOG_INIT_MODULE, LOG_OTHER_CLASS,"ctiotObjArray[%d]==NULL\r\n", i);
		goto exit;
	}
    else
    	ctiot_log_debug(LOG_INIT_MODULE, LOG_OTHER_CLASS,"ctiotObjArray[%d]\r\n", i);
#endif
	//OBJ = 19
	ctiotObjArray[++i] = ctlw_get_data_report_object();
	if (NULL == ctiotObjArray[i])
	{
		result = CTIOT_SYS_API_ERROR;
		ctiot_log_info(LOG_INIT_MODULE, LOG_OTHER_CLASS,"ctiotObjArray[%d]==NULL\r\n", i);
		goto exit;
	}
	pContext->lwm2mContext = ctlw_lwm2m_init(&(pContext->clientInfo));

	if (pContext->bootFlag == BOOT_LOCAL_BOOTUP)//恢复的会话无需epname,本处赋a,为通过开源库的configure检查
	{
		sprintf(epName, "%s\0", "a");
	}
	else//登录
	{
		ret = ctiotprv_get_ep(pContext, epName);
		if (ret != CTIOT_SUCCESS)
		{
			result = CTIOT_OTHER_ERROR;
			goto exit;
		}
	}
	result = ctlw_lwm2m_configure(pContext->lwm2mContext, epName, NULL, NULL, i + 1, ctiotObjArray);
	if (result != 0)
	{
		result = CTIOT_SYS_API_ERROR; //ctlw_lwm2m_configure fail
		ctiot_log_info(LOG_INIT_MODULE, LOG_OTHER_CLASS,"result!= 0\r\n");
		goto exit;
	}
#ifdef CTLW_APP_FUNCTION
	ctiot_add_user_objects();
#endif
	pContext->lwm2mContext->state = STATE_INITIAL;
exit:

	if (result != CTIOT_NB_SUCCESS)
	{
		ctiotprv_free_objects_by_array(ctiotObjArray, i + 1);
		ctiotprv_clear_lwm2mContext(pContext);
	}
	return result;
}

static bool ctiotprv_recover_connect(ctiot_context_t *pContext)
{
	if ( pContext->lwm2mContext == NULL)
	{
		ctiot_log_info(LOG_COMMON_MODULE, LOG_OTHER_CLASS, "lwm2m not initialized!\r\n");
		return false;
	}
	lwm2m_server_t *server = pContext->lwm2mContext->serverList;
	pContext->lwm2mContext->userData = (void *)&pContext->clientInfo;
	while(server)
	{
		if (server->sessionH == NULL)
		{
			server->sessionH = ctlw_lwm2m_connect_server(server->secObjInstID, pContext->lwm2mContext->userData);
		}
		if (server->sessionH == NULL)
		{
			ctiot_log_info(LOG_COMMON_MODULE, LOG_OTHER_CLASS, "connect to server failed\r\n");
		}
		server = server->next;
	}
	return true;
}

int ctiotprv_net_is_validipv6(const char *hostAddr)
{
	struct sockaddr_in6 addr;

	if (!hostAddr)
		return CTIOT_FALSE;
	if (strchr(hostAddr, '.'))
		return CTIOT_FALSE; //暂时排除::ffff:204.152.189.116
	if (inet_pton(AF_INET6, hostAddr, &addr.sin6_addr) != 1)
		return CTIOT_FALSE;

	return CTIOT_SUCCESS;
}

int ctiotprv_net_is_validipv4(const char *hostAddr)
{
	struct sockaddr_in addr;

	if (!hostAddr)
		return CTIOT_FALSE;
	if (strchr(hostAddr, ':'))
		return CTIOT_FALSE;
	if (inet_pton(AF_INET, hostAddr, &addr.sin_addr) != 1)
		return CTIOT_FALSE;

	return CTIOT_SUCCESS;
}

static uint16_t ctiotprv_update_server_socket(ctiot_context_t *pContext, int sock)
{
	ctiot_log_debug(LOG_COMMON_MODULE, LOG_SOCKET_CLASS, "ctiotprv_update_server_socket >>\r\n");
	uint16_t result = CTIOT_OTHER_ERROR;

	lwm2m_context_t *pLwContext = pContext->lwm2mContext;
	if (pLwContext != NULL)
	{
		lwm2m_server_t *serverList = pLwContext->serverList;
		while (serverList != NULL)
		{
			connection_t *connP = (connection_t *)serverList->sessionH;
			if(connP != NULL)
			{
				connP->sock = sock;
			}
			serverList = serverList->next;
		}
		result = CTIOT_NB_SUCCESS;
	}

	ctiot_log_debug(LOG_COMMON_MODULE, LOG_SOCKET_CLASS, "ctiotprv_update_server_socket <<\r\n");
	return result;
}

static uint16_t ctiotprv_check_error_of_socket()//部分错误码执行了socket清理操作
{
	ctiot_context_t *pContext = ctiot_get_context();
	uint16_t code = CTIOT_OTHER_ERROR;
	int32_t errorNo= ctchip_get_sock_errno(pContext->clientInfo.sock);
	switch ( errorNo  )
	{
		case ENOMEM:
			ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_SOCKET_CLASS, "error no: ENOMEM \r\n");
			code = CTIOT_SYS_API_ERROR;
			break;
		case EBADF:
			ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_SOCKET_CLASS, "error no: EBADF \r\n");
			//ctiot_close_socket();
			pContext->clientInfo.sock = -1;
#if CTIOT_CHIPSUPPORT_DTLS == 1
			if (pContext->connectionType != MODE_NO_DTLS && pContext->connectionType != MODE_ENCRYPTED_PAYLOAD)
			{
				ctlw_dtls_update_socket_fd(pContext);
			}
			else
			{
				ctiotprv_update_server_socket(pContext, pContext->clientInfo.sock);
			}	
#endif
#if CTIOT_CHIPSUPPORT_DTLS == 0
			ctiotprv_update_server_socket(pContext, pContext->clientInfo.sock);
#endif			
			ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_SOCKET_CLASS,"pContext->clientInfo.sock = %d",pContext->clientInfo.sock);
			code = CTIOT_OTHER_ERROR;
			break;
		case EIO:
			ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_SOCKET_CLASS, "error no: EIO \r\n");
			ctiot_close_socket();
			code = CTIOT_OTHER_ERROR;
			break;
		case EINVAL:
			ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_SOCKET_CLASS, "error no: EINVAL \r\n");
			ctiot_close_socket();
			code = CTIOT_OTHER_ERROR;
			break;
		case ENOBUFS://芯片集成时待定,发送缓冲区满错误
			ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_SOCKET_CLASS, "error no: ENOBUFS \r\n");
			code = CTIOT_SOCKET_SEND_BUFFER_OVERRUN;
			break;
		default:
			ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_SOCKET_CLASS, "error no: other %d \r\n", ctchip_get_sock_errno(pContext->clientInfo.sock));
			code = CTIOT_OTHER_ERROR;
	}
	return code;
}

static uint16_t ctiotprv_check_error_of_connect()
{
	uint16_t result = CTIOT_OTHER_ERROR;
	ctiot_context_t *pContext = ctiot_get_context();
	int16_t dtlsCode;
#if CTIOT_CHIPSUPPORT_DTLS == 1
	if(pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS)
	{
		int16_t ret = pContext->lastDtlsErrCode;
		dtlsCode = ret & 0xFF80;
		if(dtlsCode != 0)
		{
			//集成ISSUE
			//DTLS层致命错误处理（需要后期根据具体需求适配）
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_DTLS_CLASS,"dtls code:%x",dtlsCode);
			if(dtlsCode == 0x6A00) //MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL
			{
				ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_DTLS_CLASS, "ctiotprv_check_error_of_connect,dtls nok error\r\n");
				result = CTIOT_DTLS_NOK_ERROR;
			}
			else //DTLS非致命错误
			{
				ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_DTLS_CLASS, "ctiotprv_check_error_of_connect,dtls oper error\r\n");
				result = CTIOT_DTLS_OPER_ERROR;
			}
			goto exit;
		}
		dtlsCode = ret & ~0xFF80;
		if(dtlsCode != 0)
		{
			result = ctiotprv_check_error_of_socket();
		}
	}
	else
	{
#endif
		result = ctiotprv_check_error_of_socket();
#if CTIOT_CHIPSUPPORT_DTLS == 1
	}
#endif
exit:
#if CTIOT_CHIPSUPPORT_DTLS == 1
	pContext->lastDtlsErrCode = 0;
#endif
	return result;
}



static uint16_t ctiotprv_check_ip_and_eps(uint8_t type) // type = 0 IP和EPS都检查  type=1 仅检查IP
{
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();

	//检查IP
	uint16_t ipStatus = ctiotprv_get_ip_status(pContext->serverIPType, 0);
	if(ipStatus == SDK_IP_STATUS_FALSE)
	{
		result = CTIOT_IP_NOK_ERROR;
	}
	else if(ipStatus == SDK_IP_STATUS_DISABLE)
	{
		result = CTIOT_IP_TYPE_ERROR;
	}
	else if(ipStatus == SDK_IP_STATUS_V6PREPARING)
	{
		result = CTIOT_IPV6_ONGOING_ERROR;
	}

	if(result != CTIOT_NB_SUCCESS)
	{
		return result;
	}

	//判断EPS
	if(type != 1)
	{
		uint8_t epsValue = ctchip_sync_cstate();
		if(epsValue != NETWORK_CONNECTED_HOME_NETWORK && epsValue != NETWORK_CONNECTED_ROAMING)
		{
			result = epsValue + CTIOT_NETWORK_ERROR_BASE;
		}
	}
	return result;
}

static void ctiotprv_set_message_send_option(ctlw_coap_packet_t *messageP, ctiot_send_mode_e sendMode)
{
	if (sendMode == SENDMODE_CON_REL || sendMode == SENDMODE_NON_RECV_REL)
	{
		messageP->sendOption = SEND_OPTION_RAI_DL_FOLLOWED;
	}
	else if (sendMode == SENDMODE_NON_REL)
	{
		messageP->sendOption = SEND_OPTION_RAI_NO_UL_DL_FOLLOWED;
	}
	else
	{
		messageP->sendOption = SEND_OPTION_NORMAL;
	}
}

static uint16_t ctiotprv_create_socket(ctiot_context_t *pContext,  int addressFamily)
{
	ctiot_log_debug(LOG_COMMON_MODULE, LOG_SOCKET_CLASS, "creat socket >>\r\n");
	ctiot_log_debug(LOG_COMMON_MODULE, LOG_SOCKET_CLASS, "ctiotprv_create_socket:begin network initialize...\r\n");
#ifndef PLATFORM_LINUX
	if (pContext->clientInfo.sock >= 0)
#else
	if (pContext->clientInfo.sock >= 0)
#endif
	{
		close(pContext->clientInfo.sock);
	}
	pContext->clientInfo.addressFamily = addressFamily;
	pContext->clientInfo.sock = -1;
#if CTIOT_CHIPSUPPORT_DTLS == 1
	if(pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS)
	{
		char* serverAddr=NULL;
		uint16_t serverPort = 0;

		if(addressFamily == AF_INET)
		{
			serverAddr = pContext->serverIPV4;
			serverPort = pContext->portV4;
			pContext->socketIPType = 1;//V4地址
			ctiot_log_debug(LOG_COMMON_MODULE, LOG_SOCKET_CLASS, "ctiotprv_create_socket:create v4 connection");
		}
		else
		{
			serverAddr = pContext->serverIPV6;
			serverPort = pContext->portV6;
			pContext->socketIPType = 2;//V6地址
			ctiot_log_debug(LOG_COMMON_MODULE, LOG_SOCKET_CLASS, "ctiotprv_create_socket:create v6 connection");
		}

		if(serverPort == 0)
		{
			if(pContext->connectionType == MODE_DTLS)
			{
				serverPort = CTIOT_DEFAULT_DTLS_PORT;
			}
			else if(pContext->connectionType == MODE_DTLS_PLUS)
			{
				serverPort = CTIOT_DEFAULT_DTLSPLUS_PORT;
			}
		}
		pContext->clientInfo.addressFamily = addressFamily;
		pContext->clientInfo.sock = ctlw_create_socket_dtls(CTIOT_DEFAULT_LOCAL_PORT, pContext->clientInfo.addressFamily,serverAddr,serverPort);
	}
	else
	{
#endif
		if(addressFamily == AF_INET)
		{
			pContext->socketIPType = 1;//V4地址
		}
		else
		{
			pContext->socketIPType = 2;//V6地址
		}
		pContext->clientInfo.addressFamily = addressFamily;
		pContext->clientInfo.sock = ctlw_create_socket(CTIOT_DEFAULT_LOCAL_PORT, pContext->clientInfo.addressFamily);
#if CTIOT_CHIPSUPPORT_DTLS == 1
	}
#endif
	if (pContext->clientInfo.sock < 0)
	{
		pContext->socketIPType = 0;
		ctiot_log_debug(LOG_COMMON_MODULE, LOG_SOCKET_CLASS, "pContext->clientInfo.sock<0\r\n");
		return CTIOT_OTHER_ERROR;
	}
#ifndef PLATFORM_XINYI
	/*LWIP不支持*/
	bool bBroadcast = true;
	setsockopt(pContext->clientInfo.sock, SOL_SOCKET, SO_BROADCAST, (const char *)&bBroadcast, sizeof(bool));
	ctiot_log_debug(LOG_COMMON_MODULE, LOG_SOCKET_CLASS, "setsockopt(pContext->clientInfo.sock,SOL_SOCKET, SO_BROADCAST, (const char*)&bBroadcast,sizeof(bool))\r\n");
#endif
	unsigned long ul = 1L;
	ioctl(pContext->clientInfo.sock, FIONBIO, &ul); //非阻塞模式
	ctiot_log_debug(LOG_COMMON_MODULE, LOG_SOCKET_CLASS, "ioctl(pContext->clientInfo.sock, FIONBIO, &ul)\r\n");
	
#ifndef PLATFORM_XINYI
	/*LWIP不支持*/
	struct timeval timeoutV = {CTIOT_SOCK_WRITE_TIMEOUT_S, CTIOT_SOCK_WRITE_TIMEOUT_MS};
	setsockopt(pContext->clientInfo.sock, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeoutV, sizeof(struct timeval));
	ctiot_log_debug(LOG_COMMON_MODULE, LOG_SOCKET_CLASS, "setsockopt(pContext->clientInfo.sock,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeoutV,sizeof(struct timeval))\r\n");
#endif	
	struct timeval recvTimeout = {CTIOT_SOCK_READ_TIMEOUT_S, CTIOT_SOCK_READ_TIMEOUT_MS};
	setsockopt(pContext->clientInfo.sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&recvTimeout, sizeof(struct timeval));
	ctiot_log_debug(LOG_COMMON_MODULE, LOG_SOCKET_CLASS, "setsockopt(pContext->clientInfo.sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&recvTimeout, sizeof(struct timeval))\r\n");

	ctiot_log_debug(LOG_COMMON_MODULE, LOG_SOCKET_CLASS, "create socket <<\r\n");
	pContext->addressFamily = addressFamily;
	return CTIOT_NB_SUCCESS;
}

static void ctiotprv_clear_update_msg(ctiot_context_t *pContext)
{
	if (pContext->updateMsg == NULL)
		return;
	if(pContext->updateMsg)
		ctlw_lwm2m_free(pContext->updateMsg);
	pContext->updateMsg = NULL;
}
static void ctiotprv_clear_uplist(ctiot_context_t *pContext)
{
	if (pContext->upMsgList == NULL)
		return;
	ctiot_up_msg_node *pTmp = (ctiot_up_msg_node *)ctiot_coap_queue_get(pContext->upMsgList);
	while (pTmp != NULL)
	{
		if(pTmp->uri!=NULL)
			ctlw_lwm2m_free(pTmp->uri);
		ctlw_lwm2m_free(pTmp);
		pTmp = (ctiot_up_msg_node *)ctiot_coap_queue_get(pContext->upMsgList);
	}
	thread_mutex_destroy(&pContext->upMsgList->mut);
	ctlw_lwm2m_free(pContext->upMsgList);
	pContext->upMsgList = NULL;

}
#ifdef CTLW_APP_FUNCTION
static void ctiotprv_clear_appmsglist(ctiot_context_t *pContext)
{
	if (pContext->appMsgList == NULL)
		return;
	ctiot_up_msg_node *pTmp = (ctiot_up_msg_node *)ctiot_coap_queue_get(pContext->appMsgList);
	while (pTmp != NULL)
	{
		if(pTmp->uri!=NULL)
			ctlw_lwm2m_free(pTmp->uri);
		ctlw_lwm2m_free(pTmp);
		pTmp = (ctiot_up_msg_node *)ctiot_coap_queue_get(pContext->appMsgList);
	}
	thread_mutex_destroy(&pContext->appMsgList->mut);
	ctlw_lwm2m_free(pContext->appMsgList);
	pContext->appMsgList = NULL;
}
#endif
static void ctiotprv_clear_downlist(ctiot_context_t *pContext)
{
	if (pContext->downMsgList == NULL)
		return;
	ctiot_down_msg_list *pTmp = (ctiot_down_msg_list *)ctiot_coap_queue_get(pContext->downMsgList);
	while (pTmp != NULL)
	{
		if (pTmp->payload != NULL)
		{
			ctlw_lwm2m_free(pTmp->payload);
		}
		ctlw_lwm2m_free(pTmp);
		pTmp = (ctiot_down_msg_list *)ctiot_coap_queue_get(pContext->downMsgList);
	}
	thread_mutex_destroy(&pContext->downMsgList->mut);
	ctlw_lwm2m_free(pContext->downMsgList);
	pContext->downMsgList = NULL;
}

static void ctiotprv_sendcon(lwm2m_transaction_t *transacP, void *message)
{
	ctlw_coap_packet_t *packet = (ctlw_coap_packet_t *)message;
	ctiot_context_t *pTmpContext = ctiot_get_context();
	if (packet == NULL)
	{
		ctiotprv_change_up_msg_status(pTmpContext, transacP->mID, CTIOT_TIME_OUT_ERROR);
		ctiot_publish_sdk_notification("/19/0/0",CTIOT_APP_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, CTIOT_TIME_OUT_ERROR, transacP->mID, 0, NULL); //发送超时
		return;
	}
	if (packet->type == COAP_TYPE_ACK)
	{

#ifdef PLATFORM_XINYI
#ifdef WITH_FOTA
		fota_notification_ack_handle(transacP->mID);
#endif
#endif
		ctiotprv_change_up_msg_status(pTmpContext, transacP->mID, QUEUE_SEND_SUCCESS);
		ctiot_publish_sdk_notification("/19/0/0",CTIOT_APP_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, QUEUE_SEND_SUCCESS, transacP->mID, 0, NULL); //成功
	}
	else if (packet->type == COAP_TYPE_RST)
	{
		ctiotprv_change_up_msg_status(pTmpContext, transacP->mID, CTIOT_RST_ERROR);
		ctiot_publish_sdk_notification("/19/0/0",CTIOT_APP_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, CTIOT_RST_ERROR, transacP->mID, 0, NULL); //平台RST
	}
}
#ifdef CTLW_APP_FUNCTION
static void ctiotprv_app_sendcon(lwm2m_transaction_t *transacP, void *message)
{
	ctlw_coap_packet_t *packet = (ctlw_coap_packet_t *)message;
	ctiot_context_t *pTmpContext = ctiot_get_context();

	ctiot_up_msg_node *node = (ctiot_up_msg_node *)ctiot_coap_queue_find(pTmpContext->appMsgList, transacP->mID);
	if(node == NULL)
	{
		return;
	}

	if (packet == NULL)
	{
		ctiot_change_msg_status(pTmpContext->appMsgList, transacP->mID, CTIOT_TIME_OUT_ERROR);
		ctiot_publish_sdk_notification((char *)node->uri,CTIOT_APP_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, CTIOT_TIME_OUT_ERROR, transacP->mID, 0, NULL); //发送超时
		return;
	}
	if (packet->type == COAP_TYPE_ACK)
	{
		ctiot_change_msg_status(pTmpContext->appMsgList, transacP->mID, QUEUE_SEND_SUCCESS);
		ctiot_publish_sdk_notification((char *)node->uri,CTIOT_APP_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, QUEUE_SEND_SUCCESS, transacP->mID, 0, NULL); //成功
	}
	else if (packet->type == COAP_TYPE_RST)
	{
		ctlw_coap_packet_t * p = (ctlw_coap_packet_t *)transacP->message;
		ctlw_observe_cancel_on_token(pTmpContext->lwm2mContext, p->token, (connection_t *)transacP->peerH);
		ctiot_change_msg_status(pTmpContext->appMsgList, transacP->mID, CTIOT_RST_ERROR);
		ctiot_publish_sdk_notification((char *)node->uri,CTIOT_APP_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, CTIOT_RST_ERROR, transacP->mID, 0, NULL); //平台RST
	}
}
#endif

static lwm2m_media_type_t ctiotprv_get_media_type(ctiot_send_format_e sendFormat)
{
	lwm2m_media_type_t result = LWM2M_CONTENT_TEXT;
	switch (sendFormat)
	{
	case DATA_FORMAT_OPAQUE:
	{
		result = LWM2M_CONTENT_OPAQUE;
		break;
	}
	case DATA_FORMAT_TLV:
	{
		result = LWM2M_CONTENT_TLV;
		break;
	}
	case DATA_FORMAT_JSON:
	{
		result = LWM2M_CONTENT_JSON;
		break;
	}
	case DATA_FORMAT_LINK:
	{
		result = LWM2M_CONTENT_LINK;
		break;
	}
	default:
	{
		break;
	}
	}
	return result;
}

static ctiot_send_format_e ctiotprv_get_send_format_type(lwm2m_media_type_t format)
{
	ctiot_send_format_e result = DATA_FORMAT_TEXT;
	switch (format)
	{
	case LWM2M_CONTENT_OPAQUE:
	{
		result = DATA_FORMAT_OPAQUE;
		break;
	}
	case LWM2M_CONTENT_TLV:
	case LWM2M_CONTENT_TLV_OLD:
	{
		result = DATA_FORMAT_TLV;
		break;
	}
	case LWM2M_CONTENT_JSON:
	case LWM2M_CONTENT_JSON_OLD:
	{
		result = DATA_FORMAT_JSON;
		break;
	}
	case LWM2M_CONTENT_LINK:
	{
		result = DATA_FORMAT_LINK;
		break;
	}
	default:
	{
		break;
	}
	}
	return result;
}

static bool ctiotprv_match_uri(lwm2m_uri_t uriC, lwm2m_uri_t targetUri)
{
	bool result = false;
	if (!LWM2M_URI_IS_SET_INSTANCE(&targetUri) || !LWM2M_URI_IS_SET_INSTANCE(&uriC))
	{
		return false;
	}
	if (LWM2M_URI_IS_SET_RESOURCE(&targetUri) && LWM2M_URI_IS_SET_RESOURCE(&uriC))
	{
		if (uriC.objectId == targetUri.objectId && uriC.instanceId == targetUri.instanceId && uriC.resourceId == targetUri.resourceId)
		{
			result = true;
		}
	}
	else if (!LWM2M_URI_IS_SET_RESOURCE(&targetUri) && !LWM2M_URI_IS_SET_RESOURCE(&uriC))
	{
		if (uriC.objectId == targetUri.objectId && uriC.instanceId == targetUri.instanceId)
		{
			result = true;
		}
	}
	return result;
}

uint16_t ctiotprv_system_para_init(ctiot_context_t *pContext)
{
	uint16_t result = CTIOT_OTHER_ERROR;
	int32_t ret = 0;
	/*设置系统socket状态为未建立*/
	pContext->clientInfo.sock = -1;

	/*初始化投票handler*/
	if(ctchip_init_ip_event_slp_handler() != CTIOT_NB_SUCCESS)
	{
		result = CTIOT_SYS_API_ERROR;
		goto exit;
	}
	if(ctchip_init_send_recv_slp_handler() != CTIOT_NB_SUCCESS)
	{
		result = CTIOT_SYS_API_ERROR;
		goto exit;
	}

	/*ip地址设为空*/
	memset(pContext->localIP, 0x0, INET6_ADDRSTRLEN);
	memset(pContext->tmpLocalIP, 0x0, INET6_ADDRSTRLEN);

	/*mutex 初始化,只做一次*/
	if (ctiot_observe_mutex_init() ||ctiot_session_status_mutex_init() ||ctiot_signal_mutex_init() ||ctiot_vote_slp_mutex_init() || ctiot_get_msg_id_mutex_init())
	{
		result = CTIOT_SYS_API_ERROR;
		goto exit;
	}

	ret = f2c_encode_params(pContext);
#ifdef PLATFORM_XINYI
	pContext->regMode = xy_ctlw_get_nvm_reg_mode();
#endif
	if (ret != NV_OK)
	{
		ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS, "f2c_encode_params:ret=%d", ret);
		if (ret == NV_ERR_CACHE_INVALID)
		{
			ctiot_publish_sdk_notification(NULL, CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LSTATUS, CTIOT_LSTATUS_LWENGINE_INI_FAIL, configchecksumfail, 0, NULL);
		}
		//读配置参数失败，恢复配置参数默认值
		ctiot_app_notify_cb_node* pList = pContext->appNotifyCbList;
		memset(pContext,0,sizeof(ctiot_context_t));
		pContext->appNotifyCbList = pList;
		pContext->lifetime = CTIOT_DEFAULT_LIFETIME;
		pContext->bootFlag = BOOT_NOT_LOAD;
		/*设置系统socket状态为未建立*/
		pContext->clientInfo.sock = -1;

		uint16_t writeParamResult = c2f_encode_params(pContext);
		uint16_t writeContextResult = c2f_encode_context(pContext, true);
		if(writeParamResult!=NV_OK ||writeContextResult!=NV_OK)
		{
			result = CTIOT_SYS_API_ERROR;
			goto exit;
		}
	}
	result = CTIOT_NB_SUCCESS;
exit:
	/*设置sendAndRecv线程状态为未启动*/
	//pContext->selectThreadStatus = THREAD_STATUS_NOT_LOADED;//no use select Thread
	pContext->sendAndRecvThreadStatus = THREAD_STATUS_NOT_LOADED;
	return result;
}

static bool ctiotprv_system_can_sleep(ctiot_context_t *pContext)
{
	bool result = true;
	if(pContext->clientWorkMode == UQ_WORK_MODE && pContext->connectionType == MODE_DTLS )
	{
		if(ctiot_signal_get_psm_status() == STATUS_NO_PSMING)
		{
			char ipAddr[INET6_ADDRSTRLEN+1]={0};
			int addressFamily = 0;
			if(pContext->socketIPType == 1)
			{
				addressFamily = AF_INET;
			}
			else if(pContext->socketIPType == 2)
			{
				addressFamily = AF_INET6;
			}
			if(ctlw_get_local_ip(ipAddr,addressFamily)== CTIOT_NB_SUCCESS)
			{
				result = false ;
			}
		}
	}
	return result;
}

#if CTIOT_CHIPSUPPORT_DTLS == 1
static connection_t * ctiotprv_connection_find(ctiot_context_t* pContext,connection_t *connList)
{
	connection_t *connP;
	connP = connList;
	while(connP!=NULL)
	{
		if(connP->ssl == pContext->ssl)
			return connP;
	}
	return NULL;
}
#endif

static uint16_t ctiotprv_session_recover(ctiot_context_t *pContext)
{
	uint16_t result = CTIOT_NB_SUCCESS;

	int32_t ret;
	ret = f2c_encode_context(pContext);
	if (ret == NV_OK)
	{
		if (pContext->lwm2mContext->serverList == NULL || ctiotprv_check_location(pContext) != true)
		{
			result = CTIOT_OTHER_ERROR;
			goto exit;
		}
	}
	else
	{
		result = CTIOT_OTHER_ERROR;
		goto exit;
	}
	pContext->lwm2mContext->state = STATE_READY;
#if SESSION_RECOVER_ERROR_TEST == 1//测试恢复会话失败
	result = CTIOT_OTHER_ERROR;
#endif

exit:
	if (result != CTIOT_NB_SUCCESS)
	{
		ctiotprv_free_objects_by_list(pContext);
		ctlw_lwm2m_clear_session(pContext->lwm2mContext);

		pContext->lwm2mContext = NULL;
	}

	return result;
}

static int ctiotprv_coap_yield()
{
	int result = CTIOT_NB_SUCCESS;
	int numBytes = 0;
#ifdef PLATFORM_XINYI
	uint8_t * buffer = g_ctlw_buffer;
	if(buffer == NULL)
		xy_assert(0);
#else
	static uint8_t buffer[CTIOT_MAX_PACKET_SIZE];
#endif
	memset(buffer, 0x00, CTIOT_MAX_PACKET_SIZE);
	struct sockaddr_storage addr;
	socklen_t addrLen;
	addrLen = sizeof(addr);
	ctiot_context_t* pContext = ctiot_get_context();
	
	if(pContext->connectionType == MODE_ENCRYPTED_PAYLOAD || pContext->connectionType == MODE_NO_DTLS)
	{	
		numBytes = ctlw_receivefrom(pContext->clientInfo.sock, buffer, CTIOT_MAX_PACKET_SIZE, (struct sockaddr *)&addr, &addrLen);
		if (numBytes <0)
		{
			if(ctchip_get_sock_errno(pContext->clientInfo.sock) != EAGAIN)
			{
				result = ctiotprv_check_error_of_socket();
				ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS, "ctiotprv_coap_yield error result : %d\r\n", result);
			}
			else
			{
				result = CTIOT_YIELD_READ_TIMEOUT;
			}
		}
		else if (numBytes > 0)
		{
			ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS, "ctiotprv_coap_yield read numBytes:%d\r\n",numBytes);

			//小于4个字节，不为coap报文
			if (numBytes < COAP_HEADER_LEN)
			{
				return CTIOT_NB_SUCCESS;
			}

			connection_t *connP;
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS, "recv: %d,addrLen: %d", numBytes,addrLen);
			connP = ctlw_connection_find(pContext->clientInfo.connList, (struct sockaddr *)&addr, addrLen);
			if (connP != NULL)
			{

				ctiot_vote_recv_send_busy();
				ctlw_lwm2m_handle_packet(pContext->lwm2mContext, buffer, numBytes, connP);

			}
			else
			{
				ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS, "ctlw_connection_find: fail");
			}
/*#ifdef PLATFORM_XINYI
			//通知select线程，下行数据包已经处理结束,可以继续开始监听
			xy_ctlw_create_ProcDlinkDateSem();
			xy_ctlw_release_ProcDlinkDateSem();
#endif*/ //No use select Thread
		}
	}
#if CTIOT_CHIPSUPPORT_DTLS == 1
	else if(pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS)
	{
		numBytes = ctlw_dtls_read(pContext->ssl, buffer, CTIOT_MAX_PACKET_SIZE, CTIOT_SOCK_READ_TIMEOUT_MS);
		if(numBytes < 0)
		{
			if(numBytes != MBEDTLS_ERR_SSL_TIMEOUT)
			{
				pContext->lastDtlsErrCode = -numBytes;
				result = ctiotprv_check_error_of_connect();
				if(result == CTIOT_DTLS_NOK_ERROR)//dtls致命错误处理
				{
					if(pContext->clientWorkMode == UQ_WORK_MODE)//UQ模式下，
					{
						ctiot_close_socket();
						ctiot_log_debug(LOG_COMMON_MODULE, LOG_DTLS_CLASS, "DTLS NOK");
						pContext->dtlsStatus = DTLS_NOK;
					}
					else//U模式下，退出会话
					{
						if (ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_OUTING))
						{
							ctiot_set_release_flag(RELEASE_MODE_DRX_DTLS_RECV_FAIL_L0, dtlsrecvdatafail);
						}
					}
					goto exit;
				}
			}
			else
			{
				result = CTIOT_YIELD_READ_TIMEOUT;
			}
		}
		else if(numBytes > 0)
		{
			if (numBytes < COAP_HEADER_LEN)//小于4个字节，不为coap报文
			{
				return 0;
			}
			connection_t *connP;
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS, "recv: %d", numBytes);
			connP = ctiotprv_connection_find(pContext, pContext->clientInfo.connList);
			if(connP != NULL)
			{
				ctiot_vote_recv_send_busy();

				ctlw_lwm2m_handle_packet(pContext->lwm2mContext, buffer, numBytes, connP);
			}
/*#ifdef PLATFORM_XINYI
			//通知select线程，下行数据包已经处理结束,可以继续开始监听
			xy_ctlw_create_ProcDlinkDateSem();
			xy_ctlw_release_ProcDlinkDateSem();
#endif*/ //no use select Thread
		}
	}
#endif
exit:
	return result;
}

static uint16_t ctiotprv_system_msg_step(ctiot_context_t *pContext)
{
	uint16_t msgResult = CTIOT_NB_SUCCESS;
	bool sendUpdate = false;
	ctiot_signal_mutex_lock();
	int addressFamily=0;
	if(pContext->IPEventFlag == 1)
	{
		pContext->IPEventFlag = 0;

		if (ctiot_trace_ip_by_bindmode() == CTIOT_ONIP_RECONNECT)//目前的设计实际不需要检查
		{
			uint16_t ipStatus = ctiotprv_get_ip_status(pContext->serverIPType, 0);

			if(ipStatus == SDK_IP_STATUS_DISABLE)
			{
				ctiot_publish_sdk_notification(NULL,CTIOT_SYSTEM_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LSTATUS, CTIOT_LSTATUS_IP_TYPE_NOT_MATCH, 0, 0, NULL);
				ctiot_chip_vote(ctchip_get_ip_event_slp_handler(),SYSTEM_STATUS_FREE);
				ctiot_signal_mutex_unlock();
				goto exit;
			}
			else if(ipStatus == SDK_IP_STATUS_TRUE_V4)
			{
				addressFamily = AF_INET;
				if(pContext->socketIPType == 2)
				{
					ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_IP_CLASS, "socket type changed,close socket, V6->v4");
					ctiot_close_socket();
				}
			}
			else if(ipStatus == SDK_IP_STATUS_TRUE_V6)
			{
				addressFamily = AF_INET6;
				if(pContext->socketIPType == 1)
				{
					ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_IP_CLASS, "socket type changed,close socket, V4->v6");
					ctiot_close_socket();
				}
			}
			else//ip地址空或preparing
			{
				ctiot_chip_vote(ctchip_get_ip_event_slp_handler(),SYSTEM_STATUS_FREE);

				ctiot_signal_mutex_unlock();
				goto exit;
			}
			memset(pContext->tmpLocalIP, 0, 40);
			if(ctlw_get_local_ip(pContext->tmpLocalIP, addressFamily) == CTIOT_NB_SUCCESS)
			{
				ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_IP_CLASS, "ctiot_ip_event ip address:%s,context ip:%s",pContext->tmpLocalIP,pContext->localIP);
				bool isIPChanged = (strcmp(pContext->localIP, pContext->tmpLocalIP) != 0);
				ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_IP_CLASS, "ctiot_ip_event:isIPChanged:%d\r\n", isIPChanged);

				if (isIPChanged)
				{
					memset(pContext->localIP, 0, 40);
					strcpy(pContext->localIP,pContext->tmpLocalIP);
					if(ctchip_write_session_ip((uint8_t *)pContext->localIP)!=CTIOT_NB_SUCCESS)//2.1写会话ip到RAM
					{
						msgResult = CTIOT_SYS_API_ERROR;
						ctiot_chip_vote(ctchip_get_ip_event_slp_handler(),SYSTEM_STATUS_FREE);
						ctiot_signal_mutex_unlock();
						goto exit;
					}
					
					if (pContext->connectionType == MODE_NO_DTLS || pContext->connectionType == MODE_ENCRYPTED_PAYLOAD)
					{
						sendUpdate = true;
					}
					else if(pContext->connectionType == MODE_DTLS_PLUS && pContext->clientWorkMode == U_WORK_MODE)
					{
						sendUpdate = true;
					}
					else if(pContext->connectionType == MODE_DTLS && pContext->clientWorkMode == U_WORK_MODE)
					{
						//SDK 2.1版本不支持U + DTLS模式
					}
					else if(pContext->clientWorkMode == UQ_WORK_MODE && (pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS))
					{
#if CTIOT_CHIPSUPPORT_DTLS == 1
						pContext->dtlsStatus = DTLS_NOK;
						ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_DTLS_CLASS, "dtls nok...");

#endif
					}
				}
			}
			else
			{
				msgResult = CTIOT_SYS_API_ERROR;
			}
		}

		ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_IP_CLASS, "ctiot_ip_event:result=%d\r\n", msgResult);

		if (msgResult == CTIOT_NB_SUCCESS && sendUpdate == true)
		{
			ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_IP_CLASS, "vote send recv handler:busy");
			ctiot_vote_recv_send_busy() ;
		}
		ctiot_chip_vote(ctchip_get_ip_event_slp_handler(),SYSTEM_STATUS_FREE);
		ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_VOTE_CLASS,"ctchip_get_ip_event_slp_handler():SYSTEM_STATUS_FREE");
	}
	ctiot_signal_mutex_unlock();

	if (sendUpdate == true)
	{
		msgResult = ctiot_network_restore(true, addressFamily);
		if (msgResult != CTIOT_NB_SUCCESS)//CTIOT_SOCK_CREATE_ERROR 或CTIOT_OTHER_ERROR
		{
			msgResult = CTIOT_SYS_API_ERROR;
			ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS, "ctiot_network_restore error: %d\r\n", msgResult);
			goto exit;
		}
	}
#if CTIOT_TIMER_AUTO_UPDATE == 1
	if (sendUpdate == true || ctiot_get_auto_update_flag() == 1)
#else
	if (sendUpdate == true)
#endif
	{
#ifndef PLATFORM_XINYI
		ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_IP_CLASS, "ctiot_ip_event:sending inner update...");
		uint16_t sendResult = ctiot_send_update_msg( SEND_OPTION_NORMAL, 0, 1);
#else
		ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_IP_CLASS, "sending outer update,auto_update_flag = %d",autoUpdateFlag);
		uint16_t sendResult = ctiot_send_update_msg( SEND_OPTION_NORMAL,ctlw_lwm2m_get_next_mid(pContext->lwm2mContext), 0);
#endif
		
		/*第一次发送失败*/
		if(sendResult != CTIOT_NB_SUCCESS )
		{

			if( sendResult == CTIOT_SYS_API_ERROR)
			{
				msgResult = sendResult;
			}
			else if(sendResult == CTIOT_DTLS_NOK_ERROR || sendResult == CTIOT_DTLS_OPER_ERROR)
			{
				if (ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_OUTING))
				{
					ctiot_set_release_flag(RELEASE_MODE_DRX_DTLS_IP_CHANGE_L0, inupdatedtlssendfail);
				}
			}
			
			/*
			else if(sendResult == CTIOT_IN_UPDATE_OTHER_FAIL)
			{
				if (ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_OUTING))
				{
					ctiot_set_release_flag(RELEASE_MODE_INNER_UPDATE_OTHER_FAIL, inupdateotherfail);
				}
			}*/
		}
		#if CTIOT_TIMER_AUTO_UPDATE == 1
		if(ctiot_get_auto_update_flag() == 1)
		{
			ctiot_set_auto_update_flag(0);
		}
		#endif
	}
exit:
	return msgResult;
}

ctiot_session_status_e ctiotprv_get_session_status(ctiot_context_t* pContext)
{
	ctiot_session_status_e sessionStatus = pContext->sessionStatus;
	if(sessionStatus == UE_LOGINED)
	{
		lwm2m_uri_t uriP[1]={0};
		if (ctlw_lwm2m_stringToUri("/19/0/0", strlen("/19/0/0"), uriP) == 0)
		{
			return sessionStatus;
		}

		if(ctlw_observe_findByUri(pContext->lwm2mContext,uriP)!=NULL)
		{
			sessionStatus = UE_LOGINED_OBSERVED;
		}
	}
	return sessionStatus;
}

static uint16_t ctiotprv_process_session_restore(ctiot_context_t *pContext)
{
	uint16_t result = CTIOT_NB_SUCCESS;

	pContext->sessionStatus = UE_LOGIN_INITIALIZE;
#ifndef CTLW_ANYTIME_SOCKET
	pContext->clientInfo.sock = -1;
#endif

#if CTIOT_CHIPSUPPORT_DTLS == 1
	//1 检查UQ状态下 UQ+DTLS的处理
	if(pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS)
	{
		if(pContext->clientWorkMode == UQ_WORK_MODE)
		{
			if(pContext->ssl == NULL)
			{
				if(ctlw_dtls_ssl_create(pContext) != 0)
				{
					result = CTIOT_SYS_API_ERROR;
					goto exit;
				}
				ctlw_dtls_update_socket_fd(pContext);
			}
			pContext->dtlsStatus = DTLS_NOK;
		}
		//u模式场景在预初始化中已处理
	}
#endif
	//2.1 初始化适配层
	result = ctiot_signal_init();//向芯片订阅ip状态变化通知
	if (result != CTIOT_NB_SUCCESS)
	{
		result = CTIOT_SYS_API_ERROR;
		goto exit;
	}
	//2.2 初始化serverIPType
	ctiotprv_set_server_ip_type(pContext);

	//3 初始化lwm2m上下文并恢复会话数据
	result = ctiotprv_lwm2m_init(pContext);
	if(result == CTIOT_NB_SUCCESS)
	{
		result = ctiotprv_session_recover(pContext);
	}
	if (result != CTIOT_NB_SUCCESS)
	{
		if(result==CTIOT_OTHER_ERROR)
		{
			if (ctiot_update_session_status( NULL,UE_LOGIN_INITIALIZE, UE_LOGIN_OUTING))
			{
				ctiot_log_info(LOG_INIT_MODULE,LOG_OTHER_CLASS,"ctiotprv_process_session_restore:ctiot_session_init failed...\r\n");
				ctiot_set_release_flag(RELEASE_MODE_RECOVER_SESSION_FAIL_L0, initdatafail);
			}
		}
		//CTIOT_SYS_API_ERROR在外层收发线程CTIOT_STATE_RECOVER_REQUIRED状态时处理
		goto exit;
	}

	//3 订阅适配层ip和psm通知
	if (ctiot_trace_ip_by_bindmode()==CTIOT_ONIP_RECONNECT)
	{
		uint8_t ipAddr[INET6_ADDRSTRLEN] = {0};
		memset(ipAddr,0,INET6_ADDRSTRLEN);
		uint16_t sessionIPResult = ctchip_get_session_ip(startReason, pContext->addressFamily,ipAddr);
		memset(pContext->localIP, 0, INET6_ADDRSTRLEN);
		if(sessionIPResult==CTIOT_NB_SUCCESS)
		{
			ctiot_log_debug(LOG_INIT_MODULE, LOG_OTHER_CLASS, "get session ip:%s",ipAddr);
			strcpy(pContext->localIP, (char *)ipAddr);
		}
		else if(sessionIPResult == CTIOT_SYS_API_ERROR)
		{
			result = CTIOT_SYS_API_ERROR;
			goto exit;
		}

		ctiot_signal_subscribe_ip_event(ctiot_ip_event_async,(server_ip_type_e)pContext->serverIPType);//订阅ip地址建立通知
	}
	if(pContext->clientWorkMode == UQ_WORK_MODE && pContext->connectionType == MODE_DTLS)
	{
		result = ctiot_signal_subscribe_psm_status(NULL);//订阅psm通知
		if(result != CTIOT_NB_SUCCESS)
		{
			result = CTIOT_SYS_API_ERROR;
			goto exit;
		}
	}

	//4. 变化登录状态
	if (result == CTIOT_NB_SUCCESS)
	{
		//目前不考虑会话状态修改失败
		ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGINED);
		ctiot_publish_sdk_notification(NULL,CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SESSION_RESTORE, 0, 0, 0, NULL);
#if CTIOT_WITH_PAYLOAD_ENCRYPT == 1
		if(pContext->connectionType == MODE_ENCRYPTED_PAYLOAD && pContext->payloadEncryptAlgorithm == PAYLOAD_ENCRYPT_SM2 && pContext->payloadEncryptInitialized == 0)
		{
			//初始化payloadEncrypt失败
			if(Sec_dev_PinCfg(pContext->payloadEncryptPin,strlen(pContext->payloadEncryptPin))!=0)
			{
				result = CTIOT_SYS_API_ERROR;
				goto exit;
			}
			if(Sec_dev_Init(1)!=0)
			{
				result = CTIOT_SYS_API_ERROR;
				goto exit;
			}
			pContext->payloadEncryptInitialized = 1;
		}
#endif
	}

exit:

	return result;
}

static void ctiotprv_down_msg_step(ctiot_context_t *pContext)
{
#ifdef PLATFORM_XINYI
	if (pContext->recvDataMode == RECV_DATA_MODE_1 && pContext->recvDataMaxCacheTime != 0)
#else
	if ((pContext->recvDataMode == RECV_DATA_MODE_1 || pContext->recvDataMode == RECV_DATA_MODE_2)&& pContext->recvDataMaxCacheTime != 0)
#endif
	{
		if (pContext->downMsgList == NULL)
		{
			return;
		}
		time_t now = ctlw_lwm2m_gettime();

		ctiot_down_msg_list *nodePtr = (ctiot_down_msg_list *)pContext->downMsgList->head;
		ctiot_down_msg_list *next = NULL;
		while (nodePtr != NULL)
		{
			next = (ctiot_down_msg_list *)nodePtr->next;
			if ((nodePtr->recvtime + pContext->recvDataMaxCacheTime) < now)
			{
				ctiot_coap_queue_remove(pContext->downMsgList, nodePtr->msgId, &nodePtr);
				if (nodePtr != NULL)
				{
					ctlw_lwm2m_free(nodePtr->payload);
					ctlw_lwm2m_free(nodePtr);
				}
			}
			else
			{
				break;
			}
			nodePtr = next;
		}
	}
}

static void ctiotprv_set_uri_option(ctlw_coap_packet_t *messageP, lwm2m_uri_t *uriP)
{
	int result;
	char stringID1[LWM2M_STRING_ID_MAX_LEN];
	result = ctlw_utils_intToText(uriP->objectId, (uint8_t *)stringID1, LWM2M_STRING_ID_MAX_LEN);
	stringID1[result] = 0;
	ctlw_coap_set_header_uri_path_segment(messageP, stringID1);
	if (LWM2M_URI_IS_SET_INSTANCE(uriP))
	{
		result = ctlw_utils_intToText(uriP->instanceId, (uint8_t *)stringID1, LWM2M_STRING_ID_MAX_LEN);
		stringID1[result] = 0;
		ctlw_coap_set_header_uri_path_segment(messageP, stringID1);
	}
	if (LWM2M_URI_IS_SET_RESOURCE(uriP))
	{
		result = ctlw_utils_intToText(uriP->resourceId, (uint8_t *)stringID1, LWM2M_STRING_ID_MAX_LEN);
		stringID1[result] = 0;
		ctlw_coap_set_header_uri_path_segment(messageP, stringID1);
	}
}

static void ctiotprv_set_message_payload(ctlw_coap_packet_t *messageP, lwm2m_uri_t *uriP, ctiot_send_format_e sendFormat,
									ctiot_data_list *dataP, lwm2m_media_type_t *mediaType)
{
	int size = 1;
	lwm2m_data_t dataB[1];
	uint8_t *buffer = NULL;
	memset(dataB,0,sizeof(lwm2m_data_t));
	switch (sendFormat)
	{
	case DATA_FORMAT_TLV:
	{
		uint8_t *newBuffer = ctlw_lwm2m_malloc((size_t)dataP->u.asBuffer.length);
		memset(newBuffer, 0, (size_t)dataP->u.asBuffer.length);
		memcpy(newBuffer, dataP->u.asBuffer.buffer, (size_t)dataP->u.asBuffer.length);
		ctlw_coap_set_payload(messageP, newBuffer, (size_t)dataP->u.asBuffer.length);
		break;
	}
	case DATA_FORMAT_OPAQUE:
	{
		ctlw_lwm2m_data_encode_opaque(dataP->u.asBuffer.buffer, dataP->u.asBuffer.length, dataB);
		int len = ctlw_lwm2m_data_serialize(uriP, size, dataB, mediaType, &buffer);
		if (len > 0)
		{
			ctlw_coap_set_payload(messageP, buffer, (size_t)len);
		}
		if (dataB->value.asBuffer.buffer != NULL)
		{
			ctlw_lwm2m_free(dataB->value.asBuffer.buffer);
		}
		break;
	}
	case DATA_FORMAT_JSON:
	{
		uint8_t *newBuffer = ctlw_lwm2m_malloc((size_t)dataP->u.asBuffer.length);
		memset(newBuffer, 0, (size_t)dataP->u.asBuffer.length);
		memcpy(newBuffer, dataP->u.asBuffer.buffer, (size_t)dataP->u.asBuffer.length);
		ctlw_coap_set_payload(messageP, newBuffer, (size_t)dataP->u.asBuffer.length);
		break;
	}
	case DATA_FORMAT_TEXT:
	{
		uint8_t *newBuffer = ctlw_lwm2m_malloc((size_t)dataP->u.asBuffer.length);
		memset(newBuffer, 0, (size_t)dataP->u.asBuffer.length);
		memcpy(newBuffer, dataP->u.asBuffer.buffer, (size_t)dataP->u.asBuffer.length);
		ctlw_coap_set_payload(messageP, newBuffer, (size_t)dataP->u.asBuffer.length);
		break;
	}
	case DATA_FORMAT_LINK:
	{
		uint8_t *newBuffer = ctlw_lwm2m_malloc((size_t)dataP->u.asBuffer.length);
		memset(newBuffer, 0, (size_t)dataP->u.asBuffer.length);
		memcpy(newBuffer, dataP->u.asBuffer.buffer, (size_t)dataP->u.asBuffer.length);
		ctlw_coap_set_payload(messageP, newBuffer, (size_t)dataP->u.asBuffer.length);
		break;
	}
	default:
		break;
	}
}

#if CTIOT_CHIPSUPPORT_DTLS == 1
static uint16_t ctiotprv_send_dtls_hs()
{
	ctiot_log_debug(LOG_COMMON_MODULE, LOG_DTLS_CLASS, "ctiotprv_send_dtls_hs >>");
	ctiot_context_t *pContext = ctiot_get_context();
	uint16_t ret = CTIOT_NB_SUCCESS;


	mbedtls_ssl_session_reset(pContext->ssl);
	int dtlsRet = ctlw_dtls_shakehand(pContext);
	if(dtlsRet != 0)
	{
		ret = CTIOT_DTLS_HS_ERROR;
	}

	ctiot_log_debug(LOG_COMMON_MODULE, LOG_DTLS_CLASS, "ctiotprv_send_dtls_hs <<");
	return ret;
}
#endif

static void ctiot_clear_up_data_list(uint16_t reason)
{
	ctiot_context_t *pContext = ctiot_get_context();
	uint16_t msgStatus = QUEUE_SEND_DATA_CACHEING;

	ctiot_up_msg_node *pTmp = NULL;
	while(true)
	{
		while (true)
		{
			pTmp = (ctiot_up_msg_node *)ctiot_coap_queue_get_msg(pContext->upMsgList, msgStatus);
			if(pTmp == NULL)
			{
				ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiotprv_clear_msg_list:upMsgList msgstatus %u is null\r\n", msgStatus);
				break;
			}

			ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiotprv_clear_msg_list:upMsgList,msgId=%d\r\n",pTmp->msgId);
			ctiotprv_change_up_msg_status(pContext, pTmp->msgId, reason);
			ctiot_publish_sdk_notification((char *)pTmp->uri, CTIOT_APP_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, reason, pTmp->msgId, 0, NULL);

			ctlw_coap_packet_t *message = (ctlw_coap_packet_t *)pTmp->node;
			if(message != NULL)
			{
				if(message->payload != NULL)
				{
					ctlw_lwm2m_free(message->payload);
				}
				ctlw_coap_free_header(message);
				ctlw_lwm2m_free(message);
			}
			pTmp->node = NULL;
		}
#ifdef CTLW_APP_FUNCTION
		while (true)
		{
			pTmp = (ctiot_up_msg_node *)ctiot_coap_queue_get_msg(pContext->appMsgList, msgStatus);
			if(pTmp == NULL)
			{
				ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiotprv_clear_msg_list:appMsgList msgstatus %u is null\r\n", msgStatus);
				break;
			}

			ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiotprv_clear_msg_list:appMsgList,msgId=%d\r\n",pTmp->msgId);
			ctiot_change_msg_status(pContext->appMsgList, pTmp->msgId, reason);
			ctiot_publish_sdk_notification((char *)pTmp->uri, CTIOT_APP_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, reason, pTmp->msgId, 0, NULL);

			ctlw_coap_packet_t *message = (ctlw_coap_packet_t *)pTmp->node;
			if(message != NULL)
			{
				if(message->payload != NULL)
				{
					ctlw_lwm2m_free(message->payload);
				}
				ctlw_coap_free_header(message);
				ctlw_lwm2m_free(message);
			}
			pTmp->node = NULL;
		}
#endif
		if(msgStatus == QUEUE_SEND_DATA_CACHEING)
		{
			msgStatus = QUEUE_SEND_DATA_SENDOUT;
		}
		else
		{
			break;
		}
	}

}

static void ctiotprv_clear_msg_list(uint16_t reason)
{
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS, "ctiotprv_clear_msg_list >>\r\n");
	ctiot_context_t *pContext = ctiot_get_context();

	ctiot_update_node *ptr = pContext->updateMsg;
	if (ptr != NULL && (ptr->msgStatus == QUEUE_SEND_DATA_CACHEING || ptr->msgStatus == QUEUE_SEND_DATA_SENDOUT))
	{
		ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS, "ctiotprv_clear_msg_list:updateMsg\r\n");
		ctiotprv_change_up_msg_status(pContext, ptr->msgId, reason);
		ctiot_publish_sdk_notification(NULL,CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_UPDATE, reason, ptr->msgId, 0, NULL);
	}

	ctiot_clear_up_data_list(reason);

	if (pContext->lwm2mContext != NULL)
	{
		ctlw_deleteTransactionList(pContext->lwm2mContext);
	}
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS, "ctiotprv_clear_msg_list <<\r\n");
}

static uint16_t ctiotprv_check_socket()
{
	ctiot_context_t *pContext = ctiot_get_context();
	uint16_t ret = CTIOT_NB_SUCCESS;

	uint16_t ipStatus = ctiotprv_get_ip_status(pContext->serverIPType, 0);
	if(ipStatus == SDK_IP_STATUS_FALSE)
	{
		ret = CTIOT_IP_NOK_ERROR;
	}
	else if(ipStatus == SDK_IP_STATUS_DISABLE)
	{
		ret = CTIOT_IP_TYPE_ERROR;
	}
	else if(ipStatus == SDK_IP_STATUS_V6PREPARING)
	{
		ret = CTIOT_IPV6_ONGOING_ERROR;
	}

	if(ret != CTIOT_NB_SUCCESS)
	{
		goto exit;
	}

	if(pContext->clientInfo.sock >= 0)
	{
		if((pContext->socketIPType == 1 && ipStatus == SDK_IP_STATUS_TRUE_V6) || (pContext->socketIPType == 2 && ipStatus == SDK_IP_STATUS_TRUE_V4))
		{
			ctiot_close_socket();
		}
	}

	if (pContext->clientInfo.sock < 0)
	{
		int addressFamily = ctiotprv_switch_address_family_with_ip_status(ipStatus);
		ret = ctiot_network_restore(true, addressFamily);
		if (ret == CTIOT_SOCK_CREATE_FAIL)
		{
			ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_SOCKET_CLASS,"socket create error\r\n");
			ret = CTIOT_SYS_API_ERROR;
			goto exit;
		}
#if CTIOT_CHIPSUPPORT_DTLS == 1
		//if(pContext->connectionType == MODE_DTLS_PLUS && pContext->clientWorkMode == U_WORK_MODE)
		if(pContext->connectionType == MODE_DTLS_PLUS || pContext->connectionType == MODE_DTLS)
		{
			ctlw_dtls_update_socket_fd(pContext);
		}
#endif
	}
exit:
	return ret;
}

#if CTIOT_CHIPSUPPORT_DTLS == 1
static uint16_t ctiotprv_check_dtls()
{
	ctiot_context_t *pContext = ctiot_get_context();
	uint16_t result = CTIOT_NB_SUCCESS;

	if(pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS)
	{
		if(pContext->dtlsStatus == DTLS_NOK )
		{
			pContext->dtlsStatus = DTLS_SWITCHING;
			ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_DTLS_CLASS,"DTLS_SWITCHING;...\r\n");
			result = ctiotprv_send_dtls_hs();
			if(pContext->atDtlsHsFlag == 1)
			{
				pContext->atDtlsHsFlag = 0;
				ctiot_publish_sdk_notification(NULL,CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_DTLS_HS, result, 0, 0, NULL);
				ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_DTLS_CLASS,"DTLS_OK;...\r\n");
			}
			if(result == CTIOT_DTLS_HS_ERROR)
			{
				//U_WORK_MODE不会有dtls_nok状态，此处只处理UQ_WORK_MODE
				ctiotprv_clear_msg_list(result);
				ctiot_close_socket();
				pContext->dtlsStatus = DTLS_NOK;
				ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_DTLS_CLASS,"DTLS_NOK;...\r\n");
				return result;
			}
			else
			{
				pContext->dtlsStatus = DTLS_OK;
			}
		}
	}
	return result;
}
#endif

static void prv_mcu_notify_callback(char* uri, ctiot_notify_type notifyType, ctiot_notify_subType subType, uint16_t value, uint32_t data1, uint32_t data2, void *reservedData)
{
	ctiot_sdk_notification(notifyType, subType, value, data1, data2, reservedData);
}

//type=0为精确匹配当uri1 和 uri2 一致返回true，否则返回false; type = 1时为包含匹配，当uri1包含uri2时返回true，否则false。例如：uri1="/19/0" , uri2="/19/0/1" 反true，交换两个uri反false
bool ctiot_compare_uri(lwm2m_uri_t *uri1, lwm2m_uri_t *uri2, uint8_t type)
{
	if(type == 0)
	{
		if(uri1->flag != uri2->flag)
			return false;
	}

	if(uri1->objectId != uri2->objectId
	||(LWM2M_URI_IS_SET_INSTANCE(uri1) && uri1->instanceId != uri2->instanceId)
	||(LWM2M_URI_IS_SET_RESOURCE(uri1) && uri1->resourceId != uri2->resourceId))
	{
		return false;
	}
	return true;
}

static void ctiotprv_add_notify_cb(uint8_t type, uint8_t * uri, uint8_t uriLen,ctiot_app_notify_cb callback)
{
	ctiot_context_t* pContext = ctiot_get_context();

	ctiot_app_notify_cb_node * newNode = (ctiot_app_notify_cb_node *)ctlw_lwm2m_malloc(sizeof(ctiot_app_notify_cb_node));
	if(newNode == NULL)
	{
		return;
	}
	memset(newNode, 0, sizeof(ctiot_app_notify_cb_node));
	if(type == CTIOT_APP_NOTIFICATION && ctlw_lwm2m_stringToUri((char *)uri, uriLen, &(newNode->uri)) == 0)
	{
		return;
	}
	newNode->type = type;
	newNode->cb = callback;

	if(pContext->appNotifyCbList == NULL)
	{
		pContext->appNotifyCbList = newNode;
	}
	else
	{
		if(newNode->uri.flag > pContext->appNotifyCbList->uri.flag)
		{
			newNode->next = pContext->appNotifyCbList;
			pContext->appNotifyCbList = newNode;
			return;
		}
		if(newNode->uri.flag == pContext->appNotifyCbList->uri.flag)
		{
			if(pContext->appNotifyCbList->type == CTIOT_APP_NOTIFICATION && ctiot_compare_uri(&(newNode->uri), &(pContext->appNotifyCbList->uri), 0))
			{
				pContext->appNotifyCbList->cb = callback;
				ctlw_lwm2m_free(newNode);
				return;
			}
		}
		ctiot_app_notify_cb_node *ptr = pContext->appNotifyCbList;
		while(ptr->next)
		{
			if( newNode->uri.flag == ptr->next->uri.flag)
			{
				if(ptr->next->type == CTIOT_APP_NOTIFICATION && ctiot_compare_uri(&(newNode->uri), &(ptr->next->uri), 0))
				{
					ptr->next->cb = callback;
					ctlw_lwm2m_free(newNode);
					return;
				}
				else
				{
					ptr = ptr->next;
					continue;
				}
			}
			else if(newNode->uri.flag < ptr->next->uri.flag)
			{
				ptr = ptr->next;
			}
			else
			{
				newNode->next = ptr->next;
				ptr->next = newNode;
				return;
			}
		}
		ptr->next = newNode;
	}
}

 int ctiot_uri_to_string(lwm2m_uri_t * uriP, uint8_t * buffer, size_t bufferLen, uri_depth_t * depthP)
{
    size_t head;
    int res;

    buffer[0] = '/';

    if (uriP == NULL)
    {
        if (depthP) *depthP = URI_DEPTH_OBJECT;
        return 1;
    }

    head = 1;

    res = ctlw_utils_intToText(uriP->objectId, buffer + head, bufferLen - head);
    if (res <= 0) return -1;
    head += res;
    if (head >= bufferLen - 1) return -1;
    if (depthP) *depthP = URI_DEPTH_OBJECT_INSTANCE;

    if (LWM2M_URI_IS_SET_INSTANCE(uriP))
    {
        buffer[head] = '/';
        head++;
        res = ctlw_utils_intToText(uriP->instanceId, buffer + head, bufferLen - head);
        if (res <= 0) return -1;
        head += res;
        if (head >= bufferLen - 1) return -1;
        if (depthP) *depthP = URI_DEPTH_RESOURCE;
        if (LWM2M_URI_IS_SET_RESOURCE(uriP))
        {
            buffer[head] = '/';
            head++;
            res = ctlw_utils_intToText(uriP->resourceId, buffer + head, bufferLen - head);
            if (res <= 0) return -1;
            head += res;
            if (head >= bufferLen - 1) return -1;
            if (depthP) *depthP = URI_DEPTH_RESOURCE_INSTANCE;
        }
    }

    return head;
}

void ctiot_add_system_notify_cb(ctiot_app_notify_cb callback)
{
	ctiotprv_add_notify_cb( CTIOT_SYSTEM_NOTIFICATION, NULL, 0, callback);
}

void ctiot_add_app_notify_cb(uint8_t * uri, uint8_t uriLen, ctiot_app_notify_cb  callback)
{
	lwm2m_uri_t uriP[1] = {0};
	if(ctlw_lwm2m_stringToUri((char *)uri, uriLen, uriP) != 0)
	{
		if(!(uriP->objectId == 19 && LWM2M_URI_IS_SET_INSTANCE(uriP) && (uriP->instanceId == 1 ||uriP->instanceId== 0)))
		{
			ctiotprv_add_notify_cb( CTIOT_APP_NOTIFICATION, uri, uriLen, callback);
		}
	}
}

void ctiot_print_notify_list()
{
	ctiot_context_t* pContext = ctiot_get_context();
	ctiot_app_notify_cb_node* ptr = pContext->appNotifyCbList;
	while(ptr)
	{
		char ip[20] = {0};
		ctiot_uri_to_string(&(ptr->uri), (uint8_t *)ip, 20, NULL);
		ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"uri: %s",ip);
		ptr = ptr->next;
	}
}

void ctiot_publish_sdk_notification(char* uri, uint8_t notifyCbType, uint8_t notifyType, uint8_t subType, uint16_t value, uint32_t data1, uint32_t data2, void *reservedData)
{
	ctiot_context_t* pContext = ctiot_get_context();

	uint8_t type;
	lwm2m_uri_t uriP[1] = {0};
	char* uriCopy = NULL;

	type = notifyCbType;

	if(type == CTIOT_APP_NOTIFICATION )
	{
		if(uri == NULL || ctlw_lwm2m_stringToUri(uri, strlen(uri), uriP) == 0)
		{
			return;
		}
		uriCopy = ctlw_lwm2m_strdup(uri);
		if(uriCopy == NULL)
		{
			return;
		}
	}
	ctiot_app_notify_cb_node* ptr = pContext->appNotifyCbList;

	while(ptr)
	{
		if(type == CTIOT_APP_NOTIFICATION )
		{
			if(ctiot_compare_uri(&ptr->uri, uriP, 1))
			{
				ptr->cb(uriCopy, (ctiot_notify_type)notifyType, (ctiot_notify_subType)subType, value, data1, data2, reservedData);
				ctlw_lwm2m_free(uriCopy);
				uriCopy = NULL;
				break;
			}

		}
		else
		{
			if(ptr->type == CTIOT_SYSTEM_NOTIFICATION)
			{
				ptr->cb(NULL, (ctiot_notify_type)notifyType, (ctiot_notify_subType)subType, value, data1, data2, reservedData);
			}
		}
		ptr = ptr->next;
	}
	if(uriCopy != NULL)
	{
		ctlw_lwm2m_free(uriCopy);
	}
}

void ctiot_free_app_notify_list(void)
{
	ctiot_context_t* pContext = ctiot_get_context();
	if(pContext && pContext->appNotifyCbList)
	{
		ctiot_app_notify_cb_node *ptr = pContext->appNotifyCbList;
		pContext->appNotifyCbList = NULL;
		while(ptr)
		{
			ctiot_app_notify_cb_node *p = ptr->next;
			ctlw_lwm2m_free(ptr);
			ptr = p;
		}
	}
}
#ifdef CTLW_APP_FUNCTION
extern void ctlw_fota_notify(char* uri, ctiot_notify_type notifyType, ctiot_notify_subType subType, uint16_t value, uint32_t data1, uint32_t data2, void *reservedData);
void ctiot_init_app_notify_cb(void)
{
	ctiot_add_app_notify_cb("/5", strlen("/5"), ctlw_fota_notify);
	ctiot_add_system_notify_cb(ctlw_fota_notify);
#ifdef CTLW_APP_DEMO
	   /*---------------*/
#endif
}
#endif
void ctiot_init_sdk_notify_cb(void)
{
	ctiot_add_system_notify_cb(prv_mcu_notify_callback);
	ctiotprv_add_notify_cb( CTIOT_APP_NOTIFICATION, "/19/0/0", strlen("/19/0/0"), prv_mcu_notify_callback);
	ctiotprv_add_notify_cb( CTIOT_APP_NOTIFICATION, "/19/1/0", strlen("/19/1/0"), prv_mcu_notify_callback);
}

int ctlw_resource_value_changed(char *uri)
{
	int result = 0;
	lwm2m_uri_t uriT;
	size_t uri_len;
	ctiot_context_t *pContext;

	pContext = ctiot_get_context();

	uri_len = strlen(uri);
	result = ctlw_lwm2m_stringToUri(uri, uri_len, &uriT);

	ctlw_lwm2m_resource_value_changed(pContext->lwm2mContext, &uriT);
	return result;
}

ctiot_chip_module_info *ctiot_get_chip_instance(void)
{
	static ctiot_chip_module_info instance = {0};
	return &instance;
}

ctiot_context_t *ctiot_get_context(void)
{
#ifdef PLATFORM_XINYI
	//内存优化,动态申请替代静态分配
	return xy_ctlw_ctiot_get_context();
#else
	static ctiot_context_t contextInstance = {0};
	contextInstance.chipInfo = ctiot_get_chip_instance();
	return &contextInstance;
#endif
}

#ifdef PLATFORM_XINYI
uint8_t ctiot_update_sdataflash_needed(bool isImmediately)
#else
uint8_t ctiot_update_sdataflash_needed()
#endif
{
    ctiot_context_t* pContext = ctiot_get_context();

    //isFlush=true表示调用持久化接口直接写，isFlush=false表示托管写（只修改数据变化标记）
    bool isFlush = false;

#if CTIOT_CHIPSUPPORT_NBSLEEPING == 0//芯片不支持休眠，默认直接写
	isFlush = true;
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"immediate write because CTIOT_CHIPSUPPORT_NBSLEEPING == 0...\r\n");
#else

#ifndef PLATFORM_XINYI
	if(pContext->onKeepSession==SYSTEM_ON_REBOOT_KEEP_SESSION)
	{
		isFlush = true;
		ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"immediate write because mod_1=1 and value is changed...\r\n");
	}
#else
	isFlush = true;
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"immediate write because xinyiplatform,mod_1=1 and value is changed...\r\n");
#endif

#endif

	if(isFlush)
	{
#ifdef PLATFORM_XINYI
		if(c2f_encode_context(pContext,isImmediately)==NV_OK)
#else
		if(c2f_encode_context(pContext,true)==NV_OK)
#endif
		{
		    sDataFlashNeeded = CTIOT_DATA_UNCHANGED;
		}
		else
		{
			ctiot_exit_on_error(pContext);
		}
	}
	else
	{
	    sDataFlashNeeded = CTIOT_DATA_CHANGED;

	}

	return sDataFlashNeeded;
}


#if 0
void ctiot_engine_entry(uint8_t autoLoginFlag)
{
	ctiot_log_debug(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"ctiot_engine_entry\r\n");
	ctiot_context_t *pContext = ctiot_get_context();
	ctiot_boot_flag_e cacheBootFlag = BOOT_NOT_LOAD;
	int32_t ret = 0;
	//1.初始化：注册App通知回调函数 && 注册SDK通知回调函数
#ifdef CTLW_APP_FUNCTION
	ctiot_init_app_notify_cb();
#endif
	ctiot_init_sdk_notify_cb();
#if CTIOT_TIMER_AUTO_UPDATE == 1
	ctchip_register_timer_callback(timer_callback);
#endif
	//2.初始化预初始化投票句柄并投忙票
	if(ctchip_init_initialization_slp_handler() != CTIOT_NB_SUCCESS)
	{
		ctiot_publish_sdk_notification(NULL, CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LSTATUS, CTIOT_LSTATUS_QUITENGINE, 0, 0, NULL);
		ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_INITIALIZE_FAILED);
		goto exit;
	}
	ctiot_log_debug(LOG_PREINIT_MODULE, LOG_VOTE_CLASS,"ctchip_get_initialization_slp_handler SYSTEM_STATUS_BUSY");
	ctiot_chip_vote(ctchip_get_initialization_slp_handler(), SYSTEM_STATUS_BUSY);
	

	//3.持久化数据处理（读取bootflag、恢复）
	
	ret = cache_get_bootflag(&cacheBootFlag);
	ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"cacheBootFlag:bootFlag=%d\r\n", cacheBootFlag);
	if (ret == NV_OK)
	{
		pContext->bootFlag = cacheBootFlag;
	}
	else if (ret == NV_ERR_CACHE_INVALID || ret == NV_ERR_CACHE_IS_NULL)
	{
		ctiot_log_debug(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"ctiot_engine_entry:checksum fail,ret = %d", ret);
		if (ret == NV_ERR_CACHE_INVALID)
		{
			if (c2f_encode_context(pContext, true) != NV_OK)//!!cache中数据校验失败
			{
				ctiot_publish_sdk_notification(NULL, CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LSTATUS, CTIOT_LSTATUS_QUITENGINE, 0, 0, NULL);
				ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_INITIALIZE_FAILED);
				goto exit;
			}
			ctiot_publish_sdk_notification(NULL, CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LSTATUS, CTIOT_LSTATUS_LWENGINE_INI_FAIL, sesdatachecksumfail, 0, NULL);
		}
	}
	ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"ctiot_engine_entry:bootFlag=%d\r\n", pContext->bootFlag);

	//4.处理持久化配置参数恢复, 两个投票句柄，四个锁句柄初始化
	if (ctiotprv_system_para_init(pContext) != CTIOT_NB_SUCCESS)//错误只有SYS_API_ERROR
	{
		ctiot_publish_sdk_notification(NULL, CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LSTATUS, CTIOT_LSTATUS_QUITENGINE, 0, 0, NULL);
		ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_INITIALIZE_FAILED);
		goto exit;
	}

	ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"ctiot_engine_entry: sys para init OK...\r\n");
	if (pContext->bootFlag == BOOT_NOT_LOAD)
	{ //不需要恢复会话
		ctiot_update_session_status( NULL,pContext->sessionStatus, UE_NOT_LOGINED);
	}
	else
	{
		startReason = ctchip_get_system_boot_reason();
		ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"ctiot_engine_entry:startReason:%d\r\n", startReason);
		ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"ctiot_engine_entry:onKeepSession:%d\r\n", pContext->onKeepSession);
		if (startReason == CTIOT_ACTIVE_REBOOT && pContext->fotaFlag == 0 && (pContext->onKeepSession == SYSTEM_ON_REBOOT_STOP_SESSION || pContext->onKeepSession == SYSTEM_ON_REBOOT_STOP_SESSION_WITH_AUTO_UPDATE))
		{
			//Reboot+不跨Root恢复会话
			ctiot_log_debug(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"startReason == CTIOT_ACTIVE_REBOOT && pContext->onKeepSession == SYSTEM_ON_REBOOT_STOP_SESSION  || pContext->onKeepSession == SYSTEM_ON_REBOOT_STOP_SESSION_WITH_AUTO_UPDATE");
			pContext->bootFlag = BOOT_NOT_LOAD;
			uint16_t flagIP = ctchip_write_session_ip(NULL);//ctchip_write_session_ip 该接口未适配
			int32_t flagC2F = c2f_encode_context(pContext, true);
			ctiot_log_debug(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"ctchip_write_session_ip:%u,c2f_encode_context:%d", flagIP, flagC2F);
			if ((flagIP != CTIOT_NB_SUCCESS) || (flagC2F != NV_OK))
			{
				ctiot_publish_sdk_notification(NULL, CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LSTATUS, CTIOT_LSTATUS_QUITENGINE, 0, 0, NULL);
				ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_INITIALIZE_FAILED);
				goto exit;
			}

			ctiot_update_session_status( NULL,pContext->sessionStatus, UE_NOT_LOGINED);
		}
		else
		{
#ifdef PLATFORM_XINYI	

			if(pContext->fotaFlag == 1 && xy_ctlw_is_lifetimeOut())//fota触发软重启，需要向云平台上报相关信息，lifetime生命周期到期，必须走重新注册
			{
				ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"need fota,but lifetime is out,registration\r\n");
				xy_ctlw_auto_registration();
				goto exit;
			}
			else if(NET_NEED_RECOVERY(CTWING_TASK) && xy_ctlw_is_lifetimeOut()) //深睡恢复,lifetime到期
			{
				ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"need xy recovery,lifetime is out,end\r\n");
				//CTWING业务恢复失败，lifetime生命周期已超，清除恢复标志位
				xy_ctlw_clear_recoverFlag();
				//CTWING业务恢复失败，lifetime生命周期已超，释放恢复信号量
				xy_ctlw_releaseRecoverySem();
				//CTWING业务深睡恢复失败，删除session文件
				cloud_remove_file(CTLW_SESSION_FILE_UNDER_DIR);
				ctiot_update_session_status( NULL,pContext->sessionStatus, UE_NOT_LOGINED);

				goto exit;
			}
			else if(!NET_NEED_RECOVERY(CTWING_TASK) && xy_ctlw_is_lifetimeOut())//断电上电，跨会话，lifetime到期
			{
				ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"reboot recovery,lifetime is out,registration\r\n");
				xy_ctlw_auto_registration();
				goto exit;
			}

#endif
#if CTIOT_CHIPSUPPORT_DTLS == 1
			if(pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS)
			{
				if(pContext->clientWorkMode == U_WORK_MODE)//u+dtls/dtlsplus模式下,休眠苏醒退出
				{
					ctiot_log_debug(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"ctiotprv_process_session_restore dtls recover\r\n");
					pContext->bootFlag = BOOT_NOT_LOAD;
					uint16_t flagIP = ctchip_write_session_ip(NULL);
					int32_t flagC2F = c2f_encode_context(pContext, true);
					if ((flagIP != CTIOT_NB_SUCCESS) || (flagC2F != NV_OK))
					{
						ctiot_publish_sdk_notification(NULL, CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LSTATUS, CTIOT_LSTATUS_QUITENGINE, 0, 0, NULL);
						ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_INITIALIZE_FAILED);
						goto exit;
					}
					ctiot_publish_sdk_notification(NULL,CTIOT_SYSTEM_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LSTATUS, CTIOT_LSTATUS_QUITSESSION, setmoderr, 0, NULL);
					ctiot_update_session_status( NULL,pContext->sessionStatus, UE_NOT_LOGINED);
					goto exit;
				}
			}
#endif
			if (!ctiotprv_init_system_list(pContext))
			{
				ctiotprv_free_system_list(pContext);
				ctiot_publish_sdk_notification(NULL, CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LSTATUS, CTIOT_LSTATUS_QUITENGINE, 0, 0, NULL);
				ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_INITIALIZE_FAILED);
				goto exit;
			}


			ret = cache_get_address_family(&pContext->addressFamily);
			if(ret != NV_OK)//不可能出现读错误
			{
				pContext->bootFlag = BOOT_NOT_LOAD;
				ctiotprv_free_system_list(pContext);
				if (c2f_encode_context(pContext, true) != NV_OK)
				{
					ctiot_publish_sdk_notification(NULL, CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LSTATUS, CTIOT_LSTATUS_QUITENGINE, 0, 0, NULL);
					ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_INITIALIZE_FAILED);
					goto exit;
				}
				ctiot_publish_sdk_notification(NULL, CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LSTATUS, CTIOT_LSTATUS_LWENGINE_INI_FAIL, sesdatachecksumfail, 0, NULL);
				ctiot_update_session_status( NULL,pContext->sessionStatus, UE_NOT_LOGINED);
				goto exit;
			}

#ifdef CTLW_ANYTIME_SOCKET //Start CTLW_ANYTIME_SOCKET
			if(ctiotprv_create_socket(pContext,pContext->addressFamily) != CTIOT_NB_SUCCESS)
			{
				ctiotprv_free_system_list(pContext);
				ctiot_publish_sdk_notification(NULL, CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LSTATUS, CTIOT_LSTATUS_QUITENGINE, 0, 0, NULL);
				ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_INITIALIZE_FAILED);
				goto exit;
			}
			laterRecoverNetwork = true; //延时恢复Lw层的connection
#endif //End CTLW_ANYTIME_SOCKET

/*#ifdef PLATFORM_XINYI
			if (ctiot_start_select_thread() != CTIOT_NB_SUCCESS)
			{
				ctiotprv_free_system_list(pContext);
				ctiot_publish_sdk_notification(NULL, CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LSTATUS, CTIOT_LSTATUS_QUITENGINE, 0, 0, NULL);
				ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_INITIALIZE_FAILED);
			    goto exit;
			}
#endif*/ //no use select Thread
			if (ctiot_start_send_recv_thread() != CTIOT_NB_SUCCESS)
			{
				ctiotprv_free_system_list(pContext);
#ifdef CTLW_ANYTIME_SOCKET //Start CTLW_ANYTIME_SOCKET
				ctiot_close_socket();
#endif
				ctiot_publish_sdk_notification(NULL, CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LSTATUS, CTIOT_LSTATUS_QUITENGINE, 0, 0, NULL);
				ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_INITIALIZE_FAILED);
			    goto exit;
			}
		}
	}
	ctiot_log_debug(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"ctiot_engine_entry end \r\n");

exit:
	ctiot_log_debug(LOG_PREINIT_MODULE, LOG_VOTE_CLASS,"ctchip_get_initialization_slp_handler SYSTEM_STATUS_FREE");
	ctiot_chip_vote(ctchip_get_initialization_slp_handler(), SYSTEM_STATUS_FREE);
	ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"sdk out");
}

#endif


static bool ctiotprv_check_socket_state_step(void)
{
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();
	if (pContext->clientInfo.sock < 0)
	{
		if(ctiotprv_check_ip_and_eps(1) == CTIOT_NB_SUCCESS)
		{
			result = ctiotprv_check_socket();
			if(result == CTIOT_SYS_API_ERROR)
			{
				return false;
			}
		}
	}
	return true;
}

static bool ctiot_recv_step(ctiot_context_t *pContext)
{
	uint16_t result = CTIOT_NB_SUCCESS;

#if CTIOT_CHIPSUPPORT_DTLS == 1
	//目前仅有MODE_NO_DTLS，MODE_ENCRYPTED_PAYLOAD，MODE_DTLS 和 MODE_DTLS_PLUS 四种场景
	if((pContext->connectionType == MODE_NO_DTLS || pContext->connectionType == MODE_ENCRYPTED_PAYLOAD || pContext->dtlsStatus == DTLS_OK))
	{
#endif
		uint8_t recvCount = 0;
		if (pContext->clientInfo.sock < 0)
		{
			uint16_t ipStatus = ctiotprv_get_ip_status(pContext->serverIPType, 0);
			if(ipStatus == SDK_IP_STATUS_FALSE)
			{
				result = CTIOT_IP_NOK_ERROR;
			}
			else if(ipStatus == SDK_IP_STATUS_DISABLE)
			{
				result = CTIOT_IP_TYPE_ERROR;
			}
			else if(ipStatus == SDK_IP_STATUS_V6PREPARING)
			{
				result = CTIOT_IPV6_ONGOING_ERROR;
			}

			if(result != CTIOT_NB_SUCCESS)
			{
				return true;
			}
			int addressFamily = ctiotprv_switch_address_family_with_ip_status(ipStatus);
			result = ctiot_network_restore(true, addressFamily);
			if (result == CTIOT_SOCK_CREATE_FAIL)
			{
				ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_SOCKET_CLASS,"socket create error\r\n");
				return	false;
			}
#if CTIOT_CHIPSUPPORT_DTLS == 1
			//if(pContext->connectionType == MODE_DTLS_PLUS && pContext->clientWorkMode == U_WORK_MODE)
			if(pContext->connectionType == MODE_DTLS_PLUS || pContext->connectionType == MODE_DTLS)
			{
				ctlw_dtls_update_socket_fd(pContext);
			}
#endif
		}
		while(recvCount < CTIOT_ONE_CYCLE_RECV_COUNTS)
		{
			result = ctiotprv_coap_yield();
			if(result == CTIOT_SYS_API_ERROR)
			{
				return false;
			}
			if(result != CTIOT_NB_SUCCESS)//CTIOT_OTHER_ERROR
			{
				break;
			}
			recvCount += 1;
		}

#if CTIOT_CHIPSUPPORT_DTLS == 1
	}
#endif

	return true;
}

static bool ctiot_dtls_hs_step()
{
	ctiot_context_t *pContext = ctiot_get_context();
	bool isOk = true;
#if CTIOT_CHIPSUPPORT_DTLS == 1
	uint16_t code = CTIOT_NB_SUCCESS;
	if(pContext->atDtlsHsFlag == 1)
	{
		pContext->dtlsStatus = DTLS_SWITCHING;
		ctiot_log_debug(LOG_COMMON_MODULE, LOG_OTHER_CLASS,"dtls switching\r\n");
		//review时需加eps检查和createsocket功能
		//ctlw_usleep(30*1000*1000);
		code = ctiotprv_send_dtls_hs();

		if(code != CTIOT_NB_SUCCESS)
		{
			if(pContext->clientWorkMode == U_WORK_MODE)
			{
				if (ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_OUTING))
				{
					ctiot_set_release_flag(RELEASE_MODE_DRX_DTLS_HS_FAIL_L0, hsfail); //drx hs fail
					ctiot_log_debug(LOG_COMMON_MODULE, LOG_OTHER_CLASS,"dtls drx release\r\n");
				}
			}
			else
			{
				ctiotprv_clear_msg_list(CTIOT_DTLS_HS_ERROR);
				pContext->dtlsStatus = DTLS_NOK;
				ctiot_close_socket();
				ctiot_log_debug(LOG_COMMON_MODULE, LOG_OTHER_CLASS,"dtls nok\r\n");
				ctiot_publish_sdk_notification(NULL,CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_DTLS_HS, code, 0, 0, NULL);
			}
		}
		else
		{
			pContext->dtlsStatus = DTLS_OK;
			ctiot_log_debug(LOG_COMMON_MODULE, LOG_OTHER_CLASS,"dtls ok\r\n");
			ctiot_publish_sdk_notification(NULL,CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_DTLS_HS, code, 0, 0, NULL);
		}

		pContext->atDtlsHsFlag = 0;
	}
#endif
	return isOk;
}

void ctiot_send_recv_thread_step(void *arg)
{
	ctiot_log_debug(LOG_INIT_MODULE, LOG_OTHER_CLASS,"entering ctiot_send_recv_thread_step\r\n");

	//获取lwm2m_step同步返回状态
	uint8_t lwStepRet = UE_NOT_LOGINED;
	ctiot_context_t *pContext = ctiot_get_context();
#ifdef PLATFORM_XINYI
	if(g_ctlw_buffer == NULL)
	{
		g_ctlw_buffer = xy_malloc(CTIOT_MAX_PACKET_SIZE);
	}
	bool if_execute_resume = false;//从RECOVER_REQUIRED到STATE_READY
#endif
	ctiot_log_debug(LOG_INIT_MODULE, LOG_OTHER_CLASS,"thread entering,%d\r\n", pContext->sessionStatus);

	while (true)
	{
		int ctlwClientStatus = pContext->ctlwClientStatus;
		switch (ctlwClientStatus)
		{
		case CTIOT_STATE_INITIAL:
		{
			if (pContext->bootFlag == BOOT_LOCAL_BOOTUP) //切换至恢复会话操作
			{
				ctiot_log_info(LOG_INIT_MODULE, LOG_OTHER_CLASS,"begin restore session...\r\n");
				ctiot_change_client_status(CTIOT_STATE_INITIAL,CTIOT_STATE_RECOVER_REQUIRED);
			}
			else if (pContext->bootFlag == BOOT_NOT_LOAD) //切换至用户登录
			{
				ctiot_log_info(LOG_INIT_MODULE, LOG_OTHER_CLASS,"begin login...\r\n");
				ctiot_change_client_status(CTIOT_STATE_INITIAL,CTIOT_STATE_REG_REQUIRED);
			}
			break;
		}
		case CTIOT_STATE_RECOVER_REQUIRED:
		{
#ifdef PLATFORM_XINYI
			if_execute_resume = true;//从RECOVER_REQUIRED到STATE_READY
#endif
			//恢复会话
			ctiot_log_info(LOG_INIT_MODULE, LOG_OTHER_CLASS,"ctiot_send_recv_thread_step:CTIOT_STATE_RECOVER_REQUIRED");
			uint16_t result = ctiotprv_process_session_restore(pContext);
			if (result == CTIOT_NB_SUCCESS)
			{
				ctiot_change_client_status(CTIOT_STATE_RECOVER_REQUIRED,CTIOT_STATE_READY);
			}
			else
			{
				//其它错误在ctiotprv_process_session_restore内部已处理
				if(result == CTIOT_SYS_API_ERROR)
				{
					ctiot_exit_on_error(pContext);
				}
				ctiot_log_info(LOG_INIT_MODULE, LOG_OTHER_CLASS, "session restore failed...");
			}
			break;
		}
		case CTIOT_STATE_REG_REQUIRED:
		{
			ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_send_recv_thread_step:CTIOT_STATE_REG_REQUIRED");
			uint16_t result = ctiot_reg_step();
			if (result == CTIOT_NB_SUCCESS)
			{
				ctiot_change_client_status(CTIOT_STATE_REG_REQUIRED,CTIOT_STATE_REG_PENDING);
			}
			else
			{
				//其它错误在ctiot_reg_step内部已处理
				ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_OTHER_CLASS,"ctiot_reg_step:result=%u",result);
				if(result == CTIOT_SYS_API_ERROR)
				{
					ctiot_exit_on_error(pContext);
				}
			}
			break;
		}
		case CTIOT_STATE_REG_PENDING:
		{
			if (!ctiot_recv_step(pContext))
			{
				ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_recv_step error\r\n");
				ctiot_exit_on_error(pContext);
				continue;
			}
			time_t lwm2mTimeOut = 1;
			lwStepRet = ctlw_lwm2m_step(pContext->lwm2mContext, &lwm2mTimeOut);
			if (lwStepRet != COAP_NO_ERROR && ctlw_registration_getStatus(pContext->lwm2mContext) != STATE_REG_FAILED)
			{
				ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"lwm2m_step error\r\n");
				if(lwStepRet == COAP_503_SERVICE_UNAVAILABLE)
				{
					if (ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_OUTING))
					{
						ctiot_set_release_flag(RELEASE_MODE_LOGIN_FAIL, CTIOT_OTHER_ERROR);
					}
				}
				if(lwStepRet == COAP_500_INTERNAL_SERVER_ERROR)
				{
					ctiot_exit_on_error(pContext);//发通知，不需要立即写Flash
				}

			}
			break;
		}
		case CTIOT_STATE_READY:
		{
			//1. 处理ip事件通知
			//2.处理非AT命令触发的update
			if (pContext->sessionStatus == UE_LOGINED)
			{
				if (ctiotprv_system_msg_step(pContext) == CTIOT_SYS_API_ERROR)
				{
					ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiotprv_system_msg_step error\r\n");
					ctiot_exit_on_error(pContext);
					continue;
				}
			}

			//2. 忙闲投票
			ctiot_vote_slp_mutex_lock();
			if (pContext->sessionStatus == UE_LOGINED)
			{
				if (ctiotprv_system_can_sleep(pContext) && ctiotprv_dataList_is_empty(pContext))
				{
#if CTIOT_CHIPSUPPORT_NBSLEEPING == 1
					if (pContext != NULL && sDataFlashNeeded == CTIOT_DATA_CHANGED && (pContext->onKeepSession == SYSTEM_ON_REBOOT_STOP_SESSION || pContext->onKeepSession == SYSTEM_ON_REBOOT_STOP_SESSION_WITH_AUTO_UPDATE))
					{
						ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS, "------------------------update context-------------------------\r\n");
						if(c2f_encode_context(pContext,false)!=NV_OK)
						{
							ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"c2f_encode_context error\r\n");
							ctiot_exit_on_error(pContext);
							ctiot_vote_slp_mutex_unlock();
							continue;
						}
						sDataFlashNeeded = CTIOT_DATA_UNCHANGED;
					}
#endif
					ctiot_chip_vote(ctchip_get_send_recv_slp_handler(), SYSTEM_STATUS_FREE);

				}
			}
			ctiot_vote_slp_mutex_unlock();

			if (pContext->sessionStatus == UE_LOGINED)
			{
				//3.  AT触发的 DTLS HS 处理
				if(pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS)
				{
					if (!ctiot_dtls_hs_step())
					{
						ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_DTLS_CLASS, "ctiot_dtls_hs_step error\r\n");
						ctiot_exit_on_error(pContext);
						continue;
					}
				}
			}

			if (pContext->sessionStatus == UE_LOGINED)
			{
				//4. AT触发的外部update处理
				if (!ctiot_send_update_step())
				{
					ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_send_update_step error\r\n");
					ctiot_exit_on_error(pContext);
					continue;
				}
			}

			if (pContext->sessionStatus == UE_LOGINED)
			{
				//5. AT触发的普通报文处理
				if (!ctiot_send_step())
				{
					ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_send_step error\r\n");
					ctiot_exit_on_error(pContext);
					continue;
				}
			}
#ifdef CTLW_APP_FUNCTION
			if (pContext->sessionStatus == UE_LOGINED)
			{
				//6. APP触发的报文处理
				if (! ctiot_app_send_step())
				{
					ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_app_send_step error\r\n");
					ctiot_exit_on_error(pContext);
					continue;
				}
			}
#endif
			if (pContext->sessionStatus == UE_LOGINED)
			{
				// 7. 接收下行报文
				if (!ctiot_recv_step(pContext))
				{
					ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_recv_step error\r\n");
					ctiot_exit_on_error(pContext);
					continue;
				}
			}

			//8. 判断socket是否创建，若为创建且符合创建条件（有ip且为已登录状态）则创建socket，创建失败则进行lw1流程
			if (pContext->sessionStatus == UE_LOGINED)
			{
				if (!ctiotprv_check_socket_state_step())
				{
					ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_SOCKET_CLASS,"ctiotprv_check_socket_state_step error\r\n");
					ctiot_exit_on_error(pContext);
					continue;
				}
			}

			if(pContext->sessionStatus == UE_LOGINED)//按时间清理下行数据队列
			{
				ctiotprv_down_msg_step(pContext);
			}

			if (pContext->sessionStatus == UE_LOGINED)
			{
				//9. LW引擎处理
				time_t lwm2mTimeOut = 1;
				ctlw_lwm2m_step(pContext->lwm2mContext, &lwm2mTimeOut);
			}

#ifdef PLATFORM_XINYI

			//业务从恢复流程到达steady稳态,释放信号量通知其他等待恢复流程结束的线程
			if(if_execute_resume == true)
			{
				xy_ctlw_release_sdk_resume_sem();
				if_execute_resume = false;
				ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_VOTE_CLASS,"release_sdk_resume_sem,resume success exit");
			}
#ifdef WITH_FOTA
			extern bool g_ctlw_fota_is_no_done;
			if (pContext->sessionStatus == UE_LOGINED && pContext->fotaFlag == 1 && g_ctlw_fota_is_no_done == false)
			{
			    if(!ctlw_fota_check())
                {
                    ctiot_fota_state_changed();
                    xy_ctlw_fota_state_clear();
					
                }
			}
#endif //end WITH_FOTA
#endif//end PLATFORM_XINYI
			break;
		}
		case CTIOT_DEREG_REQUIRED:
		case CTIOT_FREE_REQUIRED:
		{
			#if CTIOT_TIMER_AUTO_UPDATE == 1
			if(pContext->onKeepSession == SYSTEM_ON_REBOOT_STOP_SESSION_WITH_AUTO_UPDATE)
			{
				ctchip_stop_sleep_timer();
			}
			#endif
			ctiot_release_step();
			if(ctlwClientStatus == CTIOT_DEREG_REQUIRED)
			{
				ctiot_change_client_status(CTIOT_DEREG_REQUIRED,CTIOT_STATE_INITIAL);
			}
			else
			{
				ctiot_change_client_status(CTIOT_FREE_REQUIRED,CTIOT_STATE_INITIAL);
			}
			goto exit;

		}
		default:
			break;
		}
#ifdef PLATFORM_XINYI
		//检查是否有缓存数据未被读取,若有缓存数据,则加锁,不允许芯片进入deepsleep
		xy_ctlw_check_if_sleep_allow(ctlwClientStatus, pContext);
#endif
		if (ctlwClientStatus == CTIOT_STATE_READY && ctiot_get_vote_status() == 0)
		{
			//ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_VOTE_CLASS,"sleep on free");
			ctlw_usleep(CTIOT_THREAD_TIMEOUT_FREE);
		}
		else
		{
			//ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_VOTE_CLASS,"normal sleep");
			ctlw_usleep(CTIOT_THREAD_TIMEOUT);
		}
/*#ifdef PLATFORM_XINYI
		xy_sendAndrecv_sem_take();
#endif*/ // no use select Thread
	}
exit:
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_VOTE_CLASS,"ctchip_get_ip_event_slp_handler():SYSTEM_STATUS_FREE");
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_VOTE_CLASS,"ctchip_get_send_recv_slp_handler:SYSTEM_STATUS_FREE");
	ctiot_chip_vote(ctchip_get_ip_event_slp_handler(), SYSTEM_STATUS_FREE);
	ctiot_chip_vote(ctchip_get_send_recv_slp_handler(), SYSTEM_STATUS_FREE) ;
	pContext->sendAndRecvThreadStatus = THREAD_STATUS_NOT_LOADED;
#ifdef PLATFORM_XINYI
	if(g_ctlw_buffer)
	{
		xy_free(g_ctlw_buffer);
		g_ctlw_buffer = NULL;
	}
	
	//从恢复流程到退出线程,恢复失败,释放信号量,通知其他正在等待恢复流程结束的线程
	if(if_execute_resume == true)
	{
		xy_ctlw_release_sdk_resume_sem();
		if_execute_resume = false;
		ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_VOTE_CLASS,"release_sdk_resume_sem,thread exit");
	}
	
	//线程退出
	ctlw_cloud_api_sem_give(CTLW_API_DEREG_SUCCESS_SEM);//api接口发起去注册，释放信号量
	
#endif
	thread_exit(NULL);
}

int ctiot_network_restore(bool recoverConnect, int addressFamily)
{
	ctiot_context_t *pContext = ctiot_get_context();
	int result = CTIOT_NB_SUCCESS;
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_SOCKET_CLASS,"ctiot_network_restore ...\r\n");
	bool isLaterRecoverNetwork = false;
#ifdef CTLW_ANYTIME_SOCKET
	isLaterRecoverNetwork = laterRecoverNetwork;
#endif
	int32_t addressFamilyTmp = pContext->addressFamily;
	if(!isLaterRecoverNetwork && pContext->clientInfo.sock >= 0)
	{
		result = CTIOT_NB_SUCCESS;
		goto exit;
	}

	if(pContext->clientInfo.sock < 0)
	{
		if (ctiotprv_create_socket(pContext, addressFamily) != CTIOT_NB_SUCCESS)
		{
			result = CTIOT_SOCK_CREATE_FAIL;
			goto exit;
		}
		else
		{
#if CTIOT_CHIPSUPPORT_DTLS == 1
			if((pContext->connectionType == MODE_DTLS_PLUS || pContext->connectionType == MODE_DTLS) && pContext->clientWorkMode == U_WORK_MODE)
			{
				ctlw_dtls_update_socket_fd(pContext);
			}
			else
			{
#endif
				ctiotprv_update_server_socket(pContext, pContext->clientInfo.sock);
#if CTIOT_CHIPSUPPORT_DTLS == 1
			}
#endif

		}

	}

	if(recoverConnect)
	{
		if(addressFamilyTmp != pContext->addressFamily)
		{
			char* serverIP = NULL;
			int32_t port = 0;
			if(pContext->addressFamily == AF_INET) //socket IPV4
			{
				serverIP = pContext->serverIPV4;
				port = pContext->portV4;
			}
			else //socket IPV6
			{
				serverIP = pContext->serverIPV6;
				port = pContext->portV6;
			}

			char serverUri[CTIOT_MAX_IP_LEN] = {0};
#if CTIOT_CHIPSUPPORT_DTLS == 1
			if(pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS)
			{

				sprintf(serverUri, "coaps://[%s]:%d", serverIP, ctiotprv_get_port(pContext,port));
			}
			else
			{
#endif
				sprintf(serverUri, "coap://[%s]:%d", serverIP, ctiotprv_get_port(pContext,port));
#if CTIOT_CHIPSUPPORT_DTLS == 1
			}
#endif
			ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_SOCKET_CLASS, "uri:%s",serverUri);
			lwm2m_server_t *pServer = pContext->lwm2mContext->serverList;
			while(pServer)
			{
				ctlw_set_server_uri(pContext->clientInfo.securityObjP, pServer->secObjInstID, serverUri);
				pServer->sessionH = NULL;
				pServer = pServer->next;
			}
			ctlw_connection_free(pContext->clientInfo.connList);
			pContext->clientInfo.connList = NULL;

		}

		
		if (ctiotprv_recover_connect(pContext))
		{
			lwm2m_server_t *pServer = pContext->lwm2mContext->serverList;
			if(pServer!=NULL)
			{
				lwm2m_transaction_t * transacP = pContext->lwm2mContext->transactionList;
				while(transacP!=NULL)
				{
					transacP->peerH = pServer->sessionH;
					transacP = transacP->next;
				}
			}
			while (pServer != NULL)
			{
				pServer->status = STATE_REGISTERED;
				pServer = pServer->next;
			}
			
			result = CTIOT_NB_SUCCESS;
			if(addressFamilyTmp != pContext->addressFamily)
			{
				bool isFlush = false;

#if CTIOT_CHIPSUPPORT_NBSLEEPING == 0//芯片不支持休眠，默认直接写
				isFlush = true;
				ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"immediate write because CTIOT_CHIPSUPPORT_NBSLEEPING == 0...\r\n");
#else
				if(pContext->onKeepSession==SYSTEM_ON_REBOOT_KEEP_SESSION)
				{
					isFlush = true;
					ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"immediate write because mod_1=1 and value is changed...\r\n");
				}
#endif

				if(isFlush)
				{
				    if(c2f_encode_context(pContext,true)==NV_OK)
					{
					    sDataFlashNeeded = CTIOT_DATA_UNCHANGED;
					}
					else
					{
						result = CTIOT_SYS_API_ERROR;
					}
				}
				else
				{
				    sDataFlashNeeded = CTIOT_DATA_CHANGED;
				}
			}
		}
		else
		{
			ctiot_close_socket();
			result = CTIOT_OTHER_ERROR;
		}
#ifdef CTLW_ANYTIME_SOCKET
		laterRecoverNetwork = false;
#endif
	}
exit:
	return result;
}

uint16_t ctiot_reg(uint8_t *authModeStr, uint8_t *authTokenStr)
{
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();

	if (pContext->idAuthMode == AUTHMODE_EXTEND_MCU)
	{
		if (authModeStr == NULL || authTokenStr == NULL || strlen((char *)authModeStr) == 0 || strlen((char *)authTokenStr) == 0)
		{
			result = CTIOT_PARA_NUM_ERROR;
			goto exit;
		}
		if (pContext->authTokenStr != NULL)
		{
			ctiot_log_debug(LOG_AT_MODULE, LOG_OTHER_CLASS,"ctlw_lwm2m_free(pContext->authTokenStr)");
			ctlw_lwm2m_free(pContext->authTokenStr);
		}
		if (pContext->authModeStr != NULL)
		{
			ctiot_log_debug(LOG_AT_MODULE, LOG_OTHER_CLASS,"ctlw_lwm2m_free(pContext->authModeStr)");
			ctlw_lwm2m_free(pContext->authModeStr);
		}
		ctiot_log_debug(LOG_AT_MODULE, LOG_OTHER_CLASS,"ctlw_lwm2m_free cleaned");
		pContext->authTokenStr = ctlw_lwm2m_strdup((char *)authTokenStr);
		if(pContext->authTokenStr == NULL)
		{
			result = CTIOT_SYS_API_ERROR;
			ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_INITIALIZE_FAILED);
			goto exit;
		}
		pContext->authModeStr = ctlw_lwm2m_strdup((char *)authModeStr);
		if(pContext->authModeStr == NULL)
		{
			ctlw_lwm2m_free(pContext->authTokenStr);
			pContext->authTokenStr = NULL;
			result = CTIOT_SYS_API_ERROR;
			ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_INITIALIZE_FAILED);
			goto exit;
		}
	}
	else
	{
		if (authModeStr != NULL || authTokenStr != NULL)
		{
			result = CTIOT_PARA_NUM_ERROR;
			goto exit;
		}
	}
	if (pContext->idAuthMode == AUTHMODE_EXTEND_MODULE && pContext->idAuthType == AUTHTYPE_NO)
	{
		result = CTIOT_PARA_NOT_INIT_ERROR;
		goto exit;
	}


#if CTIOT_WITH_PAYLOAD_ENCRYPT == 1
	if(pContext->connectionType == MODE_ENCRYPTED_PAYLOAD)
	{
		if(pContext->payloadEncryptPin == NULL)
		{
			result = CTIOT_PARA_NOT_INIT_ERROR;
			goto exit;
		}
	}
#endif
	ctiot_log_debug(LOG_AT_MODULE, LOG_SESSTATUS_CLASS,"ctiot_reg:pContext->sessionStatus=%d\r\n", pContext->sessionStatus);

	if ((pContext->serverIPV4 == NULL || strlen(pContext->serverIPV4) < 7)
		&& (pContext->serverIPV6 == NULL || strlen(pContext->serverIPV6) < 3))
	{
		ctiot_log_info(LOG_AT_MODULE, LOG_OTHER_CLASS, "ctiot_reg:ip fail");
		result = CTIOT_PARA_NOT_INIT_ERROR;
		goto exit;
	}

	ctiot_log_debug(LOG_AT_MODULE, LOG_OTHER_CLASS, "ctiot_reg:lifetime:%d\r\n", pContext->lifetime);
	if (pContext->lifetime < CTIOT_MIN_LIFETIME || pContext->lifetime > CTIOT_MAX_LIFETIME)
	{
		result = CTIOT_PARA_NOT_INIT_ERROR;
		goto exit;
	}

#if CTIOT_CHIPSUPPORT_DTLS == 1
	if(pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS)
	{
		if (pContext->pskID == NULL || strlen(pContext->pskID) == 0 || pContext->psk == NULL || pContext->pskLen == 0)
		{
			ctiot_log_debug(LOG_AT_MODULE, LOG_OTHER_CLASS, "ctiot_reg:psk fail");
			result = CTIOT_PARA_NOT_INIT_ERROR;
			goto exit;
		}
	}
#endif
	ctiot_session_status_e sessionStatus;
	if(pContext->dns_processing_status == 1)//正在进行dns处理
	{
		result = CTIOT_DNS_ING_ERROR;
		goto exit;
	}
	if (ctiot_update_session_status( &sessionStatus,UE_NOT_LOGINED, UE_LOGINING))
	{

		uint8_t epsValue = ctchip_sync_cstate();
		if(epsValue != NETWORK_CONNECTED_HOME_NETWORK && epsValue != NETWORK_CONNECTED_ROAMING)
		{
			result = epsValue + CTIOT_NETWORK_ERROR_BASE;
			ctiot_update_session_status( NULL,UE_LOGINING, UE_NOT_LOGINED);
			goto exit;
		}
		ctiotprv_set_server_ip_type(pContext);

		uint16_t ipStatus = ctiotprv_get_ip_status(pContext->serverIPType, 1);
		if(ipStatus == SDK_IP_STATUS_FALSE)
		{
			ctiot_log_info(LOG_AT_MODULE,LOG_IP_CLASS,"reg:SDK_IP_STATUS_FALSE");
			result = CTIOT_IP_NOK_ERROR;
		}
		else if(ipStatus == SDK_IP_STATUS_DISABLE)
		{
			ctiot_log_info(LOG_AT_MODULE,LOG_IP_CLASS,"reg:SDK_IP_STATUS_DISABLE");
			result = CTIOT_IP_TYPE_ERROR;
		}
		else if(ipStatus == SDK_IP_STATUS_V6PREPARING)
		{
			ctiot_log_info(LOG_AT_MODULE,LOG_IP_CLASS,"reg:SDK_IP_STATUS_V6PREPARING");
			result = CTIOT_IPV6_ONGOING_ERROR;
		}

		if(result != CTIOT_NB_SUCCESS)
		{
			ctiot_log_debug(LOG_AT_MODULE, LOG_IP_CLASS,"ctlw_reg:ctiotprv_get_ip_status:%u",ipStatus);
			ctiot_update_session_status( NULL,UE_LOGINING, UE_NOT_LOGINED);
			goto exit;
		}

		pContext->ctlwClientStatus = CTIOT_STATE_INITIAL;
/*#ifdef PLATFORM_XINYI
		//select监听线程
		result = ctiot_start_select_thread();
		if (result != CTIOT_NB_SUCCESS)
		{
			if(result == CTIOT_SYS_API_ERROR)
			{
				ctiot_update_session_status( NULL,UE_LOGINING, UE_LOGIN_INITIALIZE_FAILED);
			}
			else
			{
				ctiot_update_session_status( NULL,UE_LOGINING, UE_NOT_LOGINED);
			}
			goto exit;
		}
#endif*/ //no use select Thread
		//启动收发线程
		result = ctiot_start_send_recv_thread();
		if (result != CTIOT_NB_SUCCESS)
		{
			if(result == CTIOT_SYS_API_ERROR)
			{
				ctiot_update_session_status( NULL,UE_LOGINING, UE_LOGIN_INITIALIZE_FAILED);
			}
			else
			{
				ctiot_update_session_status( NULL,UE_LOGINING, UE_NOT_LOGINED);
			}
			goto exit;
		}

		ctiot_log_debug(LOG_AT_MODULE, LOG_SESSTATUS_CLASS,"put reg msg OK,%d\r\n",pContext->sessionStatus);
	}
	else
	{
		result = (uint16_t)CTIOT_SESSION_ERROR_BASE+ctiotprv_get_session_status(pContext);
	}

exit:
	return result;
}

uint16_t ctiot_set_recv_data_mode( recv_data_mode_e mode, ctiot_timemode_e maxCacheTimeMode)
{
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();

	recv_data_mode_e tmpMode = RECV_DATA_MODE_0;
	ctiot_timemode_e tmpMaxCacheTimeMode = CTIOT_TIMEMODE_0;
	uint16_t tmpMaxCacheTime = 0;
	ctiot_session_status_e sessionStatus;
	
	if (mode > RECV_DATA_MODE_2)
	{
		result = CTIOT_PARA_VALUE_ERROR;
		goto exit;
	}
#if CTIOT_RECEIVE_MODE0 == 0
	if (mode == RECV_DATA_MODE_0)
	{
		result = CTIOT_PARA_NOSUP_ERROR;
		goto exit;
	}
#endif
#if CTIOT_RECEIVE_MODE1 == 0
	if (mode == RECV_DATA_MODE_1)
	{
		result = CTIOT_PARA_NOSUP_ERROR;
		goto exit;
	}
#endif
#if CTIOT_RECEIVE_MODE2 == 0
	if (mode == RECV_DATA_MODE_2)
	{
		result = CTIOT_PARA_NOSUP_ERROR;
		goto exit;
	}
#endif
	if (maxCacheTimeMode > CTIOT_TIMEMODE_256)
	{
		result = CTIOT_PARA_VALUE_ERROR;
		goto exit;
	}
	sessionStatus =ctiotprv_get_session_status(pContext);
	if (sessionStatus == UE_NOT_LOGINED)
	{
		if(pContext->dns_processing_status == 1)//正在进行dns处理
		{
			result = CTIOT_DNS_ING_ERROR;
			goto exit;
		}
		tmpMode = pContext->recvDataMode;
		tmpMaxCacheTimeMode = pContext->recvTimeMode;
		tmpMaxCacheTime = pContext->recvDataMaxCacheTime;
		pContext->recvDataMode = mode;
		switch (maxCacheTimeMode)
		{
		case CTIOT_TIMEMODE_1:
		{
			pContext->recvTimeMode = CTIOT_TIMEMODE_1;
			pContext->recvDataMaxCacheTime = 1;
			break;
		}
		case CTIOT_TIMEMODE_2:
		{
			pContext->recvTimeMode = CTIOT_TIMEMODE_2;
			pContext->recvDataMaxCacheTime = 2;
			break;
		}
		case CTIOT_TIMEMODE_8:
		{
			pContext->recvTimeMode = CTIOT_TIMEMODE_8;
			pContext->recvDataMaxCacheTime = 8;
			break;
		}
		case CTIOT_TIMEMODE_16:
		{
			pContext->recvTimeMode = CTIOT_TIMEMODE_16;
			pContext->recvDataMaxCacheTime = 16;
			break;
		}
		case CTIOT_TIMEMODE_32:
		{
			pContext->recvTimeMode = CTIOT_TIMEMODE_32;
			pContext->recvDataMaxCacheTime = 32;
			break;
		}
		case CTIOT_TIMEMODE_64:
		{
			pContext->recvTimeMode = CTIOT_TIMEMODE_64;
			pContext->recvDataMaxCacheTime = 64;
			break;
		}
		case CTIOT_TIMEMODE_128:
		{
			pContext->recvTimeMode = CTIOT_TIMEMODE_128;
			pContext->recvDataMaxCacheTime = 128;
			break;
		}
		case CTIOT_TIMEMODE_256:
		{
			pContext->recvTimeMode = CTIOT_TIMEMODE_256;
			pContext->recvDataMaxCacheTime = 256;
			break;
		}
		default:
		{
			pContext->recvTimeMode = CTIOT_TIMEMODE_0;
			pContext->recvDataMaxCacheTime = 0;
			break;
		}
		}
	}
	else
	{
		result = CTIOT_SESSION_ERROR_BASE + sessionStatus;
		goto exit;
	}

	if (c2f_encode_params(pContext) != NV_OK)
	{
		pContext->recvDataMode = tmpMode;
		pContext->recvTimeMode = tmpMaxCacheTimeMode;
		pContext->recvDataMaxCacheTime = tmpMaxCacheTime;
		result = CTIOT_SYS_API_ERROR;
		pContext->sessionStatus = UE_LOGIN_INITIALIZE_FAILED;
		goto exit;
	}

exit:

	return result;
}

uint16_t ctiot_get_recv_data_mode( uint8_t *buff)//调用时，保证buff不为空，且长度足够
{
	uint16_t ret = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();

	if (pContext->sessionStatus == UE_LOGIN_INITIALIZE)
	{
		return CTIOT_SESSION_ERROR_BASE + UE_LOGIN_INITIALIZE;
	}

	uint8_t *header = "+CTLWRECV";
	if (buff != NULL)
	{
		if(pContext->recvDataMode == 0 || pContext->recvDataMode == 2)
		{
			sprintf((char *)buff, "\r\n%s: %u\r\n", header, pContext->recvDataMode);
		}
		else
		{
			sprintf((char *)buff, "\r\n%s: %u,%u\r\n", header, pContext->recvDataMode, pContext->recvTimeMode);
		}
	}
	else
	{
		return CTIOT_OTHER_ERROR;
	}
	return ret;
}

uint16_t ctiot_get_recv_data(uint8_t *buff)//调用时，保证buff不为空，且长度足够
{
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();
	ctiot_log_debug(LOG_AT_MODULE, LOG_OTHER_CLASS,"ctiot_get_recv_data()\r\n");
	ctiot_session_status_e sessionStatus = (ctiot_session_status_e)ctiotprv_get_session_status(pContext);
	if (sessionStatus == UE_LOGINED || sessionStatus == UE_LOGINED_OBSERVED)
	{
		if (pContext->recvDataMode == RECV_DATA_MODE_0)
		{
			result = CTIOT_NO_QUEUE_ERROR;
			goto exit;
		}
#ifdef PLATFORM_XINYI
		ctiotprv_down_msg_step(pContext);//清除超时下行数据
#endif
		ctiot_down_msg_list *node = (ctiot_down_msg_list *)ctiot_coap_queue_get(pContext->downMsgList);
		ctiot_log_debug(LOG_AT_MODULE, LOG_OTHER_CLASS,"ctiot_get_recv_data():ctiot_coap_queue_get()\r\n");
		uint8_t *payload = NULL;
		uint8_t type = 0;
		uint16_t payloadLen = 0;
		if (node != NULL)
		{
			payload = node->payload;
			type = node->msgStatus;
			if (payload != NULL)
				payloadLen = strlen((char *)payload);
			ctiot_log_info(LOG_AT_MODULE, LOG_OTHER_CLASS,"payload_len:%d\r\n", payloadLen);
			sprintf((char *)buff, "+CTLWGETRECVDATA: %d,%d,%s", type, payloadLen, (char *)payload);
			ctlw_lwm2m_free(node->payload);
			ctlw_lwm2m_free(node);
		}
		else
		{
			sprintf((char *)buff, "+CTLWGETRECVDATA: 0,0");
		}
/*#ifdef PLATFORM_XINYI
		xy_sendAndrecv_sem_give();
#endif*/ // no use select Thread
	}
	else
	{
		result = CTIOT_SESSION_ERROR_BASE + sessionStatus;
	}

exit:
	return result;
}

uint16_t ctiot_query_version(uint8_t *buff)
{
	uint16_t result = CTIOT_OTHER_ERROR;//默认返回其他异常
	ctiot_context_t *pContext = ctiot_get_context();
	uint8_t *header = "+CTLWVER";
	if (buff != NULL)
	{
		if (pContext->sessionStatus == UE_LOGIN_INITIALIZE || pContext->sessionStatus == UE_LOGIN_INITIALIZE_FAILED)
		{//当前会话状态不允许，返回：错误码80X
			result = CTIOT_SESSION_ERROR_BASE + ctiotprv_get_session_status(pContext);
		}
		else
		{//正常，返回：OK+版本信息
			ctchip_get_module_info( (uint8_t *)pContext->chipInfo->sv,CTIOT_SV_LEN, (uint8_t *)pContext->chipInfo->chip,CTIOT_CHIP_LEN, (uint8_t *)pContext->chipInfo->module,CTIOT_MODULE_LEN);
			sprintf((char *)buff, "\r\n%s: %s,%s,%s,%s,%s\r\n", header, LWM2M_VERSION,CTIOT_SDK_VERSION, pContext->chipInfo->module, pContext->chipInfo->chip, pContext->chipInfo->sv);
			result =CTIOT_NB_SUCCESS;
		}
	}
	return result;
}

uint16_t ctiot_set_mod( ctiot_mode_id_e modeId, uint8_t modeValue)
{
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();
	uint8_t tmpModeValue = 0;

#if CTIOT_ENABLE_MODE1 == 0
	if (modeId == ONKEEPSESSION_MODE)
	{
		return CTIOT_PARA_NOSUP_ERROR;
	}
#endif
#if CTIOT_ENABLE_MODE2 == 0
	if (modeId == REGISTER_MODE)
	{
		return CTIOT_PARA_NOSUP_ERROR;
	}
#endif
#if CTIOT_ENABLE_MODE3 == 0
	if (modeId == AUTH_PROTOCOL_TYPE)
	{
		return CTIOT_PARA_NOSUP_ERROR;
	}
#endif
#if CTIOT_ENABLE_MODE4 == 0
	if (modeId == MODE_DTLS_TYPE)
	{
		return CTIOT_PARA_NOSUP_ERROR;
	}
#endif
#if CTIOT_ENABLE_MODE5 == 0
	if (modeId == CLIENT_U_OR_UQ)
	{
		return CTIOT_PARA_NOSUP_ERROR;
	}
#endif
	if(pContext->dns_processing_status == 1)//正在进行dns处理
	{
		return CTIOT_DNS_ING_ERROR;
	}
	switch (modeId)
	{
		case ONKEEPSESSION_MODE: //Session Over Reboot
		{
			if (modeValue >= SYSTEM_ON_REBOOT_MAX)
			{
				return CTIOT_PARA_VALUE_ERROR;
			}
			ctiot_session_status_e sessionStatus = (ctiot_session_status_e)ctiotprv_get_session_status(pContext);
			if (sessionStatus == UE_NOT_LOGINED)
			{
				if(pContext->clientWorkMode == U_WORK_MODE && (pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS) && (modeValue == SYSTEM_ON_REBOOT_KEEP_SESSION || modeValue == SYSTEM_ON_REBOOT_STOP_SESSION_WITH_AUTO_UPDATE))
				{
					return CTIOT_PARAM_CONFLICATE;
				}
				tmpModeValue = pContext->onKeepSession;
				pContext->onKeepSession = (ctiot_keep_session_e)modeValue;
			}
			else
			{
				result = CTIOT_SESSION_ERROR_BASE + sessionStatus;
			}
			break;
		}
		case REGISTER_MODE: //Register Parameter Mode
		{
			if (modeValue >= PARAM_MODE_MAX)
			{
				return CTIOT_PARA_VALUE_ERROR;
			}
			ctiot_session_status_e sessionStatus = (ctiot_session_status_e)ctiotprv_get_session_status(pContext);
			if (sessionStatus == UE_NOT_LOGINED)
			{
				tmpModeValue = pContext->regParaMode;
				pContext->regParaMode = (reg_param_mode_e)modeValue;
			}
			else
			{
				result = CTIOT_SESSION_ERROR_BASE + sessionStatus;
			}
			break;
		}
		case AUTH_PROTOCOL_TYPE:
		{
			if (modeValue >= AUTHMODE_MAX)
			{
				return CTIOT_PARA_VALUE_ERROR;
			}
			ctiot_session_status_e sessionStatus = (ctiot_session_status_e)ctiotprv_get_session_status(pContext);
			if (sessionStatus == UE_NOT_LOGINED)
			{
#if CTIOT_SIMID_ENABLED == 0
				if (modeValue == AUTHMODE_EXTEND_MODULE)
				{
					return CTIOT_PARA_NOSUP_ERROR;
				}
#endif
				if(pContext->connectionType == MODE_ENCRYPTED_PAYLOAD && (modeValue == AUTHMODE_EXTEND_MCU || modeValue == AUTHMODE_EXTEND_MODULE))
				{
					return CTIOT_PARAM_CONFLICATE;
				}
				tmpModeValue = pContext->idAuthMode;
				pContext->idAuthMode = (ctiot_id_auth_mode_e)modeValue;
			}
			else
			{
				result = CTIOT_SESSION_ERROR_BASE + sessionStatus;
			}
			break;
		}
		case MODE_DTLS_TYPE:
		{
			if (modeValue > 9 ||(modeValue > 2 && modeValue < 9))
			{
				return CTIOT_PARA_VALUE_ERROR;
			}
			ctiot_session_status_e sessionStatus = (ctiot_session_status_e)ctiotprv_get_session_status(pContext);
			if (sessionStatus == UE_NOT_LOGINED)
			{
#if CTIOT_CHIPSUPPORT_DTLS == 0
				if(modeValue == 1 ||modeValue ==9)
				{
					return CTIOT_PARA_NOSUP_ERROR;
				}
#endif
#if CTIOT_WITH_PAYLOAD_ENCRYPT ==0
				if(modeValue == MODE_ENCRYPTED_PAYLOAD)
				{
					return CTIOT_PARA_NOSUP_ERROR;
				}
#endif
				if(modeValue == MODE_ENCRYPTED_PAYLOAD && (pContext->idAuthMode == AUTHMODE_EXTEND_MCU || pContext->idAuthMode == AUTHMODE_EXTEND_MODULE))
				{
					return CTIOT_PARAM_CONFLICATE;
				}
#if	CTIOT_CHIPSUPPORT_DTLS == 1

				if(pContext->clientWorkMode == U_WORK_MODE&& modeValue == MODE_DTLS)
				{
					return CTIOT_PARA_VALUE_ERROR;
				}
				if(pContext->clientWorkMode == U_WORK_MODE && (pContext->onKeepSession == SYSTEM_ON_REBOOT_KEEP_SESSION || pContext->onKeepSession == SYSTEM_ON_REBOOT_STOP_SESSION_WITH_AUTO_UPDATE) 
				&& modeValue == MODE_DTLS_PLUS)
				{
					return CTIOT_PARAM_CONFLICATE;
				}
#endif
				tmpModeValue = pContext->connectionType;
				pContext->connectionType = (mode_connection_type_e)modeValue;
			}
			else
			{
				result = CTIOT_SESSION_ERROR_BASE + sessionStatus;
			}
			break;
		}
		case CLIENT_U_OR_UQ:
		{
			if (modeValue >= WORK_MODE_MAX)
			{
				return CTIOT_PARA_VALUE_ERROR;
			}

			ctiot_session_status_e sessionStatus = (ctiot_session_status_e)ctiotprv_get_session_status(pContext);
			if (sessionStatus == UE_NOT_LOGINED)
			{
#if	CTIOT_CHIPSUPPORT_DTLS == 1
				if(modeValue == U_WORK_MODE && pContext->connectionType == MODE_DTLS)
				{
					return CTIOT_PARA_VALUE_ERROR;
				}
				if(modeValue == U_WORK_MODE && (pContext->onKeepSession == SYSTEM_ON_REBOOT_KEEP_SESSION || pContext->onKeepSession == SYSTEM_ON_REBOOT_STOP_SESSION_WITH_AUTO_UPDATE)  && pContext->connectionType == MODE_DTLS_PLUS)
				{
					return CTIOT_PARAM_CONFLICATE;
				}
#endif
				tmpModeValue = pContext->clientWorkMode;
				pContext->clientWorkMode = (client_work_mode_e)modeValue;
			}
			else
			{
				result = CTIOT_SESSION_ERROR_BASE + sessionStatus;
			}
			break;
		}
		default:
		{
			return CTIOT_PARA_VALUE_ERROR;
		}
	}

	if (result == CTIOT_NB_SUCCESS && c2f_encode_params(pContext) != NV_OK)
	{
		result = CTIOT_SYS_API_ERROR;
		pContext->sessionStatus = UE_LOGIN_INITIALIZE_FAILED;
		switch (modeId)
		{
		case ONKEEPSESSION_MODE:
		{
			pContext->onKeepSession = (ctiot_keep_session_e)tmpModeValue;
			break;
		}
		case REGISTER_MODE:
		{
			pContext->regParaMode = (reg_param_mode_e)tmpModeValue;
			break;
		}
		case MODE_DTLS_TYPE:
		{
			pContext->connectionType = (mode_connection_type_e)tmpModeValue;
			break;
		}
		case AUTH_PROTOCOL_TYPE:
		{
			pContext->idAuthMode = (ctiot_id_auth_mode_e)tmpModeValue;
			break;
		}
		case CLIENT_U_OR_UQ:
		{
			pContext->clientWorkMode = (client_work_mode_e)tmpModeValue;
			break;
		}
		default:
			break;
		}
	}
	return result;
}

uint16_t ctiot_get_mod(uint8_t *buff)//调用时，保证buff不为空，且长度足够
{
	uint16_t ret = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext =ctiot_get_context();

	if (pContext->sessionStatus == UE_LOGIN_INITIALIZE)
	{
		return CTIOT_SESSION_ERROR_BASE + UE_LOGIN_INITIALIZE;
	}

	if (buff != NULL)
	{
		sprintf((char *)buff, "\r\n+CTLWSETMOD: 1, %d\r\n+CTLWSETMOD: 2, %d\r\n+CTLWSETMOD: 3, %d\r\n+CTLWSETMOD: 4, %d\r\n+CTLWSETMOD: 5, %d\r\n",
				pContext->onKeepSession, pContext->regParaMode, pContext->idAuthMode,
				pContext->connectionType, pContext->clientWorkMode);
	}
	else
	{
		return CTIOT_OTHER_ERROR;
	}

	return ret;
}

void ctiot_ip_event_async(void)
{
	ctiot_context_t *pContext = ctiot_get_context();
	ctiot_signal_mutex_lock();
	ctiot_chip_vote(ctchip_get_ip_event_slp_handler(), SYSTEM_STATUS_BUSY) ;

	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_VOTE_CLASS,"ctchip_get_ip_event_slp_handler():SYSTEM_STATUS_BUSY");

	pContext->IPEventFlag = 1;//有ip事件
	ctiot_signal_mutex_unlock();
}

uint16_t ctiot_reg_step()
{
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();

	//适配层初始化
	result = ctiot_signal_init();
	if (result != CTIOT_NB_SUCCESS)
	{
		ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_signal_init failed\r\n");
		result = CTIOT_SYS_API_ERROR;
		goto exit;
	}

	if (!ctiotprv_init_system_list(pContext))
	{
		result = CTIOT_SYS_API_ERROR;
		goto exit;
	}

	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_SESSTATUS_CLASS,"pContext->sessionStatus=%d\r\n", pContext->sessionStatus);

	result = ctiotprv_check_ip_and_eps(0);
	if (result != CTIOT_NB_SUCCESS)
	{
		goto exit;
	}
	else
	{
		ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_reg:Network Ok\r\n");
		ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"pContext->idAuthMode=%d\tpContext->idAuthType=%d\r\n", pContext->idAuthMode, pContext->idAuthType);
		//apn/imei/iccid/芯片固件版本:若芯片支持同步查询，应在同步接口返回
		if(ctchip_get_apn_info((uint8_t *)pContext->chipInfo->apn,CTIOT_APN_LEN)!=CTIOT_NB_SUCCESS || ctchip_get_imei_info((uint8_t *)pContext->chipInfo->imei,CTIOT_IMEI_LEN) != CTIOT_NB_SUCCESS ||ctchip_get_iccid_info((uint8_t *)pContext->chipInfo->iccid,CTIOT_ICCID_LEN)!=CTIOT_NB_SUCCESS)
		{
			result = CTIOT_IN_PARA_ERROR;
			goto exit;
		}

		if(ctchip_get_module_info((uint8_t *)pContext->chipInfo->sv,CTIOT_SV_LEN,(uint8_t *)pContext->chipInfo->chip,CTIOT_CHIP_LEN,(uint8_t *)pContext->chipInfo->module,CTIOT_MODULE_LEN))
		{
			result = CTIOT_IN_PARA_ERROR;
			goto exit;
		}
		ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"begin get wireless info...");
		if(ctchip_get_wireless_signal_info((uint8_t *)pContext->wirelessSignal.rsrp, 6,(uint8_t *)pContext->wirelessSignal.sinr, 6,(uint8_t *)pContext->wirelessSignal.txpower,6,(uint8_t *)pContext->wirelessSignal.cellid, CTIOT_CELLID_LEN))
		{
			result = CTIOT_IN_PARA_ERROR;
			goto exit;
		}
		ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"get wireless info success...,rsrp=%s\tsinr=%s\ttxpower=%s\tcellid=%s",pContext->wirelessSignal.rsrp,pContext->wirelessSignal.sinr,pContext->wirelessSignal.txpower,pContext->wirelessSignal.cellid);
		if (ctchip_get_imsi_info((uint8_t *)pContext->chipInfo->imsi,CTIOT_IMSI_LEN)!=CTIOT_NB_SUCCESS )
		{
			ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_reg:ctchip_get_imsi_info fail...\r\n");
			result = CTIOT_IN_PARA_ERROR;
			//处理imsi获取错误
			goto exit;
		}
#if	CTIOT_TIMER_AUTO_UPDATE == 1           
		ctiot_set_auto_update_flag(0);
#endif
#if CTIOT_WITH_PAYLOAD_ENCRYPT == 1
	if(pContext->connectionType == MODE_ENCRYPTED_PAYLOAD && pContext->payloadEncryptAlgorithm == PAYLOAD_ENCRYPT_SM2 && pContext->payloadEncryptInitialized == 0)
	{
		//初始化payloadEncrypt设备,需添加
		if(Sec_dev_PinCfg(pContext->payloadEncryptPin,strlen(pContext->payloadEncryptPin))!=0)
		{
			result = CTIOT_ENC_PIN_INIT_ERROR;
			goto exit;
		}
		if(Sec_dev_Init(1)!=0)
		{
			result = CTIOT_ENC_INIT_ERROR;
			goto exit;
		}
		pContext->payloadEncryptInitialized = 1;
	}
#endif

		if (pContext->idAuthMode == AUTHMODE_EXTEND_MODULE)
		{
			if (pContext->authTokenStr != NULL)
			{
				ctlw_lwm2m_free(pContext->authTokenStr);
				pContext->authTokenStr = NULL;
			}
#if CTIOT_SIMID_ENABLED == 1
			if(pContext->idAuthType == AUTHTYPE_SIMID)
			{
				char *simidstr = ctlw_lwm2m_malloc(CTIOT_MAX_SIMID_LENGTH);
				if (simidstr != NULL)
				{
					int ret;
					ret = ctiot_get_simid_ciphertext(simidstr, CTIOT_MAX_SIMID_LENGTH);
					ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"get simidstr:%s",simidstr);
					if (ret == CTIOT_SIMID_SUCCESS)
					{
						pContext->authTokenStr = simidstr;
					}
					else
					{
						ctlw_lwm2m_free(simidstr);
						result = CTIOT_AUTH_API_ERROR;
						goto exit;
					}
				}
				else
				{
					result = CTIOT_SYS_API_ERROR;
					goto exit;
				}
				ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_reg:get simid success:%s\r\n", pContext->authTokenStr);
			}
#endif
#if CTIOT_SM9_ENABLED == 1
			if(pContext->idAuthType == AUTHTYPE_SM9)
			{
				//目前未实现sm9认证串获取, 暂时返回CTIOT_OTHER_ERROR错误码
				result = CTIOT_OTHER_ERROR;
				goto exit;
			}
#endif

		}


		uint16_t ipStatus = ctiotprv_get_ip_status(pContext->serverIPType, 0);
		if(ipStatus == SDK_IP_STATUS_FALSE)
		{
			ctiot_log_info(LOG_AT_MODULE,LOG_IP_CLASS,"reg_step:SDK_IP_STATUS_FALSE");
			result = CTIOT_IP_NOK_ERROR;
		}
		else if(ipStatus == SDK_IP_STATUS_DISABLE)
		{
			ctiot_log_info(LOG_AT_MODULE,LOG_IP_CLASS,"reg_step:SDK_IP_STATUS_DISABLE");
			result = CTIOT_IP_TYPE_ERROR;
		}
		else if(ipStatus == SDK_IP_STATUS_V6PREPARING)
		{
			ctiot_log_info(LOG_AT_MODULE,LOG_IP_CLASS,"reg_step:SDK_IP_STATUS_V6PREPARING");
			result = CTIOT_IPV6_ONGOING_ERROR;
		}


		if(result != CTIOT_NB_SUCCESS)
		{
			ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_IP_CLASS,"ctlw_reg_step:ctlw_get_local_ip:%u",result);
			goto exit;
		}

		int addressFamily= ctiotprv_switch_address_family_with_ip_status(ipStatus);
		if(addressFamily == 0)
		{
			result = CTIOT_OTHER_ERROR;
			goto exit;
		}

		//根据ip跟踪开关获取会话ip
		ctiotprv_trace_ip_by_workmode(pContext);
		if(pContext->onIP== CTIOT_ONIP_RECONNECT)
		{
			result = ctlw_get_local_ip(pContext->localIP,addressFamily);
			ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_IP_CLASS,"ctiot_reg_step:pContext->onIP== CTIOT_ONIP_RECONNECT,%s",pContext->localIP);
			if(result != CTIOT_NB_SUCCESS)
			{
				goto exit;
			}
			if(ctchip_write_session_ip((uint8_t *)pContext->localIP)!=CTIOT_NB_SUCCESS)//写会话ip到RAM
			{
				result = CTIOT_SYS_API_ERROR;
				goto exit;
			}
		}

		//创建socket
		if (ctiotprv_create_socket(pContext, addressFamily)!= CTIOT_NB_SUCCESS)
		{
			ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_SOCKET_CLASS,"reg socket error...\r\n");
			result = CTIOT_SYS_API_ERROR;
			goto exit;
		}

#if CTIOT_CHIPSUPPORT_DTLS == 1
		if(pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS)
		{
			if(ctlw_dtls_ssl_create(pContext) != 0)
			{
				result = CTIOT_SYS_API_ERROR;
				goto exit;
			}
			ctlw_dtls_update_socket_fd(pContext);
			/*if(ctlw_dtls_shakehand(pContext)!=0)
			{
				result = CTIOT_DTLS_HS_ERROR;
				goto exit;
			}*/
			int ret = ctlw_dtls_shakehand(pContext);
			if(ret != 0)
			{
				ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_SOCKET_CLASS,"dtls shake err=%d\r\n",ret);
				result = CTIOT_DTLS_HS_ERROR;
				goto exit;
			}
			pContext->dtlsStatus = DTLS_OK;
		}
#endif

		ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_session_init\r\n");
		if (pContext->lwm2mContext != NULL)
		{
			ctiotprv_clear_lwm2mContext(pContext);
		}
		result = ctiotprv_lwm2m_init(pContext);
		ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_session_init:result=%d\r\n", result);
		if (result != CTIOT_NB_SUCCESS)
		{
			goto exit;
		}
	}
exit:

	if(result != CTIOT_NB_SUCCESS)
	{
		if(result != CTIOT_SYS_API_ERROR)
		{
			ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"reg fail:%d\r\n",result);
			if(result == CTIOT_IP_NOK_ERROR || result == CTIOT_IP_TYPE_ERROR || result == CTIOT_IPV6_ONGOING_ERROR)
			{
				result = CTIOT_OTHER_ERROR;
			}
			if(ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_OUTING))
			{
				ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_exit_on_error...\r\n");
				ctiot_set_release_flag(RELEASE_MODE_LOGIN_FAIL_2, result);
			}
		}
	}
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_reg_step:%d\r\n",result);
	return result;
}

//发送update数据
uint16_t ctiot_send_update_msg(uint8_t raiIndex, uint16_t msgId, uint8_t updateType) //0:外部  1:内部
{
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();

	lwm2m_transaction_t *transaction = NULL;
	lwm2m_context_t *lwm2mContxt = pContext->lwm2mContext;
	lwm2m_server_t *server = NULL;
	if (lwm2mContxt == NULL)
	{
		result = CTIOT_OTHER_ERROR;
		goto exit;
	}
	if (!ctiotprv_check_location(pContext))
	{
		ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_update_bindmode:ctiotprv_check_location:false\r\n");
		result = CTIOT_OTHER_ERROR;
		goto exit;
	}

#ifdef PLATFORM_XINYI
#if CTIOT_CHIPSUPPORT_DTLS == 1
		//check dtls status
		if(pContext->connectionType == MODE_DTLS && pContext->dtlsStatus == DTLS_NOK)
		{
			pContext->dtlsStatus = DTLS_SWITCHING;
			ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_DTLS_CLASS,"DTLS_SWITCHING;...\r\n");
			int code = ctiotprv_send_dtls_hs();
			if(pContext->atDtlsHsFlag == 1)
			{
				pContext->atDtlsHsFlag = 0;
				ctiot_publish_sdk_notification(NULL,CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_DTLS_HS, result, 0, 0, NULL);
				ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_DTLS_CLASS,"DTLS_OK;...\r\n");
			}

			if(code == CTIOT_DTLS_HS_ERROR)
			{
				pContext->dtlsStatus = DTLS_NOK;
				result = CTIOT_DTLS_NOK_ERROR;
				goto exit;
			}
			else
			{
				pContext->dtlsStatus = DTLS_OK;
			}
		}
#endif
#endif
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiotprv_check_location success...\r\n");
	server = lwm2mContxt->serverList;
	while (server != NULL)
	{
		if (updateType == 0)
		{
			transaction = ctlw_transaction_new(server->sessionH, COAP_POST, NULL, NULL, msgId, 4, NULL);
		}
		else
		{
			transaction = ctlw_transaction_new(server->sessionH, COAP_POST, NULL, NULL, ctlw_lwm2m_get_next_mid(pContext->lwm2mContext), 4, NULL);
			pContext->lastInnerUpdateTransID = transaction->mID;
		}
		if (transaction == NULL)
		{
			ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"malloc transaction error...\r\n");
			result = CTIOT_SYS_API_ERROR;
			goto exit;
		}
		ctlw_coap_set_header_uri_path(transaction->message, server->location);
#ifdef PLATFORM_XINYI
		//发送update时携带lifetime 和binding_mode
		char *query = ctlw_lwm2m_malloc(20);
		memset(query,0x00,20);
		xy_ctlw_get_uri_query(server,query);
		ctlw_coap_set_header_uri_query(transaction->message, query);
		ctlw_lwm2m_free(query);
#endif
		if (updateType == 0)
		{
			transaction->callback = ctiotprv_handle_outer_update_reply; // 外部update回调
		}
		else
		{
			transaction->callback = ctiotprv_handle_inner_update_reply; // 内部update回调
		}
		transaction->userData = (void *)server;
		ctlw_coap_packet_t *messageP = (ctlw_coap_packet_t *)transaction->message;
		if(raiIndex == 0)
		{
			messageP->sendOption = SEND_OPTION_NORMAL;
		}
		else if(raiIndex == 1)
		{
			messageP->sendOption =SEND_OPTION_RAI_DL_FOLLOWED;
		}
		int32_t trResult = ctlw_transaction_send(lwm2mContxt, transaction);
		if (trResult != 0)
		{
			if(trResult == COAP_500_INTERNAL_SERVER_ERROR)
			{
				if(updateType == 1)
				{
					result = CTIOT_SYS_API_ERROR;
				}
				else
				{
					result = CTIOT_SYS_API_ERROR;
				}
				goto exit;
			}
			if(trResult == -2)//ctlw_transaction_send获取系统时间错误
			{
				ctlw_transaction_free(transaction);
				result = CTIOT_SYS_API_ERROR;
				goto exit;
			}
			if(updateType == 1)//内部update
			{
				result = ctiotprv_check_error_of_connect();
				//内部update,socket 发送 buffer 溢出错误和其它错误 处理方式暂定一致
				if(result == CTIOT_OTHER_ERROR || result == CTIOT_SOCKET_SEND_BUFFER_OVERRUN)
				{
					lwm2mContxt->transactionList =(lwm2m_transaction_t *) LWM2M_LIST_ADD(lwm2mContxt->transactionList, transaction);
					ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_OTHER_CLASS,"result = %u,add transactionList",result);
					result = CTIOT_NB_SUCCESS;
				}
				else//DTLS_NOK_ERROR,DTLS_OPER_ERROR,SYS_API_ERROR(源自NOMEM等)
				{
					ctlw_transaction_free(transaction);
				}
			}
			else
			{
				result = ctiotprv_check_error_of_connect();
				ctlw_transaction_free(transaction);
			}
			goto exit;
		}

		lwm2mContxt->transactionList =(lwm2m_transaction_t *) LWM2M_LIST_ADD(lwm2mContxt->transactionList, transaction);
		server = server->next;
	}
exit:
	return result;
}

bool ctiot_send_update_step(void)
{
	bool result = true;
	uint16_t status = CTIOT_NB_SUCCESS;
	uint8_t epsValue;
	ctiot_context_t *pContext = ctiot_get_context();

	ctiot_update_node *pTmp = pContext->updateMsg;

	if (pTmp == NULL || pTmp->msgStatus != QUEUE_SEND_DATA_CACHEING)
	{
		goto exit;
	}


	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_SOCKET_CLASS,"ctiot_send_update_step_start : socket=%d\r\n", pContext->clientInfo.sock);

	epsValue = ctchip_sync_cstate();
	if(epsValue != NETWORK_CONNECTED_HOME_NETWORK && epsValue != NETWORK_CONNECTED_ROAMING)
	{
		status = CTIOT_OTHER_ERROR;
		ctiotprv_change_up_msg_status(pContext, pTmp->msgId, status);
		ctiot_publish_sdk_notification(NULL,CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_UPDATE, status, pTmp->msgId, 0, NULL);
		goto exit;
	}

	status = ctiotprv_check_socket();
	if(status != CTIOT_NB_SUCCESS)
	{
		if(status == CTIOT_SYS_API_ERROR)
		{
			ctiotprv_change_up_msg_status(pContext, pTmp->msgId, status);
			goto exit;
		}
		else //目前场景只有status == CTIOT_IP_NOK_ERROR || status == CTIOT_IP_TYPE_ERROR || status == CTIOT_IPV6_ONGOING_ERROR || status == CTIOT_OTHER_ERROR
		{
			status = CTIOT_OTHER_ERROR;
			ctiotprv_change_up_msg_status(pContext, pTmp->msgId, status);
			ctiot_publish_sdk_notification(NULL,CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_UPDATE, status, pTmp->msgId, 0, NULL);
			goto exit;
		}
	}

#if CTIOT_CHIPSUPPORT_DTLS == 1
	if(pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS)
	{
		status = ctiotprv_check_dtls();
		if(status == CTIOT_DTLS_HS_ERROR)
		{
			goto exit; // DTLS NOK 下  HS 失败 结束此次处理
		}
	}
#endif

	status = ctiot_send_update_msg(pTmp->raiIndex, pTmp->msgId, 0);
	if(status == CTIOT_NB_SUCCESS)
	{
		ctiotprv_change_up_msg_status(pContext, pTmp->msgId, QUEUE_SEND_DATA_SENDOUT);
	}
	else
	{

#if CTIOT_CHIPSUPPORT_DTLS == 1
		if(status == CTIOT_DTLS_NOK_ERROR)//dtls致命错误处理
		{
			if(pContext->clientWorkMode == UQ_WORK_MODE)//UQ模式下，
			{
				ctiotprv_change_up_msg_status(pContext, pTmp->msgId, status);
				ctiot_publish_sdk_notification(NULL,CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_UPDATE, status, pTmp->msgId, 0, NULL);
				ctiot_close_socket();
				ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_DTLS_CLASS, "dtls nok...");
				pContext->dtlsStatus = DTLS_NOK;
			}
			else//U模式下，退出会话
			{
				if (ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_OUTING))
				{
					ctiot_set_release_flag(RELEASE_MODE_DRX_DTLS_SEND_FAIL_L0, dtlssendfail);
				}
			}
			goto exit;
		}
#endif
		ctiotprv_change_up_msg_status(pContext, pTmp->msgId, status);
	}

	if (status != CTIOT_NB_SUCCESS && status != CTIOT_SYS_API_ERROR)//DTLS_OPER_ERROR,OTHER_ERROR,OVERRUN_ERROR
	{
		ctiot_publish_sdk_notification(NULL,CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_UPDATE, status, pTmp->msgId, 0, NULL);
	}
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_SOCKET_CLASS,"ctiot_send_update_step_start : socket=%d\r\n", pContext->clientInfo.sock);

exit:
	if (status == CTIOT_SYS_API_ERROR)
	{
		result = false;
	}

	return result;
}

static uint16_t ctiotprv_send_msg(ctiot_up_msg_node *pMsg, ctiot_session_status_e srcStatus, ctiot_session_status_e destStatus, lwm2m_transaction_callback_t callback)
{
	ctiot_log_debug(LOG_COMMON_MODULE, LOG_OTHER_CLASS, "ctiotprv_send_msg");
	ctiot_context_t *pContext = ctiot_get_context();
	uint16_t code = CTIOT_NB_SUCCESS;

	connection_t *sessionH = NULL;
	void *userData = NULL;
	lwm2m_observed_t *targetP = NULL;

	if (pMsg == NULL )
	{
		return CTIOT_OTHER_ERROR;
	}
	ctlw_coap_packet_t *message = (ctlw_coap_packet_t *)pMsg->node;
	pMsg->node = NULL;

	//1.数据空指针校验保护
	if (message == NULL)
	{
		ctiotprv_change_up_msg_status(pContext, pMsg->msgId, CTIOT_OTHER_ERROR);
		ctiot_publish_sdk_notification((char *)pMsg->uri, CTIOT_APP_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, CTIOT_OTHER_ERROR, pMsg->msgId, 0, NULL);
		return CTIOT_OTHER_ERROR;
	}

	//2.会话状态校验
	if (srcStatus != destStatus)
	{
		ctiotprv_change_up_msg_status(pContext, pMsg->msgId, CTIOT_SESSION_ERROR_BASE + ctiotprv_get_session_status(pContext));
		ctiot_publish_sdk_notification((char *)pMsg->uri,CTIOT_APP_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, CTIOT_SESSION_ERROR_BASE + ctiotprv_get_session_status(pContext), pMsg->msgId, 0, NULL);

		ctlw_lwm2m_free(message->payload);
		ctlw_coap_free_header(message);
		ctlw_lwm2m_free(message);
		return CTIOT_SESSION_ERROR_BASE + ctiotprv_get_session_status(pContext);
	}

	//3.判断EPS
	uint8_t epsValue = ctchip_sync_cstate();
	if(epsValue != NETWORK_CONNECTED_HOME_NETWORK && epsValue != NETWORK_CONNECTED_ROAMING)
	{
		code = CTIOT_OTHER_ERROR;
		ctiotprv_change_up_msg_status(pContext, pMsg->msgId, code);
		ctiot_publish_sdk_notification((char *)pMsg->uri, CTIOT_APP_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, code, pMsg->msgId, 0, NULL);
		ctlw_lwm2m_free(message->payload);
		ctlw_coap_free_header(message);
		ctlw_lwm2m_free(message);
		return CTIOT_OTHER_ERROR;
	}

	//4.判断Socket和IP
	code = ctiotprv_check_socket();
	if(code != CTIOT_NB_SUCCESS)
	{
		if(code == CTIOT_SYS_API_ERROR)
		{
			ctiotprv_change_up_msg_status(pContext, pMsg->msgId, code);
		}
		else //code == CTIOT_IP_NOK_ERROR || code == CTIOT_IP_TYPE_ERROR || code == CTIOT_IPV6_ONGOING_ERROR || code == CTIOT_OTHER_ERROR
		{
			code = CTIOT_OTHER_ERROR;
			ctiotprv_change_up_msg_status(pContext, pMsg->msgId, code);
			ctiot_publish_sdk_notification((char *)pMsg->uri, CTIOT_APP_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, code, pMsg->msgId, 0, NULL);
		}
		ctlw_lwm2m_free(message->payload);
		ctlw_coap_free_header(message);
		ctlw_lwm2m_free(message);
		return code;
	}

#if CTIOT_CHIPSUPPORT_DTLS == 1
	//5.Send前检查DTLS状态
	if(pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS)
	{
		code = ctiotprv_check_dtls();
		if(code == CTIOT_DTLS_HS_ERROR)
		{
			ctlw_lwm2m_free(message->payload);
			ctlw_coap_free_header(message);
			ctlw_lwm2m_free(message);
			return CTIOT_DTLS_HS_ERROR; // DTLS NOK 下	HS 失败 结束此次处理
		}
	}
#endif

#if CTIOT_WITH_PAYLOAD_ENCRYPT == 1
	//6.加密payload处理
	if(pContext->connectionType == MODE_ENCRYPTED_PAYLOAD)
	{
		uint8_t *newPayload = ctlw_lwm2m_malloc(message->payload_len + 244); // 由于加密后数据长度会大于明文长度，所以密文数据缓冲区的长度要在明文数据长度基础上添加244byte的缓冲区长度
		if(newPayload == NULL)
		{
			ctiotprv_change_up_msg_status(pContext, pMsg->msgId, CTIOT_SYS_API_ERROR);
			ctlw_lwm2m_free(message->payload);
			ctlw_coap_free_header(message);
			ctlw_lwm2m_free(message);
			return CTIOT_SYS_API_ERROR;
		}
		int32_t newPayloadLen = 0;
		int32_t encrtpyResult = Sec_dev_Encrypt(message->payload, message->payload_len, newPayload, message->payload_len + 244,  &newPayloadLen);
		ctlw_lwm2m_free(message->payload);
		if(encrtpyResult == 0) //加密成功
		{
			ctiot_log_debug(LOG_COMMON_MODULE, LOG_OTHER_CLASS, "newPayloadLen:%d",newPayloadLen);
			if(newPayloadLen < MAX_SEND_DATA_LEN)
			{
				message->payload = newPayload;
				message->payload_len = newPayloadLen;
			}
			else
			{
				ctiotprv_change_up_msg_status(pContext, pMsg->msgId, CTIOT_ENC_LEN_ERROR);
				ctiot_publish_sdk_notification((char *)pMsg->uri, CTIOT_APP_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, CTIOT_ENC_LEN_ERROR, pMsg->msgId, 0, NULL);
				ctlw_lwm2m_free(newPayload);
				ctlw_coap_free_header(message);
				ctlw_lwm2m_free(message);
				return CTIOT_ENC_LEN_ERROR;
			}
		}
		else
		{
			ctiotprv_change_up_msg_status(pContext, pMsg->msgId, CTIOT_ENC_API_ERROR);
			ctiot_publish_sdk_notification((char *)pMsg->uri, CTIOT_APP_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, CTIOT_ENC_API_ERROR, pMsg->msgId, 0, NULL);
			ctlw_lwm2m_free(newPayload);
			ctlw_coap_free_header(message);
			ctlw_lwm2m_free(message);
			return CTIOT_ENC_API_ERROR;
		}
	}

#endif


	//7.URI校验
	lwm2m_uri_t uriC;
	if (ctlw_lwm2m_stringToUri((char *)pMsg->uri, strlen((char *)pMsg->uri), &uriC) == 0)
	{
		ctiotprv_change_up_msg_status(pContext, pMsg->msgId, CTIOT_OTHER_ERROR);
		ctiot_publish_sdk_notification((char *)pMsg->uri, CTIOT_APP_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, CTIOT_OTHER_ERROR, pMsg->msgId, 0, NULL);
		ctlw_lwm2m_free(message->payload);
		ctlw_coap_free_header(message);
		ctlw_lwm2m_free(message);
		return CTIOT_OTHER_ERROR;
	}

	//8.订阅关系校验
	bool found = false;
	for (targetP = pContext->lwm2mContext->observedList; targetP != NULL; targetP = targetP->next)
	{
		if (!ctiotprv_match_uri(uriC, targetP->uri))
			continue;

		lwm2m_watcher_t *watcherP;
		for (watcherP = targetP->watcherList; watcherP != NULL; watcherP = watcherP->next)
		{
			if (watcherP->active == true)
			{
				watcherP->lastTime = ctlw_lwm2m_gettime();
				watcherP->lastMid = pMsg->msgId;
				ctlw_coap_set_header_token(message, watcherP->token, watcherP->tokenLen);
				ctlw_coap_set_header_observe(message, /*watcherP->counter++*/ 1000);
				sessionH = watcherP->server->sessionH;
				userData = (void *)watcherP->server;
				found = true;
				break;
			}
		}
		if (found)
			break;
	}
	if (!found)
	{
		ctiotprv_change_up_msg_status(pContext, pMsg->msgId, CTIOT_SESSION_ERROR_BASE+ pContext->sessionStatus);
		ctiot_publish_sdk_notification((char *)pMsg->uri, CTIOT_APP_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, CTIOT_SESSION_ERROR_BASE + pContext->sessionStatus, pMsg->msgId, 0, NULL);
		ctlw_lwm2m_free(message->payload);
		ctlw_coap_free_header(message);
		ctlw_lwm2m_free(message);
		return CTIOT_SESSION_ERROR_BASE+pContext->sessionStatus;
	}

	//9.处理报文：包含CON/CON RAI报文和NON/NON RAI报文两个分支,其中Con报文是先发后挂：先发送报文，然后再入transaction队列
	if (pMsg->mode == SENDMODE_CON || pMsg->mode == SENDMODE_CON_REL)//CON，CON RAI报文
	{
		//9.1.1 拼装Con报文的transaction节点
		lwm2m_transaction_t *transaction = NULL;
		transaction = (lwm2m_transaction_t *)ctlw_lwm2m_malloc(sizeof(lwm2m_transaction_t));
		if (transaction == NULL)
		{
			ctiotprv_change_up_msg_status(pContext, pMsg->msgId, CTIOT_SYS_API_ERROR);
			ctlw_lwm2m_free(message->payload);
			ctlw_coap_free_header(message);
			ctlw_lwm2m_free(message);
			return CTIOT_SYS_API_ERROR;
		}
		memset(transaction, 0, sizeof(lwm2m_transaction_t));
		transaction->mID = pMsg->msgId;
		transaction->peerH = sessionH;
		transaction->userData = userData;
		transaction->message = message;
		transaction->callback = (lwm2m_transaction_callback_t)callback;

		//9.1.2 发送Con报文
		uint8_t *payloadPtr = message->payload;
		int32_t trResult = ctlw_transaction_send(pContext->lwm2mContext, transaction);

		//9.1.3 发送结果处理，包含两个分支：发送失败的后检查和发送成功后挂transaction队列；
		if (0 != trResult)
		{
			ctlw_lwm2m_free(payloadPtr);//transaction不自动释放payload,message其它内容在ctlw_transaction_send出500错误时自动释放
			if(trResult == COAP_500_INTERNAL_SERVER_ERROR)
			{
				ctiotprv_change_up_msg_status(pContext, pMsg->msgId, CTIOT_SYS_API_ERROR);
				return CTIOT_SYS_API_ERROR;
			}
			else if(trResult == -2)
			{
				ctlw_transaction_free(transaction);//ctlw_transaction_send返回-2，手动释放transaction上的message
				ctiotprv_change_up_msg_status(pContext, pMsg->msgId, CTIOT_SYS_API_ERROR);
				return CTIOT_SYS_API_ERROR;
			}
			else
			{
				ctlw_transaction_free(transaction);//ctlw_transaction_send返回-1，手动释放transaction上的message
			}

			code = ctiotprv_check_error_of_connect();
#if CTIOT_CHIPSUPPORT_DTLS == 1
			if(code == CTIOT_DTLS_NOK_ERROR)//dtls致命错误处理
			{
				if(pContext->clientWorkMode == UQ_WORK_MODE)//UQ模式下，
				{
					ctiotprv_change_up_msg_status(pContext, pMsg->msgId, code);
					ctiot_publish_sdk_notification((char *)pMsg->uri, CTIOT_APP_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, code, pMsg->msgId, 0, NULL);
					ctiot_close_socket();
					ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_DTLS_CLASS, "dtls nok...");
					pContext->dtlsStatus = DTLS_NOK;
					return code;
				}
				else//U模式下，退出会话
				{
					if (ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_OUTING))
					{
						ctiot_set_release_flag(RELEASE_MODE_DRX_DTLS_SEND_FAIL_L0, dtlssendfail);
					}
					return CTIOT_SESSION_ERROR_BASE + UE_LOGIN_OUTING;
				}

			}
#endif
			ctiotprv_change_up_msg_status(pContext, pMsg->msgId, code);

			if (code != CTIOT_SYS_API_ERROR)//DTLS_OPER_ERROR,OTHER_ERROR,OVERRUN_ERROR
			{
				ctiot_publish_sdk_notification((char *)pMsg->uri, CTIOT_APP_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, code, pMsg->msgId, 0, NULL);
				return code;
			}
			else
			{
				return CTIOT_SYS_API_ERROR;
			}

		} //9.1.3 将发送成功的Con报文放入Transaction队列，进入LwM2M层处理
		else
		{
			ctlw_lwm2m_free(payloadPtr);
			ctlw_transaction_add(pContext->lwm2mContext, transaction);
			ctiotprv_change_up_msg_status(pContext, pMsg->msgId, QUEUE_SEND_DATA_SENDOUT);
			return CTIOT_NB_SUCCESS;
		}
	}
	else //NON，NON RAI报文
	{
		int msgResult = ctlw_message_send(pContext->lwm2mContext, message, sessionH);
		ctlw_lwm2m_free(message->payload);
		ctlw_coap_free_header(message);
		ctlw_lwm2m_free(message);
		if (msgResult == COAP_NO_ERROR)
		{
			ctiotprv_change_up_msg_status(pContext, pMsg->msgId, QUEUE_SEND_SUCCESS);
			ctiot_publish_sdk_notification((char *)pMsg->uri, CTIOT_APP_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, QUEUE_SEND_SUCCESS, pMsg->msgId, 0, NULL);
			return CTIOT_NB_SUCCESS;
		}
		else
		{
			//根据ERRORNO处理
			code = ctiotprv_check_error_of_connect();

#if CTIOT_CHIPSUPPORT_DTLS == 1
			if(code == CTIOT_DTLS_NOK_ERROR)//dtls致命错误处理
			{
				if(pContext->clientWorkMode == UQ_WORK_MODE)//UQ模式下，
				{
					ctiotprv_change_up_msg_status(pContext, pMsg->msgId, code);
					ctiot_publish_sdk_notification((char *)pMsg->uri, CTIOT_APP_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, code, pMsg->msgId, 0, NULL);
					ctiot_close_socket();
					ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_DTLS_CLASS, "dtls nok...");
					pContext->dtlsStatus = DTLS_NOK;
					return code;
				}
				else//U模式下，退出会话
				{
					if (ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_OUTING))
					{
						ctiot_set_release_flag(RELEASE_MODE_DRX_DTLS_SEND_FAIL_L0, dtlssendfail);
					}
					return CTIOT_SESSION_ERROR_BASE + UE_LOGIN_OUTING;
				}

			}
#endif
			ctiotprv_change_up_msg_status(pContext, pMsg->msgId, code);

			if (code != CTIOT_SYS_API_ERROR)//DTLS_OPER_ERROR,OTHER_ERROR,OVERRUN_ERROR
			{
				ctiot_publish_sdk_notification((char *)pMsg->uri, CTIOT_APP_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, code, pMsg->msgId, 0, NULL);
				return code;
			}
			else
			{
				return CTIOT_SYS_API_ERROR;
			}
		}
	}
}
#ifdef CTLW_APP_FUNCTION
bool ctiot_app_send_step(void)
{
	bool result = true;
	uint16_t code = CTIOT_NB_SUCCESS;

	ctiot_context_t *pContext = ctiot_get_context();

	int sendCount = 0;
	while (sendCount < CTIOT_MAX_SEND_COUNT)
	{
		ctiot_up_msg_node *pTmp = (ctiot_up_msg_node *)ctiot_coap_queue_get_msg(pContext->appMsgList, QUEUE_SEND_DATA_CACHEING);
		if (pTmp == NULL)
		{
			break;
		}

		code = ctiotprv_send_msg(pTmp, pContext->sessionStatus, UE_LOGINED, ctiotprv_app_sendcon);

		if(code == CTIOT_SYS_API_ERROR)
		{
			result = false;
			break;
		}
		if(code == CTIOT_SESSION_ERROR_BASE + UE_LOGIN_OUTING)
		{
			break;
		}
		sendCount++;
	}

	return result;
}
#endif
bool ctiot_send_step(void)
{
	bool result = true;
	uint16_t code = CTIOT_NB_SUCCESS;

	ctiot_context_t *pContext = ctiot_get_context();

	int sendCount = 0;
	while (sendCount < CTIOT_MAX_SEND_COUNT)
	{
		ctiot_up_msg_node *pTmp = (ctiot_up_msg_node *)ctiot_coap_queue_get_msg(pContext->upMsgList, QUEUE_SEND_DATA_CACHEING);
		if (pTmp == NULL)
		{
			break;
		}
		code = ctiotprv_send_msg(pTmp, ctiotprv_get_session_status(pContext), UE_LOGINED_OBSERVED, ctiotprv_sendcon);
		if(code == CTIOT_SYS_API_ERROR)
		{
			result = false;
			break;
		}
		if(code == CTIOT_SESSION_ERROR_BASE + UE_LOGIN_OUTING)
		{
			break;
		}
		sendCount ++ ;
	}
	return result;
}

uint16_t ctiot_get_status(uint8_t queryType, uint16_t msgId, uint8_t *buff)
{
	uint16_t res = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();
#if CTIOT_QUERY_TYPE0 == 0
	if (queryType == 0)
	{
		return CTIOT_PARA_NOSUP_ERROR;
	}
#endif
#if CTIOT_QUERY_TYPE1 == 0
	if (queryType == 1)
	{
		return CTIOT_PARA_NOSUP_ERROR;
	}
#endif
#if CTIOT_QUERY_TYPE2 == 0
	if (queryType == 2)
	{
		return CTIOT_PARA_NOSUP_ERROR;
	}
#endif
#if CTIOT_QUERY_TYPE3 == 0
	if (queryType == 3)
	{
		return CTIOT_PARA_NOSUP_ERROR;
	}
#endif
#if CTIOT_QUERY_TYPE4 == 0
	if (queryType == 4)
	{
		return CTIOT_PARA_NOSUP_ERROR;
	}
#endif
#if CTIOT_QUERY_TYPE5 == 0
	if (queryType == 5)
	{
		return CTIOT_PARA_NOSUP_ERROR;
	}
#endif
#if CTIOT_QUERY_TYPE6 == 0
	if (queryType == 6)
	{
		return CTIOT_PARA_NOSUP_ERROR;
	}
#endif
#if CTIOT_QUERY_TYPE7 == 0
		if (queryType == 7)
		{
			return CTIOT_PARA_NOSUP_ERROR;
		}
#endif


	if (buff != NULL)
	{
		uint8_t *header = "+CTLWGETSTATUS";
		switch (queryType)
		{
		case STATUS_TYPE_SESSION:
			sprintf((char *)buff, "\r\n%s: %d,%d\r\n", header, STATUS_TYPE_SESSION, ctiotprv_get_session_status(pContext));
			break;
		case STATUS_TYPE_CONNECT:
			sprintf((char *)buff, "\r\n%s: %d,%d\r\n", header, STATUS_TYPE_CONNECT, ctchip_sync_cstate());
			break;
		case STATUS_TYPE_MSG:
			res = ctiot_get_msg_status(msgId, buff);
			break;
		case STATUS_TYPE_QUEUE_LEN:
			res = ctiot_get_current_queue_len(buff);
			break;
		case STATUS_TYPE_RECV_MSG_LEN:
		{
			ctiot_session_status_e sessionStatus =ctiotprv_get_session_status(pContext);
			if (sessionStatus == UE_LOGINED || sessionStatus == UE_LOGINED_OBSERVED)
			{
				if (pContext->recvDataMode == RECV_DATA_MODE_0)
				{
#if CTIOT_RECEIVE_MODE1 == 1 || CTIOT_RECEIVE_MODE2 == 1
					res = CTIOT_NO_QUEUE_ERROR;
#else
					res = CTIOT_PARA_NOSUP_ERROR;
#endif
				}
				else
				{
					if (pContext->downMsgList != NULL)
					{
						sprintf((char *)buff, "\r\n%s: %d,%u\r\n", header, STATUS_TYPE_RECV_MSG_LEN, pContext->downMsgList->msg_count);
					}
					else
					{
						sprintf((char *)buff, "\r\n%s: %d,%u\r\n", header, STATUS_TYPE_RECV_MSG_LEN, 0 /*CTIOT_MAX_QUEUE_SIZE*/);
					}
				}
			}
			else
			{
				return CTIOT_SESSION_ERROR_BASE +sessionStatus;
			}
			break;
		}
		case STATUS_TYPE_CURRENT_SERVER:
		{
			ctiot_session_status_e sessionStatus = ctiotprv_get_session_status(pContext);
			if (sessionStatus == UE_LOGINED || sessionStatus == UE_LOGINED_OBSERVED)
			{
				if(pContext->socketIPType == 1)
				{
					sprintf((char*)buff,"\r\n%s: %d,%s,%d\r\n",header,STATUS_TYPE_CURRENT_SERVER,pContext->serverIPV4,ctiotprv_get_port(pContext,pContext->portV4));
				}
				else if(pContext->socketIPType == 2)
				{
					sprintf((char*)buff,"\r\n%s: %d,%s,%d\r\n",header,STATUS_TYPE_CURRENT_SERVER,pContext->serverIPV6,ctiotprv_get_port(pContext,pContext->portV6));
				}
				else
				{
					sprintf((char*)buff,"\r\n%s: %d,\r\n",header,STATUS_TYPE_CURRENT_SERVER);
				}
			}
			else
			{
				return CTIOT_SESSION_ERROR_BASE + sessionStatus;
			}
			break;
		}
		case STATUS_TYPE_CURRENT_IP:
		{
			ctiot_session_status_e sessionStatus =ctiotprv_get_session_status(pContext);
			if (sessionStatus == UE_LOGINED || sessionStatus == UE_LOGINED_OBSERVED)
			{
				if(pContext->contextBindMode == CTIOT_BINDMODE_U)
				{
					ctiot_log_debug(LOG_AT_MODULE, LOG_IP_CLASS,"get status: session ip: %s",pContext->localIP);
					sprintf((char*)buff,"\r\n%s: %d,%s\r\n",header,STATUS_TYPE_CURRENT_IP,pContext->localIP);
				}
				else
				{
					char ip_addr[INET6_ADDRSTRLEN]={0};
					int addressFamily = 0;
					if(pContext->socketIPType == 1)
					{
						addressFamily = AF_INET;
					}
					else if(pContext->socketIPType == 2)
					{
						addressFamily = AF_INET6;
					}
					if(ctlw_get_local_ip(ip_addr,addressFamily)==CTIOT_NB_SUCCESS)
					{
						ctiot_log_debug(LOG_AT_MODULE, LOG_IP_CLASS,"get status: chip ip: %s",ip_addr);
						sprintf((char*)buff,"\r\n%s: %d,%s\r\n",header,STATUS_TYPE_CURRENT_IP,ip_addr);
					}
					else
					{
						sprintf((char*)buff,"\r\n%s: %d,\r\n",header,STATUS_TYPE_CURRENT_IP);
					}
				}
			}
			else
			{
				return CTIOT_SESSION_ERROR_BASE + sessionStatus;
			}
			break;
		}
		case STATUS_TYPE_DTLS_STATUS:
		{
			ctiot_session_status_e sessionStatus =ctiotprv_get_session_status(pContext);
			if (sessionStatus == UE_LOGINED || sessionStatus == UE_LOGINED_OBSERVED)
			{
				ctiot_log_debug(LOG_AT_MODULE, LOG_SESSTATUS_CLASS,"current login status:%u\r\n",pContext->sessionStatus);
				#if CTIOT_CHIPSUPPORT_DTLS == 1
				if(pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS)
				{
					sprintf((char *)buff, "\r\n%s: %d,%d\r\n", header, STATUS_TYPE_DTLS_STATUS, pContext->dtlsStatus);
				}
				else
				{
					return CTIOT_NO_DTLS_ERROR;
				}
				#else
				return CTIOT_NO_DTLS_ERROR;
				#endif
			}
			else
			{
				return CTIOT_SESSION_ERROR_BASE + sessionStatus;
			}
			break;
		}
		default:
			return CTIOT_PARA_VALUE_ERROR;
		}
	}
	else
	{
		return CTIOT_OTHER_ERROR;
	}

	return res;
}
uint16_t ctiot_get_msg_status(uint16_t msgId, uint8_t *buff)//调用时，保证buff不为空，且长度足够
{
	uint16_t ret = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();
	uint8_t *header = "+CTLWGETSTATUS";
	ctiot_session_status_e sessionStatus =ctiotprv_get_session_status(pContext);
	if (sessionStatus == UE_LOGINED || sessionStatus == UE_LOGINED_OBSERVED)
	{
		uint16_t result = QUEUE_SEND_DATA_MSGID_ERROR;
		if (pContext->updateMsg != NULL && msgId == pContext->updateMsg->msgId)
		{
			result = pContext->updateMsg->msgStatus;
		}
		else
		{
			ctiot_list_t *node = ctiot_coap_queue_find(pContext->upMsgList, msgId);
			if (node != NULL)
			{
				result = node->msgStatus;
			}
		}
		if (buff != NULL)
		{
			sprintf((char *)buff, "\r\n%s: %d,%u\r\n", header, STATUS_TYPE_MSG, result);
		}
		else
		{
			ret = CTIOT_OTHER_ERROR;
		}
	}
	else
	{
		ret = CTIOT_SESSION_ERROR_BASE + sessionStatus;
	}

	return ret;
}

uint16_t ctiot_get_current_queue_len(uint8_t *buff)//调用时，保证buff不为空，且长度足够
{
	uint16_t ret = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();

	uint8_t *header = "+CTLWGETSTATUS";

	ctiot_session_status_e sessionStatus =ctiotprv_get_session_status(pContext);
	if (sessionStatus == UE_LOGINED || sessionStatus == UE_LOGINED_OBSERVED)
	{
		uint16_t len = ctiot_get_available_len(pContext->upMsgList);
		if (buff != NULL)
		{
			sprintf((char *)buff, "\r\n%s: %d,%u\r\n", header, STATUS_TYPE_QUEUE_LEN, len);
		}
		else
		{
			ret = CTIOT_OTHER_ERROR;
		}
	}
	else
	{
		ret = CTIOT_SESSION_ERROR_BASE + sessionStatus;
	}
	return ret;
}

uint16_t ctiot_at_decode(char *data, int datalen,uint8_t** retBuf)
{
	int bufferlen = datalen / 2;
	(*retBuf) = ctlw_lwm2m_malloc(bufferlen + 1);
	if ((*retBuf )!= NULL)
	{
		memset((*retBuf), 0, bufferlen + 1);
		int i = 0;
		char tmpdata;
		uint8_t tmpValue = 0;
		while (i < datalen)
		{
			tmpdata = data[i];
			if (tmpdata <= '9' && tmpdata >= '0')
			{
				tmpValue = tmpdata - '0';
			}
			else if (tmpdata <= 'F' && tmpdata >= 'A')
			{
				tmpValue = tmpdata - 'A' + 10;
			}
			else if (tmpdata <= 'f' && tmpdata >= 'a')
			{
				tmpValue = tmpdata - 'a' + 10;
			}
			else
			{
				ctlw_lwm2m_free((*retBuf));
				return CTIOT_PARA_VALUE_ERROR;
			}
			if (i % 2 == 0)
			{
				(*retBuf)[i / 2] = tmpValue << 4;
			}
			else
			{
				(*retBuf)[i / 2] |= tmpValue;
			}
			i++;
			//重复代码
			tmpdata = data[i];
			if (tmpdata <= '9' && tmpdata >= '0')
			{
				tmpValue = tmpdata - '0';
			}
			else if (tmpdata <= 'F' && tmpdata >= 'A')
			{
				tmpValue = tmpdata - 'A' + 10;
			}
			else if (tmpdata <= 'f' && tmpdata >= 'a')
			{
				tmpValue = tmpdata - 'a' + 10;
			}
			else
			{
				ctlw_lwm2m_free((*retBuf));
				return CTIOT_PARA_VALUE_ERROR;
			}
			if (i % 2 == 0)
			{
				(*retBuf)[i / 2] = tmpValue << 4;
			}
			else
			{
				(*retBuf)[i / 2] |= tmpValue;
			}
			i++;
		}
	}
	else
	{
		return CTIOT_SYS_API_ERROR;//开辟内存失败
	}
	return CTIOT_NB_SUCCESS;
}

char *ctiot_at_encode(uint8_t *data, int datalen)
{
	int bufferlen = datalen * 2;
	char *buffer = ctlw_lwm2m_malloc(bufferlen + 1);
	if (buffer == NULL)
		return NULL;
	memset(buffer, 0, bufferlen + 1);
	int i = 0;
	while (i < datalen)
	{
		uint8_t tmpValue = data[i] >> 4;
		if (tmpValue < 10)
		{
			buffer[2 * i] = tmpValue + '0';
		}
		else
		{
			buffer[2 * i] = tmpValue + 'A' - 10;
		}
		tmpValue = data[i] & 0x0f;
		if (tmpValue < 10)
		{
			buffer[2 * i + 1] = tmpValue + '0';
		}
		else
		{
			buffer[2 * i + 1] = tmpValue + 'A' - 10;
		}
		i++;
	}
	return buffer;
}

static uint16_t ctiotprv_prepare_up_msg(ctiot_context_t *pContext, ctiot_data_list *dataP, uint8_t *uri, ctiot_send_mode_e sendMode, ctiot_send_format_e sendFormat, uint16_t *msgId, ctlw_coap_packet_t **message)
{
	uint16_t result = CTIOT_NB_SUCCESS;

	(*msgId) = ctlw_lwm2m_get_next_mid(pContext->lwm2mContext);
	(*message) = ctlw_lwm2m_malloc(sizeof(ctlw_coap_packet_t));

	lwm2m_uri_t uriC;
	lwm2m_media_type_t mediaType = LWM2M_CONTENT_OPAQUE;
	if (ctlw_lwm2m_stringToUri((char *)uri, strlen((char *)uri), &uriC) == 0)
	{
		result = CTIOT_OTHER_ERROR;
		goto exit;
	}

	if (sendMode == SENDMODE_CON || sendMode == SENDMODE_CON_REL)
	{
		ctlw_coap_init_message((*message), COAP_TYPE_CON, COAP_205_CONTENT, (*msgId));
	}
	else
	{
		ctlw_coap_init_message((*message), COAP_TYPE_NON, COAP_205_CONTENT, (*msgId));
	}

	mediaType = ctiotprv_get_media_type(sendFormat);

	ctlw_coap_set_header_content_type((*message), mediaType);
	ctiotprv_set_message_send_option((*message), sendMode);
	ctiotprv_set_uri_option((*message), &uriC);
	ctiotprv_set_message_payload((*message), &uriC, sendFormat, dataP, &mediaType);

exit:
	return result;
}

int16_t ctiot_lwm2m_obj_notify_data_serialize(char* uri, uint16_t size, lwm2m_data_t * dataP, ctiot_send_format_e sendFormat, char** buff)
{
	int16_t result = -1;
	lwm2m_uri_t uriP[1] = {0};

	lwm2m_media_type_t formatP = ctiotprv_get_media_type(sendFormat);

	if(uri == NULL || ctlw_lwm2m_stringToUri(uri, strlen(uri), uriP) == 0)
	{
		return -1;
	}

	result = ctlw_lwm2m_data_serialize(uriP, size, dataP, &formatP, (uint8_t **)buff);

	return result;
}

uint16_t ctiot_lwm2m_obj_notify(char *data, uint16_t dataLen, ctiot_send_mode_e sendMode, ctiot_send_format_e sendFormat, char* uri, uint16_t *msgID)
{
	ctiot_context_t *pContext = ctiot_get_context();
	uint16_t result = CTIOT_NB_SUCCESS;
	lwm2m_uri_t uriP[1] = {0};
	bool isUri1900=false;
	ctiot_session_status_e sessionStatus;
	if(dataLen == 0)
	{
		result = CTIOT_PARA_NUM_ERROR; // 参数数量错误
		goto exit;
	}

	if(uri == NULL || ctlw_lwm2m_stringToUri(uri, strlen(uri), uriP) == 0)
	{
		return CTIOT_OTHER_ERROR;
	}

	if (sendMode > SENDMODE_CON_REL)
	{
		result = CTIOT_PARA_VALUE_ERROR; // 参数值错误
		goto exit;
	}
	if(uriP->objectId == 19
		&& (LWM2M_URI_IS_SET_INSTANCE(uriP) && uriP->instanceId == 0)
		&&(LWM2M_URI_IS_SET_RESOURCE(uriP) && uriP->resourceId == 0))
	{
		isUri1900 = true;
		if (dataLen % 2 != 0)
		{
			result = CTIOT_DATA_VAL_ERROR; //	Data字段长度不是偶数
			goto exit;
		}
		dataLen = dataLen / 2;
	}
	if (dataLen > MAX_SEND_DATA_LEN)
	{
		result = CTIOT_DATA_LEN_ERROR; //14 Data字段长度超过上限
		goto exit;
	}

#if CTIOT_CHIPSUPPORT_RAI == 0
	if (sendMode == SENDMODE_NON_REL || sendMode == SENDMODE_NON_RECV_REL || sendMode == SENDMODE_CON_REL)
	{
		result = CTIOT_PARA_NOSUP_ERROR; //参数指示的模式不支持（mode值在有效范围内，但是当前模块不支持该模式）
		goto exit;
	}
#endif
	sessionStatus = pContext->sessionStatus;
	if (sessionStatus == UE_LOGINED)
	{
		if(ctlw_observe_findByUri(pContext->lwm2mContext,uriP) == NULL)
		{
			if(isUri1900)
			{
				result = CTIOT_SESSION_ERROR_BASE + UE_LOGINED;
			}
			else
			{
				result = CTIOT_SESSION_ERROR_BASE + UE_LOGINED; //app object未observe
			}

			goto exit;
		}

#if CTIOT_CHIPSUPPORT_DTLS == 1
		if(pContext->connectionType == MODE_DTLS || pContext->connectionType ==  MODE_DTLS_PLUS)
		{
			if(pContext->dtlsStatus == DTLS_SWITCHING)
			{
				result = CTIOT_DTLS_SWITCH_ERROR;
				goto exit;
			}
		}
#endif

		ctlw_coap_packet_t *message = NULL;
		uint8_t *msgData = NULL;

		result = ctiotprv_check_ip_and_eps(0);
		if (result != CTIOT_NB_SUCCESS)
		{

			goto exit;
		}

#if defined(CTLW_APP_FUNCTION) || defined(PLATFORM_XINYI)
		if(isUri1900)
		{
#endif 
			//普通报文是否超限
			if ( pContext->upMsgList != NULL && ctiot_get_available_len(pContext->upMsgList) == 0)
			{
				result = CTIOT_OVERRUN_ERROR;
				goto exit;
			}
			result = ctiot_at_decode(data, dataLen * 2 , &msgData);
			if (result !=CTIOT_NB_SUCCESS)
			{
				goto exit;
			}
#if defined(CTLW_APP_FUNCTION) || defined(PLATFORM_XINYI)
		}
		else//app报文
		{
#ifdef PLATFORM_XINYI
			if(pContext->upMsgList != NULL && ctiot_get_available_len(pContext->upMsgList) == 0)
#else
			if (pContext->appMsgList != NULL && ctiot_get_available_len(pContext->appMsgList) == 0)
#endif	
			{
				result = CTIOT_OVERRUN_ERROR;
				goto exit;
			}

			msgData = ctlw_lwm2m_malloc(dataLen);
			if(msgData == NULL)
			{
				result = CTIOT_SYS_API_ERROR;
				goto exit;
			}
			memset(msgData, 0, dataLen);
			memcpy(msgData, data, dataLen);
		}
#endif
		ctiot_up_msg_node *updataNode = NULL;
		ctiot_up_msg_node *removeNode = NULL;
		updataNode = (ctiot_up_msg_node *)ctlw_lwm2m_malloc(sizeof(ctiot_up_msg_node));
		if (updataNode == NULL)
		{
			ctlw_lwm2m_free(msgData);
			result = CTIOT_SYS_API_ERROR;
			goto exit;
		}
		updataNode->uri =(uint8_t *)ctlw_lwm2m_strdup(uri);
		if(updataNode->uri == NULL)
		{
			ctlw_lwm2m_free(msgData);
			ctlw_lwm2m_free(updataNode);
			result = CTIOT_SYS_API_ERROR;
			goto exit;
		}

		ctiot_data_list *dataList;
		dataList = (ctiot_data_list *)ctlw_lwm2m_malloc(sizeof(ctiot_data_list));
		if (dataList == NULL)
		{
			ctlw_lwm2m_free(msgData);
			ctlw_lwm2m_free(updataNode->uri);
			ctlw_lwm2m_free(updataNode);
			result = CTIOT_SYS_API_ERROR;
			goto exit;
		}
		dataList->next = NULL;
		dataList->dataType = DATA_TYPE_OPAQUE;//暂时无效
		dataList->u.asBuffer.length = dataLen;
		dataList->u.asBuffer.buffer = msgData;

		result = ctiotprv_prepare_up_msg(pContext, dataList, (uint8_t *)uri, sendMode, sendFormat, msgID, &message);
		ctlw_lwm2m_free(dataList->u.asBuffer.buffer);
		ctlw_lwm2m_free(dataList);
		if (result != CTIOT_NB_SUCCESS)
		{
			ctlw_lwm2m_free(updataNode->uri);
			ctlw_lwm2m_free(updataNode);
			result = CTIOT_OTHER_ERROR;
			goto exit;
		}

		updataNode->msgId = *msgID;
		updataNode->msgStatus = QUEUE_SEND_DATA_CACHEING;
		updataNode->node = message;
		updataNode->mode = sendMode;
		updataNode->sendFormat = sendFormat;


		int isSuccess = CTIOT_OTHER_ERROR;

		ctiot_session_status_mutex_lock();
		ctiot_session_status_e tmpSessionStatus = ctiotprv_get_session_status(pContext);
#ifdef CTLW_APP_FUNCTION
		if(isUri1900)
		{
#endif
			if(tmpSessionStatus == UE_LOGINED_OBSERVED)
			{
				ctiot_log_debug(LOG_AT_MODULE, LOG_OTHER_CLASS,"msg add to mcu msg queue");
				isSuccess = ctiot_coap_queue_add_msg(pContext->upMsgList, (ctiot_list_t *)updataNode, &removeNode);
			}
			else
			{
				ctlw_lwm2m_free(message->payload);
				ctlw_coap_free_header(message);
				ctlw_lwm2m_free(message);
				ctlw_lwm2m_free(updataNode->uri);
				ctlw_lwm2m_free(updataNode);
				ctiot_session_status_mutex_unlock();
				result = CTIOT_SESSION_ERROR_BASE + tmpSessionStatus;
				goto exit;
			}
#ifdef CTLW_APP_FUNCTION
		}
		else
		{
			if(tmpSessionStatus == UE_LOGINED_OBSERVED || tmpSessionStatus == UE_LOGINED)
			{
				ctiot_log_debug(LOG_AT_MODULE, LOG_OTHER_CLASS,"msg add to app msg queue");
				isSuccess = ctiot_coap_queue_add_msg(pContext->appMsgList, (ctiot_list_t *)updataNode, &removeNode);
			}
			else
			{
				ctlw_lwm2m_free(message->payload);
				ctlw_coap_free_header(message);
				ctlw_lwm2m_free(message);
				ctlw_lwm2m_free(updataNode->uri);
				ctlw_lwm2m_free(updataNode);
				ctiot_session_status_mutex_unlock();
				result = CTIOT_SESSION_ERROR_BASE + tmpSessionStatus;
				goto exit;
			}
		}
#endif
		ctiot_session_status_mutex_unlock();

		if (removeNode != NULL)
		{
			ctlw_lwm2m_free(removeNode->uri);
			ctlw_lwm2m_free(removeNode);
		}

		if (isSuccess != CTIOT_NB_SUCCESS)
		{
			result = CTIOT_OVERRUN_ERROR;
			ctlw_lwm2m_free(message->payload);
			ctlw_coap_free_header(message);
			ctlw_lwm2m_free(message);
			ctlw_lwm2m_free(updataNode->uri);
			ctlw_lwm2m_free(updataNode);
		}
		else
		{
			ctiot_log_debug(LOG_AT_MODULE, LOG_VOTE_CLASS,"ctiot_send ctchip_get_send_recv_slp_handler() SYSTEM_STATUS_BUSY");
			ctiot_vote_recv_send_busy() ;
		}

	}
	else
	{
		result = CTIOT_SESSION_ERROR_BASE +sessionStatus;
	}

exit:
	ctiot_log_debug(LOG_AT_MODULE, LOG_OTHER_CLASS,"ctiot_send result=%d\r\n", result);
	if(result == CTIOT_SYS_API_ERROR)
	{
		ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_INITIALIZE_FAILED);
		ctiot_set_release_flag(RELEASE_MODE_L1_WITHOUT_NOTIFY, 0);
	}
	return result;
}

uint16_t ctiot_send(char *data, ctiot_send_mode_e sendMode, uint8_t *buff)
{
	uint16_t result = CTIOT_NB_SUCCESS;
	uint16_t msgID = 0;
	int datalen = strlen(data);

	result = ctiot_lwm2m_obj_notify(data, datalen, sendMode, DATA_FORMAT_OPAQUE, "/19/0/0", &msgID);
/*#ifdef PLATFORM_XINYI
	xy_sendAndrecv_sem_give();
#endif*/ // no use select Thread

	if (result == CTIOT_NB_SUCCESS)
	{
		uint8_t *header = "+CTLWSEND";
		sprintf((char *)buff, "\r\n%s: %u\r\n", header, msgID);
	}
	return result;
}

void ctiot_notify_nb_info(CTIOT_NB_ERRORS errInfo, ctiot_at_to_mcu_type_e infoType, void *params, uint16_t paramLen)
{
	char *at_str = NULL;
	switch (infoType)
	{
	case AT_TO_MCU_RECEIVE:
	{
		uint16_t len = 0;
		ctiot_context_t *pContext = ctiot_get_context();
		if (params == NULL) //mcu get模式
		{
			at_str = ctlw_lwm2m_strdup("\r\n+CTLWRECV\r\n");
		}
		else
		{
			if (paramLen / 2 > MAX_RECV_DATA_LEN) //数据长度判断
			{
				return;
			}
			char *tmpbuf = params;

			if (pContext->recvDataMode == RECV_DATA_MODE_0) //直接通知消息模式
			{
				len = strlen("\r\n+CTLWRECV: ") + paramLen + 10;

				ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"at:%s\r\n", tmpbuf);
				if (tmpbuf != NULL)
				{
					at_str = ctlw_lwm2m_malloc(len);
					memset(at_str, 0, len);
					if (at_str != NULL)
					{
						sprintf((char *)at_str, "\r\n+CTLWRECV: 0,%s\r\n", tmpbuf);
					}
				}
			}
		}

		break;
	}

	case AT_TO_MCU_STATUS:
	{
		ctiot_status_t *pTmpStatus = (ctiot_status_t *)params;
		uint16_t len = 40;
		if (pTmpStatus->extraInfo != NULL)
		{
			if (pTmpStatus->extraInfoLen == 0)
				len += pTmpStatus->extraInfoLen + 1;
			else
				len += 6; //uint16_t msgId 最大值65536
		}
		at_str = ctlw_lwm2m_malloc(len);
		memset(at_str, 0, len);
		if (at_str != NULL)
		{
			if (pTmpStatus->extraInfo == NULL)
			{
				sprintf((char *)at_str, "\r\n+CTLW: %s\r\n", pTmpStatus->baseInfo);
			}
			else
			{
				if (pTmpStatus->extraInfoLen != 0)
				{
					sprintf((char *)at_str, "\r\n+CTLW: %s,%s\r\n", pTmpStatus->baseInfo, (char *)pTmpStatus->extraInfo);
				}
				else
				{
					uint16_t *tmpvalue = (uint16_t *)(pTmpStatus->extraInfo);
					sprintf((char *)at_str, "\r\n+CTLW: %s,%u\r\n", pTmpStatus->baseInfo, *tmpvalue);
				}
			}
		}
		break;
	}
	case AT_TO_MCU_COMMAND:
	{
		ctiot_object_operation_t *pTmpCommand = (ctiot_object_operation_t *)params;
		uint16_t len = strlen("+CTLWCMD:") + pTmpCommand->tokenLen * 2 + strlen((char *)pTmpCommand->uri) + 50;
		char *token = ctiot_at_encode(pTmpCommand->token, pTmpCommand->tokenLen);
		if (pTmpCommand->cmdType == CMD_TYPE_OBSERVE)
		{
			len += 2;
			at_str = ctlw_lwm2m_malloc(len);
			memset(at_str, 0, len);
			sprintf((char *)at_str, "+CTLWCMD:%d,%d,%s,%d,%s\r\n", pTmpCommand->msgId, pTmpCommand->cmdType, pTmpCommand->uri, pTmpCommand->observe, token);
		}
		else if (pTmpCommand->cmdType == CMD_TYPE_READ || pTmpCommand->cmdType == CMD_TYPE_DISCOVER || pTmpCommand->cmdType == CMD_TYPE_DELETE)
		{

			at_str = ctlw_lwm2m_malloc(len);
			memset(at_str, 0, len);
			sprintf((char *)at_str, "+CTLWCMD:%d,%d,%s\r\n", pTmpCommand->msgId, pTmpCommand->cmdType, pTmpCommand->uri);
		}
		else if (pTmpCommand->cmdType == CMD_TYPE_WRITE || pTmpCommand->cmdType == CMD_TYPE_WRITE_PARTIAL || pTmpCommand->cmdType == CMD_TYPE_WRITE_ATTRIBUTE || pTmpCommand->cmdType == CMD_TYPE_EXECUTE || pTmpCommand->cmdType == CMD_TYPE_CREATE)
		{
			ctiot_send_format_e dataFormat = ctiotprv_get_send_format_type(pTmpCommand->dataFormat);
			if (pTmpCommand->dataLen == 0)
			{
				len += 4;
				at_str = ctlw_lwm2m_malloc(len);
				sprintf((char *)at_str, "+CTLWCMD:%d,%d,%s\r\n", pTmpCommand->msgId, pTmpCommand->cmdType, pTmpCommand->uri);
			}
			else
			{
				len += pTmpCommand->dataLen * 2 + 4;
				at_str = ctlw_lwm2m_malloc(len);
				memset(at_str, 0, len);
				if (dataFormat == DATA_FORMAT_TLV || dataFormat == DATA_FORMAT_OPAQUE)
				{
					char *payload = ctiot_at_encode(pTmpCommand->data, pTmpCommand->dataLen);
					sprintf((char *)at_str, "+CTLWCMD:%d,%d,%s,%d,%s\r\n", pTmpCommand->msgId, pTmpCommand->cmdType, pTmpCommand->uri, dataFormat, payload);
					ctlw_lwm2m_free(payload);
				}
				else
				{
					char *payload = ctlw_lwm2m_malloc(pTmpCommand->dataLen + 1);
					memset(payload, 0x00, pTmpCommand->dataLen + 1);
					memcpy(payload, pTmpCommand->data, pTmpCommand->dataLen);
					sprintf((char *)at_str, "+CTLWCMD:%d,%d,%s,%d,%s\r\n", pTmpCommand->msgId, pTmpCommand->cmdType, pTmpCommand->uri, dataFormat, payload);
					ctlw_lwm2m_free(payload);
				}
			}
		}
		break;
	}
	case AT_TO_MCU_QUERY_STATUS:
	{
		uint16_t len = strlen("+CTLWTGETSTATUS:") + 50;
		ctiot_query_status_t *pTmpQueryStatus = (ctiot_query_status_t *)params;

		at_str = ctlw_lwm2m_malloc(len);
		memset(at_str, 0, len);
		if (pTmpQueryStatus->queryType == STATUS_TYPE_MSG)
		{
			sprintf((char *)at_str, "+CTLWTGETSTATUS:%d,%d,%llu\r\n", pTmpQueryStatus->queryType, pTmpQueryStatus->queryResult, pTmpQueryStatus->u.extraInt);
		}
		else if (pTmpQueryStatus->queryType == STATUS_TYPE_CONNECT && pTmpQueryStatus->queryResult == QUERY_STATUS_CONNECTION_AVAILABLE)
		{
			sprintf((char *)at_str, "+CTLWTGETSTATUS:%d,%d,%llu\r\n", pTmpQueryStatus->queryType, pTmpQueryStatus->queryResult, pTmpQueryStatus->u.extraInt);
		}
		else
		{
			sprintf((char *)at_str, "+CTLWTGETSTATUS:%d,%d\r\n", pTmpQueryStatus->queryType, pTmpQueryStatus->queryResult);
		}
		break;
	}
	case AT_TO_MCU_SENDSTATUS:
	{
		ctiot_status_t *pTmpStatus = (ctiot_status_t *)params;
		uint16_t len = 50;
		if (pTmpStatus->extraInfo != NULL)
		{
			if (pTmpStatus->extraInfoLen == 0)
				len += pTmpStatus->extraInfoLen + 1;
			else
				len += 6; //uint16_t msgid 最大值65536
		}
		at_str = ctlw_lwm2m_malloc(len);
		memset(at_str, 0, len);
		if (at_str != NULL)
		{
			if (pTmpStatus->extraInfo == NULL)
			{
				sprintf((char *)at_str, "+CTLWSEND:\r\n");
			}
			else
			{
				if (pTmpStatus->extraInfoLen != 0)
				{
					sprintf((char *)at_str, "+CTLWSEND:%s\r\n", (char *)pTmpStatus->extraInfo);
				}
				else
				{
					uint16_t *tmpvalue = (uint16_t *)(pTmpStatus->extraInfo);
					sprintf((char *)at_str, "+CTLWSEND:%u\r\n", *tmpvalue);
				}
			}
			break;
		}
	}
	case AT_TO_MCU_SENDERROR:
	{
		int len = 30;
		ctiot_status_t *pStatus = (ctiot_status_t *)params;
		if (pStatus != NULL)
		{
			if (pStatus->baseInfo != NULL)
			{
				len += strlen(pStatus->baseInfo) + 10;
			}
		}
		at_str = ctlw_lwm2m_malloc(len);
		memset(at_str, 0, len);
		if (errInfo == CTIOT_NB_SUCCESS)
		{
			if (pStatus == NULL)
			{
				sprintf((char *)at_str, "OK\r\n");
			}
			else
			{
				uint16_t *msgId = (uint16_t *)pStatus->extraInfo;
				if (msgId != NULL)
				{
					sprintf((char *)at_str, "%s:%u\r\nOK\r\n", pStatus->baseInfo, *msgId);
				}
				else
				{
					sprintf((char *)at_str, "%s\r\nOK\r\n", pStatus->baseInfo);
				}
			}
		}
		else
		{
			if (pStatus == NULL)
			{
				sprintf((char *)at_str, "+CTLW ERROR:%d\r\n", errInfo);
			}
			else
			{

				uint16_t *msgId = (uint16_t *)pStatus->extraInfo;
				if (msgId != NULL)
				{
					sprintf((char *)at_str, "+CTLW ERROR:%d,%d\r\n", errInfo, *msgId);
				}
				else
				{
					sprintf((char *)at_str, "+CTLW ERROR:%d\r\n", errInfo);
				}
			}
		}
		break;
	}
	case AT_TO_MCU_QUERY_PARAM:
	{
		ctiot_status_t *pStatus = (ctiot_status_t *)params;
		if (pStatus->baseInfo == NULL || pStatus->extraInfo == NULL || pStatus->extraInfoLen == 0)
		{
			break;
		}
		uint32_t len = strlen(pStatus->baseInfo) + pStatus->extraInfoLen + 10;
		at_str = ctlw_lwm2m_malloc(len);
		if (at_str != NULL)
		{
			sprintf((char *)at_str, "%s:%s", pStatus->baseInfo, (char *)pStatus->extraInfo);
		}
	}
	default:
		break;
	}
	if (at_str != NULL)
	{
		ctchip_asyn_notify((char *)at_str);
		ctlw_lwm2m_free(at_str);
	}
}

void ctiot_sdk_notification(uint8_t notifyType, uint8_t subType, uint16_t value, uint32_t data1, uint32_t data2, void *reservedData)
{

#ifdef PLATFORM_XINYI
	ctiot_context_t *pContext = ctiot_get_context();
	if(pContext->abstractCloudFlag == 1)//当前业务由抽象云AT触发,屏蔽URC,跳过URC上报处理
		goto next;
#endif

	if (subType == CTIOT_NOTIFY_SUBTYPE_LSTATUS && value >= 4)
	{
		return;
	}
	switch (notifyType)
	{
	case CTIOT_NOTIFY_ASYNC_NOTICE:
	{
		char *prefix = NULL;
		bool prefixSuccess = false;
		switch (subType)
		{
		case CTIOT_NOTIFY_SUBTYPE_REG:
		{
			prefix = "reg";
			prefixSuccess = true;
			break;
		}
		case CTIOT_NOTIFY_SUBTYPE_LWEVENT:
		{
			prefix = "lwevent";
			prefixSuccess = true;
			break;
		}
		case CTIOT_NOTIFY_SUBTYPE_UPDATE:
		{
			prefix = "update";
			prefixSuccess = true;
			break;
		}
		case CTIOT_NOTIFY_SUBTYPE_DEREG:
		{
			prefix = "dereg";
			prefixSuccess = true;
			break;
		}
		case CTIOT_NOTIFY_SUBTYPE_SEND:
		{
			prefix = "send";
			prefixSuccess = true;
			break;
		}
		case CTIOT_NOTIFY_SUBTYPE_LSTATUS:
		{
			prefix = "lwstatus";
			prefixSuccess = true;
			break;
		}
		case CTIOT_NOTIFY_SUBTYPE_DTLS_HS:
		{
			prefix = "dtlshs";
			prefixSuccess = true;
			break;
		}
		default:
		{
			break;
		}
		}
		if (prefixSuccess == false)
		{
			break;
		}
		ctiot_status_t atStatus[1] = {0};
		char baseInfo[20] = {0};
		if(subType == CTIOT_NOTIFY_SUBTYPE_REG ||subType == CTIOT_NOTIFY_SUBTYPE_DTLS_HS )
		{
			sprintf(baseInfo, "%s,%u,0", prefix, value);
		}
		else
		{
			sprintf(baseInfo, "%s,%u", prefix, value);
		}
		atStatus->baseInfo = baseInfo;
		if (subType == CTIOT_NOTIFY_SUBTYPE_UPDATE || subType == CTIOT_NOTIFY_SUBTYPE_SEND || subType == CTIOT_NOTIFY_SUBTYPE_LWEVENT || subType == CTIOT_NOTIFY_SUBTYPE_LSTATUS )
		{
			atStatus->extraInfo = (void *)&data1;
		}
		ctiot_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, (void *)atStatus, 0);
		break;
	}
	case CTIOT_SYS_API_ERROR:
	{
		ctiot_notify_nb_info((CTIOT_NB_ERRORS)value, AT_TO_MCU_SENDERROR, 0, 0);
		break;
	}
	case CTIOT_NOTIFY_RECV_DATA:
	{
		ctiot_context_t *pContext = ctiot_get_context();

		if (pContext->sessionStatus == UE_LOGIN_OUTING)
		{
			return;
		}

		if (subType == 0) //直接通知
		{
			ctiot_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_RECEIVE, reservedData, strlen(reservedData));
		}
		else if (subType == 1) //空通知
		{
			ctiot_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_RECEIVE, NULL, 0);
		}

		break;
	}
	default:
		break;
	}
#ifdef PLATFORM_XINYI
next:	
	xy_ctlw_procedure_with_notify(notifyType, subType, value, data1);
	ctlw_notify_api_event_process(notifyType, subType, value, data1);
#endif
}
uint16_t ctiot_get_reg_status( uint8_t *buff)
{
	uint16_t ret = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();

	uint8_t *header = "+CTLWREG";

	if (buff != NULL)
	{
		sprintf((char *)buff, "\r\n%s: %d\r\n", header, ctiotprv_get_session_status(pContext));
	}
	else
	{
		return CTIOT_OTHER_ERROR;
	}

	return ret;
}

uint16_t ctiot_set_psk(uint8_t mode, uint8_t *pskId, uint8_t *psk)
{
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();

	char *tmpPsk = NULL;
	char *tmpPskId = NULL;
	uint8_t tmpPskMode;
	uint16_t tmpPskLen = 0;
	ctiot_session_status_e sessionStatus;
#if CTIOT_CHIPSUPPORT_DTLS == 0
	result = CTIOT_PARA_NOSUP_ERROR; //新版错误码缺
#endif
#if CTIOT_CHIPSUPPORT_DTLS == 1
	if(mode > CTIOT_PSK_MODE_BINARY)
	{
		result = CTIOT_PARA_VALUE_ERROR;
		goto exit;
	}
	if (psk == NULL || strlen((char *)psk) == 0)
	{
		result = CTIOT_DATA_VAL_ERROR;
		goto exit;
	}
	
	if(mode == CTIOT_PSK_MODE_ASCII)
	{
		if(strlen((char *)psk) > PSK_LEN)
		{
			result = CTIOT_DATA_LEN_ERROR;
			goto exit;
		}
		if(pskId!=NULL && strlen((char *)pskId) > PSKID_LEN)
		{
			result = CTIOT_DATA_LEN_ERROR;
			goto exit;
		}
	}
	else
	{
		if(strlen((char *)psk) > 2*PSK_LEN)
		{
			result = CTIOT_DATA_LEN_ERROR;
			goto exit;
		}
		if(strlen((char *)psk)%2 != 0)
		{
			result = CTIOT_DATA_VAL_ERROR;
			goto exit;
		}
		if(pskId!=NULL && strlen((char *)pskId) > 2*PSKID_LEN)
		{
			result = CTIOT_DATA_LEN_ERROR;
			goto exit;
		}
		if(pskId!=NULL && strlen((char *)pskId)%2 != 0)
		{
			result = CTIOT_DATA_VAL_ERROR;
			goto exit;
		}
	}
	sessionStatus =ctiotprv_get_session_status(pContext);
	if (sessionStatus == UE_NOT_LOGINED)
	{
		if(pContext->dns_processing_status == 1)//正在进行dns处理
		{
			result = CTIOT_DNS_ING_ERROR;
			goto exit;
		}
		tmpPskLen = pContext->pskLen;
		tmpPskMode = pContext->pskMode;
		if (pContext->psk != NULL)
		{
			tmpPsk = ctlw_lwm2m_malloc(pContext->pskLen);
			if (tmpPsk == NULL)
			{
				result = CTIOT_SYS_API_ERROR;
				goto exit;
			}
			memcpy(tmpPsk,pContext->psk,tmpPskLen);
			ctlw_lwm2m_free(pContext->psk);
			//修复开源SDKBUG,PSK赋值流程为先判断非空，释放原值指针，赋新值指针。发送AT+CTLWSETPSK=1,FXTERU72RD22UTB6释放PSK指针后，未置PSK指针NULL，又因“FXTERU72RD22UTB6”为非法Hex字符串导致，PSK指针未被赋值，变成野指针。
			pContext->psk = NULL; 
		}
		if (pContext->pskID != NULL)
		{
			tmpPskId = ctlw_lwm2m_strdup(pContext->pskID);
			if (tmpPskId == NULL)
			{
				result = CTIOT_OTHER_ERROR;
				goto exit;
			}
			ctlw_lwm2m_free(pContext->pskID);
			//修复开源SDKBUG,PSK赋值流程为先判断非空，释放原值指针，赋新值指针。发送AT+CTLWSETPSK=1,FXTERU72RD22UTB6释放PSK指针后，未置PSK指针NULL，又因“FXTERU72RD22UTB6”为非法Hex字符串导致，PSK指针未被赋值，变成野指针。
			pContext->pskID = NULL;
		}
		if(mode == CTIOT_PSK_MODE_ASCII)
		{
			pContext->psk = ctlw_lwm2m_strdup((char *)psk);
			pContext->pskLen = strlen((char *)psk);
		}
		else
		{
			uint8_t* pskBuf;
			result = ctiot_at_decode((char *)psk, strlen((char *)psk), &pskBuf);
			if(result != CTIOT_NB_SUCCESS)
			{
				goto exit;
			}
			pContext->psk = (char *)pskBuf;
			pContext->pskLen = strlen((char *)psk)/2;
		}
		if(pskId!=NULL && strlen((char *)pskId)>0)
		{
#ifdef PLATFORM_XINYI
			if(mode == CTIOT_PSK_MODE_ASCII)
				pContext->pskID = ctlw_lwm2m_strdup((char *)pskId);
			else
			{
				uint8_t* pskIdBuf;
				result = ctiot_at_decode((char *)pskId, strlen((char *)pskId), &pskIdBuf);
				if(result != CTIOT_NB_SUCCESS)
				{
					goto exit;
				}
				pContext->pskID = (char *)pskIdBuf;
			}
#else
			pContext->pskID = ctlw_lwm2m_strdup((char *)pskId);
#endif	
		}
		else
		{
			uint8_t imei[CTIOT_IMEI_LEN]={0};
			if(ctchip_get_imei_info(imei,CTIOT_IMEI_LEN)==CTIOT_NB_SUCCESS)
			{
				pContext->pskID=ctlw_lwm2m_strdup((char *)imei);
			}
			else
			{
				result = CTIOT_SYS_API_ERROR;
				pContext->sessionStatus = UE_LOGIN_INITIALIZE_FAILED;
				goto exit;
			}
		}
		pContext->pskMode = mode;
		if (c2f_encode_params(pContext) != NV_OK)
		{
			result = CTIOT_SYS_API_ERROR;
			pContext->sessionStatus = UE_LOGIN_INITIALIZE_FAILED;
			pContext->pskLen = tmpPskLen;
			pContext->pskMode = tmpPskMode;
			if (tmpPsk != NULL)
			{
				pContext->psk = ctlw_lwm2m_malloc(tmpPskLen);
				if(pContext->psk != NULL)
				{
					memcpy(pContext->psk,tmpPsk,tmpPskLen);
				}
			}
			if (tmpPskId != NULL)
			{
				pContext->pskID = ctlw_lwm2m_strdup(tmpPskId);
			}
			goto exit;
		}
	}
	else
	{

		result = CTIOT_SESSION_ERROR_BASE + sessionStatus;
	}

exit:
	if (tmpPsk != NULL)
	{
		ctlw_lwm2m_free(tmpPsk);
	}
	if (tmpPskId != NULL)
	{
		ctlw_lwm2m_free(tmpPskId);
	}
#endif
	return result;
}

uint16_t ctiot_get_psk( uint8_t *buff)//调用时，保证buff不为空，且长度足够
{
	return CTIOT_OPERATOR_NOT_SUPPORTED;//去除状态错误，直接返回838
	#if 0
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();

	if (pContext->sessionStatus == UE_LOGIN_INITIALIZE)
	{
		return CTIOT_SESSION_ERROR_BASE + UE_LOGIN_INITIALIZE;
	}

	if (buff != NULL)
	{
#if	CTIOT_CHIPSUPPORT_DTLS == 1
		if(pContext->pskMode == CTIOT_PSK_MODE_BINARY)
		{
			char* pskBuf = NULL;
			if(pContext->psk)
			{
				pskBuf =ctiot_at_encode((uint8_t *)pContext->psk, pContext->pskLen);
			}
#ifdef PLATFORM_XINYI
			char * pskIdBuf = NULL;
			if(pContext->pskID)
			{
				pskIdBuf = ctiot_at_encode((uint8_t *)pContext->pskID, strlen(pContext->pskID));
			}
			sprintf((char *)buff, "\r\n+CTLWSETPSK: %d,%s,%s\r\n",pContext->pskMode,
			pskBuf ? pskBuf : "",pskIdBuf ? pskIdBuf : "");
			if(pskIdBuf != NULL)
				ctlw_lwm2m_free(pskIdBuf);
#else
			sprintf((char *)buff, "\r\n+CTLWSETPSK: %d,%s,%s\r\n",pContext->pskMode,
			pskBuf ? pskBuf : "",pContext->pskID ? pContext->pskID : "");
#endif
			if(pskBuf != NULL)
			{
				ctlw_lwm2m_free(pskBuf);
			}
		}
		else
		{
			sprintf((char *)buff, "\r\n+CTLWSETPSK: %d,%s,%s\r\n",pContext->pskMode,
			pContext->psk ? pContext->psk : "",pContext->pskID ? pContext->pskID : "");
		}
#endif
#if	CTIOT_CHIPSUPPORT_DTLS == 0
			sprintf((char *)buff, "\r\n+CTLWSETPSK: ,,\r\n");
#endif

	}
	else
	{
		return CTIOT_OTHER_ERROR;
	}
	return result;
	#endif
}

uint16_t ctiot_set_auth_type(ctiot_id_auth_type_e authType/*, uint8_t serverType*/)
{
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();
	ctiot_session_status_e sessionStatus;
	if(authType >= AUTHTYPE_MAX)
	{
		result = CTIOT_PARA_VALUE_ERROR;
		goto exit;
	}
	sessionStatus =ctiotprv_get_session_status(pContext);
	if (sessionStatus == UE_NOT_LOGINED)
	{
		if(pContext->dns_processing_status == 1)//正在进行dns处理
		{
			result = CTIOT_DNS_ING_ERROR;
			goto exit;
		}
#if CTIOT_SIMID_ENABLED == 0
		if(authType == AUTHTYPE_SIMID)
		{
			result = CTIOT_PARA_NOSUP_ERROR;
			goto exit;
		}
#endif
#if CTIOT_SM9_ENABLED == 0
		if(authType == AUTHTYPE_SM9)
		{
			result = CTIOT_PARA_NOSUP_ERROR;
			goto exit;
		}
#endif
		ctiot_id_auth_type_e tmpAuthType = AUTHTYPE_NO;
		tmpAuthType=pContext->idAuthType;
		pContext->idAuthType = authType;
		if(c2f_encode_params(pContext)!=NV_OK)
		{
			pContext->idAuthType= tmpAuthType;
			result = CTIOT_SYS_API_ERROR;
			pContext->sessionStatus = UE_LOGIN_INITIALIZE_FAILED;
		}
	}
	else
	{
		result = CTIOT_SESSION_ERROR_BASE +sessionStatus;
	}
exit:
	return result;
}

uint16_t ctiot_get_auth_type(uint8_t *buff)//调用时，保证buff不为空，且长度足够
{
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();

	if (pContext->sessionStatus == UE_LOGIN_INITIALIZE )
	{
		return CTIOT_SESSION_ERROR_BASE + UE_LOGIN_INITIALIZE;
	}

	if (buff != NULL)
	{
		sprintf((char *)buff, "\r\n+CTLWSETAUTH: %u\r\n", pContext->idAuthType);
	}
	else
	{
		return CTIOT_OTHER_ERROR;
	}
	return result;
}

int ctiot_coap_extend_query_len(void)
{
	int length = 0;
	ctiot_context_t *pContext = ctiot_get_context();
	uint8_t dest_sv[21]={0};//dest_chip[21]={0},dest_module[17]={0};
	snprintf(dest_sv,21,"%s",pContext->chipInfo->sv);
		
	if (pContext->regParaMode == PARAM_CT_ENHANCED_MODE)
	{

		length += strlen("&sv=");
		length += strlen(dest_sv)+20;//strlen(pContext->chipInfo->sv);

		length += strlen("&apn=");
		length += strlen(pContext->chipInfo->apn);

		length += strlen("&imsi=");
		length += strlen(pContext->chipInfo->imsi);

		length += strlen("&iccid=");
		length += strlen(pContext->chipInfo->iccid);

		length += strlen("&chip=");
		length += strlen(pContext->chipInfo->chip);

		length += strlen("&module=");
		length += strlen(pContext->chipInfo->module);

		length += strlen("&rsrp=");
		length += strlen(pContext->wirelessSignal.rsrp);//ctiotprv_calc_digit_len(pContext->wirelessSignal.rsrp);

		length += strlen("&sinr=");
		length += strlen(pContext->wirelessSignal.sinr);// ctiotprv_calc_digit_len(pContext->wirelessSignal.sinr);

		length += strlen("&txpower=");
		length += strlen(pContext->wirelessSignal.txpower);//ctiotprv_calc_digit_len(pContext->wirelessSignal.txpower);

		length += strlen("&cellid=");
		length += strlen(pContext->wirelessSignal.cellid);
		ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_coap_extend_query_len:%d",length);
	}
	return length;
}

char *ctiot_coap_extend_query(int querylen)
{

	char *extend_query = NULL;
	if(querylen == 0)
	{
		return NULL;
	}
	ctiot_context_t *pContext = ctiot_get_context();
	uint8_t dest_sv[21]={0},capbility[21]={0};//dest_chip[21]={0},,dest_module[17]={0}

	snprintf((char *)dest_sv,21,"%s",(char *)pContext->chipInfo->sv);

	sprintf((char *)capbility,"TYWL11" CT_MODULE_CAPBILITY);
	extend_query = ctlw_lwm2m_malloc(querylen + 1);
	if (extend_query == NULL)
		return NULL;
	
	memset(extend_query, 0, querylen + 1);
	
	if (pContext->regParaMode == PARAM_CT_ENHANCED_MODE)
	{
		
		sprintf(extend_query,"&sv=%s%s&apn=%s&imsi=%s&iccid=%s&chip=%s&module=%s&rsrp=%s&sinr=%s&txpower=%s&cellid=%s",
															/*pContext->chipInfo->sv*/dest_sv,capbility,pContext->chipInfo->apn,pContext->chipInfo->imsi,pContext->chipInfo->iccid,
															pContext->chipInfo->chip,pContext->chipInfo->module,pContext->wirelessSignal.rsrp,pContext->wirelessSignal.sinr,pContext->wirelessSignal.txpower,pContext->wirelessSignal.cellid);//,
		ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"extend query:%s",extend_query);
	}

	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"extend query:%s",extend_query);
	return extend_query;
}

uint16_t ctiot_set_server(uint8_t action, uint8_t ipType,char *serverIp, uint16_t port)
{
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();
	ctiot_session_status_e sessionStatus =ctiotprv_get_session_status(pContext);
	if (sessionStatus == UE_NOT_LOGINED)
	{
		if(pContext->dns_processing_status == 1)//正在进行dns处理
		{
			result = CTIOT_DNS_ING_ERROR;
			goto exit;
		}
		if(action > 1 || ipType >= CTIOT_IPTYPE_MAX)
		{
			result = CTIOT_PARA_VALUE_ERROR;
			goto exit;
		}
		
		if(action == 0)
		{
			if(serverIp != NULL && strlen(serverIp)>0)
			{
				if(ipType == CTIOT_IPTYPE_V4)
				{
					if(ctiotprv_net_is_validipv4(serverIp)!=CTIOT_NB_SUCCESS)
					{
						result = CTIOT_PARA_VALUE_ERROR;
						goto exit;
					}
				}
				else if(ipType == CTIOT_IPTYPE_V6)
				{
					if(ctiotprv_net_is_validipv6(serverIp)!=CTIOT_NB_SUCCESS)
					{
						result = CTIOT_PARA_VALUE_ERROR;
						goto exit;
					}
				}
			}
			else
			{
				result = CTIOT_PARA_NUM_ERROR;
				goto exit;
			}
		}
		if(action == 0)/*set ip port*/
		{
			char *tmpServerIP = NULL;
			if(ipType == CTIOT_IPTYPE_V4)
			{
				if(serverIp == NULL && pContext->serverIPV4 == NULL)
				{
					result = CTIOT_PARA_VALUE_ERROR;
					goto exit;
				}
				int tmpPort = pContext->portV4;
				if (pContext->serverIPV4 != NULL)
				{
					tmpServerIP = ctlw_lwm2m_strdup(pContext->serverIPV4);
					if (tmpServerIP == NULL)
					{
						result = CTIOT_OTHER_ERROR;
						goto exit;
					}
					if(serverIp != NULL)
						ctlw_lwm2m_free(pContext->serverIPV4);
				}
				if(serverIp != NULL)
					pContext->serverIPV4 = ctlw_lwm2m_strdup(serverIp);
				pContext->portV4 = port;
				if (c2f_encode_params(pContext) != NV_OK)
				{
					pContext->sessionStatus = UE_LOGIN_INITIALIZE_FAILED;
					if (pContext->serverIPV4 != NULL)
					{
						ctlw_lwm2m_free(pContext->serverIPV4);
						pContext->serverIPV4 = NULL;
					}
					if (tmpServerIP != NULL)
					{
						pContext->serverIPV4 = ctlw_lwm2m_strdup(tmpServerIP);
						ctlw_lwm2m_free(tmpServerIP);
						tmpServerIP = NULL;
						if (pContext->serverIPV4 == NULL)
						{
							result = CTIOT_SYS_API_ERROR;
							goto exit;
						}
					}
					pContext->portV4 = tmpPort;
					result = CTIOT_SYS_API_ERROR;
					goto exit;
				}
				if (tmpServerIP != NULL)
				{
					ctlw_lwm2m_free(tmpServerIP);
				}
			}
			else
			{
				if(serverIp == NULL && pContext->serverIPV6 == NULL)
				{
					result = CTIOT_PARA_VALUE_ERROR;
					goto exit;
				}
				int tmpPort = pContext->portV6;
				if (pContext->serverIPV6 != NULL)
				{
					tmpServerIP = ctlw_lwm2m_strdup(pContext->serverIPV6);
					if (tmpServerIP == NULL)
					{
						result = CTIOT_OTHER_ERROR;
						goto exit;
					}
					if(serverIp != NULL)
						ctlw_lwm2m_free(pContext->serverIPV6);
				}
				if(serverIp!=NULL)
					pContext->serverIPV6 = ctlw_lwm2m_strdup(serverIp);
				pContext->portV6 = port;
				if (c2f_encode_params(pContext) != NV_OK)
				{
					pContext->sessionStatus = UE_LOGIN_INITIALIZE_FAILED;
					if (pContext->serverIPV6 != NULL)
					{
						ctlw_lwm2m_free(pContext->serverIPV6);
						pContext->serverIPV6 = NULL;
					}
					if (tmpServerIP != NULL)
					{
						pContext->serverIPV6 = ctlw_lwm2m_strdup(tmpServerIP);
						ctlw_lwm2m_free(tmpServerIP);
						tmpServerIP = NULL;
						if (pContext->serverIPV6 == NULL)
						{
							result = CTIOT_SYS_API_ERROR;
							goto exit;
						}
					}
					pContext->portV6 = tmpPort;
					result = CTIOT_SYS_API_ERROR;
					goto exit;
				}
				if (tmpServerIP != NULL)
				{
					ctlw_lwm2m_free(tmpServerIP);
				}
			}
		}
		else //1-delete ip address
		{
			if(ipType == CTIOT_IPTYPE_V4)
			{
				char *tmpServerIP = NULL;
				int tmpPort = pContext->portV4;
				if (pContext->serverIPV4 != NULL)
				{
					tmpServerIP = ctlw_lwm2m_strdup(pContext->serverIPV4);
					if (tmpServerIP == NULL)
					{
						result = CTIOT_OTHER_ERROR;
						goto exit;
					}
					ctlw_lwm2m_free(pContext->serverIPV4);
					pContext->serverIPV4 = NULL;
					pContext->portV4 = 0;
				}
				if (c2f_encode_params(pContext) != NV_OK)
				{
					pContext->sessionStatus = UE_LOGIN_INITIALIZE_FAILED;
					result = CTIOT_SYS_API_ERROR;
					pContext->serverIPV4= ctlw_lwm2m_strdup(tmpServerIP);
					pContext->portV4 = tmpPort;
				}
				if (tmpServerIP != NULL)
				{
					ctlw_lwm2m_free(tmpServerIP);
				}
			}
			else if(ipType == CTIOT_IPTYPE_V6)
			{
				char *tmpServerIP = NULL;
				int tmpPort = pContext->portV6;
				if (pContext->serverIPV6 != NULL)
				{
					tmpServerIP = ctlw_lwm2m_strdup(pContext->serverIPV6);
					if (tmpServerIP == NULL)
					{
						result = CTIOT_OTHER_ERROR;
						goto exit;
					}
					ctlw_lwm2m_free(pContext->serverIPV6);
					pContext->serverIPV6 = NULL;
					pContext->portV6 = 0;
				}
				if (c2f_encode_params(pContext) != NV_OK)
				{
					pContext->sessionStatus = UE_LOGIN_INITIALIZE_FAILED;
					result = CTIOT_SYS_API_ERROR;
					pContext->serverIPV6= ctlw_lwm2m_strdup(tmpServerIP);
					pContext->portV6 = tmpPort;
				}
				if (tmpServerIP != NULL)
				{
					ctlw_lwm2m_free(tmpServerIP);
				}
			}
		}

	}
	else
	{
		result = CTIOT_SESSION_ERROR_BASE + sessionStatus;
	}
exit:
	return result;
}

uint16_t ctiot_get_server(uint8_t *buff)//调用时，保证buff不为空，且长度足够
{
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();

	if(pContext->sessionStatus != UE_LOGIN_INITIALIZE)
	{
		if(buff != NULL)
		{
			if(pContext->serverIPV4 == NULL)
			{
				sprintf((char *)buff, "\r\n+CTLWSETSERVER: LW_V4,,\r\n");
			}
			else
			{
				sprintf((char *)buff, "\r\n+CTLWSETSERVER: LW_V4,%s,%u\r\n", pContext->serverIPV4, ctiotprv_get_port(pContext,pContext->portV4));
			}

			if(pContext->serverIPV6 == NULL)
			{
				sprintf((char *)buff, "%s\r\n+CTLWSETSERVER: LW_V6,,\r\n",buff);
			}
			else
			{
				sprintf((char *)buff, "%s\r\n+CTLWSETSERVER: LW_V6,%s,%u\r\n",buff, pContext->serverIPV6, ctiotprv_get_port(pContext,pContext->portV6));
			}
		}
		else
		{
			result = CTIOT_OTHER_ERROR;
		}
	}
	else
	{
		result = CTIOT_SESSION_ERROR_BASE + UE_LOGIN_INITIALIZE;
	}
	return result;
}

uint16_t ctiot_set_lifetime( uint32_t lifetime)
{
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();

	ctiot_session_status_e sessionStatus =ctiotprv_get_session_status(pContext);
	if (sessionStatus == UE_NOT_LOGINED)
	{
		if(pContext->dns_processing_status == 1)//正在进行dns处理
		{
			result = CTIOT_DNS_ING_ERROR;
			goto exit;
		}
		if (lifetime < CTIOT_MIN_LIFETIME || lifetime > CTIOT_MAX_LIFETIME)
		{
			result = CTIOT_PARA_VALUE_ERROR;
			goto exit;
		}
		
		int tmpLifetime = pContext->lifetime;
		pContext->lifetime = lifetime;
		if (c2f_encode_params(pContext) != NV_OK)
		{
			pContext->lifetime = tmpLifetime;
			pContext->sessionStatus = UE_LOGIN_INITIALIZE_FAILED;
			result = CTIOT_SYS_API_ERROR;
			goto exit;
		}
	}
	else
	{
		result = CTIOT_SESSION_ERROR_BASE + sessionStatus;
	}
exit:
	return result;
}

uint16_t ctiot_get_lifetime(uint8_t *buff)//调用时，保证buff不为空，且长度足够
{
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();

	if(pContext->sessionStatus != UE_LOGIN_INITIALIZE)
	{
		if(buff != NULL)
		{
			sprintf((char *)buff, "\r\n+CTLWSETLT: %u\r\n", pContext->lifetime);
		}
		else
		{
			result = CTIOT_OTHER_ERROR;
		}
	}
	else
	{
		result = CTIOT_SESSION_ERROR_BASE + UE_LOGIN_INITIALIZE;
	}
	return result;

}

bool ctiot_update_session_status(ctiot_session_status_e* outStatus,ctiot_session_status_e curStatus, ctiot_session_status_e destStatus)
{
	bool result = false;
	ctiot_context_t *pContext = ctiot_get_context();

	ctiot_session_status_mutex_lock();
	if(outStatus != NULL)
	{
		*outStatus = pContext->sessionStatus;
	}
	if (pContext->sessionStatus != curStatus)
	{
		ctiot_session_status_mutex_unlock();
		return false;
	}
	if (curStatus != destStatus)
	{
		ctiot_log_debug(LOG_COMMON_MODULE, LOG_SESSTATUS_CLASS,"sessionStatus: start %u -> %u\r\n", pContext->sessionStatus,  destStatus);
		switch (destStatus)
		{
		case UE_NOT_LOGINED:
		{
			if (curStatus == UE_LOGIN_INITIALIZE || curStatus == UE_LOGIN_OUTING ||  curStatus == UE_LOGINING)
			{
				pContext->sessionStatus = destStatus;
				result = true;
			}
			break;
		}
		case UE_LOGINING:
		{
			if (curStatus == UE_NOT_LOGINED)
			{
				pContext->sessionStatus = destStatus;
				result = true;
			}
			break;
		}
		case UE_LOGINED:
		{
			if (curStatus == UE_LOGIN_INITIALIZE || curStatus == UE_NOT_LOGINED || curStatus == UE_LOGINING)
			{
				pContext->sessionStatus = destStatus;
				result = true;
			}
			break;
		}
		case UE_LOGIN_OUTING:
		{
			if (curStatus == UE_LOGINED || curStatus == UE_LOGIN_INITIALIZE || curStatus == UE_LOGINING)
			{
				pContext->sessionStatus = destStatus;
				result = true;
			}
			break;
		}
		case UE_LOGIN_INITIALIZE_FAILED:
		{
			if(curStatus != UE_LOGIN_OUTING)
			{
				pContext->sessionStatus = destStatus;
				result = true;
			}
			break;
		}
		default:
			break;
		}
	}
	else
	{
		if (curStatus == UE_LOGIN_INITIALIZE)
		{
			result = true;
			pContext->sessionStatus = UE_LOGIN_INITIALIZE;
		}
	}
	ctiot_session_status_mutex_unlock();

	ctiot_log_debug(LOG_COMMON_MODULE, LOG_SESSTATUS_CLASS,"sessionStatus:%u\r\n", pContext->sessionStatus);
	return result;
}

uint16_t ctiot_session_data(uint8_t action)
{
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();

	if (action != 0)
	{
		return CTIOT_PARA_VALUE_ERROR;
	}
	if (action == 0)
	{
		if (pContext->sessionStatus == UE_LOGIN_INITIALIZE_FAILED)
		{
			pContext->bootFlag = BOOT_NOT_LOAD;
			if(c2f_encode_context(pContext,1)!= NV_OK)
			{
				result = CTIOT_SYS_API_ERROR;
			}
		}
		else
		{
			result = CTIOT_SESSION_ERROR_BASE + ctiotprv_get_session_status(pContext);
		}
	}

	return result;
}

uint16_t ctiot_cfg_reset(uint8_t resetMode)
{
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();

	if (resetMode != 0)
	{
		return CTIOT_PARA_VALUE_ERROR;
	}
	if (pContext->sessionStatus == UE_NOT_LOGINED)
	{
		if(pContext->dns_processing_status == 1)//正在进行dns处理
		{
			return  CTIOT_DNS_ING_ERROR;
		}
		//清除init数据
		if (pContext->serverIPV4 != NULL)
		{
			ctlw_lwm2m_free(pContext->serverIPV4);
			pContext->serverIPV4 = NULL;
		}
		if (pContext->serverIPV6 != NULL)
		{
			ctlw_lwm2m_free(pContext->serverIPV6);
			pContext->serverIPV6 = NULL;
		}

		pContext->portV4 = 0;
		pContext->portV6 = 0;
		pContext->lifetime = CTIOT_DEFAULT_LIFETIME;

#if	CTIOT_CHIPSUPPORT_DTLS == 1
		//清除psk数据
		if (pContext->psk != NULL)
		{
			ctlw_lwm2m_free(pContext->psk);
			pContext->psk = NULL;
		}
		if (pContext->pskID != NULL)
		{
			ctlw_lwm2m_free(pContext->pskID);
			pContext->pskID = NULL;
		}
		pContext->pskLen = 0;
		pContext->pskMode = 0;
#endif
		//清除auth数据
		pContext->idAuthType = AUTHTYPE_NO;
		//清除mode数据
		pContext->onKeepSession = SYSTEM_ON_REBOOT_STOP_SESSION;
		pContext->regParaMode = PARAM_CT_ENHANCED_MODE;
		pContext->connectionType = MODE_NO_DTLS;
		pContext->idAuthMode = AUTHMODE_STANDARD;
		pContext->clientWorkMode = UQ_WORK_MODE;
		//清除recv数据
		pContext->recvDataMode = RECV_DATA_MODE_0;
		pContext->recvTimeMode = CTIOT_TIMEMODE_8;
		pContext->recvDataMaxCacheTime = 8; //默认8秒
		//清除报文加密数据
		#if CTIOT_WITH_PAYLOAD_ENCRYPT == 1
		pContext->payloadEncryptAlgorithm = PAYLOAD_ENCRYPT_NO;
		if(pContext->payloadEncryptPin != NULL)
		{
			ctlw_lwm2m_free(pContext->payloadEncryptPin);
			pContext->payloadEncryptPin = NULL;
		}
		#endif
		if(c2f_encode_params(pContext)!=NV_OK)
		{
			pContext->sessionStatus = UE_LOGIN_INITIALIZE_FAILED;
			result = CTIOT_SYS_API_ERROR;
		}

	}
	else
	{
		result = CTIOT_SESSION_ERROR_BASE + ctiotprv_get_session_status(pContext);
	}

	return result;
}

uint16_t ctiot_close_socket(void)
{
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_SOCKET_CLASS,"close socket >> \r\n");

	/* mode值只影响dtls模式下的通道关闭，mode = 0时，关闭dtls通道及dtls socket，mode = 1时，只关闭socket */
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();

	if (pContext->clientInfo.sock != -1)
	{
/*#ifdef PLATFORM_XINYI
		ctiot_destroy_select_thread();
#endif*/ // no use select Thread
		close(pContext->clientInfo.sock);
	}
	pContext->clientInfo.sock = -1;

#if CTIOT_CHIPSUPPORT_DTLS == 1
	if (pContext->connectionType != MODE_NO_DTLS && pContext->connectionType != MODE_ENCRYPTED_PAYLOAD)
	{
		ctlw_dtls_update_socket_fd(pContext);
	}
	else
#endif
	{
		ctiotprv_update_server_socket(pContext, pContext->clientInfo.sock);
	}
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_SOCKET_CLASS,"close socket <<\r\n");
	pContext->socketIPType = 0;//服务器地址类型待确定
	return result;
}

void ctiot_exit_on_error(ctiot_context_t* pContext)
{
	if(pContext->sessionStatus != UE_LOGIN_OUTING && pContext->sessionStatus != UE_LOGIN_INITIALIZE_FAILED)
	{
		if(ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_INITIALIZE_FAILED))
		{
			ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_exit_on_error...\r\n");
			ctiot_set_release_flag(RELEASE_MODE_L1, 0);//4-发lstatus 1,不清除flash
		}
	}
}

uint16_t ctiot_dereg(uint8_t deregMode)
{
	ctiot_log_debug(LOG_AT_MODULE, LOG_OTHER_CLASS,"ctiot_dereg:deregMode=%d\r\n", deregMode);
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();

	//1.Parameters Validation.
	if (deregMode != 0 && deregMode != 1)
	{
		result = CTIOT_PARA_VALUE_ERROR;
		goto exit;
	}

	//2.Transaction Operation：Session Status Validation. Update Session Status.
	ctiot_session_status_e sessionStatus;
	if (ctiot_update_session_status( &sessionStatus,UE_LOGINED, UE_LOGIN_OUTING))
	{
		ctiot_vote_recv_send_busy() ;

		ctiot_log_info(LOG_AT_MODULE, LOG_OTHER_CLASS,"dereg:%d\r\n",deregMode);
		if(deregMode == 0)
		{
			ctiot_set_release_flag(RELEASE_MODE_DEREG_0, 0);
		}
		else
		{
			ctiot_set_release_flag(RELEASE_MODE_DEREG_1, 0);
		}
/*#ifdef PLATFORM_XINYI
	xy_sendAndrecv_sem_give();
#endif*/ // no use select Thread
	}
	else
	{ //Transaction Operation Fail.
		ctiot_log_info(LOG_AT_MODULE, LOG_SESSTATUS_CLASS,"ctiot_update_session_status fail:pContext->sessionStatus=%d\r\n", pContext->sessionStatus);
		result = CTIOT_SESSION_ERROR_BASE + sessionStatus;
		goto exit;
	}
exit:
	ctiot_log_info(LOG_AT_MODULE, LOG_OTHER_CLASS,"ctiot_dereg:result=%d\r\n", result);
	return result;
}

void ctiot_set_release_flag(uint8_t mode, uint16_t reason)
{
	ctiot_context_t *pContext = ctiot_get_context();
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_set_release_flag....\r\n");
	pContext->releaseFlag = mode;
	if(pContext->ctlwClientStatus != CTIOT_FREE_REQUIRED && pContext->ctlwClientStatus != CTIOT_DEREG_REQUIRED && pContext->ctlwClientStatus != CTIOT_DEREG_PENDING)
	{
		if(mode == RELEASE_MODE_DEREG_0 || mode ==  RELEASE_MODE_DEREG_1)
		{
		    pContext->ctlwClientStatus = CTIOT_DEREG_REQUIRED;
		}
		else
		{
		    pContext->ctlwClientStatus = CTIOT_FREE_REQUIRED;
		}
		pContext->releaseReason = reason;
	}
}

uint16_t ctiot_update(uint8_t raiIndex, uint16_t *msgId)
{
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();
	ctiot_session_status_e sessionStatus;
	if (msgId)
	{
		*msgId = 0;
	}
	if (raiIndex > 1)
	{
		result = CTIOT_PARA_VALUE_ERROR;
		goto exit;
	}
#if CTIOT_CHIPSUPPORT_RAI == 0
	if (raiIndex == 1)
	{
		result = CTIOT_PARA_NOSUP_ERROR;
		goto exit;
	}
#endif
	sessionStatus =pContext->sessionStatus;
	if (sessionStatus == UE_LOGINED)
	{
#if CTIOT_CHIPSUPPORT_DTLS == 1
		if(pContext->connectionType == MODE_DTLS || pContext->connectionType ==  MODE_DTLS_PLUS)
		{
			if(pContext->dtlsStatus == DTLS_SWITCHING)
			{
				result = CTIOT_DTLS_SWITCH_ERROR;
				goto exit;
			}
		}
#endif

	//检查IP
		result = ctiotprv_check_ip_and_eps(0);
		if (result != CTIOT_NB_SUCCESS)
		{
			goto exit;
		}
		if (pContext->updateMsg != NULL && (pContext->updateMsg->msgStatus == QUEUE_SEND_DATA_CACHEING || pContext->updateMsg->msgStatus == QUEUE_SEND_DATA_SENDOUT))
		{
			result = CTIOT_OVERRUN_ERROR;
			goto exit;
		}
		ctiot_update_node *updateNode = NULL;
		updateNode = ctlw_lwm2m_malloc(sizeof(ctiot_update_node));
		if (updateNode == NULL)
		{
			result = CTIOT_SYS_API_ERROR;
			goto exit;
		}
		updateNode->msgId = ctlw_lwm2m_get_next_mid(pContext->lwm2mContext);
		(*msgId) = updateNode->msgId;
		updateNode->msgStatus = QUEUE_SEND_DATA_CACHEING;
		updateNode->raiIndex = raiIndex;

		ctiot_session_status_mutex_lock();
		ctiot_session_status_e tmpSessionStatus =pContext->sessionStatus;
		if(tmpSessionStatus == UE_LOGINED)
		{
			if(pContext->updateMsg)
			{
				ctlw_lwm2m_free(pContext->updateMsg);
			}

			pContext->updateMsg = updateNode;
		}
		else
		{
			ctlw_lwm2m_free(updateNode);
			result = CTIOT_SESSION_ERROR_BASE+tmpSessionStatus;
			ctiot_session_status_mutex_unlock();
			goto exit;
		}
		ctiot_session_status_mutex_unlock();
		ctiot_log_debug(LOG_AT_MODULE, LOG_VOTE_CLASS,"ctiot_update ctchip_get_send_recv_slp_handler() SYSTEM_STATUS_BUSY");
		ctiot_vote_recv_send_busy() ;
/*#ifdef PLATFORM_XINYI
	xy_sendAndrecv_sem_give();
#endif*/ // no use select Thread
	}
	else
	{
		result = CTIOT_SESSION_ERROR_BASE + sessionStatus;
	}

exit:
	if(result == CTIOT_SYS_API_ERROR)
	{
		ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_INITIALIZE_FAILED);
		ctiot_set_release_flag(RELEASE_MODE_L1_WITHOUT_NOTIFY, 0);
	}
	return result;
}

uint16_t ctiot_start_send_recv_thread(void)
{
	ctiot_context_t *pContext = ctiot_get_context();
	uint16_t result = CTIOT_OTHER_ERROR;

	//起发送接收线程
	if (pContext->sendAndRecvThreadStatus == THREAD_STATUS_NOT_LOADED)
	{
		pContext->sendAndRecvThreadStatus = THREAD_STATUS_WORKING;
		//投收发线程忙
		ctiot_vote_recv_send_busy() ;

		if (thread_create(&sendRecvThreadHandle, NULL, (PTHREAD_ROUTINE)ctiot_send_recv_thread_step, NULL)==0)
		{
			result = CTIOT_NB_SUCCESS;
		}
		else
		{ //send_recv Thread Starting Failed.
			ctiot_chip_vote(ctchip_get_send_recv_slp_handler(), SYSTEM_STATUS_FREE);
			pContext->sendAndRecvThreadStatus = THREAD_STATUS_NOT_LOADED;
			result = CTIOT_SYS_API_ERROR;
			goto exit;
		}
	}
exit:
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_start_send_recv_thread:result=%d\r\n", result);
	return result;
}

int ctiot_location_path_validation(char *location)
{
   char * path= "/rd/" ;
   ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctiot_location_path_validation,location=%s\r\n", location );
   if(location != NULL && memcmp(location, path ,4 )== 0 && strlen (location)> 4)
     return CTIOT_NB_SUCCESS ;
   else
   	 return CTIOT_OTHER_ERROR ;
}

void ctiot_vote_recv_send_busy(void)
{
	ctiot_vote_slp_mutex_lock();
	ctiot_log_debug(LOG_COMMON_MODULE, LOG_VOTE_CLASS,"ctchip_get_send_recv_slp_handler:SYSTEM_STATUS_BUSY");
	ctiot_chip_vote(ctchip_get_send_recv_slp_handler(), SYSTEM_STATUS_BUSY);

	ctiot_vote_slp_mutex_unlock();
	return ;
}

//释放线程处理模块
void ctiot_deregister_step(uint8_t deregMode)
{
	//Validation校验
	ctiot_context_t *pContext = ctiot_get_context();


	//1.Unsubscribe
	ctiot_signal_cancel_all();


#ifdef CTLW_APP_FUNCTION
	ctiotprv_clear_appmsglist(pContext);
#endif
	if (pContext->lwm2mContext != NULL)
	{
		ctlw_deleteTransactionList(pContext->lwm2mContext);
	}

	//3.发送报文、//4.等待报文返回
	bool sendDeregNeeded = true;

	uint8_t epsValue = ctchip_sync_cstate();
	if (deregMode == 1)
	{
		pContext->deregResult = 0;
		sendDeregNeeded = false;
	}
	else if (epsValue != NETWORK_CONNECTED_HOME_NETWORK && epsValue != NETWORK_CONNECTED_ROAMING)
	{
		pContext->deregResult = 1;
		sendDeregNeeded = false;
	}
#if CTIOT_CHIPSUPPORT_DTLS
	else if((pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS) && (pContext->dtlsStatus== DTLS_NOK||pContext->dtlsStatus==DTLS_SWITCHING))
	{
#ifdef PLATFORM_XINYI
		if(pContext->connectionType == MODE_DTLS && pContext->dtlsStatus== DTLS_NOK)
		{
			int32_t result = CTIOT_NB_SUCCESS;
			result = ctiotprv_send_dtls_hs();
			if(result == CTIOT_DTLS_HS_ERROR)
			{
				pContext->deregResult = 1;
				sendDeregNeeded = false;			
			}
			else
				pContext->dtlsStatus = DTLS_OK;
		}
#else
		pContext->deregResult = 1;
		sendDeregNeeded = false;
#endif
	}
#endif
	else
	{
		uint16_t scoketStatus = ctiotprv_check_socket();
		if (scoketStatus != CTIOT_NB_SUCCESS)
		{
			pContext->deregResult = 1;
			sendDeregNeeded = false;
		}
	}

	if (sendDeregNeeded && pContext->lwm2mContext !=NULL)
	{
		lwm2m_server_t *server = pContext->lwm2mContext->serverList;
		while (NULL != server)
		{
			ctlw_registration_deregister_by_con(pContext->lwm2mContext, server);
			server = server->next;
		}

		//4.等待报文返回
		uint8_t exitStatus = UE_NOT_LOGINED;
		bool isOk = false;
		while (true)
		{
			isOk = ctiot_recv_step(pContext);
			if (isOk == false)
			{
				ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_recv_step error\r\n");
			}
			time_t lwm2mTimeOut = 1;
			exitStatus = ctlw_lwm2m_step(pContext->lwm2mContext, &lwm2mTimeOut);
			if (exitStatus != COAP_NO_ERROR)
			{
				ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"lwm2m_step error\r\n");
			}

			lwm2m_status_t deregStatus = ctlw_registration_getStatus(pContext->lwm2mContext);
			if (deregStatus == STATE_DEREGISTERED)
			{
				ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"dereg success...\r\n");
				pContext->deregResult = 0; //success

				break;
			}
			else if (deregStatus == STATE_DEREG_FAILED)
			{
				ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"dereg fail...\r\n");
				pContext->deregResult = 1; //fail

				break;
			}
#ifdef PLATFORM_XINYI
			ctlw_usleep(10*1000);
#endif
		}
	}

	//2.Clear list data
	ctiotprv_clear_update_msg(pContext);
	ctiotprv_clear_uplist(pContext);
	ctiotprv_clear_downlist(pContext);

	//5.清除lwm2m context
	ctiotprv_free_objects_by_list(pContext);
	ctlw_lwm2m_clear_session(pContext->lwm2mContext);
	pContext->lwm2mContext = NULL;

	//6. Free ctiot context
	//6.1 release socket and connlist
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"close server socekt and free connlist\r\n");
	#if CTIOT_CHIPSUPPORT_DTLS == 1
	if(pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS)
	{
		ctlw_dtls_ssl_destroy(pContext);
	}
	else
	#endif
	{
		close(pContext->clientInfo.sock);
		pContext->clientInfo.sock = -1;
	}

	ctlw_connection_free(pContext->clientInfo.connList);
	pContext->clientInfo.connList = NULL;
	pContext->clientInfo.securityObjP = NULL;
	pContext->clientInfo.serverObject = NULL;
	//6.2 clear bootflag
	pContext->bootFlag = BOOT_NOT_LOAD;
	//6.3 clean authTokenStr
	if (pContext->authTokenStr != NULL)
	{
		ctlw_lwm2m_free(pContext->authTokenStr);
		pContext->authTokenStr = NULL;
	}
	if (pContext->authModeStr != NULL)
	{
		ctlw_lwm2m_free(pContext->authModeStr);
		pContext->authModeStr = NULL;
	}

	//6.4 Unsubcribe system callback
	ctiot_signal_destroy();

	//---------芯片性能测试----------------
	//---------芯片性能测试----------------
	return;
}

void ctiot_release_step(void)
{
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"entering ctiot_release_step..\r\n");
	ctiot_context_t *pContext = ctiot_get_context();

	bool needWriteSessionIP=true;
	int mode = pContext->releaseFlag;
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_release_step:%d\r\n", mode);

	int onIP = ctiot_trace_ip_by_bindmode();//根据模式类型，确定IP类型

	bool isFlush = false;//true托管写,false托管写
#if CTIOT_CHIPSUPPORT_NBSLEEPING == 0//芯片不支持休眠，默认直接写
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"immediate write because CTIOT_CHIPSUPPORT_NBSLEEPING == 0...\r\n");
	isFlush = true;
#endif
#if CTIOT_CHIPSUPPORT_NBSLEEPING == 1
	if(pContext->onKeepSession==SYSTEM_ON_REBOOT_KEEP_SESSION
#ifndef PLATFORM_XINYI
	)
#else
	|| PLATFORM_XINYI == 1)
#endif
	{
		ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"immediate write because mod_1=1 and value is changed...\r\n");
#ifdef PLATFORM_XINYI
		ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"CTLW immediate write because release step!\r\n");
#endif
		isFlush = true;
	}
#endif
/*#ifdef PLATFORM_XINYI
	//释放下行数据监听线程
	ctiot_destroy_select_thread();
#endif	*/ // no use select Thread
	switch (mode)
	{
	//AT登出资源释放处理
	case RELEASE_MODE_DEREG_0: //dereg 0，发送dereg报文
	case RELEASE_MODE_DEREG_1: //dereg 1，不发送dereg报文
	{
		if (pContext->sessionStatus != UE_LOGIN_OUTING)
		{
			ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_SESSTATUS_CLASS,"Wrong sessionStatus=%d\r\n", pContext->sessionStatus);
			goto exit;
		}
		ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"starting dereg ...\r\n");
		ctiot_deregister_step(mode);
		//uint16_t deregResult = CTIOT_NB_SUCCESS; //登出成功
		/*if (pContext->deregResult != 0)
		{
			deregResult = CTIOT_OTHER_ERROR; //登出异常
		}*/
		if(c2f_encode_context(pContext, isFlush)!=NV_OK)
		{
			pContext->sessionStatus = UE_LOGIN_INITIALIZE_FAILED;
			ctiot_publish_sdk_notification(NULL,CTIOT_SYSTEM_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LSTATUS, CTIOT_LSTATUS_QUITENGINE, 0, 0, NULL);
		}
		else
		{
			ctiot_update_session_status( NULL,pContext->sessionStatus, UE_NOT_LOGINED);
			if(pContext->deregResult == 0)
			{
				ctiot_publish_sdk_notification(NULL,CTIOT_SYSTEM_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_DEREG, 0, 0, 0, NULL);
			}
			else
			{
				ctiot_publish_sdk_notification(NULL,CTIOT_SYSTEM_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LSTATUS, CTIOT_LSTATUS_QUITSESSION, deregothererr, 0, NULL);
			}
		}
		break;
	}
	//异步登录失败资源释放处理
	case RELEASE_MODE_LOGIN_FAIL: //lwstatus 0(login failed)
	case RELEASE_MODE_LOGIN_FAIL_2: // 登录失败的释放资源操作（通知由前面流程实现，不含quit engine）
	{
		if (pContext->sessionStatus != UE_LOGIN_OUTING)
		{
			ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"Wrong sessionStatus=%d\r\n", pContext->sessionStatus);
			goto exit;
		}
		ctiot_free_context();//待讨论不需要立即写Flash，发通知
		ctiot_update_session_status( NULL,pContext->sessionStatus, UE_NOT_LOGINED);
		ctiot_publish_sdk_notification(NULL,CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_REG, pContext->releaseReason, 0, 0, NULL);
		break;
	}
	//quitSession资源释放处理
	case RELEASE_MODE_RECOVER_SESSION_FAIL_L0: //lwstatus 0(restore session failed)
	case RELEASE_MODE_INNER_UPDATE_404_L0: //lwstatus 0(inner update 404)
	case RELEASE_MODE_INNER_UPDATE_TIMEOUT_L0: //lwstatus 0(inner timeout)
	case RELEASE_MODE_INNER_UPDATE_OTHER_FAIL:
	case RELEASE_MODE_OUTER_UPDATE_404_L0: //lwstatus 0(outer update 404)
	case RELEASE_MODE_DRX_DTLS_IP_CHANGE_L0://内部update发送DTLS致命错误和普通错误
	case RELEASE_MODE_DRX_DTLS_SEND_FAIL_L0://发送DTLS报文失败（致命错误）(DRX+”DTLS/DTLS+”模式适用)
	case RELEASE_MODE_DRX_DTLS_RECV_FAIL_L0://接收DTLS报文发生致命错误(DRX+”DTLS/DTLS+”模式适用)
	case RELEASE_MODE_DRX_DTLS_HS_FAIL_L0://HS失败，发出status 0.8通知(DRX+”DTLS/DTLS+”模式适用)
	case RELEASE_MODE_PAYLOAD_ENCRYPT_INIT_FAIL_L0://报文加密初始化失败返回(“收发报文加密”模式适用)
	case RELEASE_MODE_PAYLOAD_ENCRYPT_PIN_FAIL_L0://报文加密初始化失败返回(“收发报文加密”模式适用)
	{
		if (pContext->sessionStatus != UE_LOGIN_OUTING)
		{
			ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"Wrong sessionStatus=%d\r\n", pContext->sessionStatus);
			goto exit;
		}
		ctiot_free_context();
		pContext->bootFlag = BOOT_NOT_LOAD;
		if(c2f_encode_context(pContext,isFlush)!=NV_OK)
		{
			pContext->sessionStatus = UE_LOGIN_INITIALIZE_FAILED;
			ctiot_publish_sdk_notification(NULL,CTIOT_SYSTEM_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LSTATUS, CTIOT_LSTATUS_QUITENGINE, 0, 0, NULL);
		}
		else
		{
			ctiot_update_session_status( NULL,pContext->sessionStatus, UE_NOT_LOGINED);
			ctiot_publish_sdk_notification(NULL,CTIOT_SYSTEM_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LSTATUS, CTIOT_LSTATUS_QUITSESSION, pContext->releaseReason, 0, NULL);
		}
		break;
	}
	//quitEngine资源释放有通知
	case RELEASE_MODE_L1: //lwstatus 1（引擎无法工作）
	case RELEASE_MODE_APP_ERROR: // app error，发lwstatus 1（引擎无法工作）
	{
		if (pContext->sessionStatus != UE_LOGIN_INITIALIZE_FAILED)
		{
			ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"Wrong sessionStatus=%d\r\n", pContext->sessionStatus);
			goto exit;
		}
		ctiot_free_context();
		ctiot_publish_sdk_notification(NULL,CTIOT_SYSTEM_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LSTATUS, CTIOT_LSTATUS_QUITENGINE, pContext->releaseReason, 0, NULL);
		needWriteSessionIP = false;
		break;
	}
	//quitEngine资源释放无通知
	case RELEASE_MODE_L1_WITHOUT_NOTIFY: //不发通知的引擎无法工作状态，V2.1仅有AT触发此场景
	{
		if (pContext->sessionStatus != UE_LOGIN_INITIALIZE_FAILED)
		{
			ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"Wrong sessionStatus=%d\r\n", pContext->sessionStatus);
			goto exit;
		}
		ctiot_free_context();
		break;
	}
	default:
		break;
	}
	if(needWriteSessionIP && onIP == CTIOT_ONIP_RECONNECT )
	{
		ctchip_write_session_ip(NULL);
	}

exit:
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_VOTE_CLASS,"ctchip_get_send_recv_slp_handler:SYSTEM_STATUS_FREE");
	return;
}

uint16_t ctiot_free_context(void)
{
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"entering ctiot_free_context..\r\n");
	ctiot_context_t *pContext = ctiot_get_context();

	uint16_t result = CTIOT_NB_SUCCESS;
	//1.Unsubscribe
	ctiot_signal_cancel_all();
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_signal_cancel_all finished");
	//2. release lwm2m conext
	ctiotprv_free_objects_by_list(pContext);
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctlw_lwm2m_clear_session...");
	if(pContext->lwm2mContext!=NULL)
	{
		ctlw_lwm2m_clear_session(pContext->lwm2mContext);
		pContext->lwm2mContext = NULL;
	}

	//3. free message queue list
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_free_context:free message queue list\r\n");
	ctiotprv_clear_update_msg(pContext);
	ctiotprv_clear_uplist(pContext);
#ifdef CTLW_APP_FUNCTION
	ctiotprv_clear_appmsglist(pContext);
#endif
	ctiotprv_clear_downlist(pContext);

	//4.Free ctiot context
	//4.1 release socket and connlist
	ctiot_log_debug(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"close server socekt and free connlist\r\n");
	#if CTIOT_CHIPSUPPORT_DTLS == 1
	if(pContext->connectionType == MODE_DTLS || pContext->connectionType == MODE_DTLS_PLUS)
	{
		ctlw_dtls_ssl_destroy(pContext);
	}
	else
	#endif
	{
		close(pContext->clientInfo.sock);
		pContext->clientInfo.sock = -1;
	}

	ctlw_connection_free(pContext->clientInfo.connList);
	pContext->clientInfo.connList = NULL;
	pContext->clientInfo.securityObjP = NULL;
	pContext->clientInfo.serverObject = NULL;
	/*
	if (isC2FNeeded)
	{
		//4.2 clear bootflag
		pContext->bootFlag = BOOT_NOT_LOAD;

		//4.3 clean cache
		if(c2f_encode_context(pContext, 0)!=NV_OK)
		{
			ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_INITIALIZE_FAILED);
		}
	}*/
	//4.4 clear authTokenStr
	if (pContext->authTokenStr != NULL)
	{
		ctlw_lwm2m_free(pContext->authTokenStr);
		pContext->authTokenStr = NULL;
	}
	if (pContext->authModeStr != NULL)
	{
		ctlw_lwm2m_free(pContext->authModeStr);
		pContext->authModeStr = NULL;
	}

	//4.5 Unsubcribe system callback
	ctiot_signal_destroy();
	//---------芯片性能测试----------------
	//---------芯片性能测试----------------
	return result;
}

bool ctiot_change_client_status(ctiot_client_status_e srcStatus,ctiot_client_status_e destStatus)
{
	ctiot_context_t* pContext = ctiot_get_context();
	if(pContext->ctlwClientStatus != srcStatus)
	{
		return false;
	}

	pContext->ctlwClientStatus = destStatus;
	return true;
}

uint16_t ctiot_set_payload_encrypt(uint8_t type,uint8_t* value)
{
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();

	if(type > 1)
	{
		result = CTIOT_PARA_VALUE_ERROR;
		goto exit;
	}
	#if CTIOT_WITH_PAYLOAD_ENCRYPT == 1
	ctiot_session_status_e sessionStatus =ctiotprv_get_session_status(pContext);
	if(sessionStatus == UE_NOT_LOGINED)
	{
		if(pContext->dns_processing_status == 1)//正在进行dns处理
		{
			result = CTIOT_DNS_ING_ERROR;
			goto exit;
		}
		if(type == 0)
		{
			ctiot_payload_encrypt_algorithm_e tmpAlgorithm = pContext->payloadEncryptAlgorithm;
			if( value != NULL && (value[0]=='0' || value[0]=='1') && strlen(value) == 1 )
			{
				if(pContext->payloadEncryptAlgorithm != (value[0] -'0'))
				{
					pContext->payloadEncryptAlgorithm = value[0] - '0';
					if(c2f_encode_params(pContext)!=NV_OK)
					{
						pContext->payloadEncryptAlgorithm = tmpAlgorithm;
						pContext->sessionStatus = UE_LOGIN_INITIALIZE_FAILED;
						result = CTIOT_SYS_API_ERROR;
					}
				}
			}
			else
			{
				result = CTIOT_PARA_VALUE_ERROR;
			}
		}
		else if(type == 1)
		{
			if(strlen(value)  > MAX_ENCRYPT_PIN_LEN)
			{
				return CTIOT_PARA_VALUE_ERROR;
			}
			if(pContext->payloadEncryptPin == NULL || strcmp(value, pContext->payloadEncryptPin) != 0)
			{
				uint8_t* tmpPin = NULL;
				if(pContext->payloadEncryptPin != NULL)
				{
					tmpPin = ctlw_lwm2m_strdup(pContext->payloadEncryptPin);
					if(tmpPin == NULL)
					{
						result = CTIOT_SYS_API_ERROR;
						goto exit;
					}
					ctlw_lwm2m_free(pContext->payloadEncryptPin);
					pContext->payloadEncryptPin = NULL;
				}
				if(value != NULL)
				{
					pContext->payloadEncryptPin = ctlw_lwm2m_strdup(value);
					if(pContext->payloadEncryptPin == NULL)
					{
						pContext->payloadEncryptPin = tmpPin;
						result = CTIOT_SYS_API_ERROR;
						goto exit;
					}
				}
				if(c2f_encode_params(pContext)!=NV_OK)
				{
					if(pContext->payloadEncryptPin != NULL)
					{
						ctlw_lwm2m_free(pContext->payloadEncryptPin);
					}
					pContext->payloadEncryptPin = tmpPin;
					result = CTIOT_SYS_API_ERROR;
					pContext->sessionStatus = UE_LOGIN_INITIALIZE_FAILED;
					goto exit;
				}
				else
				{
					if(tmpPin!= NULL)
					{
						ctlw_lwm2m_free(tmpPin);
					}
				}
			}
		}
	}
	else
	{
		result = CTIOT_SESSION_ERROR_BASE + sessionStatus;
	}
	#else
		result = CTIOT_PARA_NOSUP_ERROR;
	#endif
exit:
	return result;
}

uint16_t ctiot_get_payload_encrypt( uint8_t *buff)//调用时，保证buff不为空，且长度足够
{
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t *pContext = ctiot_get_context();

	if (pContext->sessionStatus == UE_LOGIN_INITIALIZE)
	{
		return CTIOT_SESSION_ERROR_BASE + UE_LOGIN_INITIALIZE;
	}
	if (buff != NULL)
	{
#if CTIOT_WITH_PAYLOAD_ENCRYPT == 1
		sprintf((char *)buff, "\r\n+CTLWSETPCRYPT: %u\r\n", pContext->payloadEncryptAlgorithm);
#endif
#if CTIOT_WITH_PAYLOAD_ENCRYPT == 0
		sprintf((char *)buff, "\r\n+CTLWSETPCRYPT: \r\n");
#endif
	}
	else
	{
		return CTIOT_OTHER_ERROR;
	}
	return result;
}

uint16_t ctiot_at_dtls_hs(void)
{
	uint16_t result = CTIOT_NB_SUCCESS;
	ctiot_context_t* pContext = ctiot_get_context();
	if(pContext->sessionStatus == UE_LOGINED)
	{
		if(pContext->connectionType == MODE_NO_DTLS || pContext->connectionType == MODE_ENCRYPTED_PAYLOAD)
		{
			result = CTIOT_NO_DTLS_ERROR;
			goto exit;
		}
#if CTIOT_CHIPSUPPORT_DTLS == 1
		if(pContext->atDtlsHsFlag == 1)
		{
			result = CTIOT_OVERRUN_ERROR;
			goto exit;
		}
		if(pContext->dtlsStatus == DTLS_SWITCHING)
		{
			result = CTIOT_DTLS_SWITCH_ERROR;
			goto exit;
		}
		uint8_t epsValue = ctchip_sync_cstate();
		if(epsValue != NETWORK_CONNECTED_HOME_NETWORK && epsValue != NETWORK_CONNECTED_ROAMING)
		{
			result = epsValue + CTIOT_NETWORK_ERROR_BASE;
			goto exit;
		}

		uint16_t ipStatus = ctiotprv_get_ip_status(pContext->serverIPType, 0);
		if(ipStatus == SDK_IP_STATUS_FALSE)
		{
			result = CTIOT_IP_NOK_ERROR;
			goto exit;
		}
		else if(ipStatus == SDK_IP_STATUS_DISABLE)
		{
			result = CTIOT_IP_TYPE_ERROR;
			goto exit;
		}
		else if(ipStatus == SDK_IP_STATUS_V6PREPARING)
		{
			result = CTIOT_IPV6_ONGOING_ERROR;
			goto exit;
		}

		ctiot_vote_recv_send_busy() ;

		pContext->atDtlsHsFlag = 1;

/*#ifdef PLATFORM_XINYI
		xy_sendAndrecv_sem_give(); 
#endif*/ // no use select Thread

#endif
	}
	else
	{
		result = CTIOT_SESSION_ERROR_BASE + ctiotprv_get_session_status(pContext);
	}

exit:
	if(result == CTIOT_SYS_API_ERROR)
	{
		ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_INITIALIZE_FAILED);
		ctiot_set_release_flag(RELEASE_MODE_L1_WITHOUT_NOTIFY, 0);
	}

	return result;

}

uint16_t ctiot_app_release_sdk(void)
{
	ctiot_context_t* pContext = ctiot_get_context();

	if(pContext->sessionStatus != UE_LOGIN_OUTING && pContext->sessionStatus != UE_LOGIN_INITIALIZE_FAILED)
	{
		if(ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_INITIALIZE_FAILED))
		{
			ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS,"ctiot_app_release_sdk...\r\n");
			ctiot_set_release_flag(RELEASE_MODE_APP_ERROR, 0);
		}
		else
			return CTIOT_OTHER_ERROR;
	}
	else
		return CTIOT_OTHER_ERROR;
	return CTIOT_NB_SUCCESS;
}

void ctiot_chip_vote(uint8_t slpHandler, system_status_e status)
{
#if CTIOT_CHIPSUPPORT_NBSLEEPING == 0
#else
	if (status == SYSTEM_STATUS_FREE)
	{
		ctlwIsBusy = 0;
		ctchip_enable_sleepmode(slpHandler);
	}
	else
	{
		ctlwIsBusy = 1;
		ctchip_disable_sleepmode(slpHandler);
	}
	return ;
#endif
}

uint8_t ctiot_get_vote_status(void)
{
	return ctlwIsBusy;
}
uint16_t ctiot_set_dns_process_status(uint8_t curStatus)
{
	if(curStatus == 0 || curStatus ==1)
	{
		ctiot_context_t* pContext = ctiot_get_context();
		pContext->dns_processing_status = curStatus;
		return 0;
	}
	return 1;
}
#if CTIOT_TIMER_AUTO_UPDATE == 1
void ctiot_set_auto_update_flag(uint8_t flag)//1-需要update,0-无update
{
	ctiot_context_t* pContext = ctiot_get_context();
	if(flag == 1 && pContext->sessionStatus != UE_LOGINED)
		return;

	autoUpdateFlag = flag;

}
uint8_t ctiot_get_auto_update_flag(void)
{
	//ctchip_asyn_notify("ctiot_get_auto_update_flag...\r\n");
	return autoUpdateFlag;
}

void ctiot_start_auto_update_timer(void)
{
	ctiot_context_t* pContext = ctiot_get_context();
#ifndef PLATFORM_XINYI
	if(pContext->onKeepSession == SYSTEM_ON_REBOOT_STOP_SESSION_WITH_AUTO_UPDATE)//不跨reboot
	{
		ctchip_start_sleep_timer((pContext->lifetime - 180)*1000);//lifetime到期前3分钟timer到期
	}
#else
	if(pContext->onKeepSession == SYSTEM_ON_REBOOT_STOP_SESSION_WITH_AUTO_UPDATE)
	{
		ctchip_start_sleep_timer(((int)CTLW_LIFETIME_DELTA(pContext->lifetime))*1000);
	}
#endif
}
uint16_t ctiot_auto_update_step(void)
{
	uint16_t sendResult = CTIOT_NB_SUCCESS;
	if(ctiot_get_auto_update_flag()==1)
	{
		ctiot_context_t* pContext = ctiot_get_context();
		sendResult = ctiot_send_update_msg( SEND_OPTION_NORMAL, 0, 1);
		/*第一次发送失败*/
		if(sendResult != CTIOT_NB_SUCCESS )
		{
			if(sendResult == CTIOT_DTLS_NOK_ERROR || sendResult == CTIOT_DTLS_OPER_ERROR)
			{
				if (ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_OUTING))
				{
					ctiot_set_release_flag(RELEASE_MODE_DRX_DTLS_IP_CHANGE_L0, inupdatedtlssendfail);
				}
				sendResult = CTIOT_NB_SUCCESS;
			}
		}
		ctiot_set_auto_update_flag(0);
	}
	return sendResult;
}
#endif
