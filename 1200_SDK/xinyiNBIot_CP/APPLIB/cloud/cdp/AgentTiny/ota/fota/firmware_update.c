/*----------------------------------------------------------------------------
 * Copyright (c) <2016-2018>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 *---------------------------------------------------------------------------*/

#include "xy_utils.h"
#if TELECOM_VER
#include "internals.h"
#include "atiny_socket.h"
#include "connection.h"
#include "agent_tiny_demo.h"
#include "atiny_context.h"
#include "agenttiny.h"
#include "atiny_log.h"
#include "firmware_update.h"
#include "net_app_resume.h"
// #include "xy_fota_proxy.h"
#include "cdp_backup.h"

#define BLOCK_SIZE 512          //获取差分包的块大小
#define MAX_DOWNLOAD_TIMES 5    //最大重传次数
#define MAX_PACKET_SIZE_2 600   //最大接收下行数据大小

#define COAP_PROTO_PREFIX "coap://"
#define COAPS_PROTO_PREFIX "coaps://"

static char *g_ota_uri_path = NULL;
static int g_block_num = 0;
static int messageID = 6000;

int start_new_req(lwm2m_transaction_t *transacP);

static void firmware_download_reply(lwm2m_transaction_t *transacP,
                                    void *message)
{
    coap_packet_t *packet = (coap_packet_t *)message;
    lwm2m_context_t *contextP = (lwm2m_context_t *)(transacP->userData);
 
    uint32_t len = 0;
    uint32_t block_num = 0;
    uint8_t  block2_more = 0;
    uint16_t block_size = BLOCK_SIZE;
    uint32_t block_offset = 0;
    int ret = 0;
    lwm2m_observe_info_t observe_info;

    if(NULL == message)
    {
        ATINY_LOG(LOG_INFO, "transaction timeout");
        ret = FIRMWARE_DOWNING_TIMEOUT;
        goto failed_exit;
    }

    if(1 != xy_coap_get_header_block2(message, &block_num, &block2_more, &block_size, &block_offset))
    {
        ATINY_LOG(LOG_ERR, "xy_coap_get_header_block2 failed");
        ret = FIRMWARE_DOWNING_URI_ERR;
        goto failed_exit;
    }

    ATINY_LOG(LOG_ERR, "block_num : %lu, block2_more : %lu, block_offset : %lu, payload_len is %u",
              block_num, (uint32_t)block2_more, block_offset, packet->payload_len);

	xy_printf(0,XYAPP, WARN_LOG, "[CDP]size:%d,num:%d,offset:%d,more:%d", block_size, block_num, block_offset, block2_more);
    
    len = (uint32_t)(packet->payload_len);
    ret = OTA_save_one_packet(packet->payload, len);
    if(ret != 0)
    {
        ret = FIRMWARE_DOWNING_FILE_ERR;
        goto failed_exit;
    }

    len = block_offset + (uint32_t)(packet->payload_len);

    //是否还有数据包
    if(block2_more)
    {
    	//继续请求数据包
        if(start_new_req(transacP) != 0)
        {
            ATINY_LOG(LOG_ERR, "start new fota request failed!!!");
            ret = FIRMWARE_DOWNING_TIMEOUT;
            goto failed_exit;
        }
    }
    else
    {				
        ret = OTA_delta_check();
        if(ret != XY_OK)
        {
            ret = FIRMWARE_DOWNING_CHECK_ERR;
            goto failed_exit;
        }
		
        atiny_fota_manager_update_notify(FIRMWARE_UPDATE_RST_SUCCESS, atiny_fota_manager_get_instance());
		//下载完成清除下载记录
		clean_firmware_record();
        ATINY_LOG(LOG_ERR, "download success");
    }
    return;
    
failed_exit:
	clean_firmware_record();

    ATINY_LOG(LOG_INFO, "firmware_update_notify FIRMWARE_UPDATE_RST_FAILED");
    atiny_fota_manager_update_notify(ret, atiny_fota_manager_get_instance());
    return;
}

//Make new request to downloading fota pkt
int start_new_req(lwm2m_transaction_t *transacP)
{
    int ret = 0;

    lwm2m_transaction_t *transaction = NULL;
    lwm2m_context_t *contextP = (lwm2m_context_t *)(transacP->userData);

    transaction = xy_transaction_new(transacP->peerH, COAP_GET, NULL, NULL, contextP->nextMID++, 4, NULL);
    if(!transaction)
    {
        ATINY_LOG(LOG_ERR, "xy_transaction_new failed");
        return -1;
    }

    ret = xy_coap_set_header_uri_path(transaction->message, g_ota_uri_path);
    if(ret < 0 || NULL == transaction->message->uri_path)
    {
        xy_transaction_free(transaction);
        return -1;
    }
    
    ret = xy_coap_set_header_block2(transaction->message, ++g_block_num, 0, BLOCK_SIZE);
    if(ret < 0)
    {
        xy_transaction_free(transaction);
        return -1;
    }

    transaction->callback = firmware_download_reply;
    transaction->userData = (void *)contextP;

    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);

    if(xy_transaction_send(contextP, transaction) != 0)
    {
        ATINY_LOG(LOG_ERR, "xy_transaction_send failed");
        return -1;
    }

	return 0;
}

int parse_firmware_uri(char *uri, char *parsed_host, char *parsed_port)
{
    char *uri_ptr = uri;
    int   protocalLen = 0;

    if(!uri_ptr || !parsed_host || !parsed_port)
	{
		return -1;
	}

    //协议类型判断
	if (strncasecmp(uri_ptr, "coap://", 7) == 0)
	{
		protocalLen = 7;
	}

	else if (strncasecmp(uri_ptr, "coaps://", 8) == 0)
	{
		protocalLen = 8;
	}
	else
	{
		xy_printf(0,XYAPP, WARN_LOG, "parse_firmware_uri url protocol no support");
		return -1;
	}

    uri_ptr += protocalLen; //跳过协议头,指向第一个字符

    if(uri_ptr[0] == '[')//找到'[',尝试匹配']'判断是否为V6地址
    {
    	
        if(!strchr(uri_ptr, ']'))//找到']'
        	return -1;
        
        int ipLen = strchr(uri_ptr, ']') - uri_ptr -1;

        memcpy(parsed_host, uri_ptr + 1, ipLen);

		parsed_host[ipLen] = '\0';

        char *posPtr = parsed_host;

        while(*posPtr != '\0')//检测IPV6字符合法性
        {
            if((*posPtr >= '0' && *posPtr <= '9') || (*posPtr >= 'a' && *posPtr <= 'f') || (*posPtr >= 'A' && *posPtr <= 'F') || *posPtr == ':')
            {
                posPtr++;
            }       
            else
            {
				xy_printf(0,XYAPP, WARN_LOG, "parse_firmware_uri ip is invalid");
                return -1;
            }
                        
        }
        uri_ptr += (ipLen + 2); //V6:偏移[]和V6 IP地址, 指向':'(若有)或'\'或'/'位置        
    }
    else // V4 or domain
    {
        char * ipTailPtr = strchr(uri_ptr, ':'); //寻找V4地址后的第一个冒号
		
        if(!ipTailPtr)
        {
            ipTailPtr = strchr(uri_ptr , '/');

            if(!ipTailPtr)
            {
                ipTailPtr = strchr(uri_ptr, '\\');

                if(!ipTailPtr)
            	{
					xy_printf(0,XYAPP, WARN_LOG, "parse_firmware_uri no fota url source");
				 	return -1; //未找到资源定位符'/'或'\\'
            	}
            }
        }
		
		memcpy(parsed_host, uri_ptr, ipTailPtr - uri_ptr);
        parsed_host[ipTailPtr - uri_ptr] = '\0';
		
        uri_ptr += (ipTailPtr - uri_ptr); //V4或域名:偏移IP地址长度,指向':'(若有),或'/'或'\' 
    }

    if (uri_ptr[0] == ':')//有port,解析port
    {
		char * portTailPtr = strchr(uri_ptr, '/');
		if(!portTailPtr)
		{
			portTailPtr =  strchr(uri_ptr, '\\');

			if(!portTailPtr)
			{
				xy_printf(0,XYAPP, WARN_LOG, "parse_firmware_uri no fota url source");
				return -1;
			}
		}

		//port长度大于5:非法网络端口
		if(portTailPtr - uri_ptr - 1 > 5)
        {
        	xy_printf(0,XYAPP, WARN_LOG, "parse_firmware_uri port invalid\r\n");
            return -1;
        }

		memcpy(parsed_port, uri_ptr + 1, portTailPtr - uri_ptr - 1);
		parsed_port[portTailPtr - uri_ptr -1] = '\0';

		uri_ptr += (portTailPtr - uri_ptr); //port存在,偏移port长度,指向URL '/'或'\'
    }
    else
    {
        if(protocalLen == 7)
        {
            memcpy(parsed_port, "5683", strlen("5683"));
        }     
        else if(protocalLen == 8)
        {
            memcpy(parsed_port, "5684", strlen("5684"));
        }       
    }
    
    if(uri_ptr[0] == '/' || uri_ptr[0] == '\\') //FOTA url字符串
    {
        g_ota_uri_path = (char *)xy_malloc(strlen(uri_ptr) + 1);
        memset(g_ota_uri_path, 0x00, strlen(uri_ptr) + 1);
        strncpy(g_ota_uri_path, uri_ptr, strlen(uri_ptr));
    }
    else
    {
    	xy_printf(0, XYAPP, WARN_LOG, "parse_firmware_uri no url\r\n");
        return -1;//无URL资源
    }
    xy_printf(0,XYAPP, WARN_LOG, "[CDP] fota host:%s, port:%s", parsed_host, parsed_port);
    return 0;
}


int start_firmware_download(lwm2m_context_t *contextP, char *uri)
{
    lwm2m_transaction_t *transaction;
    char *fota_host = NULL;
    char fota_port[6] = {0};
    int ret = -1;
    int uri_len;
    xy_lwm2m_server_t *server;

    if(!contextP || !uri || *uri == '\0')
    {
        ATINY_LOG(LOG_ERR, "invalid params");
        return -1;
    }

    ATINY_LOG(LOG_INFO, "start download");
    
    server = registration_get_registered_server(contextP);
    if(NULL == server)
    {
        ATINY_LOG(LOG_ERR, "registration_get_registered_server failed");
        return -1;
    }

	fota_host = xy_malloc(strlen(uri));
    ret = parse_firmware_uri(uri, fota_host, fota_port);
    if(0 != ret)
    {
        ATINY_LOG(LOG_ERR, "parse_firmware_uri failed");
		xy_free(fota_host);
        return -1;
    }

	OTA_upgrade_init();

    //若下载和通信的host或port不同，则需要使用xy_ota_proxy线程完成OTA PACKET的下载工作
    if(strcmp(g_cdp_config_data->cloud_server_ip, fota_host) || g_cdp_config_data->cloud_server_port != (int)strtol(fota_port,NULL,10))
    {
        // xy_ota_proxy_init();
        // send_msg_2_ota_proxy(OTA_PROXY_MSG_CDP, uri, strlen(uri)+1);
        extern void cdp_start_fota_download(char *ota_url);
        osThreadAttr_t attr = {0};
        attr.name = "cdp_fota";
        attr.priority = osPriorityNormal1;
        attr.stack_size = osStackShared;
        osThreadNew((osThreadFunc_t)cdp_start_fota_download, uri, &attr);
		xy_free(fota_host);
        return 0;
    }

	xy_free(fota_host);

    transaction = xy_transaction_new(server->sessionH, COAP_GET, NULL, NULL, contextP->nextMID++, 4, NULL);
    if(!transaction)
    {
        ATINY_LOG(LOG_ERR, "xy_transaction_new failed");
        return -1;
    }
    ret = xy_coap_set_header_uri_path(transaction->message, g_ota_uri_path);
    if(ret < 0 || NULL == transaction->message->uri_path)
    {
        xy_transaction_free(transaction);
        return -1;
    }

    ret = xy_coap_set_header_block2(transaction->message, g_block_num, 0, BLOCK_SIZE);
    if(ret != 1)
    {
        xy_transaction_free(transaction);
        return -1;
    }

    transaction->callback = firmware_download_reply;
    transaction->userData = (void *)contextP;

    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);

    if(xy_transaction_send(contextP, transaction) != 0)
    {
        ATINY_LOG(LOG_ERR, "xy_transaction_send failed");
        return -1;
    }

    return 0;
}

void clean_firmware_record(void)
{
    if(NULL != g_ota_uri_path)
    {
        lwm2m_free(g_ota_uri_path);
        g_ota_uri_path = NULL;
    }

    g_block_num = 0;
}

atiny_net_context *g_ota_ctx = NULL; //socket fd结构体
static int resendCount = 0;              //重传计数
static int recved_size = 0;

int send_resquest(uint32_t num, uint8_t more, uint16_t size)
{
	int result = 0;
    uint8_t *pktBuffer = NULL;
	size_t pktBufferLen = 0;
	size_t allocLen = 0;
	coap_packet_t *message = (coap_packet_t *)lwm2m_malloc(sizeof(coap_packet_t));

	xy_printf(0,XYAPP, WARN_LOG, "send_resquest message ID is %d\n", messageID);
	xy_coap_init_message(message, COAP_TYPE_CON, COAP_GET, messageID);
	xy_coap_set_header_uri_path(message, g_ota_uri_path);
    
	//if (query != NULL)
		//coap_set_header_uri_query(message, query);
	xy_coap_set_header_block2(message, num, more, size);

	allocLen = xy_coap_serialize_get_size(message);
	if (allocLen == 0)
		return COAP_500_INTERNAL_SERVER_ERROR;

	pktBuffer = (uint8_t *)lwm2m_malloc(allocLen);
	if (pktBuffer != NULL)
	{
		pktBufferLen = xy_coap_serialize_message(message, pktBuffer);
		if (0 != pktBufferLen)
		{
			result = atiny_net_send(g_ota_ctx, pktBuffer, pktBufferLen);
			xy_printf(0,XYAPP, WARN_LOG, "send_resquest sendto result %d\n", result);
		}
		lwm2m_free(pktBuffer);
	}

    lwm2m_free(message);
	return result;
}

int ota_packet_handle(char *recv_buf , uint16_t size)
{
    int result = -1;
    int packetSize = 0;
    uint8_t coap_error_code = NO_ERROR;
    static coap_packet_t message[1];
    coap_error_code = xy_coap_parse_message(message, recv_buf, size);

    if (coap_error_code == NO_ERROR)
    {
        switch (message->type)
        {
        case COAP_TYPE_NON:
        case COAP_TYPE_CON:
        case COAP_TYPE_RST:
            xy_printf(0,XYAPP, WARN_LOG, "ota_packet_handle no ack\n");
            break;
        case COAP_TYPE_ACK:
        {	
            if(messageID != message->mid)
                break;
            
            xy_coap_get_header_size(message, &packetSize);   
            xy_printf(0,XYAPP, WARN_LOG, "ota_packet_handle packetSize is %d\n", message->payload_len);          
            //ATINY_LOG("blockNumber is %d,receive len is %d, block2_more is %d", blockNumber, recved_size + message->payload_len, message->block2_more);

            if (OTA_save_one_packet(message->payload, message->payload_len) != XY_OK)
            {
                result = FIRMWARE_DOWNING_FILE_ERR;
                goto exit;
            }

            if (message->block2_more != 0)
            {
                xy_printf(0,XYAPP, WARN_LOG, "ota_packet_handle packetSize more %d\n", g_block_num + 1);
                resendCount = 5;
                g_block_num++;
                messageID++;
                if (send_resquest(message->block2_num + 1, 0, message->block2_size) == -1)
                {
                    result = FIRMWARE_DOWNING_TIMEOUT;
                    goto exit;
                }
            }
            else
            {
                xy_printf(0,XYAPP, WARN_LOG, "ota_packet_handle ota packet recv over");
                if (!OTA_delta_check())
                {
                    result = FIRMWARE_DOWNING_SUCCESS;
                }
                else
                {
                    result = FIRMWARE_DOWNING_CHECK_ERR;
                }                                
                goto exit;
            }

            break;
        }

        default:
            break;
        }
    }

exit:
	return result;
}

void cdp_start_fota_download(char *ota_url)
{
    char *fota_host = NULL;
    char fota_port[6] = {0};
    int numBytes = 0, blockNumber = 0, recved_size = 0;
	char *recv_buf = NULL;

    //解析fota_uri，获取host和port
    fota_host = xy_malloc(strlen(ota_url));
    if(ota_url != NULL)
    {
        if(parse_firmware_uri(ota_url, fota_host, fota_port))
        {
            xy_printf(0,XYAPP, WARN_LOG, "parse_firmware_uri failed");
            goto exit;
        }
    }
    else
        goto exit;

	//创建socket
	g_ota_ctx = atiny_net_connect(fota_host, fota_port, ATINY_PROTO_UDP);
    if(g_ota_ctx == NULL)
        goto exit;
    xy_printf(0,XYAPP, WARN_LOG, "socket fd: %d", g_ota_ctx->fd);

    //发送第一个请求数据包
    resendCount = 5;
    send_resquest(blockNumber, 0, BLOCK_SIZE);

    //获取差分包数据
    recv_buf = lwm2m_malloc(MAX_PACKET_SIZE_2);
	while (1)
	{
        numBytes = atiny_net_recv_timeout(g_ota_ctx, recv_buf, MAX_PACKET_SIZE, 5000);
		if (numBytes > 0)
		{
            int result = ota_packet_handle(recv_buf, numBytes);
            if(result == FIRMWARE_DOWNING_SUCCESS)
            {
                xy_printf(0,XYAPP, WARN_LOG, "FOTA PACKET download success");
                atiny_fota_manager_update_notify(FIRMWARE_UPDATE_RST_SUCCESS, atiny_fota_manager_get_instance());
                goto exit;
            }
            else if(result > FIRMWARE_DOWNING_SUCCESS)
            {
                xy_printf(0,XYAPP, WARN_LOG, "FOTA PACKET download fail");
                atiny_fota_manager_update_notify(result, atiny_fota_manager_get_instance());
                goto exit;
            }

        }
		else
		{
			if (resendCount == 0)
			{
                xy_printf(0,XYAPP, WARN_LOG, "FOTA PACKET download fail");
                atiny_fota_manager_update_notify(FIRMWARE_DOWNING_TIMEOUT, atiny_fota_manager_get_instance());
                goto exit;
			}
			else
			{
                xy_printf(0,XYAPP, WARN_LOG, "!!!!!!!!!!!!!!!resend packet!!!!!!!!!!!!!!!!\n");
				send_resquest(g_block_num, 0, BLOCK_SIZE);
				resendCount--;
			}
		}
	}
    
exit:
    g_block_num = 0;
    messageID = 6000;
    recved_size = 0;
	xy_free(fota_host);
	if(recv_buf != NULL)
    	lwm2m_free(recv_buf);
    xy_printf(0,XYAPP, WARN_LOG, "cdp_start_fota_download ended\n");
	osThreadExit();
}
#endif
