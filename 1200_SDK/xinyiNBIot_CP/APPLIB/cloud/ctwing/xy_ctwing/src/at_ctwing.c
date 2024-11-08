#include "at_ctwing.h"

#include "xy_utils.h"
#include "xy_at_api.h"
#include "ctlw_lwm2m_sdk.h"
#include "xy_net_api.h"
#include "oss_nv.h"
#include "net_app_resume.h"
#include "ctwing_util.h"


#define CTLW_AT_ERR_BUILD(a) ctlw_at_err_build_info(a, __FILE__, __LINE__)

extern osThreadId_t g_ctlw_dns_tsk_handle;
extern char g_ctlw_dns_ip[XY_CTLW_DNS_IP_MAX_LEN];
extern char g_ctlw_host_name[XY_CTLW_HOSTNAME_MAX_LEN];
extern uint8_t g_ctlw_dns_req_ip_type;



char *ctlw_at_err_build_info(int err_no, char *file, int line)
{
    (void)line;
	char *at_str = NULL;

	xy_assert(err_no != 0);
	at_str = xy_malloc(64);

	sprintf(at_str, "\r\n+CTLW ERROR: %d\r\n", err_no);

	if (HWREGB(BAK_MEM_XY_DUMP) == 1)
	{
		char xy_file[20] = {0};
		if (strlen(file) >= 20)
			memcpy(xy_file, file + strlen(file) - 19, 19);
		else
			memcpy(xy_file, file, strlen(file));
		xy_printf(0,XYAPP, WARN_LOG,"+CTLW ERROR:%d,line:%d,file:%s\r\n", err_no, line, xy_file);
	}

	return at_str;
}


/**
 * @brief 查询模组接入物联网开放平台软件版本
 * @note AT+CTLWVER?
 */
int at_CTLWVER_req(char *at_buf, char **prsp_cmd)
{
	int32_t ret = xy_ctlw_module_entry();

	if(ret != CTIOT_NB_SUCCESS)
	{
		*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		return AT_END;
	}

	if(g_req_type != AT_CMD_QUERY)
	{
		*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_OPERATOR_NOT_SUPPORTED);
		return AT_END;
	}
	
	*prsp_cmd = xy_malloc(120);

	ret = ctiot_query_version(*prsp_cmd);

    if (ret != CTIOT_NB_SUCCESS)
    {
		xy_free(*prsp_cmd);
        *prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		return AT_END;
    }

	snprintf(*prsp_cmd,120,"%s\r\nOK\r\n",*prsp_cmd);
    return AT_END;
}


/**
 * @brief 从 DNS 获取物联网开放平台 IP 地址
 * @note AT+CTLWGETSRVFRMDNS=<dns_server_ip>,<lw_server_dn> [,<ip_type>]
 */
int at_CTLWGETSRVFRMDNS_req(char *at_buf, char **prsp_cmd)
{
	int32_t ret = xy_ctlw_module_entry();

	if(ret != CTIOT_NB_SUCCESS)
	{
		*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		return AT_END;
	}

    if(g_req_type == AT_CMD_REQ)
    {   
		int ipType = 0;
		int result = -1;
        
		/*dns server ip maxsize is 46,hostname maxsize is 32*/
        if (at_parse_param("%47s(),%33s(),%d[0-2]", at_buf, g_ctlw_dns_ip, g_ctlw_host_name, &ipType) != AT_OK)
        {
            *prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
            return AT_END;
        }

		if(xy_get_IpAddr_type(g_ctlw_dns_ip) == -1)//g_ctlw_dns_ip不是合法的IPV4或者V6地址
        {
            *prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
            return AT_END;
        }

		g_ctlw_dns_req_ip_type = ipType;
           
		if(g_ctlw_dns_tsk_handle == NULL)
		{
			osThreadAttr_t thread_attr = {0};
			thread_attr.name       = "ctlw_dns_task";
			thread_attr.priority   = osPriorityNormal1;
			thread_attr.stack_size = osStackShared;
			g_ctlw_dns_tsk_handle = osThreadNew((osThreadFunc_t)(xy_ctlw_dns_task), NULL, &thread_attr);
		}
		else
		{
			*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_DNS_ING_ERROR);
		}
    }
#if (AT_CUT != 1)
    else if(g_req_type == AT_CMD_TEST)
    {
        *prsp_cmd = xy_malloc(100);
        snprintf(*prsp_cmd, 100, "(0-2)");
    }
#endif
    else
        *prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_OPERATOR_NOT_SUPPORTED);
    
    return AT_END;
}


/**
 * @brief 初始化接入物联网开放平台IP地址,设置、删除模组接入平台IP地址参数
 * 
 * @note AT+CTLWSETSERVER=<action_type>,<ip_type>[,<sever_ip>[,<port>]]
 */
int at_CTLWSETSERVER_req(char *at_buf, char **prsp_cmd)
{
	int32_t ret = xy_ctlw_module_entry();

	if(ret != CTIOT_NB_SUCCESS)
	{
		*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		return AT_END;
	}

	if(g_req_type == AT_CMD_REQ)
	{
		uint8_t action = -1;
		uint8_t ipType = -1;
		uint8_t *serverIP = NULL;
		
		serverIP = xy_malloc(47);
		memset(serverIP, 0x00, 47);
		
		ctiot_context_t *pContext = xy_ctlw_ctiot_get_context();
		int32_t port = CTIOT_DEFAULT_PORT;
#if	CTIOT_CHIPSUPPORT_DTLS == 1
		if(pContext->connectionType == MODE_DTLS)
			port = CTIOT_DEFAULT_DTLS_PORT;
#endif		

		if(at_parse_param("%1d(0-1),%1d(0-1),%47s,%d[1-65535]",at_buf,&action,&ipType,serverIP,&port)!= AT_OK)
        {
            *prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
			goto exit;
        }

		if(action == 0 && strlen(serverIP) == 0)
        {
            *prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
            goto exit;
        }

		//iptye 0:v4,1:v6
		Netif_IPType_T type = ipType == 0 ? IPV4_TYPE:IPV6_TYPE;

		if((action == 0) && xy_get_IpAddr_type(serverIP) != type)
		{
			*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
			goto exit;
		}

		ret = ctiot_set_server(action, ipType, serverIP, port);

		if(ret != CTIOT_NB_SUCCESS)
			 *prsp_cmd = CTLW_AT_ERR_BUILD(ret);
exit:
		if(serverIP)
			xy_free(serverIP);
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(150);

		ret=ctiot_get_server(*prsp_cmd);

		if(ret != CTIOT_NB_SUCCESS)
		{
			xy_free(*prsp_cmd);
			*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		}
		else
		{
			sprintf(*prsp_cmd,"%s\r\nOK\r\n" , *prsp_cmd);
		}
		
	}
#if (AT_CUT != 1)
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(80);
		snprintf(*prsp_cmd, 80, "(0-1),(0-1),,(0-65535)");
	}
#endif
    else 
    {
        *prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_OPERATOR_NOT_SUPPORTED);
    }
    
	return AT_END;
}


/**
 * @brief 初始化接入物联网开放平台 LIFETIME
 * 
 * @note AT+CTLWSETLT=<lifetime>
 */
int at_CTLWSETLT_req(char *at_buf, char **prsp_cmd)
{
	int32_t ret = xy_ctlw_module_entry();
	
	if(ret != CTIOT_NB_SUCCESS)
	{
		*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		return AT_END;
	}
	
	if(g_req_type == AT_CMD_REQ)
	{
		int32_t lifetime = -1;
        
		if(at_parse_param("%d(300-2592000)", at_buf, &lifetime) != AT_OK)
		{
		    *prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
			return AT_END;
		}	
		
		ret = ctiot_set_lifetime(lifetime);

		if(ret != CTIOT_NB_SUCCESS)
		{
			 *prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		}
	}
	else if(g_req_type == AT_CMD_QUERY)
	{

		uint8_t *lfquery = xy_malloc(50);
		
		ret = ctiot_get_lifetime(lfquery);

		if(ret == CTIOT_NB_SUCCESS)
		{	
			*prsp_cmd = xy_malloc(50);
		   	snprintf(*prsp_cmd,50,"%s\r\nOK\r\n",lfquery);
		}
		else
			 *prsp_cmd = CTLW_AT_ERR_BUILD(ret);

		xy_free(lfquery);
	}
#if (AT_CUT != 1)
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(80);
		snprintf(*prsp_cmd, 80, "(300-2592000)");
	}
#endif
	else
	 	*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_OPERATOR_NOT_SUPPORTED);

	return AT_END;
}


/**
 * @brief DTLS 参数设置
 * 
 * @note AT+CTLWSETPSK=<mode>,<PSK> [,<PSKID>]
 */
int at_CTLWSETPSK_req(char *at_buf, char **prsp_cmd)
{
    uint8_t mode = -1;
	uint8_t *psk = xy_malloc(33);
	uint8_t *pskid = xy_malloc(33);
	memset(pskid,0x00, 33);

	int32_t ret = xy_ctlw_module_entry();

	if(ret != CTIOT_NB_SUCCESS)
	{
		*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		return AT_END;
	}

    if(g_req_type == AT_CMD_REQ)
    {
        if(at_parse_param("%1d(0-1),%33s(),%33s", at_buf, &mode, psk, pskid)!=AT_OK)
        {
			*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
            goto PROC;
        }

		if((mode == 0 && strlen(psk) > 16))
        {
            *prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
            goto PROC;
        }

        if(strlen(pskid) == 0)
        {
        	ret = ctiot_set_psk(mode, NULL, psk);
        }
        else
		{
			if(strlen(pskid) > XY_CTLW_PSKID_LEN)
			{
				*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
				goto PROC;
			}

        	ret = ctiot_set_psk(mode, pskid, psk);
		}

        if(CTIOT_NB_SUCCESS != ret)
        {
            *prsp_cmd = CTLW_AT_ERR_BUILD(ret);
            goto PROC;
        }
    }
#if (AT_CUT != 1)
	else if(g_req_type == AT_CMD_TEST)
    {
        *prsp_cmd = xy_malloc(120);
        snprintf(*prsp_cmd,120,"(0-1)");
    }
#endif
    else
        *prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_OPERATOR_NOT_SUPPORTED);
    
PROC:
    if(psk)
        xy_free(psk);
    if(pskid)
        xy_free(pskid);
	
    return AT_END;

}


/**
 * @brief 终端扩展认证参数设置（模组提供认证串）
 * 
 * @note AT+CTLWSETAUTH=<auth type>
 */
int at_CTLWSETAUTH_req(char *at_buf, char **prsp_cmd)
{
	int32_t ret = xy_ctlw_module_entry();

	if(ret != CTIOT_NB_SUCCESS)
	{
		*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		return AT_END;
	}

	if(g_req_type == AT_CMD_REQ)
	{
		int authType = -1;

		if(at_parse_param("%d(0-2)",at_buf,&authType) != AT_OK)
		{
			*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
			return AT_END;
		}

		ret=ctiot_set_auth_type(authType);

		if(ret != CTIOT_NB_SUCCESS)
		{
			*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
			return AT_END;
		}
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		char *queryResult = xy_malloc(30);

		ret = ctiot_get_auth_type(queryResult);

		if(ret != CTIOT_NB_SUCCESS)
		{
			*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
			xy_free(queryResult);
			return AT_END;
		}

		*prsp_cmd = xy_malloc(50);
		snprintf(*prsp_cmd,50,"%s\r\n",queryResult);
		strcat(*prsp_cmd, "OK\r\n");	
		xy_free(queryResult);
	}
#if (AT_CUT != 1)
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(50);
		snprintf(*prsp_cmd,50,"(0-2)");
	}
#endif
	else
		*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_OPERATOR_NOT_SUPPORTED);

	return AT_END;
}


/**
 * @brief 终端报文加密参数设置（模组加密收发报文PAYLOAD）
 * 
 * @note AT+CTLWSETPCRYPT=<type>，<value>
 */
int at_CTLWSETPCRYPT_req(char *at_buf, char **prsp_cmd)
{
	int32_t ret = xy_ctlw_module_entry();

	if(ret != CTIOT_NB_SUCCESS)
	{
		*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		return AT_END;
	}
	if(g_req_type == AT_CMD_REQ)
	{
		uint8_t type = -1;
		char *value = xy_malloc(65);
		memset(value, 0x00, 65);

		if(at_parse_param("%1d(0-1),%65s()",at_buf, &type, value)!= AT_OK)
        {
			*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
            xy_free(value);
            return AT_END;
        }


        if(type == 0 && (strcmp(value, "0") && strcmp(value, "1")))
        {
            *prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
            xy_free(value);
            return AT_END;
        }    

		ret = ctiot_set_payload_encrypt(type, value);

		if(ret != CTIOT_NB_SUCCESS)
			*prsp_cmd = CTLW_AT_ERR_BUILD(ret);

		xy_free(value);
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(150);

		ret = ctiot_get_payload_encrypt(*prsp_cmd);

		if(ret != CTIOT_NB_SUCCESS)
		{
			xy_free(*prsp_cmd);
			*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		}
		else
		{
			sprintf(*prsp_cmd,"%s\r\nOK\r\n", *prsp_cmd);
		}
		 
	}
#if (AT_CUT != 1)
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(80);
		snprintf(*prsp_cmd, 80, "(0-1)");
	}
#endif
    else 
    {
        *prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_OPERATOR_NOT_SUPPORTED);
    }
    
	return AT_END;
}


/**
 * @brief 初始化模组会话模式
 * 
 * @note AT+CTLWSETMOD=<MOD_ID>,<MOD_DATA>
 */
int at_CTLWSETMOD_req(char *at_buf, char **prsp_cmd)
{
	int32_t ret = xy_ctlw_module_entry();

	if(ret != CTIOT_NB_SUCCESS)
	{
		*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		return AT_END;
	}
	if(g_req_type == AT_CMD_REQ)
	{
		int mode_id = -1;
		int mode_value = -1;

		if(at_parse_param("%d(1-5),%d()", at_buf, &mode_id, &mode_value) != AT_OK)
		{
			*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
			return AT_END;
		}

		//终端endpoint模式不支持扩展认证方式1和扩展认证方式2；加密模式不支持收发报文加密和DTLS+
		if((mode_id == AUTH_PROTOCOL_TYPE && (mode_value == AUTHMODE_EXTEND_MCU || mode_value == AUTHMODE_EXTEND_MODULE))
			|| (mode_id == MODE_DTLS_TYPE && (mode_value == MODE_ENCRYPTED_PAYLOAD || mode_value == MODE_DTLS_PLUS)))
		{

			*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_NOSUP_ERROR);
			return AT_END;
	
		}

        ret = ctiot_set_mod(mode_id, mode_value);

		if(ret != CTIOT_NB_SUCCESS)
		{
			 *prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		} 
	}
	else if(g_req_type == AT_CMD_QUERY)
	{

		uint8_t *getMode = xy_malloc(110);
		
		if(ctiot_get_mod(getMode) == CTIOT_NB_SUCCESS)
		{
			*prsp_cmd = xy_malloc(120);

			snprintf(*prsp_cmd, 120,"%s\r\nOK\r\n", getMode);
		}
		else
			 *prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);

		xy_free(getMode);
	}
#if (AT_CUT != 1)
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(150);
		snprintf(*prsp_cmd, 150, "1,(0-2)\r\n+CTLWSETMOD: 2,(0-1)\r\n+CTLWSETMOD: 3,(0-4)\r\n+CTLWSETMOD: 4,(0-2,9)\r\n+CTLWSETMOD: 5,(0-1)");
	}
#endif
    else 
    {
        *prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_OPERATOR_NOT_SUPPORTED);
    }
	return AT_END;

}

/**
 * @brief 登录物联网开放平台
 * 
 * @note AT+CTLWREG[=<auth mode str>,<auth token str>]
 */
int at_CTLWREG_req(char *at_buf, char **prsp_cmd)
{
    char *regModeStr = NULL;
    char *regAuthStr = NULL;
    char *regAuthStr_temp = NULL;
    int32_t ret = xy_ctlw_module_entry();

	if(ret != CTIOT_NB_SUCCESS)
	{
		*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		return AT_END;
	}

	if(g_req_type == AT_CMD_REQ)//不支持注册时，endpoint为带参数的扩展认证方式1
	{
		*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_NOSUP_ERROR);
		return AT_END;
	}
	else if(g_req_type == AT_CMD_ACTIVE)
	{
		ret = ctiot_reg(NULL, NULL);
        if(ret != CTIOT_NB_SUCCESS)
        {
            *prsp_cmd = CTLW_AT_ERR_BUILD(ret);
        }
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		uint8_t *reg_status = xy_malloc(30);

		ret=ctiot_get_reg_status(reg_status);

		if(ret == CTIOT_NB_SUCCESS)
		{
			*prsp_cmd = xy_malloc(60);
			snprintf(*prsp_cmd,60,"%s\r\nOK\r\n",reg_status);
		}
		else
			*prsp_cmd = CTLW_AT_ERR_BUILD(ret);

		xy_free(reg_status);		
	}
#if (AT_CUT != 1)
	else if(g_req_type == AT_CMD_TEST)
	{
		return AT_END;
	}
#endif
	else
		*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_OPERATOR_NOT_SUPPORTED);

exit:
    if(regModeStr != NULL)
        xy_free(regModeStr);
    if(regAuthStr != NULL)
        xy_free(regAuthStr);
    if(regAuthStr_temp != NULL)
        xy_free(regAuthStr_temp);

	return AT_END;

}



/**
 * @brief 通知模组向平台发送心跳或延长已有的LIFETIME
 * 
 * @note AT+CTLWUPDATE[=<RAI Indication>]
 * 
 * <RAI Indication>：整型(缺省为0)，0 –常规模式； 1- 发出报文后收到下行的报文后释放空口（RAI）
 */
int at_CTLWUPDATE_req(char *at_buf, char **prsp_cmd)
{
	uint16_t msgid = 0;

	int32_t ret = xy_ctlw_module_entry();
	
	if(ret != CTIOT_NB_SUCCESS)
	{
		*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		return AT_END;
	}
	if(g_req_type == AT_CMD_REQ)
	{
		uint8_t raiIndex = -1;	
		if(at_parse_param("%1d(0-1)", at_buf, &raiIndex) != AT_OK)
        {
			*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
            return AT_END;
        }

		ret = ctiot_update(raiIndex, &msgid);
		if(ret != CTIOT_NB_SUCCESS)
		{
			 *prsp_cmd = CTLW_AT_ERR_BUILD(ret);
			 return AT_END;
		}

		*prsp_cmd = xy_malloc(50);
		snprintf(*prsp_cmd, 50, "%d",msgid);
	}
	else if(g_req_type == AT_CMD_ACTIVE)
	{
		ret = ctiot_update(0, &msgid);
		if(ret != CTIOT_NB_SUCCESS)
		{
			 *prsp_cmd = CTLW_AT_ERR_BUILD(ret);
			 return AT_END;
		}

		*prsp_cmd = xy_malloc(50);
		snprintf(*prsp_cmd, 50, "%d",msgid);
		
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
	  *prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_OPERATOR_NOT_SUPPORTED);
	  return AT_END;
		
	}
#if (AT_CUT != 1)
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(80);
		snprintf(*prsp_cmd, 80, "(0-1)");
	}
#endif
    
	return AT_END;
}


/**
 * @brief 模组主动发送DTLS HS操作,用于定期刷新DTLS会话的上下文数据，增强安全性
 * 或者实现DTLS Server要求的某些心跳机制
 * 
 * @note AT+CTLWDTLSHS
 */
int at_CTLWDTLSHS_req(char *at_buf, char **prsp_cmd)
{
	int32_t ret = xy_ctlw_module_entry();

	if(ret != CTIOT_NB_SUCCESS)
	{
		*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		return AT_END;
	}
	if(g_req_type == AT_CMD_ACTIVE)
	{
		ret = ctiot_at_dtls_hs();

		if(ret != CTIOT_NB_SUCCESS)
		{
			 *prsp_cmd = CTLW_AT_ERR_BUILD(ret);
			 return AT_END;
		}
		
	}
#if (AT_CUT != 1)
	else if(g_req_type == AT_CMD_TEST)
	{
		return AT_END;
	}
#endif
	else
		*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_OPERATOR_NOT_SUPPORTED);

	return AT_END;
}


/**
 * @brief 取消登录物联网开放平台(登出平台)
 * 
 * @note AT+CTLWDEREG[=mode]
 * 
 * <mode>：整型，（缺省为0-常规DEREG）
 * 常规DEREG, 适用于MCU登出平台后需要过一段时间再登录平台的场景
 * 本地DEREG，适用于MCU需要DEREG后马上登录平台的场景
 */
int at_CTLWDEREG_req(char *at_buf, char **prsp_cmd)
{
	int32_t ret = xy_ctlw_module_entry();

	if(ret != CTIOT_NB_SUCCESS)
	{
		*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		return AT_END;
	}
	if(g_req_type == AT_CMD_REQ)
    {
        uint8_t dereg_mode = -1;

        if(at_parse_param("%1d(0-1)", at_buf, &dereg_mode) != AT_OK)
        {
			*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
            return AT_END;
        }


        ret = ctiot_dereg(dereg_mode);

        if(ret != CTIOT_NB_SUCCESS)
        {
        	*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
        }
    }
    else if(g_req_type == AT_CMD_ACTIVE)
    {
        ret = ctiot_dereg(0);
        if(ret != CTIOT_NB_SUCCESS)
        {
        	*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
        }
    }
#if (AT_CUT != 1)
	else if(g_req_type == AT_CMD_TEST)
    {
		*prsp_cmd = xy_malloc(80);
		snprintf(*prsp_cmd, 80, "(0-1)");
    }
#endif
    else
        *prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_OPERATOR_NOT_SUPPORTED);
    
	return AT_END;
}


/**
 * @brief 状态查询
 * 
 * @note AT+CTLWGETSTATUS=<query_type> [,<data1>]
 */
int at_CTLWGETSTATUS_req(char *at_buf, char **prsp_cmd)
{
    uint8_t query_type = -1;
    int32_t msgid = -1;
    uint8_t *query_result = xy_malloc(70);
	
	int32_t ret = xy_ctlw_module_entry();

    if(g_req_type == AT_CMD_REQ)
    {
        if(at_parse_param("%1d(0-7),%d[0-65535]", at_buf, &query_type, &msgid) != AT_OK)
        {   
			*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
            goto exit;
        }
        
        if(query_type == 2 && msgid == -1)
        {
        	*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
        	goto exit;
        }
		
		if(ret != CTIOT_NB_SUCCESS)
		{
			if(query_type == 1)//查询EPS状态
			{
				*prsp_cmd = xy_malloc(64);
        		snprintf(*prsp_cmd,64,"1,%d",xy_ctlw_sync_cstate());
			}
			else
			{
				*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
			}
			xy_free(query_result);
			return AT_END;
		}
		

        ret = ctiot_get_status(query_type, msgid, query_result);

        if(ret != CTIOT_NB_SUCCESS)
        {
            *prsp_cmd = CTLW_AT_ERR_BUILD(ret);
            goto exit;
        }
        
        *prsp_cmd = xy_malloc(96);
        snprintf(*prsp_cmd,96,"%s\r\nOK\r\n",query_result);
    }
#if (AT_CUT != 1)
	else if(g_req_type==AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(80);
		snprintf(*prsp_cmd, 80, "(0-7)");
	}
#endif
    else
        *prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_OPERATOR_NOT_SUPPORTED);
exit:
    if(query_result)
        xy_free(query_result);
    return AT_END;

}


/**
 * @brief 重置模组接入平台软件配置数据,将持久化存储中的接入平台的配置数据恢复到缺省状态
 * 
 * @note AT+CTLWCFGRST [=<mode>]
 */
int at_CTLWCFGRST_req(char *at_buf, char **prsp_cmd)
{
	int32_t resetMode = -1;
	int32_t ret = xy_ctlw_module_entry();

	if(ret != CTIOT_NB_SUCCESS)
	{
		*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		return AT_END;
	}
	if(g_req_type == AT_CMD_REQ)
	{
		if(at_parse_param("%d(0-0)", at_buf, &resetMode) != AT_OK)
		{
			*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
			return AT_END;
		}
		else
		{
			ret = ctiot_cfg_reset(resetMode);

			if(ret != CTIOT_NB_SUCCESS)
			{
				*prsp_cmd = CTLW_AT_ERR_BUILD(ret);	
			}
		}
	}
	else if(g_req_type == AT_CMD_ACTIVE)
	{
		resetMode = 0;

		ret = ctiot_cfg_reset(resetMode);

		if(ret != CTIOT_NB_SUCCESS)
			*prsp_cmd = CTLW_AT_ERR_BUILD(ret);	
	}
#if (AT_CUT != 1)
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(40);
		snprintf(*prsp_cmd, 40, "(0)");
	}
#endif
	else
		*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_OPERATOR_NOT_SUPPORTED);	

	return AT_END;
}


/**
 * @brief 清除持久化存储中的当前会话数据
 * （只能在“引擎无法工作”状态下使用，使得重启模组后会话处于“未登录”状态）
 * 
 * @note AT+CTLWSESDATA=<action>
 */
int at_CTLWSESDATA_req(char *at_buf, char **prsp_cmd)
{
	int32_t ret = xy_ctlw_module_entry();

	if(ret != CTIOT_NB_SUCCESS)
	{
		*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		return AT_END;
	}
	if(g_req_type == AT_CMD_REQ)
	{
		int action = -1;
		if(at_parse_param("%d(0-0)", at_buf, &action) != AT_OK)
		{
			*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
			return AT_END;
		}

		ret = ctiot_session_data(action);

		if(ret != CTIOT_NB_SUCCESS)
			*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
	}
#if (AT_CUT != 1)
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(40);
		snprintf(*prsp_cmd, 40, "(0)");
	}
#endif
	else
		*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_OPERATOR_NOT_SUPPORTED);
    
	return AT_END;
}


/**
 * @brief 通知模组向平台发送业务数据，支持选择使用CON模式或NON模式
 * 
 * @note AT+CTLWSEND=<data>[,<mode>]<CR>
 */
int at_CTLWSEND_req(char *at_buf, char **prsp_cmd)
{
	int32_t ret = xy_ctlw_module_entry();

	if(ret != CTIOT_NB_SUCCESS)
	{
		*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		return AT_END;
	}
	if(g_req_type == AT_CMD_REQ)
	{
		int send_mode = 0;
		char result_buf[30] = {0};
		char *data = NULL;

		if(is_Uplink_FlowCtl_Open())//流控生效时丢弃上行数据并报错
		{
			*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_SYS_API_ERROR);
			return AT_END;
		}

		if(at_parse_param("%p(),%1d[0-4]", at_buf, &data, &send_mode) != AT_OK)
		{
			*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
			return AT_END;
		}

		ret = ctiot_send(data, send_mode, result_buf);

		if(ret == CTIOT_NB_SUCCESS)
		{
			*prsp_cmd = xy_malloc(40);
			snprintf(*prsp_cmd, 40, "%s\r\nOK\r\n", result_buf);
		}
		else
			*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
	    *prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_OPERATOR_NOT_SUPPORTED);
	  	return AT_END;
	}
#if (AT_CUT != 1)
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(80);
		snprintf(*prsp_cmd, 80, "(0-4)");
	}
#endif
	else
		*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_OPERATOR_NOT_SUPPORTED);
	
	return AT_END;

}


/**
 * @brief 模组使用该指令设置模组接收平台下发数据的模式
 * 
 * @note AT+CTLWRECV=<mode>[,<data1>]
 * <data1>下行报文保存在缓存中的时间
 */
int at_CTLWRECV_req(char *at_buf, char **prsp_cmd)
{
	int32_t ret = xy_ctlw_module_entry();

	if(ret != CTIOT_NB_SUCCESS)
	{
		*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		return AT_END;
	}
	if(g_req_type == AT_CMD_REQ)
	{
        int recv_mode = -1;
        int cache_time_mode = -1;

		if(at_parse_param("%d(0-2),%d[0-8]",at_buf,&recv_mode,&cache_time_mode) != AT_OK)
		{
			*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
			return AT_END;
		}

		if(recv_mode == RECV_DATA_MODE_0 || recv_mode == RECV_DATA_MODE_2)
		{
			ret = ctiot_set_recv_data_mode(recv_mode,CTIOT_TIMEMODE_0);
		}
		else
		{
			if(cache_time_mode == -1)
			{
				ret = ctiot_set_recv_data_mode(recv_mode,CTIOT_TIMEMODE_8);
			}
			else
			{
				ret = ctiot_set_recv_data_mode(recv_mode,cache_time_mode);
			}
		}
		
		if(ret != CTIOT_NB_SUCCESS)
		{
            *prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		}
    }
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(50);
		memset(*prsp_cmd, 0x00, 50);

		int ret = ctiot_get_recv_data_mode(*prsp_cmd);

		if(ret != CTIOT_NB_SUCCESS)
		{
			xy_free(*prsp_cmd);
            *prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		}
		snprintf(*prsp_cmd, 50, "%s\r\nOK\r\n", *prsp_cmd);
	}
#if (AT_CUT != 1)
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(50);
		memset(*prsp_cmd, 0x00, 50);
		snprintf(*prsp_cmd, 50, "(0-2),(0-8)");
	}
#endif
    else
        *prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_OPERATOR_NOT_SUPPORTED);
	
	return AT_END;
}


/**
 * @brief 读取物联网开放平台下发的数据
 * 
 * @note AT+CTLWGETRECVDATA
 */
int at_CTLWGETRECVDATA_req(char *at_buf, char **prsp_cmd)
{
	int32_t ret = xy_ctlw_module_entry();

	if(ret != CTIOT_NB_SUCCESS)
	{
		*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		return AT_END;
	}
	if(g_req_type == AT_CMD_ACTIVE)
	{
        char *recv_buff = xy_malloc2(1060);

		if(recv_buff == NULL)
		{
			*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_SYS_API_ERROR);
			return AT_END;
		}

		ret = ctiot_get_recv_data(recv_buff);

		if(ret == CTIOT_NB_SUCCESS)
		{
		    *prsp_cmd = xy_malloc2(strlen(recv_buff)+20);

			if(*prsp_cmd == NULL)
			{
				*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_SYS_API_ERROR);
				xy_free(recv_buff);
				return AT_END;
			}

		    sprintf(*prsp_cmd,"%s", recv_buff);
		}
        else
            *prsp_cmd = CTLW_AT_ERR_BUILD(ret);
        
        xy_free(recv_buff);
    }
#if (AT_CUT != 1)
	else if(g_req_type == AT_CMD_TEST)
	{
		return AT_END;
	}
#endif
	else
        *prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_OPERATOR_NOT_SUPPORTED);
    
	return AT_END;
}


/**
 * @brief 设置注册模式,0:手动注册连接平台 1:上电自动连接云平台
 * 
 * @note AT+CTLWSETREGMOD=<regmode>
 */
int at_CTLWSETREGMOD_req(char *at_buf, char **prsp_cmd)
{
	int32_t ret = xy_ctlw_module_entry();

	if(ret != CTIOT_NB_SUCCESS)
	{
		*prsp_cmd = CTLW_AT_ERR_BUILD(ret);
		return AT_END;
	}
	if(g_req_type == AT_CMD_REQ)
	{
		char reg_type = -1;
		
		if (at_parse_param("%1d", at_buf, &reg_type) != AT_OK)
		{
			*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
			return AT_END;
		}
		if(reg_type != 0 && reg_type != 1)
		{
			*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_PARA_VALUE_ERROR);
			return AT_END;
		}
		
		//设置注册模式并保存到FS
		xy_ctlw_set_reg_mode(reg_type);
	}
	else if(g_req_type == AT_CMD_QUERY)
	{	
		*prsp_cmd = xy_malloc(40);

		snprintf(*prsp_cmd, 40, "%d",xy_ctlw_get_reg_mode());

	}
#if (AT_CUT != 1)
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(50);
		snprintf(*prsp_cmd, 50, "(0-1)");
	}
#endif
	else
		*prsp_cmd = CTLW_AT_ERR_BUILD(CTIOT_OPERATOR_NOT_SUPPORTED);

	
	return AT_END;
}


