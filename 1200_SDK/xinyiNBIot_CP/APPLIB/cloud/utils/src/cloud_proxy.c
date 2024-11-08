
/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "xy_at_api.h"
#include "xy_utils.h"
#include "cloud_proxy.h"


#if MOBILE_VER
#include "cis_proxy.h"
#endif

#if TELECOM_VER
#include "cdp_proxy.h"
#endif

#if CTWING_VER
#include "ctwing_proxy.h"
extern cloud_buffer_list_head_t * g_ctlw_cloud_buf_list_head;//ctwing抽象云AT下行数据缓存链表头节点
#endif


#define CLOUD_BUFLIST_COUNT_MAX 8


 int g_cloud_type = -1;

cloud_proxy_callback_t cloud_proxy_array[] = {
#if TELECOM_VER
	{cdp_proxy, cdpProxyConfigProc_test,cdpProxySendProc_test,cdpProxyRecvProc_test},
#endif
#if MOBILE_VER
	{cis_proxy, cisProxyConfigProc,cisProxySendProc,cisProxyRecvProc},
#endif
#if CTWING_VER
	{ctwing_proxy, ctlwProxyConfigProc, ctlwProxySendProc, ctlwProxyRecvProc},
#endif
};

/**
 * @brief 抽象云AT框架使用及测试示例代码
 * 
 */
proxy_recv_callback cdpProxyRecvProc_test(uint8_t req_type,uint8_t* paramList,uint8_t **prsp_cmd)
{
	if (req_type == AT_CMD_ACTIVE)
	{
		*prsp_cmd = xy_malloc(50);
		sprintf(*prsp_cmd, "+XYRECV:999");
	}
	else if(req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(50);
		sprintf(*prsp_cmd, "+XYRECV:0");
	}
	else
		return CLOUD_PROXY_ERR;

    return CLOUD_PROXY_SUCCESS;
}

/**
 * @brief 抽象云AT框架使用及测试示例代码
 * 
 */
proxy_send_callback cdpProxySendProc_test(uint8_t req_type,uint8_t* paramList, uint8_t **prsp_cmd)
{
	int ret = CLOUD_PROXY_SUCCESS;

	if (req_type == AT_CMD_REQ)
	{
		int dataType = -1;
		int dataLen = -1;
		char* data = xy_malloc(strlen(paramList));
	
		if (at_parse_param("%d,%d,%s", paramList,&dataType, &dataLen, data) != AT_OK || dataType == -1 ||(dataLen!=-1 && data== NULL))
		{
			xy_printf(0,XYAPP, WARN_LOG, "[CDP_PROXY] parse error\r\n");
			ret = CLOUD_PROXY_ERR;
		}
		if(ret == CLOUD_PROXY_SUCCESS)
		{
			*prsp_cmd = xy_malloc(100);
			sprintf(*prsp_cmd, "\r\n%d,%d,%s",dataType, dataLen, data);
		}
		
		xy_free(data);
	}
	else
	{
		xy_printf(0,XYAPP, WARN_LOG, "[CDP_PROXY]err req_type ");
		ret = CLOUD_PROXY_ERR;
	}
	
	return ret;
}

/**
 * @brief 抽象云AT框架使用及测试示例代码
 * 
 */
proxy_config_callback cdpProxyConfigProc_test(uint8_t req_type,uint8_t* paramList, uint8_t **prsp_cmd)
{
	int ret = CLOUD_PROXY_SUCCESS;
	if (req_type == AT_CMD_REQ)
	{
		uint8_t* serverIP =  xy_malloc(strlen(paramList));
		int serverPort = 5683;
		int lifetime = 86400;

	    if (at_parse_param(",%s,%d,%d", paramList,serverIP, &serverPort, &lifetime) != AT_OK)
	    {
	    	xy_printf(0,XYAPP, WARN_LOG, "[CDPDEMO]Err:  cdpProxyConfig failed");
	    	ret = CLOUD_PROXY_ERR;
	    }

		if(ret == CLOUD_PROXY_SUCCESS)
		{
			*prsp_cmd = xy_malloc(100);
			sprintf(*prsp_cmd, "\r\n%s,%d,%d",serverIP,serverPort,lifetime);
		}
		xy_free(serverIP);
	}
	else if(req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(32);

		sprintf(*prsp_cmd, "+XYCONFIG:success");

	}
	else
		ret = CLOUD_PROXY_ERR;

	return ret;
}

void cloud_init_bufList(cloud_buffer_list_head_t **head)
{
    if (*head != NULL)
    {
    	cloud_clear_bufList_head(head);
    }
    *head = xy_malloc(sizeof(cloud_buffer_list_head_t));
    memset(*head, 0, sizeof(cloud_buffer_list_head_t));
}


void cloud_clear_bufList_head(cloud_buffer_list_head_t **head)
{
    if (*head == NULL)
    {
        return;
    }
    cloud_buffer_list_t *node = (*head)->first;
    cloud_buffer_list_t *temp_node;

    while (node != NULL)
    {
        temp_node = node->next;
        if(node->msg != NULL)
        	xy_free(node->msg);
        xy_free(node);
        node = temp_node;
    }

    if(*head != NULL)
    {
    	xy_free(*head);
    	*head = NULL;
    }
}

cloud_buffer_list_t *cloud_msg_bufList_exists(cloud_buffer_list_head_t *head,int msgid)
{
	cloud_buffer_list_t *temp = head->first;
    while (temp)
    {
        if (temp->msg->msgid == msgid)
        {
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}


/**
 * @brief 根据通知类型，进行对应的URC上报
 * 
 * @param notifyType cloud_notify_type_e类型
 * @param notify_buf 
 * @return void
 */
void cloud_notification(cloud_notify_type_e notifyType, cloud_proxy_type_e cloudType)
{
	uint8_t * notify_str = NULL;
	switch (notifyType)
	{
		case CLOUD_NOTIFY_RECV_SUCCESS:
		{	
	
			if(cloudType == ctwing_proxy)
			{
#if CTWING_VER	
				notify_str = xy_malloc(40);
				snprintf(notify_str, 40, "\r\n+XYRECV:%d\r\n", g_ctlw_cloud_buf_list_head->count);
#endif
			}

			
			break;
		}
		case CLOUD_NOTIFY_RECV_OVERFLOW:
		{
			notify_str = xy_malloc(32);
    		snprintf(notify_str, 32, "\r\n+XYRECV:buffer overload\r\n");
    		
			break;
		}
		default:
			break;
	}

	if(notify_str !=NULL)
	{
		send_rsp_at_to_ext(notify_str);
		xy_free(notify_str);
	}
}


int32_t cloud_insert_bufList_node(cloud_buffer_list_head_t *head, cloud_buffer_list_t *node)
{
    if (head == NULL)
    {
        return CLOUD_PROXY_ERR;
    }

    //缓存个数限制
    if(head->count >= CLOUD_BUFLIST_COUNT_MAX)
    {
    	return CLOUD_PROXY_OVERFLOW_ERR;
    }

    cloud_buffer_list_t *temp = head->first;
    if(temp == NULL)
    	head->first = node;
    else
    {
    	while(temp->next != NULL)
    	    temp = temp->next;

    	temp->next = node;
    }
    head->count++;

    return CLOUD_PROXY_SUCCESS;
}

cloud_buffer_list_t *cloud_pop_bufList_first_node(cloud_buffer_list_head_t *head)
{
    if (head->count == 0)
    {
        return NULL;
    }
    cloud_buffer_list_t *node = head->first;

    if (head->count == 1)
    {
    	head->first = NULL;
    }
    else
    {
    	head->first = head->first->next;
    }
    head->count--;
    return node;
}

cloud_buffer_list_t *cloud_pop_bufList_node_byId(cloud_buffer_list_head_t *head, int msgid)
{
	if (head->count == 0)
	{
		return NULL;
	}

	cloud_buffer_list_t *temp1 = head->first;
	cloud_buffer_list_t *temp2 = NULL;

	if (temp1->msg->msgid == msgid)
	{
		head->first = temp1->next;
		head->count--;
		return temp1;
	}
	else
	{
		temp2 = temp1->next;
		while(temp2 != NULL)
		{
			if(temp2->msg->msgid == msgid)
			{
				temp1->next = temp2->next;
				head->count--;
				return temp2;
			}
			temp1 = temp2;
			temp2 = temp2->next;
		}

		return NULL;
	}
}



/**
 * @note  配置物联网平台的IP地址,端口号，lifetime等相关服务器配置后向云平台发起注册
 * @brief +XYCONFIG=<cloud_type>,<server_ip>[,<server_port>][,<lifetime>]
 * cloud_type 必填
 * server_ip  必填
 * server_port 选填 缺省默认port=5683
 * lifetime    选填 缺省默认 lifetime =86400
 * @param at_buf 
 * @param prsp_cmd 
 * @return int 
 * 
 */
int at_XYCONFIG_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type != AT_CMD_QUERY && g_req_type != AT_CMD_REQ)
	{
		return ATERR_NOT_ALLOWED;
	}
	
	if(!xy_tcpip_is_ok()) 
	{
		return ATERR_NOT_NET_CONNECT;
	}

	if (at_parse_param("%d", at_buf, &g_cloud_type) != AT_OK || g_cloud_type == -1)
	{
		return ATERR_PARAM_INVALID;
	}

	int i;
	int ret = CLOUD_PROXY_ERR;
	
	for (i = 0; i < sizeof(cloud_proxy_array) / sizeof(cloud_proxy_callback_t); i++)
	{
		if (g_cloud_type == cloud_proxy_array[i].cloudType)
		{
			ret = cloud_proxy_array[i].cloudConfigProc(g_req_type, at_buf, prsp_cmd);
		}
	}

	if(ret != CLOUD_PROXY_SUCCESS)
	{
		return ATERR_NOT_ALLOWED;
	}
	else
	{
		return AT_END;
	}
}

/**
 * @brief 配置注册成功后，通过该命令将数据发送至云平台
 * +XYSEND=<data_type>[,<data_len>][,data][,msg_type]
 * data_type 整型 0:数据发送 1:lifetime更新请求 2:发起注销请求
 * data_len 数据长度 data_type为0时必填
 * data 十六进制格式字符串类型 data_type为0时必填
 * msg_type 数据发送类型0:CON 1:NON data_type为0时必填
 * @param at_buf 
 * @param prsp_cmd 
 * @return int 
 */
int at_XYSEND_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type != AT_CMD_REQ)
	{
		return ATERR_NOT_ALLOWED;
	}

	if(!xy_tcpip_is_ok())
	{
		return ATERR_NOT_NET_CONNECT;
	}

	int i = 0;
	int ret = CLOUD_PROXY_ERR;

	for (i = 0; i < sizeof(cloud_proxy_array) / sizeof(cloud_proxy_callback_t); i++)
	{
		if (g_cloud_type == cloud_proxy_array[i].cloudType)
		{
			ret = cloud_proxy_array[i].cloudSendProc(g_req_type, at_buf, prsp_cmd);
		}
	}
	
	if(ret != CLOUD_PROXY_SUCCESS)
		return ATERR_NOT_ALLOWED;
	else
		return  AT_END;
	
}

/**
 * @brief +XYRECV  从下行数据缓存链表中读取数据
 * +XYRECV?  查询当前下行数据链表中缓存数据的个数，若无，则查询数量为0
 * 
 * @param at_buf 
 * @param prsp_cmd 
 * @return int 
 */
int at_XYRECV_req(char *at_buf, char **prsp_cmd)
{
	int i;
	int ret = CLOUD_PROXY_ERR;

	if(g_req_type != AT_CMD_ACTIVE && g_req_type != AT_CMD_QUERY)
		return ATERR_NOT_ALLOWED;

	for (i = 0; i < sizeof(cloud_proxy_array) / sizeof(cloud_proxy_callback_t); i++)
	{
		if (g_cloud_type == cloud_proxy_array[i].cloudType)
		{
			ret = cloud_proxy_array[i].cloudRecvProc(g_req_type, at_buf, prsp_cmd);
		}
	}

	if(ret != CLOUD_PROXY_SUCCESS)
	{
		return ATERR_NOT_ALLOWED;
	}
	else
	{
		return AT_END;
	}
	
}


