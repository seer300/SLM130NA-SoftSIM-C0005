/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_com.h"
#include "at_tcpip_api.h"
#include "net_api_priv.h"
#include "softap_nv.h"
#include "ps_netif_api.h"
#include "xy_at_api.h"
#include "xy_net_api.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "xy_socket_api.h"

/*******************************************************************************
 *						  Local variable definitions						   *
 ******************************************************************************/
osThreadId_t ntp_thd_handle = NULL;
osThreadId_t dns_thd_handle = NULL;

/*******************************************************************************
 *						function implementations						       *
 ******************************************************************************/
ntp_zone_t get_zone_info(void)
{
	ntp_zone_t info = {0};
	char positive_zone;

	// 一个时区15分钟，若无时区信息，则默认时区为北京时区+32
	info.zone_sec = (g_softap_var_nv->g_zone != 0) ? (int)g_softap_var_nv->g_zone * 15 * 60 : 8 * 60 * 60;

	if (g_softap_var_nv->g_zone >= 0)
	{
		positive_zone = g_softap_var_nv->g_zone;
		if (g_softap_var_nv->g_zone >= 10)
			sprintf(info.zone, "+%2d", positive_zone);
		else
			sprintf(info.zone, "+%1d", positive_zone);
	}
	else
	{
		positive_zone = 0 - g_softap_var_nv->g_zone;
		if (g_softap_var_nv->g_zone <= -10)
			sprintf(info.zone, "-%2d", positive_zone);
		else
			sprintf(info.zone, "-%1d", positive_zone);
	}
	return info;
}

int query_ntp(ntp_query_param_t* arg)
{
    if (arg == NULL)
    {
        return XY_Err_Parameter;
    }

	if (!xy_tcpip_is_ok())
	{
		return XY_Err_NoConnected;
	}

    // 目前针对NTP相关AT指令，host一般为必选参数，不为NULL，其他业务存在host为NULL的情况时，使用默认的NTP服务器
	if (arg->host == NULL || strlen(arg->host) == 0)
	{
		arg->host = XY_DEFAULT_NTP_SERVER;
	}
	
	/* 平台内部使用默认值20秒；因NB速率低，如果设置超时时间，建议该值不得小于20秒 */
	if (arg->timeout == 0)
		arg->timeout = 20;

	struct sntp_time ptimemap = {0};
	int ret = gettimebysntp(arg->host, arg->port, arg->timeout, &ptimemap);
	if (ret == -1)
	{
		return covert_to_xy_net_errcode(errno);
	}
    else
    {
        xy_gmtime_r(ptimemap.sec * 1000ULL, &arg->rtctime);
    }

	return XY_OK;
}

int query_dns(dns_query_param_t *arg)
{
    if (arg == NULL || arg->host == NULL)
    {
        return XY_Err_Parameter;
    }

    if (!xy_tcpip_is_ok())
    {
        return XY_Err_NoConnected;
    }

	// 中移等客户有更改timeout的需求，此处设置定时器以后，为保证其他业务的正常进行，后续会恢复成默认值DNS_TMR_INTERVAL
    if (arg->timeout != 0)
        reset_dns_tmr_interval(arg->timeout * 1000);

    struct addrinfo hint = {0};
    struct addrinfo *result = NULL;
    struct addrinfo *tmp = NULL;
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_DGRAM;
    int ret = getaddrinfo(arg->host, NULL, &hint, &result);

    if (ret != ERR_OK)
    {
        ret = covert_to_xy_net_errcode(ret);
    }
    else
    {
        arg->af_family = result->ai_family;

        int ipaddrs_len = 0;
        for (tmp = result; tmp != NULL; tmp = tmp->ai_next)
        {
            if (tmp->ai_family == AF_INET)
                ipaddrs_len += XY_IP4ADDR_STRLEN;
            else if (tmp->ai_family == AF_INET6)
                ipaddrs_len += XY_IP6ADDR_STRLEN;
        }

        arg->ipaddr_list = xy_malloc(ipaddrs_len + 1);
		memset(arg->ipaddr_list, 0, ipaddrs_len + 1);
		
        for (tmp = result; tmp != NULL; tmp = tmp->ai_next)
        {
            if (tmp->ai_family == AF_INET)
            {
                char ip_addr[XY_IP4ADDR_STRLEN] = {0};
                struct sockaddr_in *addr = (struct sockaddr_in *)tmp->ai_addr;
                sprintf(arg->ipaddr_list + strlen(arg->ipaddr_list), "%s", inet_ntop(AF_INET, &addr->sin_addr, ip_addr, XY_IP4ADDR_STRLEN));
				*(arg->ipaddr_list + strlen(arg->ipaddr_list)) = ',';
            }
            else if (tmp->ai_family == AF_INET6)
            {
                char ip6_addr[XY_IP6ADDR_STRLEN] = {0};
                struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)tmp->ai_addr;
                sprintf(arg->ipaddr_list + strlen(arg->ipaddr_list), "%s", inet_ntop(AF_INET6, &addr6->sin6_addr, ip6_addr, XY_IP6ADDR_STRLEN));
				*(arg->ipaddr_list + strlen(arg->ipaddr_list)) = ',';
            }
        }
        /* 重置最后一个逗号 */
        *(arg->ipaddr_list + strlen(arg->ipaddr_list) - 1) = '\0';

        if (arg->nocache == 1)
        {
            dns_clear_cache(arg->host); //不保留dns缓存
        }

        freeaddrinfo(result);

        ret = XY_OK;
    }

    // 为保证其他业务的正常进行，DNS域名解析完成后会恢复成默认值DNS_TMR_INTERVAL
    if (arg->timeout != 0)
        restore_default_dns_tmr_interval();

    return ret;
}

void query_ntp_task(void *arg)
{
	ntp_query_param_t *param = (ntp_query_param_t *)arg;
	char *prsp = xy_malloc(128);
	int ret;
	if ((ret = query_ntp(param)) != XY_OK)
	{
		if (ret == XY_Err_Timeout)
			ret = 2;
		else if (ret == XY_Err_DnsFail)
			ret = 1;
		if (param->type == TCPIP_CMD_BC25)
			sprintf(prsp, "\r\n+QNTP: 2\r\n");
		else
			sprintf(prsp, "\r\n+CMNTP:%d\r\n", ret);
	}
	else
	{
		ntp_zone_t info = get_zone_info();
		if (1 == param->update)
			reset_universal_timer(&param->rtctime, info.zone_sec);
		if (param->type == TCPIP_CMD_BC25)
		{
			sprintf(prsp, "\r\n+QNTP: 0,\"%02lu/%02lu/%02lu,%02lu:%02lu:%02lu%s\"\r\n",
					param->rtctime.wall_clock.tm_year - 2000, param->rtctime.wall_clock.tm_mon, param->rtctime.wall_clock.tm_mday, param->rtctime.wall_clock.tm_hour,
					param->rtctime.wall_clock.tm_min, param->rtctime.wall_clock.tm_sec, info.zone);
		}
		else
		{
			/* +CMNTP:<err_no>[,<time>] 例如+CMNTP: 0,"19/06/05,08:53:37+32"*/
			sprintf(prsp, "\r\n+CMNTP:0,\"%02lu/%02lu/%02lu,%02lu:%02lu:%02lu%s\"\r\n",
					param->rtctime.wall_clock.tm_year - 2000, param->rtctime.wall_clock.tm_mon, param->rtctime.wall_clock.tm_mday, param->rtctime.wall_clock.tm_hour,
					param->rtctime.wall_clock.tm_min, param->rtctime.wall_clock.tm_sec, info.zone);
		}
	}

	send_rsp_at_to_ext(prsp);
	xy_free(prsp);
	/* 异步线程需要释放内存 */
	if (param != NULL && param->host != NULL)
		xy_free(param->host);
	if (param != NULL)
		xy_free(param);

	ntp_thd_handle = NULL;
	osThreadExit();
}

void query_dns_task(void *param)
{
	dns_query_param_t *arg = (dns_query_param_t *)param;
	char* prsp = NULL;
	if (arg == NULL)
	{
		xy_printf(0, XYAPP, WARN_LOG, "async query dns fail arg null");
		goto NULL_END;
	}
	if (arg->type == TCPIP_CMD_QUECTEL)
	{
        /* 移远重传一次 */
        dns_set_max_retries(1);
    }
	if (query_dns(arg) != XY_OK)
	{
		if (arg->type == TCPIP_CMD_CMCC)
		{
			prsp = xy_malloc(32);
			strcpy(prsp, "\r\n+CMDNS:QUERY_DNS_FAILED\r\n");
		}
		else if (arg->type == TCPIP_CMD_QUECTEL)
		{
			prsp = xy_malloc(32);
			strcpy(prsp, "\r\n+QDNS:QUERY_DNS_FAILED\r\n");
		}
		else if (arg->type == TCPIP_CMD_BC25)
		{
			prsp = xy_malloc(32);
			snprintf(prsp, 32, "\r\n+QIURC: \"dnsgip\",%d\r\n", TCPIP_Err_DnsFail);
		}		
	}
	else
	{
		int ipaddrs_len = strlen(arg->ipaddr_list);
		prsp = xy_malloc(ipaddrs_len + 64);
		if (arg->type == TCPIP_CMD_CMCC)
			sprintf(prsp, "\r\n+CMDNS:%s\r\n", arg->ipaddr_list);
		else if (arg->type == TCPIP_CMD_QUECTEL)
			sprintf(prsp, "\r\n+QDNS:%s\r\n", arg->ipaddr_list);
		else if (arg->type == TCPIP_CMD_BC25)
		{
			sprintf(prsp, "\r\n+QIURC: \"dnsgip\",0,1,0\r\n+QIURC: \"dnsgip\",\"%s\"\r\n", arg->ipaddr_list);
		}
			
		xy_free(arg->ipaddr_list);
	}

    if (arg->type == TCPIP_CMD_QUECTEL)
    {
        dns_set_max_retries(DNS_MAX_RETRIES);
    }
	if (prsp != NULL)
	{
		send_rsp_at_to_ext(prsp);
		xy_free(prsp);
	}
	xy_printf(0,XYAPP, WARN_LOG, "async query dns tpye(%d) exit", arg->type);
	/* 异步线程需要释放内存 */
	if (arg != NULL && arg->host != NULL)
		xy_free(arg->host);
	if (arg != NULL)
		xy_free(arg);
NULL_END:
	/* 线程退出 */
	dns_thd_handle = NULL;
	osThreadExit();
}

int at_query_dns_async(dns_query_param_t* arg)
{
	if (arg->host == NULL || strlen(arg->host) == 0 || strlen(arg->host) > DNS_MAX_NAME_LENGTH)
	{
		return ATERR_PARAM_INVALID;
	}
    /* 有dns查询正在执行不允许操作 */
	if (dns_thd_handle != NULL)
	{
		return ATERR_NOT_ALLOWED;
	}

	if (!xy_tcpip_is_ok())
	{
		return ATERR_NOT_NET_CONNECT;
	}

	dns_query_param_t *param = (dns_query_param_t *)xy_malloc(sizeof(dns_query_param_t));
	memcpy(param, arg, sizeof(dns_query_param_t));

	/*  query_dns_task处理完成释放 */
	param->host = xy_malloc(strlen(arg->host) + 1);
	strcpy(param->host, arg->host);

	osThreadAttr_t thread_attr = {0};
	thread_attr.name 		= "at_query_dns";
	thread_attr.priority 	= osPriorityNormal1;
	thread_attr.stack_size 	= osStackShared;
	dns_thd_handle = osThreadNew((osThreadFunc_t)(query_dns_task), param, &thread_attr);

	return AT_END;
}

int at_query_ntp_async(ntp_query_param_t *arg)
{
	if (g_NITZ_mode != 1)
	{
		return ATERR_NOT_ALLOWED;
	}

	// host参数为NULL时，用默认的NTP查询服务器域名
	if (arg->host != NULL && strlen(arg->host) > DNS_MAX_NAME_LENGTH)
	{
		return ATERR_PARAM_INVALID;
	}

    /* 有ntp查询正在执行不允许操作 */
	if (ntp_thd_handle != NULL)
	{
		return ATERR_NOT_ALLOWED;
	}

	if (!xy_tcpip_is_ok())
	{
		return ATERR_NOT_NET_CONNECT;
	}

	ntp_query_param_t *param = (ntp_query_param_t *)xy_malloc(sizeof(ntp_query_param_t));
	memcpy(param, arg, sizeof(ntp_query_param_t));

	/*  query_dns_task处理完成释放 */
	param->host = xy_malloc(strlen(arg->host) + 1);
	strcpy(param->host, arg->host);

	osThreadAttr_t thread_attr = {0};
	thread_attr.name 		= "at_query_ntp";
	thread_attr.priority 	= osPriorityNormal1;
	thread_attr.stack_size 	= osStackShared;
	ntp_thd_handle = osThreadNew((osThreadFunc_t)(query_ntp_task), param, &thread_attr);

	return AT_END;
}

int at_dns_config(dns_cfg_param_t *arg)
{
	xy_assert(arg != NULL);

	if (dns_thd_handle != NULL)
	{
		/* 有dns查询正在执行不允许操作 */
		xy_printf(0,XYAPP, WARN_LOG, "at_dns_config but dns query is executing");
		return ATERR_NOT_ALLOWED;
	}    

	// 主DNS为必填参数，故此处不再判断主DNS是否为NULL，参数解析时已经保证不为NULL，若为可选参数，请注意！
	if (!xy_dns_set(arg->pridns, 0, arg->save))
	{
		xy_printf(0,XYAPP, WARN_LOG, "at_dns_config set pridns:%s err", arg->pridns);
		return ATERR_PARAM_INVALID;
	}
	// 辅DNS可为NULL，保证主DNS能正常使用即可
	if (arg->secdns != NULL)
	{
		if (!xy_dns_set(arg->secdns, 1, arg->save))
		{
			xy_printf(0,XYAPP, WARN_LOG, "at_dns_config set secdns:%s err", arg->secdns);
			return ATERR_PARAM_INVALID;
		}
	}

	return AT_END;
}


/*******************************************************************************
 *					            	BC25指定AT指令 					            *
 ******************************************************************************/
typedef struct
{
    int err;             //错误码
    char *description;   //at命令错误描述，一般为模组厂家自定义错误描述
} plat_at_err_tbl_t;

plat_at_err_tbl_t s_plat_at_err_tbl[] = {

    {TCPIP_OP_OK,              "operation success"},
    {TCPIP_Err_Unknown,        "unknown error"}, 
    {TCPIP_Err_InProgress,     "operation busy"},
    {TCPIP_Err_Timeout,        "operation timeout"},
    {TCPIP_Err_NotSupport,     "operation not supported"}, 
    {TCPIP_Err_NotAllowed,     "operation not allowed"}, 
    {TCPIP_Err_Block,          "operation blocked"},
    {TCPIP_Err_NoMemory,       "memory not enough"},
    {TCPIP_Err_Parameter,      "invalid parameters"},
    {TCPIP_Err_PortBusy,       "port busy"}, 
    {TCPIP_Err_SockCreate,     "socket creation failed"},
    {TCPIP_Err_SockBind,       "socket bind failed"},
    {TCPIP_Err_SockConnect,    "socket connect failed"},
    {TCPIP_Err_SockListen,     "socket listen failed"}, 
    {TCPIP_Err_SockAccept,     "socket accept failed"},
    {TCPIP_Err_SockWrite,      "socket write failed"},
    {TCPIP_Err_SockRead,       "socket read failed"},
    {TCPIP_Err_SockInuse,      "socket identity has been used"},
    {TCPIP_Err_SockClosed,     "socket has been closed"},
    {TCPIP_Err_DnsBusy,        "DNS busy"},
    {TCPIP_Err_DnsFail,        "DNS parse failed"},
    {TCPIP_Err_PdpOpen,        "PDP context opening failed"},
    {TCPIP_Err_PdpClose,       "PDP context closure failed"},
    {TCPIP_Err_PdpBroken,      "PDP context broken down"},
    {TCPIP_Err_ApnConfig,      "APN not configured"},
};

static int g_at_tcpip_err = TCPIP_OP_OK;

int set_at_tcpip_err(int err)
{
    g_at_tcpip_err = err;
	return 0;
}

int get_at_tcpip_err(void)
{
    return g_at_tcpip_err;
}

char* get_TCPIP_err_info(int errno)
{
    uint32_t i = 0;
    for (i = 0; i < (sizeof(s_plat_at_err_tbl) / sizeof(s_plat_at_err_tbl[0])); i++)
    {
        if (errno == s_plat_at_err_tbl[i].err)
        {
            return s_plat_at_err_tbl[i].description;
        }
    }
    /* 返回未知错误 */
    return get_TCPIP_err_info(TCPIP_Err_Unknown);
}

char *TCPIP_Err_info_build(uint32_t err_no, char *file, uint32_t line)
{
	char *outStr = xy_malloc(32);

    if (err_no == XY_OK)
        snprintf(outStr, 32, "\r\nOK\r\n");
    else
	{
		// 只显示ERROR
		snprintf(outStr, 32, "\r\nERROR\r\n");
		if (HWREGB(BAK_MEM_XY_DUMP) == 1)
			xy_printf(0, PLATFORM_AP, WARN_LOG, "socketErr_info_build line:%d,file:%s,err:%d\n", line, file, err_no);
 	}

	//设置错误码，用于QIGETERROR查询使用
	set_at_tcpip_err(err_no);

	return outStr;
}

