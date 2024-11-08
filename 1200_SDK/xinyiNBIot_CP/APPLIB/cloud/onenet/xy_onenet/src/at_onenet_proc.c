
/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "xy_utils.h"
#include "at_onenet.h"
#include "xy_cis_api.h"
#include "xy_at_api.h"
#include "xy_system.h"
#include "oss_nv.h"
#include "net_app_resume.h"
#include "onenet_utils.h"
#include "at_com.h"
#include "cloud_utils.h"
#include "ps_netif_api.h"
#include "xy_net_api.h"
#include "softap_nv.h"
#include "cis_config.h"
#include "at_config.h"
#include <float.h>

/*******************************************************************************
 *						  Local function declarations						   *
 ******************************************************************************/

/*******************************************************************************
 *						   Local variable definitions						   *
 ******************************************************************************/
onenet_context_config_t onenet_context_configs[CIS_REF_MAX_NUM] = {0};
onenet_context_reference_t onenet_context_refs[CIS_REF_MAX_NUM] = {0};
onenet_config_nvm_t *g_onenet_config_data = NULL;

#if VER_BC95
    uint8_t g_ONENET_ACK_TIMEOUT = 2;
#else
    uint8_t g_ONENET_ACK_TIMEOUT = 4;
#endif
//static int onenet_out_fd;
osMutexId_t g_onenet_mutex= NULL;
osSemaphoreId_t g_cis_del_sem = NULL;
osSemaphoreId_t g_cis_rcv_sem = NULL;
osSemaphoreId_t cis_recovery_sem = NULL;
osThreadId_t onenet_resume_task_id = NULL;

// extern char *g_Remote_AT_Rsp;
extern char* cis_cfg_tool(char* ip,unsigned int port,char is_bs,char* authcode,char is_dtls,char* psk,int *cfg_out_len);
#if LWM2M_COMMON_VER
extern lwm2m_common_user_config_nvm_t *g_lwm2m_common_config_data;
extern int convert_cis_event_to_common_urc(char *at_str, int max_len, int cis_eid, void *param);
#endif
//pump

/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/

/*******************************************************************************
 *						Inline function implementations 					   *
 ******************************************************************************/

/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/

static int onet_set_rai(int raiType,uint8_t* raiflag)
{
    int ret = 0;
    if(raiType == 0)
        *raiflag = RAI_NULL;
    else if(raiType == 0x200)
        *raiflag = RAI_REL_UP;
    else if(raiType == 0x400)
        *raiflag = RAI_REL_DOWN;
    else
        ret = -1;

    xy_printf(0,XYAPP, WARN_LOG, "[CIS]rai(%d)", *raiflag);
    return ret;
}
int onet_resume_task_start()
{
	osThreadAttr_t task_attr = {0};

	if(!is_onenet_task_running(0) && onenet_resume_task_id==NULL)
	{
        if(cis_recovery_sem == NULL)
            cis_recovery_sem = osSemaphoreNew(0xFFFF, 0, NULL);
		
		task_attr.name = "resume_onenet";
		task_attr.priority = osPriorityNormal1;
		task_attr.stack_size = osStackShared;
		onenet_resume_task_id = osThreadNew((osThreadFunc_t)(onenet_resume_task),NULL, &task_attr);
		if(cis_recovery_sem != NULL)
			osSemaphoreAcquire(cis_recovery_sem, osWaitForever);
	}
	
	return 0;
}

unsigned char get_coap_result_code(unsigned char  at_result_code)
{
    unsigned int index = 0;
    const struct result_code_map code_map[]=
    {
        { 1,  CIS_COAP_205_CONTENT },
        { 2,  CIS_COAP_204_CHANGED},
        { 11, CIS_COAP_400_BAD_REQUEST },
        { 12, CIS_COAP_401_UNAUTHORIZED },
        { 13, CIS_COAP_404_NOT_FOUND },
        { 14, CIS_COAP_405_METHOD_NOT_ALLOWED },
        { 15, CIS_COAP_406_NOT_ACCEPTABLE }
    };
    for(; index < sizeof(code_map)/sizeof(struct result_code_map); index++){
        if(code_map[index].at_result_code == at_result_code){
            return code_map[index].coap_result_code;
        }
    }
    return CIS_COAP_503_SERVICE_UNAVAILABLE;   
}

int check_coap_result_code(int code, enum onenet_rsp_type type)
{
	int res = -1;
	switch (type) {
    	case RSP_READ:
    	case RSP_OBSERVE:
    	case RSP_DISCOVER:
    		if (code == 1 || code == 11 || code == 12 
    			|| code == 13 || code == 14 || code == 15) {
    			res = 0;
    		}
    		break;
    	case RSP_WRITE:
    	case RSP_EXECUTE:
    	case RSP_SETPARAMS:
    		if (code == 2 || code == 11 || code == 12 
    			|| code == 13 || code == 14) {
    			res = 0;
    		}
    		break;
    	default:
    		break;
	}
	return res;
}

bool onet_at_get_notify_value(int value_type, int value_len, char *at_value, char *value)
{
    int ret = false;
    char *stop_str = NULL;
    if(at_value == NULL || value == NULL)
        return false;

    if (value_type == cis_data_type_integer)
    {
        if (value_len != 2 && value_len != 4 && value_len != 8)
            return false;

        if((value_len == 2) && (((int)strtol(at_value, &stop_str,10) <INT16_MIN) || ((int)strtol(at_value,NULL,10) >INT16_MAX)))
        {
            return false;
        }
        else if((value_len == 4) && (((long long)strtoll(at_value, &stop_str,10) <INT32_MIN) || ((long long)strtoll(at_value,NULL,10) >INT32_MAX)))
        {
            return false;
        }
        else if((value_len == 8) && (((long long)strtoll(at_value, &stop_str,10) <INT64_MIN) || ((long long)strtoll(at_value,NULL,10) >INT64_MAX)))
        {
            return false;
        }

        if((stop_str != NULL )&&(strlen(stop_str) != 0))
            return false;

        strcpy(value, at_value);
    }
    else if (value_type == cis_data_type_float)
    {
#if VER_BC95
        if (value_len != 4)
#else
        if (value_len != 4 && value_len != 8)
#endif
            return false;

        if((value_len == 4) && (atof(at_value) >FLT_MAX))
        {
            return false;
        }
        else if((value_len == 8) && (atof(at_value) >DBL_MAX))
        {
            return false;
        }

        strcpy(value, at_value);
    }
    else if (value_type == cis_data_type_bool)
    {
        if (value_len != 1 || (*at_value != '1' && *at_value != '0') || strlen(at_value) > 1)
            return false;

        strcpy(value, at_value);
    }
    else if ((value_type == cis_data_type_string))
    {
        if (value_len > STR_VALUE_LEN || (strlen(at_value) != (size_t)value_len))
            return false;

        strcpy(value, at_value);
    }
    else if (value_type == cis_data_type_opaque) 
    {
        if ((value_len * 2) > OPAQUE_VALUE_LEN || value_len*2 != strlen(at_value))
            return false;

        if (hexstr2bytes(at_value, value_len* 2, value, value_len) == -1)
        {
            return false;
        }
    } 
    else
    {
        return false;
    }

    return true;
}

int onet_check_reqType_uri(et_callback_type_t type, int msgId,int objId, int insId, int resId)
{
	st_context_t* context = (st_context_t*)onenet_context_refs[0].onenet_context;
	st_request_t * request = NULL;

	cloud_mutex_lock(&context->lockRequest,MUTEX_LOCK_INFINITY);
	request = (st_request_t *)CIS_LIST_FIND(context->requestList, msgId);
	cloud_mutex_unlock(&context->lockRequest);

	if(request == NULL)
		return -1;

	if(request->type != type)
		return -1;

	if(request->uri.flag & URI_FLAG_OBJECT_ID)
	{
		if(request->uri.objectId != objId)
			return -1;
	}

	if(request->uri.flag & URI_FLAG_INSTANCE_ID)
	{
		if(request->uri.instanceId != insId)
			return -1;
	}

	if(request->uri.flag & URI_FLAG_RESOURCE_ID)
	{
		if(request->uri.resourceId != resId)
			return -1;
	}

	return 0;
}

int onet_check_reqType(et_callback_type_t type, int msgId)
{

	int result = -1;
	st_context_t* context = (st_context_t*)onenet_context_refs[0].onenet_context;
	st_request_t * request = NULL;

	cloud_mutex_lock(&context->lockRequest,MUTEX_LOCK_INFINITY);
	request = (st_request_t *)CIS_LIST_FIND(context->requestList, msgId);
	cloud_mutex_unlock(&context->lockRequest);

	if(request != NULL && request->type == type)
	{
	   	result = 0;
	}
	else{
		result = -1;
	}

	return result;
}

onenet_context_config_t* find_proper_onenet_context_config(int totalsize, int index, int currentsize)
{
    int i;
    for (i = 0; i < CIS_REF_MAX_NUM; i++)
    {
        if (onenet_context_configs[i].config_hex != NULL && onenet_context_configs[i].total_len == totalsize 
            && onenet_context_configs[i].index == index + 1 && onenet_context_configs[i].offset + currentsize <= totalsize)
        {
            return &onenet_context_configs[i];
        }
    }
    for (i = 0; i < CIS_REF_MAX_NUM; i++)
    {
        if (onenet_context_configs[i].config_hex == NULL)
        {
            return &onenet_context_configs[i];
        }
    }
    return NULL;
}

int is_onenet_task_running(unsigned int ref)
{
    onenet_context_reference_t *onenet_context_ref = NULL;
    if (ref >= CIS_REF_MAX_NUM)
    {
        return 0;
    }
    onenet_context_ref = &onenet_context_refs[ref];
    //if (onenet_context_ref == NULL)
    //{
    //    return 0;
    //}
    if (onenet_context_ref->onenet_context == NULL || onenet_context_ref->onet_at_thread_id == NULL)
    {
        return 0;
    }
    return 1;
}

void free_onenet_context_ref(onenet_context_reference_t *onenet_context_ref)
{
    onenet_context_ref->onenet_context = NULL;
    //onenet_context_ref->onet_at_thread_id = -1;
    onenet_context_ref->thread_quit = 0;
    g_cis_downstream_cb = NULL;
}

int onet_deinit(onenet_context_reference_t *onenet_context_ref)
{
    if(onenet_context_ref->onenet_context != NULL)
        onenet_context_ref->thread_quit = 1;

    return 0;
}

int onet_init(onenet_context_reference_t *onenet_context_ref, onenet_context_config_t *onenet_context_config)
{
    int ret = 0;
    
    ret = cis_init((void **)&onenet_context_ref->onenet_context, onenet_context_config->config_hex, onenet_context_config->total_len, NULL);
    if (ret != CIS_RET_OK) 
    {
		onet_deinit(onenet_context_ref);
		return -1;
	}
    onenet_context_ref->onenet_context_config = onenet_context_config;
    return 0;
}

int get_free_onet_context_ref()
{
	int i;

    for (i = 0; i < CIS_REF_MAX_NUM; i++)
    {
        if (onenet_context_refs[i].onenet_context == NULL && onenet_context_refs[i].onet_at_thread_id == NULL)
        {
            return (int)&onenet_context_refs[i];
        }
    }
    return 0;
}
extern osSemaphoreId_t cis_poll_sem;
static cis_time_t onet_get_lifewait(st_context_t *onenet_context, cis_time_t cur_sec)
{
	cis_time_t lifetime;
	cis_time_t interval;
	cis_time_t lasttime;
	cis_time_t notifytime;
	cis_time_t wait_sec = 0;
	cis_time_t step = 0;
	lifetime = onenet_context->lifetime;
	lasttime = onenet_context->server->registration;

	if(lifetime > COAP_MAX_TRANSMIT_WAIT)
	{
		notifytime = (cis_time_t)COAP_MAX_TRANSMIT_WAIT;
	}
	else if(lifetime > COAP_MIN_TRANSMIT_WAIT)
	{
		notifytime = COAP_MIN_TRANSMIT_WAIT;
	}
	else
	{
		notifytime = (cis_time_t)(lifetime / 2);
	}

	interval = lasttime + lifetime - cur_sec; 
	xy_printf(0,XYAPP, WARN_LOG, "[CIS]lifewait interval (%d)s, notify(%d)", interval, notifytime);

    if(interval <= 0)
    {
        wait_sec = 0;
    }
    else
    {
#if VER_BC95
	#if LWM2M_COMMON_VER
        if(onenet_context->cloud_platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->lifetime_enable == 1)
            step = lifetime - CLOUD_LIFETIME_DELTA(lifetime);
        else
    #endif
            step = notifytime;
#else
    #if LWM2M_COMMON_VER
        if((onenet_context->cloud_platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->lifetime_enable == 1)
                || onenet_context->cloud_platform ==CLOUD_PLATFORM_ONENET)
            step = lifetime - CLOUD_LIFETIME_DELTA(lifetime);
        else
            step = notifytime;
    #else
            step = lifetime - CLOUD_LIFETIME_DELTA(lifetime);
    #endif

#endif

        xy_printf(0,XYAPP, WARN_LOG, "[CIS] life timer step (%d)s", step);
        while((interval-step) <= 0)
        {
            step = step>>1;
        }

        wait_sec = interval - step;
    }

	xy_printf(0,XYAPP, WARN_LOG, "[CIS] life timer wait_sec (%d)s", wait_sec);
	return wait_sec;
}

void onet_at_pump(void* param)
{
    onenet_context_reference_t *onenet_context_ref = (onenet_context_reference_t *)param;
    unsigned int ret;
	st_transaction_t * transacP = NULL;
	cis_time_t cur_sec;
	cis_time_t wait_sec;
	cis_time_t tmp_sec;

    while (onenet_context_ref != NULL && !onenet_context_ref->thread_quit && onenet_context_ref->onenet_context != NULL) {
        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = cis_pump(onenet_context_ref->onenet_context);
		//[XY]Add for onenet loop
		if(cis_poll_sem != NULL && onenet_context_ref->onenet_context->stateStep == PUMP_STATE_READY
			&& onenet_context_ref->onenet_context->server->status == STATE_REGISTERED)
		{
	        osMutexRelease(g_onenet_mutex);
			cur_sec = utils_gettime_s();
			wait_sec = onet_get_lifewait(onenet_context_ref->onenet_context, cur_sec);
			if(wait_sec <= 0)
			{
				continue;
			}

			transacP = onenet_context_ref->onenet_context->transactionList;
			if(transacP != NULL)
			{				
				tmp_sec = transacP->retransTime - cur_sec;
				if(tmp_sec <= 0)
					continue;
				else
				{
					xy_printf(0,XYAPP, WARN_LOG, "[CIS]wait_sec(%d), tmp_sec(%d)", wait_sec, tmp_sec);
					if(tmp_sec <= wait_sec)
						osSemaphoreAcquire(cis_poll_sem, tmp_sec * 1000);
					else
						osSemaphoreAcquire(cis_poll_sem, wait_sec * 1000);
				}										
			}
			else
			{
				osSemaphoreAcquire(cis_poll_sem, wait_sec * 1000);
			}
		}
		else
		{
	        osMutexRelease(g_onenet_mutex);
			osDelay(50);
        }

	}
    
out:
	if(onenet_context_ref->onenet_context_config != NULL)
	{
		if (onenet_context_ref->onenet_context_config->config_hex != NULL)
		{
			xy_free(onenet_context_ref->onenet_context_config->config_hex);
			memset(onenet_context_ref->onenet_context_config, 0, sizeof(onenet_context_config_t));
		}
	}

    if (onenet_context_ref->onenet_context != NULL)
        cis_deinit((void **)&onenet_context_ref->onenet_context);
    free_onenet_context_ref(onenet_context_ref);
	xy_printf(0,XYAPP, WARN_LOG, "onenetdown\n");
    //softap_TaskDelete_Index(tsk_onenet);
	onenet_context_ref->onet_at_thread_id = NULL;
	osThreadExit();
}

int onenet_miplcreate()
{
    onenet_context_reference_t *onenet_context_ref = NULL;
    onenet_context_config_t *onenet_context_config = NULL;
	osThreadAttr_t task_attr = {0};

	xy_printf(0,XYAPP, WARN_LOG, "[CIS]recovery onenet");

    onenet_context_ref = (onenet_context_reference_t *)get_free_onet_context_ref();
    if (onenet_context_ref == NULL) {
		goto failed;
	}

    onenet_context_config = &onenet_context_configs[onenet_context_ref->ref];
    if (onenet_context_config->config_hex != NULL)
    {
        xy_free(onenet_context_config->config_hex);
    }
    memset(onenet_context_config, 0, sizeof(onenet_context_config_t));

    if(g_onenet_session_info->cloud_platform == CLOUD_PLATFORM_ONENET)
    {
        onenet_context_config->config_hex = cis_cfg_tool(g_onenet_config_data->server_host, g_onenet_config_data->server_port, g_onenet_config_data->bs_enable,
                    g_onenet_config_data->auth_code,g_onenet_config_data->dtls_enable,g_onenet_config_data->psk,&onenet_context_config->total_len);
        onenet_context_config->offset = onenet_context_config->total_len;
        onenet_context_config->index = 0;
        if (onet_init(onenet_context_ref, onenet_context_config) < 0)
        {
            xy_printf(0,XYAPP, WARN_LOG, "[CIS]onet_init failed");
            goto failed;
        }
        task_attr.name = "onenet_tk";

    }
    else
    {
#if LWM2M_COMMON_VER
        onenet_context_config->config_hex = cis_cfg_tool(g_lwm2m_common_config_data->server_host, g_lwm2m_common_config_data->port, g_lwm2m_common_config_data->bootstrap_flag, NULL,
                g_lwm2m_common_config_data->security_mode == 0 ? 1 : 0, g_lwm2m_common_config_data->security_mode == 0 ? g_lwm2m_common_config_data->psk : NULL, &onenet_context_config->total_len);

        onenet_context_config->offset = onenet_context_config->total_len;
        onenet_context_config->index = 0;

        init_xy_lwm2m_lists();
        
        if (xy_lwm2m_init(onenet_context_ref, onenet_context_config) < 0)
        {
            xy_printf(0,XYAPP, WARN_LOG, "[CIS]lwm2m]init failed");
            goto failed;
        }
        task_attr.name = "xy_lwm2m_tk";
#endif
    }
	task_attr.priority = osPriorityNormal1;
	task_attr.stack_size = osStackShared;

	onenet_context_ref->onet_at_thread_id = osThreadNew ((osThreadFunc_t)(onet_at_pump), onenet_context_ref, &task_attr);

	return 0;
failed:
    if (onenet_context_config != NULL && onenet_context_config->config_hex != NULL)
    {
        xy_free(onenet_context_config->config_hex);
        onenet_context_config->config_hex = NULL;
		memset(onenet_context_config, 0, sizeof(onenet_context_config_t));
    }
	return  -1;
}



//static cis_ret_t onet_on_read_cb(void* context, cis_uri_t* uri, cis_instcount_t instcount, cis_mid_t mid)
cis_coapret_t onet_on_read_cb(void* context,cis_uri_t* uri,cis_mid_t mid)
{
    char *at_str = xy_malloc2(MAX_ONE_NET_AT_SIZE);
    if(at_str == NULL)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[CIS]memory not enough, read fail");
        return CIS_RET_MEMORY_ERR;
    }

    // (void)context;
    st_context_t *contextP = (st_context_t *)context;
    int ins_id = CIS_URI_IS_SET_INSTANCE(uri) ? uri->instanceId : -1;
    int res_id = CIS_URI_IS_SET_RESOURCE(uri) ? uri->resourceId : -1;

    if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
    {
#if LWM2M_COMMON_VER
        if (g_lwm2m_common_config_data->access_mode == 0)
            snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+QLAURC: \"read\",%d,%d,%d,%d",
                     mid, uri->objectId, ins_id, res_id);
        else
        {
            xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
            memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
            xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_READ;
            xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
            snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"read\",%d,%d,%d,%d",
                     mid, uri->objectId, ins_id, res_id);
            insert_cached_urc_node(xy_lwm2m_cached_urc_common);
        }
#endif
    }
    else
    {
#if	VER_BC95		
        snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+MIPLREAD: %d,%d,%d,%d,%d",
                 CIS_REF, mid, uri->objectId, ins_id, res_id);
#else
        snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+MIPLREAD:%d,%d,%d,%d,%d",
                 CIS_REF, mid, uri->objectId, ins_id, res_id);
#endif
    }

	//at_farps_fd_write(onenet_out_fd, at_str, strlen(at_str));
#if LWM2M_COMMON_VER
	if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->access_mode == 1)
	{
	}
	else
#endif
	    send_urc_to_ext(at_str, strlen(at_str));
	xy_free(at_str);
	return CIS_CALLBACK_CONFORM;
}
void get_write_flag(int i, int maxi, int *flag)
{
	if (i == 0) {
		*flag = 0;
	} else if (i == maxi) {
		*flag = 1;
	} else {
		*flag = 2;
	}

}
// extern int xy_Remote_AT_Req(char *req_at);
cis_ret_t onet_notify_data(st_context_t *onenet_context, struct onenet_notify *param);
extern int xy_get_observeMsgId(st_context_t *contextP, cis_uri_t *uriP);

//attention: onenet server only support cis_data_type_opaque
cis_ret_t onet_miplnotify_req(st_context_t *onenet_context, struct onenet_notify *param)
{
	cis_ret_t ret = CIS_RET_INVILID;
	ret = onet_notify_data(onenet_context, param);
	return ret;
}

cis_coapret_t onet_on_write_cb(void* context,cis_uri_t* uri, const cis_data_t* value,cis_attrcount_t attrcount,cis_mid_t mid)
{
	int i;
	struct onenet_notify paramATRsp = {0};
	char *at_str = NULL;
	char *strBuffer = NULL;
	int str_len = 0;
	// (void)context;
	st_context_t *contextP = (st_context_t *)context;

	for (i = attrcount - 1; i >= 0; i--) {
		cis_data_t *data = value + (attrcount - 1 - i);
		int flag = 0;
		get_write_flag(i,attrcount - 1,&flag);
		switch (data->type) {
		//目前下发的所有写类型数据均按不透明型数据处理
		case cis_data_type_opaque: {
			str_len = ON_WRITE_LEN + data->asBuffer.length * 2;
			strBuffer = xy_malloc2(data->asBuffer.length * 2+1);
			if(strBuffer == NULL)
			{
			    xy_printf(0,XYAPP, WARN_LOG, "[CIS]memory not enough, write opaque fail 0");
			    return CIS_RET_MEMORY_ERR;
			}
			at_str = xy_malloc2(str_len);
			if(at_str == NULL)
            {
			    xy_free(strBuffer);
                xy_printf(0,XYAPP, WARN_LOG, "[CIS]memory not enough, write opaque fail 1");
                return CIS_RET_MEMORY_ERR;
            }
			xy_printf(0,XYAPP, WARN_LOG, "[CIS]data:(%s)", data->asBuffer.buffer);
			// xy_Remote_AT_Req(data->asBuffer.buffer);			
			if (bytes2hexstr((char *)data->asBuffer.buffer, data->asBuffer.length, strBuffer, data->asBuffer.length * 2+1) <= 0 ){
				break;
            }
			if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
			{
#if LWM2M_COMMON_VER
				if (g_lwm2m_common_config_data->access_mode == 0)
					snprintf(at_str, str_len, "+QLAURC: \"write\",%d,%d,%d,%d,%d,%d,%s,%d",
							 mid, uri->objectId, uri->instanceId, data->id, data->type,
							 data->asBuffer.length, strBuffer, i);
				else
				{
					xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
					memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
					xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_WRITE;
					xy_lwm2m_cached_urc_common->urc_data = xy_malloc2(str_len);
					if(xy_lwm2m_cached_urc_common->urc_data != NULL)
					{
                        snprintf(xy_lwm2m_cached_urc_common->urc_data, str_len, "\"write\",%d,%d,%d,%d,%d,%d,%s,%d",
                                 mid, uri->objectId, uri->instanceId, data->id, data->type,
                                 data->asBuffer.length, strBuffer, i);
                        insert_cached_urc_node(xy_lwm2m_cached_urc_common);
					}
					else
					{
					    xy_free(xy_lwm2m_cached_urc_common);
					    xy_printf(0,XYAPP, WARN_LOG, "[cis_lwm2m]memory not enough,opaque cache fail");
					}
				}
#endif
			}
			else
			{
#if VER_BC95 
				if(g_onenet_config_data != NULL && g_onenet_config_data->write_format == 0)
					snprintf(at_str, str_len, "+MIPLWRITE: %d,%d,%d,%d,%d,%d,%d,%s,%d,%d",
						 	CIS_REF, mid, uri->objectId, uri->instanceId, data->id, data->type,
							 data->asBuffer.length, strBuffer, flag, i);
				else
					snprintf(at_str, str_len, "+MIPLWRITE: %d,%d,%d,%d,%d,%d,%d,%s,%d,%d",
						 	CIS_REF, mid, uri->objectId, uri->instanceId, data->id, data->type,
							 data->asBuffer.length, data->asBuffer.buffer, flag, i);
#else
				snprintf(at_str, str_len, "+MIPLWRITE:%d,%d,%d,%d,%d,%d,%d,%s,%d,%d",
						 CIS_REF, mid, uri->objectId, uri->instanceId, data->id, data->type,
						 data->asBuffer.length, strBuffer, flag, i);
#endif
			}
			//at_farps_fd_write(onenet_out_fd, at_str, strlen(at_str));
#if LWM2M_COMMON_VER
			if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->access_mode == 1)
			{
			}
			else
#endif
			    send_urc_to_ext(at_str, strlen(at_str));

			// if(g_Remote_AT_Rsp != NULL)
			// {
			// 	cis_memset(&paramATRsp, 0, sizeof(paramATRsp));

			// 	paramATRsp.ref = 0;
			// 	paramATRsp.objId = uri->objectId;
			// 	paramATRsp.insId = uri->instanceId;
			// 	paramATRsp.resId = data->id;
			// 	paramATRsp.value_type = cis_data_type_string;
			// 	paramATRsp.index = 0;
			// 	paramATRsp.flag = 0;
			// 	paramATRsp.ackid = 0;
			// 	paramATRsp.value = g_Remote_AT_Rsp;
			// 	paramATRsp.len = strlen(g_Remote_AT_Rsp);
			// 	cis_memcpy(paramATRsp.value, g_Remote_AT_Rsp, strlen(g_Remote_AT_Rsp));	

			// 	paramATRsp.msgId = xy_get_observeMsgId(onenet_context_refs[0].onenet_context, uri);
			// 	xy_printf(0,XYAPP, WARN_LOG, "[CIS]Remote AT mid(%d)", paramATRsp.msgId);
			// 	if(paramATRsp.msgId == -1)
			// 	{
			// 		if(paramATRsp.value != NULL)
			// 		{
			// 			xy_free(paramATRsp.value);
			// 			g_Remote_AT_Rsp = NULL;
			// 		}
			// 		break;
			// 	}

			// 	onet_miplnotify_req(onenet_context_refs[paramATRsp.ref].onenet_context, &paramATRsp);
			// 	if(paramATRsp.value != NULL)
			// 	{
			// 		xy_free(paramATRsp.value);
			// 		g_Remote_AT_Rsp = NULL;
			// 	}
			// }			
			break;
		}
		case cis_data_type_string: {
			str_len = ON_WRITE_LEN + data->asBuffer.length;
			at_str = xy_malloc2(str_len);
			if(at_str == NULL)
            {
                xy_printf(0,XYAPP, WARN_LOG, "[CIS]memory not enough, write string fail");
                return CIS_RET_MEMORY_ERR;
            }
			// xy_Remote_AT_Req(data->asBuffer.buffer);
			if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
			{
#if LWM2M_COMMON_VER
				if (g_lwm2m_common_config_data->access_mode == 0)
					snprintf(at_str, str_len, "+QLAURC: \"write\",%d,%d,%d,%d,%d,%d,\"%s\",%d",
							 mid, uri->objectId, uri->instanceId, data->id, data->type,
							 data->asBuffer.length, data->asBuffer.buffer, i);
				else
				{
					xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
					memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
					xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_WRITE;
					xy_lwm2m_cached_urc_common->urc_data = xy_malloc2(str_len);
					if(xy_lwm2m_cached_urc_common->urc_data != NULL)
					{
                        snprintf(xy_lwm2m_cached_urc_common->urc_data, str_len, "\"write\",%d,%d,%d,%d,%d,%d,\"%s\",%d",
                                 mid, uri->objectId, uri->instanceId, data->id, data->type,
                                 data->asBuffer.length, data->asBuffer.buffer, i);
                        insert_cached_urc_node(xy_lwm2m_cached_urc_common);
					}
					else
					{
					    xy_free(xy_lwm2m_cached_urc_common);
					    xy_printf(0,XYAPP, WARN_LOG, "[cis_lwm2m]memory not enough, string cache fail");
					}
				}
#endif
			}
			else
			{
				snprintf(at_str, str_len, "+MIPLWRITE:%d,%d,%d,%d,%d,%d,%d,\"%s\",%d,%d",
						 CIS_REF, mid, uri->objectId, uri->instanceId, data->id, data->type,
						 data->asBuffer.length, data->asBuffer.buffer, flag, i);
				
			}
			//at_farps_fd_write(onenet_out_fd, at_str, strlen(at_str));
#if LWM2M_COMMON_VER
			if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->access_mode == 1)
			{
			}
			else
#endif
			    send_urc_to_ext(at_str, strlen(at_str));

			// if(g_Remote_AT_Rsp != NULL)
			// {
			// 	cis_memset(&paramATRsp, 0, sizeof(paramATRsp));

			// 	paramATRsp.ref = 0;
			// 	paramATRsp.objId = uri->objectId;
			// 	paramATRsp.insId = uri->instanceId;
			// 	paramATRsp.resId = data->id;
			// 	paramATRsp.value_type = cis_data_type_string;
			// 	paramATRsp.index = 0;
			// 	paramATRsp.flag = 0;
			// 	paramATRsp.ackid = 0;
			// 	paramATRsp.value = g_Remote_AT_Rsp;
			// 	paramATRsp.len = strlen(g_Remote_AT_Rsp);
			// 	cis_memcpy(paramATRsp.value, g_Remote_AT_Rsp, strlen(g_Remote_AT_Rsp));	

			// 	paramATRsp.msgId = xy_get_observeMsgId(onenet_context_refs[0].onenet_context, uri);
			// 	xy_printf(0,XYAPP, WARN_LOG, "[CIS]Remote AT mid(%d)", paramATRsp.msgId);
			// 	if(paramATRsp.msgId == -1)
			// 	{
			// 		if(paramATRsp.value != NULL)
			// 		{
			// 			xy_free(paramATRsp.value);
			// 			g_Remote_AT_Rsp = NULL;
			// 		}
			// 	}

			// 	onet_miplnotify_req(onenet_context_refs[paramATRsp.ref].onenet_context, &paramATRsp);
			// 	if(paramATRsp.value != NULL)
			// 	{
			// 		xy_free(paramATRsp.value);
			// 		g_Remote_AT_Rsp = NULL;
			// 	}
			// }
			break;
		}
		case cis_data_type_integer:
		{
			at_str = xy_malloc(MAX_ONE_NET_AT_SIZE);
			if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
			{
#if LWM2M_COMMON_VER
				if (g_lwm2m_common_config_data->access_mode == 0)
					snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+QLAURC: \"write\",%d,%d,%d,%d,%d,%d,%d,%d",
							 mid, uri->objectId, uri->instanceId, data->id, data->type,
							 4, (int)data->value.asInteger, i);
				else
				{
					xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
					memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
					xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_WRITE;
					xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
					snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"write\",%d,%d,%d,%d,%d,%d,%d,%d",
							 mid, uri->objectId, uri->instanceId, data->id, data->type,
							 4, (int)data->value.asInteger, i);
					insert_cached_urc_node(xy_lwm2m_cached_urc_common);
				}
#endif
			}
			else
			{
				snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+MIPLWRITE:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
						 CIS_REF, mid, uri->objectId, uri->instanceId, data->id, data->type,
						 4, (int)data->value.asInteger, flag, i);
			}
#if LWM2M_COMMON_VER
			//at_farps_fd_write(onenet_out_fd, at_str, strlen(at_str));
			if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->access_mode == 1)
			{
			}
			else
#endif
			    send_urc_to_ext(at_str, strlen(at_str));
			
			break;
		}
		case cis_data_type_float:
		{
			at_str = xy_malloc(MAX_ONE_NET_AT_SIZE);
			if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
			{
#if LWM2M_COMMON_VER
				if (g_lwm2m_common_config_data->access_mode == 0)
					snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+QLAURC: \"write\",%d,%d,%d,%d,%d,%d,%.3f,%d",
							 mid, uri->objectId, uri->instanceId, data->id, data->type,
							 8, data->value.asFloat, i);
				else
				{
					xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
					memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
					xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_WRITE;
					xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
					snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"write\",%d,%d,%d,%d,%d,%d,%.3f,%d",
							 mid, uri->objectId, uri->instanceId, data->id, data->type,
							 8, data->value.asFloat, i);
					insert_cached_urc_node(xy_lwm2m_cached_urc_common);
				}
#endif
			}
			else
			{
				snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+MIPLWRITE:%d,%d,%d,%d,%d,%d,%d,%.3f,%d,%d",
						 CIS_REF, mid, uri->objectId, uri->instanceId, data->id, data->type,
						 8, data->value.asFloat, flag, i);
			}
#if LWM2M_COMMON_VER
			//at_farps_fd_write(onenet_out_fd, at_str, strlen(at_str));
			if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->access_mode == 1)
			{
			}
			else
#endif
			    send_urc_to_ext(at_str, strlen(at_str));

			break;
		}
		case cis_data_type_bool:
		{
			at_str = xy_malloc(MAX_ONE_NET_AT_SIZE);
			if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
			{
#if LWM2M_COMMON_VER
				if (g_lwm2m_common_config_data->access_mode == 0)
					snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+QLAURC: \"write\",%d,%d,%d,%d,%d,%d,%d,%d",
							 mid, uri->objectId, uri->instanceId, data->id, data->type,
							 1, data->value.asBoolean, i);
				else
				{
					xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
					memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
					xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_WRITE;
					xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
					snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"write\",%d,%d,%d,%d,%d,%d,%d,%d",
							 mid, uri->objectId, uri->instanceId, data->id, data->type,
							 1, data->value.asBoolean, i);
					insert_cached_urc_node(xy_lwm2m_cached_urc_common);
				}
#endif
			}
			else
			{
				snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+MIPLWRITE:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
						 CIS_REF, mid, uri->objectId, uri->instanceId, data->id, data->type,
						 1, data->value.asBoolean, flag, i);
			}
#if LWM2M_COMMON_VER
			//at_farps_fd_write(onenet_out_fd, at_str, strlen(at_str));
			if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->access_mode == 1)
			{
			}
			else
#endif
			    send_urc_to_ext(at_str, strlen(at_str));
            
			break;
		}
		default:
			return CIS_CALLBACK_METHOD_NOT_ALLOWED;
		}

		if(at_str != NULL)
			xy_free(at_str);
		if(strBuffer != NULL)
			xy_free(strBuffer);

	}
	return CIS_CALLBACK_CONFORM;
}

//static cis_ret_t onet_on_execute_cb(void* context, cis_uri_t* uri, const uint8_t* buffer, uint32_t length, cis_mid_t mid)
cis_coapret_t onet_on_execute_cb(void* context, cis_uri_t* uri, const uint8_t* value, uint32_t length,cis_mid_t mid)
{
    char *at_str = xy_malloc2(MAX_ONE_NET_AT_SIZE + length + 5);
    if(at_str == NULL)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[CIS]memory not enough, execute fail");
        return CIS_RET_MEMORY_ERR;
    }
    memset(at_str, 0, MAX_ONE_NET_AT_SIZE + length + 5);
	// (void)context;
	st_context_t *contextP = (st_context_t *)context;

	if (length > 0)
	{
		if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
		{
#if LWM2M_COMMON_VER
			if (g_lwm2m_common_config_data->access_mode == 0)
				snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+QLAURC: \"execute\",%d,%d,%d,%d",
						 mid, uri->objectId, uri->instanceId, uri->resourceId);
			else
			{
				xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
				memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
				xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_EXECUTE;
				xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
				snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"execute\",%d,%d,%d,%d",
						 mid, uri->objectId, uri->instanceId, uri->resourceId);
				insert_cached_urc_node(xy_lwm2m_cached_urc_common);
			}
#endif
		}
		else
		{
#if VER_BC95			
			snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+MIPLEXECUTE: %d,%d,%d,%d,%d,%d,\"",
					 CIS_REF, mid, uri->objectId, uri->instanceId, uri->resourceId, length);
			memcpy(at_str + strlen(at_str), value, length);
			strcat(at_str, "\"");

#else
			snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+MIPLEXECUTE:%d,%d,%d,%d,%d,%d,",
					 CIS_REF, mid, uri->objectId, uri->instanceId, uri->resourceId, length);
			memcpy(at_str + strlen(at_str), value, length);
#endif
			
		}
	}
	else
	{
		if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
		{
#if LWM2M_COMMON_VER
			if (g_lwm2m_common_config_data->access_mode == 0)
				snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+QLAURC: \"execute\",%d,%d,%d,%d",
						 mid, uri->objectId, uri->instanceId, uri->resourceId);
			else
			{
				xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
				memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
				xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_EXECUTE;
				xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
				snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"execute\",%d,%d,%d,%d",
						 mid, uri->objectId, uri->instanceId, uri->resourceId);
				insert_cached_urc_node(xy_lwm2m_cached_urc_common);
			}
#endif
		}
		else
#if VER_BC95	
			snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+MIPLEXECUTE: %d,%d,%d,%d,%d",
					 CIS_REF, mid, uri->objectId, uri->instanceId, uri->resourceId);
#else
			snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+MIPLEXECUTE:%d,%d,%d,%d,%d",
					 CIS_REF, mid, uri->objectId, uri->instanceId, uri->resourceId);
#endif

	}
#if LWM2M_COMMON_VER
	if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->access_mode == 1)
	{
	}
	else
#endif
	    send_urc_to_ext(at_str, strlen(at_str));
    xy_free(at_str);

	return CIS_CALLBACK_CONFORM;
}
cis_coapret_t onet_on_observe_cb(void* context, cis_uri_t* uri, bool flag, cis_mid_t mid)
{
    char *at_str = xy_malloc2(MAX_ONE_NET_AT_SIZE);
    if(at_str == NULL)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[CIS]memory not enough, observe fail");
        return CIS_RET_MEMORY_ERR;
    }

    st_context_t *contextP = (st_context_t *)context;
    int ins_id = CIS_URI_IS_SET_INSTANCE(uri) ? uri->instanceId : -1;
    int res_id = CIS_URI_IS_SET_RESOURCE(uri) ? uri->resourceId : -1;

    if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
    {
#if LWM2M_COMMON_VER
        if (g_lwm2m_common_config_data->access_mode == 0)
            snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+QLAURC: \"observe\",%d,%d,%d,%d,%d",
                     mid, flag, uri->objectId, ins_id, res_id);
        else
        {
            xy_lwm2m_cached_urc_common_t *xy_lwm2m_cached_urc_common = xy_malloc(sizeof(xy_lwm2m_cached_urc_common_t));
            memset(xy_lwm2m_cached_urc_common, 0, sizeof(xy_lwm2m_cached_urc_common_t));
            xy_lwm2m_cached_urc_common->urc_type = XY_LWM2M_CACHED_URC_TYPE_OBSERVE;
            xy_lwm2m_cached_urc_common->urc_data = xy_malloc(MAX_ONE_NET_AT_SIZE);
            snprintf(xy_lwm2m_cached_urc_common->urc_data, MAX_ONE_NET_AT_SIZE, "\"observe\",%d,%d,%d,%d,%d",
                     mid, flag, uri->objectId, ins_id, res_id);
            insert_cached_urc_node(xy_lwm2m_cached_urc_common);
        }
#endif
    }
    else
#if VER_BC95	
        snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+MIPLOBSERVE: %d,%d,%d,%d,%d,%d",
                 CIS_REF, mid, flag, uri->objectId, ins_id, res_id);
#else
        snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+MIPLOBSERVE:%d,%d,%d,%d,%d,%d",
                 CIS_REF, mid, flag, uri->objectId, ins_id, res_id);
#endif

    if(contextP->cloud_platform == CLOUD_PLATFORM_ONENET && g_onenet_config_data->obs_autoack)
    {
        cis_response(onenet_context_refs[0].onenet_context, NULL, NULL, mid, get_coap_result_code(1),0);
    }
#if LWM2M_COMMON_VER
	if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->access_mode == 1)
	{
	}
	else
#endif
	    send_urc_to_ext(at_str, strlen(at_str));
    xy_free(at_str);

	return CIS_CALLBACK_CONFORM;
}

void onet_on_parameter_cb_ex(uint8_t toSet, uint8_t flag,char *str_params, const char *print,int param, uint8_t type)
{
    if ((toSet & flag) != 0) {
        if(strlen(str_params) > 0) {
            snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), ";");
        }
		if (type)
			snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), print, (float)param);
		else
        	snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), print, param);
    }
}

cis_coapret_t onet_on_parameter_cb(void* context, cis_uri_t* uri, cis_observe_attr_t parameters, cis_mid_t mid)
{
    char *at_str = xy_malloc2(MAX_SET_PARAM_AT_SIZE);
    if(at_str == NULL)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[CIS]memory not enough, para set fail0");
        return CIS_RET_MEMORY_ERR;
    }
    char *str_params = xy_malloc2(MAX_SET_PARAM_SIZE);
    if(str_params == NULL)
    {
        xy_free(at_str);
        xy_printf(0,XYAPP, WARN_LOG, "[CIS]memory not enough, para set fail1");
        return CIS_RET_MEMORY_ERR;
    }
    memset(str_params, 0, MAX_SET_PARAM_SIZE);
    int ins_id = CIS_URI_IS_SET_INSTANCE(uri) ? uri->instanceId : -1;
    int res_id = CIS_URI_IS_SET_RESOURCE(uri) ? uri->resourceId : -1;
	//uint8_t toSet = parameters.toSet;

	(void) context;

    if ((parameters.toSet & ATTR_FLAG_MIN_PERIOD) != 0) {
        snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), "pmin=%d",  parameters.minPeriod);
    }
    if ((parameters.toSet & ATTR_FLAG_MAX_PERIOD) != 0) {
        if(strlen(str_params) > 0) {
            snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), ";");
        }
        snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), "pmax=%d",  parameters.maxPeriod);
    }
	if ((parameters.toSet & ATTR_FLAG_LESS_THAN) != 0) {
        if(strlen(str_params) > 0) {
            snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), ";");
        }
        snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), "lt=%.1f",  parameters.lessThan);
    }
    if ((parameters.toSet & ATTR_FLAG_GREATER_THAN) != 0) {
        if(strlen(str_params) > 0) {
            snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), ";");
        }
        snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), "gt=%.1f",  parameters.greaterThan);
    }   
    if ((parameters.toSet & ATTR_FLAG_STEP) != 0) {
        if(strlen(str_params) > 0) {
            snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), ";");
        }
        snprintf(str_params + strlen(str_params), MAX_SET_PARAM_SIZE - strlen(str_params), "st=%.1f",  parameters.step);
    }
#if VER_BC95
    snprintf(at_str, MAX_SET_PARAM_AT_SIZE, "+MIPLPARAMETER: %d,%d,%d,%d,%d,%d,\"%s\"", CIS_REF, mid, uri->objectId,
             ins_id, res_id, strlen(str_params), str_params);
#else
    snprintf(at_str, MAX_SET_PARAM_AT_SIZE, "+MIPLPARAMETER:%d,%d,%d,%d,%d,%d,\"%s\"", CIS_REF, mid, uri->objectId,
             ins_id, res_id, strlen(str_params), str_params);
#endif
    send_urc_to_ext(at_str, strlen(at_str));
    xy_free(str_params);
    xy_free(at_str);
    return CIS_CALLBACK_CONFORM;
}
//static cis_ret_t onet_on_discover_cb(void* context,cis_uri_t* uri, cis_instcount_t instcount,cis_mid_t mid)
cis_coapret_t onet_on_discover_cb(void* context,cis_uri_t* uri,cis_mid_t mid)
{
    char *at_str = xy_malloc2(MAX_ONE_NET_AT_SIZE);
    if(at_str == NULL)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[CIS]memory not enough, discover fail");
        return CIS_RET_MEMORY_ERR;
    }

    (void) context;

#if VER_BC95
	snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+MIPLDISCOVER: %d,%d,%d", CIS_REF, mid, uri->objectId); //len parameter
#else
	snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+MIPLDISCOVER:%d,%d,%d", CIS_REF, mid, uri->objectId);
#endif

	send_urc_to_ext(at_str, strlen(at_str));
    xy_free(at_str);
	return CIS_CALLBACK_CONFORM;
}

//static void onet_on_event_cb(void* context, cis_evt_t eid)
void onet_on_event_cb(void* context, cis_evt_t eid, void* param)
{
	char *at_str = NULL;

	// (void) context;
	st_context_t *contextP = (st_context_t *)context;

    at_str = xy_malloc2(MAX_ONE_NET_AT_SIZE);
    if(at_str == NULL)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[CIS]memory not enough, event urc fail");
        return;
    }
	// ota_state_callback(eid - 40);
	onenet_resume_state_process(eid);
	//change fota report information
    switch(eid)
    {
        case CIS_EVENT_FIRMWARE_DOWNLOADING:
    	{
        	snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+FIRMWARE DOWNLOADING");
           	break;
    	}
        case CIS_EVENT_FIRMWARE_DOWNLOAD_FAILED:
		{
        	snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+FIRMWARE DOWNLOAD FAILED");
           	break;
		}
        case CIS_EVENT_FIRMWARE_DOWNLOADED:
		{
        	snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+FIRMWARE DOWNLOADED");
           	break;
		}
        case CIS_EVENT_FIRMWARE_UPDATING:
    	{
        	snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+FIRMWARE UPDATING");
           	break;
    	}
        case CIS_EVENT_FIRMWARE_UPDATE_SUCCESS:
		{
        	snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+FIRMWARE UPDATE SUCCESS");
           	break;
		}
        case CIS_EVENT_FIRMWARE_UPDATE_FAILED:
		{
        	snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+FIRMWARE UPDATE FAILED");
           	break;
		}
        case CIS_EVENT_FIRMWARE_UPDATE_OVER:
		{
        	snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+FIRMWARE UPDATE OVER");
           	break;
		}
        case CIS_EVENT_FIRMWARE_ERASE_SUCCESS:
		{
        	snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+FIRMWARE ERASE SUCCESS");
           	break;
		}
        case CIS_EVENT_COMMON_STATUS_READY:     //该事件仅做session文件保存，无须urc上报
        {
            goto out;
        }
        default:
        {
			if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
			{
#if LWM2M_COMMON_VER
				if (convert_cis_event_to_common_urc(at_str, MAX_ONE_NET_AT_SIZE, eid, param) < 0)
					goto out;
#endif
			}
			else
			{
#if VER_BC95
			snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+MIPLEVENT: %d,%d", CIS_REF, eid);
#else
			snprintf(at_str, MAX_ONE_NET_AT_SIZE, "+MIPLEVENT:%d,%d", CIS_REF, eid);
#endif
			}
			break;
		}
	}
	if (contextP->cloud_platform != CLOUD_PLATFORM_COMMON)
	{
		switch (eid)
		{
    		case CIS_EVENT_NOTIFY_SUCCESS:
    		case CIS_EVENT_NOTIFY_FAILED:
    		case CIS_EVENT_UPDATE_NEED:
    		case CIS_EVENT_RESPONSE_FAILED:
            {
    	       snprintf(at_str + strlen(at_str), MAX_ONE_NET_AT_SIZE - strlen(at_str), ",%d", (int)param);
               break;
            }

    		default:
    		{
    			break;
    		}
		}
	}
	//snprintf(at_str + strlen(at_str), MAX_ONE_NET_AT_SIZE - strlen(at_str), "\r\n");
	send_urc_to_ext(at_str, strlen(at_str));
out:
	xy_free(at_str);
    if(g_cis_downstream_cb != NULL)
        g_cis_downstream_cb(CALLBACK_TYPE_EVENT, NULL, NULL, 0, eid, cis_data_type_undefine, NULL, 0);

}

int onet_register(void* context, unsigned int lifetime)
{
	cis_callback_t callback;
	callback.onRead = onet_on_read_cb;
	callback.onWrite = onet_on_write_cb;
	callback.onExec = onet_on_execute_cb;
	callback.onObserve = onet_on_observe_cb;
    callback.onDiscover = onet_on_discover_cb;
	callback.onSetParams = onet_on_parameter_cb;
	callback.onEvent = onet_on_event_cb;

	return cis_register(context, lifetime, &callback);
}

cis_ret_t onet_mipladdobj_req(st_context_t *onenet_context, struct onenet_addobj *para)
{
	cis_ret_t ret = 0;
	cis_inst_bitmap_t bitmap = {0};
	uint8_t * instPtr;
	uint16_t instBytes;
	int i;
	cis_res_count_t  rescount;
	//make bitmap;
	bitmap.instanceCount = para->insCount;
	//bitmap.instanceBitmap = addobj->insBitmap; //"1101010111"

	instBytes = (bitmap.instanceCount - 1) / 8 + 1;
	instPtr = (uint8_t*)cis_malloc(instBytes);
	memset(instPtr, 0, instBytes);

	for (i = 0; i < bitmap.instanceCount; i++) {
		uint8_t instBytePos = i / 8;
		uint8_t instByteOffset = 7 - (i % 8);
		if (*(para->insBitmap + i) == '1') {
			instPtr[instBytePos] += 0x01 << instByteOffset;
		}
	}
	bitmap.instanceBitmap = instPtr;
	bitmap.instanceBytes = (para->insCount - 1) / 8 + 1;

	rescount.attrCount = para->attrCount;
    rescount.actCount = para->actCount;

	ret = cis_addobject(onenet_context, para->objId, &bitmap, &rescount);

	xy_free(instPtr);
	return ret;
}

cis_ret_t onet_read_param(struct onenet_read *param, cis_data_t* dataP)
{
    dataP->type = (cis_datatype_t)param->value_type;
    dataP->id = param->resId;
    if (dataP->type == cis_data_type_integer) {
        dataP->value.asInteger = (long long)strtoll(param->value,NULL,10);//atoll
    } else if (dataP->type == cis_data_type_float) {
        dataP->value.asFloat = atof((char *)param->value);
    } else if (dataP->type == cis_data_type_bool) {
        dataP->value.asBoolean = (int)strtol(param->value,NULL,10);
    } else if (dataP->type == cis_data_type_string) {
        dataP->asBuffer.length = param->len;
        dataP->asBuffer.buffer = (uint8_t*)(param->value);//(uint8_t*)strdup(param->value);
    } else if (dataP->type == cis_data_type_opaque) {
        dataP->asBuffer.length = param->len;
        dataP->asBuffer.buffer = (uint8_t*)(param->value);//(uint8_t*)strdup(param->value);
    } else {
        return CIS_CALLBACK_NOT_FOUND;
    }
    return CIS_RET_OK;
}

cis_ret_t onet_read_data(st_context_t *onenet_context, struct onenet_read *param)
{
	cis_data_t tmpdata = {0};
	cis_uri_t uri = {0};
	cis_ret_t ret = 0;
	cis_coapret_t result = CIS_RESPONSE_CONTINUE;
	
	if (param->flag != 0 && param->index == 0) {
		return CIS_RET_PARAMETER_ERR;
	}

	uri.objectId = param->objId;
	uri.instanceId = param->insId;
	uri.resourceId = param->resId;
	cis_uri_update(&uri);

	if ((ret = onet_read_param(param, &tmpdata)) != CIS_RET_OK) {
		return ret;
	}
	
	if (param->flag == 0 && param->index == 0) {
		result = CIS_RESPONSE_READ;
	}
	
	ret = cis_response(onenet_context, &uri, &tmpdata, param->msgId, result, param->raiflag);

	return ret;
}

cis_ret_t onet_notify_data(st_context_t *onenet_context, struct onenet_notify *param)
{
	cis_data_t tmpdata = {0};
	cis_uri_t uri = {0};
	cis_ret_t ret = 0;
	cis_coapret_t result = CIS_NOTIFY_CONTINUE;
	
	if (param->flag != 0 && param->index == 0) {
		return CIS_RET_PARAMETER_ERR;
	}

	if (param->ackid > 65535)
		return CIS_RET_PARAMETER_ERR;

	uri.objectId = param->objId;
	uri.instanceId = param->insId;
	uri.resourceId = param->resId;
	cis_uri_update(&uri);

	if ((ret = onet_read_param((struct onenet_read *)param, &tmpdata)) != CIS_RET_OK) {
		return ret;
	}

	if (param->flag == 0 && param->index == 0) {
		result = CIS_NOTIFY_CONTENT;
	}
	
	ret = cis_notify(onenet_context, &uri, &tmpdata, param->msgId, result, 1, param->ackid,param->raiflag);

	return ret;
}

//opt: onenet_context or ref as a parameter
cis_ret_t onet_miplread_req(st_context_t *onenet_context, struct onenet_read *param)
{
	cis_ret_t ret = CIS_RET_INVILID;

	ret = onet_read_data(onenet_context, param);
	return ret;
}

bool isPureNumAndAlpha(char *buf)
{
    while(*buf != '\0')
    {
        if(*buf < '0' || (*buf > '9' &&  *buf < 'A') || (*buf > 'Z' && * buf < 'a') || *buf > 'z')
        	return false;
        buf++;
    }

    return true;
}

int onet_mipcfg(uint8_t mode, uint8_t* param1, uint8_t* param2)
{
    //onenet任务开启状态下，不允许参数改变设置
    if (is_onenet_task_running(0)) {
        return CIS_Err_NotAllowed;
    }

    int port = -1;
    int autoack = -1;
    char *tmp = NULL;
    int ack_timeout = 0;
    int buf_cfg = 0;
    switch(mode)
    {
       case 0:  //非引导模式，并配置接入机 IP 地址和端口号
       case 1:  //开启引导模式，并配置引导服务器 IP 地址和端口号。默认的引导服务器 IP 地 址为 183.230.40.39，端口号为 5683
           if(param1 == NULL || param2 == NULL)
               return CIS_Err_Parameter;
           port = strtol(param2, &tmp, 10);
           if(strlen(param1)>100 || !xy_domain_is_valid(param1) || port < 1 || port > 65535 || (tmp != NULL && strlen(tmp) > 0))
               return CIS_Err_Parameter;

           strcpy(g_onenet_config_data->server_host, param1);
           g_onenet_config_data->server_port = port;
           g_onenet_config_data->bs_enable = mode;
           break;
       case 2:  //设置 CoAP 协议的 ACK_TIMEOUT 参数，ACK_TIMEOUT
           if(param1 == NULL || param2 == NULL)
               return CIS_Err_Parameter;
           ack_timeout = strtol(param2, &tmp, 10);
           if(ack_timeout < 2 || ack_timeout > 20 || strlen(param1) > 1|| *param1 != '1' || (tmp != NULL && strlen(tmp) > 0))
               return CIS_Err_Parameter;
           g_onenet_config_data->ack_timeout = g_ONENET_ACK_TIMEOUT = ack_timeout;
           break;
       case 3:  //设置是否启用自动响应订阅请求
           if(param1 == NULL)
               return CIS_Err_Parameter;
           autoack = strtol(param1, &tmp, 10);
           if((autoack != 0 && autoack != 1) || strlen(param1) > 1 || (tmp != NULL && strlen(tmp) > 0))
               return CIS_Err_Parameter;
           g_onenet_config_data->obs_autoack = autoack;
           break;
       case 4:  //设置是否使能鉴权接入并配置鉴权码(1-16个英文或数字)
           if(param1 == NULL || strlen(param1) > 1)
               return CIS_Err_Parameter;
           if(*param1 == '0')
           {
               memset(g_onenet_config_data->auth_code,0,sizeof(g_onenet_config_data->auth_code));
           }
           else if(*param1 == '1')
           {
               if(param2 == NULL || strlen(param2) <= 0 || strlen(param2) > 16 || !isPureNumAndAlpha(param2))
                   return CIS_Err_Parameter;
               strcpy(g_onenet_config_data->auth_code, param2);
           }
           else
               return CIS_Err_Parameter;
           break;
       case 5:  //设置是否启用 DTLS 模式并配置PSK(8-16个英文或数字)
           return CIS_Err_Parameter;        //暂不支持此功能，配置报错
           if(param1 == NULL || param2 == NULL || strlen(param1) > 1)
               return CIS_Err_Parameter;

           if(*param1 == '0')
               memset( g_onenet_config_data->psk,0,sizeof( g_onenet_config_data->psk));
           else if(*param1 == '1')
           {
               if(strlen(param2) < 8 || strlen(param2) > 16 || !isPureNumAndAlpha(param2))
                   return CIS_Err_Parameter;
               strcpy(g_onenet_config_data->psk, param2);
           }
           else
               return CIS_Err_Parameter;
           break;
       case 6:  //设置接收写数据的输出格式
           if(param1 == NULL || strlen(param1) > 1)
               return CIS_Err_Parameter;

           if(*param1 == '0')
              g_onenet_config_data->write_format = 0;
           else if(*param1 == '1')
              g_onenet_config_data->write_format = 1;
           else
              return CIS_Err_Parameter;
           break;
       case 7:    //下行数据缓存配置
           return CIS_Err_Parameter;        //暂不支持此功能，配置报错
           if(param1 == NULL || param2 == NULL)
               return CIS_Err_Parameter;
           buf_cfg = strtol(param1, &tmp, 10);
           if(buf_cfg < 0 || buf_cfg > 3 || (tmp != NULL && strlen(tmp) > 0) || (*param2 != '0' && *param2 != '1') || strlen(param2) > 1)
               return CIS_Err_Parameter;

           if(*param2 == '0')
               g_onenet_config_data->buf_URC_mode = 0;
           else if(*param2 == '1')
               g_onenet_config_data->buf_URC_mode = 1;
           else
               return CIS_Err_Parameter;
           g_onenet_config_data->buf_cfg = buf_cfg;
           break;
       default:
           return CIS_Err_Parameter;
           break;
    }

    //store g_onenet_config_data to lfs
    cloud_save_file(ONENET_CONFIG_NVM_FILE_NAME,(void*)g_onenet_config_data,sizeof(onenet_config_nvm_t));

    return CIS_OK;
}

/*
 * AT+MIPLCONFIG=<mode>,<parameter1>[,<parameter2>]
 */
int at_proc_miplconfig_req(char *at_buf, char **rsp_cmd)
{
    uint8_t mode = 0;
    uint8_t *param1 = NULL;
    uint8_t *param2 = NULL;
    uint32_t ret = -1;

    cis_module_init();
    onenet_resume_session();
    if(g_req_type == AT_CMD_REQ)
    {
        if(at_parse_param("%1d(0-7),%p(),%p", at_buf, &mode, &param1, &param2)!= AT_OK)
            return ATERR_PARAM_INVALID;

        ret = onet_mipcfg(mode, param1, param2);

        return at_get_errno_by_syserr(ret);
    }
    else if(g_req_type == AT_CMD_QUERY)
    {
        int len = strlen(g_onenet_config_data->server_host) + 150;
        *rsp_cmd = xy_malloc(len);
        if(strlen(g_onenet_config_data->server_host) == 0)
            snprintf(*rsp_cmd, len, "\r\n+MIPLCONFIG:%d,NULL,%d\r\n", g_onenet_config_data->bs_enable, g_onenet_config_data->server_port);
        else
            snprintf(*rsp_cmd, len, "\r\n+MIPLCONFIG:%d,%s,%d\r\n", g_onenet_config_data->bs_enable, g_onenet_config_data->server_host, g_onenet_config_data->server_port);

        snprintf(*rsp_cmd + strlen(*rsp_cmd), len- strlen(*rsp_cmd), "+MIPLCONFIG:2,%d\r\n", g_ONENET_ACK_TIMEOUT);
        snprintf(*rsp_cmd + strlen(*rsp_cmd), len- strlen(*rsp_cmd), "+MIPLCONFIG:3,%d\r\n", g_onenet_config_data->obs_autoack);
        if(strlen(g_onenet_config_data->auth_code) != 0)
            snprintf(*rsp_cmd + strlen(*rsp_cmd), len - strlen(*rsp_cmd), "+MIPLCONFIG:4,1,%s\r\n",g_onenet_config_data->auth_code);
        else
            snprintf(*rsp_cmd + strlen(*rsp_cmd), len - strlen(*rsp_cmd), "+MIPLCONFIG:4,0\r\n");
        if(g_onenet_config_data->dtls_enable)
            snprintf(*rsp_cmd + strlen(*rsp_cmd), len - strlen(*rsp_cmd), "+MIPLCONFIG:5,1,%s\r\n",g_onenet_config_data->psk);
        else
            snprintf(*rsp_cmd + strlen(*rsp_cmd), len - strlen(*rsp_cmd), "+MIPLCONFIG:5,0\r\n");
        snprintf(*rsp_cmd + strlen(*rsp_cmd), len - strlen(*rsp_cmd), "+MIPLCONFIG:6,%d\r\n\r\nOK\r\n", g_onenet_config_data->write_format );
        //snprintf(*rsp_cmd + strlen(*rsp_cmd), 200 - strlen(*rsp_cmd), "+MIPLCONFIG:7,%d,%d\r\n\r\nOK\r\n", g_onenet_config_data->buf_cfg, g_onenet_config_data->buf_URC_mode);
    }
#if (AT_CUT!=1)
	else if(g_req_type == AT_CMD_TEST)
	{
		*rsp_cmd = xy_malloc(70);
		snprintf(*rsp_cmd, 70, "<mode>,<parameter1>[,<parameter2>]");
	}
#endif
	else
    {
        return ATERR_PARAM_INVALID;
    }

    return AT_END;
}


//AT+MIPLCREATE=<totalsize>,<config>,<index>,<currentsize>,<flag>
int at_onenet_create_req(char *at_buf, char **rsp_cmd)
{
    cis_module_init();

    if(!xy_tcpip_is_ok())
    {
        return ATERR_NOT_NET_CONNECT;
    }

    if(g_req_type == AT_CMD_REQ)
    {
        return at_proc_miplconf_req(at_buf,rsp_cmd);
    }
    else if(g_req_type == AT_CMD_ACTIVE)
    {
        return at_proc_miplcreate_req(at_buf,rsp_cmd);
    }
    else
    {
        return ATERR_PARAM_INVALID;
    }
    return AT_END;
}

/*
 * AT+MIPLCREATE
 */
int at_proc_miplcreate_req(char *at_buf, char **rsp_cmd)
{
    onenet_context_reference_t *onenet_context_ref = NULL;
    onenet_context_config_t *onenet_context_config = NULL;
    unsigned int errorcode = ATERR_NOT_ALLOWED;
    osThreadAttr_t task_attr = {0};

    cis_module_init();

    onenet_resume_session();

    onenet_context_ref = (onenet_context_reference_t *)get_free_onet_context_ref();
    if (onenet_context_ref == NULL) {
        errorcode = ATERR_NOT_ALLOWED;
        goto failed;
    }

    onenet_context_config = &onenet_context_configs[onenet_context_ref->ref];
    if (onenet_context_config->config_hex != NULL)
    {
        xy_free(onenet_context_config->config_hex);
    }
    onenet_context_config->config_hex = cis_cfg_tool(g_onenet_config_data->server_host, g_onenet_config_data->server_port, g_onenet_config_data->bs_enable,
                    g_onenet_config_data->auth_code,g_onenet_config_data->dtls_enable,g_onenet_config_data->psk,&onenet_context_config->total_len);
    onenet_context_config->offset = onenet_context_config->total_len;
    onenet_context_config->index = 0;

    if (onet_init(onenet_context_ref, onenet_context_config) < 0)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[CIS]at_miplcreate onet_init error\r\n");
        errorcode = ATERR_NOT_ALLOWED;
        goto failed;
    }
    else
    {
        *rsp_cmd = xy_malloc(32);

        task_attr.name = "onenet_tk";
        task_attr.priority = osPriorityNormal1;
        task_attr.stack_size = osStackShared;
        onenet_context_ref->onet_at_thread_id = osThreadNew ((osThreadFunc_t)(onet_at_pump), onenet_context_ref, &task_attr);

        sprintf(*rsp_cmd, "+MIPLCREATE:%d", onenet_context_ref->ref);
    }

    return AT_END;

failed:
    if (onenet_context_config != NULL && onenet_context_config->config_hex != NULL)
    {
        xy_free(onenet_context_config->config_hex);
        memset(onenet_context_config, 0, sizeof(onenet_context_config_t));
    }

    if(onenet_context_ref != NULL)
    {
        if (onenet_context_ref->onenet_context != NULL)
            cis_deinit((void **)&onenet_context_ref->onenet_context);

        free_onenet_context_ref(onenet_context_ref);
    }
    return  errorcode;
}

//解析config_hex
int onenet_config_hex_parsing(char *config_hex, int hex_len)
{
    ciscfg_context_t *cfg_tmp = NULL;
    int len = 0;
    char port_tmp[10] = {0};
    uint8_t *tmp_ptr = NULL;
    
    if(cis_config_init(&cfg_tmp, config_hex, hex_len) == 0)
    {
        tmp_ptr = strrchr(cfg_tmp->cfgNet.host.data, ':');
        if(tmp_ptr != NULL) 
        {
            len = tmp_ptr - cfg_tmp->cfgNet.host.data;
            memset(g_onenet_config_data->server_host, 0, sizeof(g_onenet_config_data->server_host));
            memcpy(g_onenet_config_data->server_host, cfg_tmp->cfgNet.host.data, len);
            memcpy(port_tmp, tmp_ptr+1, cfg_tmp->cfgNet.host.len - len - 1);
            g_onenet_config_data->server_port = (int)strtol(port_tmp,NULL,10);
        }
        g_onenet_config_data->bs_enable = cfg_tmp->cfgNet.bs_enabled;
        //user_data= "AuthCode:xxxx;PSK:xxxx;"
        memset(g_onenet_config_data->auth_code, 0, sizeof(g_onenet_config_data->auth_code));
        memset(g_onenet_config_data->psk, 0, sizeof(g_onenet_config_data->psk));
        if(cfg_tmp->cfgNet.user_data.len > 5)       //len > strlen("AuthCode:;Psk:;")
        {
            tmp_ptr = strstr(cfg_tmp->cfgNet.user_data.data, "AuthCode:");
            if(tmp_ptr != NULL)
            {
                tmp_ptr = tmp_ptr + 9;
                len = (uint8_t *)strchr(tmp_ptr, ';') - tmp_ptr;
                if(len > 0 && len <20)
                {
                    memcpy(g_onenet_config_data->auth_code, tmp_ptr, len);
                }
            }

            tmp_ptr = strstr(cfg_tmp->cfgNet.user_data.data, "PSK:");
            if(tmp_ptr != NULL)
            {
                tmp_ptr = tmp_ptr + 4;
                len = (uint8_t *)strchr(tmp_ptr, ';') - tmp_ptr;
                if(len > 0 && len <20)
                {
                    memcpy(g_onenet_config_data->psk, tmp_ptr, len);
                }
            }
        }
    }
    else
        return XY_ERR;

    if(cfg_tmp != NULL)
        xy_free(cfg_tmp);

    return CIS_OK;
}


/*
 * AT+MIPLCREATE=<totalsize>,<config>,<index>,<currentsize>,<flag>
 */
int at_proc_miplconf_req(char *at_buf, char **rsp_cmd)
{
    onenet_context_reference_t *onenet_context_ref = NULL;
    onenet_context_config_t *onenet_context_config = NULL;
    int  totalsize = 0;
    int  index = 0;
    int  currentsize = 0;
    int  flag = 0;
    unsigned int errorcode = ATERR_NOT_ALLOWED;
    osThreadAttr_t task_attr = {0};
    if(strlen(at_buf)/2 <= 0)
        return ATERR_PARAM_INVALID;

    char *config = xy_malloc(strlen(at_buf)/2);
    cis_module_init();

    onenet_resume_session();

    if (at_parse_param("%d(1-),%h,%d(0-),%l(0-120),%d(0-2)", at_buf, &totalsize,config, &index, &currentsize, &flag) != AT_OK || totalsize < currentsize)
    {
        errorcode = ATERR_PARAM_INVALID;
        goto failed;
    }

    onenet_context_config = find_proper_onenet_context_config(totalsize, index, currentsize);
    if (onenet_context_config == NULL)
    {
        errorcode = ATERR_NOT_ALLOWED;
        goto failed;
    }

    if (onenet_context_config->config_hex == NULL)
    {
        memset(onenet_context_config, 0, sizeof(onenet_context_config_t));
        onenet_context_config->config_hex = xy_malloc(totalsize);
        onenet_context_config->total_len = totalsize;
    }

    memcpy(onenet_context_config->config_hex + onenet_context_config->offset, config, currentsize);
    onenet_context_config->offset += currentsize;
    onenet_context_config->index = index;

    if (index ==0 && flag == 0)
    {
        if (onenet_context_config->total_len != onenet_context_config->offset)
        {
            errorcode = ATERR_PARAM_INVALID;
            goto failed;
        }
        onenet_context_ref = (onenet_context_reference_t *)get_free_onet_context_ref();
        if (onenet_context_ref == NULL) {
            errorcode = ATERR_NOT_ALLOWED;
            goto failed;
        }

        if (onet_init(onenet_context_ref, onenet_context_config) < 0)
        {
            errorcode = ATERR_NOT_ALLOWED;
            goto failed;
        }
        else
        {
            *rsp_cmd = xy_malloc(32);
            sprintf(*rsp_cmd, "+MIPLCREATE:%d", onenet_context_ref->ref);

            task_attr.name = "onenet_tk";
            task_attr.priority = osPriorityNormal1;
            task_attr.stack_size = osStackShared;
            onenet_context_ref->onet_at_thread_id = osThreadNew((osThreadFunc_t)(onet_at_pump), onenet_context_ref, &task_attr);

            //store factory_nv  //解析config_hex,兼容带参创建指令，需要解析出config参数后保存
            if(onenet_config_hex_parsing(onenet_context_config->config_hex, onenet_context_config->total_len) == XY_OK) 
                cloud_save_file(ONENET_CONFIG_NVM_FILE_NAME,(void*)g_onenet_config_data,sizeof(onenet_config_nvm_t));
        }
    }
    if (config != NULL)
        xy_free(config);
    return  AT_END;

failed:
    if (config != NULL)
        xy_free(config);
    if (onenet_context_config != NULL && onenet_context_config->config_hex != NULL)
    {
        xy_free(onenet_context_config->config_hex);
        onenet_context_config->config_hex = NULL;
        memset(onenet_context_config, 0, sizeof(onenet_context_config_t));
    }

    if(onenet_context_ref != NULL)
    {
        if (onenet_context_ref->onenet_context != NULL)
            cis_deinit((void **)&onenet_context_ref->onenet_context);

        if (onenet_context_ref->onet_at_thread_id != NULL)
        {
            osThreadTerminate(onenet_context_ref->onet_at_thread_id);
            onenet_context_ref->onet_at_thread_id = NULL;
        }
        free_onenet_context_ref(onenet_context_ref);
    }

    return  errorcode;
}

/*
 * AT+MIPLDELETE=<ref>
 */
int at_proc_mipldel_req(char *at_buf, char **rsp_cmd)
{
    cis_module_init();
    if(g_req_type == AT_CMD_REQ)
    {
        int ref = 0;
        if(!xy_tcpip_is_ok()) {
            return ATERR_NOT_NET_CONNECT;
        }

        onenet_resume_session();

        if (at_parse_param("%d(0-0)", at_buf, &ref) != AT_OK) 
        {
            return ATERR_PARAM_INVALID;
        }

        if (!is_onenet_task_running(ref)) {
            if(onenet_context_configs[ref].config_hex != NULL)
            {
                xy_free(onenet_context_configs[ref].config_hex);
                memset(&onenet_context_configs[ref], 0, sizeof(onenet_context_config_t));
                return AT_END;
            }
            else
            {
                return ATERR_PARAM_INVALID;
            }
        }

        //onet_deinit(&onenet_context_refs[ref]);
        osMutexAcquire(g_onenet_mutex, osWaitForever);
        cis_unregister(onenet_context_refs[ref].onenet_context);
        osMutexRelease(g_onenet_mutex);
        while (onenet_context_refs[ref].onenet_context != NULL && onenet_context_refs[ref].onenet_context->registerEnabled != 0)
        {
            osSemaphoreAcquire(g_cis_del_sem, osWaitForever);
        }

        onet_deinit(&onenet_context_refs[ref]);

        return AT_END;
    }
#if (AT_CUT!=1)
    else if(g_req_type == AT_CMD_TEST)
    {
        *rsp_cmd = xy_malloc(40);
        snprintf(*rsp_cmd, 40, "<ref>");
    }
#endif
    else
        return ATERR_PARAM_INVALID;

    return AT_END;
}

/*
 * AT+MIPLOPEN=<ref>,<lifetime>,[<timeout>]
 */
int at_proc_miplopen_req(char *at_buf, char **rsp_cmd)
{
    cis_module_init();
    if(g_req_type == AT_CMD_REQ)
    {
        int ref = 0;
        int lifetime = 0;
#if VER_BC95
        int timeout = 0x1E;
#else
        int timeout = 0;
#endif
        int ret= CIS_RET_ERROR;

        if(!xy_tcpip_is_ok()) {
            return ATERR_NOT_NET_CONNECT;
        }

        onenet_resume_session();
#if VER_BC95
        if (at_parse_param("%d(0-0),%d(16-268435454),%d[30-65535]", at_buf, &ref , &lifetime, &timeout) != AT_OK) {
#else
        if (at_parse_param("%d(0-0),%d(120-268435455),%d[30-65535]", at_buf, &ref , &lifetime, &timeout) != AT_OK) {
#endif
            return ATERR_PARAM_INVALID;
        }

#if VER_BC95
		g_onenet_config_data->reg_timeout = timeout;
#endif
        if (!is_onenet_task_running(ref) || onenet_context_refs[ref].onenet_context->registerEnabled == true) {
            return ATERR_NOT_ALLOWED;
        }

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = onet_register(onenet_context_refs[ref].onenet_context, lifetime);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK)
            return ATERR_PARAM_INVALID;

    }
#if (AT_CUT!=1)
    else if(g_req_type == AT_CMD_TEST)
    {
        *rsp_cmd = xy_malloc(60);
        snprintf(*rsp_cmd, 60, "<ref>,<lifetime>[,<timeout>]");
    }
#endif
    else
    {
        return ATERR_PARAM_INVALID;
    }
    return AT_END;
}

/*
 *  AT+MIPLADDOBJ=<ref>,<objectid>,<instancecount>,<instancebitmap>,<attributecount>,<actioncount>
 */
int at_proc_mipladdobj_req(char *at_buf, char **rsp_cmd)
{
    cis_module_init();
    if(g_req_type == AT_CMD_REQ)
    {
        struct onenet_addobj param = {0};
        int ret= CIS_RET_ERROR;

        if(!xy_tcpip_is_ok()) {
            return ATERR_NOT_NET_CONNECT;
        }

        onenet_resume_session();

        if (at_parse_param("%d(0-0),%d(0-65535),%l(1-8),%p(),%d(1-49),%d(0-49)", at_buf, &param.ref, &param.objId, &param.insCount, &param.insBitmap, &param.attrCount, &param.actCount) != AT_OK) {
            return ATERR_PARAM_INVALID;
        }

        if (!is_onenet_task_running(param.ref)) {
            return ATERR_NOT_ALLOWED;
        }

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = onet_mipladdobj_req(onenet_context_refs[param.ref].onenet_context,&param);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_NO_ERROR) {
            return ATERR_NOT_ALLOWED;
        }

        return AT_END;
    }
#if (AT_CUT!=1)
	else if(g_req_type == AT_CMD_TEST)
	{
		*rsp_cmd = xy_malloc(100);
		snprintf(*rsp_cmd, 100, "<ref>,<objID>,<inscount>,<insbitmap>,<attrcount>,<actcount>");
	}
#endif
    else
    {
        return ATERR_PARAM_INVALID;
    }

    return  AT_END;
}

/*
 * AT+MIPLDELOBJ=<ref>,<objectid>
 */
int at_proc_mipldelobj_req(char *at_buf, char **rsp_cmd)
{
    cis_module_init();
    if(g_req_type == AT_CMD_REQ)
    {
        struct onenet_delobj param = {0};
        int ret= CIS_RET_ERROR;

        if(!xy_tcpip_is_ok()) {
            return ATERR_NOT_NET_CONNECT;
        }

        onenet_resume_session();

        if (at_parse_param("%d(0-0),%d(0-65535)", at_buf, &param.ref, &param.objId) != AT_OK) {
            return ATERR_PARAM_INVALID;
        }

        if (!is_onenet_task_running(param.ref)) {
            return ATERR_NOT_ALLOWED;
        }

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = cis_delobject(onenet_context_refs[param.ref].onenet_context, param.objId);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_NO_ERROR)
            return ATERR_PARAM_INVALID;

        return AT_END;
    }
#if (AT_CUT!=1)
    else if(g_req_type == AT_CMD_TEST)
    {
        *rsp_cmd = xy_malloc(50);
        snprintf(*rsp_cmd, 50, "<ref>,<objID>");
    }
#endif
    else
    {
        return ATERR_PARAM_INVALID;
    }

    return  AT_END;
}

/*
 * AT+MIPLCLOSE=<ref>
 */
int at_proc_miplclose_req(char *at_buf, char **rsp_cmd)
{
    cis_module_init();
    if(g_req_type == AT_CMD_REQ)
    {
        int ref = 0;
        int ret= CIS_RET_ERROR;

        if(!xy_tcpip_is_ok()) {
            return ATERR_NOT_NET_CONNECT;
        }

        onenet_resume_session();

        if (at_parse_param("%d(0-0)", at_buf, &ref) != AT_OK) {
            return ATERR_PARAM_INVALID;
        }

        if (!is_onenet_task_running(ref)) {
            return ATERR_NOT_ALLOWED;
        }

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        if (onenet_context_refs[ref].onenet_context->registerEnabled == true)
            ret = cis_unregister(onenet_context_refs[ref].onenet_context);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK)
            return ATERR_PARAM_INVALID;

        return AT_END;
    }
#if (AT_CUT!=1)
	else if(g_req_type == AT_CMD_TEST)
	{
		*rsp_cmd = xy_malloc(40);
		snprintf(*rsp_cmd, 40, "<ref>");
	}
#endif
    else
    {
        return ATERR_PARAM_INVALID;
    }

    return  AT_END;
}

/*
 * AT+MIPLNOTIFY=<ref>,<msgid>,<objectid>,<instanceid>,<resourceid>,<valuetype>,<len>,<value>,<index>,<flag>[,<ackid>[,<raimode>]]
 */
int at_proc_miplnotify_req(char *at_buf, char **rsp_cmd)
{
    cis_module_init();
    if(g_req_type == AT_CMD_REQ)
    {
        st_observed_t* observe = NULL;
        struct onenet_notify param = {0};
        int errorcode = AT_END;
        cis_ret_t ret = CIS_RET_INVILID;
        int raiType = 0;
        char *at_value = NULL;
        st_uri_t uriP = {0};

        if(!xy_tcpip_is_ok()) {
            return ATERR_NOT_NET_CONNECT;
        }

        onenet_resume_session();

        param.value = xy_malloc2(strlen(at_buf));
        if(param.value == NULL)
            return ATERR_NO_MEM;
        memset(param.value, 0, strlen(at_buf));

        if (at_parse_param("%d(0-0),%d(0-),%d(0-65535),%d(0-8),%d(0-65535),%d(1-5),%d(0-1400),%p(),%d(0-),%d(0-2),%d[0-65535],%d[0|0x200|0x400]",
                at_buf, &param.ref, &param.msgId, &param.objId, &param.insId, &param.resId, &param.value_type,
                &param.len, &at_value, &param.index, &param.flag, &param.ackid, &raiType) != AT_OK)
        {
            goto param_error;
        }

        //value
        if (!onet_at_get_notify_value(param.value_type, param.len, at_value, param.value) ||
                (param.ackid != 0 && raiType == 0x200) || onet_set_rai(raiType,&param.raiflag) != 0)
        {
            goto param_error;
        }

        if (!is_onenet_task_running(param.ref)) {
            goto status_error;
        }

        uri_make(param.objId, URI_INVALID, URI_INVALID, &uriP);
        observe = observe_findByUri(onenet_context_refs[0].onenet_context, &uriP);
        if (observe == NULL)
        {
            uri_make(param.objId, param.insId, URI_INVALID, &uriP);
            observe = observe_findByUri(onenet_context_refs[0].onenet_context, &uriP);
            if (observe == NULL)
            {
                uri_make(param.objId, param.insId, param.resId, &uriP);
                observe = observe_findByUri(onenet_context_refs[0].onenet_context, &uriP);
            }
        }

        if(observe == NULL){
            goto param_error;
        }

        param.msgId = observe->msgid;
        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = onet_miplnotify_req(onenet_context_refs[param.ref].onenet_context, &param);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK && ret != COAP_205_CONTENT)
            goto param_error;
        else
            goto out;

    param_error:
        errorcode = ATERR_PARAM_INVALID;
        goto out;
    status_error:
        errorcode = ATERR_NOT_ALLOWED;
    out:
        if (param.value != NULL)
            xy_free(param.value);
        return  errorcode;
    }
#if (AT_CUT!=1)
    else if(g_req_type == AT_CMD_TEST)
    {
        *rsp_cmd = xy_malloc(140);
        snprintf(*rsp_cmd, 140, "<ref>,<msgID>,<objID>,<insID>,<resID>,<value_type>,<len>,<value>,<index>,<flag>[,<ackID>[,<raimode>]]");
    }
#endif
    else
    {
        return  ATERR_PARAM_INVALID;
    }

    return  AT_END;
}

/*
 * AT+MIPLREADRSP=<ref>,<msgid>,<result>[,<objectid>,<instanceid>,<resourceid>,<valuetype>,<len>,<value>,<index>,<flag>[,<raimode>]]
 */
int at_proc_miplread_req(char *at_buf, char **rsp_cmd)
{
    cis_module_init();
    if(g_req_type == AT_CMD_REQ)
    {
        struct onenet_read param = {0};
        cis_coapret_t result;
        unsigned int errorcode = AT_END;
        int raiType = 0;
        int ret= CIS_RET_ERROR;
        char *at_value = NULL;

        if(!xy_tcpip_is_ok()) {
            return ATERR_NOT_NET_CONNECT;
        }

        param.value = xy_malloc2(strlen(at_buf));
        if(param.value == NULL)
            return ATERR_NO_MEM;
        memset(param.value, 0, strlen(at_buf));

        if (at_parse_param("%d(0-0),%d(0-),%d(),%d,%d,%d,%d,%d,%p,%d(0-),%d(0-2),%d[0|0x200|0x400]",
                at_buf, &param.ref, &param.msgId, &param.result, &param.objId, &param.insId, &param.resId,
                &param.value_type, &param.len, &at_value, &param.index, &param.flag, &raiType) != AT_OK) {
            goto param_error;
        }

        if (!is_onenet_task_running(param.ref)) {
            goto status_error;
        }

        if (0 != check_coap_result_code(param.result, RSP_READ))
            goto param_error;

        if(onet_set_rai(raiType,&param.raiflag) != 0)
            goto param_error;

        if ((result = get_coap_result_code(param.result)) != CIS_COAP_205_CONTENT){

            if(onet_check_reqType(CALLBACK_TYPE_READ, param.msgId) != 0)
                goto param_error;

            osMutexAcquire(g_onenet_mutex, osWaitForever);
            cis_response(onenet_context_refs[param.ref].onenet_context, NULL, NULL, param.msgId, result,param.raiflag);
            osMutexRelease(g_onenet_mutex);
            goto out;
        }
        else
        {
            if(onet_check_reqType_uri(CALLBACK_TYPE_READ, param.msgId, param.objId, param.insId, param.resId) != 0)
                goto param_error;
        }

        if (!onet_at_get_notify_value(param.value_type, param.len, at_value, param.value))
        {
            goto param_error;
        }

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = onet_miplread_req(onenet_context_refs[param.ref].onenet_context, &param);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK)
            goto param_error;
        else
            goto out;
    param_error:
        errorcode = ATERR_PARAM_INVALID;
        goto out;
    status_error:
        errorcode = ATERR_NOT_ALLOWED;
    out:
        if (param.value != NULL)
            xy_free(param.value);
        return  errorcode;
	}	
#if (AT_CUT!=1)
	else if(g_req_type == AT_CMD_TEST)
	{
		*rsp_cmd = xy_malloc(140);
		snprintf(*rsp_cmd, 140, "<ref>,<msgID>,<result>[,<objID>,<insID>,<resID>,<value_type>,<len>,<value>,<index>,<flag>[,<raimode>]]");
	}
#endif
	else
	{
	    return  ATERR_PARAM_INVALID;
	}

	return  AT_END;
}

/*
 * AT+MIPLWRITERSP=<ref>,<msgid>,<result>[,<raimode>]
 */
int at_proc_miplwrite_req(char *at_buf, char **rsp_cmd)
{
    cis_module_init();
    if(g_req_type == AT_CMD_REQ)
    {
        struct onenet_write_exe param = {0};
        int raiType = 0;
        int ret= CIS_RET_ERROR;

        if(!xy_tcpip_is_ok()) {
            return ATERR_NOT_NET_CONNECT;
        }

        if (at_parse_param("%d(0-0),%d(0-),%d(),%d[0|0x200|0x400]", at_buf, &param.ref, &param.msg_id, &param.result, &raiType) != AT_OK) {
            return ATERR_PARAM_INVALID;
        }

        if(onet_set_rai(raiType,&param.raiflag) != 0)
            return ATERR_PARAM_INVALID;

        if (!is_onenet_task_running(param.ref)) {
            return ATERR_NOT_ALLOWED;
        }

        if (0 != check_coap_result_code(param.result, RSP_WRITE) || onet_check_reqType(CALLBACK_TYPE_WRITE, param.msg_id) != 0)
            return ATERR_PARAM_INVALID;

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = cis_response(onenet_context_refs[param.ref].onenet_context, NULL, NULL, param.msg_id, get_coap_result_code(param.result),param.raiflag);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK)
            return ATERR_PARAM_INVALID;

    }
#if (AT_CUT!=1)
    else if(g_req_type == AT_CMD_TEST)
    {
        *rsp_cmd = xy_malloc(70);
        snprintf(*rsp_cmd, 70, "<ref>,<msgID>,<result>[,<raimode>]");
    }
#endif
    else
    {
        return ATERR_PARAM_INVALID;
    }

    return AT_END;
}

/*
 * AT+MIPLEXECUTERSP=<ref>,<msgid>,<result>[,<raimode>]
 */
int at_proc_miplexecute_req(char *at_buf, char **rsp_cmd)
{
    cis_module_init();
    if(g_req_type == AT_CMD_REQ)
    {
        struct onenet_write_exe param = {0};
        int raiType = 0;
        int ret= CIS_RET_ERROR;

        if(!xy_tcpip_is_ok()) {
            return ATERR_NOT_NET_CONNECT;
        }

        if (at_parse_param("%d(0-0),%d(0-),%d(),%d[0|0x200|0x400]", at_buf, &param.ref, &param.msg_id, &param.result, &raiType) != AT_OK) {
            return ATERR_PARAM_INVALID;
        }

        if(onet_set_rai(raiType,&param.raiflag) != 0)
            return ATERR_PARAM_INVALID;

        if (!is_onenet_task_running(param.ref)) {
            return ATERR_NOT_ALLOWED;
        }

        if (0 != check_coap_result_code(param.result, RSP_EXECUTE) || onet_check_reqType(CALLBACK_TYPE_EXECUTE, param.msg_id) != 0)
            return ATERR_PARAM_INVALID;

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = cis_response(onenet_context_refs[param.ref].onenet_context, NULL, NULL, param.msg_id, get_coap_result_code(param.result),param.raiflag);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK)
            return ATERR_PARAM_INVALID;

    }
#if (AT_CUT!=1)
    else if(g_req_type == AT_CMD_TEST)
    {
        *rsp_cmd = xy_malloc(70);
        snprintf(*rsp_cmd, 70, "<ref>,<msgID>,<result>[,<raimode>]");
    }
#endif
    else
    {
        return ATERR_PARAM_INVALID;
    }

    return  AT_END;
}

/*
 * AT+MIPLOBSERVERSP=<ref>,<msgid>,<result>>[,<raimode>]
 */
int at_proc_miplobserve_req(char *at_buf, char **rsp_cmd)
{
    cis_module_init();
    if(g_req_type == AT_CMD_REQ)
    {
        struct onenet_write_exe param = {0};
        int raiType = 0;
        int ret= CIS_RET_ERROR;
        cis_mid_t temp_observeMid;

        if(!xy_tcpip_is_ok()) {
            return ATERR_NOT_NET_CONNECT;
        }

        if (at_parse_param("%d(0-0),%d(0-),%d(),%d[0|0x200|0x400]", at_buf, &param.ref, &param.msg_id, &param.result, &raiType) != AT_OK) {
            return ATERR_PARAM_INVALID;
        }

        if(onet_set_rai(raiType,&param.raiflag) != 0)
            return ATERR_PARAM_INVALID;

        if (!is_onenet_task_running(param.ref)) {
            return ATERR_NOT_ALLOWED;
        }

        if (0 != check_coap_result_code(param.result, RSP_OBSERVE))
            return ATERR_PARAM_INVALID;

        if(onet_check_reqType(CALLBACK_TYPE_OBSERVE, param.msg_id) != 0 && onet_check_reqType(CALLBACK_TYPE_OBSERVE_CANCEL, param.msg_id) != 0)
            return ATERR_PARAM_INVALID;


        osMutexAcquire(g_onenet_mutex, osWaitForever);
        if (observe_findByMsgid(onenet_context_refs[param.ref].onenet_context, param.msg_id) == NULL) {
            if (!packet_asynFindObserveRequest(onenet_context_refs[param.ref].onenet_context, param.msg_id, &temp_observeMid)) {
                osMutexRelease(g_onenet_mutex);
                return ATERR_PARAM_INVALID;
            }
        }
        ret = cis_response(onenet_context_refs[param.ref].onenet_context, NULL, NULL, param.msg_id, get_coap_result_code(param.result),param.raiflag);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK)
            return ATERR_PARAM_INVALID;

    }
#if (AT_CUT!=1)
    else if(g_req_type == AT_CMD_TEST)
    {
        *rsp_cmd = xy_malloc(70);
        snprintf(*rsp_cmd, 70, "<ref>,<msgID>,<result>[,<raimode>]");
    }
#endif
    else
    {
        return ATERR_PARAM_INVALID;
    }
    return  AT_END;
}

/*
 * AT+MIPLVER?
 */
int at_proc_miplver_req(char *at_buf, char **rsp_cmd)
{
    (void) at_buf;

    cis_module_init();
    if(g_req_type==AT_CMD_QUERY)
    {
        cis_version_t ver;
        *rsp_cmd = xy_malloc(32);
        cis_version(&ver);
        snprintf(*rsp_cmd, 32, "%x.%x.%x", ver.major, ver.minor, ver.micro);
    }
    else
    {
        return ATERR_PARAM_INVALID;
    }

    return  AT_END;
}

/*
 * AT+MIPLDISCOVERRSP=<ref>,<msgid>,<result>,<length>,<valuestring>[,<raimode>]
 */
int at_proc_discover_req(char *at_buf, char **rsp_cmd)
{
    cis_module_init();
	if(g_req_type==AT_CMD_REQ)
	{
	    struct onenet_discover_rsp param = { 0 };
	    int raiType = 0;
	    int ret= CIS_RET_ERROR;

		if(!xy_tcpip_is_ok()) {
			return ATERR_NOT_NET_CONNECT;
		}
		
	    if (at_parse_param("%d(0-0),%d(0-),%d(),%l(0-1024),%p(),%d[0|0x200|0x400]", at_buf, &param.ref, &param.msgId, &param.result, &param.length, &param.value, &raiType) != AT_OK) {
	        return ATERR_PARAM_INVALID;
		}

		if(onet_set_rai(raiType,&param.raiflag) != 0)
		    return ATERR_PARAM_INVALID;
		
		if (!is_onenet_task_running(param.ref)) {
		    return ATERR_NOT_ALLOWED;
		}
		
		if (0 != check_coap_result_code(param.result, RSP_DISCOVER))
		    return ATERR_PARAM_INVALID;

		if(onet_check_reqType(CALLBACK_TYPE_DISCOVER, param.msgId) != 0)
			return ATERR_PARAM_INVALID;
		
	    if(param.result == 1 && param.length > 0) {
	    	char *buft = param.value;
			char *lastp = NULL;
			char *strres = NULL;
	        while ((strres = strtok_r(buft, ";", &lastp)) != NULL) {
				cis_uri_t uri = {0};
				uri.objectId = URI_INVALID;
	            uri.instanceId = URI_INVALID;
	            uri.resourceId = (int)strtol(strres,NULL,10);
	            cis_uri_update(&uri);
	            osMutexAcquire(g_onenet_mutex, osWaitForever);
	            cis_response(onenet_context_refs[param.ref].onenet_context, &uri, NULL, param.msgId, CIS_RESPONSE_CONTINUE,param.raiflag);
	            osMutexRelease(g_onenet_mutex);
				buft = NULL;
			}
	    }

	    osMutexAcquire(g_onenet_mutex, osWaitForever);
	    ret = cis_response(onenet_context_refs[param.ref].onenet_context, NULL, NULL, param.msgId, get_coap_result_code(param.result),param.raiflag);
	    osMutexRelease(g_onenet_mutex);
	    if (ret != CIS_RET_OK){
	        return ATERR_PARAM_INVALID;
	    }
	}
#if (AT_CUT!=1)
	else if(g_req_type == AT_CMD_TEST)
	{
		*rsp_cmd = xy_malloc(100);
		snprintf(*rsp_cmd, 100, "<ref>,<msgID>,<result>[,<length>,<value_string>[,<raimode>]]");
	}
#endif
	else
	{
	    return ATERR_PARAM_INVALID;
	}
	return  AT_END;
}


/*
 * AT+MIPLPARAMETERRSP=<ref>,<msgid>,<result>[,<raimode>]
 */
int at_proc_setparam_req(char *at_buf, char **rsp_cmd)
{
    cis_module_init();
    if(g_req_type==AT_CMD_REQ)
    {
        struct onenet_parameter_rsp param = { 0 };
        int raiType = 0;
        int ret= CIS_RET_ERROR;

        if(!xy_tcpip_is_ok()) {
            return ATERR_NOT_NET_CONNECT;
        }

        if (at_parse_param("%d(0-0),%d(0-),%d(),%d[0|0x200|0x400]", at_buf, &param.ref, &param.msg_id, &param.result, &raiType) != AT_OK) {
            return ATERR_PARAM_INVALID;
        }

        if(onet_set_rai(raiType,&param.raiflag) != 0)
            return ATERR_PARAM_INVALID;

        if (!is_onenet_task_running(param.ref)) {
            return ATERR_NOT_ALLOWED;
        }

        if (0 != check_coap_result_code(param.result, RSP_SETPARAMS))
            return ATERR_PARAM_INVALID;

        if(onet_check_reqType(CALLBACK_TYPE_OBSERVE_PARAMS, param.msg_id) != 0)
            return ATERR_PARAM_INVALID;

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = cis_response(onenet_context_refs[param.ref].onenet_context, NULL, NULL, param.msg_id, get_coap_result_code(param.result),param.raiflag);
        osMutexRelease(g_onenet_mutex);
        if (ret != CIS_RET_OK) {
            return ATERR_PARAM_INVALID;
        }
    }
#if (AT_CUT!=1)
    else if(g_req_type == AT_CMD_TEST)
    {
        *rsp_cmd = xy_malloc(80);
        snprintf(*rsp_cmd, 80, "<ref>,<msgID>,<result>[,<raimode>]");
    }
#endif
    else
    {
        return ATERR_PARAM_INVALID;
    }
    return  AT_END;
}

/*
 * AT+MIPLUPDATE=<ref>,<lifetime>,<withObjectFlag>[,<raimode>]
 */
int at_proc_update_req(char *at_buf, char **rsp_cmd)
{
    cis_module_init();
    if(g_req_type==AT_CMD_REQ)
    {
        struct onenet_update param = { 0 };
        int raiType = 0;
        int ret= CIS_RET_ERROR;

        if(!xy_tcpip_is_ok()) {
            return ATERR_NOT_NET_CONNECT;
        }

        onenet_resume_session();

        if (at_parse_param("%d(0-0),%d(),%d(0-1),%d[0|0x400]", at_buf, &param.ref, &param.lifetime, &param.withObjFlag, &raiType) != AT_OK) {
            return ATERR_PARAM_INVALID;
        }

        if(onet_set_rai(raiType,&param.raiflag) != 0)
            return ATERR_PARAM_INVALID;

        if (!is_onenet_task_running(param.ref)) {
            return ATERR_NOT_ALLOWED;
        }

        if (param.lifetime == 0) {
            param.lifetime = DEFAULT_LIFETIME;
        } else if ( (uint32_t)param.lifetime < LIFETIME_LIMIT_MIN || (uint32_t) param.lifetime > LIFETIME_LIMIT_MAX) {
            return ATERR_PARAM_INVALID;
        }

        osMutexAcquire(g_onenet_mutex, osWaitForever);
        ret = cis_update_reg(onenet_context_refs[param.ref].onenet_context, param.lifetime, param.withObjFlag ,param.raiflag);
        osMutexRelease(g_onenet_mutex);
        if (ret != COAP_NO_ERROR) {
            return ATERR_NOT_ALLOWED;
        }

    }
#if (AT_CUT!=1)
    else if(g_req_type == AT_CMD_TEST)
    {
        *rsp_cmd = xy_malloc(80);
        snprintf(*rsp_cmd, 80, "<ref>,<lifetime>,<with_object_flag>[,<raimode>]");
    }
#endif
    else
    {
        return ATERR_PARAM_INVALID;
    }

    return AT_END;
}

/*
 * AT+ONETRMLFT
 */
int at_proc_rmlft_req(char *at_buf, char **prsp_cmd)
{
    int app_type = 0;
    int remain_lifetime = -1;

    (void) at_buf;

    cis_module_init();
    if(g_req_type != AT_CMD_ACTIVE)
    {
        return ATERR_PARAM_INVALID;
    }

    if (onenet_context_refs[0].onenet_context != NULL && onenet_context_refs[0].onenet_context->stateStep != PUMP_STATE_READY)
        return ATERR_NOT_ALLOWED;

    if (onenet_context_refs[0].onet_at_thread_id == NULL)
    {
        if(g_onenet_session_info->life_time == 0 || g_onenet_session_info->last_update_time == 0)
        {
            return ATERR_NOT_ALLOWED;
        }

        remain_lifetime = g_onenet_session_info->last_update_time + g_onenet_session_info->life_time - cloud_gettime_s();
        if(remain_lifetime <= 0)
        {
            //唤醒后未恢复先查询，若lifetime已超期，不影响随后直接创建通讯套件
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
        return ATERR_NOT_ALLOWED;


    if (remain_lifetime > 0)
    {
        *prsp_cmd = xy_malloc(40);
        sprintf(*prsp_cmd,"%d", remain_lifetime);
    }
    else
    {
        return ATERR_NOT_ALLOWED;
    }

    return AT_END;

}
