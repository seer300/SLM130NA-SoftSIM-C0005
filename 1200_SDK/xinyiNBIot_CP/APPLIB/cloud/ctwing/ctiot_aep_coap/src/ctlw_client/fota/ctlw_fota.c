

#include "ctlw_config.h"

#ifdef WITH_FOTA
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "ctlw_fota.h"
#include "ctlw_parseuri.h"
#include "../../core/er-coap-13/ctlw-er-coap-13.h"
#include "ctwing_util.h"
#include "xy_fota.h"
#include "../../src/port/ctlw_connection.h"

#include "ctlw_lwm2m_sdk.h"
#include "xy_socket_api.h"
#include "ctlw_NV_data.h"

#define MAX_PACKET_SIZE_1 1024

extern firmwareWritePara firmwareWriteParameter;
ctiotFotaManage fotaStateManage = {fotaState:FOTA_STATE_IDIL, fotaResult:FOTA_RESULT_INIT, sockfd:-1, messageID:6000, notifyUpdatingMsgId:1};

bool g_ctlw_fota_is_no_done = false;//为ture时，指示fotaflag标识已置,但本地fota升级未开始,主线程此时不能获取fota结果
static uint32_t blockNumber = 0;
static uint8_t resendCount = 5;
static uint32_t recved_size = 0;
/******************************************************************/

void ctiot_fota_state_changed(void)
{
    lwm2m_uri_t uri;
    ctlw_lwm2m_stringToUri("/5/0", 4, &uri);

    lwm2m_data_t dataArrayP[1];
    memset(dataArrayP,0,sizeof(lwm2m_data_t));
    dataArrayP[0].id = 3;
    ctlw_lwm2m_data_encode_int(fotaStateManage.fotaState, &dataArrayP[0]);
    char * buff = NULL;
    int len = ctiot_lwm2m_obj_notify_data_serialize("/5/0/3", 1, dataArrayP, DATA_FORMAT_TLV, &buff);
    if(len < 0)
    {
        ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"fota result serialize error");
        return ;
    }
    uint16_t msgId = 0;
    uint16_t result = ctiot_lwm2m_obj_notify(buff, len, SENDMODE_CON, DATA_FORMAT_TLV, "/5/0/3", &msgId);
	
	if(fotaStateManage.notifyUpdatingMsgId == 0)//本次fota上报为平台put 5/0/1 URL后的第一条上报，记录本次msgId
	{
		fotaStateManage.notifyUpdatingMsgId = msgId;
		xy_printf(0,XYAPP, WARN_LOG, "is fota 5/0/3 notify msgId=%d",fotaStateManage.notifyUpdatingMsgId);
	}
		

	
    ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"fota result notify : %u\t msgid:%u", result,msgId);
    
    ctlw_lwm2m_free(buff);
    xy_ctlw_fota_notify(fotaStateManage.fotaState, fotaStateManage.fotaResult);
    // xy_sendAndrecv_sem_give();
    return;

}

void setUrl(void)
{
	char *queryTemp = NULL;
	fotaStateManage.uri = strtok(fotaStateManage.fotaUrl.uri, "?");
	queryTemp = strtok(NULL, "?");
	ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"uri is %s\n", fotaStateManage.uri);
	if (queryTemp != NULL)
	{
		fotaStateManage.query = (char *)ctlw_lwm2m_malloc(strlen(queryTemp) + 2);
		memset(fotaStateManage.query, 0, strlen(queryTemp) + 2);
		strcpy(fotaStateManage.query, "?");
		strcat(fotaStateManage.query, queryTemp);

		ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"fotaStateManage.query is %s\n", fotaStateManage.query);
	}
}

static int send_resquest(uint32_t num, uint8_t more, uint16_t size)
{
	int result = 0;

	//message = NULL;
	ctlw_coap_packet_t *message = (ctlw_coap_packet_t *)ctlw_lwm2m_malloc(sizeof(ctlw_coap_packet_t));
	ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"message ID is %d\n", fotaStateManage.messageID);
	ctlw_coap_init_message(message, COAP_TYPE_CON, COAP_GET, fotaStateManage.messageID);
	ctlw_coap_set_header_uri_path(message, fotaStateManage.uri);
	if (fotaStateManage.query != NULL)
		ctlw_coap_set_header_uri_query(message, fotaStateManage.query);
	ctlw_coap_set_header_block2(message, num, more, size);
	uint8_t *pktBuffer;
	size_t pktBufferLen = 0;
	size_t allocLen;

	allocLen = ctlw_coap_serialize_get_size(message);
	if (allocLen == 0)
		return COAP_500_INTERNAL_SERVER_ERROR;

	pktBuffer = (uint8_t *)ctlw_lwm2m_malloc(allocLen);
	if (pktBuffer != NULL)
	{
		pktBufferLen = ctlw_coap_serialize_message(message, pktBuffer);
		if (0 != pktBufferLen)
		{

			result = sendto(fotaStateManage.sockfd, pktBuffer, pktBufferLen, 0, (struct sockaddr *)&(fotaStateManage.addr), fotaStateManage.addr_len);
			ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"sendto result %d\n", result);
		}
		ctlw_lwm2m_free(pktBuffer);
	}

    ctlw_lwm2m_free(message);
	return result;
}

void *firmware_fota(void *arg)
{
	int ReDownloadtimes = 0;
	uint32_t packetSize = 0;
	//bool runThread = 1;
    uint8_t *buffer= (uint8_t *)ctlw_lwm2m_malloc(MAX_PACKET_SIZE_1);
    
	setUrl();
	send_resquest(START_NUM, START_MORE, BLOCK_SIZE);

	while (1)
	{
		struct timeval tv;
		int result;
		tv.tv_sec = 5; //读socket时间
		tv.tv_usec = 0;
		int numBytes;
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(fotaStateManage.sockfd, &readfds);
		result = select(FD_SETSIZE, &readfds, NULL, NULL, &tv);
		if (result > 0)
		{
			if (FD_ISSET(fotaStateManage.sockfd, &readfds))
			{
				struct sockaddr_storage addr;
				socklen_t addrLen;
				addrLen = sizeof(addr);
				numBytes = recvfrom(fotaStateManage.sockfd, buffer, MAX_PACKET_SIZE_1, 0, (struct sockaddr *)&addr, &addrLen);

				if (0 > numBytes)
				{
					ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS, "Error in recvfrom(): %d %s\r\n", errno, strerror(errno));
				}
				else if (0 < numBytes)
				{
					uint8_t ctlw_coap_error_code = NO_ERROR;
					static ctlw_coap_packet_t message[1];
					ctlw_coap_error_code = ctlw_coap_parse_message(message, buffer, numBytes);
                    
					if (ctlw_coap_error_code == NO_ERROR)
					{
						switch (message->type)
						{
						case COAP_TYPE_NON:
						case COAP_TYPE_CON:
						case COAP_TYPE_RST:
							ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"no ack\n");
							break;
						case COAP_TYPE_ACK:
						{	
							if(fotaStateManage.messageID != message->mid)
                                break;
                            
                            ctlw_coap_get_header_size(message, &packetSize);   
							ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"packetSize is %d\n", packetSize);
							ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"blockNumber is %d,receive len is %d, block2_more is %d", blockNumber, recved_size + message->payload_len, message->block2_more);

                            
                            OTA_save_one_packet(message->payload, message->payload_len);

                            if (message->block2_more != 0)
							{
								resendCount = 5;
								blockNumber++;
								fotaStateManage.messageID++;
								if (send_resquest(message->block2_num + 1, 0, message->block2_size) == -1)
								{
									fotaStateManage.fotaState = FOTA_STATE_IDIL;
									fotaStateManage.fotaResult = FOTA_RESULT_DISCONNECT;
									ctiot_fota_state_changed();
                                    goto exit;
								}
							}
							else
							{
								ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"recv over");
								if (!OTA_delta_check())
								{
									fotaStateManage.fotaState = FOTA_STATE_DOWNLOADED;
									fotaStateManage.fotaResult = FOTA_RESULT_INIT;
									ctiot_fota_state_changed();
								}
								else
								{
									fotaStateManage.fotaState = FOTA_STATE_IDIL;
									fotaStateManage.fotaResult = FOTA_RESULT_CHECKFAIL;
									ctiot_fota_state_changed();
								}                                
                                goto exit;
							}

							break;
						}

						default:
							break;
						}
					} /* Request or Response */
				}
			}
		}
		else
		{
			if (resendCount == 0)
			{
                ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"platform no ack received!\n");
                fotaStateManage.fotaState = FOTA_STATE_IDIL;
                fotaStateManage.fotaResult = FOTA_RESULT_DISCONNECT;
                goto exit;
			}
			else
			{
				send_resquest(blockNumber, START_MORE, BLOCK_SIZE);
				ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"!!!!!!!!!!!!!!!resend packet!!!!!!!!!!!!!!!!\n");
				resendCount--;
			}
		}
	}

	/*
		if(fotaStateManage.fotaState = FOTA_STATE_DOWNLOADING && (fotaStateManage.fotaResult == FOTA_RESULT_DISCONNECT || fotaStateManage.fotaResult == FOTA_RESULT_CHECKFAIL))
		{
			ReDownloadtimes++;
			if(ReDownloadtimes <= MAX_DOWNLOAD_TIMES)
				goto ReDownload;
		}
    */
    
exit:
	if(fotaStateManage.sockfd >= 0)
	{
		close(fotaStateManage.sockfd);
		fotaStateManage.sockfd = -1;
	}
		
    blockNumber = 0;
    fotaStateManage.messageID = 6000;
	if(fotaStateManage.query)
	{
		ctlw_lwm2m_free(fotaStateManage.query);
		fotaStateManage.query = NULL;
	}

	if(fotaStateManage.fotaUrl.address != NULL)
	{
		xy_free(fotaStateManage.fotaUrl.address);
		fotaStateManage.fotaUrl.address = NULL;
	}

	if(fotaStateManage.fotaUrl.uri !=NULL)
	{
		xy_free(fotaStateManage.fotaUrl.uri);
		fotaStateManage.fotaUrl.uri = NULL;
	}

    ctlw_lwm2m_free(buffer);
    ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"runThread ended\n");
	thread_exit(NULL);
}

bool fota_create_socket(char *host, int port)
{
	if(fotaStateManage.sockfd >=0)
	{
		ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"ctlw fota socket exit error!");
		return false;
	}
	char *server_ip = fotaStateManage.fotaUrl.address;
	int32_t server_port = fotaStateManage.fotaUrl.port;
	
	ip_addr_t* remote_addr = xy_malloc(sizeof(ip_addr_t));

	fotaStateManage.sockfd = xy_socket_by_host(server_ip, Sock_IPv46, IPPROTO_UDP, NULL, server_port, remote_addr);

	if(fotaStateManage.sockfd < 0)
	{
		ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"ctlw fota create socket err");
		return false;
	}

	if(remote_addr->type == IPADDR_TYPE_V4)
	{
		memset(&(fotaStateManage.addr), 0x00, sizeof(fotaStateManage.addr));
		((struct sockaddr_in *)&(fotaStateManage.addr))->sin_addr.s_addr = remote_addr->u_addr.ip4.addr;
		((struct sockaddr_in *)&(fotaStateManage.addr))->sin_port = htons(server_port);
		((struct sockaddr_in *)&(fotaStateManage.addr))->sin_family = AF_INET;
		fotaStateManage.addr_len = sizeof(struct sockaddr_in);

	}
	else if(remote_addr->type == IPADDR_TYPE_V6)
	{
		memset(&(fotaStateManage.addr), 0x00, sizeof(fotaStateManage.addr));
		memcpy(((struct sockaddr_in6 *)&(fotaStateManage.addr))->sin6_addr.s6_addr, remote_addr->u_addr.ip6.addr, sizeof(remote_addr->u_addr.ip6.addr));
		((struct sockaddr_in6 *)&(fotaStateManage.addr))->sin6_family = AF_INET6;
		((struct sockaddr_in6 *)&(fotaStateManage.addr))->sin6_port = htons(server_port);
		fotaStateManage.addr_len = sizeof(struct sockaddr_in6);
	}
	else
	{
		xy_assert(0);
	}
	xy_free(remote_addr);
	return true;
}

int fota_start(char *url)
{
	int err = 0;
	bool result = true;
	thread_handle_t tid;

	result = ctiot_parse_url(url, &fotaStateManage.fotaUrl);

	if (result == false)
	{
		fotaStateManage.fotaState = FOTA_STATE_IDIL;
		fotaStateManage.fotaResult = FOTA_RESULT_URIINVALID;
		ctiot_fota_state_changed();
		goto exit;
	}
	if (strcmp(fotaStateManage.fotaUrl.protocol, "coap") != 0)
	{
		ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"fotaUrl.protocol is %s\n", fotaStateManage.fotaUrl.protocol);
		fotaStateManage.fotaState = FOTA_STATE_IDIL;
		fotaStateManage.fotaResult = FOTA_RESULT_PROTOCOLFAIL;
		ctiot_fota_state_changed();
		result = false;
		goto exit;
	}

	//create socket
	if (fota_create_socket(fotaStateManage.fotaUrl.address, fotaStateManage.fotaUrl.port) == false)
	{
		ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"can't create socket\n");

		fotaStateManage.fotaState = FOTA_STATE_IDIL;
		fotaStateManage.fotaResult = FOTA_RESULT_URIINVALID;
		ctiot_fota_state_changed();
		result = false;
		goto exit;
	}
	
	OTA_upgrade_init();

	osThreadAttr_t thread_attr = {0};
	thread_attr.name	   = "ctlw_fota_dnload";
	thread_attr.priority   = osPriorityNormal1;
	thread_attr.stack_size = osStackShared;
	
	err = thread_create(&tid, &thread_attr, firmware_fota, NULL);//create fota downing thread
	if (err != 0)
	{
		ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"can't create thread: %s\n", strerror(err));
		result = false;
		goto exit;
	}

exit:
	if(result == false)
	{
		if(fotaStateManage.fotaUrl.address != NULL)
		{
			xy_free(fotaStateManage.fotaUrl.address);
			fotaStateManage.fotaUrl.address = NULL;
		}
		if(fotaStateManage.fotaUrl.uri !=NULL)
		{
			xy_free(fotaStateManage.fotaUrl.uri);
			fotaStateManage.fotaUrl.uri = NULL;
		}
	}
	
	return result;
}


void xy_ctlw_fota_state_clear()
{
	ctiot_context_t *pContext = ctiot_get_context();
	extern unsigned char *g_ctlw_user_cache;
	
	pContext->fotaFlag = 0;
	
	c2f_encode_params(pContext);

	c2f_encode_context(pContext,true);
}


int ctlw_fota_check()
{    
    int ret = -1;
    ctiot_context_t* pContext = ctiot_get_context();
    
    ret = OTA_get_upgrade_result(); 
    if (ret == XY_OK)
    {
        fotaStateManage.fotaState = FOTA_STATE_IDIL;
        fotaStateManage.fotaResult = FOTA_RESULT_SUCCESS;
        ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"update success!\n");
        ret = CTIOT_NB_SUCCESS;
    }
    else if(ret == XY_ERR)
    {
        fotaStateManage.fotaState = FOTA_STATE_IDIL;
        fotaStateManage.fotaResult = FOTA_RESULT_UPDATEFAIL;  
        ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"update fail!\n");
        ret = CTIOT_NB_SUCCESS;
    }
    else
    {
        ret = CTIOT_NB_FAILED;
        return ret;
    }
    
    return ret;
}

/*app notify*/
void ctlw_fota_notify(char* uri, ctiot_notify_type notifyType, ctiot_notify_subType subType, uint16_t value, uint32_t data1, uint32_t data2, void *reservedData)
{
	log_debug("ctlw_fota_app_notify,notifyType=%u,subType=%u,value=%u\r\n",notifyType,subType,value);
	if (subType == CTIOT_NOTIFY_SUBTYPE_LSTATUS && value >= 2)
	{
		return;
	}
	switch (notifyType)
	{
		case CTIOT_NOTIFY_ASYNC_NOTICE:
		{
			switch (subType)
			{
				case CTIOT_NOTIFY_SUBTYPE_REG:
				{
					ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_REG,result=%u\r\n",value);
					break;
				}
				case CTIOT_NOTIFY_SUBTYPE_LWEVENT:
				{
					ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_LWEVENT,result=%u,data1=%u\r\n",value,data1);
					break;
				}
				case CTIOT_NOTIFY_SUBTYPE_UPDATE:
				{
					ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_UPDATE,result=%u,data1=%u\r\n",value,data1);
					break;
				}
				case CTIOT_NOTIFY_SUBTYPE_DEREG:
				{
					ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_DEREG,result=%u\r\n",value);
					break;
				}
				case CTIOT_NOTIFY_SUBTYPE_SEND:
				{
					ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_SEND,result=%u,data1=%u\r\n",value,data1);
					break;
				}
				case CTIOT_NOTIFY_SUBTYPE_LSTATUS:
				{
					ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_LSTATUS,result=%u,data1=%u\r\n",value,data1);
					break;
				}
				case CTIOT_NOTIFY_SUBTYPE_DTLS_HS:
				{
					ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_DTLS_HS,result=%u\r\n",value);
					break;
				}
				case CTIOT_NOTIFY_SUBTYPE_SESSION_RESTORE:
				{
					ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_SESSION_RESTORE\r\n");
					break;
				}
				default:
				{
					ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"unknown subtype\r\n");
					break;
				}
			}
			break;
		}
		case CTIOT_SYS_API_ERROR:
		{
			ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"send error,value = %u",value);
			break;
		}
		case CTIOT_NOTIFY_RECV_DATA:
		{
			if(subType == 0)
			{
				ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"CTIOT_NOTIFY_RECV_DATA:reservedData:%s\r\n",(char *)reservedData);
			}
			else if(subType == 1)
			{
				ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"CTIOT_NOTIFY_RECV_DATA:NULL\r\n");
			}
			break;
		}
		default:
		{
			ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"unknown notifyType\r\n");
			break;
		}
	}
}

/**
 * @brief 判断本次ACK是否为平台下方5/0/1URL后，主动上报5/0/3的ACK回复,若判断为真，开启fota下载线程
 * 
 * @param msgId 
 */
void fota_notification_ack_handle(int msgId)
{
	if(fotaStateManage.notifyUpdatingMsgId == msgId)//平台下方5/0/1URL后，主动上报5/0/3的ACK回复
	{
		xy_printf(0,XYAPP, WARN_LOG, "[CTLW_FOTA] fota start");
		fota_start(firmwareWriteParameter.packageUri);
		
	}
}


#endif
