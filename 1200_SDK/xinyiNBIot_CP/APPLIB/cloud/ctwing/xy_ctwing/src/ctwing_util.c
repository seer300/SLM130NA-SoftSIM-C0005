#include "ctlw_NV_data.h"
#include "ctlw_lwm2m_sdk.h"
#include "ctlw_fota.h"
#ifdef WITH_FOTA
#include "xy_fota.h"
#endif
#include "ctlw_liblwm2m.h"
#include "ctlw_internals.h"
#include "ctlw_platform.h"
#include "ctlw_abstract_signal.h"
#include "ctlw_heap.h"
#include "ctlw_abstract_os.h"
#include "ctlw_sdk_internals.h"
#include "ctlw_NV_data.h"
#include "xy_ps_api.h"
#include "xy_system.h"
#include "lwip/errno.h"
#include "xy_at_api.h"
#include "ps_netif_api.h"
#include "xy_net_api.h"
#include "ctlw_NV_data.h"
#include "ctwing_util.h"
#include "ctwing_resume.h"
#include "factory_nv.h"
#include "xy_rtc_api.h"
#include "oss_nv.h"
#include "atc_ps_def.h"





thread_handle_t g_ctlw_dns_tsk_handle = NULL;//dns线程句柄

thread_handle_t g_ctlw_auto_reg_tsk_handle = NULL;//自注册线程handle
thread_handle_t g_ctlw_netif_ip_event_tsk_handle = NULL;//netif的IP事件处理线程handle
thread_handle_t g_ctlw_fota_event_task_handle = NULL;//fota上报触发恢复线程handle



uint8_t g_ctlw_dns_req_ip_type = 0;
char g_ctlw_dns_ip[XY_CTLW_DNS_IP_MAX_LEN]={0};
char g_ctlw_host_name[XY_CTLW_HOSTNAME_MAX_LEN]={0};

extern uint16_t ctiotprv_system_para_init(ctiot_context_t *pContext);

int8_t g_ctlw_data_cache_lock_fd = -1;//数据缓存睡眠锁句柄







/**
 * @brief 检查是否有缓存数据未被读取,若有缓存数据,则加锁,不允许芯片进入deepsleep
 * 
 * @param ctlwClientStatus
 * @param pContext 
 */
void xy_ctlw_check_if_sleep_allow(ctiot_client_status_e ctlwClientStatus, ctiot_context_t *pContext)
{
    //业务处于ready状态且缓存中有数据未被读取,缓存中有数据，芯片不可进入deepsleep
    if(ctlwClientStatus == CTIOT_STATE_READY && pContext->downMsgList->head != NULL && g_ctlw_data_cache_lock_fd == -1)
    {
        ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_OTHER_CLASS,"xy_ctlw_check_if_sleep_allow,cache data,lock deepsleep\r\n");
        
        if((g_ctlw_data_cache_lock_fd = create_sleep_lock("ctlw_cache_data_lock")) == -1)//创建睡眠锁失败,系统异常
        {
            xy_assert(0);
        }

        sleep_lock(g_ctlw_data_cache_lock_fd, LPM_DEEPSLEEP);//芯片不可以进入deepsleep
    }//缓存中无数据，若有锁，清除锁
    else if(ctlwClientStatus == CTIOT_STATE_READY && pContext->downMsgList->head == NULL && g_ctlw_data_cache_lock_fd != -1)
    {
        ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_OTHER_CLASS,"xy_ctlw_check_if_sleep_allow,cache data,unlock deepsleep\r\n");
        
        sleep_unlock(g_ctlw_data_cache_lock_fd, LPM_DEEPSLEEP);
        
        delete_sleep_lock(g_ctlw_data_cache_lock_fd);
       
        g_ctlw_data_cache_lock_fd = -1;
    }
}
/**
 * @brief 根据入参值，返回当前notify事件原因
 * 
 * @return @ref xy_ctlw_notify_state_e类型值
 */
xy_ctlw_notify_state_e xy_ctlw_get_notify_state(uint8_t subType, uint16_t value, uint32_t data1)
{
    xy_ctlw_notify_state_e state = XY_CTLW_STATE_MAX;

    switch(subType)
    {
        case CTIOT_NOTIFY_SUBTYPE_LWEVENT:
        {
            if(value == 0 && data1 == 0)
                state = XY_CTLW_STATE_19OBSERVED;//注册后,19/0/0 observed
            break;
        }
        case CTIOT_NOTIFY_SUBTYPE_REG:
        {
            if(value == 0)
                state = XY_CTLW_STATE_REGISTERED;//注册成功
            
            break;
        }
        case CTIOT_NOTIFY_SUBTYPE_UPDATE:
        {
            if(value == CTIOT_NB_SUCCESS)
                state = XY_CTLW_STATE_UPDATE_SUCESSED;//update 成功
            else
                state = XY_CTLW_STATE_UPDATE_FAILED; //update失败

            break;
        }
        case CTIOT_NOTIFY_SUBTYPE_LSTATUS:
        {
            if(value == CTIOT_LSTATUS_QUITSESSION && (data1 == dtlssendfail ||data1 == dtlsrecvdatafail || data1 == hsfail))
                state = XY_CTLW_STATE_RELEASE;//各种原因造成资源释放
            //update失败 或 DTLS 模式下自动update失败
            else if(value == CTIOT_LSTATUS_QUITSESSION && (data1 == atupdate404 || data1 == inupdatedtlssendfail))
                state = XY_CTLW_STATE_UPDATE_FAILED;

            else if(value == CTIOT_LSTATUS_QUITSESSION && data1 == deregothererr) //登出异常
                state = XY_CTLW_STATE_DEREGISTERED;//去注册

            else if(value == CTIOT_LSTATUS_QUITENGINE && data1 == 0)
                state = XY_CTLW_STATE_RELEASE; //各种原因造成资源释放

            else if(value == CTIOT_LSTATUS_QUITSESSION && (data1 == initdatafail || data1 == setmoderr))
                state = XY_CTLW_STATE_RECOVER_FAILED; //恢复失败

            break;
        }
        case CTIOT_NOTIFY_SUBTYPE_DEREG:
        {
            state = XY_CTLW_STATE_DEREGISTERED; //去注册
            break;
        }
        case CTIOT_NOTIFY_SUBTYPE_SEND:
        {
            if(value == QUEUE_SEND_SUCCESS)
            {
                state = XY_CTLW_DATA_SENT_SUCCESS;
            }
            break;
        }
        default:
            break;
    }
    return state;
}



/**
 * @brief 根据SDK的notify主动上报，执行对应的处理
 */
void xy_ctlw_procedure_with_notify(uint8_t notifyType, uint8_t subType, uint16_t value, uint32_t data1)
{
    ctiot_context_t *pContext = xy_ctlw_ctiot_get_context();
    xy_ctlw_notify_state_e state = XY_CTLW_STATE_MAX;

    ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_OTHER_CLASS,"xy_ctlw_procedure_with_notify,subType= %d,notifyType= %d \r\n",subType, notifyType);

    if(notifyType != CTIOT_NOTIFY_ASYNC_NOTICE)
        return ;
    
    state = xy_ctlw_get_notify_state(subType, value, data1);

    ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_OTHER_CLASS,"xy_ctlw_procedure_with_notify:%d \r\n",state);

    switch(state)
    {
        case XY_CTLW_STATE_REGISTERED:
        {
            //ctlw_registrtion.c注册成功回调中,根据用户设置的有无心跳模式,决定是否创建用于update的RTC
            break;
        }
        case XY_CTLW_STATE_UPDATE_SUCESSED:
        {
            // ctchip_start_sleep_timer();SDK time rtc回调中已再次创建RTC
            //! 不在判断lifetime时间超期时间，此处暂不需要更新注册时间
            // ctiot_update_sdataflash_needed(true);//update成功，registration时间更新，需要实时更新flash
            break;
        }
        case XY_CTLW_STATE_UPDATE_FAILED:
        case XY_CTLW_STATE_DEREGISTERED:
        case XY_CTLW_STATE_RECOVER_FAILED:
        case XY_CTLW_STATE_RELEASE:
        {
            //上述几种状态,删除用于主动发起update的RTC
            ctchip_stop_sleep_timer();
            break;
        }

        default:
            break;
    }
}


/**
 * @brief 检查业务是否在运行(处于可收发数据状态)
 */
bool xy_ctlw_check_if_task_running(void)
{
    ctiot_context_t *pContext = xy_ctlw_ctiot_get_context();

    if(pContext->sendAndRecvThreadStatus ==THREAD_STATUS_WORKING
    && pContext->ctlwClientStatus == CTIOT_STATE_READY)
    {
        return true;
    }

    return false;

}

#ifdef WITH_FOTA
/**
 * @brief 根据config文件中保存的fotaflag，判断当前fota升级使用的云是否为CTWING
 */
bool xy_ctlw_check_if_fota_running(void)
{
    bool ret = false;
    NV_params_t *NV_params = xy_malloc(sizeof(NV_params_t));

    if((cloud_read_file(CTLW_CFG_FILE_UNDER_DIR, (void*)NV_params,sizeof(NV_params_t)) == XY_OK) && NV_params->fotaFlag == 1)
    {
        ret = true;
    }

    xy_free(NV_params);

    return ret;
}
#endif




/**
 * @brief 检查业务运行中IP地址是否发生了变化
 */
bool xy_ctlw_check_if_ip_changed(void)
{
    ip_addr_t pre_local_ip = {0};
    ip_addr_t new_local_ip = {0};
    ctiot_context_t *pContext = xy_ctlw_ctiot_get_context();

    ipaddr_aton(pContext->localIP, &pre_local_ip);//获取当前业务正在使用的IP

    if(xy_get_ipaddr(pre_local_ip.type,&new_local_ip) == false)//获取原ip_type对应的ip地址失败
    {
        ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"xy_ctlw_check_if_ip_changed,get ip err,true");
        return true;//ip is changed
    }

    if(ip_addr_cmp(&pre_local_ip,&new_local_ip))
    {
        ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"xy_ctlw_check_if_ip_changed,false");
        return false;//ip no changed
    }
    else
    {
        ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"xy_ctlw_check_if_ip_changed,true");
        return true;//ip is changed
    }
}


/**
 * @brief 设置注册模式 @ref xy_ctlw_reg_mode_e
 * 
 * @param reg_mode (XY_CTLW_MANUAL_REG:手动注册,通过AT命令连接云平台)
 * ,(XY_CTLW_AUTO_REG:自动注册,上电自动连接AEP云平台)
 */
void xy_ctlw_set_reg_mode(xy_ctlw_reg_mode_e reg_mode)
{
    ctiot_context_t *pContext = xy_ctlw_ctiot_get_context();
    
    pContext->regMode = reg_mode;
	
    if(pContext->regMode == XY_CTLW_AUTO_REG)
    {   
        g_softap_fac_nv->telecom_ctl |= (1 << 1);
    }
    else//pContext->regMode == XY_CTLW_MANUAL_REG
    {
        g_softap_fac_nv->telecom_ctl &= ~(1<<1);
    }

    SAVE_FAC_PARAM(telecom_ctl);
}


/**
 * @brief 获取注册模式 @ref xy_ctlw_reg_mode_e
 * 
 * @return xy_ctlw_reg_mode_e 
 */
xy_ctlw_reg_mode_e xy_ctlw_get_reg_mode()
{
    ctiot_context_t *pContext = xy_ctlw_ctiot_get_context();
    
    return pContext->regMode;
}




/**
 * @brief 从factory NV中获取注册模式
 * 
 * @return xy_ctlw_reg_mode_e XY_CTLW_MANUAL_REG:手动注册,XY_CTLW_AUTO_REG:自动注册
 */
xy_ctlw_reg_mode_e xy_ctlw_get_nvm_reg_mode()
{
    if((g_softap_fac_nv->telecom_ctl & (1<<1))!=0)
    {
        return XY_CTLW_AUTO_REG;
    }
    else
    {
        return XY_CTLW_MANUAL_REG;
    }
}



/**
 * @brief 通过dns查询域名对应的IP地址
 * 
 * @param iptype 待查询IP类型 0:V4，1：V6
 * @param dns_ip dns服务器IP 字符串,入参不为NULL,则将dns服务器IP设置到DNS服务器index为0的位置
 * @param host_name hostname 字符串 待查询dns域名
 * @return int32_t 
 */
static int32_t xy_ctlw_set_server_by_dns(int iptype, uint8_t * dns_ip, uint8_t *host_name)
{
    int result = CTIOT_NB_SUCCESS;
    struct addrinfo hints={0},*res =NULL,*rp =NULL;
    char ipbuf[16],ip6buf[64];

    struct sockaddr_in *addr;
	struct sockaddr_in6 *addr6;
    ctiot_context_t *pContext = xy_ctlw_ctiot_get_context();
	
    //设置DNS服务器IP
    if(dns_ip != NULL)
    {
        xy_dns_set((char *)dns_ip,0,false);
    }
    
    if(iptype)
        hints.ai_family = AF_INET6;
    else
        hints.ai_family = AF_INET;

    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    result = getaddrinfo(host_name, NULL, &hints, &res);//DNS查询

	if(result !=0)
    {
		ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_OTHER_CLASS,"ctlw dns query failed = %d", result);
        goto exit;
    }

    for(rp = res;rp != NULL;rp = rp->ai_next)
	{
		if(rp->ai_family == AF_INET)		
		{	
            memset(ipbuf, 0x00, 16);
            addr = (struct sockaddr_in*)rp->ai_addr;   
            inet_ntop(AF_INET, &addr->sin_addr, ipbuf, 16);
            if(strlen(ipbuf) != 0)
            {
                if(pContext->serverIPV4 != NULL)
                {
                    ctlw_lwm2m_free(pContext->serverIPV4);
                    pContext->serverIPV4 = NULL;
                }
                pContext->serverIPV4 = ctlw_lwm2m_strdup(ipbuf);
            }


		}		
		else		
		{	
            memset(ip6buf, 0x00, 64);
            addr6 = (struct sockaddr_in6*)rp->ai_addr;   
            inet_ntop(AF_INET6, &addr6->sin6_addr, ip6buf, 64);

            if(strlen(ip6buf) != 0)
            {
                if(pContext->serverIPV6 != NULL)
                {
                    ctlw_lwm2m_free(pContext->serverIPV6);
                    pContext->serverIPV6 = NULL;
                }
                pContext->serverIPV6 = ctlw_lwm2m_strdup(ip6buf);
            }
		}
	}
	freeaddrinfo(res);

exit:
    
    return result;
}

/**
 * @brief dns处理线程
 */
void xy_ctlw_dns_task(void)
{
    int32_t result;
    uint8_t *rspbuf = xy_malloc(84);
    ctiot_context_t *pContext = xy_ctlw_ctiot_get_context();

    ctiot_set_dns_process_status(1);//置dns正在处理状态位

    if(g_ctlw_dns_req_ip_type == CTLW_DNS_V4ONLY || g_ctlw_dns_req_ip_type == CTLW_DNS_V6ONLY)
    {
        result = xy_ctlw_set_server_by_dns(g_ctlw_dns_req_ip_type, g_ctlw_dns_ip, g_ctlw_host_name);

        if(!result)
        {
            c2f_encode_params(pContext);//dns结果保存至文件系统
            sprintf(rspbuf, "+CTLW: dns,0,0");            
        }
        else
        {
            sprintf(rspbuf, "+CTLW: dns,1,0");
        }

        ctchip_asyn_notify(rspbuf);//urc上报dns结果
    }
    else//g_ctlw_dns_req_ip_type == CTLW_DNS_V4AndV6
    {
        //request V4
        result = xy_ctlw_set_server_by_dns(CTLW_DNS_V4ONLY, g_ctlw_dns_ip, g_ctlw_host_name);

        if(!result)
        {
            c2f_encode_params(pContext);//dns结果保存至文件系统
            sprintf(rspbuf, "+CTLW: dns,0,0");
        }
        else
        {
            sprintf(rspbuf, "+CTLW: dns,1,0");
        }

        ctchip_asyn_notify(rspbuf);//urc上报dns v4结果

        //request V6
        result = xy_ctlw_set_server_by_dns(CTLW_DNS_V6ONLY, g_ctlw_dns_ip, g_ctlw_host_name);

        if(!result)
        {
            c2f_encode_params(pContext);//dns结果保存至文件系统
            sprintf(rspbuf, "+CTLW: dns,0,0");
        }
        else
        {
            sprintf(rspbuf, "+CTLW: dns,1,0");
        }

        ctchip_asyn_notify(rspbuf);//urc上报dns v6结果
    }

    if(rspbuf)
    {
    	xy_free(rspbuf);
    }

    g_ctlw_dns_req_ip_type = 0;//恢复默认值

    g_ctlw_dns_tsk_handle = NULL;

    ctiot_set_dns_process_status(0);//置dns结束状态位

    osThreadExit();
}

void NV_free_cache(void)
{
    extern uint8_t *flashCache;
    extern char *g_ctlw_user_cache;

	if (flashCache)
	{
		xy_free(g_ctlw_user_cache);
		flashCache = NULL;
		g_ctlw_user_cache = NULL;
	}
}
/**
 * @brief 从FS中获取最近一次业务注册状态
 */
void xy_ctlw_get_nvm_bootflag(ctiot_context_t *pContext)
{
    NV_lwm2m_context_t *NV_lwm2m_context = (uint8_t *)xy_malloc(sizeof(NV_lwm2m_context_t));

    if(cloud_read_file(CTLW_SESSION_FILE_UNDER_DIR, (void*)NV_lwm2m_context,sizeof(NV_lwm2m_context_t)) == XY_OK)
    {
        if(NV_lwm2m_context->bootFlag == BOOT_NOT_LOAD || NV_lwm2m_context->bootFlag == BOOT_LOCAL_BOOTUP)
        {
            pContext->bootFlag = NV_lwm2m_context->bootFlag;
        }
    }
    xy_free(NV_lwm2m_context);
}



/**
 * @brief FOTA流程URC上报
 */

void xy_ctlw_fota_notify(int fotaState, int fotaResult)
{
    char *rsp_cmd = xy_malloc(48);
    memset(rsp_cmd, 0x00, 48);

    switch (fotaState)
    {
    case FOTA_STATE_IDIL:
    {
        if((fotaResult >= FOTA_RESULT_NOFREE && fotaResult <= FOTA_RESULT_URIINVALID)
            ||fotaResult == FOTA_RESULT_PROTOCOLFAIL)
        {
            sprintf(rsp_cmd,"+FIRMWARE: DOWNLOAD FAILED");
        }

        else if(fotaResult == FOTA_RESULT_SUCCESS)
        {
            sprintf(rsp_cmd,"+FIRMWARE: UPDATE SUCCESS");
        }

        else if(fotaResult == FOTA_RESULT_OVER)
        {
            sprintf(rsp_cmd,"+FIRMWARE: UPDATE OVER");
        }
        
        break;
    }
    case FOTA_STATE_DOWNLOADING:
    {
        sprintf(rsp_cmd, "+FIRMWARE: DOWNLOADING");
        break;
    }

    case FOTA_STATE_DOWNLOADED:
    {
        sprintf(rsp_cmd, "+FIRMWARE: DOWNLOADED");
        break;
    }

    case FOTA_STATE_UPDATING:
    {
        sprintf(rsp_cmd, "+FIRMWARE: UPDATING");
        break;
    }

		default:
			break;
    }

    if(strlen(rsp_cmd) != 0)
    {
        send_urc_to_ext(rsp_cmd, strlen(rsp_cmd));
    }

    xy_free(rsp_cmd);

    return;
}




/**
 * @brief 获取update query参数
 * 
 * @param server lwm2m_server_t *
 * @param query char*
 */
void xy_ctlw_get_uri_query(lwm2m_server_t *server, char* query)
{
	sprintf(query,"lt=%d",server->lifetime);
	if(server->binding == BINDING_U)// UDP
    {
        strcat(query,"&b=U");
    }
    else if(server->binding == BINDING_UQ)//UDP queue mode
    {
        strcat(query,"&b=UQ");
    }
}

/**
 * @brief Ctwing平台修改本地binding_mode参数
 * 
 * @param binding_mode 
 * @return true  修改成功
 * @return false 修改失败
 */
bool xy_ctlw_set_binding_by_iot_plat(const char * binding_mode, size_t size)
{
    ctiot_context_t *pContext = xy_ctlw_ctiot_get_context();
    lwm2m_context_t *lwm2mContxt = pContext->lwm2mContext;
    lwm2m_server_t *server = lwm2mContxt->serverList;

	if(strncmp(binding_mode, "U", size) == 0)
    {
        //SDK不支持dtls模式+U模式
        if(pContext->connectionType == MODE_DTLS)
        {
            return false;
        }
        pContext->clientWorkMode = U_WORK_MODE;
        server->binding = BINDING_U;
    }    
	else if(strncmp(binding_mode, "UQ", size) == 0)
    {
        pContext->clientWorkMode = UQ_WORK_MODE;
        server->binding = BINDING_UQ;
    }
    else
    {
        return false;
    }
    
    c2f_encode_params(pContext);
    ctiot_update_sdataflash_needed(true);//IOT平台侧修改binding_mode，需要实时更新保存到文件系统

    return true;
}




/**
 * @brief 删除用于深睡恢复的相关信息
 * 
 */
void xy_ctlw_clear_recover_info(void)
{
    ctiot_context_t *pContext = xy_ctlw_ctiot_get_context();

    //重置bootflag标志位
    pContext->bootFlag = BOOT_NOT_LOAD;
    
    //删除session文件
    cloud_remove_file(CTLW_SESSION_FILE_UNDER_DIR);

    ctiot_update_session_status( NULL,pContext->sessionStatus, UE_NOT_LOGINED);
}



/**
 * @brief 若云服务器V4和V6IP地址均为空,则使用默认DNS解析CTwing平台提供域名进行IP地址获取
 * @warning 接口仅供自注册功能使用
 */
static void xy_ctlw_init_server_by_dns()
{
    int result = -1;

    ctiot_context_t *pContext = xy_ctlw_ctiot_get_context();

    if(pContext->serverIPV4 != NULL || pContext->serverIPV6 != NULL)//用户已设置过,从文件系统获取到了V4或者V6的服务器IP地址
    {
        return;
    }
    

    chip_ip_type_e local_ip_type = ctlw_get_ip_type();

    switch (local_ip_type)
    {
    case CHIP_IP_TYPE_V4ONLY:
    {
        xy_ctlw_set_server_by_dns(CTLW_DNS_V4ONLY, NULL, XY_CTLW_AUTO_REG_HOST_NAME);
        break;
    }
    case CHIP_IP_TYPE_V6ONLY:
    {
        result = xy_ctlw_set_server_by_dns(CTLW_DNS_V6ONLY, NULL, XY_CTLW_AUTO_REG_HOST_NAME);
        break;
    }
    case CHIP_IP_TYPE_V4V6:
    case CHIP_IP_TYPE_V4V6_V6PREPARING:
    {
    	//先尝试获取V4地址，若V4地址获取失败，则尝试获取V6地址
        result = xy_ctlw_set_server_by_dns(CTLW_DNS_V4ONLY, NULL, XY_CTLW_AUTO_REG_HOST_NAME);

        if(result)//V4地址获取失败
        {
            result = xy_ctlw_set_server_by_dns(CTLW_DNS_V6ONLY, NULL, XY_CTLW_AUTO_REG_HOST_NAME);
        }
        break;
    }
    default:
        break;
    }
}

/**
 * @brief 模组上电，自动连接电信云平台
 */
void xy_ctlw_auto_register_task(void)
{
    ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_OTHER_CLASS,"netif up,xy_ctlw_auto_register_task");
    xy_ctlw_module_entry();//业务初始化及尝试执行恢复流程

    ctiot_context_t *pContext = xy_ctlw_ctiot_get_context();
    
    if(xy_ctlw_check_if_task_running())//业务流程恢复成功，业务已处于运行态
    {
        ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_OTHER_CLASS,"xy_ctlw_auto_register_task,task is running,exit");
        osThreadExit();
    }

    //若serverIP为空，从DNS服务器IP获取serverIP
    xy_ctlw_init_server_by_dns();
    
    //若port为空，使用默认值
    if(pContext->portV4 == 0)
        pContext->portV4 = pContext->connectionType == MODE_DTLS ? CTIOT_DEFAULT_DTLS_PORT:CTIOT_DEFAULT_PORT;

    if(pContext->portV6 == 0)
        pContext->portV6 = pContext->connectionType == MODE_DTLS ? CTIOT_DEFAULT_DTLS_PORT:CTIOT_DEFAULT_PORT;


    int32_t ret = ctiot_reg(NULL,NULL);

    //注册失败，此处返回失败,业务主线程未创建,无主线程对应URC结果上报,需自行进行URC上报通知
    if(ret != CTIOT_NB_SUCCESS)
    {
        char *ret_urc = xy_malloc(32);
        snprintf(ret_urc, 32, "+CTLW: %d",ret);
        send_urc_to_ext(ret_urc, strlen(ret_urc));
        xy_free(ret_urc);
    }
    osThreadExit();
}

/**
 * @brief 本地FOTA升级重启后,需要恢复业务线程,用于FOTA升级结果上报
 * 
 */
static void xy_ctlw_fota_resume_task(void)
{
    bool ret = xy_ctlw_resume();

    if(ret == false)
    {
        ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_OTHER_CLASS,"xy_ctlw_fota_resume_task resume err!");
    }
    osThreadExit();
}
/**
 * @brief 处理业务运行过程中IP发生变化的事件
 */
static void xy_ctlw_netif_act_proc_task()
{
    //恢复CTwing业务运行时状态
    bool ret = xy_ctlw_resume();

    if(!ret)//不存在CTwing业务,未执行恢复,退出
    {
        g_ctlw_netif_ip_event_tsk_handle = NULL;
        osThreadExit();
    }  

    //业务正在运行中,IP发生变化
    if(xy_ctlw_check_if_task_running() && xy_ctlw_check_if_ip_changed())
    {
		ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_OTHER_CLASS,"xy_ctlw_ip_changed");
        //置IP事件标志位,bindmode为U模式下订阅此事件，主线程识别后,进行update和更新local_ip
        uint16_t ip_type = ctlw_get_ip_type();
        ctiot_signal_set_chip_ip_type(ip_type);
        ctiot_signal_emit_ip_event(ip_type);
    }

    g_ctlw_netif_ip_event_tsk_handle = NULL;
    osThreadExit();
}


/**
 * @brief netif变化回调
 */
void xy_ctlw_netif_act_callback(PsStateChangeEvent event)
{
    
    osThreadAttr_t thread_attr = {0};
   
    /*上电自动连接云平,线程退出后,handle不置NULL,上电或深睡唤醒仅执行一次,若用户去注册,再次走到netif回调流程,不会执行第二次*/
    if(g_ctlw_auto_reg_tsk_handle == NULL && xy_ctlw_get_nvm_reg_mode() == XY_CTLW_AUTO_REG)
    {
        thread_attr.name = "ctlw_auto_reg";
        thread_attr.priority = osPriorityNormal1;
        thread_attr.stack_size = osStackShared;
        g_ctlw_auto_reg_tsk_handle = osThreadNew((osThreadFunc_t)(xy_ctlw_auto_register_task), NULL, &thread_attr);

    }//当前系统刚刚执行完本地FOTA且使用CTwing SDK对接云平台
#ifdef WITH_FOTA
    else if(!Is_WakeUp_From_Dsleep()  && xy_ctlw_check_if_fota_running() && g_ctlw_fota_event_task_handle == NULL)
    {
        thread_attr.name = "ctlw_fota_resume";
        thread_attr.priority = osPriorityNormal1;
        thread_attr.stack_size = osStackShared;
        g_ctlw_fota_event_task_handle = osThreadNew((osThreadFunc_t)(xy_ctlw_fota_resume_task), NULL, &thread_attr);

    }
#endif
    
    //netif的IP事件处理 
    if(g_ctlw_netif_ip_event_tsk_handle == NULL && xy_ctlw_check_if_session_file_exist())
    {
        thread_attr.name = "ctlw_pdp_act_proc";
        thread_attr.priority = osPriorityNormal1;
        thread_attr.stack_size = osStackShared;
        g_ctlw_netif_ip_event_tsk_handle = osThreadNew((osThreadFunc_t)(xy_ctlw_netif_act_proc_task), NULL, &thread_attr);

    }
}


void xy_ctlw_ctl_init(void)
{
    static bool is_ctlw_ctl_init = false;

    if(is_ctlw_ctl_init != false)
    {
        return;
    }
    is_ctlw_ctl_init = true;

    extern osMutexId_t g_ctlw_module_entry_mutex;
    //模块初始化时使用的互斥锁初始化
    if(g_ctlw_module_entry_mutex == NULL)
    {
        g_ctlw_module_entry_mutex = osMutexNew(NULL);
    }

    xy_reg_psnetif_callback(EVENT_PSNETIF_VALID, xy_ctlw_netif_act_callback);

}


