/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_tcpip_cmd.h"
#include "at_tcpip_api.h"
#include "at_com.h"
#include "ps_netif_api.h"
#include "xy_at_api.h"
#include "net_api_priv.h"
#include "xy_socket_api.h"
#include "lwip/dns.h"

/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
/*********************** 芯翼默认DNS/NTP 命令 **********************************/
/* AT+XDNSCFGQ=<pri_dns>[,<sec_dns>] */
int at_XDNSCFG_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		dns_cfg_param_t param = {0};
		param.save = true;
		// DNS地址字符串的最大长度为XY_IPADDR_STRLEN_MAX
		if (at_parse_param("%47p(),%47p", at_buf, &param.pridns, &param.secdns) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}
		return at_dns_config(&param);
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(128);
		char* pri_dns = xy_malloc(XY_IPADDR_STRLEN_MAX);
		char* sec_dns = xy_malloc(XY_IPADDR_STRLEN_MAX);
		memset(pri_dns, 0, XY_IPADDR_STRLEN_MAX);
		memset(sec_dns, 0, XY_IPADDR_STRLEN_MAX);

		xy_dns_get(0, pri_dns);
		xy_dns_get(1, sec_dns);

		snprintf(*prsp_cmd, 128, "+XDNSCFG:%s,%s", pri_dns, sec_dns);

		xy_free(pri_dns);
		xy_free(sec_dns);
		return AT_END;
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

/* AT+XDNS=<domain_name> */
int at_XDNS_req(char *at_buf, char **prsp_cmd)
{
	UNUSED_ARG(prsp_cmd);
	if (g_req_type == AT_CMD_REQ)
	{
		dns_query_param_t arg = {0};
		arg.type = TCPIP_CMD_DEFAULT;
		if (at_parse_param("%p", at_buf, &arg.host) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		if (query_dns(&arg) != XY_OK)
		{
			return ATERR_NOT_NET_CONNECT;
		}
		else
		{
			int ipaddrs_len = strlen(arg.ipaddr_list);
			*prsp_cmd = xy_malloc(ipaddrs_len + 32);
			sprintf(*prsp_cmd, "+XDNS:%s", arg.ipaddr_list);
			xy_free(arg.ipaddr_list);
			return AT_END;
		}
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

/* AT+XNTP=[<domain>[,<port>,[<update_rtc>,[<timeout]]]] */
int at_XNTP_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		ntp_query_param_t param = {0};
		param.update = 1;

		if (g_NITZ_mode != 1)
		{
			return ATERR_NOT_ALLOWED;
		}

		if (at_parse_param("%p,%2d[0-65535],%1d[1-1],%d[0-300]", at_buf, &param.host, &param.port, &param.update, &param.timeout) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		if (query_ntp(&param) != XY_OK)
		{
			return ATERR_NOT_NET_CONNECT;
		}
		else
		{
			ntp_zone_t info = get_zone_info();
			
			// 更新时间快照信息，方便其他业务查询世界时间
			if (1 == param.update)
				reset_universal_timer(&param.rtctime, info.zone_sec);
			/* +XNTP:<time> 例如+XNTP:19/06/05,08:53:37+32 */
			*prsp_cmd = xy_malloc(70);
			sprintf(*prsp_cmd, "+XNTP:%02lu/%02lu/%02lu,%02lu:%02lu:%02lu%s",
					param.rtctime.wall_clock.tm_year - 2000, param.rtctime.wall_clock.tm_mon, param.rtctime.wall_clock.tm_mday, param.rtctime.wall_clock.tm_hour,
					param.rtctime.wall_clock.tm_min, param.rtctime.wall_clock.tm_sec, info.zone);
			return AT_END;
		}
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

/*********************** 移远BC95 DNS/NTP 命令 **********************************/
/* AT+QIDNSCFGQ=<pri_dns>[,<sec_dns>] */
int at_QIDNSCFG_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		dns_cfg_param_t param = {0};
		param.save = false;
		// DNS地址字符串的最大长度为XY_IPADDR_STRLEN_MAX
		if (at_parse_param("%47p(),%47p", at_buf, &param.pridns, &param.secdns) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}
		return at_dns_config(&param);
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(128);
		char *pri_dns = xy_malloc(XY_IPADDR_STRLEN_MAX);
		char *sec_dns = xy_malloc(XY_IPADDR_STRLEN_MAX);
		memset(pri_dns, 0, XY_IPADDR_STRLEN_MAX);
		memset(sec_dns, 0, XY_IPADDR_STRLEN_MAX);

		xy_dns_get(0, pri_dns);
		xy_dns_get(1, sec_dns);
		snprintf(*prsp_cmd, 128, "\r\nPrimaryDns:%s\r\nSecondaryDns:%s", pri_dns, sec_dns);
		xy_free(pri_dns);
		xy_free(sec_dns);
		return AT_END;
	}
#if (AT_CUT != 1)
	else if (g_req_type == AT_CMD_TEST)
	{
		return AT_END;
	}
#endif
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

/* AT+QDNS=[<mode>],<hostname> */
int at_QDNS_req(char *at_buf, char **prsp_cmd)
{
	UNUSED_ARG(prsp_cmd);
	if (g_req_type == AT_CMD_REQ)
	{
		int mode = 0;
		uint32_t domain_name_size = 0;
		dns_query_param_t dns_arg = {0};
		dns_arg.type = TCPIP_CMD_QUECTEL;

		if (at_parse_param("%d(0-2),%p", at_buf, &mode, &dns_arg.host) != AT_OK 
			 || (dns_arg.host != NULL && (domain_name_size = strlen(dns_arg.host)) > DNS_MAX_NAME_LENGTH))
		{
			return ATERR_PARAM_INVALID;
		}

		if (mode == 1)
		{
			if (domain_name_size == 0)
				dns_clear_cache(NULL);
			else
				dns_clear_cache(dns_arg.host);
			return AT_END;
		}
		else // mode == 0/2
		{
			if (domain_name_size == 0)
			{
				return ATERR_PARAM_INVALID;
			}
			/* mode=2 清除dns cahce */
			dns_arg.nocache = (mode == 2) ? 1 : 0;
			return at_query_dns_async(&dns_arg);
		}
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

/*********************** 中移物联DNS/NTP 命令 **********************************/
/* AT+CMNTP=[<domain>[,<port>,[<update_rtc>,[<timeout]]]] */
int at_CMNTP_req(char *at_buf, char **prsp_cmd)
{
	UNUSED_ARG(prsp_cmd);
	if (g_req_type == AT_CMD_REQ || g_req_type == AT_CMD_ACTIVE)
	{
		ntp_query_param_t param = {0};
		param.update = 1;
		param.type = TCPIP_CMD_CMCC;

		if (at_parse_param("%p,%2d[0-65535],%1d[1-1],%d[0-300]", at_buf, &param.host, &param.port, &param.update, &param.timeout) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		// 存在host为NULL的情况时，使用默认的NTP服务器
		if (param.host == NULL || strlen(param.host) == 0)
		{
			param.host = XY_DEFAULT_NTP_SERVER;
		}

		return at_query_ntp_async(&param);
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

/* AT+CMDNS=<domain_name>[,<pri_dns>,<sec_dns>,<port>,<interval_time>] */
int at_CMDNS_req(char *at_buf, char **prsp_cmd)
{
	UNUSED_ARG(prsp_cmd);
	if (g_req_type == AT_CMD_REQ)
	{
		int port = 0;
		int ret = AT_END;
		dns_cfg_param_t dns_cfg = {0};
		dns_cfg.save = true;
		dns_query_param_t dns_arg = {0};
		dns_arg.type = TCPIP_CMD_CMCC;
		dns_arg.timeout = 30;
		
		if (at_parse_param("%p,%p,%p,%d[],%2d[]", at_buf, &dns_arg.host, &dns_cfg.pridns, &dns_cfg.secdns, &port, &dns_arg.timeout) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		/* CMDNS配置dns保存到文件系统 */
		if (dns_cfg.pridns != NULL && (ret = at_dns_config(&dns_cfg)) != AT_END)
		{
			return ret;
		}
		
		return at_query_dns_async(&dns_arg);
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

/*********************** 移远BC25 命令 **********************************/
/* AT+QIDNSGIP=<contextID>,<hostname> */
int at_QIDNSGIP_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		int contextID = -1;
		dns_query_param_t dns_arg = {0};

		dns_arg.type = TCPIP_CMD_BC25;
		if (at_parse_param("%d(1-3),%51p()", at_buf, &contextID, &dns_arg.host) != XY_OK) 
		{
            *prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
            return AT_END;
        }

        int ret = at_query_dns_async(&dns_arg);
		if (ret != AT_END)
		{	
			*prsp_cmd = AT_TCPIP_ERR((ret == ATERR_NOT_NET_CONNECT) ? TCPIP_Err_PdpBroken : TCPIP_Err_Block);
			return AT_END;
		}
	}
#if (AT_CUT != 1)
	else if (g_req_type == AT_CMD_TEST)
	{
        *prsp_cmd = xy_malloc(50);
        snprintf(*prsp_cmd, 50, "\r\n+QIDNSGIP: (1-3),<hostname>\r\n\r\nOK\r\n");
	}
#endif
	else
	{
        *prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
        return AT_END;
    }

    set_at_tcpip_err(TCPIP_OP_OK);
	return AT_END;
}

/* AT+QIDNSCFG=<PrimaryDns>[,<SecondaryDns>] */
int at_QIDNSCFG_req2(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		dns_cfg_param_t param = {0};
		// DNS地址字符串的最大长度为XY_IPADDR_STRLEN_MAX
		if (at_parse_param("%51p(),%51p", at_buf, &param.pridns, &param.secdns) != AT_OK)
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
			return AT_END;
		}
		int ret = at_dns_config(&param);
		if (ret != AT_END)
		{
			*prsp_cmd = AT_TCPIP_ERR((ret == ATERR_NOT_ALLOWED) ? TCPIP_Err_Block : TCPIP_Err_Parameter);
			return AT_END;		
		}
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(256);
		char *pri_dns = xy_malloc(XY_IPADDR_STRLEN_MAX);
		char *sec_dns = xy_malloc(XY_IPADDR_STRLEN_MAX);
		char *pri_dns2 = xy_malloc(XY_IPADDR_STRLEN_MAX);
		char *sec_dns2 = xy_malloc(XY_IPADDR_STRLEN_MAX);		
		memset(pri_dns, 0, XY_IPADDR_STRLEN_MAX);
		memset(sec_dns, 0, XY_IPADDR_STRLEN_MAX);
		memset(pri_dns2, 0, XY_IPADDR_STRLEN_MAX);
		memset(sec_dns2, 0, XY_IPADDR_STRLEN_MAX);

		xy_dns_get(0, pri_dns);
		xy_dns_get(1, sec_dns);
		xy_dns_get(2, pri_dns2);
		xy_dns_get(3, sec_dns2);
		snprintf(*prsp_cmd, 256, "\r\n+QIDNSCFG: \"%s\",\"%s\",\"%s\",\"%s\"\r\n\r\nOK\r\n", pri_dns, sec_dns, pri_dns2, sec_dns2);
		xy_free(pri_dns);
		xy_free(sec_dns);
		xy_free(pri_dns2);
		xy_free(sec_dns2);
	}
#if (AT_CUT != 1)
	else if (g_req_type == AT_CMD_TEST)
	{
        *prsp_cmd = xy_malloc(64);
        snprintf(*prsp_cmd, 64, "\r\n+QIDNSCFG: <PrimaryDns>[,<SecondaryDns>]\r\n\r\nOK\r\n");
	}
#endif
	else
	{
		*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
		return AT_END;
	}

    set_at_tcpip_err(TCPIP_OP_OK);
	return AT_END;	
}

/* AT+QNTP==<contextID>,<server>[,<port>[,<autosettime>]] */
int at_QNTP_req(char *at_buf, char **prsp_cmd)
{
    int ret = XY_OK;
    if (g_req_type == AT_CMD_REQ)
	{
		int contextID = -1;
		ntp_query_param_t param = {0};

		param.update = 1;
		param.timeout = 40;
		param.type = TCPIP_CMD_BC25;
		if (at_parse_param("%d(1-3),%51p(),%2d[1-65535],%1d[0-1]", at_buf, &contextID, &param.host, &param.port, &param.update) != XY_OK)
        {
            *prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
            return AT_END;
        }

        ret = at_query_ntp_async(&param);
		if (ret != AT_END)
		{	
			*prsp_cmd = AT_TCPIP_ERR((ret == ATERR_NOT_NET_CONNECT) ? TCPIP_Err_PdpBroken : TCPIP_Err_Block);
			return AT_END;
		}
	}
#if (AT_CUT != 1)
	else if (g_req_type == AT_CMD_TEST)
	{
        *prsp_cmd = xy_malloc(60);
        snprintf(*prsp_cmd, 60, "\r\n+QNTP: (1-3),\"<server>\"[,<port>[,(0-1)]]\r\n\r\nOK\r\n");
	}
#endif
	else
	{
        *prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
        return AT_END;
    }

    set_at_tcpip_err(ret);
	return AT_END;
}


// AT+QIGETERROR 查询上一个AT命令错误代码 
int at_QIGETERROR_req(char *at_buf, char **prsp_cmd)
{
    (void) at_buf;
 	if (g_req_type == AT_CMD_ACTIVE)
	{
        *prsp_cmd = xy_malloc(64);
        int err = get_at_tcpip_err();
        snprintf(*prsp_cmd, 64, "\r\n+QIGETERROR:%d,%s\r\n\r\nOK\r\n", err, get_TCPIP_err_info(err));
    }
#if (AT_CUT != 1)
    else if (g_req_type != AT_CMD_TEST)
    {
        *prsp_cmd = AT_TCPIP_ERR(XY_Err_NotAllowed);
    }
#endif
    return AT_END;
}

