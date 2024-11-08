/**
 * @file xy_ctwing_api.c
 * @author your name (you@domain.com)
 * @brief Ctwing API
 * @version 0.1
 * @date 2023-02-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "xy_utils.h"
#include "xy_at_api.h"
#include "ctlw_lwm2m_sdk.h"
#include "xy_net_api.h"
#include "oss_nv.h"
#include "net_app_resume.h"
#include "ctwing_util.h"
#include "xy_ctwing_api.h"
#include "ctwing_resume.h"


osSemaphoreId_t ctlw_api_register_sem = NULL;
osSemaphoreId_t ctlw_api_deregister_sem = NULL;
osSemaphoreId_t ctlw_api_update_sem= NULL;
osSemaphoreId_t ctlw_api_data_sent_sem= NULL;



/**
 * @brief 设置云连接参数
 * 
 * @param server_ip 合法IP字符串，支持IPV4，IPV6，点分十进制，例"221.229.214.202"
 * @param server_port 端口，合法范围:大于0,小于65535
 * @param lifetime 合法范围:大于等于300，小于等于30*86400
 * @param auth_mode 终端认证协议,ctiot_id_auth_mode_e类型,建议入参AUTHMODE_SIMPLIFIED
 * @return int 
 */
int32_t ctlw_cloud_setting(uint8_t *server_ip, int32_t server_port, int32_t lifetime, int32_t auth_mode)
{
    int32_t ret = CTIOT_NB_SUCCESS;
    
    xy_ctlw_module_entry();//模块初始化

    ctiot_context_t *pContext = xy_ctlw_ctiot_get_context();
    
	if(pContext->sessionStatus != UE_NOT_LOGINED)//非未登录状态,不允许进行参数设置
    {
        ret = CTIOT_NB_FAILED;
        goto exit;
    }

    if(server_ip == NULL || (server_port <= 0 || server_port >=65535) || lifetime <300 || lifetime >30*86400)//param is invalid
    {
        ret = CTIOT_NB_FAILED;
        goto exit;
    }    
    

    uint8_t ip_type = xy_get_IpAddr_type(server_ip);

    if(ip_type == -1)
    {
        ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctlw ip is illegal\r\n");
        ret = CTIOT_NB_FAILED;
        goto exit;        
    }

    ret = ctiot_set_server(0, ip_type, server_ip, server_port);//设置IP地址

    if(ret != CTIOT_NB_SUCCESS)
    {
        ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctlw api set server failed %d\r\n",ret);
        ret = CTIOT_NB_FAILED;
        goto exit;
    }

    ret = ctiot_set_lifetime(lifetime);//设置lifetime
    if(ret != CTIOT_NB_SUCCESS)
    {
        ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctlw api set lifetime failed %d\r\n",ret);
        ret = CTIOT_NB_FAILED;
        goto exit;
    }

    ret = ctiot_set_mod(AUTH_PROTOCOL_TYPE, auth_mode);//设置终端认证协议类型 imei

    if(ret != CTIOT_NB_SUCCESS)
    {
        ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctlw api set lifetime failed %d\r\n",ret);
        ret = CTIOT_NB_FAILED;
        goto exit;
    }

exit:
    return ret;
    
}




/**
 * @brief 向AEP平台发起注册,注册成功后可进行数据收发，注册失败则处于未登录状态
 * 
 * @param timeout 等待时长,单位:秒
 * @return int32_t 
 * @note  同步接口
 */
int32_t ctlw_cloud_register(int32_t timeout)
{
    int ret = CTIOT_NB_SUCCESS;
    
    if(xy_ctlw_check_if_task_running() && ctiotprv_get_session_status() == UE_LOGINED_OBSERVED)//云业务已在运行，当做注册成功处理
        goto exit;

    xy_ctlw_clear_recover_info();//API接口注册不考虑深睡恢复,清除深睡恢复相关信息

    if(ctlw_api_register_sem == NULL)
        ctlw_api_register_sem = osSemaphoreNew(0xFFFF, 0, NULL);

    ret = ctiot_reg(NULL,NULL);

    if(ret != CTIOT_NB_SUCCESS)//注册失败，此时ctlw主线程未创建
    {
        ret = CTIOT_NB_FAILED;
        ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctlw api reg step failed %d\r\n",ret);
        goto exit;
    }

    while(osSemaphoreAcquire(ctlw_api_register_sem, 0) == osOK){};//清除可能多于的信号量，确保信号量与本次注册配对使用
    if(osSemaphoreAcquire(ctlw_api_register_sem, timeout*1000) != osOK)//注册失败
    {
        if(xy_ctlw_check_if_task_running() && ctiotprv_get_session_status() == UE_LOGINED)//已注册,未19/0/0observe,未达到能数据收发，判定为注册失败
        {
            ctiot_dereg(1);//本地注销,恢复成未登录状态,不与远程AEP平台通信交互

        }
        ctiot_context_t *pContext = xy_ctlw_ctiot_get_context();
        while(pContext->sendAndRecvThreadStatus != THREAD_STATUS_NOT_LOADED)
        {
            osDelay(100);
        }
        ret =  CTIOT_NB_FAILED;
    }

exit:
    return ret;
}    
    



/**
 * @brief 发送数据到AEP平台
 * @param data 待发送数据
 * @param sendMode  SENDMODE_NON:NON data ,SENDMODE_CON:CON data
 * @param timeout 等待时长,单位:秒
 * @return int32_t CTIOT_NB_SUCCESS:发送成功    CTIOT_NB_FAILED:发送失败
 * @note 同步接口
 */
int32_t ctlw_cloud_send_data(uint8_t *data, ctiot_send_mode_e sendMode, int32_t timeout)
{
    int32_t ret = CTIOT_NB_SUCCESS;
    uint8_t * resultBuf = xy_malloc(40);

    if(!xy_ctlw_check_if_task_running())
    {
        ret = CTIOT_NB_FAILED;
        goto exit;
    }
        
    if(ctlw_api_data_sent_sem == NULL)
        ctlw_api_data_sent_sem = osSemaphoreNew(0xFFFF, 0, NULL);


    if(ctiot_send(data, sendMode, resultBuf) != CTIOT_NB_SUCCESS)
    {
        ret = CTIOT_NB_FAILED;
        xy_printf(0,XYAPP, WARN_LOG, "[ctlw_send]: ctiot_send failed!");
        goto exit;
    }

    while(osSemaphoreAcquire(ctlw_api_data_sent_sem, 0) == osOK){};//清除可能多于的已release信号量，确保信号量与本次发送数据配对使用
    
    if(osSemaphoreAcquire(ctlw_api_data_sent_sem, timeout *1000)!= osOK)
    {
        ret = CTIOT_NB_FAILED;
        goto exit;
    }


exit:
    if(resultBuf)
        xy_free(resultBuf);

    return ret;
}



/**
 * @brief 向云平台发起lifetime更新请求,若成功,状态不变仍处于已登录状态，若update失败，则状态更新为未登录状态
 * 
 * @param timeout 等待时长,单位:秒
 * @return int32_t 
 */
int32_t ctlw_cloud_update(int32_t timeout)
{
    int32_t ret = CTIOT_NB_SUCCESS;

    if(!xy_ctlw_check_if_task_running())
    {
        ret = CTIOT_NB_FAILED;
        goto exit;
    }
        
    if(ctlw_api_update_sem == NULL)
        ctlw_api_update_sem = osSemaphoreNew(0xFFFF, 0, NULL);


    uint16_t msgid = 0;
    ret = ctiot_update(0, &msgid);

    if(ret != CTIOT_NB_SUCCESS)//update失败，若主线程已创建,此时并未通知主线程发送update
    {
        ret = CTIOT_NB_FAILED;
        goto dereg;
    }

    while(osSemaphoreAcquire(ctlw_api_update_sem, 0) == osOK){};//清除可能多于的已release信号量，确保信号量与本次update配对使用
    if(osSemaphoreAcquire(ctlw_api_update_sem, timeout *1000) != osOK)//update failed
    {
dereg:
        if(xy_ctlw_check_if_task_running())
        {
            ctiot_dereg(1);//update失败,执行本地注销
        }
        ctiot_context_t *pContext = xy_ctlw_ctiot_get_context();
        while(pContext->sendAndRecvThreadStatus != THREAD_STATUS_NOT_LOADED)
        {
            osDelay(100);
        }
        ret =  CTIOT_NB_FAILED;
    }
    
exit:
    return ret;
}

/**
 * @brief 向AEP云平台发起去注册请求，去注册完成后本地处于未登录状态
 * 
 * @param timeout 等待时长,单位:秒
 * @return int32_t 
 */
int32_t ctlw_cloud_deregister(int32_t timeout)
{
    int ret = CTIOT_NB_SUCCESS;

    if(!xy_ctlw_check_if_task_running())//业务模块不在运行,不需要执行去注册,返回去注册成功
        return ret;

    if(ctlw_api_deregister_sem == NULL)
        ctlw_api_deregister_sem = osSemaphoreNew(0xFFFF, 0, NULL);

    ctiot_dereg(0);//通知主线程执行deregister操作

    while(osSemaphoreAcquire(ctlw_api_deregister_sem, 0) == osOK){};//清除可能多于的已release信号量，确保信号量与本次去注册配对使用
    osSemaphoreAcquire(ctlw_api_deregister_sem, timeout *1000);


    return ret;
}


/**
 * @brief api信号量释放
 * 
 * @param sem_type   ctlw_api_sem_type_e类型
 */
void ctlw_cloud_api_sem_give(ctlw_api_sem_type_e sem_type)
{
    switch(sem_type)
    {
        case CTLW_API_REG_SUCCESS_SEM:
        {
            if(ctlw_api_register_sem == NULL)
                return;

            osSemaphoreRelease(ctlw_api_register_sem);
            break;
        }
        case CTLW_API_DEREG_SUCCESS_SEM:
        {
            if(ctlw_api_deregister_sem == NULL)
                return;
            
            osSemaphoreRelease(ctlw_api_deregister_sem);           
            break;
        }
        case CTLW_API_UPDATE_SUCCESS_SEM:
        {
            if(ctlw_api_update_sem == NULL)
                return;

            osSemaphoreRelease(ctlw_api_update_sem);       
            break;
        }
        case CTLW_API_DATA_SENT_SUCCESS_SEM:
        {
            if(ctlw_api_data_sent_sem == NULL)
                return;

            osSemaphoreRelease(ctlw_api_data_sent_sem);
            break;
        }
    }
}


/**
 * @brief 根据对应notify事件，执行api接口相关的处理
 * 
 * @param notifyType 
 * @param subType 
 * @param value 
 * @param data1 
 */
void ctlw_notify_api_event_process(uint8_t notifyType, uint8_t subType, uint16_t value, uint32_t data1)
{
    ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_OTHER_CLASS,"ctlw_notify_api_event_process,subType= %d,notifyType= %d \r\n",subType, notifyType);
    
    xy_ctlw_notify_state_e state = XY_CTLW_STATE_MAX;

    if(notifyType != CTIOT_NOTIFY_ASYNC_NOTICE)
        return;
    
    state = xy_ctlw_get_notify_state(subType, value, data1);

    ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_OTHER_CLASS,"xy ctlw get notify state= %d \r\n",state);

    switch(state)
    {
        case XY_CTLW_STATE_19OBSERVED://注册成功且19/0/0observed
        {
            ctlw_cloud_api_sem_give(CTLW_API_REG_SUCCESS_SEM);
            break;
        }
        case XY_CTLW_STATE_UPDATE_SUCESSED:
        {
            ctlw_cloud_api_sem_give(CTLW_API_UPDATE_SUCCESS_SEM);
            break;
        }
        case XY_CTLW_STATE_DEREGISTERED:
        {
            ctlw_cloud_api_sem_give(CTLW_API_DEREG_SUCCESS_SEM);
            break;
        }
        case XY_CTLW_DATA_SENT_SUCCESS:
        {
            ctlw_cloud_api_sem_give(CTLW_API_DATA_SENT_SUCCESS_SEM);
            break;
        }

        default:
            break;
    }
}


