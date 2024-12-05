#include "at_perf.h"
#include "xyperf.h"
#include "xy_at_api.h"
#include "xy_socket_api.h"
#if XY_PING
#include "at_ping.h"
#endif
#include "ps_netif_api.h"

#if XY_PERF

/**
 * AT+XYPERF=<host>,<port>,<net_type>,<size>,<bandwidth>,<time>,<param>
 * time:单位秒,param:固定为1
 */
int at_XYPERF_req(char *at_buf, char **prsp_cmd)
{
	xyperf_arguments_t xyperf_param;
    char protocol_str[4] = {0};
    char bandwidth_str[16] = {0};
    int len = 0;
    int base = 0;
    int times = 1;
    int i;
	
	(void) prsp_cmd;
	
    memset(&xyperf_param, 0, sizeof(xyperf_arguments_t));

    if (at_parse_param("%16s,%d,%4s,%d,%s,%d,%d,%d", \
        at_buf, xyperf_param.remote_ip, &xyperf_param.remote_port, protocol_str, &xyperf_param.packet_size, bandwidth_str, &xyperf_param.duration, &xyperf_param.print_interval, &xyperf_param.rai_val) != AT_OK)
	{
		return ATERR_PARAM_INVALID;
	}
    if (g_xyperf_TskHandle != NULL)
    {
        //灌包处理线程已经运行
		return ATERR_NOT_ALLOWED;
	}
	
    xyperf_param.ip_type = packet_type_check(xyperf_param.remote_ip);
    if (xyperf_param.ip_type != AF_INET && xyperf_param.ip_type != AF_INET6)
    {
		return ATERR_PARAM_INVALID;
    }

	if (xyperf_param.ip_type == AF_INET)
	{
		// ipv4
		if (!xy_tcpip_v4_is_ok())
		{
			xy_printf(0,XYAPP, WARN_LOG, "xyperf ps netif ipv4 is not ok!");
			return ATERR_NOT_NET_CONNECT;
		}
	}
	else
	{
		// ipv6
		if (!xy_tcpip_v6_is_ok())
		{
			xy_printf(0,XYAPP, WARN_LOG, "xyperf ps netif ipv6 is not ok!");
			return ATERR_NOT_NET_CONNECT;
		}
	}

    if (strcmp(protocol_str, "udp") == 0)
    {
        xyperf_param.protocol_type = 0;
    }
    else if (strcmp(protocol_str, "tcp") == 0)
    {
        xyperf_param.protocol_type = 1;
    }
    else
    {
        return ATERR_PARAM_INVALID;
    }

	len = strlen(bandwidth_str);
	//<bandwidth>设置时带单位如: k,K,m,M,g,G
    if (bandwidth_str[len - 1] < '0' || bandwidth_str[len - 1] > '9')
    {
        if (bandwidth_str[len - 1] == 'k' || bandwidth_str[len - 1] == 'K')
        {
            times = 1;
        }
        else if (bandwidth_str[len - 1] == 'm' || bandwidth_str[len - 1] == 'M')
        {
            times = 1024;
        }
        else if (bandwidth_str[len - 1] == 'g' || bandwidth_str[len - 1] == 'G')
        {
            times = 1024 * 1024;
        }
        else
        {
            return ATERR_PARAM_INVALID;
        }
        for (i = 0; i < len - 1; i++)
        {
            if (bandwidth_str[i] < '0' || bandwidth_str[i] > '9')
            {
                return ATERR_PARAM_INVALID;
            }
            else
            {
                base = base * 10 + bandwidth_str[i] - '0';
            }
        }
    }
	//<bandwidth>设置时为纯数字不带单位
    else
    {
        for (i = 0; i < len; i++)
        {
            if (bandwidth_str[i] < '0' || bandwidth_str[i] > '9')
            {
                return ATERR_PARAM_INVALID;
            }
            else
            {
                base += base * 10 + bandwidth_str[i] - '0';
            }
        }
        if (base < 1024)
        {
            return ATERR_PARAM_INVALID;
        }
        times = 1;
        base = base / 1024;
    }
	xyperf_param.rate = base * times;

    start_xyperf(&xyperf_param);

	return AT_END;
}

#endif //XY_PERF
