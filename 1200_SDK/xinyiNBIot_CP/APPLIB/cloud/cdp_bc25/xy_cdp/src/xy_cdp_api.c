
#if TELECOM_VER
#include "xy_utils.h"
#include "cloud_utils.h"
#include "xy_at_api.h"
#include "cdp_backup.h"
#include "cc.h"
#include "lwip/inet.h"
#include "xy_cdp_api.h"
#include "agenttiny.h"
#include "atiny_context.h"
#include "agent_tiny_demo.h"
#include "softap_nv.h"
#include "oss_nv.h"
#include "net_app_resume.h"
#include "xy_net_api.h"
#include "ps_netif_api.h"
#include "xy_system.h"

extern int cdp_register_fail_flag;

volatile int cdp_send_mid = 0;
cdp_send_asyn_ack g_cdp_send_asyn_ack = NULL;
cdp_downstream_callback g_cdp_downstream_callback = NULL;

int cdp_cloud_setting(char *ip_addr_str, int port)
{
	cdp_module_init();

	if((strlen(ip_addr_str) <= 0)||(strlen(ip_addr_str) > 40) || (port < 0) || (port > 65535))
	{
		xy_printf(0,XYAPP, WARN_LOG, "[cdp_cloud_setting]:ip or port error!");
        return XY_ERR;
	}

    if(set_cdp_server_settings(ip_addr_str, (u16_t)port))
    {
        xy_printf(0,XYAPP, WARN_LOG, "[cdp_cloud_setting]:cdp server setting error!");
        return XY_ERR;
    }

    return XY_OK;
}


int cdp_register(int lifetime, int timeout)
{
    //wait PDP active
	if(!xy_tcpip_is_ok())
    {
    	xy_printf(0,XYAPP, WARN_LOG, "[cdp_register]:Network disconnection!");
    	return XY_ERR;
    }


    //if cdp is registered after deepsleep to resume cdp task
    cdp_module_init();
    cdp_resume_app();

    if(!is_cdp_running())
    {
        if(lifetime < 120 || lifetime > LWM2M_DEFAULT_LIFETIME * 30)
        {
            xy_printf(0,XYAPP, WARN_LOG, "[cdp_register]:cdp server lifetime is error!");
            return XY_ERR;
        }
        if(strcmp((const char *)get_cdp_server_ip_addr_str(), "") == 0)
        {
            xy_printf(0,XYAPP, WARN_LOG, "[cdp_register]:cdp server addr is empty!");
            return XY_ERR;
        }
        else if(cdp_create_lwm2m_task(lifetime))
        {
            xy_printf(0,XYAPP, WARN_LOG, "[cdp_register]:registed failed!");
            return XY_ERR;
        }
    }
    else
    {
        xy_printf(0,XYAPP, WARN_LOG, "[cdp_register]:cdp have registed!");
        return XY_OK;
    }    
    
    if(cdp_api_register_sem == NULL)
        cdp_api_register_sem = osSemaphoreNew(0xFFFF, 0, NULL);

    while (osSemaphoreAcquire(cdp_api_register_sem, 0) == osOK) {};
    if (osSemaphoreAcquire(cdp_api_register_sem, timeout * 1000) != osOK)
    {
        cdp_register_fail_flag = 1;
        while(is_cdp_running())
        {
            osDelay(100);
        }

        xy_printf(0,XYAPP, WARN_LOG, "[cdp_register]:cdp register fail, timeout!");
        return XY_ERR;
    }

    if(g_phandle == NULL)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[cdp_register]:cdp register fail!");
        return XY_ERR;
    }
    
    xy_printf(0,XYAPP, WARN_LOG, "[cdp_register]:cdp register success!");
    return XY_OK;
}


int cdp_deregister(int timeout)
{
    //wait PDP active
	int temp_timeout = timeout;
	while (!xy_tcpip_is_ok())
	{
		osDelay(100);
		temp_timeout -= 100;
		if (temp_timeout <= 0)
        {
            xy_printf(0,XYAPP, WARN_LOG, "[cdp_deregister]:Network disconnection!");
            return XY_ERR;
        }
	}

    //if cdp is registered after deepsleep to resume cdp task
    cdp_module_init();
    cdp_resume_app();

    if(cdp_delete_lwm2m_task() == 0)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[cdp_deregister]:cdp deregister failed!");
        return XY_ERR;
    }

    if(cdp_api_deregister_sem == NULL)
        cdp_api_deregister_sem = osSemaphoreNew(0xFFFF, 0, NULL);
    
    while (osSemaphoreAcquire(cdp_api_deregister_sem, 0) == osOK) {};
    osSemaphoreAcquire(cdp_api_deregister_sem, timeout*1000);
  
    xy_printf(0,XYAPP, WARN_LOG, "[cdp_deregister]:cdp deregister success!");
    return XY_OK;
}


int cdp_send_syn(char *data, int len, int msg_type)
{
    int errr_num = -1;

    if(data == NULL)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[cdp_send_syn]:data is NULL!");
        return XY_ERR;
    }
    
    if(len>MAX_REPORT_DATA_LEN || len<=0)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[cdp_send_syn]:data len error!");
        return XY_ERR;
    }
        
    if (!xy_tcpip_is_ok()) 
    {
        xy_printf(0,XYAPP, WARN_LOG, "[cdp_send_syn]:Network disconnection!");
        return XY_ERR;
    }


    cdp_module_init();
    cdp_resume_app();
    
    lwm2m_context_t *context = g_phandle->lwm2m_context;
    if ((is_cdp_running() == 0) 
            || (context->state != STATE_READY))
    {
        xy_printf(0,XYAPP, WARN_LOG, "[cdp_send_syn]:cdp not exist!");
        return XY_ERR;
    }

	if(!is_cdp_upstream_ok())
		return XY_ERR;

    if(get_upstream_message_pending_num())
    {
        xy_printf(0,XYAPP, WARN_LOG, "[cdp_send_syn]:Data is being sent!");
        return XY_ERR;
    }
        
    errr_num = get_upstream_message_error_num();
    if (send_message_via_lwm2m(data, len, msg_type,0))
        return XY_ERR;

    while(get_upstream_message_pending_num())
    {   
        osDelay(100);
    }

	if(get_upstream_message_error_num() - errr_num != 0)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[cdp_send_syn]:Data is send fail!");
		return XY_ERR;
	}

    xy_printf(0,XYAPP, WARN_LOG, "[cdp_send_syn]:Data is send success!"); 
    return XY_OK;
}

int cdp_send_asyn(char *data, int len, int msg_type)
{
    int pending_num = -1;
    int mid = -1;
    
    if(data == NULL)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[cdp_send_asyn]:data is NULL!");
        return XY_ERR;
    }
    
    if(len>MAX_REPORT_DATA_LEN || len<=0)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[cdp_send_asyn]:data len error!");
        return XY_ERR;
    }
        
    if (!xy_tcpip_is_ok()) 
    {
        xy_printf(0,XYAPP, WARN_LOG, "[cdp_send_asyn]:Network disconnection!");
        return XY_ERR;
    }

	
    cdp_module_init();
    cdp_resume_app();
    
    lwm2m_context_t *context = g_phandle->lwm2m_context;
    if ((is_cdp_running() == 0) 
            || (context->state != STATE_READY))
    {
        xy_printf(0,XYAPP, WARN_LOG, "[cdp_send_syn]:cdp not exist!");
        return XY_ERR;
    }

	if(!is_cdp_upstream_ok())
		return XY_ERR;

    pending_num = get_upstream_message_pending_num();
    if(pending_num >= 8)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[cdp_send_asyn]:The data link list is full!");
        return XY_ERR;
    }

    cdp_send_mid = -1;
    if (send_message_via_lwm2m(data, len, msg_type,0))
        return XY_ERR;

    if(NULL != cdp_api_sendasyn_sem)
    {
        osSemaphoreAcquire(cdp_api_sendasyn_sem, 10*1000);
        mid = cdp_send_mid;
    }
    
    return mid;
}

int cdp_send_status_check()
{
    if(g_phandle == NULL)
        return XY_ERR;
    
    if(!is_cdp_running())
        return XY_ERR;

    if((g_phandle->lwm2m_context->observedList == NULL )
        ||(g_phandle->lwm2m_context->state != STATE_READY))
        return XY_ERR;
        
    return XY_OK;
}


int cdp_lifetime_update(int timeout)
{
    int ret = -1;
        
    if (!xy_tcpip_is_ok()) 
    {
        xy_printf(0,XYAPP, WARN_LOG, "[cdp_update]:Network disconnection!");
        return XY_ERR;
    }

	
    cdp_module_init();
    cdp_resume_app();
    
    lwm2m_context_t *context = g_phandle->lwm2m_context;
    if ((is_cdp_running() == 0) 
            || (context->state != STATE_READY))
    {
        xy_printf(0,XYAPP, WARN_LOG, "[cdp_send_syn]:cdp not exist!");
        return XY_ERR;
    }

    xy_lwm2m_server_t *targetP = context->serverList;
    if(targetP == NULL)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[cdp_send_syn]:cdp not ready!");
        return XY_ERR;
    }

    ret = cdp_update_proc(targetP);
    if (ret == -1)
    {
        return XY_ERR;
    }

    if(cdp_api_update_sem == NULL)
        cdp_api_update_sem = osSemaphoreNew(0xFFFF, 0, NULL);

    while (osSemaphoreAcquire(cdp_api_update_sem, 0) == osOK) {};
    if (osSemaphoreAcquire(cdp_api_update_sem, timeout * 1000) != osOK)
    {
        cdp_register_fail_flag = 1;
        while(is_cdp_running())
        {
            osDelay(100);
        }
        
        xy_printf(0,XYAPP, WARN_LOG, "[cdp_update] cdp update fail!");
        return XY_ERR;
    }    
    
    return XY_OK;
}

int cdp_rmlft_get(void)
{
    int app_type = -1;
    int remain_lifetime = -1;

	cdp_module_init();

    if (g_phandle != NULL && g_phandle->lwm2m_context != NULL && g_phandle->lwm2m_context->state != STATE_READY)
        return XY_ERR;

    if (!is_cdp_running())
    {
        if(!CDP_RECOVERY_FLAG_EXIST() ||!CDP_NEED_RECOVERY(g_cdp_session_info->cdp_lwm2m_event_status))
            return XY_ERR;

        remain_lifetime = g_cdp_session_info->regtime + g_cdp_session_info->lifetime - cloud_gettime_s();
    }
    else if ( g_phandle !=NULL && g_phandle->lwm2m_context != NULL && g_phandle->lwm2m_context->serverList != NULL)
    {
        remain_lifetime = g_phandle->lwm2m_context->serverList->registration +
            g_phandle->lwm2m_context->serverList->lifetime - cloud_gettime_s();
    }
    else
        return XY_ERR;
    
    if (remain_lifetime >= 0)
    {
        return remain_lifetime;      
    }
    else
    {
        return XY_ERR;
    }
}


int cdp_callbak_set(cdp_downstream_callback downstream_callback, cdp_send_asyn_ack send_asyn_ack)
{
    g_cdp_send_asyn_ack = send_asyn_ack;
    g_cdp_downstream_callback = downstream_callback;
    return XY_OK;
}


int cdp_set_endpoint_name(char * endpointname)
{
	cdp_module_init();
	if(endpointname == NULL || (endpointname != NULL && strlen(endpointname) > 255))
    {
        xy_printf(0,XYAPP, WARN_LOG, "[cdp_set_endpoint_name]:Parameter error!");
        return XY_ERR;
    }
	
	memset(g_cdp_session_info->endpointname, 0x00, sizeof(g_cdp_session_info->endpointname));
	memcpy(g_cdp_session_info->endpointname, endpointname, strlen(endpointname));
	cloud_save_file(CDP_SESSION_NVM_FILE_NAME,(void*)g_cdp_session_info,sizeof(cdp_session_info_t));
	//设置恢复标志位，表示文件系统可用，下次深睡恢复读文件系统里的值；
	CDP_SET_RECOVERY_FLAG();
	return XY_OK;
}

char *cdp_get_endpoint_name()
{
    cdp_module_init();
	//如果设置了，就用设置里面的值，如果没有设置，则使用默认值
	if(strlen(g_cdp_session_info->endpointname) == 0 && !xy_get_IMEI(g_cdp_session_info->endpointname, 16))
        return NULL;

	return g_cdp_session_info->endpointname;
}
#endif

