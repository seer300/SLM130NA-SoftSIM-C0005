#if XY_PING
#include "ping_api.h"
#include "oss_nv.h"
#include "ps_netif_api.h"
#include "xy_utils.h"
#include "xy_system.h"
#include "xy_at_api.h"
#include "xy_net_api.h"
#include "xy_socket_api.h"

#include "lwip/netdb.h"
#include "lwip/nd6.h"
#include "lwip/priv/nd6_priv.h"
#include "lwip/prot/nd6.h"
#include "lwip/prot/icmp6.h"
#include "lwip/pbuf.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/ip6.h"
#include "lwip/ip6_addr.h"
#include "lwip/inet_chksum.h"
#include "lwip/netif.h"
#include "lwip/icmp.h"
#include "lwip/icmp6.h"
#include "lwip/mld6.h"
#include "lwip/ip.h"
#include "lwip/stats.h"
#include "lwip/dns.h"
#include "lwip/inet.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"

// 协议栈监测到netif状态发生相应变化时，会执行相应回调，通知到业务层
#define EVENT_PSNETIF_PING (EVENT_PSNETIF_IPV4_VALID | EVENT_PSNETIF_INVALID)
#define EVENT_PSNETIF_PING6 (EVENT_PSNETIF_IPV6_VALID | EVENT_PSNETIF_INVALID)

// 可保证终止尚未进行的ping包过程
uint8_t g_ping_stop;
osThreadId_t at_ping_thread_id = NULL;

// 记录ICMP ICMPV6包的标识id，和序列号配合使用，用于保证接收到对应的下行ping6 reply包
static uint16_t echo_id = 0;

void ping4_netif_event_callback(PsStateChangeEvent event)
{
    if (event == EVENT_PSNETIF_IPV4_VALID)
    {
		g_ping_stop = 0;
        xy_printf(0,XYAPP, WARN_LOG, "ping4_netif_event_callback, netif up");
    }
    else
    {
		g_ping_stop = 1;
        osSemaphoreRelease(g_out_OOS_sem); //put semaphore when netif down
        xy_printf(0,XYAPP, WARN_LOG, "ping4_netif_event_callback, netif down");
    }
}

int _ping_ipv4(ping_para_t *ping_para)
{
    int ret = 0;
    int sock = -1;
	struct timeval tv;
    uint8_t is_timedout = 0;
    fd_set readfds, exceptfds; 
    struct icmp_echo_hdr *icmpechohdr = NULL;
    // 记录ICMP包的标识id，和序列号seqno配合使用，用于保证接收到对应的下行ping reply包
    uint16_t seqno = osKernelGetTickCount();
    int peer_addrlen = sizeof(struct sockaddr_in);
    struct sockaddr_in addr = {0}, peer_addr = {0};
    // 记录从发送ping请求包到接收到ping回复包过程的时间信息
    uint32_t start_time, total_rtt = 0;
	uint16_t current_echo_id;
    ping_info_t* ping_info = (ping_info_t *)xy_malloc(sizeof(ping_info_t));
	memset(ping_info, 0, sizeof(ping_info_t));
	ping_info->host_ip = (char *)xy_malloc(XY_IP4ADDR_STRLEN);
	ping_info->len = ping_para->data_len;
	
    g_ping_stop = 0; 

    // 注册netif状态事件
    xy_reg_psnetif_callback(EVENT_PSNETIF_PING, ping4_netif_event_callback);

	if ( (sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0 )
		xy_assert(0);

    addr.sin_family      = AF_INET;
    addr.sin_len         = sizeof(struct sockaddr_in);
    addr.sin_port        = lwip_htons((u16_t)0);
	memcpy(&addr.sin_addr, &ping_para->ip_addr, sizeof(struct in_addr));
    inet_ntop(AF_INET, &ping_para->ip_addr, ping_info->host_ip, XY_IP4ADDR_STRLEN);

    char *rsp_cmd   = (char *)xy_malloc(128);
    char *buffer = (char*)xy_malloc2(sizeof(struct icmp_echo_hdr) + ping_para->data_len);
    char *recv_data = xy_malloc2(IP_HLEN + ping_para->data_len + sizeof(struct icmp_echo_hdr));
	if (buffer == NULL || recv_data == NULL)
	{
    	user_ping_reply_info_hook(ping_info, rsp_cmd, PING_PAC_NO_MEM);
		goto OUT;
	}

    // icmp header填充
	echo_id++;
	current_echo_id = echo_id;
    icmpechohdr = (struct icmp_echo_hdr*)buffer;
    icmpechohdr->type = ICMP_ECHO;
    icmpechohdr->code = 0;
    icmpechohdr->id = lwip_htons(current_echo_id);

    // 填充数据
    char *icmp_data = buffer + sizeof(struct icmp_echo_hdr);
    for (int i = 0; i < ping_para->data_len; i++)
        icmp_data[i] = i + 'a';

	xy_printf(0,XYAPP, WARN_LOG, "start send ping\ndst addr:%s,datalen:%d,timeout:%d,interval_time:%d,rai:%d", ping_info->host_ip, ping_para->data_len, ping_para->time_out, ping_para->interval_time, ping_para->rai_val);
	while (!g_ping_stop && ping_para->ping_num--)
    {
        is_timedout = 0;
        
        if(ps_is_oos())
        {
			osSemaphoreAcquire(g_out_OOS_sem, osWaitForever);
			// 若协议栈监测到当前网络处于OOS状态，可能会触发CFUN0操作，此时需要停止ping包
			if(g_ping_stop == 1)
			{
				xy_printf(0,XYAPP, WARN_LOG, "report OOS and exit ping\n");
				break;
			}
        }
		++seqno;
        icmpechohdr->seqno = lwip_htons(seqno);
        icmpechohdr->chksum = 0;
        icmpechohdr->chksum = inet_chksum(icmpechohdr, sizeof(struct icmp_echo_hdr) + ping_para->data_len);
		xy_printf(0,XYAPP, WARN_LOG, "id:%d,seqno:%d", icmpechohdr->id, seqno);

		start_time = osKernelGetTickCount();
		ret = sendto2(sock, icmpechohdr, ping_para->data_len + sizeof(struct icmp_echo_hdr), 0, (const struct sockaddr *)&addr, sizeof(addr), 0, ping_para->rai_val);
		if(ret <= 0)
		{
			xy_printf(0,XYAPP, WARN_LOG, "\r\nsendto failed and again!!!\r\n");
			osDelay(500);
			continue;
		}		
        
        ping_info->ping_send_num++;
		tv.tv_sec = ping_para->time_out;
		tv.tv_usec = 0;
		
AGAIN:
		FD_ZERO(&readfds); 
		FD_ZERO(&exceptfds);
		FD_SET(sock, &readfds);
		FD_SET(sock, &exceptfds);
		ret = select(sock + 1, &readfds, NULL, &exceptfds, &tv);
		if(ret < 0) 
			xy_assert(0);
		else if(0 == ret)
		{
			is_timedout = 1;
			user_ping_reply_info_hook(ping_info, rsp_cmd, SINGLE_PAC_TIMEOUT);
			xy_printf(0,XYAPP, WARN_LOG, "%d select timeout\n", seqno);
		}
		else
        {
			if(FD_ISSET(sock, &exceptfds))
				xy_assert(0);

			if(FD_ISSET(sock, &readfds))
			{
				ret = recvfrom(sock, recv_data, IP_HLEN + ping_para->data_len + sizeof(struct icmp_echo_hdr), 0, (struct sockaddr *)&peer_addr, (socklen_t *)&peer_addrlen);
				xy_printf(0,XYAPP, WARN_LOG, "ret=%d, port=%d\n",ret,lwip_htons(peer_addr.sin_port));
				if( (ret > 0) && (IPPROTO_ICMP == lwip_htons(peer_addr.sin_port)) )
				{
					struct icmp_echo_hdr *reply_hdr = (struct icmp_echo_hdr *)(recv_data + IP_HLEN);

					xy_printf(0,XYAPP, WARN_LOG, "reply_hdr->type=%d id:%d seq:%d-%d\n", reply_hdr->type, reply_hdr->id, seqno, reply_hdr->seqno);
					if(ICMP_ER == reply_hdr->type && reply_hdr->id == lwip_htons(current_echo_id) && reply_hdr->seqno == lwip_htons(seqno))
					{
						ping_info->ping_reply_num++;
	                    ping_info->rtt = osKernelGetTickCount() - start_time;
	                    if ( (ping_info->longest_rtt == 0) || ((uint32_t)(ping_info->longest_rtt) < ping_info->rtt) )
	                        ping_info->longest_rtt = ping_info->rtt;
	                    if ( (ping_info->shortest_rtt == 0) || ((uint32_t)(ping_info->shortest_rtt) > ping_info->rtt) )
	                        ping_info->shortest_rtt = ping_info->rtt;                  
	                    total_rtt += ping_info->rtt;
	                    ping_info->time_average = total_rtt / ping_info->ping_reply_num;
						ping_info->ttl = ((struct ip_hdr *)recv_data)->_ttl;
						user_ping_reply_info_hook(ping_info, rsp_cmd, SINGLE_PAC_SUCC);
						xy_printf(0,XYAPP, WARN_LOG, "reply from %s[%s], %d bytes, %d ms, TTL=%d\n", ping_para->host, ping_info->host_ip, ping_para->data_len, ping_info->rtt, ping_info->ttl);
						if (ping_para->ping_num == 0)
							break;						
                        if (ping_info->rtt < (uint32_t)(ping_para->interval_time * 1000))
	            			osDelay((uint32_t)(ping_para->interval_time * 1000) - ping_info->rtt);

						continue;
					}
				}

				tv.tv_sec = ping_para->time_out - ((osKernelGetTickCount() - start_time + 500) / 1000); // 4舍五入
				xy_printf(0,XYAPP, WARN_LOG, "reply hdr err and goto select\n");
				goto AGAIN;
			}
        }
        
    }

	user_ping_reply_info_hook(ping_info, rsp_cmd, PING_END_REPLY);

OUT:
	if (sock != -1)
        close(sock);
	if (buffer != NULL)	
    	xy_free(buffer);
	if (recv_data != NULL)	
		xy_free(recv_data);
    xy_free(rsp_cmd);
	xy_free(ping_info->host_ip);
	xy_free(ping_info);

	xy_deReg_psnetif_callback(EVENT_PSNETIF_PING, ping4_netif_event_callback);

    return 0;
}

void ping6_netif_event_callback(PsStateChangeEvent event)
{
	if (event == EVENT_PSNETIF_IPV6_VALID)
    {
    	g_ping_stop = 0;
        xy_printf(0,XYAPP, WARN_LOG, "ping6_netif_event_callback, netif up");
    }
    else if(event == EVENT_PSNETIF_INVALID)
    {
    	g_ping_stop = 1;
        xy_printf(0,XYAPP, WARN_LOG, "ping6_netif_event_callback, netif down");
    }
}

int _ping_ipv6(ping_para_t *ping_para)
{
	int ret = 0;
	int sock = -1;
    struct timeval tv;
    uint8_t seq_no = 1;
    fd_set readfds, exceptfds;
	struct icmp6_echo_hdr *echo_hdr = NULL;
 	struct sockaddr_in6 sa6 = {0}, peer_sa6 = {0};
	int peer_addrlen = sizeof(struct sockaddr_in6);  
	uint32_t start_time, total_rtt = 0;
	// 记录ICMPV6包的标识id，和序列号配合使用，用于保证接收到对应的下行ping6 reply包
    uint16_t current_echo_id;

    ping_info_t* ping6_info = (ping_info_t *)xy_malloc(sizeof(ping_info_t));
	memset(ping6_info, 0, sizeof(ping_info_t));
	ping6_info->host_ip = (char *)xy_malloc(XY_IP6ADDR_STRLEN);
	ping6_info->len = ping_para->data_len;
	
	g_ping_stop = 0;

	xy_reg_psnetif_callback(EVENT_PSNETIF_PING6, ping6_netif_event_callback);
    
	if ( (sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) < 0 )
		xy_assert(0);
	
	sa6.sin6_family = AF_INET6;
	sa6.sin6_len    = sizeof(struct sockaddr_in6);
	sa6.sin6_port   = lwip_htons((u16_t)0);
	memcpy(&sa6.sin6_addr, &ping_para->ip_addr, sizeof(struct in6_addr));
	if (connect(sock, (struct sockaddr *)&sa6, sizeof(sa6)) == -1)
		xy_printf(0,XYAPP, WARN_LOG, "connect fail:%d", sock);
	inet_ntop(AF_INET6, &ping_para->ip_addr, ping6_info->host_ip, XY_IP6ADDR_STRLEN);

    char *rsp_cmd   = (char *)xy_malloc(128);
	char *buff = xy_malloc2(ping_para->data_len + sizeof(struct icmp6_echo_hdr));
    char *recv_data = xy_malloc2(IP6_HLEN + ping_para->data_len + sizeof(struct icmp6_echo_hdr));
	if (buff == NULL || recv_data == NULL)
	{
    	user_ping_reply_info_hook(ping6_info, rsp_cmd, PING_PAC_NO_MEM);
		goto OUT;
	}

	echo_id++;
	current_echo_id = echo_id;
	echo_hdr = (struct icmp6_echo_hdr *)buff;
	echo_hdr->type  = ICMP6_TYPE_EREQ;
	echo_hdr->code  = 0;
	echo_hdr->id    = lwip_htons(current_echo_id);

    char *data      = buff + sizeof(struct icmp6_echo_hdr);

	xy_printf(0,XYAPP, WARN_LOG, "start send ping6\ndst addr:%s,datalen:%d,timeout:%d,interval_time:%d", ping6_info->host_ip, ping_para->data_len, ping_para->time_out, ping_para->interval_time);
	while(ping_para->ping_num-- && !g_ping_stop)
	{
		++seq_no;
		echo_hdr->seqno  = lwip_htons((uint16_t)seq_no);
		echo_hdr->chksum = 0;
		memset(data, seq_no, ping_para->data_len);
		xy_printf(0,XYAPP, WARN_LOG, "id:%d,seq_no:%d", echo_id, seq_no);
		start_time = osKernelGetTickCount();
		ret = sendto2(sock, echo_hdr, ping_para->data_len + sizeof(struct icmp6_echo_hdr), 0, (const struct sockaddr *)&sa6, sizeof(sa6), 0, ping_para->rai_val);
		if(ret <= 0)
		{
			xy_printf(0,XYAPP, WARN_LOG, "\r\nsendto failed and again!!!\r\n");
			osDelay(500);
			continue;
		}
		
		ping6_info->ping_send_num++;
		tv.tv_sec = ping_para->time_out;
		tv.tv_usec = 0;		
AGAIN:
		FD_ZERO(&readfds); 
		FD_ZERO(&exceptfds);
		FD_SET(sock, &readfds);
		FD_SET(sock, &exceptfds);
		ret = select(sock + 1, &readfds, NULL, &exceptfds, &tv);
		if(ret < 0)
			xy_assert(0);
		else if(0 == ret)
		{
			user_ping_reply_info_hook(ping6_info, rsp_cmd, SINGLE_PAC_TIMEOUT);
			xy_printf(0,XYAPP, WARN_LOG, "%d select timeout\n",seq_no);
		}
		else
		{
			if(FD_ISSET(sock, &exceptfds))
				xy_assert(0);

			if(FD_ISSET(sock, &readfds))
			{
				ret = recvfrom(sock, recv_data, IP6_HLEN + ping_para->data_len + sizeof(struct icmp6_echo_hdr), 0, (struct sockaddr *)&peer_sa6, (socklen_t *)&peer_addrlen);
				xy_printf(0,XYAPP, WARN_LOG, "ret=%d, port=%d\n",ret,lwip_htons(peer_sa6.sin6_port));
				if( (ret > 0) && (IPPROTO_ICMPV6 == lwip_htons(peer_sa6.sin6_port)) )
				{
					struct icmp6_echo_hdr *reply_hdr = (struct icmp6_echo_hdr *)(recv_data + IP6_HLEN);

					xy_printf(0,XYAPP, WARN_LOG, "reply_hdr->type=%d id:%d-%d seq:%d-%d\n", reply_hdr->type, current_echo_id, reply_hdr->id, seq_no, reply_hdr->seqno);
					if(ICMP6_TYPE_EREP == reply_hdr->type && reply_hdr->id == lwip_htons(current_echo_id) && reply_hdr->seqno == lwip_htons(seq_no))
					{
						ping6_info->ping_reply_num++;
	                    ping6_info->rtt = osKernelGetTickCount() - start_time;
	                    if ( (ping6_info->longest_rtt == 0) || ((uint32_t)(ping6_info->longest_rtt) < ping6_info->rtt) )
	                        ping6_info->longest_rtt = ping6_info->rtt;
	                    if ( (ping6_info->shortest_rtt == 0) || ((uint32_t)(ping6_info->shortest_rtt) > ping6_info->rtt) )
	                        ping6_info->shortest_rtt = ping6_info->rtt;                  
	                    total_rtt += ping6_info->rtt;
	                    ping6_info->time_average = total_rtt / ping6_info->ping_reply_num;
						ping6_info->ttl = ((struct ip6_hdr *)recv_data)->_hoplim;
						user_ping_reply_info_hook(ping6_info, rsp_cmd, SINGLE_PAC_SUCC);
						
						xy_printf(0,XYAPP, WARN_LOG, "reply from %s[%s], %d bytes, %d ms, TTL=%d\n", ping_para->host, ping6_info->host_ip, ping_para->data_len, ping6_info->rtt, ping6_info->ttl);
						if (ping_para->ping_num == 0)
							break;						
                        if (ping6_info->rtt < (uint32_t)(ping_para->interval_time * 1000))
	            			osDelay((uint32_t)(ping_para->interval_time * 1000) - ping6_info->rtt);

						continue;
					}
				}

				tv.tv_sec = ping_para->time_out - ((osKernelGetTickCount() - start_time + 500) / 1000); // 4舍五入
				xy_printf(0,XYAPP, WARN_LOG, "reply hdr err and goto select\n");
				goto AGAIN;
			}
		}
	}
	
    user_ping_reply_info_hook(ping6_info, rsp_cmd, PING_END_REPLY);

OUT:
	if (sock != -1)
        close(sock);
	if (buff != NULL)
		xy_free(buff);
	if (recv_data != NULL)
		xy_free(recv_data);
	xy_free(rsp_cmd);
	xy_free(ping6_info->host_ip);	
	xy_free(ping6_info);

	xy_deReg_psnetif_callback(EVENT_PSNETIF_PING6, ping6_netif_event_callback);

	return ret;
}

void process_ping_task(ping_para_t *ping_para)
{
    // 域名解析获取远端ip
    struct addrinfo *result = NULL;
    struct addrinfo hint    = {0};
    int    getaddr_ret      = 0;
    hint.ai_family   = AF_UNSPEC;
    hint.ai_socktype = SOCK_DGRAM;

    if ((getaddr_ret = getaddrinfo(ping_para->host, NULL, &hint, &result)) != 0)
	{
		user_ping_reply_info_hook(NULL, NULL, PING_UNKNOWEN);
		xy_printf(0,XYAPP, WARN_LOG, "get dst addr failed and result_dns:%d\n", getaddr_ret);
	}
    else
    {
        if (result->ai_family == AF_INET)
        {
			memcpy(&ping_para->ip_addr, &(((struct sockaddr_in *)(result->ai_addr))->sin_addr), sizeof(struct in_addr)); 
            freeaddrinfo(result);
            
            _ping_ipv4(ping_para);   
        }
#if LWIP_IPV6    
        else if (result->ai_family == AF_INET6)
        {   
            memcpy(&ping_para->ip_addr, &(((struct sockaddr_in6 *)(result->ai_addr))->sin6_addr), sizeof(struct in6_addr));
			freeaddrinfo(result);

            _ping_ipv6(ping_para);
        }
#endif
    }
    
    at_ping_thread_id = NULL;
    xy_free(ping_para);
    osThreadExit();
}

int start_ping(ping_para_t *para)
{
    osThreadAttr_t task_attr = {0};

    if (at_ping_thread_id != NULL)
    {
        if (g_ping_stop == 1)
            xy_printf(0,XYAPP, WARN_LOG, "ping is stopping, please wait!");
        else
            xy_printf(0,XYAPP, WARN_LOG, "error, ping is in progress, please wait until ping finished!");

        return XY_ERR;
    }

    // process_ping_task处理完后进行释放
    ping_para_t *ping_para = (ping_para_t*)xy_malloc(sizeof(ping_para_t));
    memcpy(ping_para, para, sizeof(ping_para_t));

    task_attr.name = PING_THREAD_NAME;
	task_attr.priority = PING_THREAD_PRIO;
	task_attr.stack_size = PING_THREAD_STACK_SIZE;
    at_ping_thread_id = osThreadNew((osThreadFunc_t)(process_ping_task), ping_para, &task_attr);

    return XY_OK;
}

int stop_ping()
{
    if (at_ping_thread_id == NULL)
        return 0;

    g_ping_stop = 1;
	if(ps_is_oos()) //put semaphore when stop ping
		osSemaphoreRelease(g_out_OOS_sem);
        
	return 1;
}

#endif
