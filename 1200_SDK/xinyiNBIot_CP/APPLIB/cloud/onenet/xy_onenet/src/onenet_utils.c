#include "softap_nv.h"
#include "onenet_utils.h"
#include "flash_adapt.h"
#include "xy_flash.h"
#include "xy_system.h"
#include "low_power.h"
#include "factory_nv.h"
#include "net_app_resume.h"
#include "cloud_utils.h"
#include "main_proxy.h"
#include "xy_at_api.h"
#include "xy_rtc_api.h"
#include "xy_fs.h"
#include "net_app_mem.h"

onenet_session_info_t *g_onenet_session_info = NULL;
extern onenet_context_reference_t onenet_context_refs[CIS_REF_MAX_NUM];
extern onenet_context_config_t onenet_context_configs[CIS_REF_MAX_NUM];
extern osMutexId_t g_onenet_mutex;
extern osSemaphoreId_t cis_recovery_sem;
extern osThreadId_t onenet_resume_task_id;
osThreadId_t g_onenet_resume_TskHandle = NULL;
osThreadId_t g_onenet_rtc_resume_TskHandle = NULL;

/*****************************************************************************
 Function    : check_onenet_regInfo
 Description : check onenet back reginfo to decide whether needing to register onenet or not
 Input       : mode   ---Calculate or check
               data   ---databuf
               size   ---datalen
 Output      : NULL
 Return      : 0,success;
 *****************************************************************************/
int check_onenet_session_info()
{
    int current_sec = 0;

    if(g_onenet_session_info->last_update_time == 0 || g_onenet_session_info->life_time == 0)
    {
        //session文件不存在，无需恢复
        return RESUME_OTHER_ERROR;
    }

    current_sec = utils_gettime_s();

    if(!xy_tcpip_is_ok())
    {
        xy_printf(0,XYAPP, WARN_LOG, "[CIS] resume fail,tcpip is not ok.");
        return RESUME_STATE_ERROR;
    }

#if !VER_BC95
    if (g_onenet_session_info->last_update_time + g_onenet_session_info->life_time <= current_sec
        || current_sec <= g_onenet_session_info->last_update_time)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[CIS]error:onenet expired, pre_update_time:%d lifetime:%d curtime:%d\n",
            g_onenet_session_info->last_update_time, g_onenet_session_info->life_time, current_sec);
        return RESUME_LIFETIME_TIMEOUT;
    }
#endif

    return RESUME_SUCCEED;
}

int is_user_onenet_context(st_context_t* context)
{
    int i;
    if (context == 0)
        return 0;
#if CIS_ENABLE_DM
    if(context->isDM == TRUE)
    {
        return 1;
    }
#endif
    for (i = 0; i < CIS_REF_MAX_NUM; i++)
    {
        if (context == onenet_context_refs[i].onenet_context)
        {
            return 1;
        }
    }

    return 0;
}

extern st_observed_t * prv_getObserved(st_context_t * contextP,st_uri_t * uriP);

void resume_onenet_context(st_context_t* ctx)
{
    unsigned int i;
    st_observed_t *observed;
    onenet_observed_t *backup_observed;
    onenet_object_t *backup_object;
    cis_inst_bitmap_t bitmap = {0};
    cis_res_count_t  rescount;
    cis_list_t * securityInstP;
    security_instance_dup_t * targetP;

    osMutexAcquire(g_onenet_mutex, osWaitForever);

    ctx->cloud_platform = g_onenet_session_info->cloud_platform;
    ctx->platform_common_type = g_onenet_session_info->platform_common_type;

    ctx->lifetime = g_onenet_session_info->life_time;

    ctx->callback.onRead = onet_on_read_cb;
    ctx->callback.onWrite = onet_on_write_cb;
    ctx->callback.onExec = onet_on_execute_cb;
    ctx->callback.onObserve = onet_on_observe_cb;
    ctx->callback.onDiscover = onet_on_discover_cb;
    ctx->callback.onSetParams = onet_on_parameter_cb;
    ctx->callback.onEvent = onet_on_event_cb;

    xy_printf(0,XYAPP, WARN_LOG, "[CIS]resume cis context, obj count(%d)", g_onenet_session_info->object_count);
    for (i = 0; i <g_onenet_session_info->object_count; i++)
    {
        backup_object = &g_onenet_session_info->onenet_object[i];

        //make bitmap;
        bitmap.instanceCount = backup_object->instBitmapCount;
        bitmap.instanceBitmap = backup_object->instBitmapPtr;
        bitmap.instanceBytes = backup_object->instBitmapBytes;

        rescount.attrCount = backup_object->attributeCount;
        rescount.actCount = backup_object->actionCount;
        cis_addobject(ctx, backup_object->objID, &bitmap, &rescount);
    }

    securityInstP = (cis_list_t *)ctx->instSecurity;
    while (securityInstP != NULL)
    {
        targetP = (security_instance_dup_t *)securityInstP;
        if (targetP->instanceId == g_onenet_session_info->onenet_security_instance.instanceId)
        {
            if (targetP->host != NULL) cis_free(targetP->host);
            targetP->host = (char *)cis_malloc(strlen(g_onenet_session_info->onenet_security_instance.host) + 1);
            strcpy(targetP->host, g_onenet_session_info->onenet_security_instance.host);
#if CIS_ENABLE_DTLS
            // todo
#endif
            targetP->isBootstrap = g_onenet_session_info->onenet_security_instance.isBootstrap;
            targetP->shortID = g_onenet_session_info->onenet_security_instance.shortID;
            targetP->clientHoldOffTime = g_onenet_session_info->onenet_security_instance.clientHoldOffTime;
            targetP->securityMode = g_onenet_session_info->onenet_security_instance.securityMode;
#if CIS_ENABLE_DTLS
#if CIS_ENABLE_PSK
            // todo
#endif
#endif
            break;
        }
        securityInstP = securityInstP->next;
    }

    char host_len = strlen(ipaddr_ntoa(&g_onenet_session_info->net_info.remote_ip));
    ctx->server = management_makeServerList(ctx, false);
    xy_free(ctx->server->host);
    ctx->server->host = xy_malloc(host_len + 1);
    strcpy(ctx->server->host,ipaddr_ntoa(&g_onenet_session_info->net_info.remote_ip));//保存恢复之前域名会转成IP
    management_createNetwork(ctx, ctx->server);
    management_connectServer(ctx, ctx->server);
    ctx->server->registration = g_onenet_session_info->last_update_time;
    ctx->server->status = STATE_REGISTERED;
    ctx->server->location = xy_malloc(strlen((const char *)g_onenet_session_info->location) + 1);
    strcpy(ctx->server->location, (const char *)g_onenet_session_info->location);

    xy_printf(0,XYAPP, WARN_LOG, "[CIS]resume cis context, obsv count(%d)", g_onenet_session_info->observed_count);
    for (i = 0; i <g_onenet_session_info->observed_count; i++)
    {
        backup_observed = &g_onenet_session_info->observed[i];
        observed = prv_getObserved(ctx, &backup_observed->uri);

        observed->actived = true;
        observed->msgid = backup_observed->msgid;
        observed->tokenLen = backup_observed->token_len;
        cis_memcpy(observed->token, backup_observed->token, backup_observed->token_len);
        observed->lastTime = backup_observed->last_time;
        observed->counter = backup_observed->counter;
        observed->lastMid = backup_observed->lastMid;
        observed->format = backup_observed->format;
    }

    core_updatePumpState(ctx, PUMP_STATE_READY);
    ctx->registerEnabled = true;

    osMutexRelease(g_onenet_mutex);
}

void onenet_resume_task()
{
    int temp_count = 0;

    if (!is_onenet_task_running(g_onenet_session_info->ref))
    {
        if(onenet_miplcreate() != XY_OK)
        {
            xy_printf(0,XYAPP, WARN_LOG, "[CIS]onenet_miplcreate failed, give cis_recovery_sem");
            if(cis_recovery_sem != NULL)
                osSemaphoreRelease(cis_recovery_sem);

            goto out;
        }

        resume_onenet_context(onenet_context_refs[g_onenet_session_info->ref].onenet_context);

    }
    else
    {
        if(cis_recovery_sem != NULL)
            osSemaphoreRelease(cis_recovery_sem);

        goto out;
    }

out:
    onenet_resume_task_id=NULL;
    osThreadExit();
}

void onenet_rtc_resume_cb(void)
{
    send_debug_by_at_uart("+DBGINFO:[CIS] RTC update process\r\n");
    cis_module_init();
    //update前先上锁防止过程中随时睡下去
    app_delay_lock(1000);
    if(is_onenet_task_running(0))
        return;

    if(g_onenet_rtc_resume_TskHandle == NULL)
    {
        osThreadAttr_t thread_attr = {0};
        thread_attr.name = "onenet_keelive_update";
        thread_attr.priority = osPriorityNormal1;
        thread_attr.stack_size = osStackShared;     //0x400->osStackShared
        g_onenet_rtc_resume_TskHandle = osThreadNew((osThreadFunc_t)(onenet_rtc_resume_process), NULL, &thread_attr);
    }
    return;
}

int onenet_bak_allInfo(st_context_t* context)
{
    st_object_t * objectP = NULL;
    onenet_object_t *temp = NULL;
    cis_list_t * securityInstP = NULL;
    security_instance_dup_t * targetP = NULL;
    int len = 0;
    st_observed_t *observed = NULL;
    onenet_observed_t *observed_temp = NULL;
    st_server_t *server = NULL;
    unsigned short remotPort = 0;

    if (!is_user_onenet_context(context))
        return XY_ERR;

    server = context->server;
    if (server == NULL)
        return XY_ERR;

    if (context->stateStep != PUMP_STATE_READY)
    {
        send_debug_by_at_uart("+DBGINFO:[CIS] regStatus error\r\n");
        return XY_ERR;
    }

#if CIS_ENABLE_DM
    if(!context->isDM)
    {
#endif
        memcpy(g_onenet_session_info->endpointname, context->endpointName, sizeof(g_onenet_session_info->endpointname));
        len = strlen(g_onenet_session_info->endpointname);
        if(len <= 0)
        {
            send_debug_by_at_uart("+DBGINFO:[CIS] GetENDPOINTNAME err\r\n");
            return XY_ERR;
        }
#if CIS_ENABLE_DM
    }
#endif

    g_onenet_session_info->cloud_platform = context->cloud_platform;
    g_onenet_session_info->net_info.local_port = ((cisnet_t)server->sessionH)->local_port;
    g_onenet_session_info->net_info.remote_port = ((cisnet_t)server->sessionH)->port;
    xy_get_ipaddr(g_onenet_session_info->net_info.remote_ip.type, &g_onenet_session_info->net_info.local_ip);
#if CIS_ENABLE_DM
    g_onenet_session_info->net_info.is_dm = context->isDM;
#endif
    g_onenet_session_info->ref = 0; // customized, only support one onenet context now
    g_onenet_session_info->last_update_time = server->registration;
    g_onenet_session_info->life_time = context->lifetime;
    strcpy((char *)g_onenet_session_info->location, server->location);

    g_onenet_session_info->object_count = 0;
    for (objectP = context->objectList; objectP != NULL; objectP = objectP->next)
    {
        if (!std_object_isStdObject(objectP->objID))
        {
            if (g_onenet_session_info->object_count >= OBJECT_BACKUP_MAX)
            {
                send_debug_by_at_uart("+DBGINFO:[CIS] objectcount out\r\n");
                return XY_ERR;
            }
            temp = &g_onenet_session_info->onenet_object[g_onenet_session_info->object_count];
            temp->objID = objectP->objID;
            temp->instBitmapBytes = objectP->instBitmapBytes;
            temp->instBitmapCount = objectP->instBitmapCount;
            memcpy(temp->instBitmapPtr, objectP->instBitmapPtr, objectP->instBitmapBytes);
            temp->instValidCount = objectP->instValidCount;
            temp->attributeCount = objectP->attributeCount;
            temp->actionCount = objectP->actionCount;

            g_onenet_session_info->object_count++;
        }
    }

    securityInstP = (cis_list_t *)context->instSecurity;
    while (securityInstP != NULL)
    {
        targetP = (security_instance_dup_t *)securityInstP;
        if (!targetP->isBootstrap)
        {
            g_onenet_session_info->onenet_security_instance.instanceId = targetP->instanceId;
            strcpy(g_onenet_session_info->onenet_security_instance.host, targetP->host);
#if CIS_ENABLE_DTLS
            // todo
#endif
            g_onenet_session_info->onenet_security_instance.isBootstrap = targetP->isBootstrap;
            g_onenet_session_info->onenet_security_instance.shortID = targetP->shortID;
            g_onenet_session_info->onenet_security_instance.clientHoldOffTime = targetP->clientHoldOffTime;
            g_onenet_session_info->onenet_security_instance.securityMode = targetP->securityMode;
#if CIS_ENABLE_DTLS
#if CIS_ENABLE_PSK
            // todo
#endif
#endif
            break;
        }
        securityInstP = securityInstP->next;
    }

    observed = context->observedList;
    g_onenet_session_info->observed_count = 0;
    memset(g_onenet_session_info->observed, 0, sizeof(onenet_observed_t) * OBSERVE_BACKUP_MAX);
    while (observed != NULL)
    {
        if (g_onenet_session_info->observed_count >= OBSERVE_BACKUP_MAX)
        {
            send_debug_by_at_uart("+DBGINFO:[CIS] observe count out\r\n");
            return XY_ERR;
        }
        observed_temp = &(g_onenet_session_info->observed[g_onenet_session_info->observed_count]);
        if (observed->actived)
        {
            observed_temp->msgid = observed->msgid;
            observed_temp->token_len = observed->tokenLen;
            memcpy(observed_temp->token, observed->token, observed->tokenLen);
            observed_temp->last_time = observed->lastTime;
            observed_temp->counter = observed->counter;
            observed_temp->lastMid = observed->lastMid;
            observed_temp->format = observed->format;
            memcpy(&observed_temp->uri, &observed->uri, sizeof(st_uri_t));
        }
        observed = observed->next;
        g_onenet_session_info->observed_count++;
    }

    return XY_OK;
}

void onenet_netif_up_resume_process()
{
    int ret = 0;
    unsigned short mid;
    ip_addr_t pre_local_ip = {0};
    ip_addr_t new_local_ip = {0};

    cis_module_init();

    ip_addr_copy(pre_local_ip,g_onenet_session_info->net_info.local_ip);

    if((pre_local_ip.type !=IPADDR_TYPE_V4 && pre_local_ip.type !=IPADDR_TYPE_V6)
            || xy_get_ipaddr(pre_local_ip.type,&new_local_ip) == 0
            || !ip_addr_cmp(&pre_local_ip,&new_local_ip))
    {
        app_delay_lock(1000);
        onenet_resume_session();

        if (is_onenet_task_running(0))
        {
            osMutexAcquire(g_onenet_mutex, osWaitForever);
            if(g_onenet_session_info != NULL && g_onenet_session_info->cloud_platform == CLOUD_PLATFORM_COMMON)
            {
#if LWM2M_COMMON_VER
                ret = cis_update_reg_common(onenet_context_refs[0].onenet_context, g_lwm2m_common_config_data->lifetime,
                                        g_lwm2m_common_config_data->binding_mode, 0, &mid, 1 , RAI_NULL);
#endif
            }
            else
                ret = cis_update_reg(onenet_context_refs[0].onenet_context, 0, 0,RAI_NULL);
            osMutexRelease(g_onenet_mutex);
        }
    }

    g_onenet_resume_TskHandle= NULL;
    osThreadExit();
}

void onenet_rtc_resume_process()
{
    cis_module_init();
    int ret = onenet_resume_session();
    xy_printf(0,XYAPP, WARN_LOG, "\r\n onenet_rtc_resume_process update %d \r\n",ret);

    g_onenet_rtc_resume_TskHandle= NULL;
    osThreadExit();
}

void onenet_resume_timer_create(void)
{
    int lifetime = 0;
    cloud_platform_e platform = onenet_context_refs[0].onenet_context->cloud_platform;
    if(onenet_context_refs[0].onenet_context == NULL)
        return;

    lifetime = onenet_context_refs[0].onenet_context->lifetime;
    if(lifetime <= 0)
        xy_assert(0);
#if VER_BC95
#if LWM2M_COMMON_VER
    if(platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->lifetime_enable == 1)
        xy_rtc_timer_create(RTC_TIMER_ONENET,(int)CLOUD_LIFETIME_DELTA(lifetime),onenet_rtc_resume_cb,(uint8_t)0);
#endif
#else
    #if LWM2M_COMMON_VER
        if(platform == CLOUD_PLATFORM_ONENET || (platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->lifetime_enable == 1))
    #endif
            xy_rtc_timer_create(RTC_TIMER_ONENET,(int)CLOUD_LIFETIME_DELTA(lifetime),onenet_rtc_resume_cb,(uint8_t)0);
#endif
    return;
}

void onenet_resume_timer_delete(void)
{
#if VER_BC95
#if LWM2M_COMMON_VER
    if(onenet_context_refs[0].onenet_context->cloud_platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->lifetime_enable == 1)
        xy_rtc_timer_delete(RTC_TIMER_ONENET);
#endif
#else
    #if LWM2M_COMMON_VER
        if(onenet_context_refs[0].onenet_context->cloud_platform == CLOUD_PLATFORM_ONENET
                || (onenet_context_refs[0].onenet_context->cloud_platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->lifetime_enable == 1))
    #endif
            xy_rtc_timer_delete(RTC_TIMER_ONENET);
#endif

    return;
}

void onenet_resume_state_process(int code)
{
   switch (code)
   {
    case CIS_EVENT_REG_SUCCESS:
        onenet_resume_timer_create();
        break;
    case CIS_EVENT_REG_FAILED:
    case CIS_EVENT_REG_TIMEOUT:
        onenet_resume_timer_delete();
        break;
    case CIS_EVENT_UPDATE_SUCCESS:
        if(onenet_bak_allInfo(onenet_context_refs[0].onenet_context) == XY_OK)
        {
            onenet_resume_timer_create();
            cloud_save_file(ONENET_SESSION_NVM_FILE_NAME,(void*)g_onenet_session_info,sizeof(onenet_session_info_t));
        }
        else
        {
            cloud_remove_file(ONENET_SESSION_NVM_FILE_NAME);
            xy_printf(0,XYAPP, WARN_LOG, "\r\n onenet_bak_allInfo failed \r\n");
        }
        break;
    case CIS_EVENT_UPDATE_FAILED:
    case CIS_EVENT_UPDATE_TIMEOUT://update流程都要判断是否需要释放worklock
        onenet_resume_timer_delete();
        cloud_remove_file(ONENET_SESSION_NVM_FILE_NAME);
        break;
    case CIS_EVENT_UNREG_DONE:
        onenet_resume_timer_delete();
        cloud_remove_file(ONENET_SESSION_NVM_FILE_NAME);
        break;
    case CIS_EVENT_COMMON_STATUS_READY:
        if(onenet_bak_allInfo(onenet_context_refs[0].onenet_context) == XY_OK)
        {
            cloud_save_file(ONENET_SESSION_NVM_FILE_NAME,(void*)g_onenet_session_info,sizeof(onenet_session_info_t));
            onenet_resume_timer_create();
        }
        else
        {
            cloud_remove_file(ONENET_SESSION_NVM_FILE_NAME);
            xy_printf(0,XYAPP, WARN_LOG, "\r\n onenet_bak_allInfo failed \r\n");
        }
        break;
    default:
        break;
    }
}

bool onenet_session_file_exist(void)
{
    /*AP ram存储*/
    if(Is_OpenCpu_Ver())
    {
        if((*((char *)(g_cloud_mem_p + CLOUD_CHKSUM_LEN)) & (1<<3)))
            return true;
        else
            return false;
    }

    xy_file fp = NULL;

    fp = xy_fopen(ONENET_SESSION_NVM_FILE_NAME, "rb", FS_DEFAULT);

    if (fp != NULL)
    {
        xy_fclose(fp);
        return true;
    }
    else
        return false;
}

extern osMutexId_t g_onenet_module_init_mutex;
int onenet_resume_session()
{
    int check_result = RESUME_SWITCH_INACTIVE;          //不需要恢复

    if(onenet_session_file_exist() && !is_onenet_task_running(0))
    {
        osMutexAcquire(g_onenet_module_init_mutex, osWaitForever);
        check_result = check_onenet_session_info();
        if(check_result == XY_OK)
            onet_resume_task_start();
        else if(check_result == RESUME_LIFETIME_TIMEOUT)
            send_debug_by_at_uart("+DBGINFO:[CIS] lifetime expired\r\n");
        else if(check_result == RESUME_STATE_ERROR)
            send_debug_by_at_uart("+DBGINFO:[CIS] net not ok\r\n");
        osMutexRelease(g_onenet_module_init_mutex);

        xy_printf(0,XYAPP, WARN_LOG, "[onenet_resume_app] result[%d]!\n", check_result);
    }
    return check_result;
}

bool onenet_resume(void)
{
    //检查是否存在Session会话文件
    if(!onenet_session_file_exist())
    {
        return false;//不存在onenet业务
    }

    cis_module_init();
    onenet_resume_session();

    return true;
}

void cis_user_config_init()
{
    //配置类数据初始化，初始值不为0的配置需在该处完成初始化
#if VER_BC95
    strcpy(g_onenet_config_data->server_host, "183.230.40.39");
    g_onenet_config_data->server_port = 5683;
    g_onenet_config_data->bs_enable = 1;
    g_onenet_config_data->dtls_enable = 0;
    g_onenet_config_data->ack_timeout = g_ONENET_ACK_TIMEOUT;
    g_onenet_config_data->obs_autoack = 1;

#endif
}

void cis_netif_event_callback(PsStateChangeEvent event)
{
    osThreadAttr_t task_attr = {0};
    xy_printf(0,XYAPP, WARN_LOG, "onenet_netif_event_callback, netif up");

#if CIS_ENABLE_UPDATE
    if(!is_onenet_task_running(0))
    {
        cissys_ota_urc_send();
    }
#endif

    //保活模式，IP变化触发update
    if (g_onenet_resume_TskHandle == NULL && (onenet_session_file_exist() || is_onenet_task_running(0)))
    {
        task_attr.name = "onenet_netif_up_resume";
        task_attr.priority = osPriorityNormal1;
        task_attr.stack_size = osStackShared;
        g_onenet_resume_TskHandle = osThreadNew((osThreadFunc_t)(onenet_netif_up_resume_process), NULL, &task_attr);
    }

}

void cis_module_mutex_init()
{
    if(g_onenet_module_init_mutex == NULL)
        g_onenet_module_init_mutex = osMutexNew(NULL);

    return;
}

void onet_remove_session()
{
    //非深睡唤醒删除session文件    防止误恢复
    if(!Is_WakeUp_From_Dsleep() && onenet_session_file_exist())
        cloud_remove_file(ONENET_SESSION_NVM_FILE_NAME);
}

uint16_t  g_cislwm2m_inited = 0;
void cis_module_init(void)
{
    int i;
    osMutexAcquire(g_onenet_module_init_mutex, osWaitForever);
    if(g_cislwm2m_inited == 0)
    {
        g_cislwm2m_inited = 1;
        g_onenet_session_info = cloud_malloc(ONENET_SESSION_NVM_FILE_NAME);

        if(Is_WakeUp_From_Dsleep())
            cloud_read_file(ONENET_SESSION_NVM_FILE_NAME,(void*)g_onenet_session_info,sizeof(onenet_session_info_t));
        else
        {
            cloud_remove_file(ONENET_SESSION_NVM_FILE_NAME);
        }

        cloud_mutex_create(&g_onenet_mutex);
        for (i = 0; i < CIS_REF_MAX_NUM; i++)
        {
            memset(&(onenet_context_configs[i]), 0, sizeof(onenet_context_config_t));
            memset(&(onenet_context_refs[i]), 0, sizeof(onenet_context_reference_t));
            onenet_context_refs[i].ref = i;
        }

#if LWM2M_COMMON_VER
        if(g_onenet_session_info->cloud_platform == CLOUD_PLATFORM_COMMON)
        {
            //lwm2m config初始化
            g_lwm2m_common_config_data = xy_malloc(sizeof(lwm2m_common_user_config_nvm_t));
            memset(g_lwm2m_common_config_data, 0, sizeof(lwm2m_common_user_config_nvm_t));
            init_xy_lwm2m_lists();
            if( XY_ERR == cloud_read_file(LWM2M_COMMON_CONFIG_NVM_FILE_NAME,(void*)g_lwm2m_common_config_data,sizeof(lwm2m_common_user_config_nvm_t)))
                init_xy_lwm2m_config();
        }
        else
#endif
        {
            //onenet config 初始化
            g_onenet_config_data = cloud_malloc(ONENET_CONFIG_NVM_FILE_NAME);
            if( XY_ERR == cloud_read_file(ONENET_CONFIG_NVM_FILE_NAME,(void*)g_onenet_config_data,sizeof(onenet_config_nvm_t)))
                cis_user_config_init();
            else
            {
                if(g_onenet_config_data->ack_timeout > 0 && g_ONENET_ACK_TIMEOUT != g_onenet_config_data->ack_timeout)
                    g_ONENET_ACK_TIMEOUT = g_onenet_config_data->ack_timeout;
            }
        }
        osMutexRelease(g_onenet_module_init_mutex);
    }
    else
        osMutexRelease(g_onenet_module_init_mutex);
}

