#include "xy_at_api.h"
#include "factory_nv.h"
#include "xy_utils.h"
#include "lwip/netif.h"
#include "oss_nv.h"
#include "xy_system.h"
#include "xy_net_api.h"
#include "xy_ps_api.h"
#include "net_app_resume.h"
#include "tele_uni_dm.h"
#include "dm_ctl.h"


#define DM_AT_TEST_JSON_STR "{\"regver\":\"\",\"swver\":\"\",\"model\":\"\",\"osver\":\"\",\"imei\":\"\",\"uetype\":\"\"}"
extern osThreadId_t g_dm_TskHandle;
extern osMutexId_t g_tudm_context_mutex;
//设置电信联通DM开关
int at_QSREGENABLE_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
	    int type = -1;
	    if (at_parse_param("%d", at_buf, &type) != AT_OK) 
		{
			return  (ATERR_PARAM_INVALID);
		}
        
	    if (type >=0 && type < 2)
	    { 	
			g_softap_fac_nv->need_start_dm = type;
			SAVE_FAC_PARAM(need_start_dm);	
	    }
	    else
	    {
	        return  (ATERR_PARAM_INVALID);
	    }
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(30);
		snprintf(*prsp_cmd, 30, "\r\n+QSREGENABLE:%d\r\n\r\nOK\r\n", g_softap_fac_nv->need_start_dm);
	}
    else
    {
        return  (ATERR_PARAM_INVALID);
    }
	
	return AT_END;
}

//BC25对标，AT+QSELFREGISTER    启用/禁用运营商自注册功能
int at_QSELFREGISTER_req(char *at_buf, char **prsp_cmd)
{
    if(g_req_type == AT_CMD_REQ)
    {
        int mode = -1;
        if (at_parse_param("%d(0-1)",at_buf,&mode)!= AT_OK || mode == -1)
        {
            return ATERR_PARAM_INVALID;
        }

        g_softap_fac_nv->need_start_dm = mode;
        SAVE_FAC_PARAM(need_start_dm);
    }
    else if(g_req_type == AT_CMD_QUERY)
    {
        *prsp_cmd = xy_malloc(30);
        snprintf(*prsp_cmd, 30, "\r\n+QSELFREGISTER: %d\r\n\r\nOK\r\n", g_softap_fac_nv->need_start_dm);
    }
    else
        return ATERR_PARAM_INVALID;

    return AT_END;
}


//extern unsigned int g_cmcc_dm_rtcflag;
void dm_netif_event_callback(PsStateChangeEvent event)
{
	int uicc_type = UICC_UNKNOWN;
	
	xy_get_UICC_TYPE(&uicc_type);
	xy_printf(0,XYAPP, INFO_LOG, "dm_netif_event_callback, netif up,uicc_type=%d\n",uicc_type);

	switch(uicc_type)
	{
#if XY_DM 
    case UICC_UNICOM:
	case UICC_TELECOM:
		if(!Is_WakeUp_From_Dsleep())
			tele_uni_dm_start_task(uicc_type);
		break;
#endif
#if MOBILE_VER && XY_DM
	case UICC_MOBILE: //mobile
	    if(!Is_WakeUp_From_Dsleep())
	        cmcc_dm_init();
	break;
#endif
	default:
		{
			//xy_assert(0);
			break;
		}
	}
}

void dm_ctl_init(void)
{
	osMutexAttr_t mutex_attr = {0};
	mutex_attr.attr_bits = osMutexRecursive;
	
	if(g_tudm_context_mutex == NULL)
	{
		g_tudm_context_mutex = osMutexNew(&mutex_attr);
	}
	
	if(g_softap_fac_nv->need_start_dm == 1) //dm switch
	{
		xy_reg_psnetif_callback(EVENT_PSNETIF_VALID, dm_netif_event_callback);
	}
}


int at_XYDMCFG_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		int32_t ret = DM_PROC_FAILED;
		int32_t set_mode = -1;
		dm_context_t* dm_pcontext = NULL;
		char * host = NULL;
		uint8_t * jsonStr = NULL;

		if(g_dm_TskHandle != NULL)//DM运行状态,不允许进行参数设置
		{
			xy_printf(0,XYAPP, WARN_LOG, "[TUDM]at_XYDMCFG_req,dm task is running,not allow setting!");
		}

		if(dm_context_init() != DM_PROC_SUCCESS)
		{
			xy_printf(0,XYAPP, WARN_LOG, "[TUDM]at_XYDMCFG_req,dm_context_init err");
			ret = DM_AT_INITIALIZE_ERR;
			goto req_exit;
		}

		if(at_parse_param("%d(0-2)", at_buf, &set_mode) != AT_OK)
		{
			xy_printf(0,XYAPP, WARN_LOG, "[TUDM]at_XYDMCFG_req,at_parse_param set_mode err");
			ret = DM_AT_PARAMS_VALUE_ERR;
			goto req_exit;
		}

		dm_pcontext = dm_get_context();

		switch (set_mode)
		{
			case DM_SET_SETTING_CFG://设置DM开关,配置文件所属运营商,平台连接参数
			{
				uint8_t dm_switch = 0;									//DM开关,必选参数
				uint8_t setting_uicc_owner = 0;							//DM配置文件所属运营商，必选参数
				uint8_t  retry_num = dm_get_retry_num_def_val();		//可选参数,初始化为默认值
				uint16_t retry_time = dm_get_retry_time_def_val();		//可选参数,初始化为默认值
				uint32_t uni_reg_time = dm_get_uni_reg_time_def_val();	//可选参数,初始化为默认值
				uint16_t port = dm_get_port_def_val();					//可选参数,初始化为默认值

				host = xy_malloc(DM_HOST_NAME_LEN);
				memset(host, 0x00, DM_HOST_NAME_LEN);	

				if(at_parse_param(",%1d(0|1),%1d(1|2), %1d[0-10], %2d[1800-7200], %d[86400-5184000], %41s[], %2d[1-65535]", at_buf, &dm_switch,&setting_uicc_owner,&retry_num, &retry_time, &uni_reg_time, host, &port) != AT_OK)
				{
					xy_printf(0,XYAPP, WARN_LOG, "[TUDM]at_XYDMCFG_req,at_parse_param DM_SET_SETTING_CFG,err!");
					ret = DM_AT_PARAMS_VALUE_ERR;
					goto req_exit;
				}

				//检测setting_owner与当前sim卡运营商是否匹配
				if(dm_check_if_setting_owner_valid(setting_uicc_owner) == false)
				{
					xy_printf(0,XYAPP, WARN_LOG, "[TUDM]at_XYDMCFG_req,DM_SET_SETTING_CFG,dm_check_if_setting_owner_valid err!");
					ret = DM_AT_SETTING_TYPE_ERR;
					goto req_exit;					
				}

				if(strlen(host) == 0)
				{
					dm_get_host_def_val(host);
				}
				else
				{
					if(dm_check_if_dm_host_valid(host) == false)//检测host值是否合法
					{
						xy_printf(0,XYAPP, WARN_LOG, "[TUDM]at_XYDMCFG_req,dm_check_if_dm_host_valid err!");
						ret = DM_AT_PARAMS_VALUE_ERR;
						goto req_exit;
					}
				}	


				dm_set_setting_cfg_params(setting_uicc_owner, retry_num,retry_time, uni_reg_time, port, host);
				dm_set_switch(dm_switch);
				ret = DM_PROC_SUCCESS;
				break;
			}
			case DM_SET_JSON_PARAMS:
			{
				char regver[DM_REG_VER_LEN] = {0};
				char uetype[DM_UETYPE_LEN]={0};
				char swver[DM_SW_VER_LEN]={0};
				char modver[DM_MOUDLE_VER_LEN]={0};
				char osver[DM_OS_VER_LEN]={0};
				char imei[DM_IMEI_LEN]={0};

				if(at_parse_param(", %3s[], %5s[], %31s[], %21s[], %33s[], %16s[]", at_buf, regver, uetype, swver, modver, osver, imei) != AT_OK)
				{
					xy_printf(0,XYAPP, WARN_LOG, "at_XYDMCFG_req,DM_SET_JSON_PARAMS params err");
					ret = DM_AT_PARAMS_VALUE_ERR;
					goto req_exit;
				}

				if(dm_check_if_cfg_uicc_owner_match_uicc_type() == false)
				{
					xy_printf(0,XYAPP, WARN_LOG, "[TUDM]at_XYDMCFG_req,case %d dm_check_if_cfg_uicc_owner_match_uicc_type err",set_mode);
					ret = DM_AT_SETTING_TYPE_ERR;
					goto req_exit;
				}

				//可选参数若为空,获取系统默认的内容
				if(strlen(regver) == 0)
					dm_get_regver_def_str(regver);
				
				if(strlen(uetype) == 0)
					dm_get_uetype_def_str(uetype);

				if(strlen(swver) == 0)
					dm_get_swver_def_str(swver);

				if(strlen(modver) == 0)
					dm_get_modver_def_str(modver);

				if(strlen(osver) == 0)
					dm_get_osver_def_str(osver);

				if(strlen(imei) == 0)
				{
					if(dm_get_imei_def_str(imei) == DM_PROC_FAILED)
					{
						xy_printf(0,XYAPP, WARN_LOG, "[TUDM]at_XYDMCFG_req,dm_get_imei_def_str err");
						ret = DM_AT_PARAMS_VALUE_ERR;
						goto req_exit;
					}
				}

				//以json字符串的格式保存上述参数
				dm_set_params_in_jsonstr_format(regver, uetype, swver, modver, osver, imei);
				
				ret = DM_PROC_SUCCESS;
				break;
			}
			case DM_SET_JSON_STRING://设置用户自定义的json内容,检查是否为合法字符串，透传json字符串内容
			{
				jsonStr = xy_malloc(401);
				memset(jsonStr,0x00,401);

				if (at_parse_param(",%401s", at_buf, jsonStr) != AT_OK)
				{
					xy_printf(0,XYAPP, WARN_LOG, "at_XYDMCFG_req,at_parse_param jsonstr params err");
					ret = DM_AT_PARAMS_VALUE_ERR;
					goto req_exit;
				}

				if(dm_check_if_cfg_uicc_owner_match_uicc_type() == false)
				{
					xy_printf(0,XYAPP, WARN_LOG, "[TUDM]at_XYDMCFG_req,case %d dm_check_if_cfg_uicc_owner_match_uicc_type err",set_mode);
					ret = DM_AT_SETTING_TYPE_ERR;
					goto req_exit;
				}
				
				if(dm_set_others_json_context(jsonStr) != DM_PROC_SUCCESS)
				{
					xy_printf(0,XYAPP, WARN_LOG, "[at_XYDMCFG_req],dm_set_others_json_context err!");
					ret = DM_AT_PARAMS_VALUE_ERR;
					goto req_exit;
				}
				
				
				ret = DM_PROC_SUCCESS;
				break;
			}
		}
req_exit:
		if(ret != DM_PROC_SUCCESS)
		{
			*prsp_cmd = AT_ERR_BUILD(ret);
		}

		if(jsonStr)
		{
			xy_free(jsonStr);
		}
		if(host)
		{
			xy_free(host);
		}
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		

		if(dm_context_init() != DM_PROC_SUCCESS)
		{
			*prsp_cmd = AT_ERR_BUILD(DM_AT_INITIALIZE_ERR);
			xy_printf(0,XYAPP, WARN_LOG, "[TUDM]at_XYDMCFG_req,dm_context_init err!");

			return AT_END;
		}
		
		dm_context_t* dm_pcontext = dm_get_context();

		if(dm_pcontext->report_mode == XINYI_MESSAGE_FORMAT)
		{
			*prsp_cmd = xy_malloc(360);
			
			int n = 0;
			n = sprintf(*prsp_cmd,"\r\n+DMCFG: 0,%d,%d,%d,%d,%d,%s,%d\r\n",g_softap_fac_nv->need_start_dm,dm_pcontext->setting_uicc_owner,dm_pcontext->retry_num,dm_pcontext->retry_time,dm_pcontext->uni_reg_time,dm_pcontext->host,dm_pcontext->port);

			char regver[DM_REG_VER_LEN] = {0};
			char uetype[DM_UETYPE_LEN]={0};
			char swver[DM_SW_VER_LEN]={0};
			char modver[DM_MOUDLE_VER_LEN]={0};
			char osver[DM_OS_VER_LEN]={0};
			char imei[DM_IMEI_LEN]={0};
			
			dm_get_regver(regver);
			dm_get_uetype(uetype);
			dm_get_swver(swver);
			dm_get_model(modver);
			dm_get_osver(osver);
			dm_get_imei(imei);

			n += sprintf(*prsp_cmd + n, "+DMCFG: 1,%s,%s,%s,%s,%s,%s\r\n+DMCFG: 2,\r\n\r\nOK\r\n", regver, uetype, swver, modver, osver, imei);
		}
		else//dm_pcontext->report_mode == USER_MESSAGE_FORMAT
		{
			*prsp_cmd = xy_malloc(strlen(dm_pcontext->json_report_str) +160);

			int n = 0;
			n = sprintf(*prsp_cmd,"\r\n+DMCFG: 0,%d,%d,%d,%d,%d,%s,%d\r\n",g_softap_fac_nv->need_start_dm,dm_pcontext->setting_uicc_owner,dm_pcontext->retry_num,dm_pcontext->retry_time,dm_pcontext->uni_reg_time,dm_pcontext->host,dm_pcontext->port);
			n += sprintf(*prsp_cmd + n, "+DMCFG: 1,\r\n+DMCFG: 2,%s\r\n\r\nOK\r\n", dm_pcontext->json_report_str);
		}
	}
#if (AT_CUT != 1)
	else if(g_req_type == AT_CMD_TEST)
	{
		int32_t len = 240 + strlen(DM_AT_TEST_JSON_STR);
		*prsp_cmd = xy_malloc(len);

		int n = 0;
		n = sprintf(*prsp_cmd, "\r\n+DMCFG: 0,(0,1),(1,2),(0-10),(1800-7200),(86400-5184000),"",(1-65535)\r\n");
		n+= sprintf(*prsp_cmd + n,"+DMCFG: 1,("","","","","","")\r\n");
		n+= sprintf(*prsp_cmd + n,"+DMCFG: 2,(\"%s\")\r\n\r\nOK\r\n",DM_AT_TEST_JSON_STR);
	}
#endif
	else
	{
		return ATERR_PARAM_INVALID;
	}
	return AT_END;
}