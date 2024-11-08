/**
 * @file ctwing_proxy.c
 * @author your name (you@domain.com)
 * @brief 云抽象AT
 * @version 0.1
 * @date 2023-02-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */


#include "xy_at_api.h"
#include "xy_utils.h"
#include "xy_system.h"
#include "ps_netif_api.h"
#include "cloud_proxy.h"
#include "ctlw_lwm2m_sdk.h"
#include "ctwing_util.h"
#include "xy_ctwing_api.h"

#define CTLW_PROXY_TIMEOUT 180 		//超时时间,单个数据包超时重传总耗时155秒
#define CTLW_PROXY_DEFAULT_PORT 5683	//默认服务器端口号
#define CTLW_PROXY_DEFAULT_LIFETIME 86400	//默认lifetime生命周期


typedef enum
{
    CTLW_PROXY_SEND_DATA = 0,//发送业务数据
    CTLW_PROXY_UPDATE = 1,//更新lifetime周期
    CTLW_PROXY_DEREG = 2,//注销业务
}ctlw_proxy_req_type_e;//请求类型

typedef enum
{
    CTLW_PROXY_DATA_CON = 0,
    CTLW_PROXY_DATA_NON = 1,
}ctlw_proxy_data_type_e;

cloud_buffer_list_head_t * g_ctlw_cloud_buf_list_head = NULL;//抽象云AT下行数据缓存链表头节点
osMutexId_t g_ctlw_cloud_buf_mutex = NULL;//抽象云AT下行数据缓存链表读写锁


/**
 * @brief 设置抽象云AT标志位
 * 
 * @param flag  1:当前注册由抽象云AT发起及维持  0:清除当前注册由抽象云AT发起及维持标记
 * @return int32_t 
 */
void ctlw_set_abstract_cloud_flag(uint8_t flag)
{
    ctiot_context_t *pContext = xy_ctlw_ctiot_get_context();
    pContext->abstractCloudFlag = flag;
}


/**
 * @brief 抽象云AT相关参数初始化
 * 
 */
void ctlw_proxy_init()
{
    cloud_init_bufList(&g_ctlw_cloud_buf_list_head);//下行数据链表初始化

    cloud_mutex_create(&g_ctlw_cloud_buf_mutex);//初始化抽象云AT下行数据缓存链表读写锁

    ctlw_set_abstract_cloud_flag(1);//设置抽象云AT标志位

    ctiot_set_recv_data_mode(RECV_DATA_MODE_0,CTIOT_TIMEMODE_0);//设置下行数据为非缓存模式,不使用SDK内部下行链表。抽象云AT单独维护独立的下行缓存链表
}



/**
 * @brief 平台下行数据加入抽象云AT下行数据缓存链表
 * 
 * @param newNode 
 */
void ctlw_proxy_cloud_buf_add(uint8_t *payloadBuf)
{
    ctiot_context_t* pContext=xy_ctlw_ctiot_get_context();

    int32_t data_len = strlen(payloadBuf);
    
    cloud_buffer_list_t *msg_node_list = xy_malloc2(sizeof(cloud_buffer_list_t));

    if(msg_node_list == NULL)
    {
        return;
    }

    memset(msg_node_list, 0x00, sizeof(cloud_buffer_list_t));

    msg_node_list->msg= xy_malloc2(sizeof(cloud_buffer_pkt_msg) + data_len +1);

    if(msg_node_list->msg == NULL)
    {
        if(msg_node_list)
        {
            xy_free(msg_node_list);
        }
        return;    
    }

    memset(msg_node_list->msg, 0x00, sizeof(cloud_buffer_pkt_msg) + data_len +1);
    strcpy(msg_node_list->msg->data, payloadBuf);
    msg_node_list->msg->data_len = data_len;
    msg_node_list->msg->msgid = pContext->abstractDownMsgId;

    osMutexAcquire(g_ctlw_cloud_buf_mutex, osWaitForever);
    int32_t ret = cloud_insert_bufList_node(g_ctlw_cloud_buf_list_head, msg_node_list);
    
	if(ret == CLOUD_PROXY_SUCCESS)
	{
		cloud_notification(CLOUD_NOTIFY_RECV_SUCCESS, ctwing_proxy);
	}
	else if(ret == CLOUD_PROXY_OVERFLOW_ERR)
	{
		cloud_notification(CLOUD_NOTIFY_RECV_OVERFLOW, ctwing_proxy);
	}
    osMutexRelease(g_ctlw_cloud_buf_mutex);
}



/**
 * @brief 抽象云AT ctwing云配置及注册
 * 
 * @param req_type 
 * @param paramList 
 * @param prsp_cmd 
 * @return proxy_config_callback 
 */
proxy_config_callback ctlwProxyConfigProc(uint8_t req_type,uint8_t* paramList, uint8_t **prsp_cmd)
{
    int32_t ret = CLOUD_PROXY_ERR;

    xy_ctlw_module_entry();

    if (req_type == AT_CMD_REQ)
    {
        int port = CTLW_PROXY_DEFAULT_PORT;
        int lifetime = CTLW_PROXY_DEFAULT_LIFETIME;
        int32_t cloudType = 0;
        uint8_t * serverIP = xy_malloc(47);
        memset(serverIP, 0x00, 47);
        
        if(at_parse_param("%d,%47s,%d[1-65535],%d[300-2592000]",paramList, &cloudType, serverIP, &port, &lifetime)!= AT_OK || strlen(serverIP) == 0)
        {
            xy_printf(0,XYAPP, WARN_LOG, "[CTLW_PROXY] parameter parsing failed");
            goto exit;
        }
        
        if(ctlw_cloud_setting(serverIP, port, lifetime, AUTHMODE_SIMPLIFIED) != CTIOT_NB_SUCCESS)
        {
            xy_printf(0,XYAPP, WARN_LOG, "[CTLW_PROXY] ctlw cloud setting failed");
            goto exit;
        }

        ctlw_proxy_init();

        if(ctlw_cloud_register(CTLW_PROXY_TIMEOUT) != CTIOT_NB_SUCCESS)
        {
            xy_printf(0,XYAPP, WARN_LOG, "[CTLW_PROXY] ctlw cloud setting failed");
            ctlw_set_abstract_cloud_flag(0);//注册失败,清除当前注册由抽象云AT发起及维持标记
            goto exit;
        }
        ret = CLOUD_PROXY_SUCCESS;

exit:
        xy_free(serverIP);
    }
    else if(req_type == AT_CMD_QUERY)
    {
        ret = CLOUD_PROXY_SUCCESS;
        *prsp_cmd = xy_malloc(40);

        if(xy_ctlw_check_if_task_running())
        {
            sprintf(*prsp_cmd, "+XYCONFIG:success");
        }
        else
        {
            sprintf(*prsp_cmd, "+XYCONFIG:fail");
        }       
    }

    return ret;
}



/**
 * @brief 抽象云AT,ctwing数据发送,update及去注册
 * 
 * @param req_type 
 * @param paramList 
 * @param prsp_cmd 
 * @return proxy_send_callback 
 */
proxy_send_callback ctlwProxySendProc(uint8_t req_type,uint8_t* paramList, uint8_t **prsp_cmd)
{
    int32_t ret = CLOUD_PROXY_ERR;

    if (req_type == AT_CMD_REQ)
    {
        int32_t dataReqType = -1;
        int32_t dataLen = -1;
        int32_t msgType = -1;
        uint8_t *data = NULL;
        int32_t sendMode = -1;

        if(at_parse_param("%d(0-2), %d[1-1024], %p[], %d[0|1]", paramList, &dataReqType, &dataLen, &data, &msgType) !=AT_OK)
        {
            xy_printf(0,XYAPP, WARN_LOG, "[CTLW_PROXY] parse error\r\n");
            return ret;
        }
        switch (dataReqType)
        {
            case CTLW_PROXY_SEND_DATA:
            {   
                xy_printf(0,XYAPP, WARN_LOG, "[CTLW_PROXY] ctlw proxy send");
                if(dataLen == -1 || data == NULL || msgType == -1)
                {
                    xy_printf(0,XYAPP, WARN_LOG, "[CTLW_PROXY] ctlw proxy send parse error\r\n");
                    break;
                }
                
                sendMode = (msgType == CTLW_PROXY_DATA_CON)? SENDMODE_CON : SENDMODE_NON;

                if(ctlw_cloud_send_data(data, sendMode, CTLW_PROXY_TIMEOUT) != CTIOT_NB_SUCCESS)
                {
                    xy_printf(0,XYAPP, WARN_LOG, "[CTLW_PROXY] ctlw cloud send data error\r\n");
                    break;
                }
                ret = CLOUD_PROXY_SUCCESS;
                break;
            }
            case CTLW_PROXY_UPDATE: //update
            {
                xy_printf(0,XYAPP, WARN_LOG, "[CTLW_PROXY] ctlw proxy update");

                if(ctlw_cloud_update(CTLW_PROXY_TIMEOUT) != CTIOT_NB_SUCCESS)
                {
                    xy_printf(0,XYAPP, WARN_LOG, "[CTLW_PROXY] ctlw cloud send updata error\r\n");
                    break; 
                }
                ret = CLOUD_PROXY_SUCCESS;
                break; 
            }
            case CTLW_PROXY_DEREG: //去注册
            {
                xy_printf(0,XYAPP, WARN_LOG, "[CTLW_PROXY] ctlw proxy deregister");
                
                if(ctlw_cloud_deregister(CTLW_PROXY_TIMEOUT) != CTIOT_NB_SUCCESS)
                {
                    xy_printf(0,XYAPP, WARN_LOG, "[CTLW_PROXY] ctlw cloud deregister error\r\n");
                    
                    break; 
                }
                ret = CLOUD_PROXY_SUCCESS;
                ctlw_set_abstract_cloud_flag(0);
                break;
            }

            default:
                break;
        }

    }
    
    return ret;   
}


/**
 * @brief ctwing抽象云AT,获取下行缓存数据
 * 
 * @param req_type 
 * @param paramList 
 * @param prsp_cmd 
 * @return proxy_send_callback 
 */
proxy_send_callback ctlwProxyRecvProc(uint8_t req_type,uint8_t* paramList, uint8_t **prsp_cmd)
{
    int32_t ret = CLOUD_PROXY_ERR;
    if(req_type != AT_CMD_ACTIVE && req_type != AT_CMD_QUERY)
        return ret;
    
    if(g_ctlw_cloud_buf_list_head == NULL)
    {
        *prsp_cmd = xy_malloc(40);
        sprintf(*prsp_cmd,"+XYRECV:0");
    }

    if(req_type == AT_CMD_ACTIVE)//响应+XYRECV:<mid>,<hex_data>
    {
        cloud_buffer_list_t * recvNodeList = NULL;

        osMutexAcquire(g_ctlw_cloud_buf_mutex, osWaitForever);
        recvNodeList = cloud_pop_bufList_first_node(g_ctlw_cloud_buf_list_head);
        osMutexRelease(g_ctlw_cloud_buf_mutex);

        if(recvNodeList != NULL)
        {
            *prsp_cmd = xy_malloc2(recvNodeList->msg->data_len + 40);

            if(*prsp_cmd == NULL)
            {
                xy_free(recvNodeList->msg);
                xy_free(recvNodeList);
                return ret;
            }

            sprintf(*prsp_cmd,"+XYRECV:%d,%s",recvNodeList->msg->msgid,recvNodeList->msg->data);

            xy_free(recvNodeList->msg);
            xy_free(recvNodeList);
        }
        else
        {
            *prsp_cmd = xy_malloc(40);
            sprintf(*prsp_cmd,"+XYRECV:0,0");
        }
    }
    else if(req_type == AT_CMD_QUERY)
    {
        *prsp_cmd = xy_malloc(40);
        sprintf(*prsp_cmd,"+XYRECV:%d",g_ctlw_cloud_buf_list_head->count);
    }

    ret = CLOUD_PROXY_SUCCESS;
    return ret;
}


