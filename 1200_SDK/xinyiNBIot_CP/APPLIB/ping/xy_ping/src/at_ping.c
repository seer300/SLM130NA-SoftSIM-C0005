#if XY_PING

#include "xy_at_api.h"
#include "at_ping.h"
#include "ping_api.h"
#include "lwip/sockets.h"
#include "ps_netif_api.h"
#include "xy_utils.h"
#include "net_api_priv.h"
#include "at_tcpip_api.h"

/**
 * @brief AT+NPING=<host>,<data_len>,<ping_num>,<time_out>,<interval_time>[,<rai>]
 */
int at_NPING_req(char *at_buf,char **prsp_cmd)
{
    int ret = AT_END;
	ping_para_t *ping_para = (ping_para_t *)xy_malloc(sizeof(ping_para_t));
    memset(ping_para, 0, sizeof(ping_para_t));
    // 初始值为无效值，实际取值见RAI_TYPE枚举
	ping_para->rai_val = 0xFF;

    if (  at_parse_param("%100s(),%2d(1-),%2d(1-65535),%d(1-),%d(0-),%1d[0-2]", at_buf, ping_para->host, &ping_para->data_len, &ping_para->ping_num, &ping_para->time_out, &ping_para->interval_time, &ping_para->rai_val) != AT_OK \
	   || strlen(ping_para->host) == 0 || ping_para->data_len > XY_PING_MAX_DATA_LENGTH)
    {
		ret = ATERR_PARAM_INVALID;
        goto OUT;
	}

	if (!xy_tcpip_is_ok())
    {
        xy_printf(0,XYAPP, WARN_LOG, "ps netif is not ok!");
		ret = ATERR_NOT_NET_CONNECT;
        goto OUT;
    }

	if (ps_is_oos())
	{
    	xy_printf(0,XYAPP, WARN_LOG, "ps netif is OOS!");
		ret = ATERR_NOT_NET_CONNECT;
        goto OUT;
    }

    /**
     * ping第一包成功后，网侧一般等15~20s 下发链路释放；而在ping后面包时容易和前面的链路释放碰撞，导致上行ping发出后得不到回包。
     * 如果采用单包发送的方式ping包或者两次ping包间隔超过20s，可以设置rai=2, 告知网络ping包收到回包后就没有后续数据了，网络会快速释放链路。
     */
    if (ping_para->rai_val == 0xFF)
    {
        if (ping_para->ping_num == 1 || ping_para->interval_time > 20)
            ping_para->rai_val = RAI_REL_DOWN;
        else
            ping_para->rai_val = RAI_NULL;
    }

    if (start_ping(ping_para) == XY_ERR)
		ret = ATERR_NOT_ALLOWED;

OUT:    
    xy_free(ping_para);
    return ret;
}

int at_NPINGSTOP_req(char *at_buf,char **prsp_cmd)
{
    UNUSED_ARG(at_buf);
    UNUSED_ARG(prsp_cmd);
    stop_ping();
	return AT_END;
}

static int domain_is_valid(uint8_t wan_iptype, const char *ipaddr)
{
	int ipaddr_len = strlen(ipaddr);
	if (ipaddr_len == 0)
		return XY_ERR;

	ip_addr_t result;
	int ret = ipaddr_aton(ipaddr, &result);
	xy_printf(0, PLATFORM_AP, INFO_LOG, "ipaddr %s %d %d %d!", ipaddr, ret, ipaddr_len, result.type);
	
	if (!ret)
	{
		int count = 0;
		char *src = ipaddr;

		result.type = IPV46_TYPE;
		for ( ; *src != '\0'; ++src)
		{
			if (*src >= '0' && *src <= '9')
				++count;
			else
				break;
		}
		if (count == ipaddr_len)
		{
			xy_printf(0, PLATFORM_AP, INFO_LOG, "[%s]host only contain numbers are not supported:%s", __FUNCTION__, ipaddr);
			return XY_ERR;
		}
	}
	else if ( (wan_iptype == IPV4_TYPE && result.type == IPV6_TYPE) || (wan_iptype == IPV6_TYPE && result.type == IPV4_TYPE) )
	{
		xy_printf(0, PLATFORM_AP, INFO_LOG, "[%s]iptype not active:%d,%d", __FUNCTION__, wan_iptype, result.type);
		return XY_ERR;
	}
	
	// IPV4地址格式做严格检查
	if (result.type == IPV4_TYPE && xy_IpAddr_Check(ipaddr, result.type) == 0)
	{
		xy_printf(0, PLATFORM_AP, INFO_LOG, "[%s]ipv4 addr format err");
		return XY_ERR;
	}

	return XY_OK;
}
#if VER_BC25
/**
 * @brief AT+QPING=<contextID>,"<host>"[,<time_out>[,<ping_num>[,<ping_size>]]]
 */
int at_QPING_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
        int contextID = 1;
		ping_para_t *ping_para = (ping_para_t *)xy_malloc(sizeof(ping_para_t));
		memset(ping_para, 0, sizeof(ping_para_t));
		ping_para->ping_num = 4;
		ping_para->time_out = 4;
		ping_para->data_len = 32;

		if (at_parse_param("%1d(1-3),%51s(),%d[1-255],%2d[1-10],%2d[32-200]", at_buf, &contextID, ping_para->host, &ping_para->time_out, &ping_para->ping_num, &ping_para->data_len) != XY_OK)
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
			goto OUT;
		}

		if (domain_is_valid(xy_get_netif_iptype(), ping_para->host) != XY_OK)
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
			goto OUT;
		}

        if (!xy_tcpip_is_ok() || ps_is_oos())
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_PdpBroken);
            xy_printf(0, PLATFORM_AP, INFO_LOG, "[%s]net status err:%d", __FUNCTION__, xy_tcpip_is_ok(), ps_is_oos());
			goto OUT;
		} 

        if (start_ping(ping_para) == XY_OK)
            set_at_tcpip_err(TCPIP_OP_OK);
        else
            *prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Block);    

OUT:
		xy_free(ping_para);		
	}
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(100);
		snprintf(*prsp_cmd, 100, "\r\n+QPING: (1-3),\"<host>\"[,(1-255)[,(1-10)[,(32-200)]]]\r\n\r\nOK\r\n");
	}
    else
		*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
	
	return AT_END;
}
#elif VER_260Y
/**
 * @brief AT+QPING=<contextID>,<host>[,<time_out>[,<ping_num>[,<ping_size>[,<rai_mode>]]]]
 */
int at_QPING_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
        int contextID = 1;
		ping_para_t *ping_para = (ping_para_t *)xy_malloc(sizeof(ping_para_t));
		memset(ping_para, 0, sizeof(ping_para_t));
		ping_para->ping_num = 4;
		ping_para->time_out = 4;
		ping_para->data_len = 32;

		if (at_parse_param("%1d(0-0),%151s(),%d[1-255],%2d[1-10],%2d[32-1500],%1d[0-1]", at_buf, &contextID, ping_para->host, &ping_para->time_out, &ping_para->ping_num, &ping_para->data_len, &ping_para->rai_val) != XY_OK)
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
			goto OUT;
		}

        if (!xy_tcpip_is_ok() || ps_is_oos())
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_PdpBroken);
            xy_printf(0, PLATFORM, INFO_LOG, "[%s]net status err:%d", __FUNCTION__, xy_tcpip_is_ok(), ps_is_oos());
			goto OUT;
		} 

        if (start_ping(ping_para) == XY_OK)
            set_at_tcpip_err(TCPIP_OP_OK);
        else
            *prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Block);    

OUT:
		xy_free(ping_para);		
	}
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(100);
		snprintf(*prsp_cmd, 100, "\r\n+QPING: (0-10),<host>,(1-255),(1-10),(32-1500),(0,1)\r\n\r\nOK\r\n");
	}
    else
		*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
	
	return AT_END;
}

void user_ping_reply_info_hook_260Y(ping_info_t* ping_info, char *rsp_cmd, ping_reply_info_e type)
{
	if (ping_info == NULL || rsp_cmd == NULL)
	{
		send_rsp_at_to_ext("\r\n+QPING: 4\r\n");
		return;
	}
		
	memset(rsp_cmd, 0, 128);
	if (type == SINGLE_PAC_TIMEOUT)
		sprintf(rsp_cmd, "\r\n+QPING: 1");
	else if (type == SINGLE_PAC_SUCC)
	    sprintf(rsp_cmd, "\r\n+QPING: 0,%s,%d,%d,%d", ping_info->host_ip, ping_info->len, ping_info->rtt, ping_info->ttl);
	else if (type == PING_PAC_NO_MEM)
		sprintf(rsp_cmd, "\r\n+QPING: %d\r\n", TCPIP_Err_NoMemory);
	else
	{
	    snprintf(rsp_cmd, 128, "\r\n\r\n+QPING: 2,%d,%d,%d,%d,%d,%d\r\n", ping_info->ping_send_num, ping_info->ping_reply_num, 
            ping_info->ping_send_num - ping_info->ping_reply_num, ping_info->shortest_rtt, ping_info->longest_rtt, ping_info->time_average);		
	}
	send_rsp_at_to_ext(rsp_cmd);
}
#endif

void user_ping_reply_info_hook2(ping_info_t* ping_info, char *rsp_cmd, ping_reply_info_e type)
{
	if (ping_info == NULL || rsp_cmd == NULL)
	{
		send_rsp_at_to_ext("\r\n+QPING: 4\r\n");
		return;
	}
		
	memset(rsp_cmd, 0, 128);
	if (type == SINGLE_PAC_TIMEOUT)
		sprintf(rsp_cmd, "\r\n+QPING: 1");
	else if (type == SINGLE_PAC_SUCC)
	    sprintf(rsp_cmd, "\r\n+QPING: 0,%s,%d,%d,%d", ping_info->host_ip, ping_info->len, ping_info->rtt, ping_info->ttl);
	else if (type == PING_PAC_NO_MEM)
		sprintf(rsp_cmd, "\r\n+QPING: %d\r\n", TCPIP_Err_NoMemory);
	else
	{
	    snprintf(rsp_cmd, 128, "\r\n\r\n+QPING: 2,%d,%d,%d,%d,%d,%d\r\n", ping_info->ping_send_num, ping_info->ping_reply_num, 
            ping_info->ping_send_num - ping_info->ping_reply_num, ping_info->shortest_rtt, ping_info->longest_rtt, ping_info->time_average);		
	}
	send_rsp_at_to_ext(rsp_cmd);
}

void user_ping_reply_info_hook1(ping_info_t* ping_info, char *rsp_cmd, ping_reply_info_e type)
{
	if (ping_info == NULL || rsp_cmd == NULL)
		return;
	memset(rsp_cmd,0,128);
	if (type == SINGLE_PAC_TIMEOUT)
		sprintf(rsp_cmd, "\r\nPing timeout\r\n");
	else if (type == SINGLE_PAC_SUCC)
	    sprintf(rsp_cmd, "\r\nreply from %s, %d bytes %d ms\r\n", ping_info->host_ip, ping_info->len, ping_info->rtt);
	else if (type == PING_PAC_NO_MEM)
		sprintf(rsp_cmd, "\r\nPing no mem\r\n");
	else
	{
#if VER_BC95
	    snprintf(rsp_cmd, 128, "\r\n+NPING:statistics: ping num:%d, reply:%d, longest_rtt:%dms, shortest_rtt:%dms, average_time:%dms\r\n", 
			ping_info->ping_send_num, ping_info->ping_reply_num, ping_info->longest_rtt, ping_info->shortest_rtt, ping_info->time_average);
#else
	    snprintf(rsp_cmd, 128, "\r\nstatistics: ping num:%d, reply:%d, longest_rtt:%dms, shortest_rtt:%dms, average_time:%dms\r\n", 
            ping_info->ping_send_num, ping_info->ping_reply_num, ping_info->longest_rtt, ping_info->shortest_rtt, ping_info->time_average);
#endif
				
	}
	send_rsp_at_to_ext(rsp_cmd);
}


void user_ping_reply_info_hook(ping_info_t* ping_info, char *rsp_cmd, ping_reply_info_e type)
{
#if VER_BC25
	user_ping_reply_info_hook2(ping_info, rsp_cmd, type);
#elif VER_260Y
	user_ping_reply_info_hook_260Y(ping_info, rsp_cmd, type);
#else
	user_ping_reply_info_hook1(ping_info, rsp_cmd, type);
#endif
}

#endif //XY_PING
