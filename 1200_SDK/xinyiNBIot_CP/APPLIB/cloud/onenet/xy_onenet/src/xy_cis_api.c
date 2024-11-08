
/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/

#include "xy_utils.h"
#include "xy_system.h"
#include "cloud_utils.h"
#include "at_config.h"
#include "oss_nv.h"
#include "at_onenet.h"
#include "xy_cis_api.h"
#include "net_app_resume.h"
#include "ps_netif_api.h"
#include "onenet_utils.h"

/*******************************************************************************
 *						  Local function declarations						   *
 ******************************************************************************/

/*******************************************************************************
 *						   Local variable definitions						   *
 ******************************************************************************/
extern onenet_context_config_t onenet_context_configs[CIS_REF_MAX_NUM];
extern onenet_context_reference_t onenet_context_refs[CIS_REF_MAX_NUM];
extern onenet_config_nvm_t *g_onenet_config_data;
extern osMutexId_t g_onenet_mutex;
extern osSemaphoreId_t g_cis_del_sem;
osMutexId_t g_onenet_module_init_mutex = NULL;
//Onenet net config paras. Can not be deleted.
//User can make to order but it is recommended to use defalut value
unsigned int g_cis_netmtu 		= 1024;
unsigned int g_cis_netlinktype  = 1;
unsigned int g_cis_netbandtype  = 1;
char *g_cis_netapn = "CMIOT";
uint8_t g_cis_rai = 0;                     //维持API格式不变，做数据中转
osMessageQueueId_t cis_pkt_msg_q = NULL;   //上下行报文消息队列
cis_downstream_callback g_cis_downstream_cb = NULL;
osSemaphoreId_t g_cis_notify_sem = NULL;

/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/

/*******************************************************************************
 *						Inline function implementations 					   *
 ******************************************************************************/

/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/
 /*
 * description: Get the first msgid in the request list for the spiciefied uri
 * in: 
 *           context: onenet context
 *           uri:	  discover response uri
 * out:      
 *
 * return:
 * 			 success: XY_OK
 * 			 fail:	  XY_ERR
 *
 */
static int xy_get_DiscoverMsgId(st_context_t *context, cis_uri_t *uriP)
{
	st_request_t * targetRequest = NULL;
	int msg_id = -1;
	
	if(context == NULL)
		return XY_ERR;
	
	/*check request node list*/
	cloud_mutex_lock(&context->lockRequest,MUTEX_LOCK_INFINITY);
	targetRequest = context->requestList;
	cloud_mutex_unlock(&context->lockRequest);
	if(targetRequest == NULL)
		return XY_ERR;
	
	while(targetRequest != NULL)
	{
		if(uri_exist(&targetRequest->uri,uriP) && targetRequest->type == CALLBACK_TYPE_DISCOVER)
		{
			msg_id = targetRequest->mid;
			break;
		}
		else
		{
			cloud_mutex_lock(&context->lockRequest,MUTEX_LOCK_INFINITY);
			targetRequest = targetRequest->next;
			cloud_mutex_unlock(&context->lockRequest);
			continue;
		}
	}
	
	return msg_id;
}

/*
* description: Get the first msgid in the observed list for the spiciefied uri
* in: 
*			context: onenet context
*			uri:	 observe response uri
* out:		
*
* return:
*			success: XY_OK
*			fail:	 XY_ERR
*
*/
int xy_get_observeMsgId(st_context_t *contextP, cis_uri_t *uriP)
{
    st_observed_t * targetP = NULL;
	st_request_t * targetRequest = NULL;
	int msg_id = -1;
	bool is_find = false;
	
    targetP = contextP->observedList;
    while (targetP != NULL)
    {
		CIS_LOG_URI("[CIS]observe uri", &targetP->uri);
		if(!uri_exist(&targetP->uri,uriP))
		{
        	targetP = targetP->next;		
		}
		else
		{
			msg_id = targetP->msgid;
			is_find = true;
			break;
		}
    }

	if(!is_find)
	{
		/*check request node list*/
		cloud_mutex_lock(&contextP->lockRequest,MUTEX_LOCK_INFINITY);
		targetRequest = contextP->requestList;
		cloud_mutex_unlock(&contextP->lockRequest);
		if(targetRequest == NULL)
			return XY_ERR;
		
		while(targetRequest != NULL)
		{
			CIS_LOG_URI("[CIS]request uri", &targetRequest->uri);
			if(uri_exist(&targetRequest->uri,uriP) && targetRequest->type == CALLBACK_TYPE_OBSERVE)
			{
				msg_id = targetRequest->mid;
				is_find = true;
				break;	
			}
			else
			{
				cloud_mutex_lock(&contextP->lockRequest,MUTEX_LOCK_INFINITY);
				targetRequest = targetRequest->next;	
				cloud_mutex_unlock(&contextP->lockRequest);
				continue;
			}
		}		
	}

	return msg_id;
} 

/*
* description: Get the first msgid in the request list for the spiciefied reqType
* in: 
*			context: onenet context
*			uri:	 discover response uri
* out:		
*
* return:
*			success: XY_OK
*			fail:	 XY_ERR
*
*/
static int xy_get_reqMsgId(st_context_t *context, int reqType)
{
	st_request_t * targetRequest = NULL;
	int msg_id = -1;
	
	if(context == NULL)
		return XY_ERR;
	
	/*check request node list*/
	cloud_mutex_lock(&context->lockRequest,MUTEX_LOCK_INFINITY);
	targetRequest = context->requestList;
	cloud_mutex_unlock(&context->lockRequest);
	if(targetRequest == NULL)
		return XY_ERR;
	
	while(targetRequest != NULL)
	{
		if(targetRequest->type == reqType)
		{
			msg_id = targetRequest->mid;
			break;
		}
		else
		{
			cloud_mutex_lock(&context->lockRequest,MUTEX_LOCK_INFINITY);
			targetRequest = targetRequest->next;
			cloud_mutex_unlock(&context->lockRequest);
			continue;
		}
	}
	
	return msg_id;
}

/*
* description: copy value to the read response data
* in: 
*			value:   
*
* out:		
*			dst:	 response data for read request
*
* return:
*			success: XY_OK
*			fail:	 XY_ERR
*
*/
static int xy_get_read_value(char *value, struct onenet_read *dst)
{
	
	if(value == NULL || strlen(value) == 0 || dst == NULL)
		goto  ERR_PROC;
	
	if (dst->value_type == cis_data_type_integer) 
	{
		dst->value = xy_malloc(strlen(value) + 1);
		strcpy(dst->value,value);
		
		if(((int)strtol(dst->value,NULL,10) >= INT16_MIN) && ((int)strtol(dst->value,NULL,10) <= INT16_MAX))
		{
			dst->len = 2;
		}
		else if(((long long)strtoll(dst->value,NULL,10) >= INT32_MIN) && ((long long)strtoll(dst->value,NULL,10) <= INT32_MAX))
		{
			dst->len = 4;
		}
		else if(((long long)strtoll(dst->value,NULL,10) >= INT64_MIN) && ((long long)strtoll(dst->value,NULL,10) <= INT64_MAX))
		{
			dst->len = 8;
		}
		else
			goto  ERR_PROC;

	} 
	else if (dst->value_type == cis_data_type_float) 
	{
		dst->value = xy_malloc(strlen(value) + 1);
		strcpy(dst->value,value);
		if(atof(dst->value) <= FLT_MAX)
		{
			dst->len = 4;
		}
		else if(atof(dst->value) <= DBL_MAX)
		{
			dst->len = 8;
		}
		else
			goto  ERR_PROC;

	} 
	else if (dst->value_type == cis_data_type_bool) 
	{
		dst->value = xy_malloc(strlen(value) + 1);
		strcpy(dst->value,value);
		if((int)strtol(dst->value,NULL,10) > 1 || (int)strtol(dst->value,NULL,10) < 0)
			goto  ERR_PROC;
		
		dst->len = 1;


	} 
	else if (dst->value_type == cis_data_type_string)	
	{
		dst->len = strlen(value);
		dst->value = xy_malloc(dst->len + 1);
		strcpy(dst->value,value);
	} 
	else if (dst->value_type == cis_data_type_opaque) 
	{
		if(strlen(value)%2 != 0)
			goto  ERR_PROC;
		else
			dst->len = strlen(value) / 2;

		dst->value = xy_malloc(dst->len + 1);
		if (hexstr2bytes(value, strlen(value), dst->value, dst->len) == XY_ERR)
		{
			goto  ERR_PROC;
		}
		dst->value[dst->len] = '\0';
	}
	else 
	{
		goto  ERR_PROC;
	}

	return XY_OK;
ERR_PROC:	
	if(dst != NULL && dst->value != NULL)
	{
		xy_free(dst->value);
		dst->value = NULL;
	}
	return XY_ERR;
}

/*
* description: copy value to the notify response data
* in: 
*			value:   
*
* out:		
*			dst:	 notify data for observe request
*
* return:
*			success: XY_OK
*			fail:	 XY_ERR
*
*/
static int xy_get_notify_value(char* value, struct onenet_notify *dst)
{
	if(value == NULL || strlen(value) == 0 || dst == NULL)
		goto  ERR_PROC;
	
	if (dst->value_type == cis_data_type_integer) 
	{
		dst->value = xy_malloc(strlen(value) + 1);
		strcpy(dst->value,value);
		if(((int)strtol(dst->value,NULL,10) >= INT16_MIN) && ((int)strtol(dst->value,NULL,10) <= INT16_MAX))
		{
			dst->len = 2;
		}
		else if(((int)strtol(dst->value,NULL,10) >= INT32_MIN) && ((int)strtol(dst->value,NULL,10) <= INT32_MAX))
		{
			dst->len = 4;
		}
		else if(((int)strtol(dst->value,NULL,10) >= INT64_MIN) && ((int)strtol(dst->value,NULL,10) <= INT64_MAX))
		{
			dst->len = 8;
		}
		else
			goto  ERR_PROC;

	} 
	else if (dst->value_type == cis_data_type_float) 
	{
		dst->value = xy_malloc(strlen(value) + 1);
		strcpy(dst->value,value);
		
		if(atof(dst->value) <= FLT_MAX)
		{
			dst->len = 4;
		}
		else if(atof(dst->value) <= DBL_MAX)
		{
			dst->len = 8;
		}
		else
			goto  ERR_PROC;

	} 
	else if (dst->value_type == cis_data_type_bool) 
	{
		dst->value = xy_malloc(strlen(value) + 1);
		strcpy(dst->value,value);
		if((int)strtol(dst->value,NULL,10) > 1 || (int)strtol(dst->value,NULL,10) < 0)
			goto  ERR_PROC;
		
		dst->len = 1;


	} 
	else if (dst->value_type == cis_data_type_string) 	
	{
		dst->len = strlen(value);
		dst->value = xy_malloc(dst->len + 1);
		strcpy(dst->value,value);
	} 
	else if (dst->value_type == cis_data_type_opaque) 
	{
		dst->value = xy_malloc(dst->len + 1);
		if (hexstr2bytes(value, strlen(value), dst->value, dst->len) == XY_ERR)
		{
			goto  ERR_PROC;
		}
		dst->value[dst->len] = '\0';
 	}
	else 
	{
		goto  ERR_PROC;
	}

	return XY_OK;
ERR_PROC:	
	if(dst != NULL && dst->value != NULL)
	{
		xy_free(dst->value);
		dst->value = NULL;
	}
	return XY_ERR;
}

static unsigned short bswap_16(unsigned short x)
{
    return (((unsigned short)(x) & 0x00ff) << 8) | \
           (((unsigned short)(x) & 0xff00) >> 8) ;
}

//将下行数据添加至队列，该回调不能阻塞
void cis_downstream_cb(et_callback_type_t type, void* context,cis_uri_t* uri,cis_mid_t mid, cis_evt_t eid, int valueType,
char* value, int valueLen)
{
    (void) context;
    (void) mid;
    (void) valueType;

    if(cis_pkt_msg_q == NULL)
        return;

    cis_pkt_msg *msg =NULL;
    msg = xy_malloc(sizeof(cis_pkt_msg) + valueLen + 1);
    msg->type = type;
    msg->flag = uri->flag;
    msg->objId = uri->objectId;
    msg->insId = uri->instanceId;
    msg->resId = uri->resourceId;
    msg->evtId = eid;
	msg->data_len = 0;
	msg->index = 0;
	msg->valueType = 0;
	msg->data[0] = 0;

    if(value !=NULL && valueLen > 0)
    {
        memcpy(msg->data, value, valueLen);
		msg->data[valueLen] = '\0';
		msg->data_len = valueLen;
    }

    if(cis_pkt_msg_q == NULL)
        cis_pkt_msg_q = osMessageQueueNew(16, sizeof(void *), NULL);
    xy_assert(cis_pkt_msg_q != NULL);
    osMessageQueuePut(cis_pkt_msg_q, &msg, 0, osWaitForever);
}

//CIS处理所有类型下行报文的回调
void  cis_set_downstream_cb(cis_downstream_callback downstream_cb)
{
    g_cis_downstream_cb = downstream_cb;
}

static int cis_cfg_set_len_and_value(char *cur, char *data)
{
	unsigned short tmp = 0;
	if (data != NULL)
	{
		tmp = bswap_16(strlen(data));
		memcpy(cur, &tmp, 2);
		cur += 2;
		memcpy(cur, data, strlen(data));
		return 2 + strlen(data);
	}
	else
	{
		memcpy(cur, &tmp, 2);
		return 2;
	}
}
char* cis_cfg_tool(char* ip,unsigned int port,char is_bs,char* authcode,char is_dtls,char* psk,int *cfg_out_len)
{
	// head
	char version = 1;
	char cfgcnt = 3;

	// item id 1

	// item id 2
	unsigned short Mtu = 1024;
	char Linktype = 1;
	char Bandtype = 1;
	char BootstrapEnabled = is_bs;
	char DTLSEnabled = is_dtls;
	char *APN= "CMIOT";
	char *Username = NULL;
	char *Password = NULL;
    char *Host = NULL;
    char *Userdata2 = NULL;
    Host = xy_malloc(strlen(ip) + 7);         //ip_len+':'+port+'\0'   len + 1 + 5 + 1
    sprintf(Host, "%s:%d", ip, port);
	//char *Host = "183.230.40.39:5684";
	Userdata2 = xy_malloc(80);
	if(Userdata2 != NULL && authcode!=NULL && psk !=NULL)
	    sprintf(Userdata2, "AuthCode:%s;PSK:%s;", authcode,psk);
	else if (authcode !=NULL)
	    sprintf(Userdata2, "AuthCode:%s;PSK:;", authcode);
    else if (psk !=NULL)
        sprintf(Userdata2, "AuthCode:;PSK:%s;", psk);
    else
        sprintf(Userdata2, "AuthCode:;PSK:;");

//	char *Userdata2 = "AuthCode:;PSK:;";

	// item id 3
	char LogEnabled = 1;
	char LogExtOutput = 1;
	char LogOutputType = 2;
	char LogOutputLevel = 4;

	unsigned short LogBufferSize = 200;
	char *Userdata3 = NULL;


	int len = 32;
	char *res = NULL;
	//int index = 0;
	unsigned short total_len;
	unsigned char *total_len_p = NULL;
	unsigned short sub_len;
	unsigned char *sub_len_p = NULL;
	char *cur = NULL;
	short tmp = 0;
	int ret;

	if (APN != NULL)
		len += strlen(APN);
	if (Username != NULL)
		len += strlen(Username);
	if (Password != NULL)
		len += strlen(Password);
	if (Host != NULL)
		len += strlen(Host);
	if (Userdata2 != NULL)
		len += strlen(Userdata2);
	if (Userdata3 != NULL)
		len += strlen(Userdata3);

	res = xy_malloc(len);
	cur = res;
	*cur = (version << 4) + cfgcnt;
	total_len_p = cur + 1;
	total_len = 3;
	cur += 3;
	// item id 1
	*cur = 0xf1;
	sub_len_p = cur + 1;
	sub_len = bswap_16(3);
	memcpy(sub_len_p, &sub_len, 2);
	cur += 3;
	total_len += 3;
	// item id 2
	*cur = 0xf2;
	sub_len_p = cur + 1;
	sub_len = 3;
	cur += 3;
	tmp = bswap_16(Mtu);
	memcpy(cur, &tmp, 2);
	cur += 2;
	*cur = (Linktype << 4) + Bandtype;
	cur ++;
	*cur = (BootstrapEnabled << 7) + (DTLSEnabled << 6);
	cur ++;

	sub_len += 4;
	ret = cis_cfg_set_len_and_value(cur, APN);
	sub_len += ret;
	cur += ret;
	ret = cis_cfg_set_len_and_value(cur, Username);
	sub_len += ret;
	cur += ret;
	ret = cis_cfg_set_len_and_value(cur, Password);
	sub_len += ret;
	cur += ret;
	ret = cis_cfg_set_len_and_value(cur, Host);
	sub_len += ret;
	cur += ret;
	ret = cis_cfg_set_len_and_value(cur, Userdata2);
	sub_len += ret;
	cur += ret;
	total_len += sub_len;
	sub_len = bswap_16(sub_len);
	memcpy(sub_len_p, &sub_len, 2);

	// item id 3
	*cur = 0xf3;
	sub_len_p = cur + 1;
	sub_len = 3;
	cur += 3;
	*cur = (((LogEnabled << 3) + (LogExtOutput << 2) + LogOutputType) << 4) + LogOutputLevel;
	cur ++;
	tmp = bswap_16(LogBufferSize);
	memcpy(cur, &tmp, 2);
	cur += 2;
	sub_len += 3;
	ret = cis_cfg_set_len_and_value(cur, Userdata3);
	sub_len += ret;
	cur += ret;
	total_len += sub_len;
	sub_len = bswap_16(sub_len);
	memcpy(sub_len_p, &sub_len, 2);

	*cfg_out_len = total_len;
	total_len = bswap_16(total_len);
	memcpy(total_len_p, &total_len, 2);

    if(Host != NULL)
        xy_free(Host);
    if(Userdata2 != NULL)
        xy_free(Userdata2);

	return res;
}

void cis_set_rai_flag(int value)
{
    g_cis_rai = value;
}

/*
* description: Create onenet instance 
* in: 
*			ip:     server ip
*			port:   server port
*			is_bs:  bootstrap is enable or not
*
* return:
*			success: XY_OK
*			fail:	 XY_Err_NoConnected
*                    XY_Err_Parameter
*                    XY_ERR
*
*/
int cis_create(char *ip, unsigned int port, int is_bs, char* auth)
{
	char* config = NULL;
	int cfg_len  = 0;
    int ret      = CIS_RET_ERROR;
	onenet_context_reference_t *contextRef = NULL;
	onenet_context_config_t *onenet_context_config = NULL;
	osThreadAttr_t task_attr = {0};
	
    cis_module_init();
	if(!xy_tcpip_is_ok()) {
		return XY_Err_NoConnected;
	}

	onenet_resume_session();

	if(is_onenet_task_running(0))
	{
		xy_printf(0,XYAPP, WARN_LOG, "[CIS] Onenet is running.....");
		return XY_OK;
	}
		
	if(ip==NULL || strlen(ip)==0 || port <= 0 || port > 65535)
		return XY_Err_Parameter;

	if(auth==NULL || strlen(auth)==0)
		config = cis_cfg_tool(ip, port, is_bs, "",false,"",&cfg_len);
	else
		config = cis_cfg_tool(ip, port, is_bs, auth,false,"",&cfg_len);

	if(cfg_len == 0)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[CIS] Onenet config error");
		return XY_Err_Parameter;
	}		

	onenet_context_config = (onenet_context_config_t *)find_proper_onenet_context_config(cfg_len, 0, cfg_len);
	if (onenet_context_config == NULL)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[CIS] no free onenet_context_configs.");
		if (config != NULL)
			xy_free(config);
		return  XY_ERR;
	}

	if (onenet_context_config->config_hex == NULL)
	{
	   memset(onenet_context_config, 0, sizeof(onenet_context_config_t));
	   onenet_context_config->config_hex = xy_malloc(cfg_len);
	   onenet_context_config->total_len = cfg_len;
	}

	memcpy(onenet_context_config->config_hex, config, cfg_len);
	onenet_context_config->index = 0;

	contextRef = (onenet_context_reference_t *)get_free_onet_context_ref();
	if (contextRef == NULL) {
		xy_printf(0,XYAPP, WARN_LOG, "[CIS] no free onenet_context_refs.");
		if (config != NULL)
			xy_free(config);
		return XY_ERR;
	}

	ret = onet_init(contextRef, onenet_context_config);
	if(ret != CIS_RET_OK)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[CIS] init failed.");
		if (config != NULL)
			xy_free(config);
		return XY_ERR;
	}

	task_attr.name = "onenet_tk";
	task_attr.priority = osPriorityNormal1;
	task_attr.stack_size = osStackShared;
	contextRef->onet_at_thread_id = osThreadNew((osThreadFunc_t)(onet_at_pump), contextRef, &task_attr);

	//store factory_nv
    memcpy(g_onenet_config_data->server_host, ip, sizeof(g_onenet_config_data->server_host));
    g_onenet_config_data->server_port = port;
    g_onenet_config_data->bs_enable = is_bs;
    memcpy(g_onenet_config_data->auth_code, auth, sizeof(g_onenet_config_data->auth_code));
    cloud_save_file(ONENET_CONFIG_NVM_FILE_NAME,(void*)g_onenet_config_data,sizeof(onenet_config_nvm_t));

    if (config != NULL)
        xy_free(config);
	return XY_OK;
}

et_client_state_t cis_get_context_state()
{
    onenet_context_reference_t *onenet_context_ref = NULL;
	//deepsleep recovery
    onenet_resume_session();

	onenet_context_ref = &onenet_context_refs[0];

    if (onenet_context_ref->onenet_context == NULL || onenet_context_ref->onet_at_thread_id == NULL)
    {
        return PUMP_STATE_HALT;
    }

    return onenet_context_ref->onenet_context->stateStep;
}

et_status_t cis_get_server_status()
{
    st_server_t * targetP = NULL;
    onenet_context_reference_t *onenet_context_ref = NULL;

	//deepsleep recovery
    onenet_resume_session();

	onenet_context_ref = &onenet_context_refs[0];

    if (onenet_context_ref->onenet_context == NULL || onenet_context_ref->onet_at_thread_id == NULL)
    {
        return STATE_UNCREATED;
    }

    targetP = onenet_context_ref->onenet_context->server;
	if(targetP != NULL)
	{
		return targetP->status;
	}else{
		return STATE_UNCREATED;
	}	
}

/*****************************************************************************
 Function    : onenet_checkObserve
 Description : check context state, server status and observe list
 Input       : objId -observe objid
               insId -observe instID
 			   resId -observe resID
 Output      : None
 Return      : XY_OK success    (can notify data for specifed objid/insId/resId)
               XY_ERR error
 *****************************************************************************/
int cis_check_observe(int objId, int insId, int resId)
{
    cis_uri_t uri = {0};
    st_observed_t * targetP = NULL;
    st_server_t * serverP = NULL;
	onenet_context_reference_t *onenet_context_ref = NULL;

	//deepsleep recovery
	onenet_resume_session();

	onenet_context_ref = &onenet_context_refs[0];

	if (onenet_context_ref->onenet_context == NULL || onenet_context_ref->onet_at_thread_id == NULL || onenet_context_ref->onenet_context->stateStep != PUMP_STATE_READY)
	{
		return XY_ERR;
	}

    serverP = onenet_context_ref->onenet_context->server;
	if(serverP == NULL || serverP->status != STATE_REGISTERED)
	{
		return XY_ERR;
	}

	uri.objectId = objId;
	uri.instanceId = insId;
	uri.resourceId = resId;
	cis_uri_update(&uri);	
    targetP = onenet_context_ref->onenet_context->observedList;
    while (targetP != NULL)
    {
		if(!uri_exist(&targetP->uri,&uri))
		{
        	targetP = targetP->next;		
		}
		else
		{
			return XY_OK;
		}
    }

	return XY_ERR;
}

/*
* description: Delete onenet instance 
* in: 
*			ip:       server ip
*			port:     server port
*			is_bs:    bootstrap is enable or not
*			authcode: authcode is the same with the device registerd on the onenet cloud
*			psk:      psk value is the same with the device registerd on the onenet cloud
*
* return:
*			success: XY_OK
*			fail:	 XY_ERR
*
*/
int cis_delete(void)
{
    cis_module_init();
    onenet_resume_session();

	if (!is_onenet_task_running(0)) {
		if(onenet_context_configs[0].config_hex != NULL)
		{
			xy_free(onenet_context_configs[0].config_hex);
			memset(&onenet_context_configs[0], 0, sizeof(onenet_context_config_t));
			return XY_OK;
		}
		else
			return XY_ERR;
	}
	
	osMutexAcquire(g_onenet_mutex, osWaitForever);
	cis_unregister(onenet_context_refs[0].onenet_context);
    osMutexRelease(g_onenet_mutex);
    while (onenet_context_refs[0].onenet_context != NULL && onenet_context_refs[0].onenet_context->registerEnabled != 0)
    {
		osSemaphoreAcquire(g_cis_del_sem, osWaitForever);
    }

	onet_deinit(&onenet_context_refs[0]);
	if(g_cis_rai != 0)
        cis_set_rai_flag(0);

	return XY_OK;
}

/*
* description: Register onenet instance to onenet cloud 
* in: 
*			lifetime:    onenet instance lifetime
*
* return:
*			success: XY_OK
*			fail:	 XY_Err_NoConnected
*			         XY_Err_NotAllowed
*                    XY_Err_Parameter
*                    XY_ERR
*
*/
int cis_reg(int lifetime)
{
	int errorcode = CIS_RET_ERROR;
	if(lifetime < 120 && lifetime != 0)
		goto param_error;
	
    cis_module_init();
	if(!xy_tcpip_is_ok()) {
		return XY_Err_NoConnected;
	}

	onenet_resume_session();
	
	if (!is_onenet_task_running(0)) {
        return XY_Err_NotAllowed;
	}

	if(onenet_context_refs[0].onenet_context->registerEnabled == true)
		goto status_error;
	
	osMutexAcquire(g_onenet_mutex, osWaitForever);
	errorcode = onet_register(onenet_context_refs[0].onenet_context, lifetime);
	osMutexRelease(g_onenet_mutex);
	if (errorcode != CIS_RET_OK)
		goto param_error;
	
	return errorcode;
param_error:
	errorcode = XY_Err_Parameter;
	goto failed;	
status_error:
	errorcode = XY_ERR;
failed:
	return errorcode;
}

/*
* description: Add new object to onenet instance
* in: 
*			objId:      object id
*			insCount:   instance count
*			insBitmap:  bitmap of instance count
*			attrCount:  
*			actCount:   
*
* return:
*			success: XY_OK
*			fail:	 XY_Err_NoConnected
*			         XY_Err_NotAllowed
*			         XY_Err_Parameter
*                    XY_ERR
*
*/
int cis_addobj(int objId, int insCount, char* insBitmap, int attrCount, int actCount)
{
	struct onenet_addobj param = {0};
    int errorcode = CIS_RET_OK;

    cis_module_init();
	if(!xy_tcpip_is_ok()) {
        return XY_Err_NoConnected;
	}

	onenet_resume_session();

    if (!is_onenet_task_running(0)) {
        xy_printf(0,XYAPP, WARN_LOG, "[CIS] cis is not running!");
        return XY_Err_NotAllowed;
    }

	if(objId<0 || insCount<0)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[CIS] error:objId(%d),insCount(%d)", objId, insCount);
		goto param_error;
	}	
	
	if(insCount != (int)strlen(insBitmap))
	{
		xy_printf(0,XYAPP, WARN_LOG, "[CIS] error:insBitmap(%s),len(%d)", insBitmap, strlen(insBitmap));
		goto param_error;
	}	

	if(attrCount<0 || actCount<0)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[CIS] error:attrCount(%d),actCount(%d)", attrCount, actCount);
		goto param_error;
	}	

	param.ref = 0;
	param.objId = objId;
	param.insCount = insCount;
	param.attrCount = attrCount;
	param.actCount = actCount;
	param.insBitmap = xy_malloc(param.insCount + 1);
	cis_memcpy(param.insBitmap, insBitmap, param.insCount);
	param.insBitmap[param.insCount] = '\0';
	xy_printf(0,XYAPP, WARN_LOG, "[CIS] cis is addobj!");
    osMutexAcquire(g_onenet_mutex, osWaitForever);
    errorcode = onet_mipladdobj_req(onenet_context_refs[0].onenet_context, &param);
    osMutexRelease(g_onenet_mutex);
	if (errorcode != CIS_RET_NO_ERROR) {
		goto param_error;
	}

	xy_free(param.insBitmap);

	return errorcode;
param_error:
    errorcode = XY_Err_Parameter;
	if (param.insBitmap != NULL) 
		xy_free(param.insBitmap);

	return  errorcode;
}


/*
* description: Delete object of onenet instance
* in: 
*			objId:      object id  
*
* return:
*			success: XY_OK
*           fail:    XY_Err_NoConnected
*                    XY_Err_NotAllowed
*                    XY_Err_Parameter
*                    XY_ERR
*
*/
int cis_delobj(int objId)
{
	struct onenet_delobj param = {0};
    int errorcode = CIS_RET_OK;
	
    cis_module_init();
	if(!xy_tcpip_is_ok()) {
		return XY_Err_NoConnected;
	}

	onenet_resume_session();

    if (!is_onenet_task_running(param.ref)) {
        return XY_Err_NotAllowed;
    }

    if(objId < 0)
        goto param_error;

	param.ref = 0;
	param.objId = objId;

    osMutexAcquire(g_onenet_mutex, osWaitForever);
    errorcode = cis_delobject(onenet_context_refs[param.ref].onenet_context, param.objId);
    osMutexRelease(g_onenet_mutex);
	if (errorcode != CIS_RET_NO_ERROR)
		goto param_error;

	return errorcode;
param_error:
    errorcode = XY_Err_Parameter;
	return  errorcode;
}

/*****************************************************************************
 Function    : onenet_cloud_setting
 Description : setting cloud ip and port
 Input       : ip_addr_str -cloud server ip
               port        -cloud server port(default port 5683)
 Output      : None
 Return      : XY_OK success
              XY_ERR error
 *****************************************************************************/
int cis_cloud_setting(char *ip_addr_str, int port, char *authcode)
{
    cis_module_init();
    if((INADDR_NONE == inet_addr(ip_addr_str)) || (port < 0) || (port > 65535)
            || strlen(ip_addr_str) > sizeof(g_onenet_config_data->server_host))
    {
        return XY_ERR;
    }

	memset(g_onenet_config_data->server_host, 0, sizeof(g_onenet_config_data->server_host));
    memcpy(g_onenet_config_data->server_host, ip_addr_str, strlen(ip_addr_str));
	if(authcode!=NULL && strlen(authcode)!=0)
	{
		xy_assert(strlen(authcode)<16);
		memset(g_onenet_config_data->auth_code, 0, sizeof(g_onenet_config_data->auth_code));
		memcpy(g_onenet_config_data->auth_code, authcode, strlen(authcode));
	}
	g_onenet_config_data->server_port = port;
	cloud_save_file(ONENET_CONFIG_NVM_FILE_NAME,(void*)g_onenet_config_data,sizeof(onenet_config_nvm_t));
    return XY_OK;
}

/*
* description: Notify response for observe object
* in: 
*			msgId:      specifed msg id; if set -1, it will find the first observed list node to notify
*			objId:   	response object id
*			insId:  	response instance id
*			resId:  	response resouce id
*			valueType:  response value type
*			value:      response value
*			index:   	
*			flag:  		
*			ackid:  
*
* return:
*           success: XY_OK
*           fail:    XY_Err_NoConnected
*                    XY_Err_NotAllowed
*                    XY_Err_Parameter
*                    XY_ERR
*
* Note: Upload data must use this function and the ackid param must be set.
*        Because it can be get ack response.
*
*/
int cis_notify_sync(int msgId, int objId, int insId, int resId, int valueType, int len, char* value, int index, int flag, int
ackid)
{
	struct onenet_notify notifyParam = {0};
    int errorcode = CIS_RET_ERROR;
	cis_uri_t uri = {0};
	//st_observed_t * observe = NULL;
	
    cis_module_init();
	if(!xy_tcpip_is_ok()) {
		return XY_Err_NoConnected;
	}

	onenet_resume_session();

    if (!is_onenet_task_running(0)) {
        return XY_Err_NotAllowed;
    }

	if(value == NULL || objId < 0 || insId < -1 || resId < -1 || index < 0 || flag < 0 || flag > 2)
		goto param_error;	

	notifyParam.ref = 0;
	notifyParam.objId = objId;
	notifyParam.insId = insId;
	notifyParam.resId = resId;
	notifyParam.value_type = valueType;
	notifyParam.len = len;
	notifyParam.index = index;
	notifyParam.flag = flag;
	notifyParam.ackid = ackid;
	notifyParam.raiflag = g_cis_rai;

	if(msgId == -1)
	{
		uri.objectId = objId;
		uri.instanceId = insId;
		uri.resourceId = resId;
		cis_uri_update(&uri);
		notifyParam.msgId = xy_get_observeMsgId(onenet_context_refs[0].onenet_context, &uri);
		xy_printf(0,XYAPP, WARN_LOG, "[CIS]observe mid(%d)", notifyParam.msgId);
		if(notifyParam.msgId == -1)
			goto status_error;
	}
	else if(msgId < 0 )
	{			
		goto param_error;
	}
	else
	{
		notifyParam.msgId = msgId;
	}

	notifyParam.value = NULL;
	if(len == 0)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[CIS]observe at value(NULL)");
		goto param_error;
	}
	else
	{
		if (XY_OK != xy_get_notify_value(value, &notifyParam)) {
			xy_printf(0,XYAPP, WARN_LOG, "[CIS]observe get value error");
			goto param_error;
		}		
	}
	
	//Use notify sem to sync send data
	if(g_cis_notify_sem == NULL)
		g_cis_notify_sem = osSemaphoreNew(0xFFFF, 0, NULL);

	osMutexAcquire(g_onenet_mutex, osWaitForever);
	errorcode = onet_miplnotify_req(onenet_context_refs[notifyParam.ref].onenet_context, &notifyParam);
	osMutexRelease(g_onenet_mutex);
	xy_printf(0,XYAPP, WARN_LOG, "[CIS]observe errorcode(%d)", errorcode);

	if (errorcode != CIS_RET_OK && errorcode != COAP_205_CONTENT)
		goto param_error;

	if(notifyParam.value != NULL)
		xy_free(notifyParam.value);

	//Block until get ack response
	osSemaphoreAcquire(g_cis_notify_sem, osWaitForever);
	
	return errorcode;
param_error:
	errorcode = XY_Err_Parameter;
	goto failed;	
status_error:
	errorcode = XY_ERR;
failed:
	if (notifyParam.value != NULL)
		xy_free(notifyParam.value);
	return	errorcode;
}


int cis_notify_sync_with_rai(int msgId, int objId, int insId, int resId, int valueType, int len, char* value, int index, int flag, int
ackid)
{
    int ret = XY_ERR;
    cis_module_init();
    if(ackid == 0)
        cis_set_rai_flag(RAI_REL_UP);
    else
        cis_set_rai_flag(RAI_REL_DOWN);
    ret = cis_notify_sync( msgId,  objId,  insId,  resId,  valueType,  len,  value,  index,  flag, ackid);
    cis_set_rai_flag(RAI_NULL);
    return ret;
}

/*
* description: Notify response for observe object
* in: 
*			msgId:      specifed msg id; if set -1, it will find the first observed list node to notify
*			objId:   	response object id
*			insId:  	response instance id
*			resId:  	response resouce id
*			valueType:  response value type
*			value:      response value
*			index:   	
*			flag:  		
*			ackid:  0-non,>0-con
*
* return:
*           success: XY_OK
*           fail:    XY_Err_NoConnected
*                    XY_Err_NotAllowed
*                    XY_Err_Parameter
*                    XY_ERR
*
* Note: Upload data without ack response
*/
int cis_notify_asyn(int msgId, int objId, int insId, int resId, int valueType, int len, char* value, int index, int flag , int ackid)
{
	struct onenet_notify notifyParam = {0};
    int errorcode = CIS_RET_ERROR;
	cis_uri_t uri = {0};
	
    cis_module_init();
	if(!xy_tcpip_is_ok()) {
		return XY_Err_NoConnected;
	}

	onenet_resume_session();

    if (!is_onenet_task_running(0)) {
        return XY_Err_NotAllowed;
    }

	if(len < 0 || value == NULL || objId < 0 || insId < -1 || resId < -1 || index < 0 || flag < 0 || flag > 2)
		goto param_error;	

	notifyParam.ref = 0;
	notifyParam.objId = objId;
	notifyParam.insId = insId;
	notifyParam.resId = resId;
	notifyParam.value_type = valueType;
	notifyParam.len = len;
	notifyParam.index = index;
	notifyParam.flag = flag;
	notifyParam.raiflag = g_cis_rai;

    /*if ackid is 0 , pkt type is non;if ackid > 0 , pkt type is con*/
    if(ackid)
        notifyParam.ackid = ackid;
    else
        notifyParam.ackid = 0;
    
	if(msgId == -1)
	{
		uri.objectId = objId;
		uri.instanceId = insId;
		uri.resourceId = resId;
		cis_uri_update(&uri);
		notifyParam.msgId = xy_get_observeMsgId(onenet_context_refs[0].onenet_context, &uri);
		xy_printf(0,XYAPP, WARN_LOG, "[CIS]observe mid(%d)", notifyParam.msgId);
		if(notifyParam.msgId == -1)
			goto status_error;
	}
	else if(msgId < 0)
	{			
		goto param_error;
	}
	else
	{
		notifyParam.msgId = msgId;
	}

	if(len == 0)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[CIS]observe at value(NULL)");
		goto param_error;
	}
	else
	{
		if (XY_OK != xy_get_notify_value(value, &notifyParam)) {
			xy_printf(0,XYAPP, WARN_LOG, "[CIS]observe get value error");
			goto param_error;
		}		
	}
	
	osMutexAcquire(g_onenet_mutex, osWaitForever);
	errorcode = onet_miplnotify_req(onenet_context_refs[notifyParam.ref].onenet_context, &notifyParam);
	osMutexRelease(g_onenet_mutex);
	xy_printf(0,XYAPP, WARN_LOG, "[CIS]observe errorcode(%d)", errorcode);

	if (errorcode != CIS_RET_OK && errorcode != COAP_205_CONTENT)
		goto param_error;

	if(notifyParam.value != NULL)
		xy_free(notifyParam.value);
	
	return errorcode;
param_error:
	errorcode = XY_Err_Parameter;
	goto failed;	
status_error:
	errorcode = XY_ERR;
failed:
	if (notifyParam.value != NULL)
		xy_free(notifyParam.value);
	return	errorcode;
}

int cis_notify_asyn_with_rai(int msgId, int objId, int insId, int resId, int valueType, int len, char* value, int index, int flag , int ackid)
{
    int ret = XY_ERR;
    if(ackid == 0)
        cis_set_rai_flag(RAI_REL_UP);
    else
        cis_set_rai_flag(RAI_REL_DOWN);
    ret = cis_notify_asyn( msgId,  objId,  insId,  resId,  valueType,  len,  value,  index,  flag ,  ackid);
    cis_set_rai_flag(RAI_NULL);
    return ret;
}
/*
* description: Read response 
* in: 
*			msgId:      specifed msg id; if set -1, it will find the first request list node with read type
*			objId:   	response object id
*			insId:  	response instance id
*			resId:  	response resouce id
*			valueType:  response value type
*			value:      response value
*			index:   	
*			flag:  		
*
* return:
*           success: XY_OK
*           fail:    XY_Err_NoConnected
*                    XY_Err_NotAllowed
*                    XY_Err_Parameter
*                    XY_ERR
*
*/
int cis_read_rsp(int msgId, int result, int objId, int insId, int resId, int valueType, int len, char* value, int index, int flag)
{
	struct onenet_read readParam = {0};
	int errorcode = CIS_RET_ERROR;

    cis_module_init();
	if(!xy_tcpip_is_ok()) {
		return XY_Err_NoConnected;
	}

	onenet_resume_session();

    if (!is_onenet_task_running(0)) {
        return XY_Err_NotAllowed;
    }
	
	if(len < 0 || value == NULL || objId < 0 || insId < -1 || resId < -1 || index < 0 || flag < 0 || flag > 2)
		goto param_error;

	readParam.ref = 0;
	readParam.result = result;
	readParam.objId = objId;
	readParam.insId = insId;
	readParam.resId = resId;
	readParam.value_type = valueType;
	readParam.len = len;
	readParam.index = index;
	readParam.flag = flag;
	readParam.raiflag = g_cis_rai;

	if(msgId == -1)
	{
		readParam.msgId = xy_get_reqMsgId(onenet_context_refs[0].onenet_context, CALLBACK_TYPE_READ);
		if(readParam.msgId == -1)
			goto status_error;
	}
	else if(msgId < 0 )
	{			
		goto param_error;
	}
	else
	{
		readParam.msgId = msgId;
	}

	if(strlen(value) == 0)
	{
		goto param_error;
	}
	else
	{
		if (XY_OK != xy_get_read_value(value, &readParam)) {
			goto param_error;
		}
	}

	if (XY_OK != check_coap_result_code(readParam.result, RSP_READ))
		goto param_error;
	
	if ((result = get_coap_result_code(readParam.result)) != CIS_COAP_205_CONTENT)
	{
		osMutexAcquire(g_onenet_mutex, osWaitForever);
		cis_response(onenet_context_refs[readParam.ref].onenet_context, NULL, NULL, readParam.msgId, result,readParam.raiflag);
		osMutexRelease(g_onenet_mutex);
		goto out_ok;
	}
	
	osMutexAcquire(g_onenet_mutex, osWaitForever);
	errorcode = onet_miplread_req(onenet_context_refs[readParam.ref].onenet_context, &readParam);
	osMutexRelease(g_onenet_mutex);
	if (errorcode != CIS_RET_OK)
		goto param_error;

out_ok:
	if (readParam.value != NULL)
		xy_free(readParam.value);
	return XY_OK;
param_error:
	errorcode = XY_Err_Parameter;
	goto failed;	
status_error:
	errorcode = XY_ERR;
failed:
	if (readParam.value != NULL)
		xy_free(readParam.value);
	return	errorcode;
}

int cis_read_rsp_with_rai(int msgId, int result, int objId, int insId, int resId, int valueType, int len, char* value, int index, int flag)
{
    int ret = XY_ERR;
    cis_set_rai_flag(RAI_REL_UP);
    ret = cis_read_rsp(msgId,result,objId,insId, resId, valueType, len, value, index, flag);
    cis_set_rai_flag(RAI_NULL);
    return ret;
}

/*
* description: Write response 
* in: 
*			msgId:      specifed msg id; if set -1, it will find the first request list node with write type
*			result:   	request result
*
* return:
*           success: XY_OK
*           fail:    XY_Err_NoConnected
*                    XY_Err_NotAllowed
*                    XY_Err_Parameter
*                    XY_ERR
*
*/
int cis_write_rsp(int msg_id, int result)
{
    int errorcode = CIS_RET_ERROR;
	int write_Mid = -1;

    cis_module_init();
	if(!xy_tcpip_is_ok()) {
		return XY_Err_NoConnected;
	}
	
	onenet_resume_session();

	if (!is_onenet_task_running(0)) {
        return XY_Err_NotAllowed;
	}

	if (XY_OK != check_coap_result_code(result, RSP_WRITE))
		goto param_error;

	if(msg_id == -1)
	{
		write_Mid = xy_get_reqMsgId(onenet_context_refs[0].onenet_context, CALLBACK_TYPE_WRITE);
		if(write_Mid == -1)
			goto status_error;
	}
	else if(msg_id < 0 )
	{			
		goto param_error;
	}
	else
	{
		write_Mid = msg_id;
	}

	osMutexAcquire(g_onenet_mutex, osWaitForever);
	errorcode = cis_response(onenet_context_refs[0].onenet_context, NULL, NULL, write_Mid, get_coap_result_code(result),g_cis_rai);
	osMutexRelease(g_onenet_mutex);
	if (errorcode != CIS_RET_OK)
		goto param_error;

	return XY_OK;
param_error:
	errorcode = XY_Err_Parameter;
	goto failed;
status_error:
	errorcode = XY_ERR;
failed:
	return	errorcode;
}

int cis_write_rsp_with_rai(int msg_id, int result)
{
    int ret = XY_ERR;
    cis_set_rai_flag(RAI_REL_UP);
    ret = cis_write_rsp(msg_id, result);
    cis_set_rai_flag(RAI_NULL);
    return ret;
}

/*
* description: Execute response 
* in: 
*			msgId:      specifed msg id; if set -1, it will find the first request list node with execute type
*			result:   	request result
*
* return:
*           success: XY_OK
*           fail:    XY_Err_NoConnected
*                    XY_Err_NotAllowed
*                    XY_Err_Parameter
*                    XY_ERR
*
*/

int cis_execute_rsp(int msg_id, int result)
{
	int errorcode = CIS_RET_OK;
	int exe_Mid   = -1;

    cis_module_init();
	if(!xy_tcpip_is_ok()) {
		return XY_Err_NoConnected;
	}

	onenet_resume_session();

	if (!is_onenet_task_running(0)) {
	    return XY_Err_NotAllowed;
	}
	
	if (XY_OK != check_coap_result_code(result, RSP_EXECUTE))
		goto param_error;

	if(msg_id == -1)
	{
		exe_Mid = xy_get_reqMsgId(onenet_context_refs[0].onenet_context, CALLBACK_TYPE_EXECUTE);
		if(exe_Mid == -1)
			goto status_error;
	}
	else if(msg_id < 0)
	{			
		goto param_error;
	}
	else
	{
		exe_Mid = msg_id;
	}

	osMutexAcquire(g_onenet_mutex, osWaitForever);
	errorcode = cis_response(onenet_context_refs[0].onenet_context, NULL, NULL, exe_Mid, get_coap_result_code(result),g_cis_rai);
	osMutexRelease(g_onenet_mutex);
	if (errorcode != CIS_RET_OK)
		goto param_error;

	return errorcode;
param_error:
	errorcode = XY_Err_Parameter;
	goto failed;
status_error:
	errorcode = XY_ERR;
failed:
	return	errorcode;
}

int cis_execute_rsp_with_rai(int msg_id, int result)
{
    int ret = XY_ERR;
    cis_set_rai_flag(RAI_REL_UP);
    ret = cis_execute_rsp(msg_id, result);
    cis_set_rai_flag(RAI_NULL);
    return ret;
}
/*
* description: Observe response 
* in: 
*			msgId:      specifed msg id; if set -1, it will find the first request list node with observe type
*			result:   	request result
*
* return:
*           success: XY_OK
*           fail:    XY_Err_NoConnected
*                    XY_Err_NotAllowed
*                    XY_Err_Parameter
*                    XY_ERR
*
*/
int cis_observe_rsp(int msg_id, int result)
{
	int errorcode = CIS_RET_ERROR;
	cis_mid_t observeMid = -1;

    cis_module_init();
	if(!xy_tcpip_is_ok()) {
		return XY_Err_NoConnected;
	}
	
	onenet_resume_session();

	if (!is_onenet_task_running(0)) {
	    return XY_Err_NotAllowed;
	}
	
	if (XY_OK != check_coap_result_code(result, RSP_OBSERVE))
		goto param_error;
	
	if(msg_id == -1)
	{
		observeMid = xy_get_reqMsgId(onenet_context_refs[0].onenet_context, CALLBACK_TYPE_OBSERVE);
		if(observeMid == (unsigned int)-1)
		{
			observeMid = xy_get_reqMsgId(onenet_context_refs[0].onenet_context, CALLBACK_TYPE_OBSERVE_CANCEL);
			if(observeMid == (unsigned int)-1)
				goto status_error;
		}
	}
	else if(msg_id < 0)
	{			
		goto param_error;
	}
	else
	{
		observeMid = msg_id;
	}

	osMutexAcquire(g_onenet_mutex, osWaitForever);
	if (observe_findByMsgid(onenet_context_refs[0].onenet_context, observeMid) == NULL) {
		if (!packet_asynFindObserveRequest(onenet_context_refs[0].onenet_context, observeMid, &observeMid)) {
			osMutexRelease(g_onenet_mutex);
			goto param_error;
		}
	}

	errorcode = cis_response(onenet_context_refs[0].onenet_context, NULL, NULL, observeMid, get_coap_result_code(result),g_cis_rai);
	osMutexRelease(g_onenet_mutex);
	if (errorcode != CIS_RET_OK)
		goto param_error;

	return errorcode;
param_error:
	errorcode = XY_Err_Parameter;
	goto failed;	
status_error:
	errorcode = XY_ERR;
failed:
	return	errorcode;
}

int cis_observe_rsp_with_rai(int msg_id, int result)
{
    int ret = XY_ERR;
    cis_set_rai_flag(RAI_REL_UP);
    ret = cis_observe_rsp(msg_id, result);
    cis_set_rai_flag(RAI_NULL);
    return ret;
}
/*
* description: Get onenet sdk versoni
* out: 
*			rsp_cmd:      onenet sdk version
*
* return:   void
*			
*
*/
void cis_miplver(char **rsp_cmd)
{
	cis_version_t ver = {0};
	*rsp_cmd = xy_malloc(32);
	cis_version(&ver);
	snprintf(*rsp_cmd, 32, "\r\n+MIPLVER:%x.%x.%x%s\r\n", ver.major, ver.minor, ver.micro, AT_RSP_OK);
}

/*
* description: Discover response 
* in: 
*			msgId:      specifed msg id; if set -1, it will find the first request list node with discover type
*			objId:   	response object id
*			result:  	request result
*			length:  	length of value
*			valueStr:   value string 		
*
* return:
*           success: XY_OK
*           fail:    XY_Err_NoConnected
*                    XY_Err_NotAllowed
*                    XY_Err_Parameter
*                    XY_ERR
*
*/
int cis_discover_rsp(int msg_id, int objId, int result, int length, char* valueStr)
{
	char *temp_value = NULL;
	cis_uri_t uri = {0};
	int errorcode = CIS_RET_OK;
	int discover_Mid = -1;	

    cis_module_init();
	if(!xy_tcpip_is_ok()) {
		return XY_Err_NoConnected;
	}
	
	onenet_resume_session();

    if (!is_onenet_task_running(0)) {
        return XY_Err_NotAllowed;
    }

	if(valueStr == NULL || length != (int)strlen(valueStr))
		goto param_error;
	
	if (XY_OK != check_coap_result_code(result, RSP_DISCOVER))
		goto param_error;
	
	if (length <= STR_VALUE_LEN) {
		temp_value = xy_malloc(length + 1);
		cis_memcpy(temp_value, valueStr, length);
		temp_value[length] = '\0';
	} else {
		goto param_error;
	}

	if(msg_id == -1)
	{
		uri.objectId = objId;
		cis_uri_update(&uri);
		discover_Mid = xy_get_DiscoverMsgId(onenet_context_refs[0].onenet_context, &uri);
		if(discover_Mid == -1)
			goto status_error;
	}
	else if(msg_id < 0)
	{			
		goto param_error;
	}
	else
	{
		discover_Mid = msg_id;
	}

	if(result == 1 && length > 0) {
		char *buft = temp_value;
		char *lastp = NULL;
		char *strres = NULL;
		while ((strres = strtok_r(buft, ";", &lastp)) != NULL) {
			cis_uri_t uri = {0};
			uri.objectId = URI_INVALID;
			uri.instanceId = URI_INVALID;
			uri.resourceId = (int)strtol(strres,NULL,10);
			cis_uri_update(&uri);
			osMutexAcquire(g_onenet_mutex, osWaitForever);
			cis_response(onenet_context_refs[0].onenet_context, &uri, NULL, discover_Mid, CIS_RESPONSE_CONTINUE,g_cis_rai);
			osMutexRelease(g_onenet_mutex);
			buft = NULL;
		}
	}

	osMutexAcquire(g_onenet_mutex, osWaitForever);
	errorcode = cis_response(onenet_context_refs[0].onenet_context, NULL, NULL, discover_Mid, get_coap_result_code(result),g_cis_rai);
	osMutexRelease(g_onenet_mutex);
	if (errorcode != CIS_RET_OK){
		goto param_error;
	}
	if (temp_value != NULL)
		xy_free(temp_value);

	return errorcode;
param_error:
	errorcode = XY_Err_Parameter;
	goto failed;
status_error:
	errorcode = XY_ERR;
failed:
	if (temp_value != NULL)
		xy_free(temp_value);
	return	errorcode;
}

int cis_discover_rsp_with_rai(int msg_id, int objId, int result, int length, char* valueStr)
{
    int ret = XY_ERR;
    cis_set_rai_flag(RAI_REL_UP);
    ret = cis_discover_rsp(msg_id, objId, result, length, valueStr);
    cis_set_rai_flag(RAI_NULL);
    return ret;
}
/*
* description: Set Parameter response 
* in: 
*			msgId:      specifed msg id; if set -1, it will find the first request list node with observe param type
*			result:   	request result
*
* return:
*           success: XY_OK
*           fail:    XY_Err_NoConnected
*                    XY_Err_NotAllowed
*                    XY_Err_Parameter
*                    XY_ERR
*
*/
int cis_rsp_withparam(int msg_id, int result)
{
    int errorcode = CIS_RET_ERROR;
	int param_Mid = -1;	

    cis_module_init();
	if(!xy_tcpip_is_ok()) {
		return XY_Err_NoConnected;
	}
	
	onenet_resume_session();
	
	if (!is_onenet_task_running(0)) {
	    return XY_Err_NotAllowed;
	}

	if (XY_OK != check_coap_result_code(result, RSP_SETPARAMS))
		goto param_error;

	if(msg_id == -1)
	{
		param_Mid = xy_get_reqMsgId(onenet_context_refs[0].onenet_context, CALLBACK_TYPE_OBSERVE_PARAMS);
		if(param_Mid == -1)
			goto status_error;
	}
	else if(msg_id < 0)
	{			
		goto param_error;
	}
	else
	{
		param_Mid = msg_id;
	}

	osMutexAcquire(g_onenet_mutex, osWaitForever);
	errorcode = cis_response(onenet_context_refs[0].onenet_context, NULL, NULL, param_Mid, get_coap_result_code(result),g_cis_rai);
	osMutexRelease(g_onenet_mutex);
	if (errorcode != CIS_RET_OK) {
		goto param_error;
	}

	return errorcode;
param_error:
	errorcode = XY_Err_Parameter;
	goto failed;	
status_error:
	errorcode = XY_ERR;
failed:
	return	errorcode;
}

int cis_rsp_withparam_with_rai(int msg_id, int result)
{
    int ret = XY_ERR;
    cis_set_rai_flag(RAI_REL_UP);
    ret = cis_rsp_withparam(msg_id, result);
    cis_set_rai_flag(RAI_NULL);
    return ret;
}
/*
* description: Upate onenet instance
* in: 
*			lifetime:      new lifetime
*			withObjFlag:   flag of update with object
*
* return:
*           success: XY_OK
*           fail:    XY_Err_NoConnected
*                    XY_Err_NotAllowed
*                    XY_Err_Parameter
*                    XY_ERR
*
*/
int cis_updatelife(int lifetime, bool withObjFlag)
{
    int errorcode = CIS_RET_OK;
	int cis_lifetime = lifetime;

    cis_module_init();
	if(!xy_tcpip_is_ok()) {
        return XY_Err_NoConnected;
	}
	
	onenet_resume_session();

    if (!is_onenet_task_running(0)) {
        return XY_Err_NotAllowed;
    }

	if(withObjFlag != 0 && withObjFlag != 1)
		return XY_Err_Parameter;

    if (cis_lifetime == 0) {
        cis_lifetime = DEFAULT_LIFETIME; // seconds
    } else if ((uint32_t)cis_lifetime < LIFETIME_LIMIT_MIN || (uint32_t)cis_lifetime > LIFETIME_LIMIT_MAX) {
        return XY_Err_Parameter;
    }

    osMutexAcquire(g_onenet_mutex, osWaitForever);
    errorcode = cis_update_reg(onenet_context_refs[0].onenet_context, cis_lifetime, withObjFlag,g_cis_rai);
    osMutexRelease(g_onenet_mutex);
    if (errorcode != COAP_NO_ERROR) {
        return XY_Err_NotAllowed;
    }

	return errorcode;
}

int cis_updatelife_with_rai(int lifetime, bool withObjFlag)
{
    int ret = XY_ERR;
    cis_set_rai_flag(RAI_REL_DOWN);
    ret = cis_updatelife(lifetime, withObjFlag);
    cis_set_rai_flag(RAI_NULL);
    return ret;
}

int cis_rmlft_get(void)
{
    int remain_lifetime = -1;

    cis_module_init();
    
    if (onenet_context_refs[0].onenet_context != NULL && onenet_context_refs[0].onenet_context->stateStep != PUMP_STATE_READY)
        return XY_ERR;

    if (onenet_context_refs[0].onet_at_thread_id == NULL)
    {
        if(g_onenet_session_info->life_time == 0 || g_onenet_session_info->last_update_time == 0)
           return XY_ERR;

        remain_lifetime = g_onenet_session_info->last_update_time + g_onenet_session_info->life_time - cloud_gettime_s();
        if(remain_lifetime <= 0)
        {
            cloud_remove_file(ONENET_SESSION_NVM_FILE_NAME);
            memset(g_onenet_session_info, 0, sizeof(onenet_session_info_t));
        }
    }
    else if (onenet_context_refs[0].onenet_context != NULL && onenet_context_refs[0].onenet_context->server != NULL)
    {
        remain_lifetime = onenet_context_refs[0].onenet_context->server->registration +
            onenet_context_refs[0].onenet_context->lifetime - cloud_gettime_s();
    }
    else
        return XY_ERR;
    
    if (remain_lifetime > 0)
    {
        return remain_lifetime;
    }
    else
    {
        return XY_ERR;
    }
}

