
/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "xy_at_api.h"
#include "xy_utils.h"
#include "xy_system.h"
#include "at_onenet.h"
#include "xy_cis_api.h"
#include "net_app_resume.h"
#include "ps_netif_api.h"
#include "onenet_utils.h"
#include "cloud_proxy.h"

#define CIS_PROXY_TIMEOUT       124*1000//处理超时时间

cloud_proxy_config_t *g_cis_proxy_config = NULL;
osMessageQueueId_t cis_proxy_pkt_q = NULL; //上下行报文消息队列
osMessageQueueId_t cis_proxy_cache_pkt_q = NULL; //缓存消息队列
osMutexId_t g_cis_cache_mutex = NULL;
osSemaphoreId_t g_cis_send_sem = NULL;//发送同步接口信号量
static int  cis_proxy_ackid = 100;//发送上行数据默认ack ID
static int  cis_proxy_work_state = 0;   //0,working;1,closing,2,closed
osThreadId_t g_cis_proxy_downlink_Handle = NULL;
extern onenet_context_reference_t onenet_context_refs[CIS_REF_MAX_NUM];

typedef enum
{
	cis_tpye_notify =0,
	cis_tpye_readrsp ,
	cis_tpye_writersp ,
	cis_tpye_executersp ,
	cis_tpye_dereg ,
	cis_tpye_update ,
}cis_proxy_datatype_e;

typedef struct _xy_cis_cache_list
{
    struct xy_cis_cache_t *next;
    cis_pkt_msg *msg;
} xy_cis_cache_list_t;

typedef struct
{
	xy_cis_cache_list_t *first;
	xy_cis_cache_list_t *last;
    int count;
} xy_cis_cache_list_head_t;

xy_cis_cache_list_head_t *xy_cis_cache_list_head = NULL;

void init_xy_cis_cache_list()
{
    if (xy_cis_cache_list_head != NULL)
    {
        clear_xy_cis_cache_list_head();
    }
    xy_cis_cache_list_head = xy_malloc(sizeof(xy_cis_cache_list_head_t));
    memset(xy_cis_cache_list_head, 0, sizeof(xy_cis_cache_list_head_t));
}

void clear_xy_cis_cache_list_head()
{
    if (xy_cis_cache_list_head == NULL)
    {
        return;
    }
    xy_cis_cache_list_t *node = xy_cis_cache_list_head->first;
    xy_cis_cache_list_t *temp_node;

    while (node != NULL)
    {
        temp_node = node->next;
        if(node->msg != NULL)
        	xy_free(node->msg);
        xy_free(node);
        node = temp_node;
    }

    if(xy_cis_cache_list_head != NULL)
    {
    	xy_free(xy_cis_cache_list_head);
    	xy_cis_cache_list_head = NULL;
    }

}

int xy_cis_msg_cache_exists(int msgid)
{
	xy_cis_cache_list_t *temp = xy_cis_cache_list_head->first;
    while (temp)
    {
        if (temp->msg->msgid == msgid)
        {
            return 1;
        }
        temp = temp->next;
    }
    return 0;
}

int insert_xy_cis_cache_node(xy_cis_cache_list_t *node)
{
    if (xy_cis_cache_list_head == NULL)
    {
        return -1;
    }
    if (xy_cis_cache_list_head->last)
    {
        xy_cis_cache_list_head->last->next = node;
        xy_cis_cache_list_head->last = node;
        xy_cis_cache_list_head->count++;
    }
    else
    {
    	xy_cis_cache_list_head->first = xy_cis_cache_list_head->last = node;
    	xy_cis_cache_list_head->count = 1;
    }
    char *at_str = xy_malloc(32);
	snprintf(at_str, 32, "+XYRECV: buffer");
	send_urc_to_ext(at_str, strlen(at_str));
	xy_free(at_str);

    return 0;
}

xy_cis_cache_list_t *xy_pop_cis_cache_first_node()
{
    if (xy_cis_cache_list_head->count == 0)
    {
        return NULL;
    }
    xy_cis_cache_list_t *node = xy_cis_cache_list_head->first;

    if (xy_cis_cache_list_head->count == 1)
    {
    	xy_cis_cache_list_head->first = xy_cis_cache_list_head->last = NULL;
    }
    else
    {
    	xy_cis_cache_list_head->first = xy_cis_cache_list_head->first->next;
    }
    xy_cis_cache_list_head->count--;
    return node;
}

/**
 * @brief 用户下行数据输处理函数，不能阻塞
 *@param type        	[IN]     云平台下发请求类型
 *@param context        [IN]     云平台下发请求的上下文
 *@param uri            [IN]     云平台下发请求URI
 *@param mid            [IN]     云平台下发请求消息ID
 *@param eid            [IN]     云平台下发请求事件ID
 *@param valueType      [IN]     云平台下发请求数据类型
 *@param value          [IN]     云平台下发请求数据内容
 *@param valueLen       [IN]     云平台下发请求数据长度
 */
void cisProxyDownstreamCb(et_callback_type_t type, void* context,cis_uri_t* uri,cis_mid_t mid, cis_evt_t eid, int valueType,
char* value, int valueLen)
{
	(void) context;
	(void) mid;
	(void) valueType;

	cis_pkt_msg *msg = xy_malloc(sizeof(cis_pkt_msg) + valueLen + 1);
	memset(msg, 0, (sizeof(cis_pkt_msg) + valueLen + 1));
	msg->type = type;
	msg->flag = uri->flag;
	msg->objId = uri->objectId;
	msg->insId = uri->instanceId;
	msg->resId = uri->resourceId;
	msg->evtId = eid;
	msg->msgid = mid;

    if(value !=NULL && valueLen > 0)
    {
        memcpy(msg->data, value, valueLen);
        msg->data_len = valueLen;
    }

	if(cis_proxy_pkt_q == NULL)
		cis_proxy_pkt_q = osMessageQueueNew(16, sizeof(void *), NULL);
	xy_assert(cis_proxy_pkt_q != NULL);

	osMessageQueuePut(cis_proxy_pkt_q, &msg, 0, osWaitForever);
}
/**
 * @brief 读取指定对象实例的指定资源信息
 *@param msgId        	[IN]     云平台下发请求消息ID
 *@param insId        	[IN]     发送数据的对象实例ID
 *@param resId          [IN]     发送数据的资源ID
 *@param index      	[IN]     上行响应的index
 *@param flag           [IN]     上行响应的结束标志
 *@retval #int   0
 */
static int cisReadInstResPro(int msgId,int insId,int resId,int index,int flag)
{
	int errorcode = CIS_RET_ERROR;
	char *at_str = xy_malloc(100);
	int data = 0;
	sprintf(at_str,"%d",data);
	errorcode = cis_read_rsp(msgId, 1, g_cis_proxy_config->objectID, insId, resId, 5, 1, at_str,index, flag);

	if (errorcode != CIS_RET_OK)
		xy_assert(0);
	xy_free(at_str);
	return 0;
}

/**
 * @brief 读取指定对象的所有实例和资源信息
 *@param msg_id        	[IN]     云平台下发请求消息ID
 *@retval #int   0
 */
static int cisReadObjAllPro(int msg_id)
{
	char *lastp = NULL;
	char *buft = g_cis_proxy_config->resStr;
	int i,inst_id,flag,resource_id;
	int index = 7;
	for(inst_id=0;inst_id<g_cis_proxy_config->insID;inst_id++)
	{
		if(index==7)
			flag=1;
		else if(index==0)
			flag=0;
		else
			flag=2;
		for(i=g_cis_proxy_config->resCount;i>=0;i--)
		{
			if(index==7)
				flag=1;
			else if(index==0)
				flag=0;
			else
				flag=2;

			resource_id = (int)strtol(strtok_r(buft, ";", &lastp),NULL,10);
			cisReadInstResPro(msg_id,inst_id,resource_id,index,flag);
			index--;
			buft = NULL;
		}
	}
	return 0;
}

/**
 * @brief 读取指定对象实例的所有资源信息
 *@param msgId        	[IN]     云平台下发请求消息ID
 *@param insId        	[IN]     发送数据的对象实例ID
 *@retval #int   0
 */
static int cisReadInstAllPro(int msg_id,int inst_id)
{
	int index,flag,resource_id;
	char *lastp = NULL;
	char *buft = g_cis_proxy_config->resStr;
	for(index=g_cis_proxy_config->resCount;index>0;index--)
	{
		if(index==g_cis_proxy_config->resCount)
			flag=1;
		else if(index==0)
			flag=0;
		else
			flag=2;
		resource_id = (int)strtol(strtok_r(buft, ";", &lastp),NULL,10);
		cisReadInstResPro(msg_id,inst_id,resource_id,index,flag);
		buft = NULL;
	}
	return 0;
}

/**
 * @brief 处理平台下发的read请求，并向平台发送响应结果
 *@param msgId        	[IN]     云平台下发请求消息ID
 *@param flag        	[IN]     发送数据的URI flag
 *@param objId          [IN]     发送数据的对象ID
 *@param insId          [IN]     发送数据的实例ID
 *@param resId          [IN]     发送数据的资源ID
 *@param result      	[IN]     Read请求的响应结果
 *@retval #int   0
 */
int cisProxyReadRsp(int msgId, int flag, int objId, int insId, int resId, int result)
{
	(void) result;

	xy_printf(0,XYAPP, WARN_LOG, "AP read start");

	if(objId != g_cis_proxy_config->objectID)
		xy_assert(0);
	if((flag & URI_FLAG_INSTANCE_ID) == 0)
	{
		cisReadObjAllPro(msgId);
	}
	else if((flag & URI_FLAG_RESOURCE_ID) == 0)
	{
		cisReadInstAllPro(msgId, insId);
	}
	else
	{
		cisReadInstResPro(msgId, insId, resId, 0, 0);
	}
	return 0;
}

/**
 * @brief 处理平台下发的observe请求，并向平台发送响应结果
 *@param msgId        	[IN]     云平台下发请求消息ID
 *@param flag        	[IN]     发送数据的URI flag
 *@param objId          [IN]     发送数据的对象ID
 *@param insId          [IN]     发送数据的实例ID
 *@param resId          [IN]     发送数据的资源ID
 *@param result      	[IN]     Observe请求的响应结果
 *@retval #int   0
 */
int cisObserveRspAndNotify(int msgId, int flag, int objId, int insId, int resId, int result)
{
	(void) objId;

	//response observe result
	cis_observe_rsp(msgId, result);


	//Notify all instance and all resource of the specified object id
	if((flag & URI_FLAG_INSTANCE_ID) == 0)
	{
		cisNotifyObjAllPro(msgId);
	}
	//Notify all resource of the specified objectID and instance id
	else if((flag & URI_FLAG_RESOURCE_ID) == 0)
	{
		cisNotifyInstAllPro(msgId, insId);
	}
	//Notify the specified resource id
	else
	{
		cisNotifyInstResPro(msgId, insId, resId, 0, 0);
	}

	return 0;
}

/**
 * @brief 处理平台下发数据进行event上报
 *@param event_num        	[IN]     上报的事件ID
 *@retval #int   0 is success; other value is error
 */
static int cisProxyStatusEventProc(int event_num)
{	
	int ret = CIS_RET_OK;
	if(event_num == CIS_EVENT_UNREG_DONE)
	{
		cis_proxy_work_state=2;
		xy_printf(0,XYAPP, WARN_LOG, "AP close success");
	}
	else if(event_num == CIS_EVENT_UPDATE_SUCCESS)
		osSemaphoreRelease(g_cis_send_sem);
	else if(event_num == CIS_EVENT_UPDATE_FAILED)
		osSemaphoreRelease(g_cis_send_sem);
	else if(event_num == CIS_EVENT_REG_TIMEOUT)
		xy_printf(0,XYAPP, WARN_LOG, "AP register timeout");
	else if(event_num == CIS_EVENT_REG_SUCCESS)
		xy_printf(0,XYAPP, WARN_LOG, "AP register success");
	else if(event_num == CIS_EVENT_NOTIFY_SUCCESS)
		osSemaphoreRelease(g_cis_send_sem);
	else if(event_num == CIS_EVENT_NOTIFY_FAILED)
		osSemaphoreRelease(g_cis_send_sem);
	else if(event_num == CIS_EVENT_UPDATE_NEED)
	{
		xy_printf(0,XYAPP, WARN_LOG, "AP need updata");
		if ((ret=cis_updatelife(7200, 0)) != CIS_RET_OK)
		{
			xy_assert(0);
		}		
	}

	return ret;
}


/**
 * @brief Demo任务的详细处理流程
 * @note
 注册完成后会从消息队列中读取消息，根据不同的消息类型进行响应，支持的消息类型请参考et_callback_type_t
 *@retval #void
 */
void cisProxyEventPro()
{
	int ret = CIS_RET_OK;
    cis_pkt_msg *rcv_msg = NULL;
	int evtType = -1;
	static int last_cache_msgid = 0;
	xy_cis_cache_list_t *cache_node = NULL;
	while(cis_proxy_work_state != 2)
	{
		osMessageQueueGet(cis_proxy_pkt_q, &rcv_msg, NULL, osWaitForever);
		evtType	= (int)rcv_msg->type;
		cache_node = xy_malloc(sizeof(xy_cis_cache_list_t));
		cache_node->msg = rcv_msg;
		cache_node->next = NULL;

		xy_printf(0,XYAPP, WARN_LOG, "[CIS_PROXY]work_state(%d),evtType(%d)", cis_proxy_work_state, evtType);
		//when do closing,only can proc CALLBACK_TYPE_EVENT type
		if(cis_proxy_work_state == 1 && evtType != CALLBACK_TYPE_EVENT)
		{
			xy_free(rcv_msg);
			xy_free(cache_node);
			continue;
		}

		switch(evtType)
		{
			case CALLBACK_TYPE_DISCOVER:
				xy_printf(0,XYAPP, WARN_LOG, "[CIS_PROXY]discover flag(%d)", rcv_msg->flag);
				ret = cis_discover_rsp(-1, rcv_msg->objId, 1, strlen(g_cis_proxy_config->resStr), g_cis_proxy_config->resStr);
				xy_free(rcv_msg);
				break;
			case CALLBACK_TYPE_OBSERVE:
				xy_printf(0,XYAPP, WARN_LOG, "[CIS_PROXY]observe flag(%d),objId(%d),insId(%d),resId(%d)", rcv_msg->flag,rcv_msg->objId, rcv_msg->insId, rcv_msg->resId);
				//ret = cisObserveRspAndNotify(-1, rcv_msg->flag, rcv_msg->objId, rcv_msg->insId, rcv_msg->resId, 1);
				cis_observe_rsp(-1, 1);
				xy_free(rcv_msg);
				break;
			case CALLBACK_TYPE_OBSERVE_CANCEL:
				//Process observe cancel event
				xy_printf(0,XYAPP, WARN_LOG, "[CIS_PROXY]Read flag(%d),objId(%d),insId(%d),resId(%d)", rcv_msg->flag, rcv_msg->objId, rcv_msg->insId, rcv_msg->resId);
				cis_observe_rsp(-1, 1);
				xy_free(rcv_msg);
				break;
			case CALLBACK_TYPE_READ:
				xy_printf(0,XYAPP, WARN_LOG, "[CIS_PROXY]read message cache ");
				//相同msgid的message过滤
				if(!xy_cis_msg_cache_exists(rcv_msg->msgid) && last_cache_msgid < rcv_msg->msgid)
				{
					last_cache_msgid = rcv_msg->msgid;
					osMutexAcquire(g_cis_cache_mutex, osWaitForever);
					insert_xy_cis_cache_node(cache_node);
					osMutexRelease(g_cis_cache_mutex);
				}
				break;
			case CALLBACK_TYPE_WRITE:
				xy_printf(0,XYAPP, WARN_LOG, "[CIS_PROXY]write message cache ");
				//相同msgid的message过滤
				if(!xy_cis_msg_cache_exists(rcv_msg->msgid) && last_cache_msgid < rcv_msg->msgid)
				{
					last_cache_msgid = rcv_msg->msgid;
					osMutexAcquire(g_cis_cache_mutex, osWaitForever);
					insert_xy_cis_cache_node(cache_node);
					osMutexRelease(g_cis_cache_mutex);
				}
				cis_write_rsp(-1, 2);
				break;
			case CALLBACK_TYPE_EXECUTE:
				xy_printf(0,XYAPP, WARN_LOG, "[CIS_PROXY]execute message cache ");
				//相同msgid的message过滤
				if(!xy_cis_msg_cache_exists(rcv_msg->msgid) && last_cache_msgid < rcv_msg->msgid)
				{
					last_cache_msgid = rcv_msg->msgid;
					osMutexAcquire(g_cis_cache_mutex, osWaitForever);
					insert_xy_cis_cache_node(cache_node);
					osMutexRelease(g_cis_cache_mutex);
				}
				cis_execute_rsp(-1, 2);
				break;
			case CALLBACK_TYPE_OBSERVE_PARAMS:
				//Process parameter event and send parameterRsp
				xy_printf(0,XYAPP, WARN_LOG, "[CIS_PROXY]Set Param ");
				//相同msgid的message过滤
				if(!xy_cis_msg_cache_exists(rcv_msg->msgid) && last_cache_msgid < rcv_msg->msgid)
				{
					last_cache_msgid = rcv_msg->msgid;
					osMutexAcquire(g_cis_cache_mutex, osWaitForever);
					insert_xy_cis_cache_node(cache_node);
					osMutexRelease(g_cis_cache_mutex);
				}
				ret = cis_rsp_withparam(-1, 2);
				xy_free(rcv_msg);
				break;
			case CALLBACK_TYPE_EVENT:
				xy_printf(0,XYAPP, WARN_LOG, "[CIS_PROXY]Event(%d) ", rcv_msg->evtId);
				//Process event inform and try to update
				ret = cisProxyStatusEventProc(rcv_msg->evtId);	
				break;
			default:
				xy_free(rcv_msg);
				break;
		}

		if(ret != CIS_RET_OK)
			xy_assert(0);

	}

	/***去注册，该线程退出，释放资源***/
	if(cis_proxy_pkt_q != NULL)
	{
		osMessageQueueDelete(cis_proxy_pkt_q);
		cis_proxy_pkt_q = NULL;
	}

	last_cache_msgid = 0;
	if(g_cis_send_sem != NULL)
	{
		osSemaphoreDelete(g_cis_send_sem);
		g_cis_send_sem = NULL;
	}

	cloud_mutex_destroy(&g_cis_cache_mutex);

	//清除缓存链表
	clear_xy_cis_cache_list_head();

	cisProxyConfigDeInit();
	g_cis_proxy_downlink_Handle = NULL;
	cis_proxy_work_state = 0;

	osThreadExit();
}

/**
 * @brief 创建onenet套件，添加了配置的对象并向云平台发起注册
 *@retval #int   0 is success; -1 is error
 */
 int cisProxyContextInit()
{
	char insBitmap[8]={0};
	int ret = CIS_RET_ERROR;
	cis_cloud_setting(g_cis_proxy_config->serverIP, g_cis_proxy_config->serverPort, g_cis_proxy_config->auth_code);

	ret = cis_create((char *)(g_cis_proxy_config->serverIP), (unsigned int)(g_cis_proxy_config->serverPort), g_cis_proxy_config->bsEnable, g_cis_proxy_config->auth_code);
	if(ret != CIS_RET_OK)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[CIS_PROXY]Err: Create cis failed");
		goto failed_out;
	}

	//sprintf(insBitmap,"%d",g_cis_proxy_config->insID);
	sprintf(insBitmap,"%d",1);			//目前只支持单实例(缺少insCount)
	ret = cis_addobj(g_cis_proxy_config->objectID, 1, insBitmap, g_cis_proxy_config->resCount, 0);
	if(ret != CIS_RET_OK)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[CIS_PROXY]Err: Add obj(%d) failed", g_cis_proxy_config->objectID);
		goto failed_out;
	}

	ret = cis_reg(g_cis_proxy_config->lifetime);
	if(ret != CIS_RET_OK)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[CIS_PROXY]Err: register cis failed");
		goto failed_out;
	}

	return XY_OK;

failed_out:
	cis_delete();
	return XY_ERR;
}

int cisProxyReg(void)
{
	if(cis_proxy_pkt_q == NULL)
		cis_proxy_pkt_q = osMessageQueueNew(16, sizeof(void *), NULL);

	if(g_cis_send_sem == NULL)
		g_cis_send_sem = osSemaphoreNew(0xFFFF, 0, NULL);

	init_xy_cis_cache_list();		//缓存链表初始化

	cloud_mutex_create(&g_cis_cache_mutex);

	//注册下行报文回调函数
	cis_set_downstream_cb(cisProxyDownstreamCb);

    if(g_cis_proxy_downlink_Handle == NULL)
    {
        osThreadAttr_t thd_attr = {0};
        
        thd_attr.name = "cis_proxy_downlink_task";
        thd_attr.stack_size = osStackShared;
        thd_attr.priority = osPriorityNormal1;
        
		g_cis_proxy_downlink_Handle = osThreadNew ((osThreadFunc_t)(cisProxyEventPro),NULL,&thd_attr);
    }
	//start onenet task
	if(cisProxyContextInit() != XY_OK)
	{
		goto FAIL;
	}
	return osSemaphoreAcquire(g_cis_send_sem, CIS_PROXY_TIMEOUT);
FAIL:
    //删除下行报文回调函数
    cis_set_downstream_cb(NULL);
    return XY_ERR;
}

void cisProxyConfigInit(uint8_t* at_buf)
{
	if(g_cis_proxy_config == NULL)
	{
		g_cis_proxy_config = xy_malloc(sizeof(cloud_proxy_config_t));
		g_cis_proxy_config->auth_code = xy_malloc(strlen(at_buf));
		g_cis_proxy_config->resStr = xy_malloc(strlen(at_buf));
		g_cis_proxy_config->pskID = xy_malloc(strlen(at_buf));
		g_cis_proxy_config->psk = xy_malloc(strlen(at_buf));

		g_cis_proxy_config->bsEnable = 0;	//引导模式
		g_cis_proxy_config->lifetime = 86400;
		g_cis_proxy_config->serverPort = 5683;
		g_cis_proxy_config->objectID = 3308;
		g_cis_proxy_config->insID = 0;		//不需要，要的是bitmap跟inscount
		g_cis_proxy_config->resCount = 1;
		strcpy(g_cis_proxy_config->resStr,"5750");
	}

}

void cisProxyConfigDeInit()
{
	if(g_cis_proxy_config != NULL)
	{
		xy_free(g_cis_proxy_config->auth_code);
		xy_free(g_cis_proxy_config->resStr);
		xy_free(g_cis_proxy_config->pskID);
		xy_free(g_cis_proxy_config->psk);
		xy_free(g_cis_proxy_config);
		g_cis_proxy_config = NULL;
	}
}

proxy_config_callback cisProxyConfigProc(uint8_t req_type,uint8_t* paramList, uint8_t **prsp_cmd)
{
	cis_module_init();
	cisProxyConfigInit(paramList);
	if (req_type == AT_CMD_REQ)
	{
	    if (at_parse_param(",%s,%d,%d", paramList,g_cis_proxy_config->serverIP, &g_cis_proxy_config->serverPort, &g_cis_proxy_config->lifetime) != AT_OK)
	    {
			cisProxyConfigDeInit();
			return XY_ERR;
	    }
		return cisProxyReg();
	}
	else if(req_type == AT_CMD_QUERY)
	{
	    *prsp_cmd = xy_malloc(40);
		if(onenet_context_refs[0].onenet_context != NULL && onenet_context_refs[0].onenet_context->stateStep == PUMP_STATE_READY)
		{
			sprintf(*prsp_cmd, "+XYCONFIG:success");
		}
		else
			sprintf(*prsp_cmd, "+XYCONFIG:fail");
	}
	else
	{
		xy_printf(0,XYAPP, WARN_LOG, "[CIS_PROXY]err req_type ");
		return XY_ERR;
	}
	return XY_OK;
}

proxy_recv_callback cisProxyRecvProc(uint8_t req_type,uint8_t* paramList, uint8_t **prsp_cmd)
{
	xy_cis_cache_list_t *tempNode = NULL;
	if(xy_cis_cache_list_head == NULL)
		return XY_ERR;
	cis_module_init();

	if (req_type == AT_CMD_ACTIVE)
	{
		if(xy_cis_cache_list_head->count == 0)
		{
		    *prsp_cmd = xy_malloc(40);
			sprintf(*prsp_cmd, "+XYRCEV:0");
		}
		else
		{
			osMutexAcquire(g_cis_cache_mutex, osWaitForever);
			tempNode = xy_pop_cis_cache_first_node();
			osMutexRelease(g_cis_cache_mutex);

			*prsp_cmd = xy_malloc(40 + tempNode->msg->data_len);
			snprintf(*prsp_cmd, 40 + tempNode->msg->data_len, "+XYRCEV:%d,%d,%d,%s",xy_cis_cache_list_head->count,tempNode->msg->type,tempNode->msg->data_len,tempNode->msg->data);
			xy_free(tempNode->msg);
			xy_free(tempNode);
		}
	}
	else if(req_type == AT_CMD_QUERY)
	{
	    *prsp_cmd = xy_malloc(40);
		sprintf(*prsp_cmd, "+XYRCEV:%d",xy_cis_cache_list_head->count);
	}
	else
	{
		xy_printf(0,XYAPP, WARN_LOG, "[CIS_PROXY] err req_type ");
		return XY_ERR;
	}

    return XY_OK;
}


proxy_send_callback cisProxySendProc(uint8_t req_type,uint8_t* paramList, uint8_t **prsp_cmd)
{
	int ret = XY_ERR ;
	int dataType = -1;
	int dataLen = -1;
	int raiFlag = -1;
	int resID = -1;//todo  多资源场景指定资源
	char *data = xy_malloc(strlen(paramList));
	char *lastp = NULL;
	char *buft = g_cis_proxy_config->resStr;
	cis_module_init();

	if(g_cis_proxy_downlink_Handle == NULL)
		return ret;

	if (req_type == AT_CMD_REQ)
	{
		if (at_parse_param("%d,%d,%s,%d", paramList,&dataType, &dataLen, data, &resID,&raiFlag) != AT_OK || dataType == -1 ||(dataLen!=-1 && data == NULL))
		{
			return XY_ERR;
		}

		if(resID == -1)
		{
			if(strchr(buft,";") != NULL)
				resID = (int)strtol(strtok_r(buft, ";", &lastp),NULL,10);
			else
				resID = (int)strtol(buft, NULL, 10);
		}

		switch(dataType)
		{
			case cis_tpye_notify:
				xy_printf(0,XYAPP, WARN_LOG, "[CIS_PROXY]Notify ");
				//Process send notify event
				cis_notify_sync(-1, g_cis_proxy_config->objectID, g_cis_proxy_config->insID, resID, 1,dataLen,data, 0, 0, cis_proxy_ackid++);
				ret = osSemaphoreAcquire(g_cis_send_sem, CIS_PROXY_TIMEOUT);
				break;
			case cis_tpye_readrsp:
				ret = cisProxyReadRsp(-1, 3, g_cis_proxy_config->objectID, g_cis_proxy_config->insID, resID, 1);
				break;
			case cis_tpye_writersp:
				ret = cis_write_rsp(-1, 2);
				break;
			case cis_tpye_executersp:
				ret = cis_execute_rsp(-1, 2);
				break;
			case cis_tpye_dereg:
				//Process
				cis_proxy_work_state = 1;
				//do delete
				ret = cis_delete();
				if (ret != CIS_RET_OK)
				{
					xy_assert(0);
				}
				break;
			case cis_tpye_update:
				cis_updatelife(g_cis_proxy_config->lifetime,0);
				ret = osSemaphoreAcquire(g_cis_send_sem, CIS_PROXY_TIMEOUT);
				break;
			default:
				break;
		}
	}
	else
		xy_printf(0,XYAPP, WARN_LOG, "[CIS_PROXY]err req_type ");

    return ret;
}

