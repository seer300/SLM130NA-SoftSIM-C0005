/*----------------------------------------------------------------------------
 * Copyright (c) <2016-2018>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 *---------------------------------------------------------------------------*/

#if TELECOM_VER
#include "xy_utils.h"
#include "cloud_utils.h"
#include "internals.h"
#include "agent_tiny_demo.h"
#include "agenttiny.h"
#include "atiny_context.h"
#include "atiny_socket.h"
#include "connection.h"
#include "atiny_log.h"
#include "atiny_rpt.h"
#include "atiny_osdep.h"
#include "net_app_resume.h"
#include "atiny_fota_manager.h"
#include "atiny_fota_state.h"
#include "oss_nv.h"
#include "liblwm2m.h"
#include "cdp_backup.h"
#include "xy_fota.h"

#if WITH_MBEDTLS_SUPPORT
#include "dtls_interface.h"
#endif

extern int cdp_deregister_flag;
extern osSemaphoreId_t  cdp_recovery_sem;
extern upstream_info_t *upstream_info;
extern osMutexId_t g_upstream_mutex;
extern recvdate_info_t *recvdata_info;
extern osMutexId_t g_recvdata_mutex;
extern osThreadId_t g_lwm2m_recv_TskHandle;
extern int cdp_register_fail_flag;
extern cdp_config_nvm_t *g_cdp_config_data;

osSemaphoreId_t cdp_api_register_sem   = NULL;
osSemaphoreId_t cdp_api_deregister_sem = NULL;
osSemaphoreId_t cdp_api_sendasyn_sem = NULL;
osSemaphoreId_t cdp_api_update_sem = NULL;
osSemaphoreId_t cdp_wait_sem = NULL;
char cdp_dtls_wait_flag = 0;

static void atiny_event_handle(xy_module_type_t type, int code, const char *arg, int arg_len)
{
	cdp_lwm2m_event_handle(type, code, arg);
    switch (type)
    {
    case MODULE_LWM2M:
    {  
        if (code == XY_STATE_REGISTERED)
        {
            atiny_event_notify(ATINY_REG_OK, NULL, 0);
        }
        else if (code == XY_STATE_REG_FAILED)
        {
            atiny_event_notify(ATINY_REG_FAIL, NULL, 0);

        }
        else if (code == XY_STATE_UPDATE_DONE)
        {
            atiny_event_notify(ATINY_REG_UPDATED, NULL, 0);
        }
        else if (code == XY_STATE_BS_FINISHED)
        {
            atiny_event_notify(ATINY_BS_COMPLETED, NULL, 0);
        }
        else if (code == XY_STATE_DEREGISTERED)
        {
            atiny_event_notify(ATINY_DEREG, NULL, 0);
        }
        else if (code == XY_STATE_RECOVERY_DONE)
        {
            atiny_event_notify(ATINY_RECOVERY_OK, NULL, 0);
        }
        else if (code == XY_RECV_UPDATE_PKG_URL_NEEDED)
        {
            atiny_event_notify(ATINY_RECV_UPDATE_PKG_URL_NEEDED, NULL, 0);
        }
        else if (code == XY_DOWNLOAD_COMPLETED)
        {
            atiny_event_notify(ANTIY_DOWNLOAD_COMPLETED, NULL, 0);
        }
        break;
    }
    case MODULE_NET:
    {
    	if (code == XY_DTLS_SHAKEHAND_SUCCESSED)
        {
            atiny_event_notify(ATINY_DTLS_SHAKEHAND_SUCCESS, NULL, 0);

        }
        else if (code == XY_DTLS_SHAKEHAND_FAILED)
        {
            atiny_event_notify(ATINY_DTLS_SHAKEHAND_FAILED, NULL, 0);
        }
        break;
    }
    case MODULE_URI:
    {
        if ((arg == NULL) || (arg_len < sizeof(lwm2m_uri_t)))
        {
            break;
        }
        if (code == OBSERVE_UNSUBSCRIBE)
        {
            if (dm_isUriOpaqueHandle((lwm2m_uri_t *)arg))
            {
                atiny_report_type_e rpt_type = APP_DATA;

				atiny_event_notify(ATINY_DATA_UNSUBSCRIBLE, (char *)&rpt_type, sizeof(rpt_type));
            }
            else
            {
                if (((lwm2m_uri_t*)arg)->objectId == 5 && ((lwm2m_uri_t*)arg)->instanceId == 0 && ((lwm2m_uri_t*)arg)->resourceId == 3)
                {
                    atiny_report_type_e rpt_type = FIRMWARE_UPDATE_STATE;
					
					atiny_event_notify(ATINY_DATA_UNSUBSCRIBLE, (char*)&rpt_type, sizeof(rpt_type));
                }
            }
            (void)atiny_clear_rpt_data((lwm2m_uri_t *)arg, SENT_FAIL);
        }
        else if (code == OBSERVE_SUBSCRIBE)
        {
            if(dm_isUriOpaqueHandle((lwm2m_uri_t *)arg))
            {
				atiny_report_type_e rpt_type = APP_DATA;
				atiny_event_notify(ATINY_DATA_SUBSCRIBLE, (char *)&rpt_type, sizeof(rpt_type));
            }
            else
            {
                if (((lwm2m_uri_t*)arg)->objectId == 5 && ((lwm2m_uri_t*)arg)->instanceId == 0 && ((lwm2m_uri_t*)arg)->resourceId == 3)
                {
#ifdef CONFIG_FEATURE_FOTA
                    atiny_report_type_e rpt_type = FIRMWARE_UPDATE_STATE;
                    atiny_event_notify(ATINY_DATA_SUBSCRIBLE, (char*)&rpt_type, sizeof(rpt_type));
#endif
                }
            }
        }
        break;
    }
    default:
        break;
    }
}

static void atiny_observe_handle_ack(lwm2m_transaction_t *transacP, void *message)
{
    atiny_ack_callback ack_callback = (atiny_ack_callback)transacP->cfg.callback;
    if (transacP->ack_received)
    {
        coap_packet_t* message_t = (coap_packet_t*)message;
    	if(COAP_TYPE_RST == message_t->type)
        	ack_callback((atiny_report_type_e)(transacP->cfg.type), transacP->cfg.cookie, SENT_GET_RST, transacP->mID);
		else
			ack_callback((atiny_report_type_e)(transacP->cfg.type), transacP->cfg.cookie, SENT_SUCCESS, transacP->mID);
    }
    else if (transacP->retrans_counter > COAP_MAX_RETRANSMIT)
    {
        ack_callback((atiny_report_type_e)(transacP->cfg.type), transacP->cfg.cookie, SENT_TIME_OUT, transacP->mID);
    }
    else
    {
        ack_callback((atiny_report_type_e)(transacP->cfg.type), transacP->cfg.cookie, SENT_FAIL, transacP->mID);
    }
}

static void atiny_connection_err_notify(lwm2m_context_t *context, connection_err_e err_type, bool boostrap_flag)
{
    handle_data_t *handle = NULL;

    if((NULL == context) || (NULL == context->userData))
    {
        ATINY_LOG(LOG_ERR, "null point");
        return;
    }

    if(!boostrap_flag)
    {
        handle = ATINY_FIELD_TO_STRUCT(context->userData, handle_data_t, client_data);
        (void)atiny_reconnect(handle);
    }
    ATINY_LOG(LOG_INFO, "connection err type %d bootstrap %d", err_type, boostrap_flag);
}

static int atiny_check_bootstrap_init_param(atiny_param_t *atiny_params)
{
    if(NULL == atiny_params)
    {
        return ATINY_ARG_INVALID;
    }

    if(BOOTSTRAP_FACTORY == atiny_params->server_params.bootstrap_mode)
    {
        if((NULL == atiny_params->security_params[0].server_ip) || (NULL == atiny_params->security_params[0].server_port))
        {
            LOG("[bootstrap_tag]: BOOTSTRAP_FACTORY mode's params is wrong, should have iot server ip/port");
            return ATINY_ARG_INVALID;
        }
    }
    else if(BOOTSTRAP_CLIENT_INITIATED == atiny_params->server_params.bootstrap_mode)
    {
        if((NULL == atiny_params->security_params[1].server_ip) || (NULL == atiny_params->security_params[1].server_port))
        {
            LOG("[bootstrap_tag]: BOOTSTRAP_CLIENT_INITIATED mode's params is wrong, should have bootstrap server ip/port");
            return ATINY_ARG_INVALID;
        }
    }
    else if(BOOTSTRAP_SEQUENCE == atiny_params->server_params.bootstrap_mode)
    {
        return ATINY_OK;
    }
    else
    {
        //it is ok? if the mode value is not 0,1,2, we all set it to 2 ?
        LOG("[bootstrap_tag]: BOOTSTRAP only have three mode, should been :0,1,2");
        return ATINY_ARG_INVALID;
    }


    return ATINY_OK;
}

#if WITH_MBEDTLS_SUPPORT
static int atiny_check_psk_init_param(atiny_param_t *atiny_params)
{
    int i = 0;
    int psk_id_len = 0;
    int psk_len = 0;
    const int PSK_ID_LIMIT_LEN = 128;
    const int PSK_LIMIT_LEN = 64;
    int total_element = 0;

    if(NULL == atiny_params)
    {
        return ATINY_ARG_INVALID;
    }

    //security_params have 2 element, we have 2 pair psk.
    total_element = (sizeof(atiny_params->security_params)) / (sizeof(atiny_params->security_params[0]));

    for(i = 0; i < total_element; i++)
    {
        //if there are null, we could run not in security mode
        if((atiny_params->security_params[i].psk_Id != NULL) && (atiny_params->security_params[i].psk != NULL))
        {
            psk_id_len = strlen(atiny_params->security_params[i].psk_Id);
            psk_len = strlen(atiny_params->security_params[i].psk);

            //the limit of the len, please read RFC4279  or OMA-TS-LightweightM2M E.1.1
            if((psk_id_len > PSK_ID_LIMIT_LEN) || (psk_len > PSK_LIMIT_LEN))
            {
                LOG("[bootstrap_tag]: psk_Id len over 128 or psk len over 64");
                return ATINY_ARG_INVALID;
            }
        }
    }

    return ATINY_OK;
}
#endif

/*
* modify info:
*     date:    2018-05-30
*     reason:  modify for bootstrap mode, origin code only support FACTORY mode.
*/
int atiny_init_objects(atiny_param_t *atiny_params, const atiny_device_info_t *device_info, handle_data_t *handle)
{
    int result;
	long ret;
    client_data_t *pdata;
    lwm2m_context_t *lwm2m_context = NULL;
    uint16_t serverId = SERVER_ID;
    char *epname = (char *)device_info->endpoint_name;



    pdata = &handle->client_data;
    memset(pdata, 0, sizeof(client_data_t));

    ATINY_LOG(LOG_INFO, "Trying to init objects");

    lwm2m_context = lwm2m_init(pdata);
    if (NULL == lwm2m_context)
    {
        ATINY_LOG(LOG_FATAL, "lwm2m_init fail");
        return ATINY_MALLOC_FAILED;
    }
    ret = cloud_mutex_create(&lwm2m_context->observe_mutex);
    if (XY_OK != ret)
    {
        ATINY_LOG(LOG_FATAL, "atiny_mutex_create fail");
        lwm2m_free(lwm2m_context);
        return ATINY_RESOURCE_NOT_ENOUGH;
    }

    pdata->lwm2mH = lwm2m_context;

    handle->lwm2m_context = lwm2m_context;

    //even if not in bootstrap sequence mode, still set it NO_BS_SEQUENCE_STATE
    lwm2m_initBootStrap(lwm2m_context, atiny_params->server_params.bootstrap_mode);

    handle->obj_array[OBJ_SECURITY_INDEX] = get_security_object(serverId, atiny_params, lwm2m_context);

    if (NULL ==  handle->obj_array[OBJ_SECURITY_INDEX])
    {
        ATINY_LOG(LOG_FATAL, "Failed to create security object");
        return ATINY_MALLOC_FAILED;
    }
    pdata->securityObjP = handle->obj_array[OBJ_SECURITY_INDEX];

    handle->obj_array[OBJ_SERVER_INDEX] = get_server_object(serverId, atiny_params->server_params.binding,
                                          atiny_params->server_params.life_time, atiny_params->server_params.storing_cnt != 0);
    if (NULL == handle->obj_array[OBJ_SERVER_INDEX])
    {
        ATINY_LOG(LOG_FATAL, "Failed to create server object");
        return ATINY_MALLOC_FAILED;
    }

    handle->obj_array[OBJ_ACCESS_CONTROL_INDEX] = acc_ctrl_create_object();
    if (NULL == handle->obj_array[OBJ_ACCESS_CONTROL_INDEX])
    {
        ATINY_LOG(LOG_FATAL, "Failed to create access control object");
        return ATINY_MALLOC_FAILED;
    }

    handle->obj_array[OBJ_DEVICE_INDEX] = get_object_device(atiny_params, device_info->manufacturer);
    if (NULL == handle->obj_array[OBJ_DEVICE_INDEX])
    {
        ATINY_LOG(LOG_FATAL, "Failed to create device object");
        return ATINY_MALLOC_FAILED;
    }

    handle->obj_array[OBJ_CONNECT_INDEX] = get_object_conn_m(atiny_params);
    if (NULL == handle->obj_array[OBJ_CONNECT_INDEX])
    {
        ATINY_LOG(LOG_FATAL, "Failed to create connect object");
        return ATINY_MALLOC_FAILED;
    }

    handle->obj_array[OBJ_FIRMWARE_INDEX] = get_object_firmware(atiny_params);
#ifdef CONFIG_FEATURE_FOTA
    if (NULL == handle->obj_array[OBJ_FIRMWARE_INDEX])
    {
        ATINY_LOG(LOG_FATAL, "Failed to create firmware object");
        return ATINY_MALLOC_FAILED;
    }
#endif

    handle->obj_array[OBJ_LOCATION_INDEX] = get_object_location();
    if (NULL == handle->obj_array[OBJ_LOCATION_INDEX])
    {
        ATINY_LOG(LOG_FATAL, "Failed to create location object");
        return ATINY_MALLOC_FAILED;
    }

    handle->obj_array[OBJ_APP_INDEX] = get_binary_app_data_object(atiny_params);
    if (NULL == handle->obj_array[OBJ_APP_INDEX])
    {
        ATINY_LOG(LOG_FATAL, "Failed to create app object");
        return ATINY_MALLOC_FAILED;
    }

    result = lwm2m_configure(lwm2m_context, epname, NULL, NULL, OBJ_MAX_NUM, handle->obj_array);
    if (result != 0)
    {
        return ATINY_RESOURCE_NOT_FOUND;
    }

    return ATINY_OK;
}

int atiny_init(handle_data_t *phandle)
{
    int ret;
    
    ret = atiny_init_rpt();
    if (ret != ATINY_OK)
    {
        ATINY_LOG(LOG_FATAL, "atiny_init_rpt fail,ret=%d", ret);
        return ret;
    }

    if (NULL == phandle)
    {
        ATINY_LOG(LOG_FATAL, "Invalid args");
        return ATINY_ARG_INVALID;
    }

    if(ATINY_OK != atiny_check_bootstrap_init_param(phandle->atiny_params))
    {
        LOG("[bootstrap_tag]: BOOTSTRAP's params are wrong");
        return ATINY_ARG_INVALID;
    }

#if WITH_MBEDTLS_SUPPORT
    if(ATINY_OK != atiny_check_psk_init_param(phandle->atiny_params))
    {
        LOG("[bootstrap_tag]: psk params are wrong");
    }
#endif

    ret = cloud_mutex_create(&(phandle->quit_sem));
    if (XY_OK != ret)
    {
        ATINY_LOG(LOG_FATAL, "atiny_mutex_create fail");
        return ATINY_RESOURCE_NOT_ENOUGH;
    }
    cloud_mutex_lock(&(phandle->quit_sem),MUTEX_LOCK_INFINITY);

    atiny_device_info_t *device_info = (atiny_device_info_t *)cdp_get_device_info();
	if(device_info == NULL)
	{
		return ATINY_ARG_INVALID;
	}
	
    ret = atiny_init_objects(phandle->atiny_params, device_info, phandle);

	if(device_info != NULL)
    {
    	if(device_info->endpoint_name != NULL)
		{
			xy_free(device_info->endpoint_name);
			device_info->endpoint_name = NULL;
		}
			
        xy_free(device_info);
        device_info = NULL;
    }
	
    if (ret != ATINY_OK)
    {
        ATINY_LOG(LOG_FATAL, "atiny_init_object fail %d", ret);
        return ret;
    }

#ifdef CONFIG_FEATURE_FOTA
    (void)atiny_fota_manager_set_lwm2m_context(atiny_fota_manager_get_instance(), phandle->lwm2m_context);
#endif

    lwm2m_register_observe_ack_call_back(atiny_observe_handle_ack);
    lwm2m_register_event_handler(atiny_event_handle);
    lwm2m_register_connection_err_notify(atiny_connection_err_notify);

    phandle->recv_buffer = (uint8_t *)lwm2m_malloc(MAX_PACKET_SIZE);
    if(phandle->recv_buffer == NULL)
    {
        ATINY_LOG(LOG_FATAL, "memory not enough");
        return ATINY_MALLOC_FAILED;
    }
	memset(phandle->recv_buffer, 0x00, MAX_PACKET_SIZE);
    return ATINY_OK;
}

void atiny_deinit(void *handle)
{
    handle_data_t *handle_data = (handle_data_t *)handle;
	osMutexId_t *sem = NULL;
    if (handle_data == NULL)
    {
        return;
    }

#ifdef CONFIG_FEATURE_FOTA
    atiny_fota_manager_destroy(atiny_fota_manager_get_instance());
#endif

    if(handle_data->recv_buffer != NULL)
    {
        lwm2m_free(handle_data->recv_buffer);
    }
    if (handle_data->obj_array[OBJ_SECURITY_INDEX] != NULL)
    {
        clean_security_object(handle_data->obj_array[OBJ_SECURITY_INDEX]);
    }

    if (handle_data->obj_array[OBJ_SERVER_INDEX] != NULL)
    {
        clean_server_object(handle_data->obj_array[OBJ_SERVER_INDEX]);
    }

    if (handle_data->obj_array[OBJ_ACCESS_CONTROL_INDEX] != NULL)
    {
        acl_ctrl_free_object(handle_data->obj_array[OBJ_ACCESS_CONTROL_INDEX]);
    }

    if (handle_data->obj_array[OBJ_DEVICE_INDEX] != NULL)
    {
        free_object_device(handle_data->obj_array[OBJ_DEVICE_INDEX]);
    }

    if (handle_data->obj_array[OBJ_CONNECT_INDEX] != NULL)
    {
        free_object_conn_m(handle_data->obj_array[OBJ_CONNECT_INDEX]);
    }

    if (handle_data->obj_array[OBJ_FIRMWARE_INDEX] != NULL)
    {
        free_object_firmware(handle_data->obj_array[OBJ_FIRMWARE_INDEX]);
    }

    if (handle_data->obj_array[OBJ_LOCATION_INDEX] != NULL)
    {
        free_object_location(handle_data->obj_array[OBJ_LOCATION_INDEX]);
    }

    if (handle_data->obj_array[OBJ_APP_INDEX] != NULL)
    {
        free_binary_app_data_object(handle_data->obj_array[OBJ_APP_INDEX]);
    }

    if (handle_data->lwm2m_context != NULL)
    {
        lwm2m_close(handle_data->lwm2m_context);
    }
    atiny_destroy_rpt();
    cloud_mutex_unlock(&handle_data->quit_sem);

    sem = &handle_data->quit_sem;
    if(sem != NULL)
        cloud_mutex_destroy(sem);
}

// cdp relative, send message via lwm2m 19/0/0
int atiny_recvdata_list_insert(char *data, int data_len, void *connP)
{
    recvdata_list_t *node = NULL;
    if (data == NULL || data_len == 0)
        return 0;

    osMutexAcquire(g_recvdata_mutex, portMAX_DELAY);

    node = (recvdata_list_t*)xy_malloc(sizeof(recvdata_list_t));
    if (node == NULL)
    {
        goto failed;
    }
	memset(node, 0x00, sizeof(recvdata_list_t));
    
    node->data = (char*)xy_malloc(data_len);
    if (node->data == NULL)
    {
        goto failed;
    }

    memcpy(node->data, data, data_len);
    node->data_len = data_len;
    node->connP = connP;
    if (recvdata_info->tail == NULL)
    {
        recvdata_info->head = node;
        recvdata_info->tail = node;
    }
    else
    {
        recvdata_info->tail->next = node;
        recvdata_info->tail = node;
    }

    recvdata_info->pending_num++;

    osMutexRelease(g_recvdata_mutex);
    
    if(cdp_wait_sem != NULL)
        osSemaphoreRelease(cdp_wait_sem);  

    return 0;

failed:
    if (node != NULL)
    {
        if (node->data != NULL)
        {
            xy_free(node->data);
        }
        xy_free(node);
    }

    osMutexRelease(g_recvdata_mutex);
    return -1;
}

//return the oldest buffered data, data length returned by output argument
void atiny_recvdata_proc(lwm2m_context_t *contextP)
{
    recvdata_list_t *node;
    uint8_t *data = NULL;
    int data_len = 0;
    connection_t *connP = NULL;

	if(recvdata_info == NULL)
		return;

    osMutexAcquire(g_recvdata_mutex, portMAX_DELAY);
    if (recvdata_info->pending_num > 0 && recvdata_info->head != NULL)
    {
        node = recvdata_info->head;
        recvdata_info->head = recvdata_info->head->next;
        
        recvdata_info->pending_num--;
        if (recvdata_info->pending_num == 0)
        {
            recvdata_info->tail = NULL;
        }
        
        data_len = node->data_len;
        data = node->data;
        connP = node->connP;
        
        xy_free(node);
    }

    osMutexRelease(g_recvdata_mutex);
    if(data != NULL && data_len > 0)  
    {
        lwm2m_handle_packet(contextP, data, data_len, connP);
        xy_free(data);
        data = NULL;
        data_len = 0;
    }
}

void app_downdata_recv(void)
{
    client_data_t *dataP = NULL;
    int numBytes = 0;
    connection_t *connP = NULL;
    lwm2m_context_t *contextP = g_phandle->lwm2m_context;
    uint8_t *recv_buffer = g_phandle->recv_buffer;

    ATINY_LOG(LOG_INFO,"[King] app_downdata_recv");
    while(1)
    {
		//此处位置提前，主线程注册失败标志位置位后，辅线程先退出，然后置主线程退出标志位，主线程退出
		if(0 != cdp_deregister_flag || 1 == cdp_register_fail_flag || !cdp_handle_exist())
        {
            ATINY_LOG(LOG_INFO,"[King] app_downdata_recv over");

			//退主线程
            g_phandle->atiny_quit = 1;
			g_lwm2m_recv_TskHandle = NULL;
			osThreadExit();
            return;
        }
		
        contextP = g_phandle->lwm2m_context;
		recv_buffer = g_phandle->recv_buffer;
        if(contextP == NULL || recv_buffer == NULL)
        {
            osDelay(50);
            continue;
        }
		

        dataP = (client_data_t *)(contextP->userData);
        if(dataP == NULL || dataP->connList == NULL)
        {
            osDelay(100);
            continue;
        }
		else
        	connP = dataP->connList;

        while (connP != NULL)
        {
#if WITH_MBEDTLS_SUPPORT  
			if(g_cdp_config_data->cdp_dtls_switch == 1
			&& connP->dtls_renegotiate_flag == 1)
            {
                cdp_dtls_wait_flag = 1;
                osDelay(500);
                continue;
            }
#endif
            //select Blocking waiting signal
            numBytes = lwm2m_buffer_recv(connP, recv_buffer, MAX_PACKET_SIZE, 1);
            if (numBytes <= 0)
            {
                if(numBytes == -2)
                    ATINY_LOG(LOG_INFO, "no packet arrived!");
                else if(numBytes == -1)
                {  	
                    osDelay(50);
                }
#if WITH_MBEDTLS_SUPPORT
                if(g_cdp_config_data->cdp_dtls_switch == 1)
                    osDelay(500);
#endif
            }
            else
            {
                atiny_recvdata_list_insert(recv_buffer, numBytes, connP);
                if(cdp_wait_sem != NULL)
                    osSemaphoreRelease(cdp_wait_sem);
            }
            connP = connP->next;
        }
    }
}

int atiny_data_report(void *phandle, data_report_t *report_data)
{
    lwm2m_uri_t uri;
    int ret;
    data_report_t data;


    if (NULL == phandle || NULL == report_data || report_data->len <= 0
            || report_data->len > MAX_REPORT_DATA_LEN || NULL == report_data->buf)
    {
        ATINY_LOG(LOG_ERR, "invalid args");
        return ATINY_ARG_INVALID;
    }

    memset((void *)&uri, 0, sizeof(uri));

    switch (report_data->type)
    {
    case FIRMWARE_UPDATE_STATE:
        (void)lwm2m_stringToUri("/5/0/3", 6, &uri);
        break;
    case APP_DATA:
        get_resource_uri(BINARY_APP_DATA_OBJECT_ID, 0, BINARY_APP_DATA_RES_ID, &uri);
        break;
    default:
        return ATINY_RESOURCE_NOT_FOUND;
    }

    memcpy(&data, report_data, sizeof(data_report_t));
    data.buf = (uint8_t *)lwm2m_malloc(report_data->len);
    if (NULL == data.buf)
    {
        ATINY_LOG(LOG_ERR, "lwm2m_malloc fail,len %d", data.len);
        return ATINY_MALLOC_FAILED;;
    }
    memcpy(data.buf, report_data->buf, report_data->len);

    ret = atiny_queue_rpt_data(&uri, &data);

    if (ATINY_OK != ret)
    {
        if (data.buf != NULL)
        {
            lwm2m_free(data.buf);
        }
    }

    return ret;
}

void app_data_report(void)
{
    uint8_t buf[5] = {0, 1, 6, 5, 9};
    data_report_t report_data;
    int ret = 0;
    static int cnt = 0;
    buffer_list_t *temp_node = NULL;
    report_data.buf = buf;
    report_data.cookie = 0;
    report_data.len = sizeof(buf);
    report_data.type = APP_DATA;
    (void)ret;

    if (upstream_info == NULL)
        return;

     osMutexAcquire(g_upstream_mutex, portMAX_DELAY);
     if (upstream_info->head != NULL)
     {
        if(upstream_info->head->type == cdp_NON || upstream_info->head->type == cdp_NON_RAI || upstream_info->head->type == cdp_NON_WAIT_REPLY_RAI)
            report_data.callback = NULL;
        else
            report_data.callback = cdp_upstream_ack_callback;
        report_data.buf = (uint8_t *)upstream_info->head->data;
        report_data.len = upstream_info->head->data_len;
        report_data.msg_type = upstream_info->head->type;
        report_data.cookie = upstream_info->head->seq_num;
        atiny_printf("app_data_report cookie:%d\n", report_data.cookie);
        ret = atiny_data_report(g_phandle, &report_data);
        temp_node = upstream_info->head->next;
        if (upstream_info->head->data != NULL)
            xy_free(upstream_info->head->data);
        xy_free(upstream_info->head);
        upstream_info->head = temp_node;
        if (upstream_info->head == NULL)
        {
            upstream_info->tail = NULL;
        }
		
#if VER_BC95
		if(report_data.cookie == 0 && (report_data.msg_type == cdp_NON || report_data.msg_type == cdp_NON_RAI || report_data.msg_type == cdp_NON_WAIT_REPLY_RAI))
			at_send_NSMI(0,0);
#endif
     }
     osMutexRelease(g_upstream_mutex);
}

#if WITH_MBEDTLS_SUPPORT 
static void atiny_handle_dtls_reconnect(handle_data_t *handle)
{
    lwm2m_context_t  *contextP = handle->lwm2m_context;

    if(contextP == NULL || contextP->serverList == NULL)
        return;

    connection_t *connp = (connection_t *)contextP->serverList->sessionH;

    if(connp == NULL)
        return;
        
	int shakehandle_state = ((mbedtls_ssl_context *)connp->net_context)->state;
    
   //握手成功以后才需要判断是否需要重新协商
	if(connp != NULL && g_cdp_config_data->cdp_dtls_nat_type == 0 && shakehandle_state == MBEDTLS_SSL_HANDSHAKE_OVER)
    {
        int32_t timeout = lwm2m_gettime()-((mbedtls_ssl_context *)connp->net_context)->session->start;

        if(timeout > 30)
            connp->dtls_renegotiate_flag = 1;
    }

    if(connp != NULL && connp->dtls_renegotiate_flag)
    {
        do{
            osDelay(100);
        }while(cdp_dtls_wait_flag == 0);
        
        xy_printf(0,XYAPP, WARN_LOG, "[king]lwm2m dtls reconnect");
        
        connection_free(connp);
        
        char *host = NULL;
        char *port = NULL;
        char *uri = NULL;
        
        security_instance_t *targetP = (security_instance_t *)LWM2M_LIST_FIND(connp->securityObj->instanceList, connp->securityInstId);
        if (NULL == targetP || targetP->uri == NULL)
        {
            goto fail;
        }

        uri = atiny_strdup(targetP->uri);
        if (uri == NULL)
        {
            ATINY_LOG(LOG_INFO, "atiny_strdup null!!!");
            goto fail;
        }
        
        if (connection_parse_host_ip(uri, &host, &port) != COAP_NO_ERROR)
        {
            goto fail;
        }

        if(connection_connect_dtls(connp, targetP, host, port, LWM2M_IS_CLIENT) != COAP_NO_ERROR)
            goto fail;
        
        connp->dtls_renegotiate_flag = false;
        osDelay(500);
        cdp_dtls_wait_flag = 0;

		if(uri)
			lwm2m_free(uri);
 
        return;
        
	fail:
		if(uri)
			lwm2m_free(uri);

        connp->dtls_renegotiate_flag = false;
		cdp_dtls_wait_flag = 0;
        cdp_register_fail_flag = 1;
        return;
    }
}
#endif

static void atiny_handle_reconnect(handle_data_t *handle)
{
    if(handle->reconnect_flag)
    {
        (void)lwm2m_reconnect(handle->lwm2m_context);
        handle->reconnect_flag = false;
        ATINY_LOG(LOG_INFO, "lwm2m reconnect");
    }
}

int atiny_reconnect(void *phandle)
{
    handle_data_t *handle = (handle_data_t *)phandle;

    if (NULL == phandle)
    {
        ATINY_LOG(LOG_FATAL, "Parameter null");
        return ATINY_ARG_INVALID;
    }
    handle->reconnect_flag = true;

    return ATINY_OK;
}

static void atiny_reboot_check(int reboot_flag)
{
    if(reboot_flag == 1)
    {
        (void)atiny_cmd_ioctl(ATINY_DO_DEV_REBOOT, NULL, 0);
    }
    
    if(reboot_flag == 2)
    {
        OTA_upgrade_start();
    }
}

//flag:1,正常重启;2,调用FOTA接口进行重启
void atiny_set_reboot_flag(int flag)
{
    if(g_phandle != NULL)
    {
        g_phandle->reboot_flag = flag;
    }
}

static int32_t atiny_get_waittime(int32_t regTime, int32_t lifetime, int32_t curTime)
{
	int32_t nextUpdate = 0;
	int32_t interval = 0;

    nextUpdate = CLOUD_LIFETIME_DELTA(lifetime);
    interval = (regTime + nextUpdate) - curTime;
	if(interval > 0)
		return interval;
	else
		return 0;

}

int atiny_bind(void *phandle)
{
    handle_data_t *handle = (handle_data_t *)phandle;
    time_t timeout;
    int ret;

	if(CDP_RECOVERY_FLAG_EXIST() && CDP_NEED_RECOVERY(g_cdp_session_info->cdp_lwm2m_event_status))
	{		
		if (0 != prv_refreshServerList(handle->lwm2m_context))
		{
			LOG("prv_refreshServerList fail");
			return COAP_503_SERVICE_UNAVAILABLE;
		}

		if(cdp_resume_session_info(handle->lwm2m_context))
        {
            atiny_mark_deinit(handle);
            xy_printf(0,XYAPP, WARN_LOG, "cdp_resume_session_info fail");
        }
	}

    while (!handle->atiny_quit)
    {
        timeout = BIND_TIMEOUT;
        if (handle->deregister_flag)
        {
            lwm2m_deregister(handle->lwm2m_context);
            handle->deregister_flag = 0;
        }
        else
        {
            app_data_report();
            (void)atiny_step_rpt(handle->lwm2m_context);
            atiny_handle_reconnect(handle);
        }
        
        //after deepsleep need to resume
        if(cdp_recovery_sem != NULL)
    	{
    		while (osSemaphoreAcquire(cdp_recovery_sem, 0) == osOK) {};
        	osSemaphoreRelease(cdp_recovery_sem);
    	}
        
   #if WITH_MBEDTLS_SUPPORT  
        if(g_cdp_config_data->cdp_dtls_switch == 1 && (handle->lwm2m_context->serverList != NULL && handle->lwm2m_context->serverList->status != XY_STATE_REG_FAILED)
			&& handle->lwm2m_context->state == STATE_READY)
            atiny_handle_dtls_reconnect(handle);
   #endif     

        (void)lwm2m_step(handle->lwm2m_context, &timeout);
        
        atiny_recvdata_proc(handle->lwm2m_context);
        
        atiny_reboot_check(handle->reboot_flag);

        osDelay(10);

        //if cdp take fota,it will not wait sem
        ret = atiny_fota_manager_get_rpt_state(atiny_fota_manager_get_instance()) +
            atiny_fota_manager_get_state(atiny_fota_manager_get_instance());
        if(ret != ATINY_FOTA_IDLE)
            continue;
        
        //Block all other times except to register, deregister and uplink data
        if((handle->lwm2m_context->serverList->status == XY_STATE_REGISTERED)  
           && (handle->lwm2m_context->state == STATE_READY) && (!get_upstream_message_pending_num()))
        {
            if(cdp_wait_sem == NULL)
                cdp_wait_sem = osSemaphoreNew(0xFFFF, 0, NULL);
			int waittime = atiny_get_waittime(handle->lwm2m_context->serverList->registration, 
						handle->lwm2m_context->serverList->lifetime, lwm2m_gettime());

			if(waittime > 0)
			{
	            osSemaphoreAcquire(cdp_wait_sem, waittime * 1000);	
			}
        }
            
    }

    return ATINY_OK;
}

int atiny_mark_deregister(void* phandle)
{
    handle_data_t* handle = (handle_data_t*)phandle;
    lwm2m_context_t *context = handle->lwm2m_context;

    if (context->state == STATE_BOOTSTRAP_REQUIRED
               || context->state == STATE_BOOTSTRAPPING
               || context->state == STATE_REGISTER_REQUIRED
               || context->state == STATE_REGISTERING
			   || phandle == NULL)
    {
    	return 1;
    }

    //CDP business task loop exit flag
    handle->deregister_flag = 1;
    if(cdp_wait_sem == NULL)
        cdp_wait_sem = osSemaphoreNew(0xFFFF, 0, NULL);
    osSemaphoreRelease(cdp_wait_sem);           
    return 0;
}

void atiny_mark_deinit(void* phandle)
{
    handle_data_t* handle;

    if (phandle == NULL)
    {
        return;
    }

	handle = (handle_data_t*)phandle;
    handle->atiny_quit = 1;

	if(g_lwm2m_recv_TskHandle != NULL)
 	{
	 	osThreadTerminate(g_lwm2m_recv_TskHandle);
		g_lwm2m_recv_TskHandle = NULL;
 	}
}

#endif

