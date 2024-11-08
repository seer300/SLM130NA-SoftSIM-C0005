
#if TELECOM_VER
#include "xy_utils.h"
#include "cloud_utils.h"
#include "at_cdp.h"
#include "xy_cdp_api.h"
#include "cdp_backup.h"
#include "agent_tiny_demo.h"
#include "agenttiny.h"
#include "atiny_context.h"
#include "xy_at_api.h"
#include "softap_nv.h"
#include "oss_nv.h"
#include "lwip/inet.h"
#include "net_app_resume.h"
#include "ps_netif_api.h"
#include "at_com.h"
#include "xy_system.h"
#include "atiny_fota_manager.h"
#include "atiny_fota_state.h"
#include "xy_net_api.h"



#if WITH_MBEDTLS_SUPPORT
#include "mbedtls/ssl.h"
#endif

cdp_config_nvm_t *g_cdp_config_data = NULL;
int g_send_status = 0;
extern cdp_session_info_t *g_cdp_session_info;

extern upstream_info_t *upstream_info;
extern downstream_info_t *downstream_info;
extern osSemaphoreId_t cdp_wait_sem;
extern int cdp_register_fail_flag;
extern osSemaphoreId_t cdp_task_delete_sem;

#define MSGTYPE_TRANS(type) \
{\
   if(type == 0x0000)\
   		type = cdp_NON;\
   else if(type == 0x0001) \
   		type = cdp_NON_RAI; \
   else if(type == 0x0100)   \
   	    type = cdp_CON;       \
   else if(type == 0x0101)     \
   		type = cdp_CON_WAIT_REPLY_RAI; \
   else if(type == 0x0010)     \
   		type = cdp_NON_WAIT_REPLY_RAI; \
}

#define MSGTYPE_TRANS2(type) \
{\
   if(type == 0)\
   		type = cdp_NON;\
   else if(type == 1) \
   		type = cdp_NON_RAI; \
   else if(type == 2)\
   	    type = cdp_NON_WAIT_REPLY_RAI;\       
   else if(type == 100)\     
   		type = cdp_CON;\ 
   else if(type == 101)\
   		type = cdp_CON_RAI; \
   else if(type == 102)\
   		type = cdp_CON_WAIT_REPLY_RAI; \
}

//AT+NMGS/AT+QLWULDATA\AT+QLWULDATAEX通用子接口
bool cdp_send_data(char *data, int len, int msg_type, uint8_t seq_num)
{
    int pending_num = -1;
    
    if(data == NULL)
        return 0;
    
    if(len>MAX_REPORT_DATA_LEN || len<=0)
        return 0;
	
	if(cdp_find_match_udp_node(seq_num))
		return 0;
   
    if (!xy_tcpip_is_ok()) 
        return 0;

    cdp_module_init();
    cdp_resume_app();
    
	if(!is_cdp_running())	  
	{
#if VER_BC95
		//cdp不在运行且不是在业务删除过程中，则发数据触发注册
		if(cdp_task_delete_sem == NULL && cdp_create_lwm2m_task(-1))
		{
			xy_printf(0,XYAPP, WARN_LOG, "[cdp_send_data] cdp_create_lwm2m_task failed");
		}
#endif
			
		return 0;
	}
	
	lwm2m_context_t *context = g_phandle->lwm2m_context;
	if(context->state != STATE_READY)	
	{
		return 0;
	}
	
	if(!is_cdp_upstream_ok())
	{
#if VER_BC95
		//只有在19/0/0取消订阅或者rst包后，才需要另起线程删除cdp业务并重新发起注册, 其他场景不需要此操作;
		if(g_cdp_session_info->cdp_lwm2m_event_status == 11 && cdp_restart_task())
		{
			xy_printf(0,XYAPP, WARN_LOG, "[cdp_send_data] cdp_restart_lwm2m_task failed");
		}
#endif
		return 0;
	}
	
    pending_num = get_upstream_message_pending_num();
    if(pending_num >= 8)
    {
        return 0;
    }
    if (send_message_via_lwm2m(data, len, msg_type,seq_num))
        return 0;
	
	cdp_add_sninfo_node(seq_num,msg_type);
	
    return 1;
}

//AT+NMGSEXT发送子接口
bool cdp_send_data_ex(char *data, int len, int msg_type)
{
	int pending_num = -1;
    if(!xy_tcpip_is_ok()) 
        return 0;

    cdp_module_init();
    cdp_resume_app();
    if(!is_cdp_running())
    {
		 return 0;
    }

	if(g_phandle->lwm2m_context->state != STATE_READY)
	{
		 return 0;
	}

	pending_num = get_upstream_message_pending_num();
    if(pending_num >= 8)
    {
        return 0;
    }

    g_send_status = 0;

    if(send_message_via_lwm2m(data, len, msg_type,0) < 0)
		return 0;

	return 1;
}

void at_send_NSMI(int state,uint8_t seq_num)
{
	char *rsp_cmd = xy_malloc(40);
	if(state==0)
	{
		sprintf(rsp_cmd,"+QLWEVTIND: 4");
	}	
	else
	{
		sprintf(rsp_cmd,"+QLWEVTIND: 5");
	}
	send_urc_to_ext(rsp_cmd, strlen(rsp_cmd));
	xy_free(rsp_cmd);
}

//AT+NCDPOPEN=<ip_addr>[,<port>[,<psk>]]
//AT+NCDPOPEN=?       OK
int at_NCDPOPEN_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		int port = 5683;
		int ret = AT_END;
		
		if (!xy_tcpip_is_ok()) 
		{
			return ATERR_NOT_NET_CONNECT;
		}

		char *ip_addr_str = xy_malloc(strlen(at_buf));
		char* psk = xy_malloc(strlen(at_buf));
        char* psk_temp  = xy_malloc(strlen(at_buf));

		memset(ip_addr_str, 0, strlen(at_buf));
		memset(psk, 0, strlen(at_buf));
		memset(psk_temp, 0, strlen(at_buf));

		if (at_parse_param("%s,%d[0-65535],%s", at_buf, ip_addr_str, &port, psk_temp) != AT_OK || (strlen(ip_addr_str) <= 0)||(strlen(ip_addr_str) > 40) || (strlen(psk_temp) > 32)) 
		{
			ret = ATERR_PARAM_INVALID;
			goto free_out;
		}

		if(port == 0)
			port = 5683;

		cdp_module_init(); 
    	cdp_resume_app();

		if(is_cdp_running())
		{
			ret = ATERR_NOT_ALLOWED;
			goto free_out;
		}

		if(set_cdp_server_settings(ip_addr_str, (u16_t)port) < 0)
		{
		   ret = ATERR_PARAM_INVALID;
		   goto free_out;
		}

		//如果设置了PSK，则说明需要进行加密，只支持DTLS加密
		if(strlen(psk_temp) > 0 && strlen(psk_temp) <= 32)
		{
			if (strlen(psk_temp)%2 != 0 || hexstr2bytes(psk_temp, strlen(psk_temp), psk, strlen(psk_temp)/2) == -1)
			{
				ret = ATERR_PARAM_INVALID;
				goto free_out;
			}

			memset(g_cdp_config_data->cloud_server_auth, 0, 16);
			memcpy(g_cdp_config_data->cloud_server_auth, psk, strlen(psk_temp)/2);
			g_cdp_config_data->psk_len = strlen(psk_temp)/2;

			g_cdp_config_data->cdp_dtls_switch = 1;

			g_cdp_config_data->cdp_dtls_nat_type = 0;

	    	char imei_temp[16] = {0};
	    	xy_get_IMEI(imei_temp, 16);	
			memset(g_cdp_config_data->cdp_pskid, 0, 16);
			memcpy(g_cdp_config_data->cdp_pskid, imei_temp, 16);

			cloud_save_file(CDP_CONFIG_NVM_FILE_NAME,(void*)g_cdp_config_data,sizeof(cdp_config_nvm_t));
			
		}
		else //若没有设置PSK，则使用非加密注册
		{
			g_cdp_config_data->cdp_dtls_switch = 0;
			cloud_save_file(CDP_CONFIG_NVM_FILE_NAME,(void*)g_cdp_config_data,sizeof(cdp_config_nvm_t));
		}
		
        if(cdp_create_lwm2m_task(-1))
        {
        	xy_printf(0, XYAPP, WARN_LOG, "registed failed!");
			ret = ATERR_NOT_ALLOWED;
			goto free_out;
        }
free_out:
		xy_free(ip_addr_str);
		xy_free(psk);
		xy_free(psk_temp);
		return ret;
		
	}
	else if(g_req_type != AT_CMD_TEST)
		return ATERR_PARAM_INVALID;

	return AT_END;
}

//AT+NCDPCLOSE
int at_NCDPCLOSE_req(char *at_buf, char **prsp_cmd)
{
	(void) at_buf;
	if(g_req_type != AT_CMD_ACTIVE)
	{
		return ATERR_PARAM_INVALID;
	}

	if (!xy_tcpip_is_ok()) 
	{
		return ATERR_NOT_NET_CONNECT;
	}

	cdp_module_init();
    cdp_resume_app();

	if(cdp_delete_lwm2m_task() == 0)
	{
		xy_printf(0, XYAPP, WARN_LOG, "deregisted failed!");
		return ATERR_NOT_ALLOWED;
	}
    return AT_END;   	
}

//AT+NCFG=<mode>[,value], mode 只支持0
//AT+NCFG=?
int g_cdp_need_recover = 0; //BC260Y 对标用，仅仅为了查询，深睡唤醒查询恢复初始值，无实际意义
int at_NCFG_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		int mode = -1;
		int value = -1;
		if (at_parse_param("%d(0-2),%d[0-2592000]", at_buf, &mode, &value) != AT_OK || mode != 0 && value > 1) 
		{
			return ATERR_PARAM_INVALID;
		}

		cdp_module_init();

		if(value == -1) //查询
		{
			*prsp_cmd = xy_malloc(40);
			if(mode == 0)
				snprintf(*prsp_cmd, 40, "%d", g_cdp_config_data->cdp_lifetime);
			else
				snprintf(*prsp_cmd, 40, "%d", g_cdp_need_recover);
			return AT_END;
		}

		if(mode == 0)
		{
			//生命周期最小值900
			g_cdp_config_data->cdp_lifetime = (value > 0 && value <= 900)?900:value;
			cloud_save_file(CDP_CONFIG_NVM_FILE_NAME,(void*)g_cdp_config_data,sizeof(cdp_session_info_t));
		}
		else
		{
			g_cdp_need_recover = value;
		}
	}
#if (AT_CUT!=1)
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(60);
		snprintf(*prsp_cmd, 60, "0[,(0-2592000)]");
	}
#endif
	else
		return ATERR_PARAM_INVALID;

    return AT_END;
}

//AT+NMGSEXT=<msg_type><length>,<data>
int at_NMGS_EXT_req(char *at_buf, char **prsp_cmd)
{
    int len = -1;
    int type = -1;
	char *data = NULL;
	
    if(g_req_type != AT_CMD_REQ)
    {
        return (ATERR_PARAM_INVALID);
    }

	if(is_Uplink_FlowCtl_Open())
		return ATERR_NOT_ALLOWED;
	
	data = xy_malloc2(strlen(at_buf));
	if(data == NULL)
	{
		return (ATERR_NO_MEM);
	}
	
    if(at_parse_param("%d(0-1),%l(1-1024),%h", at_buf, &type,&len,data) != AT_OK)
    {
        xy_free(data);
        return (ATERR_PARAM_INVALID);
    }

    if(!cdp_send_data_ex(data,len,type))
    {
    	xy_free(data);
        return (ATERR_NOT_ALLOWED);
    }

    xy_free(data);
    return AT_END;
}

//AT+NQMGS   PENDING=<pending>,SENT=<sent>,ERROR=<error>
int at_NQMGS_req(char *at_buf, char **prsp_cmd)
{
	(void) at_buf;
    if(g_req_type != AT_CMD_ACTIVE)
    {
        return  (ATERR_PARAM_INVALID);
    }

	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		return  (ATERR_NOT_ALLOWED);
	}

	cdp_module_init();

	*prsp_cmd = xy_malloc(60);
    if(upstream_info == NULL)
		sprintf(*prsp_cmd, "\r\nPENDING=0,SENT=0,ERROR=0");
	else
    	sprintf(*prsp_cmd, "\r\nPENDING=%d,SENT=%d,ERROR=%d", 
            get_upstream_message_pending_num(), get_upstream_message_sent_num(), get_upstream_message_error_num());
	return AT_END;
}

//AT+NQMGR   BUFFERED=<buffered>,RECEIVED=<received>,DROPPED=<dropped>
int at_NQMGR_req(char *at_buf, char **prsp_cmd)
{
	(void) at_buf;
    if(g_req_type != AT_CMD_ACTIVE)
    {
        return  (ATERR_PARAM_INVALID);
    }

	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		return  (ATERR_NOT_ALLOWED);
	}

	cdp_module_init();

	//BC260Y对标，下行缓存数据恢复，直接读，不能放在net_app_resume(若不走恢复，则读取数据失败)
	cdp_downstream_init();
	
	*prsp_cmd = xy_malloc(60);
    sprintf(*prsp_cmd, "\r\nBUFFERED=%d,RECEIVED=%d,DROPPED=%d", 
            get_downstream_message_buffered_num(), get_downstream_message_received_num(), get_downstream_message_dropped_num());
	return AT_END;
}

//AT+NMGR  <length>,<data>
int at_NMGR_req(char *at_buf, char **prsp_cmd)
{
    int data_len = 0;
    char *data = NULL;

    (void) at_buf;

    if(g_req_type != AT_CMD_ACTIVE)
    {
        return  (ATERR_PARAM_INVALID);
    }


	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		return  (ATERR_NOT_ALLOWED);
	}

	cdp_module_init();

	//BC260Y对标，下行缓存数据恢复，直接读，不能放在net_app_resume(若不走恢复，则读取数据失败)
	cdp_downstream_init();

    data = (char *)get_message_via_lwm2m((int *)&data_len);

    if (data_len > 0)
    {
        *prsp_cmd = xy_malloc2(30 + data_len*2);
		if(*prsp_cmd == NULL)
		{
			xy_free(data);
			return  ATERR_NO_MEM;
		}
		
		sprintf(*prsp_cmd,"%d,", data_len);
		bytes2hexstr(data, data_len, *prsp_cmd + strlen(*prsp_cmd), data_len * 2+1);
        xy_free(data);
    }

	return AT_END;
}

//AT+NNMI=<status>
int at_NNMI_req(char *at_buf, char **prsp_cmd)
{
	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		return  (ATERR_NOT_ALLOWED);
	}

	cdp_module_init();

	if(g_req_type == AT_CMD_REQ)
	{
	    int nnmi = -1;

		if (at_parse_param("%d(0-2)", at_buf, &nnmi) != AT_OK)
		{
				return (ATERR_PARAM_INVALID);
		}

		g_cdp_session_info->cdp_nnmi = nnmi;
		cloud_save_file(CDP_SESSION_NVM_FILE_NAME,(void*)g_cdp_session_info,sizeof(cdp_session_info_t));
		//设置恢复标志位，表示文件系统可用，下次深睡恢复读文件系统里的值；
		CDP_SET_RECOVERY_FLAG();
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(40);
		snprintf(*prsp_cmd, 40, "%d", g_cdp_session_info->cdp_nnmi);
	}
	else
		return  (ATERR_PARAM_INVALID);
	return AT_END;
}

//AT+NSMI=<status>
int at_NSMI_req(char *at_buf, char **prsp_cmd)
{
	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		return  (ATERR_NOT_ALLOWED);
	}

	cdp_module_init();
	
	if(g_req_type == AT_CMD_REQ)
	{
	    int nsmi = -1;

		if (at_parse_param("%d(0-1)", at_buf, &nsmi) != AT_OK) 
		{
				return (ATERR_PARAM_INVALID);
		}

		g_cdp_session_info->cdp_nsmi = nsmi;
		cloud_save_file(CDP_SESSION_NVM_FILE_NAME,(void*)g_cdp_session_info,sizeof(cdp_session_info_t));
		//设置恢复标志位，表示文件系统可用，下次深睡恢复读文件系统里的值；
		CDP_SET_RECOVERY_FLAG();
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(40);
		snprintf(*prsp_cmd, 40, "+NSMI:%d", g_cdp_session_info->cdp_nsmi);
	}
	else
		return  (ATERR_PARAM_INVALID);
	
	return AT_END;
}

// AT+NCDP=<ip_addr>[,<port>]
// AT+NCDP?  +NCDP:192.168.5.1,5683
int at_NCDP_req(char *at_buf, char **prsp_cmd)
{
	cdp_module_init();
	if(g_req_type == AT_CMD_REQ)
	{
	    char *ip_addr_str = xy_malloc(80);
	    int port = -1;

		if (at_parse_param("%47s(),%d[0-65535]", at_buf, ip_addr_str, &port) != AT_OK) 
		{

			xy_free(ip_addr_str);
			return (ATERR_PARAM_INVALID);
		}

		if(port == -1 || port == 0)
		{
#if VER_BC95
			if(port == 0 || (port == -1 && g_cdp_config_data->cloud_server_port == 0))
				port = 5683;
			else if(port == -1 && g_cdp_config_data->cloud_server_port != 0)
				port =g_cdp_config_data->cloud_server_port;
#else
			if(port == -1)
			{
				if(g_cdp_config_data->cdp_dtls_switch == 1)
					port = 5684;
				else
					port = 5683;
			}
#endif
		}
		

        if(set_cdp_server_settings(ip_addr_str, (u16_t)port) < 0)
    	{
    		xy_free(ip_addr_str);
    		return (ATERR_PARAM_INVALID);
    	}
		xy_free(ip_addr_str);
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(80);
		if(strcmp((const char *)get_cdp_server_ip_addr_str(), "") == 0)
		{
			snprintf(*prsp_cmd, 80, "+NCDP:");
		}
		else
			snprintf(*prsp_cmd, 80, "+NCDP:%s,%d", (char *)get_cdp_server_ip_addr_str(), (int)get_cdp_server_port());
	}
    else
    {
        return (ATERR_PARAM_INVALID);
    }
        
	return AT_END;
}

//AT+QREGSWT=<reg_type>
int at_QREGSWT_req(char *at_buf, char **prsp_cmd)
{
	cdp_module_init();
	if(g_req_type == AT_CMD_REQ)
	{
	    char type = -1;
        
	    if (at_parse_param("%1d(0-2)", at_buf, &type) != AT_OK) {
			return (ATERR_PARAM_INVALID);
		}
		
		g_softap_fac_nv->cdp_register_mode = type;
		SAVE_FAC_PARAM(cdp_register_mode);
	   
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(24);
		snprintf(*prsp_cmd, 24, "+QREGSWT:%d", g_softap_fac_nv->cdp_register_mode);
	}
    else
    {
        return (ATERR_PARAM_INVALID);
    }
	
    return AT_END;
}

//AT+QLWEVTIND=<n>
//AT+QLWEVTIND?  +QLWEVTIND:<n>,<type>
//AT+QLWEVTIND=? +QLWEVTIND:(支持的n列表) 
int at_QLWEVTIND_req(char *at_buf, char **prsp_cmd)
{
	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		return (ATERR_NOT_ALLOWED);
	}

	cdp_module_init();
	if(g_req_type == AT_CMD_REQ)
	{
	    int mode = -1; 
	    if (at_parse_param("%d(0-1)", at_buf, &mode) != AT_OK) {
			return (ATERR_PARAM_INVALID);
		}
		
		g_cdp_session_info->cdp_event_report_enable = mode;
		cloud_save_file(CDP_SESSION_NVM_FILE_NAME,(void*)g_cdp_session_info,sizeof(cdp_session_info_t));
		//设置恢复标志位，表示文件系统可用，下次深睡恢复读文件系统里的值；
		CDP_SET_RECOVERY_FLAG();
	    
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(30);
		if(g_cdp_session_info->cdp_lwm2m_event_status == 10)//服务器拒绝
			snprintf(*prsp_cmd, 30, "+QLWEVTIND:%d,255",g_cdp_session_info->cdp_event_report_enable);
		else if(g_cdp_session_info->cdp_lwm2m_event_status == 11)//取消订阅
			snprintf(*prsp_cmd, 30, "+QLWEVTIND:%d,9",g_cdp_session_info->cdp_event_report_enable);
		else
			snprintf(*prsp_cmd, 30, "+QLWEVTIND:%d,%d", g_cdp_session_info->cdp_event_report_enable, g_cdp_session_info->cdp_lwm2m_event_status);
	}
#if (AT_CUT!=1)
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(30);
		snprintf(*prsp_cmd, 30, "+QLWEVTIND:(0,1)");
	}
#endif
	else
		return (ATERR_PARAM_INVALID);
    return AT_END;
}


//AT+QLWSREGIND=<type>[,lifetime]
int at_QLWSREGIND_req(char *at_buf, char **prsp_cmd)
{
    int type = -1;
    int lifetime = -1;
    if(g_req_type != AT_CMD_REQ)
    {
        return (ATERR_PARAM_INVALID);
    }

	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		return  (ATERR_NOT_ALLOWED);
	}

	if (!xy_tcpip_is_ok()) {
		return  (ATERR_NOT_NET_CONNECT);
	}

    if (at_parse_param("%d(0-1),%d[120-2592000]", at_buf, &type,&lifetime) != AT_OK) {
		return  (ATERR_PARAM_INVALID);
	}

    //功能对标，携带lifetime参数报错
#if VER_BC95
	if(lifetime != -1)
	{
		return  (ATERR_PARAM_INVALID);
	}
#endif

    cdp_module_init();
    cdp_resume_app();
	
    if (type == 0)
    {
    	if(strcmp((const char *)get_cdp_server_ip_addr_str(), "") == 0)
    	{
    		xy_printf(0,XYAPP, WARN_LOG, "cdp server addr is empty!");
			return  (ATERR_NOT_ALLOWED);
    	}
        else if(cdp_create_lwm2m_task(lifetime))
        {
        	xy_printf(0,XYAPP, WARN_LOG, "registed failed!");
#if VER_BC95
			return AT_END;
#else
			return  (ATERR_NOT_ALLOWED);
#endif 
        }
    }
    else if (type == 1)
    {
        if(cdp_delete_lwm2m_task() == 0)
        {
        	xy_printf(0,XYAPP, WARN_LOG, "deregisted failed!");
			return  (ATERR_NOT_ALLOWED);
       	}
    }
    return AT_END;
}

//AT+CDPUPDATE
int at_CDPUPDATE_req(char *at_buf, char **prsp_cmd)
{
	int ret = -1;

	(void) at_buf;

    if(g_req_type != AT_CMD_ACTIVE)
    {
        return  (ATERR_PARAM_INVALID);
    }
        
	if (!xy_tcpip_is_ok()) {
		return  (ATERR_NOT_NET_CONNECT);
	}

    cdp_module_init();
    cdp_resume_app();

	if (!is_cdp_running())    
    {
		return  (ATERR_NOT_ALLOWED);
    }

    lwm2m_context_t *context = g_phandle->lwm2m_context;
	if(context->state != STATE_READY)	
	{
		return  (ATERR_NOT_ALLOWED);
	}

    xy_lwm2m_server_t *targetP = context->serverList;
    if(targetP == NULL)
	{
		return  (ATERR_NOT_ALLOWED);
	}

    ret = cdp_update_proc(targetP);
	if (ret == -1)
	{
		return  (ATERR_NOT_ALLOWED);
	}
	return AT_END;
}


//AT+NMSTATUS=?  +NMSTATUS:<支持的注册状态列表>
//AT+NMSTATUS?  +NMSTATUS:<reg_stauts>
int at_NMSTATUS_req(char *at_buf, char **prsp_cmd)
{
    (void) at_buf;
	if((g_req_type != AT_CMD_QUERY))
	{
		return  (ATERR_PARAM_INVALID);
	}

	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		return  ATERR_NOT_ALLOWED;
	}

	cdp_module_init();
	*prsp_cmd = xy_malloc(96);

	if(strcmp((const char *)get_cdp_server_ip_addr_str(), "") == 0)
	{
		snprintf(*prsp_cmd, 96, "UNINITIALISED");
		return AT_END;
	}

	if(g_phandle != NULL)
	{
		lwm2m_context_t *context = (lwm2m_context_t *)(g_phandle->lwm2m_context);
		if(context == NULL || (context != NULL && context->state <= STATE_REGISTER_REQUIRED))
		{
		   snprintf(*prsp_cmd, 96, "REGISTERING");
		   return AT_END;
		}
		else if(context->state == STATE_REGISTERING)
		{
		   snprintf(*prsp_cmd, 96, "REGISTERING");
		   return AT_END;
		}
	}

 	if(g_cdp_session_info->cdp_lwm2m_event_status == ATINY_REG_OK || g_cdp_session_info->cdp_lwm2m_event_status == 9)
	   snprintf(*prsp_cmd, 96, "REGISTERED");
	else if(g_cdp_session_info->cdp_lwm2m_event_status == ATINY_DEREG)
	   snprintf(*prsp_cmd, 96, "DEREGISTERED");
	else if(g_cdp_session_info->cdp_lwm2m_event_status == ATINY_REG_FAIL)
	   snprintf(*prsp_cmd, 96, "TIMEOUT");
	else if(g_cdp_session_info->cdp_lwm2m_event_status == 10) //服务器拒绝
	   snprintf(*prsp_cmd, 96, "REJECTED_BY_SERVER");
	else if(g_cdp_session_info->cdp_lwm2m_event_status == 255)
	   snprintf(*prsp_cmd, 96, "UNINITIALISED");
	else
	{
		snprintf(*prsp_cmd, 96, "REGISTERED_AND_OBSERVED");
	}
	return AT_END;
}

//AT+QLWULDATA=<length>,<data>
//功能同AT+NMGS
int at_QLWULDATA_req(char *at_buf, char **prsp_cmd)
{
	int len = -1;
	int ret = -1;
	char *src_data = NULL;
	int seq_num = 0;
	int type = 0;
	int trans_type = -1;
	
    if(g_req_type != AT_CMD_REQ)
    {
        return  (ATERR_PARAM_INVALID);
    }
	
	if(is_Uplink_FlowCtl_Open())
		return ATERR_NOT_ALLOWED;

	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		return  (ATERR_NOT_ALLOWED);
	}

	src_data = xy_malloc2(strlen(at_buf));
	if(src_data == NULL)
		return  (ATERR_NO_MEM);

	if (at_parse_param("%l(1-1024),%h,%d[0|1|2|100|101|102]", at_buf, &len,src_data, &type) != AT_OK || cdp_get_con_send_flag())
	{
		xy_free(src_data);
		return  (ATERR_PARAM_INVALID);
	}

	MSGTYPE_TRANS2(type);
	if(!cdp_send_data(src_data,len,type,0))
	{
		xy_free(src_data);
		return  (ATERR_NOT_ALLOWED);
	}

	cdp_con_flag_init(type,0);

	xy_free(src_data);
	return AT_END;
}

//AT+QLWULDATAEX=<length>,<data>,<msg_type>
int at_QLWULDATAEX_req(char *at_buf, char **prsp_cmd)
{
    int len = -1;
    int type = -1;
    char *data = NULL;
	int seq_num = 0;

    if(g_req_type != AT_CMD_REQ)
    {
        return  (ATERR_PARAM_INVALID);
    }

	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		return  (ATERR_NOT_ALLOWED);
	}
	if(is_Uplink_FlowCtl_Open())
		return ATERR_NOT_ALLOWED;

	data = xy_malloc2(strlen(at_buf));
	if(data == NULL)
	{
		return  (ATERR_NO_MEM);
	}
	
    if(at_parse_param("%l(1-1024),%h,%d(0x0000|0x0001|0x0010|0x0100|0x0101),%d[0-255]", at_buf, &len,data,&type,&seq_num) != AT_OK)
    {
    	xy_free(data);
        return  (ATERR_PARAM_INVALID);
    }

    if(cdp_get_con_send_flag())
    {
    	xy_free(data);
        return  (ATERR_PARAM_INVALID);
    }

	MSGTYPE_TRANS(type);
	if(!cdp_send_data(data,len,type,(uint8_t)seq_num))
	{		
		xy_free(data);
		return  (ATERR_NOT_ALLOWED);
	}
	cdp_con_flag_init(type,seq_num);

    xy_free(data);

    return AT_END;
}

//AT+QLWULDATASTATUS?
int at_QLWULDATASTATUS_req(char *at_buf, char **prsp_cmd)
{
	(void) at_buf;
	uint8_t seq_num = 0;

    if(g_req_type != AT_CMD_QUERY)
    {
        return  (ATERR_PARAM_INVALID);
    }

	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		return  (ATERR_NOT_ALLOWED);
	}
	
	cdp_module_init();

    *prsp_cmd = xy_malloc(36);
	
    sprintf(*prsp_cmd, "%d", g_send_status);
	
    return AT_END;
}

//AT+CDPRMLFT
int at_CDPRMLFT_req(char *at_buf, char **prsp_cmd)
{
	(void) at_buf;
    int remain_lifetime = -1;

    if(g_req_type != AT_CMD_ACTIVE)
    {
        return  (ATERR_PARAM_INVALID);
    }

	cdp_module_init();
    if (g_phandle != NULL && g_phandle->lwm2m_context != NULL && g_phandle->lwm2m_context->state != STATE_READY)
        goto error;

    if (!is_cdp_running())
    {
        if(!CDP_RECOVERY_FLAG_EXIST() || !CDP_NEED_RECOVERY(g_cdp_session_info->cdp_lwm2m_event_status))
            goto error;

        remain_lifetime = g_cdp_session_info->regtime + g_cdp_session_info->lifetime - cloud_gettime_s();
    }
    else if ( g_phandle !=NULL && g_phandle->lwm2m_context != NULL && g_phandle->lwm2m_context->serverList != NULL)
    {
        remain_lifetime = g_phandle->lwm2m_context->serverList->registration +
            g_phandle->lwm2m_context->serverList->lifetime - cloud_gettime_s();
    }
    else
        goto error;
    
    *prsp_cmd = xy_malloc(40);
    if (remain_lifetime >= 0)
    {
        sprintf(*prsp_cmd,"+CDPRMLFT:%d", remain_lifetime);        
    }
    else
    {
        sprintf(*prsp_cmd,"+CDPRMLFT:error");
    }

	return AT_END;

error:
    return  (ATERR_NOT_ALLOWED);
}

//AT+QSECSWT=<status>
int at_QSECSWT_req(char *at_buf, char **prsp_cmd)
{
	cdp_module_init();
	if(g_req_type == AT_CMD_REQ)
	{
 #if WITH_MBEDTLS_SUPPORT           
	    int dtls_switch = -1;
        int nat_type = 0;
		if (at_parse_param("%d(0-1),%d[0-1]", at_buf,&dtls_switch, &nat_type) != AT_OK)
		{
			return  (ATERR_PARAM_INVALID);
		}
		
		if(dtls_switch == 0 && at_strnchr(at_buf,',',1))
		{
			return  (ATERR_PARAM_INVALID);
		}

		g_cdp_config_data->cdp_dtls_switch = dtls_switch;
		
        if(g_cdp_config_data->cdp_dtls_switch == 1)
			g_cdp_config_data->cdp_dtls_nat_type = nat_type;

		cloud_save_file(CDP_CONFIG_NVM_FILE_NAME,(void*)g_cdp_config_data,sizeof(cdp_config_nvm_t));
#else
        return  (ATERR_NOT_ALLOWED);
#endif
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(40);
        if(g_cdp_config_data->cdp_dtls_switch == 1){
		    snprintf(*prsp_cmd, 40, "+QSECSWT:%d,%d", 
               g_cdp_config_data->cdp_dtls_switch, g_cdp_config_data->cdp_dtls_nat_type);
        }else{
            snprintf(*prsp_cmd, 40, "+QSECSWT:%d", g_cdp_config_data->cdp_dtls_switch);
        }
        
	}
	else
		return  (ATERR_PARAM_INVALID);

	return AT_END;
}

//AT+QSETPSK=<pskid>,<psk>
int at_QSETPSK_req(char *at_buf, char **prsp_cmd)
{
	cdp_module_init();
	
	if(g_req_type == AT_CMD_REQ)
	{
	
#if WITH_MBEDTLS_SUPPORT
        char* psk = NULL;
        char* psk_id = NULL;
		
#if !VER_BC95
		if((cdp_handle_exist()) || (g_cdp_config_data->cdp_dtls_switch == 0))
        {
            return  (ATERR_NOT_ALLOWED);
        }
#endif
		psk = xy_malloc(strlen(at_buf));
		if (at_parse_param("%p(),%32h()", at_buf, &psk_id, psk) != AT_OK) {
			xy_free(psk);
			return  (ATERR_PARAM_INVALID);
		}

		if((strlen(psk_id) != 15 && strlen(psk_id) != 1) || 
			(strlen(psk_id) == 1 && psk_id[0] != '0'))
		{
			xy_free(psk);
			return  (ATERR_PARAM_INVALID);
		}

		if(strlen(psk_id) == 15)
		{
			if(!is_digit_str(psk_id))
			{
				xy_free(psk);
				return  (ATERR_PARAM_INVALID);
			}
		}

		if(strlen(psk_id) == 1 && psk_id[0] == '0')
    	{
    		xy_get_IMEI(psk_id, 16);
    	}
		
		memcpy(g_cdp_config_data->cdp_pskid, psk_id, 16);
	    memcpy(g_cdp_config_data->cloud_server_auth, psk, 16);
		cloud_save_file(CDP_CONFIG_NVM_FILE_NAME,(void*)g_cdp_config_data,sizeof(cdp_config_nvm_t));

		xy_free(psk);
#else
        return  (ATERR_NOT_ALLOWED);
#endif

	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(48);
		
		snprintf(*prsp_cmd, 48, "+QSETPSK:%s,***",g_cdp_config_data->cdp_pskid);
	}
    else
    {
        return  (ATERR_PARAM_INVALID);
    }
    return AT_END;
}


//AT+QRESETDTLS
int at_QRESETDTLS_req(char *at_buf, char **prsp_cmd)
{
    if(g_req_type != AT_CMD_ACTIVE)
    {
        return  (ATERR_PARAM_INVALID);
    }

	cdp_module_init();
    if(!is_cdp_running())
	{
#if !VER_BC95
		return  (ATERR_NOT_ALLOWED);
#else
		return AT_END;
#endif
	}
		
#if WITH_MBEDTLS_SUPPORT
	if(g_cdp_config_data->cdp_dtls_switch == 0)
    {
    
#if !VER_BC95
		return  (ATERR_NOT_ALLOWED);
#else
        return AT_END;
#endif
    }
	
    connection_t  *connList = g_phandle->client_data.connList;
    if(connList == NULL)
    {
        return  (ATERR_NOT_ALLOWED);
    }
        
    mbedtls_ssl_context *ssl = (mbedtls_ssl_context *)connList->net_context;
    
    if(ssl == NULL)
    {
        return  (ATERR_NOT_ALLOWED);
    }

	if(ssl->state > MBEDTLS_SSL_HELLO_REQUEST  && ssl->state < MBEDTLS_SSL_HANDSHAKE_OVER)
		return AT_END;
	
    if(ssl->state == MBEDTLS_SSL_HANDSHAKE_OVER)
    {
        connList->dtls_renegotiate_flag = true;
    }
	return AT_END;
#else
    return  (ATERR_NOT_ALLOWED);
#endif
}

//AT+QDTLSSTAT? +QDTLSSTAT:0  OK
int at_QDTLSSTAT_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type != AT_CMD_QUERY)
	{
        return  (ATERR_PARAM_INVALID);
	}

	cdp_module_init();
#if !VER_BC95
	if(is_cdp_running() == 0)
    {
        return  (ATERR_NOT_ALLOWED);
    }
#endif


#if WITH_MBEDTLS_SUPPORT
#if !VER_BC95
	if(g_cdp_config_data->cdp_dtls_switch == 0)
    {
        return  (ATERR_NOT_ALLOWED);
    }
#endif
	if(g_cdp_config_data->cdp_dtls_switch == 0 || !is_cdp_running()
		 || (g_phandle == NULL && cdp_register_fail_flag == 0))
	{
		*prsp_cmd = xy_malloc(48);
		snprintf(*prsp_cmd, 48, "+QDTLSSTAT:1");
		return AT_END;
	}

	*prsp_cmd = xy_malloc(48);
	if(g_phandle == NULL && cdp_register_fail_flag == 1)
	{
		snprintf(*prsp_cmd, 48, "+QDTLSSTAT:3");
		return AT_END;
	}
	
    connection_t  *connList = g_phandle->client_data.connList;
	if(connList != NULL)
	{
		mbedtls_ssl_context *ssl = (mbedtls_ssl_context *)connList->net_context;

		if(ssl != NULL && ssl->state == MBEDTLS_SSL_HANDSHAKE_OVER)
		{
			if(connList->dtls_renegotiate_flag == true)
				snprintf(*prsp_cmd, 48, "+QDTLSSTAT:1");
			else
				snprintf(*prsp_cmd, 48, "+QDTLSSTAT:0");
		}
		else
			snprintf(*prsp_cmd, 48, "+QDTLSSTAT:2");
	}
	else if(cdp_register_fail_flag == 1)
		snprintf(*prsp_cmd, 48, "+QDTLSSTAT:3");
	else
	{
		snprintf(*prsp_cmd, 48, "+QDTLSSTAT:2");
	}
	return AT_END;
#else
    return  (ATERR_PARAM_INVALID);
#endif
}

//AT+QCRITICALDATA=1
int at_QCRITICALDATA_req(char *at_buf, char **prsp_cmd)
{
	cdp_module_init();
	if(g_req_type == AT_CMD_REQ)
	{
	    int state = -1;
		if(at_parse_param("%d(1-1)", at_buf, &state) != AT_OK)
		{
			return  (ATERR_PARAM_INVALID);
		}

		if(!is_cdp_running() || !CDP_NEED_RECOVERY(g_cdp_session_info->cdp_lwm2m_event_status))
			return  (ATERR_NOT_ALLOWED);
	}
	else
		return  (ATERR_PARAM_INVALID);
	
	return AT_END;
}


//AT+QCFG="LWM2M/lifetime",900
int at_QCFG_req(char *at_buf, char **prsp_cmd)
{	
	uint8_t *lwm2m_param_type = NULL;
	uint8_t *sub_param = NULL;
	
#if (AT_CUT!=1)
	if(g_req_type == AT_CMD_QUERY || g_req_type == AT_CMD_TEST)
	{
		return AT_END;
	}
#endif

	if(at_parse_param("%p,%p", at_buf, &lwm2m_param_type, &sub_param) != AT_OK) 
	{
		return  (ATERR_PARAM_INVALID);
	}
	
	cdp_module_init();

	if(at_strcasecmp(lwm2m_param_type,"LWM2M/Lifetime"))
	{
		if(sub_param == NULL)
		{
			*prsp_cmd = xy_malloc(48);	
#if VER_BC95
			snprintf(*prsp_cmd, 48, "\"LWM2M/Lifetime\",%ld",  g_cdp_config_data->cdp_lifetime);
#else
			snprintf(*prsp_cmd, 48, "+QCFG:\"LWM2M/Lifetime\",%ld",  g_cdp_config_data->cdp_lifetime);
#endif
		}
		else
		{
			if(is_cdp_running() || CDP_NEED_RECOVERY(g_cdp_session_info->cdp_lwm2m_event_status))    
		    {
				return  (ATERR_NOT_ALLOWED);
		    }

			int lifetime = strtol(sub_param, NULL, 10);
			if(lifetime > LWM2M_DEFAULT_LIFETIME *30 || lifetime < 0)
			{
				return  (ATERR_NOT_ALLOWED);
			}
			
#if VER_BC95
			g_cdp_config_data->cdp_lifetime = (lifetime > 0 && lifetime <= 900)?900:lifetime;
#else
            g_cdp_config_data->cdp_lifetime = (lifetime > 0 && lifetime <= 120)?120:lifetime;
#endif
			cloud_save_file(CDP_CONFIG_NVM_FILE_NAME,(void*)g_cdp_config_data,sizeof(cdp_config_nvm_t));
		}
	}
	else if(at_strcasecmp(lwm2m_param_type,"LWM2M/EndpointName"))
	{
		if(sub_param == NULL)
		{
			*prsp_cmd = xy_malloc(50 + strlen(g_cdp_session_info->endpointname));
#if VER_BC95
			snprintf(*prsp_cmd, 50 + strlen(g_cdp_session_info->endpointname), "\"LWM2M/EndpointName\",\"%s\"", g_cdp_session_info->endpointname);
#else
			snprintf(*prsp_cmd, 50 + strlen(g_cdp_session_info->endpointname), "+QCFG:\"LWM2M/EndpointName\",\"%s\"", g_cdp_session_info->endpointname);
#endif	
		}
		else if(strlen(sub_param) > 252) //endpoint 长度限制在252个字节，SDK内部组包会添加"ep="导致溢出
		{
			return  (ATERR_NOT_ALLOWED);
		}
		else
		{
		
#if !VER_BC95
			if(is_cdp_running() || CDP_NEED_RECOVERY(g_cdp_session_info->cdp_lwm2m_event_status))
		    {
				return  (ATERR_NOT_ALLOWED);
		    }
#endif
			if(cdp_set_endpoint_name(sub_param))
			{
				return  (ATERR_PARAM_INVALID);
			}
		}
	}
	else if(at_strcasecmp(lwm2m_param_type,"LWM2M/BindingMode"))
	{
		if(sub_param == NULL)
		{
			*prsp_cmd = xy_malloc(48);
#if VER_BC95
			snprintf(*prsp_cmd, 48, "\"LWM2M/BindingMode\",%1d", (g_cdp_config_data->binding_mode == 0)?1:(g_cdp_config_data->binding_mode));
#else
			snprintf(*prsp_cmd, 48, "+QCFG:\"LWM2M/BindingMode\",%1d", (g_cdp_config_data->binding_mode == 0)?1:(g_cdp_config_data->binding_mode));
#endif
		}
		else
		{
			int bindMode = strtol(sub_param, NULL, 10);
			if(bindMode > 2 || bindMode < 1)
			{
				return  (ATERR_NOT_ALLOWED);
			}
			
#if !VER_BC95
			if(CHECK_SDK_TYPE(OPERATION_VER)) //入库版本，设置后做UPDATE
			{
				cdp_resume_app();
				if(is_cdp_running())
				{
					handle_data_t *handle = (handle_data_t *)g_phandle;
					lwm2m_context_t *context = handle->lwm2m_context;
					xy_lwm2m_server_t *targetP = context->serverList;
					if((context != NULL && context->state != STATE_READY) || targetP == NULL || targetP->status != XY_STATE_REGISTERED)
					{
						xy_printf(0,XYAPP, WARN_LOG, "cdp current status is error!");
						return (ATERR_NOT_ALLOWED);
					}

					g_cdp_config_data->binding_mode = bindMode;
					cloud_save_file(CDP_CONFIG_NVM_FILE_NAME,(void*)g_cdp_config_data,sizeof(cdp_config_nvm_t));

					cdp_update_proc(targetP);
				}
				else //入库版本，不在运行，也允许设置
				{
					g_cdp_config_data->binding_mode = bindMode;
					cloud_save_file(CDP_CONFIG_NVM_FILE_NAME,(void*)g_cdp_config_data,sizeof(cdp_config_nvm_t));
				}
			}
			else
			{
				//标准版本在运行，不允许设置
				if(is_cdp_running() || CDP_NEED_RECOVERY(g_cdp_session_info->cdp_lwm2m_event_status))    
			    {
					return  (ATERR_NOT_ALLOWED);
			    }

				g_cdp_config_data->binding_mode = bindMode;
				cloud_save_file(CDP_CONFIG_NVM_FILE_NAME,(void*)g_cdp_config_data,sizeof(cdp_config_nvm_t));
			}
			
#else
			//移远版本随时可设置
			g_cdp_config_data->binding_mode = bindMode;
			cloud_save_file(CDP_CONFIG_NVM_FILE_NAME,(void*)g_cdp_config_data,sizeof(cdp_config_nvm_t));
#endif
		}
	}
	else
		return  (ATERR_PARAM_INVALID);
	

	return AT_END;
}



//AT+QLWFOTAIND=0
int at_QLWFOTAIND_req(char *at_buf, char **prsp_cmd)
{
#ifndef CONFIG_FEATURE_FOTA
	return  (ATERR_NOT_ALLOWED);
#else
    int type = -1;
	if(g_softap_fac_nv->cdp_register_mode == 2)
	{
		return  (ATERR_NOT_ALLOWED);
	}

    cdp_module_init();
    cdp_resume_app();

    if(g_req_type == AT_CMD_REQ)
    {
    	if (at_parse_param("%d(0-5)", at_buf, &type) != AT_OK)
	    {
	        return  (ATERR_PARAM_INVALID);
	    }

	    if(g_cdp_config_data->cdp_dfota_type == 0 && type > 1 && type < 6)
	    {
	        return  (ATERR_NOT_ALLOWED);
	    }

	    int ret = atiny_fota_manager_get_state(atiny_fota_manager_get_instance());
	    if((type == 0 || type == 1) && ret != ATINY_FOTA_IDLE)
	    {
	        return  (ATERR_NOT_ALLOWED);    
	    }

	    char *pkt_uri = atiny_fota_manager_get_pkg_uri(atiny_fota_manager_get_instance());
	    switch(type)
	    {
	    case 0:     
            if(pkt_uri != NULL && strlen(pkt_uri) != 0)
            {
                return  (ATERR_NOT_ALLOWED);   
            }
            g_cdp_config_data->cdp_dfota_type = type;  
	        break;

	   case 1:
            if(pkt_uri != NULL && strlen(pkt_uri) != 0)
            {
                return  (ATERR_NOT_ALLOWED);   
            }

            g_cdp_config_data->cdp_dfota_type = type;
	        break;

	    case 2:
	        if(g_cdp_config_data->cdp_dfota_type == 1 && ret == ATINY_FOTA_IDLE)
	        {
	            if(pkt_uri == NULL || strlen(pkt_uri) == 0)
	            {
	                return  (ATERR_NOT_ALLOWED);      
	            }

	            ret = atiny_fota_manager_start_download(atiny_fota_manager_get_instance());
	            if(ret != XY_OK)
	                return  (ATERR_NOT_ALLOWED);
	        }   
	        else
	           return  (ATERR_NOT_ALLOWED); 
	        break;

	    case 3:
	        if(g_cdp_config_data->cdp_dfota_type == 1 && ret == ATINY_FOTA_IDLE)
	        {
	            if(pkt_uri == NULL || strlen(pkt_uri) == 0)
	            {
	                return  (ATERR_NOT_ALLOWED);      
	            }

	            atiny_fota_manager_set_update_result(atiny_fota_manager_get_instance(), ATINY_FIRMWARE_UPDATE_FAIL);
	            ret = atiny_fota_manager_rpt_state(atiny_fota_manager_get_instance(), ATINY_FOTA_IDLE);
	            if(ret != XY_OK)
	                return  (ATERR_NOT_ALLOWED);
	            atiny_fota_manager_set_pkg_uri(atiny_fota_manager_get_instance(), NULL, 0);
	        }   
	        else
	           return  (ATERR_NOT_ALLOWED); 
	        break;

	    case 4:
	        if(g_cdp_config_data->cdp_dfota_type == 1 && ret == ATINY_FOTA_DOWNLOADED)
	        {
	            ret = atiny_fota_manager_execute_update(atiny_fota_manager_get_instance());
	            if(ret != XY_OK)
	                return  (ATERR_NOT_ALLOWED);
	        }   
	        else
	           return  (ATERR_NOT_ALLOWED); 
	        break;

	    case 5:
	        if(g_cdp_config_data->cdp_dfota_type == 1 && ret == ATINY_FOTA_DOWNLOADED)
	        {
	            atiny_fota_manager_set_update_result(atiny_fota_manager_get_instance(), ATINY_FIRMWARE_UPDATE_FAIL);
	            ret = atiny_fota_manager_rpt_state(atiny_fota_manager_get_instance(), ATINY_FOTA_IDLE);
	            if(ret != XY_OK)
	                return  (ATERR_NOT_ALLOWED);
	            atiny_fota_manager_set_pkg_uri(atiny_fota_manager_get_instance(), NULL, 0);
	        }   
	        else
	           return  (ATERR_NOT_ALLOWED); 
	        break;

	    default:
	        break;
	    }
		cloud_save_file(CDP_CONFIG_NVM_FILE_NAME,(void*)g_cdp_config_data,sizeof(cdp_config_nvm_t));
	    if(cdp_wait_sem != NULL)
	        osSemaphoreRelease(cdp_wait_sem);  
    }
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(40);
		 snprintf(*prsp_cmd, 40, "+QLWFOTAIND:%d",g_cdp_config_data->cdp_dfota_type);
	}
	else
		return  (ATERR_PARAM_INVALID);
#endif
    return AT_END;
}


#endif

