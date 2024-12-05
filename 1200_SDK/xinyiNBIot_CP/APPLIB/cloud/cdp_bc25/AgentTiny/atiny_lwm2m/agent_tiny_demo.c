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
#include "agent_tiny_demo.h"
#include "agent_tiny_cmd_ioctl.h"
#include "liblwm2m.h"
#include "object_comm.h"
#include "cdp_backup.h"
#include "atiny_fota_state.h"
#include "atiny_context.h"
#include "at_cdp.h"
#include "softap_nv.h"
#include "xy_system.h"
#include "oss_nv.h"
#include "low_power.h"
#include "net_app_resume.h"
#include "main_proxy.h"
#include "xy_cdp_api.h"
#include "xy_net_api.h"
#include "atiny_osdep.h"
#include "lwip/ip_addr.h"
#include "xy_atc_interface.h"
#include "atiny_socket.h"
#include "cdp_init.h"
#include "agenttiny.h"

#ifdef CONFIG_FEATURE_FOTA
#include "ota_flag.h"
#include "xy_fota.h"
#endif
#if WITH_MBEDTLS_SUPPORT
#include "mbedtls/ssl.h"
#endif

osMutexId_t g_cdp_module_init_mutex= NULL;

extern osSemaphoreId_t cdp_recovery_sem;
extern osSemaphoreId_t  cdp_wait_sem;
extern osSemaphoreId_t cdp_api_sendasyn_sem;

extern int g_send_status;
extern cdp_config_nvm_t *g_cdp_config_data;

// BC25对标 电信入库需求 重启后不保存
int g_binding_mode = 0;
int g_bc25_update = 0 ; 


//CDP全局信息结构体指针
handle_data_t *g_phandle = NULL;
upstream_info_t *upstream_info = NULL;
downstream_info_t *downstream_info = NULL;
//下行数据接收链表
recvdate_info_t *recvdata_info = NULL;

osMutexId_t g_upstream_mutex = NULL;
osMutexId_t g_downstream_mutex = NULL;
//下行数据接收锁
osMutexId_t g_recvdata_mutex = NULL;

osThreadId_t g_lwm2m_TskHandle = NULL;
osThreadId_t g_lwm2m_recv_TskHandle = NULL;

int cdp_deregister_flag = 0;
int cdp_register_fail_flag = 0;

//AT指令发送数据时携带的全局seq_num,将该值带入到cdp发送接口之前
static uint8_t g_seq_num;
//AT指令发送数据时携带的全局rai类型,将该值带入到cdp发送接口之前
static uint8_t  g_raiflag;
//AT+QLWULDATAEX指令特有标志位，用于区别其他发送指令发送con包时的状态上报
static uint8_t  g_conEX_send_flag = 0;
//AT+QLWULDATAEX指令特有标志位，用于发送con时的状态上报携带的当前seq_num值
static uint8_t  g_cur_seq_num = 0;

// extern char *g_Remote_AT_Rsp;
// extern int xy_Remote_AT_Req(char *req_at);

/************判断当前CDP主业务线程是否创建******************/
bool cdp_handle_exist()
{
	if(g_lwm2m_TskHandle != NULL)
		return 1;
	else
		return 0;
}

/************判断当前CDP运行状态以及数据发送状态************/
int is_cdp_running()
{
    if (cdp_handle_exist() && (cdp_deregister_flag == 0 && cdp_register_fail_flag == 0) && g_phandle != NULL && g_phandle->lwm2m_context != NULL
        && g_phandle->lwm2m_context->state > STATE_REGISTERING)
        return 1;
    else
        return 0;
}

bool is_cdp_upstream_ok()
{
	lwm2m_observed_t *targetP = NULL;
	lwm2m_context_t *contextP = NULL;
	if(g_phandle != NULL)
	{
		contextP = g_phandle->lwm2m_context;
		if(contextP == NULL)
			return 0;
		
		for (targetP = contextP->observedList ; targetP != NULL ; targetP = targetP->next)
		{
			if (!dm_isUriOpaqueHandle(&(targetP->uri)))
				continue;
			return 1;
		}
	}
	return 0;
}

/************对标BC95的一些全局初始化************/
void cdp_con_flag_init(int type, uint16_t seq_num)
{
	if(type != cdp_NON && type != cdp_NON_RAI && type != cdp_NON_WAIT_REPLY_RAI)
	{
		cdp_set_cur_seq_num(seq_num);
		g_conEX_send_flag = 1;
	}
}

void cdp_con_flag_deint()
{
	g_conEX_send_flag = 0;
}

int cdp_get_con_send_flag()
{
	return g_conEX_send_flag;
}


void cdp_set_seq_and_rai(uint8_t raiflag,uint8_t seq_num)
{
	g_raiflag = raiflag;
	g_seq_num = seq_num;
}


void cdp_get_seq_and_rai(uint8_t* raiflag,uint8_t* seq_num)
{
	*raiflag =  g_raiflag;
	*seq_num =  g_seq_num;
}

uint8_t cdp_get_seq_num()
{
	return g_seq_num;
}

void cdp_set_cur_seq_num(uint8_t seq_num)
{
	g_cur_seq_num = seq_num;
}
uint8_t cdp_get_cur_seq_num()
{
	return g_cur_seq_num;
}

void cdp_QLWULDATASTATUS_report(uint8_t seq_num)
{
	if(cdp_get_con_send_flag())
	{
		cdp_con_flag_deint();
	}
}


/**********根据CDP状态释放CDP API信号量**********/
void cdp_api_sem_give(int code)
{
    osSemaphoreId_t temp_sem = NULL;

    switch(code)
    {
    case XY_STATE_REG_FAILED:
    case XY_STATE_REGISTERED:
        temp_sem = cdp_api_register_sem;
        break;
    case XY_STATE_DEREGISTERED:
        temp_sem = cdp_api_deregister_sem;
        break;
     case XY_STATE_UPDATE_DONE:
        temp_sem = cdp_api_update_sem;
        break;
     default:
        break;
    }

    if(temp_sem != NULL)
    {
        while (osSemaphoreAcquire(temp_sem, 0) == osOK) {};
        osSemaphoreRelease(temp_sem);
    }

    return;
}

/**********lwm2m 事件设置及事件处理**********/
void cdp_set_report_event(xy_module_type_t type,const char *arg, int code)
{
	switch(type)
	{
		case MODULE_LWM2M:
		{
			if(code == XY_STATE_REGISTERED)
				g_cdp_session_info->cdp_lwm2m_event_status = 0;//注册完成
			else if(code == XY_STATE_UPDATE_DONE)
				g_cdp_session_info->cdp_lwm2m_event_status = 2;//更新完成
			else if(code == XY_STATE_BS_PENDING)
				g_cdp_session_info->cdp_lwm2m_event_status = 4;//Bootstrap完成
			else if(code == XY_RECV_UPDATE_PKG_URL_NEEDED)
				g_cdp_session_info->cdp_lwm2m_event_status = 6;//通知设备接收更新包url
			else if(code == XY_DOWNLOAD_COMPLETED)
				g_cdp_session_info->cdp_lwm2m_event_status = 7;//通知设备下载完成
			else if(code == XY_STATE_DEREGISTERED)
				g_cdp_session_info->cdp_lwm2m_event_status = 1;//去注册完成
			else if(code == XY_STATE_REG_FAILED)
			{
				if(g_cdp_session_info->cdp_lwm2m_event_status != 10)
					g_cdp_session_info->cdp_lwm2m_event_status = 8;//注册失败
			}
			else if(code == FORBIDDEN_4_03)
				g_cdp_session_info->cdp_lwm2m_event_status = 10;//服务器拒绝
			break;
		}
		
		case MODULE_URI:
		{
			if(code == OBSERVE_SUBSCRIBE)
			{
				if(dm_isUriOpaqueHandle((lwm2m_uri_t *)arg))
					g_cdp_session_info->cdp_lwm2m_event_status = 3;//订阅对象19/0/0完成
				else if (((lwm2m_uri_t*)arg)->objectId == 5 && ((lwm2m_uri_t*)arg)->instanceId == 0 && ((lwm2m_uri_t*)arg)->resourceId == 3)
					g_cdp_session_info->cdp_lwm2m_event_status = 5;//5/0/3资源订阅完成
			}
			else if(code == OBSERVE_UNSUBSCRIBE)
			{
				if(dm_isUriOpaqueHandle((lwm2m_uri_t *)arg))
					g_cdp_session_info->cdp_lwm2m_event_status = 9; //取消订阅对象19/0/0
			}
			break;
		}
		default:
			break;
	}

	//dtls握手成功失败不写文件系统
	if(type == MODULE_NET)
		return;
	
	cloud_save_file(CDP_SESSION_NVM_FILE_NAME,(void*)g_cdp_session_info,sizeof(cdp_session_info_t));
	//设置恢复标志位，表示文件系统可用，下次深睡恢复读文件系统里的值；
	CDP_SET_RECOVERY_FLAG();
}

void cdp_lwm2m_event_handle(xy_module_type_t type,  int code, const char *arg)
{
	if(type == MODULE_LWM2M)
	{	
		cdp_api_sem_give(code);

		//根据当前lwm2m event值，进行resume 和RTC相关操作
    	cdp_resume_state_process(code);

#ifdef CONFIG_FEATURE_FOTA
		if(code == XY_STATE_REGISTERED)
		{
        	(void)atiny_fota_manager_repot_result(atiny_fota_manager_get_instance());
		}
#endif
	}
	
    //此时终端和服务器状态切换完成处于稳态，保存相关会话配置，DFOTA 保存/5/0/3 observe 用于深睡唤醒后fota下载
	if((type == MODULE_LWM2M && code == XY_STATE_UPDATE_DONE)|| (type == MODULE_URI && (code == OBSERVE_SUBSCRIBE) && dm_isUriOpaqueHandle((lwm2m_uri_t *)arg))|| (type == MODULE_LWM2M && code == XY_RECV_UPDATE_PKG_URL_NEEDED) || (type == MODULE_LWM2M && code == XY_DOWNLOAD_COMPLETED))
	{ 
            cdp_bak_session_info(g_phandle->lwm2m_context);
            cloud_save_file(CDP_SESSION_NVM_FILE_NAME,(void*)g_cdp_session_info,sizeof(cdp_session_info_t));

#ifdef CONFIG_FEATURE_FOTA
			//DFOTA 状态信息保存
			ota_flag_write(FLAG_INVALID, NULL, 0, code);
#endif
	}
	
	//设置当前event值
	cdp_set_report_event(type, arg, code);
}


/**********供AT命令以及cdp api调用的子接口**********/
char *get_cdp_server_ip_addr_str()
{
    return (char *)g_cdp_config_data->cloud_server_ip;
}

int set_cdp_server_ip_addr_str(char *ip_addr_str)
{
	ip_addr_t addr = {0};
	uint8_t netif_iptype = 0;
	if(ipaddr_aton(ip_addr_str, &addr)) //1:ip,0:domain
	{
		netif_iptype = xy_get_netif_iptype(); //获取当前网络类型
		//如果当前网络是单栈V4但是入参传入的是V6地址，或者当前网络是单栈v6但是入参传入的是v4地址，或者ip地址非法 则返回错误。
		if((xy_IpAddr_Check(ip_addr_str, addr.type) == 0) || (netif_iptype == IPV4_TYPE && addr.type != IPADDR_TYPE_V4) || (netif_iptype == IPV6_TYPE && addr.type != IPADDR_TYPE_V6))
			return XY_ERR;
	}
	else if(xy_domain_is_valid(ip_addr_str) == 0) //域名检测
	{
		xy_printf(0,XYAPP, WARN_LOG, "xy_domain_is_valid err");
		return XY_ERR;
	}
		
	if(strcmp(g_cdp_config_data->cloud_server_ip,ip_addr_str))
	{	
		memset(g_cdp_config_data->cloud_server_ip, 0x00, sizeof(g_cdp_config_data->cloud_server_ip));
		memcpy(g_cdp_config_data->cloud_server_ip, ip_addr_str, strlen(ip_addr_str));
	}    
    return XY_OK;
}

uint16_t get_cdp_server_port()
{
    return g_cdp_config_data->cloud_server_port;
}

int set_cdp_server_port(uint16_t port)
{
    if(g_cdp_config_data->cloud_server_port != port)
    {
        g_cdp_config_data->cloud_server_port = port;
    }
    return 0;
}

int set_cdp_server_settings(char *ip_addr_str, uint16_t port)
{
    //todo: check if we are allowed to configure cdp server settings
    if(ip_addr_str != NULL && set_cdp_server_ip_addr_str(ip_addr_str) < 0)
    	return XY_ERR;

    if (set_cdp_server_port(port) < 0)
        return XY_ERR;
	cloud_save_file(CDP_CONFIG_NVM_FILE_NAME,(void*)g_cdp_config_data,sizeof(cdp_config_nvm_t));
    return XY_OK;
}

// cdp relative, send message via lwm2m 19/0/0
int send_message_via_lwm2m(char *data, int data_len, cdp_msg_type_e type, uint8_t seq_num)
{
    buffer_list_t *node = NULL;
    if (data == NULL || data_len == 0)
        return 0;

    osMutexAcquire(g_upstream_mutex, portMAX_DELAY);

    if(upstream_info->pending_num >= DATALIST_MAX_LEN)
        goto failed;

    node = (buffer_list_t*)xy_malloc(sizeof(buffer_list_t));
    if (node == NULL)
    {
        goto failed;
    }
	memset(node, 0x00, sizeof(buffer_list_t));
    
    node->data = (char*)xy_malloc2(data_len);
    if (node->data == NULL)
    {
        goto failed;
    }
    memcpy(node->data, data, data_len);
    node->data_len = data_len;
    node->type = type;
	node->seq_num = seq_num;
    if (upstream_info->tail == NULL)
    {
        upstream_info->head = node;
        upstream_info->tail = node;
    }
    else
    {
        upstream_info->tail->next = node;
        upstream_info->tail = node;
    }
    
    if(type == cdp_NON || type == cdp_NON_RAI || type == cdp_NON_WAIT_REPLY_RAI)
    {
        if(seq_num == 0)
    	{
        	upstream_info->sent_num++;
    	}
		else
			upstream_info->pending_num++;
    }
	else
	{
        upstream_info->pending_num++;
        g_send_status = 1;
	}
    osMutexRelease(g_upstream_mutex);
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
    upstream_info->error_num++;
    osMutexRelease(g_upstream_mutex);
    return -1;
}

//return the oldest buffered data, data length returned by output argument
char *get_message_via_lwm2m(int *data_len)
{
    buffer_list_t *node;
    char *data = NULL;
	if(downstream_info == NULL)
		return NULL;
    osMutexAcquire(g_downstream_mutex, portMAX_DELAY);
    if (downstream_info->buffered_num > 0)
    {
        node = downstream_info->head;
        downstream_info->head = downstream_info->head->next;
        downstream_info->buffered_num--;
		g_softap_var_nv->cdp_buffered_num--;
        if (downstream_info->buffered_num == 0)
        {
            downstream_info->tail = NULL;
        }
        *data_len = node->data_len;
        data = node->data;
        xy_free(node);
		
        osMutexRelease(g_downstream_mutex);
        return data;
    }
    osMutexRelease(g_downstream_mutex);
    return NULL;
}

int new_message_indication(char *data, int data_len)
{
    buffer_list_t *node = NULL;
    char *rsp_cmd = NULL;

    //opencpu与AT隔离
    if(g_cdp_downstream_callback != NULL)
    {
    	g_cdp_downstream_callback(data, data_len);
		return 0;
    }
    osMutexAcquire(g_downstream_mutex, portMAX_DELAY);

	downstream_info->received_num++;
	g_softap_var_nv->cdp_received_num++;

    if (g_cdp_session_info->cdp_nnmi == 0 || g_cdp_session_info->cdp_nnmi == 2) // 0:no indications, buffer the data, 2:indications only
    {
        if( (get_downstream_message_total() + data_len) <= CDP_DOWNSTREAM_MAX_LEN ) //最多缓存2k的数据
        {
			//BC25对标：只有当本地Buffer为空时才会上报+NNMI
			if(get_downstream_message_total() == 0 && g_cdp_session_info->cdp_nnmi == 2)
			{
		        rsp_cmd = (char*)xy_malloc(40);
	            snprintf(rsp_cmd, 40, "+NNMI");
	            send_urc_to_ext(rsp_cmd, strlen(rsp_cmd));
			}


    		node = (buffer_list_t*)xy_malloc2(sizeof(buffer_list_t));
	        if (node == NULL)
	        {
	            goto failed;
	        }
	        memset(node, 0x00, sizeof(buffer_list_t));
	        node->data = (char*)xy_malloc2(data_len);
	        if (node->data == NULL)
	        {
	            goto failed;
	        }
	        memset(node->data, 0x00, data_len);
	        memcpy(node->data, data, data_len);
	        node->data_len = data_len;
	        if (downstream_info->tail == NULL)
	        {
	            downstream_info->head = node;
	            downstream_info->tail = node;
	        }
	        else
	        {
	            downstream_info->tail->next = node;
	            downstream_info->tail = node;
	        }
	        downstream_info->buffered_num++;
			g_softap_var_nv->cdp_buffered_num++;
    	}
		else
		{
			rsp_cmd = (char*)xy_malloc(40);
			snprintf(rsp_cmd, 40, "+NNMI: OC\"recv\",buff full");//BC25对标,缓存满时的上报
			send_urc_to_ext(rsp_cmd, strlen(rsp_cmd));
			downstream_info->dropped_num++;
			g_softap_var_nv->cdp_dropped_num++;
		}
    }
    else if (g_cdp_session_info->cdp_nnmi == 1)
    {
        rsp_cmd = (char*)xy_malloc2(data_len*2+28);
        if (rsp_cmd == NULL)
        {
            //OutputTraceMessage(1, "malloc failed!");
            goto failed;
        }
        snprintf(rsp_cmd, 128, "+NNMI: %d,",data_len);
        bytes2hexstr((unsigned char *)data, data_len, rsp_cmd + strlen(rsp_cmd), data_len*2+1);
        send_urc_to_ext(rsp_cmd, strlen(rsp_cmd));
    }


    if (rsp_cmd != NULL)
	{
		xy_free(rsp_cmd);
		rsp_cmd = NULL;
	}


    osMutexRelease(g_downstream_mutex);
    return 0;
    
failed:
    if (node != NULL)
    {
        if (node->data != NULL)
        {
            xy_free(node->data);
            node->data = NULL;
        }
        xy_free(node);
        node = NULL;
    }
    if (rsp_cmd != NULL)
	{
		xy_free(rsp_cmd);
		rsp_cmd = NULL;
	}
    osMutexRelease(g_downstream_mutex);
    return -1;
}

int get_upstream_message_pending_num()
{
    return upstream_info->pending_num;
}

int get_upstream_message_sent_num()
{
    return upstream_info->sent_num;
}

int get_upstream_message_error_num()
{
    return upstream_info->error_num;
}

int get_downstream_message_buffered_num()
{
    return g_softap_var_nv->cdp_buffered_num;
}

int get_downstream_message_received_num()
{
    return g_softap_var_nv->cdp_received_num;
}

int get_downstream_message_dropped_num()
{
    return g_softap_var_nv->cdp_dropped_num;
}

int get_downstream_message_total()
{
	int total = 0;
	buffer_list_t *p = downstream_info->head;
	
	while(p != NULL)
	{
		total += p->data_len;
		p = p->next;
	}

	return total;
}

int cdp_update_proc(xy_lwm2m_server_t *targetP)
{
	if(targetP->status != XY_STATE_REGISTERED)
		return XY_ERR;
	
	targetP->status = XY_STATE_REG_UPDATE_NEEDED;

	if(cdp_wait_sem != NULL)
	{
        osSemaphoreRelease(cdp_wait_sem);  
	} 

    return 0;
}

int cdp_update_proc_bc25(xy_lwm2m_server_t *targetP, int bind_mode)
{
	targetP->status = XY_STATE_REG_UPDATE_NEEDED;
	g_binding_mode = bind_mode;
	g_bc25_update = 1;

	if(cdp_wait_sem != NULL)
	{
        osSemaphoreRelease(cdp_wait_sem);  
	} 
    return 0;
}

/**********FOTA升级完成后自动连云相关接口**********/
#ifdef CONFIG_FEATURE_FOTA
/*cdp公有云FOTA升级结果上报，CDP的FOTA相关信息设置*/
int cdp_fota_init(void)
{
	upgrade_state_e state = OTA_IDLE;
	
	flag_get_info(NULL, &state);
	if(state != OTA_NEED_UPGRADE)
	{
		return XY_OK;
	}
	
	int ret = OTA_get_upgrade_result(); 
    if(ret == XY_OK)
    {
		flag_upgrade_set_result(OTA_SUCCEED);
        send_urc_to_ext("+QIND: \"FOTA\",\"END\",0", strlen("+QIND: \"FOTA\",\"END\",0"));
		// ota_state_callback(4);   
		//ota_set_state(XY_FOTA_UPGRADE_SUCCESS); 
    }
    else
    {
		flag_upgrade_set_result(OTA_FAILED);
		//send_urc_to_ext("\r\nFIRMWARE UPDATE FAILED\r\n", strlen("\r\nFIRMWARE UPDATE FAILED\r\n"));
		// ota_state_callback(5);
		//ota_set_state(XY_FOTA_UPGRADE_FAIL); 
    }

	//升级完后清除config中的fota_flag标志位
    g_cdp_config_data->cdp_fota_flag = 0;
    cloud_save_file(CDP_CONFIG_NVM_FILE_NAME, (void*)g_cdp_config_data, sizeof(cdp_config_nvm_t));

	return XY_OK;
}

bool cdp_check_if_need_fota(void)
{
    bool ret = false;

    cdp_config_nvm_t *temp_params = xy_malloc(sizeof(cdp_config_nvm_t));

    if((cloud_read_file(CDP_CONFIG_NVM_FILE_NAME, (void*)temp_params, sizeof(cdp_config_nvm_t)) == XY_OK) && temp_params->cdp_fota_flag == 1)
    {
        ret = true;
    }

    xy_free(temp_params);

    return ret;
}

int cdp_fota_autoreg_info_recover()
{
	upgrade_state_e state = OTA_IDLE;
	ota_flag_init();
	flag_get_info(NULL, &state);
	//此处添加此判断是为了确认此次FOTA是cdp的fota，否则不走自注册流程
	if(state != OTA_NEED_UPGRADE)
	{
		ota_flag_destroy();
		return XY_ERR;
	}
	
	cdp_module_init();
	if(cdp_create_lwm2m_task(OTA_NEED_UPGRADE))
	{
		xy_printf(0,XYAPP, WARN_LOG, " cdp_fota_autoreg_info_recover cdp_create_lwm2m_task fail");
    	return XY_ERR;
	}
    xy_printf(0,XYAPP, WARN_LOG, "cdp_fota_autoreg_info_recover, over");
    return XY_OK;
}
#endif


/**********上电后pdp激活后相关接口处理**********/
void cdp_netif_event_callback(PsStateChangeEvent event)
{
	osThreadAttr_t thread_attr = {0};

    xy_printf(0,XYAPP, WARN_LOG, "cdp_netif_event_callback, netif up");

#ifdef CONFIG_FEATURE_FOTA
	if(!cdp_handle_exist() && cdp_check_if_need_fota() && cdp_fota_autoreg_info_recover() == XY_OK)
	{
		return;
	}
#endif
	//BC25对标：开启恢复功能需要上报QLWEVTIND:6
	if(Is_WakeUp_From_Dsleep() && CDP_RECOVERY_FLAG_EXIST())
	{
		static uint8_t recover_notifyed = 0;
		if(!recover_notifyed)
		{
			cdp_module_init();
			if(CDP_NEED_RECOVERY(g_cdp_session_info->cdp_lwm2m_event_status))
				atiny_event_notify(ATINY_BC25_RECOVERY, NULL, 0);
			recover_notifyed = 1;
			return;
		}	
	}

    //入库模式下需判断卡类型，若是无效卡则不发起自注册
    if(cdp_storage_uicc_isvalid() && g_softap_fac_nv->cdp_register_mode == 1 && !cdp_handle_exist() && g_cdp_resume_TskHandle == NULL)
    {
        thread_attr.name	   = "cdp_attach_resume";
        thread_attr.priority   = osPriorityNormal1;
		thread_attr.stack_size = osStackShared;
        g_cdp_resume_TskHandle = osThreadNew((osThreadFunc_t)(cdp_attach_resume_process), NULL, &thread_attr);
        return;
    }
#if VER_BC95
	else if(g_softap_fac_nv->cdp_register_mode == 0 && !cdp_handle_exist())
	{
		static uint8_t register_notify = 0;
		if(!register_notify)
		{
			thread_attr.name	   = "cdp_reg_notify";
			thread_attr.priority   = osPriorityNormal1;
			thread_attr.stack_size = osStackShared;
			osThreadNew((osThreadFunc_t)(cdp_reg_notify_process), NULL, &thread_attr);
			register_notify = 1;
			return;
		}	
	}
#endif
	else if(g_softap_fac_nv->cdp_register_mode == 2)
		return;

	//保活模式，IP变化触发重新注册，或者19/0/0 observe cancel 上报event9 重新注册
	if(g_cdp_resume_TskHandle == NULL && !Is_OpenCpu_Ver() && (CDP_RECOVERY_FLAG_EXIST() || is_cdp_running()))
	{
        thread_attr.name = "cdp_resume";
        thread_attr.priority = osPriorityNormal1;
        thread_attr.stack_size = osStackShared;
        g_cdp_resume_TskHandle = osThreadNew((osThreadFunc_t)(cdp_netif_up_resume_process), (void*)IP_IS_CHANGED, &thread_attr);
	}
}

/**********上行数据发送状态回调**********/
void cdp_upstream_ack_callback(atiny_report_type_e type, int cookie, data_send_status_e status, int mid)
{
    atiny_printf("type:%d cookie:%d status:%d\n", type,cookie, status);
	
	//cdp_upstream_seq_callback 回调时带seq_num con包需要过滤掉
	if(mid == -1 && cdp_check_con_node(cookie))
		return;
		
	
	if(NULL != g_cdp_send_asyn_ack && mid != -1)
		g_cdp_send_asyn_ack(mid, status);

	osMutexAcquire(g_upstream_mutex, portMAX_DELAY);
	if (status == SENT_SUCCESS)
	{
		if(mid != -1) //过滤掉NON 带seq_num 的上报
		{
			if(g_send_status == 5) //5(平台主动取消订阅/19/0/0)
			{
				//at_send_NSMI(3,cookie);
				upstream_info->sent_num--;//
				upstream_info->error_num++;
			}
			else
			{
				g_send_status = 4;	
				//是为了区别SENT和空口上报只能选其一,不带seq_num上报SENT,带seq_num走cdp_upstream_seq_callback上报
				if(cookie == 0)
					at_send_NSMI(0,cookie);
			}
		}
		
		upstream_info->sent_num++;
		upstream_info->pending_num--;
	}
	else if (status == SENT_TIME_OUT)
	{
		if(mid != -1) 
		{
			g_send_status = 3;//只判断CON发送状态
		}
		at_send_NSMI(1,cookie);//对标BC95,NON包带seq_num失败也要上报
		upstream_info->error_num++;
		upstream_info->pending_num--;
	}
	else if (status == SENT_FAIL)
	{
		if(mid != -1) //过滤掉NON 带seq_num 的上报
		{
			g_send_status = 2;
			at_send_NSMI(2,cookie);
		}
		
		upstream_info->error_num++;
		upstream_info->pending_num--;
	}
	else if (status == SENT_GET_RST)
	{
		upstream_info->error_num++;
		upstream_info->pending_num--;
	}
	cdp_QLWULDATASTATUS_report(cookie); //EX命令的主动上报
	osMutexRelease(g_upstream_mutex);

}

//带seq_num发送到空口的回调
void cdp_upstream_seq_callback(unsigned long eventId, void *param, int paramLen)
{
	xy_assert(paramLen == sizeof(ATC_MSG_IPSN_IND_STRU));
	
	unsigned short seq_soc_num = 0;
	char send_status = 0;
	unsigned short socket_fd = 0;
	unsigned short seq_num = 0;
	int soc_ctx_id  = -1;
	
	if(g_phandle == NULL)
		return;
	
	ATC_MSG_IPSN_IND_STRU *ipsn_urc = (ATC_MSG_IPSN_IND_STRU*)param;

	seq_soc_num = ipsn_urc->usIpSn;
	send_status = ipsn_urc->ucStatus;

	socket_fd = (unsigned short)((seq_soc_num & 0XFF00) >> 8);
	seq_num = (unsigned short)(seq_soc_num & 0X00FF);
	
	if (seq_num > CDP_SEQUENCE_MAX || seq_num <= 0)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[cdp_upstream_seq_callback] unexpected seq_num:%d",seq_num);
		return;
	}

	if (!cdp_find_match_udp_node(seq_num))
	{
		xy_printf(0,XYAPP, WARN_LOG, "find no match udp mode!!!");
		return;
	}

#if WITH_MBEDTLS_SUPPORT
	if(g_cdp_config_data->cdp_dtls_switch == 1)
	{
		connection_t  *connList = g_phandle->client_data.connList;
		if(connList == NULL)
			return;
		mbedtls_ssl_context *ssl = (mbedtls_ssl_context *)connList->net_context;
		if(ssl == NULL || ssl->p_bio == NULL)
			return;
		soc_ctx_id = *(int *)ssl->p_bio;
	}
	else
#endif
	{
		if(g_phandle->client_data.connList == NULL)
			return;
		soc_ctx_id  = *(int *)(g_phandle->client_data.connList->net_context);
	}

	//socket context proc
	if (socket_fd != soc_ctx_id)
		return;

	char *rsp_cmd = xy_malloc(40);
	if(send_status == CDP_SEND_STATUS_SUCCESS)
	{
		cdp_upstream_ack_callback(APP_DATA, seq_num, SENT_SUCCESS , -1);
		if(g_cdp_session_info->cdp_nsmi == 1)
		{	
			sprintf(rsp_cmd,"+NSMI:SENT_TO_AIR_INTERFACE,%d",seq_num);
			send_urc_to_ext(rsp_cmd, strlen(rsp_cmd));
		}
	}
	else if(send_status == CDP_SEND_STATUS_FAILED)
	{
		cdp_upstream_ack_callback(APP_DATA, seq_num, SENT_TIME_OUT, -1);
	}
	
	//回调成功后，需要从cdp_sn_info节点中删掉已使用的seq_num
	cdp_del_sninfo_node(seq_num);
	xy_free(rsp_cmd);
}

/**********cdp 业务主流程处理函数**********/
int cdp_get_params(handle_data_t* handle)
{
	atiny_param_t *atiny_params = xy_malloc(sizeof(atiny_param_t));
	memset(atiny_params, 0x00, sizeof(atiny_param_t));
	atiny_security_param_t  *iot_security_param = NULL;
    atiny_security_param_t  *bs_security_param = NULL;
	
    atiny_params->server_params.binding = "U";        //UDP model
    if(g_binding_mode == 1)
		atiny_params->server_params.binding = "UQ";        //UDP queue 模式

	atiny_params->server_params.life_time = handle->lifetime;
    atiny_params->server_params.storing_cnt = 0;

    atiny_params->server_params.bootstrap_mode = BOOTSTRAP_FACTORY;
    atiny_params->server_params.hold_off_time = 10;

    //pay attention: index 0 for iot server, index 1 for bootstrap server.
    iot_security_param = &(atiny_params->security_params[0]);
    bs_security_param = &(atiny_params->security_params[1]);

	iot_security_param->server_ip = xy_malloc(XY_IPADDR_STRLEN_MAX + 1);
	memset(iot_security_param->server_ip, 0x00, XY_IPADDR_STRLEN_MAX + 1);
	if(memcmp(&g_cdp_session_info->net_info.remote_ip, iot_security_param->server_ip, sizeof(g_cdp_session_info->net_info.remote_ip)) &&
		CDP_NEED_RECOVERY(g_cdp_session_info->cdp_lwm2m_event_status))
		strcpy(iot_security_param->server_ip,ipaddr_ntoa(&g_cdp_session_info->net_info.remote_ip));
	else
		strcpy(iot_security_param->server_ip,g_cdp_config_data->cloud_server_ip);
	
	bs_security_param->server_ip  = iot_security_param->server_ip;

	iot_security_param->server_port = xy_malloc(6);
	snprintf(iot_security_param->server_port, 6, "%d", g_cdp_config_data->cloud_server_port);
    bs_security_param->server_port = iot_security_param->server_port;

	if (g_cdp_config_data->cdp_dtls_switch == 1)
	{
		iot_security_param->psk_Id = g_cdp_config_data->cdp_pskid;
		bs_security_param->psk_Id = iot_security_param->psk_Id;
		
		iot_security_param->psk = g_cdp_config_data->cloud_server_auth;
		bs_security_param->psk = iot_security_param->psk;
		
		iot_security_param->psk_len = 16;
		bs_security_param->psk_len = 16;
	}

	handle->atiny_params = atiny_params;

	return XY_OK;	
}

void *cdp_get_device_info()
{
	atiny_device_info_t *device_info = xy_malloc(sizeof(atiny_device_info_t));

	if(NULL == device_info)
    {
        return NULL;
    }
	memset(device_info, 0x00, sizeof(atiny_device_info_t));
	if(cdp_get_endpoint_name() == NULL)
	{
		xy_free(device_info);
		xy_printf(0,XYAPP, WARN_LOG, "cdp_get_endpoint_name failed!!!");
		return NULL;
	}

	device_info->endpoint_name = xy_malloc(strlen(g_cdp_session_info->endpointname)+1);
	memcpy(device_info->endpoint_name, g_cdp_session_info->endpointname, strlen(g_cdp_session_info->endpointname)+1);

#ifdef CONFIG_FEATURE_FOTA
    device_info->manufacturer = "Lwm2mFota";
    device_info->dev_type = "Lwm2mFota";
#else
    device_info->manufacturer = "Agent_Tiny";
#endif

	return device_info;
}

int cdp_init(int lifetime)
{
    if ((strcmp((const char *)g_cdp_config_data->cloud_server_ip, "") == 0 )
        || g_cdp_config_data->cloud_server_port == 0 || g_cdp_config_data->cloud_server_port > 65535)
    {
        goto failed;
    }

	if(g_cdp_config_data->cdp_dtls_switch == 1 && (!strcmp((const char *)g_cdp_config_data->cdp_pskid, "") 
		|| !strcmp((const char *)g_cdp_config_data->cloud_server_auth, "")))
		goto failed;

	
	//移远需求在销毁cdp业务后，会话全局在退出会销毁置NULL;重启cdp业务需要重新给会话全局赋值
	cdp_module_init();
	
	if(g_phandle == NULL)
	{
		handle_data_t* handle_temp = (handle_data_t*)xy_malloc(sizeof(handle_data_t));
		memset(handle_temp, 0x00, sizeof(handle_data_t));
		g_phandle = handle_temp;
	}

	if(lifetime == -1) //BC25普通赋值
	{
		g_phandle->lifetime = g_cdp_config_data->cdp_lifetime;
	}
	else //BC25 fota lifetime 赋值
	{
		//BC25对标，FOTA lifetime来源于升级之前的lifetime
		cdp_config_nvm_t * cdp_config_data = xy_malloc(sizeof(cdp_config_nvm_t));
		cloud_read_file(CDP_CONFIG_NVM_FILE_NAME,(void*)cdp_config_data,sizeof(cdp_config_nvm_t));
		g_phandle->lifetime = cdp_config_data->cdp_lifetime;
		xy_free(cdp_config_data);
	}

	if (cdp_get_params(g_phandle))
	{
		xy_printf(0,XYAPP, WARN_LOG, "[CDP]cdp_get_params error");
		goto failed;
	}
	
	if(upstream_info == NULL)
	{
    	upstream_info = (upstream_info_t*)xy_malloc(sizeof(upstream_info_t));
    	memset(upstream_info, 0, sizeof(upstream_info_t));
	}

	if(downstream_info == NULL)
	{
		downstream_info = (downstream_info_t*)xy_malloc(sizeof(downstream_info_t));
		memset(downstream_info, 0, sizeof(downstream_info_t));
	}

	if(recvdata_info == NULL)
	{
		recvdata_info = (recvdate_info_t*)xy_malloc(sizeof(recvdate_info_t));
		memset(recvdata_info, 0, sizeof(recvdate_info_t));
	}

	if(g_upstream_mutex == NULL)
    	cloud_mutex_create(&g_upstream_mutex);
	
	if(g_downstream_mutex == NULL)
    	cloud_mutex_create(&g_downstream_mutex);

	if(g_recvdata_mutex == NULL)
    	cloud_mutex_create(&g_recvdata_mutex);

    if(cdp_api_sendasyn_sem == NULL)
        cdp_api_sendasyn_sem = osSemaphoreNew(0xFFFF, 0, NULL);

	if(cdp_wait_sem == NULL)
        cdp_wait_sem = osSemaphoreNew(0xFFFF, 0, NULL);

    return XY_OK;

failed:
    cdp_clear();
    return XY_ERR;
}

void cdp_stream_clear(uint32_t *mutex, void *stream)
{
    buffer_list_t *node;

    if(stream == NULL || *mutex == NULL)
    	return;

    osMutexAcquire(*mutex, portMAX_DELAY);
    while(((upstream_info_t *)stream)->head != NULL)
    {
        node = ((upstream_info_t *)stream)->head;
        ((upstream_info_t *)stream)->head = ((upstream_info_t *)stream)->head->next;
        if(node->data_len > 0)
            xy_free(node->data);
        xy_free(node);
        if (((upstream_info_t *)stream)->head == NULL)
        {
            ((upstream_info_t *)stream)->tail = NULL;
        }
    }
    osMutexRelease(*mutex);
}

//cdp业务退出，释放信号量
void cdp_delete_sem_clear()
{
    if(cdp_task_delete_sem != NULL)
    {
        osSemaphoreRelease(cdp_task_delete_sem); 
    }
}

void cdp_clear()
{
    if (upstream_info != NULL)
	{
		cdp_stream_clear(&g_upstream_mutex, (void*)upstream_info);
#if VER_BC95
		upstream_info ->pending_num = 0;
#else
		xy_free(upstream_info);
		upstream_info = NULL;
#endif
	}   

    if (downstream_info != NULL)
	{
		cdp_stream_clear(&g_downstream_mutex, (void*)downstream_info);
		xy_free(downstream_info);
		downstream_info = NULL;
	}
        
	if (recvdata_info != NULL)
	{
		cdp_stream_clear(&g_recvdata_mutex, (void*)recvdata_info);
		xy_free(recvdata_info);
		recvdata_info = NULL;
	}

    if (g_upstream_mutex != NULL)
    {
        osMutexDelete(g_upstream_mutex);
        g_upstream_mutex = NULL;
    }

    if (g_downstream_mutex != NULL)
    {
        osMutexDelete(g_downstream_mutex);
        g_downstream_mutex = NULL;
    }

    if (g_recvdata_mutex != NULL)
    {
        osMutexDelete(g_recvdata_mutex);
        g_recvdata_mutex = NULL;
    }

	if (cdp_recovery_sem != NULL)
    {
	    //恢复走异常流程，需要解除信号量的阻塞
	    while (osSemaphoreAcquire(cdp_recovery_sem, 0) == osOK) {};
	    osSemaphoreRelease(cdp_recovery_sem);
    }

    if (cdp_wait_sem != NULL)
    {
        osSemaphoreDelete(cdp_wait_sem);
        cdp_wait_sem = NULL;
    }

    if (cdp_api_sendasyn_sem != NULL)
    {
        osSemaphoreDelete(cdp_api_sendasyn_sem);
        cdp_api_sendasyn_sem = NULL;
    }

	if(g_phandle != NULL)
	{
		if(g_phandle->atiny_params != NULL)
		{
			if(g_phandle->atiny_params->security_params->server_ip != NULL)
			{
				xy_free(g_phandle->atiny_params->security_params->server_ip);
			}

			if(g_phandle->atiny_params->security_params->server_port != NULL)
			{
				xy_free(g_phandle->atiny_params->security_params->server_port);
			}
			
			xy_free(g_phandle->atiny_params);
			g_phandle->atiny_params = NULL;
		}

		xy_free(g_phandle);
        g_phandle = NULL;
	}
}

uint32_t cdp_create_recv_task()
{
    int ret = XY_OK;
	osThreadAttr_t thread_attr = {0};
    if(g_lwm2m_recv_TskHandle == NULL)
    {
		thread_attr.stack_size = osStackShared;
        thread_attr.name = "lw_down";
		thread_attr.priority = osPriorityNormal1;
		g_lwm2m_recv_TskHandle = osThreadNew((osThreadFunc_t)(app_downdata_recv), NULL, &thread_attr);
		
        osThreadSetLowPowerFlag(g_lwm2m_recv_TskHandle, osLpmNoRealtime);
    }
    else 
        ret = XY_ERR;

    return ret;
}

void cdp_lwm2m_process(void *param)
{
	int lifetime = (int)param;
	
	if(!xy_tcpip_is_ok())
	{
	    if (cdp_recovery_sem != NULL)
	    {
            //恢复走异常流程，需要解除信号量的阻塞
            while (osSemaphoreAcquire(cdp_recovery_sem, 0) == osOK) {};
            osSemaphoreRelease(cdp_recovery_sem);
	    }
		xy_printf(0,XYAPP, WARN_LOG, "cdp volunt timeout!!!");
		g_lwm2m_TskHandle = NULL;
		osThreadExit();
	}

    if(XY_OK > cdp_init(lifetime))
    {
    	xy_printf(0,XYAPP, WARN_LOG, "[CDP]cdp init error");
        goto out;
    }

    if(ATINY_OK != atiny_init(g_phandle))
    {
    	xy_printf(0,XYAPP, WARN_LOG, "[CDP]cdp atiny_init error");
        goto out;
    }

    if(XY_OK != cdp_create_recv_task())
    {
        xy_printf(0,XYAPP, WARN_LOG, "[CDP]cdp creat_cdp_recv_task error");
        goto out;
    }

    (void)atiny_bind(g_phandle);

out:
	atiny_deinit(g_phandle);

    if(1 == cdp_register_fail_flag)
    {
    	lwm2m_notify_even(MODULE_LWM2M, XY_STATE_REG_FAILED, NULL, 0);
		cdp_register_fail_flag = 0;
    }

    if(0 != cdp_deregister_flag)
    {
    	if(cdp_deregister_flag == 1)
        	lwm2m_notify_even(MODULE_LWM2M, XY_STATE_DEREGISTERED, NULL, 0);
		cdp_deregister_flag = 0;
    }

	cdp_clear();
	cdp_delete_sem_clear();
	g_lwm2m_TskHandle = NULL;
    osThreadExit();
}

int cdp_create_lwm2m_task(int lifetime)
{
	int uwRet = XY_OK;
    if (g_lwm2m_TskHandle == NULL)
    {
		osThreadAttr_t thread_attr = {0};
		
		thread_attr.name       = "cdp_lw_tk";
		thread_attr.priority   = osPriorityNormal1;
		thread_attr.stack_size = osStackShared;
		g_lwm2m_TskHandle = osThreadNew((osThreadFunc_t)(cdp_lwm2m_process), (void*)lifetime, &thread_attr);
		return uwRet;
    }
	
	return XY_ERR;
}

int cdp_delete_lwm2m_task()
{
#ifdef CONFIG_FEATURE_FOTA	
	if(	atiny_fota_manager_get_state(atiny_fota_manager_get_instance()) || 
#else
	if(
#endif
		//cdp 不在运行或者已注册可以进行去注册流程
		(!is_cdp_running() || is_cdp_running()&& !atiny_mark_deregister(g_phandle)))
	{
	    return 1;
	}
	else
		return 0;
}


/**********cdp 文件系统会话类和配置类初始化函数**********/
void cdp_session_init()
{
	//cdp事件上报和nnmi 默认置1
	g_cdp_session_info->cdp_event_report_enable = 1;
	g_cdp_session_info->cdp_lwm2m_event_status = 255;
	
	g_cdp_session_info->cdp_nnmi = 1;
}


void cdp_module_init()
{
    static int cdp_module_init = 0;
	osMutexAcquire(g_cdp_module_init_mutex, osWaitForever);
	if(g_cdp_session_info == NULL)
	{
		//cdp会话类配置全局
		g_cdp_session_info = cloud_malloc(CDP_SESSION_NVM_FILE_NAME);
		if(!CDP_RECOVERY_FLAG_EXIST())
		{
			cloud_remove_file(CDP_SESSION_NVM_FILE_NAME);
			cdp_session_init();
		}
		else
		{
			if(cloud_read_file(CDP_SESSION_NVM_FILE_NAME,(void*)g_cdp_session_info,sizeof(cdp_session_info_t))== XY_ERR)
				cdp_session_init();
		}

		//BC25对标 深睡唤醒后NMSTATUS查询只要不是注册状态，均重置为未初始化状态；
		if(!CDP_NEED_RECOVERY(g_cdp_session_info->cdp_lwm2m_event_status))
		{
			g_cdp_session_info->cdp_lwm2m_event_status = 255;
		}

	}

	if(g_cdp_config_data == NULL)	
	{
		//cdp配置类全局
		g_cdp_config_data = cloud_malloc(CDP_CONFIG_NVM_FILE_NAME);
		if(XY_ERR == cloud_read_file(CDP_CONFIG_NVM_FILE_NAME,(void*)g_cdp_config_data,sizeof(cdp_config_nvm_t)))
			cdp_user_config_init();

		if(!CDP_RECOVERY_FLAG_EXIST() && !(Get_Boot_Reason()==SOFT_RESET && Get_Boot_Sub_Reason()==SOFT_RB_BY_FOTA)) //FOTA软重启走自注册流程lifetime 不置0
		{
			//BC25对标：重启后默认为0;
			g_cdp_config_data->cdp_lifetime = 0;
			cloud_save_file(CDP_CONFIG_NVM_FILE_NAME,(void*)g_cdp_config_data,sizeof(cdp_config_nvm_t));
		}

		//BC25对标:关闭恢复功能恢复默认值,此处不能合并到cdp_session_init里，因为关闭恢复功能，不影响session里的其他默认值比如nnmi在关闭深睡恢复功能后，深睡唤醒后有效
		if(g_cdp_config_data->is_need_recover == 1) 
		{
			g_cdp_session_info->cdp_lwm2m_event_status = 255; //BC25对标：关闭恢复功能要恢复默认值
		}	
	}

    if(!cdp_module_init)
    {
		cdp_module_init = 1;
		cdp_resume_mutex_init();
		//带seq_num发送数据时注册的回调
		xy_atc_registerPSEventCallback(D_XY_PS_REG_EVENT_IPSN, cdp_upstream_seq_callback);
	}
	osMutexRelease(g_cdp_module_init_mutex);
}
#endif

