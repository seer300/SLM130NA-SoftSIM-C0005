
/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/

#if TELECOM_VER

#include "cdp_backup.h"
#include "flash_adapt.h"
#include "xy_flash.h"
#include "atiny_context.h"
#include "softap_nv.h"
#include "oss_nv.h"
#include "xy_system.h"
#include "low_power.h"
#include "net_app_resume.h"
#include "cloud_utils.h"
#include "atiny_context.h"
#include "net_app_resume.h"
#include "main_proxy.h"
#include "agent_tiny_demo.h"
#include "xy_rtc_api.h"
#include "xy_net_api.h"
#include "atiny_socket.h"
#include "xy_fs.h"
#include "ota_flag.h"
#if WITH_MBEDTLS_SUPPORT
#include "mbedtls/ssl.h"
#endif

//待写入文件系统的数据总数 包括缓存总数和统计信息
#define CDP_TOTALBUFF_MAX_LEN  CDP_DOWNSTREAM_MAX_LEN + sizeof(cdp_session_info_t) + 11 * sizeof(int) 

extern osSemaphoreId_t cdp_recovery_sem;
extern osMutexId_t g_upstream_mutex;
extern downstream_info_t *downstream_info;
extern osMutexId_t g_downstream_mutex;

unsigned int  g_cdp_keepalive_update = 0;
osThreadId_t g_cdp_timeout_restart_TskHandle = NULL;
osThreadId_t g_cdp_resume_TskHandle = NULL;
osThreadId_t g_cdp_rtc_resume_TskHandle = NULL;
osSemaphoreId_t cdp_task_delete_sem;

osMutexId_t g_cdp_resume_mutex = NULL;

extern osMutexId_t g_cdp_module_init_mutex;


extern int g_send_status;

cdp_session_info_t *g_cdp_session_info = NULL;

extern lwm2m_observed_t *prv_findObserved(lwm2m_context_t *contextP,
        lwm2m_uri_t *uriP);

extern void send_debug_by_at_uart(char *buf);

//cdp 资源初始化锁, 防止重入
void cdp_module_mutex_init()
{
	if(g_cdp_module_init_mutex == NULL)
		g_cdp_module_init_mutex = osMutexNew(NULL);
	return;
}

int cdp_bak_srvip_port(int fd, ip_addr_t *remote_ip, uint16_t port)
{
	//保存远端ip地址和绑定本地port
	struct sockaddr_in6* addr = NULL;
	int ret = -1;
	
	if(g_cdp_session_info == NULL)
	{
		return ret;
	}

	addr = xy_malloc(sizeof(struct sockaddr_in6));
	memset(addr, 0x00, sizeof(struct sockaddr_in6));
	addr->sin6_family = (remote_ip->type == IPADDR_TYPE_V6)?AF_INET6:AF_INET;

	//ctwing正常注册时才绑定port
	if(g_cdp_session_info->net_info.local_port != 0 && g_cdp_config_data->cloud_server_port == port)
	{
		addr->sin6_port = lwip_htons(g_cdp_session_info->net_info.local_port);
		ret = bind(fd, (struct sockaddr*)addr, sizeof(struct sockaddr));
		if (ret < 0)
		{
			xy_free(addr);
			return ret;
		}
	}
	
    if(remote_ip->type == IPADDR_TYPE_V6)
    {
		memcpy(&(((struct sockaddr_in6 *)addr)->sin6_addr), &remote_ip->u_addr, sizeof(remote_ip->u_addr));
    }
    else
    {
		((struct sockaddr_in *)addr)->sin_addr.s_addr = remote_ip->u_addr.ip4.addr;
    }
	addr->sin6_port = lwip_htons(port);
	ret = connect(fd, (struct sockaddr*)addr, sizeof(struct sockaddr_in6));
	
	memcpy(&g_cdp_session_info->net_info.remote_ip, remote_ip, sizeof(ip_addr_t));

	xy_free(addr);
	return ret;
}


//该接口内部禁止调用xy_printf，可调用send_debug_by_at_uart
void cdp_bak_svrInfo(xy_lwm2m_server_t * server, cdp_session_info_t *cdp_session_info)
{
	unsigned int num = strlen(server->location) + 1;
	int fd = -1;
#if WITH_MBEDTLS_SUPPORT
	if(g_cdp_config_data->cdp_dtls_switch == 1)
	{
		mbedtls_ssl_context *ssl = (mbedtls_ssl_context *)(g_phandle->client_data.connList->net_context);
		fd = *(int *)ssl->p_bio;
	}
	else
#endif
		fd  = *(int *)(g_phandle->client_data.connList->net_context);
	
    cdp_session_info->net_info.remote_port = g_cdp_config_data->cloud_server_port;

    //目前支持V4/V6
	if(0 == xy_socket_local_info(fd, NULL, &cdp_session_info->net_info.local_port))
		xy_printf(0,XYAPP, WARN_LOG, "xy_socket_local_info failed");
	
	xy_get_ipaddr(cdp_session_info->net_info.remote_ip.type, &cdp_session_info->net_info.local_ip);
	
	cdp_session_info->regtime = server->registration;
	cdp_session_info->lifetime = server->lifetime;

	num = num < (64-1) ? num : (64-1);
	cdp_session_info->location_len = num;
	memset(cdp_session_info->server_location, 0, num);
	memcpy(cdp_session_info->server_location, server->location, num);
}


//状态机恢复至READY同时将NV注册信息跟新至链表
int cdp_resume_session_info(lwm2m_context_t  * contextP)
{
	//xy_printf(0,XYAPP, WARN_LOG, "[CDP]resume cdp reginfo~");

	lwm2m_observed_t *observedP;
	cdp_lwm2m_observed_t *backup_observed;
	bool allocatedObserver = false;
	lwm2m_watcher_t *watcherP;
	cdp_lwm2m_watcher_t *backup_watcherP;
	xy_lwm2m_server_t *targetP, *tmptargetP;

	int i, j;

	contextP->state = STATE_READY;
	contextP->bsCtrl.state = STATE_INITIAL;
	contextP->nextMID = lwm2m_rand();


	tmptargetP = targetP = contextP->serverList;
	while (targetP != NULL)
	{
		targetP->sessionH = lwm2m_connect_server(targetP->secObjInstID, contextP->userData, false);	
        if(NULL == targetP->sessionH ){
            xy_printf(0,XYAPP, WARN_LOG, "[CDP]resume cdp reginfo: sessionH error");
            return -1;
        }
		targetP->status = XY_STATE_REGISTERED;
		targetP->registration = g_cdp_session_info->regtime;
		targetP->lifetime = g_cdp_session_info->lifetime;
		targetP->location = lwm2m_malloc(g_cdp_session_info->location_len+1);
		memset(targetP->location, 0, g_cdp_session_info->location_len+1);
		memcpy(targetP->location, g_cdp_session_info->server_location, g_cdp_session_info->location_len);
		targetP = targetP->next;
	}

	for (i = 0; i <g_cdp_session_info->observed_count; i++)
	{
		backup_observed = &g_cdp_session_info->observed[i];
		observedP = prv_findObserved(contextP, &backup_observed->uri);
		if (observedP == NULL)
		{
			observedP = (lwm2m_observed_t *)lwm2m_malloc(sizeof(lwm2m_observed_t));
			if (observedP == NULL) 
				return -1;
			allocatedObserver = true;
			memset(observedP, 0, sizeof(lwm2m_observed_t));
			memcpy(&(observedP->uri), &backup_observed->uri, sizeof(lwm2m_uri_t));
			cloud_mutex_lock(&contextP->observe_mutex,MUTEX_LOCK_INFINITY);
			observedP->next = contextP->observedList;
			contextP->observedList = observedP;
			cloud_mutex_unlock(&contextP->observe_mutex);
		}

		for(j = 0; j<backup_observed->wather_count; j++)
		{
			backup_watcherP = &backup_observed->watcherList[j];
			watcherP = observedP->watcherList;				
			if (watcherP == NULL)
			{
			   watcherP = (lwm2m_watcher_t *)lwm2m_malloc(sizeof(lwm2m_watcher_t));
			   if (watcherP == NULL)
			   {
				   if (allocatedObserver == true)
				   {
					   lwm2m_free(observedP);
				   }
				   return -1;
			   }
			   memset(watcherP, 0, sizeof(lwm2m_watcher_t));
			   watcherP->active = backup_watcherP->active;
			   watcherP->update = backup_watcherP->update;
			   watcherP->server = tmptargetP;
			   watcherP->format = backup_watcherP->format;
			   watcherP->tokenLen = backup_watcherP->tokenLen;
			   memcpy(watcherP->token, backup_watcherP->token, backup_watcherP->tokenLen);
			   watcherP->lastTime = backup_watcherP->lastTime;
			   watcherP->lastMid = backup_watcherP->lastMid;
			   watcherP->counter = backup_watcherP->counter;
			   memcpy(&watcherP->lastValue.asInteger, &backup_watcherP->lastValue.asInteger, sizeof(watcherP->lastValue));

			   cloud_mutex_lock(&contextP->observe_mutex,MUTEX_LOCK_INFINITY);
			   watcherP->next = observedP->watcherList;
			   observedP->watcherList = watcherP;
			   cloud_mutex_unlock(&contextP->observe_mutex);
			}
		}
	}

#ifdef CONFIG_FEATURE_FOTA
	cdp_fota_info_resume();
#endif

	return 0;
}

//该接口内部禁止调用xy_printf，可调用send_debug_by_at_uart
int cdp_bak_obsvInfo(lwm2m_context_t* context, cdp_session_info_t * cdp_session_info)
{
	//xy_printf(0,XYAPP, WARN_LOG, "[CDP]backup observe info~");

	lwm2m_observed_t *observed;
	cdp_lwm2m_observed_t *tempObserved;
	lwm2m_watcher_t *watcherP;
	cdp_lwm2m_watcher_t *tempWatcher;

	cdp_session_info->observed_count = 0;
	memset(cdp_session_info->observed, 0, sizeof(cdp_lwm2m_observed_t) * CDP_BACKUP_OBSERVE_MAX);

	observed = context->observedList;
	while (observed != NULL)
	{
		//若订阅个数超过4个则丢弃，防止溢出
		if(cdp_session_info->observed_count >= CDP_BACKUP_OBSERVE_MAX)
			break;
		
		tempObserved = &(cdp_session_info->observed[cdp_session_info->observed_count]);
		memcpy(&tempObserved->uri, &observed->uri, sizeof(lwm2m_uri_t));
		cdp_session_info->observed_count++;
		
		tempObserved->wather_count = 0;
		memset(tempObserved->watcherList, 0, sizeof(cdp_lwm2m_watcher_t) * CDP_BACKUP_OBSERVE_MAX);

		watcherP = observed->watcherList;
		while(watcherP != NULL)
		{
			tempWatcher = &(tempObserved->watcherList[tempObserved->wather_count]);
			tempWatcher->active = watcherP->active;
			tempWatcher->counter = watcherP->counter;
			tempWatcher->lastMid = watcherP->lastMid;
			tempWatcher->lastTime = watcherP->lastTime;
			memcpy(tempWatcher->token, watcherP->token, 8);
			tempWatcher->tokenLen = watcherP->tokenLen;
			tempWatcher->update = watcherP->update;
			tempWatcher->counter = watcherP->counter;
			memcpy(&tempWatcher->lastValue.asInteger, &watcherP->lastValue.asInteger, sizeof(watcherP->lastValue));
			tempObserved->wather_count++;
			watcherP = watcherP->next;
		}

		observed = observed->next;
	}

	return 0;
}

void cdp_bak_session_info(lwm2m_context_t* contextP)
{
    if (contextP == NULL || contextP->state != STATE_READY)
        return;

	memset(g_cdp_session_info->endpointname, 0x00, sizeof(g_cdp_session_info->endpointname));
    memcpy(g_cdp_session_info->endpointname, contextP->endpointName, strlen(contextP->endpointName));
	
	//observe list 信息保存至g_cdp_regInfo节点中
	cdp_bak_obsvInfo(contextP, g_cdp_session_info);
	cdp_bak_svrInfo(contextP->serverList, g_cdp_session_info);
	return;
}


//cdp线程恢复初始化化锁，恢复时使用
void cdp_resume_mutex_init()
{
    if(g_cdp_resume_mutex == NULL)
    {
        g_cdp_resume_mutex = osMutexNew(NULL);
    }
}

//主业务线程恢复子接口，配合cdp_module_init使用
int cdp_resume_app()
{
    int check_result = XY_OK;
    osMutexAcquire(g_cdp_resume_mutex, osWaitForever);

    if(!CDP_RECOVERY_FLAG_EXIST())
    {
        osMutexRelease(g_cdp_resume_mutex);
        return RESUME_FLAG_ERROR;
    }

    if(CDP_NEED_RECOVERY(g_cdp_session_info->cdp_lwm2m_event_status))
    {
        check_result = cdp_resume_task();
        osMutexRelease(g_cdp_resume_mutex);
        return check_result;
    }   

    osMutexRelease(g_cdp_resume_mutex);
    return RESUME_OTHER_ERROR;
}


int cdp_is_ip_changed()
{
    ip_addr_t pre_local_ip = {0};
    ip_addr_t new_local_ip = {0};

	cdp_session_info_t * cdp_session_info = xy_malloc(sizeof(cdp_session_info_t));
	if(cloud_read_file(CDP_SESSION_NVM_FILE_NAME,(void*)cdp_session_info,sizeof(cdp_session_info_t))== XY_ERR)
	{
		xy_free(cdp_session_info);
		return IP_RECEIVE_ERROR;
	}

    ip_addr_copy(pre_local_ip,cdp_session_info->net_info.local_ip);
	xy_free(cdp_session_info);

    if(pre_local_ip.type !=IPADDR_TYPE_V4 && pre_local_ip.type !=IPADDR_TYPE_V6)
        return IP_RECEIVE_ERROR;

    if(xy_get_ipaddr(pre_local_ip.type,&new_local_ip) == 0)
        return IP_RECEIVE_ERROR;

    if(ip_addr_cmp(&pre_local_ip,&new_local_ip))
        return IP_NO_CHANGED;
    else
        return IP_IS_CHANGED;
}

//cdp 通用恢复接口，使用场景：1 drx/edrx下行恢复 2 rtc update
void cdp_resume()
{
	if(!CDP_RECOVERY_FLAG_EXIST())
		return;

	cdp_module_init();
    cdp_resume_app();
}


//cdp 主业务恢复子接口
int cdp_resume_task()
{
	int current_sec = cloud_gettime_s();

	if(!xy_tcpip_is_ok())
    {
        xy_printf(0,XYAPP, WARN_LOG, "[cdp] resume fail,tcpip is not ok.");
        return RESUME_OTHER_ERROR;
    }

	if(g_cdp_session_info->regtime == 0)
 	{
 		xy_printf(0, XYAPP, WARN_LOG, "cdp recover failed, regtime is 0\r\n");
    	return RESUME_OTHER_ERROR;
 	}

	//若QCFG设置不为0，而会话保存为0，则认为lifetime保存异常
	if(g_cdp_config_data->cdp_lifetime != 0 && g_cdp_session_info->lifetime == 0)
	{
		xy_printf(0, XYAPP, WARN_LOG, "[CDP]error: cdp session error");
		return RESUME_OTHER_ERROR;
	}

	//OC平台允许lifietiem为0接入平台，接入时lifetime参数不携带，此处lifetime为0需过滤检查
    if((g_cdp_session_info->lifetime != 0  && (g_cdp_session_info->regtime + g_cdp_session_info->lifetime <= current_sec))
        || current_sec <= g_cdp_session_info->regtime)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[CDP]error:cdp expired, pre_update_time:%d lifetime:%d curtime:%d\n",
            g_cdp_session_info->regtime, g_cdp_session_info->lifetime, current_sec);
        return RESUME_LIFETIME_TIMEOUT;
    }
	
	if(cdp_recovery_sem == NULL)
        cdp_recovery_sem = osSemaphoreNew(0xFFFF, 0, NULL);

	if(cdp_recovery_sem != NULL)
		while(osSemaphoreAcquire(cdp_recovery_sem, 0) == osOK){};//防止其他线程走恢复流程提前释放恢复信号量，确保此信号量与本次恢复配对使用

	//不在运行且创建句柄为空才创建，可能存在两个不同的恢复场景线程同时创建，此处只要创建成功了就返回OK
	if (!cdp_create_lwm2m_task(g_cdp_session_info->lifetime))
	{
		osSemaphoreAcquire(cdp_recovery_sem, osWaitForever);
		xy_printf(0,XYAPP, WARN_LOG, "[CDP]recovery cdp wait SemaphoreAcquire");
		return XY_OK;
	}

   return XY_ERR;
}


void cdp_notice_update_process(void)
{   
    send_debug_by_at_uart("+DBGINFO:[CDP] notice proxy update\r\n");

	if(is_cdp_running())
    {
        return;
    }

    
    if (g_cdp_rtc_resume_TskHandle == NULL)
	{
		osThreadAttr_t thread_attr = {0};
		thread_attr.name = "cdp_keelive_update";
		thread_attr.priority = osPriorityNormal1;
		thread_attr.stack_size = osStackShared;	//0x400->osStackShared
		g_cdp_rtc_resume_TskHandle = osThreadNew((osThreadFunc_t)(cdp_rtc_resume_update_process), NULL, &thread_attr);
	}
   return;
}

void cdp_attach_resume_process()
{
    xy_printf(0,XYAPP, WARN_LOG, "\r\n dynamic start cdp\r\n");
    send_debug_by_at_uart("+DBGINFO:[CDP] attach_resume\r\n");
	cdp_module_init();
    if(strcmp((const char *)get_cdp_server_ip_addr_str(), "") == 0)
    {
        xy_printf(0,XYAPP, WARN_LOG,"cdp server addr is empty!");
    }
    else
	{
		app_delay_lock(1000);
		if (!cdp_handle_exist())
		{
			cdp_resume_app();
		}
		
		if(!is_cdp_running())
		{
			//自注册时，生命周期赋初始值；
			cdp_create_lwm2m_task(-1);
		}
	}
	g_cdp_resume_TskHandle = NULL;
	osThreadExit();
}

//本地销毁cdp业务，不用发去注册包；
extern int cdp_deregister_flag;
extern osSemaphoreId_t cdp_wait_sem;

int cdp_delete_task()
{
	if(is_cdp_running())
	{
		cdp_deregister_flag = 2;
		//此处置XY_STATE_DEREGISTERED状态是为了防止DTLS模式下走dtls握手，导致线程无法退出
		g_phandle->lwm2m_context->serverList->status = XY_STATE_DEREGISTERED;
		
		//置为去注册状态，下次注册走重新注册
		g_cdp_session_info->cdp_lwm2m_event_status = 1;
		cloud_save_file(CDP_SESSION_NVM_FILE_NAME, (void *)g_cdp_session_info, sizeof(cdp_session_info_t));
		
		if(cdp_wait_sem != NULL)
			osSemaphoreRelease(cdp_wait_sem);

		if(cdp_task_delete_sem == NULL)
			cdp_task_delete_sem = osSemaphoreNew(0xFFFF, 0, NULL);

	    osSemaphoreAcquire(cdp_task_delete_sem, 10*1000);

		//若等待信号量超时，且cdp还没退出，则断言
		if(is_cdp_running())
		{
			xy_assert(0);
		}

        osSemaphoreDelete(cdp_task_delete_sem);
        cdp_task_delete_sem = NULL;

	}
	//深睡唤醒之前cdp注册状态为已注册状态，或者/19/0/0取消订阅状态
	else if(CDP_RECOVERY_FLAG_EXIST() && CDP_NEED_RECOVERY(g_cdp_session_info->cdp_lwm2m_event_status) || g_cdp_session_info->cdp_lwm2m_event_status == 11)
	{
		//置为去注册状态，下次注册走重新注册
		g_cdp_session_info->cdp_lwm2m_event_status = 1;
	}
	else
	{
		return XY_ERR;
	}
	return XY_OK;
}

int cdp_restart_task()
{
	int uwRet = XY_OK;
	if(g_cdp_resume_TskHandle == NULL)
	{
		osThreadAttr_t thread_attr = {0};
        thread_attr.name = "cdp_resume";
        thread_attr.priority = osPriorityNormal1;
        thread_attr.stack_size = osStackShared;
        g_cdp_resume_TskHandle = osThreadNew((osThreadFunc_t)(cdp_netif_up_resume_process), (void*)IP_NO_CHANGED, &thread_attr);
		return uwRet;
	}
	return XY_ERR;
}

void cdp_netif_up_resume_process(void *param)
{
	int ip_need_change = (int)param;
	//ip_need_change :IP_IS_CHANGED表示需要判断IP地址是否发生变化；IP_NO_CHANGED表示不需要判断IP地址是否发生变化
    if(ip_need_change == IP_IS_CHANGED && cdp_is_ip_changed() == IP_NO_CHANGED)
	{
		xy_printf(0, XYAPP, WARN_LOG,"[cdp_netif_up_resume_process] check ip nochanged,exit");
		g_cdp_resume_TskHandle= NULL;
    	osThreadExit();
	}
	
	cdp_module_init();
#if VER_BC95
	if(cdp_delete_task() == XY_ERR)
	{
		xy_printf(0, XYAPP, WARN_LOG,"[cdp_netif_up_resume_process] delete cdp task failed!,exit");
		g_cdp_resume_TskHandle= NULL;
    	osThreadExit();
	}
	
	app_delay_lock(1000);
	if(cdp_create_lwm2m_task(g_cdp_config_data->cdp_lifetime))
	{
		xy_printf(0, XYAPP, WARN_LOG,"[cdp_netif_up_resume_process] create cdp task failed!,exit");
		g_cdp_resume_TskHandle= NULL;
    	osThreadExit();
	}
#else
    app_delay_lock(1000);
    if (!cdp_handle_exist())
    {
		cdp_resume_app();
    }
	
    if (is_cdp_running())
    {
        osMutexAcquire(g_upstream_mutex, portMAX_DELAY);
        cdp_update_proc(g_phandle->lwm2m_context->serverList);
        osMutexRelease(g_upstream_mutex);
    }
	
#endif
    
    g_cdp_resume_TskHandle= NULL;
    osThreadExit();
}

void cdp_rtc_resume_update_process()
{
    app_delay_lock(1000);
    cdp_resume();
    g_cdp_rtc_resume_TskHandle= NULL;
    osThreadExit();
}

void cdp_resume_timer_create(void)
{
    int lifetime = 0;
    if(g_phandle == NULL)
        return;

    lifetime = g_phandle->lwm2m_context->serverList->lifetime;
	
   	if(lifetime <= 0)
	{
		xy_printf(0, XYAPP, WARN_LOG,"[cdp_resume_timer_create] lifetime is 0\r\n");
		return;
	}
	
    xy_rtc_timer_create(RTC_TIMER_CDP,(int)CLOUD_LIFETIME_DELTA(lifetime),cdp_notice_update_process,(uint8_t)0);

    return;
}

void cdp_resume_timer_delete(void)
{
    xy_rtc_timer_delete(RTC_TIMER_CDP);
    return;
}

void cdp_resume_state_process(int code)
{
   switch (code)
   {
    case XY_STATE_REGISTERED:
        cdp_resume_timer_create();
        break;
    case XY_STATE_REG_FAILED:
        cdp_resume_timer_delete();//update失败，删除rtc_timer
        break;
    case XY_STATE_UPDATE_DONE:
        cdp_resume_timer_create();
        break;
    case XY_STATE_DEREGISTERED:
        cdp_resume_timer_delete();
		break;
    default:
        break;
    }
}

#ifdef CONFIG_FEATURE_FOTA
void cdp_fota_info_resume()
{
	//恢复的时候已经读过文件系统，此处不需要再读
	if(g_cdp_fota_info != NULL && (strlen(g_cdp_fota_info->uri) <= 255 && strlen(g_cdp_fota_info->uri) > 0))
	{
		atiny_fota_manager_set_pkg_uri(atiny_fota_manager_get_instance(), g_cdp_fota_info->uri, strlen(g_cdp_fota_info->uri));
		atiny_fota_manager_set_state_2(atiny_fota_manager_get_instance(), g_cdp_fota_info->state);
	}
}

void cdp_ota_state_hook(int state)
{
	/*设备FOTA升级状态的HOOK函数,用户可根据FOTA的升级状态,进行相关处理*/
    switch(state)
    {
    case XY_FOTA_DOWNLOADING:
        /*minitor user program abnormal running,propose to set user watchdog timer*/
        break;
    case XY_FOTA_DOWNLOADED:
        break;
    case XY_FOTA_UPGRADING:
        break;
    case XY_FOTA_UPGRADE_SUCCESS:
        break;
    case XY_FOTA_UPGRADE_FAIL:
        break;
    default:
        break;
    }
    xy_printf(0, PLATFORM, WARN_LOG, "[cdp_ota_state_hook]state:%d", state);
    return;
}
#endif

#endif
