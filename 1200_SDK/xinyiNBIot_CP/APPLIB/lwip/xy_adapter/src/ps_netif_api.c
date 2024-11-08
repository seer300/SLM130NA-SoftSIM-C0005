
/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "ps_netif_api.h"
#include "xy_atc_interface.h"
#include "lwip/dns.h"
#include "lwip/nd6.h"
#include "lwip/netifapi.h"
#include "lwip/tcpip.h"
#include "lwip/ip.h"
#include "net_app_resume.h"
#include "oss_nv.h"
#include "net_api_priv.h"
#include "rtc_tmr.h"
#include "xy_system.h"
#include "xy_socket_api.h"
#include "xy_ps_api.h"
#include "atc_ps_def.h"
#include "main_proxy.h"
#if XY_PING
#include "ping_api.h"
#endif
#if XY_WIRESHARK
#include "xy_wireshark.h"
#endif

/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/
uint8_t g_udp_send_rai[MEMP_NUM_NETCONN] = {0};
uint8_t g_working_cid = INVALID_CID;
uint16_t g_udp_send_seq[MEMP_NUM_NETCONN] = {0}; // 高8位为socket fd，低8位为上行报文的sequence，用于RAI的触发
uint16_t g_udp_seq = 0XA000;     /* 非socket UDP sequence以外的上行报文的起始序列号，用于RAI的触发 */
uint16_t g_udp_latest_seq = 0; 	 /* 当最后一个上行报文被PS处理完毕后(参加urc_UDPRAI_Callback)，触发RAI的发送 */
uint8_t g_ipv6_resume_flag = 0;  /* IPv4地址恢复标志位 */
uint32_t g_rate_test = 0;      	 /* 用于下行灌包测试，1:下行数据平台不做处理 */
int32_t g_null_udp_rai = 0;      /* AT+XYRAI触发的空包携带RAI指示 */
osMutexId_t g_ps_if_mutex = NULL;
osMutexId_t g_netif_callbacklist_mutex = NULL;
osSemaphoreId_t g_net_ok_sem = NULL;
psNetifEventCallbackList_T *g_netif_callback_list = NULL;
Ps_Netif_T ps_if[PS_PDP_CID_MAX] = {{INVALID_CID, NULL}, {INVALID_CID, NULL}};

/*******************************************************************************
 *						  Local variable definitions						   *
 ******************************************************************************/
static int s_Netif_IpType = IP_TYPE_INVALID; /* 网卡IP类型 */

/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
void ip_packet_information_print(unsigned char *data, unsigned short len, char in)
{
	unsigned short total_len;
	unsigned char protocol;
	unsigned int ip_header_len;
	unsigned int sub_header_len = 0;
	unsigned int data_len = 0;
	unsigned char *cur;
	char *link_str = in ? "downlink" : "uplink";
	unsigned short src_port, dst_port;
	unsigned int seqno, ackno;
	unsigned short tcp_flags;
	char tcp_flags_str[16] = {0};

	if (HWREGB(BAK_MEM_XY_DUMP) == 0 || data == NULL)
	{
		return;
	}
	uint8_t version = (uint8_t)(data[0] >> 4);

	if ((len <= 20 && version == 4) || (len <= 40 && version == 6))
	{
		return;
	}

	if (version == 4) //ipv4
	{
		unsigned short id;
		ip4_addr_t src, dst;
		char ip_src_str[16] = {0};
		char ip_dst_str[16] = {0};
		ip_header_len = (data[0] & 0x0F) * 4;
		total_len = lwip_ntohs(*((unsigned short *)(data + 2)));
		id = lwip_ntohs(*((unsigned short *)(data + 4)));
		protocol = *((unsigned char *)(data + 9));
		src.addr = *((unsigned int *)(data + 12));
		dst.addr = *((unsigned int *)(data + 16));
		inet_ntoa_r(src.addr, ip_src_str, 16);
		inet_ntoa_r(dst.addr, ip_dst_str, 16);
		xy_printf(0,XYAPP, WARN_LOG, "ip packet info: total_len %u, id %u, protocol %d, src ip %s, dst ip %s, len %u", total_len, id, protocol, ip_src_str, ip_dst_str, len);
		if (total_len != len)
		{
			return;
		}
		cur = data + ip_header_len;
		if (protocol == IPPROTO_ICMP) // icmp
		{
			sub_header_len = 8;
			data_len = total_len - ip_header_len - sub_header_len;
			xy_printf(0,XYAPP, WARN_LOG, "ip packet info: icmp, %s, from %s to %s, data len %u", link_str, ip_src_str, ip_dst_str, data_len);
		}
		else if (protocol == IPPROTO_TCP) // tcp
		{
			src_port = lwip_ntohs(*((unsigned short *)(cur)));
			dst_port = lwip_ntohs(*((unsigned short *)(cur + 2)));
			seqno = lwip_ntohl(*((unsigned int *)(cur + 4)));
			ackno = lwip_ntohl(*((unsigned int *)(cur + 8)));
			sub_header_len = (lwip_ntohs(*((unsigned short *)(cur + 12))) >> 12) * 4;
			tcp_flags = lwip_ntohs(*((unsigned short *)(cur + 12))) & 0x0FFF;
			data_len = total_len - ip_header_len - sub_header_len;
			if (tcp_flags & 0x0010)
			{
				strcat(tcp_flags_str, "A"); // ACK
			}
			if (tcp_flags & 0x0008)
			{
				strcat(tcp_flags_str, "P"); // PUSH
			}
			if (tcp_flags & 0x0004)
			{
				strcat(tcp_flags_str, "R"); // RESET
			}
			if (tcp_flags & 0x0002)
			{
				strcat(tcp_flags_str, "S"); // SYN
			}
			if (tcp_flags & 0x0001)
			{
				strcat(tcp_flags_str, "F"); // FIN
			}
			xy_printf(0,XYAPP, WARN_LOG, "ip packet info: tcp, %s, from %s:%d to %s:%d", link_str, ip_src_str, src_port, ip_dst_str, dst_port);
			xy_printf(0,XYAPP, WARN_LOG, "ip packet info: seqno %u, ackno %u, flags %s, data len %u", seqno, ackno, tcp_flags_str, data_len);
		}
		else if (protocol == IPPROTO_UDP) // udp
		{
			src_port = lwip_ntohs(*((unsigned short *)(cur)));
			dst_port = lwip_ntohs(*((unsigned short *)(cur + 2)));
			sub_header_len = 8;
			data_len = total_len - ip_header_len - sub_header_len;
			xy_printf(0,XYAPP, WARN_LOG, "ip packet info: udp, %s, from %s:%d to %s:%d, data len %u",
						  link_str, ip_src_str, src_port, ip_dst_str, dst_port, data_len);
		}
	}
	else if (version == 6) //ipv6
	{
		ip6_addr_t src, dst;
		char ip_src_str[40] = {0};
		char ip_dst_str[40] = {0};
		ip_header_len = 40;
		protocol = *((unsigned char *)(data + 6));
		uint16_t payload_len = lwip_ntohs(*((uint16_t*)(data + 4)));
		memcpy(src.addr, (unsigned char *)(data + 8), 16);
		memcpy(dst.addr, (unsigned char *)(data + 24), 16);
		inet6_ntoa_r(src.addr, ip_src_str, 40);
		inet6_ntoa_r(dst.addr, ip_dst_str, 40);
		xy_printf(0,XYAPP, WARN_LOG, "ip6 packet info: total_len %u, protocol %d, src ip %s, dst ip %s, len %u", payload_len, protocol, ip_src_str, ip_dst_str, len);
		if (payload_len + 40 != len)
		{
			return;
		}
		cur = data + ip_header_len;
		if (protocol == IPPROTO_ICMPV6)
		{
			sub_header_len = 8;
			data_len = payload_len - sub_header_len;
			xy_printf(0,XYAPP, WARN_LOG, "ip6 packet info: icmp6, %s, from %s to %s, data len %u", link_str, ip_src_str, ip_dst_str, data_len);
		}
		else if (protocol == IPPROTO_UDP)
		{
			src_port = lwip_ntohs(*((unsigned short *)(cur)));
			dst_port = lwip_ntohs(*((unsigned short *)(cur + 2)));
			sub_header_len = 8;
			data_len = payload_len - sub_header_len;
			xy_printf(0,XYAPP, WARN_LOG, "ip6 packet info: udp, %s, from %s:%d to %s:%d, data len %u",
						  link_str, ip_src_str, src_port, ip_dst_str, dst_port, data_len);
		}
		else if (protocol == IPPROTO_TCP)
		{
			src_port = lwip_ntohs(*((unsigned short *)(cur)));
			dst_port = lwip_ntohs(*((unsigned short *)(cur + 2)));
			seqno = lwip_ntohl(*((unsigned int *)(cur + 4)));
			ackno = lwip_ntohl(*((unsigned int *)(cur + 8)));
			sub_header_len = (lwip_ntohs(*((unsigned short *)(cur + 12))) >> 12) * 4;
			tcp_flags = lwip_ntohs(*((unsigned short *)(cur + 12))) & 0x0FFF;
			data_len = payload_len - sub_header_len;
			if (tcp_flags & 0x0010)
			{
				strcat(tcp_flags_str, "A"); // ACK
			}
			if (tcp_flags & 0x0008)
			{
				strcat(tcp_flags_str, "P"); // PUSH
			}
			if (tcp_flags & 0x0004)
			{
				strcat(tcp_flags_str, "R"); // RESET
			}
			if (tcp_flags & 0x0002)
			{
				strcat(tcp_flags_str, "S"); // SYN
			}
			if (tcp_flags & 0x0001)
			{
				strcat(tcp_flags_str, "F"); // FIN
			}
			xy_printf(0,XYAPP, WARN_LOG, "ip6 packet info: tcp, %s, from %s:%d to %s:%d", link_str, ip_src_str, src_port, ip_dst_str, dst_port);
			xy_printf(0,XYAPP, WARN_LOG, "ip6 packet info: seqno %u, ackno %u, flags %s, data len %u", seqno, ackno, tcp_flags_str, data_len);			
		}
	}
}

struct ps_netif *find_netif_by_cid(unsigned char cid)
{
	int i = 0;
	struct ps_netif *tmp = NULL;

	xy_mutex_acquire(g_ps_if_mutex, osWaitForever);
	for (; i < PS_PDP_CID_MAX; i++)
	{
		if (ps_if[i].cid == cid)
			tmp = ps_if + i;
	}
	xy_mutex_release(g_ps_if_mutex);

	return tmp;
}

//deliver IP packet to lwip tcpip stack
int send_ps_packet_to_tcpip(void *data, unsigned short len, unsigned char cid)
{
	int err;
	struct pbuf *p;
	struct ps_netif *ps_temp;

	ps_temp = find_netif_by_cid(cid);
	if (ps_temp == NULL || !netif_is_link_up(ps_temp->ps_eth))
	{
		xy_free(data);
		return XY_ERR;
	}

	net_resume();

	p = pbuf_alloc(PBUF_RAW, len, PBUF_REF);
	xy_assert(p != NULL);

	p->payload = data;
	p->payload_original = data; //pbuf->payload的指针位置随时会被修改
	err = ps_temp->ps_eth->input(p, ps_temp->ps_eth);
	if (err != ERR_OK)
	{
		xy_printf(0,XYAPP, WARN_LOG, "send ps packet to tcpip ERR!");
		pbuf_free(p); //tcpip_input的返回,需要手动释放pbuf
		return XY_ERR;
	}
	return XY_OK;
}

int send_packet_to_user(unsigned char cid, int len, char *data)
{
	/* 芯翼内部灌包测试使用，开启后平台不处理下行数据 */
	if (g_rate_test == 1)
		return 0;
	xy_printf(0, XYAPP, WARN_LOG, "send_packet_to_proxy");
	proxy_downlink_data_t downlink_data = {0};
	downlink_data.cid = cid;
	downlink_data.len = len;
	downlink_data.data = data;

	if (send_msg_2_proxy(PROXY_MSG_IPDATA, &downlink_data, sizeof(proxy_downlink_data_t)) == 0)
	{
		xy_printf(0, XYAPP, WARN_LOG, "ipdata proxy queue full, drop!");
		xy_free(data);
	}
	return 1;
}

err_t send_ip_packet_to_ps_net(struct netif *netif, struct pbuf *p)
{
	void *user_ipdata = NULL;
	Ps_Ipdata_Info_T ps_ipdata_info = {0};
	struct pbuf* q = p;
	unsigned short pos_offset = p->tot_len;
	int udp_seq_id = -1;

	if(is_Uplink_FlowCtl_Open())
		return ERR_FLOWCTL;
	
	if (q->tot_len > 0)
	{
		uint16_t offset_to = 0;
		user_ipdata = (void *)xy_malloc(pos_offset);
		memset(user_ipdata, 0, pos_offset);
		xy_printf(0,XYAPP, WARN_LOG, "ip data write to ps,mem addr=0x%X", user_ipdata);
		do
		{
			if (q->soc_id >= 0 && q->soc_id < MEMP_NUM_NETCONN)
				udp_seq_id = q->soc_id;
			memcpy((char *)user_ipdata + offset_to, (char *)q->payload, q->len);
			offset_to += q->len;
			q = q->next;
		} while (q);
	}
	
	if(g_null_udp_rai == 1)
	{
		ps_ipdata_info.rai = RAI_REL_UP;
		ps_ipdata_info.cid = 0xFD;
		g_null_udp_rai = 0;
		xy_printf(0, XYAPP, WARN_LOG, "g_null_udp_rai: %d,cid: %d", g_null_udp_rai, ps_ipdata_info.cid);
	}
	else if(udp_seq_id != -1 && g_udp_send_rai[udp_seq_id] != RAI_NULL)
	{
		ps_ipdata_info.rai = g_udp_send_rai[udp_seq_id];
		ps_ipdata_info.cid = g_working_cid;
	}
	else
	{
		ps_ipdata_info.rai = RAI_NULL;
		ps_ipdata_info.cid = g_working_cid;
	}

	// xy_printf(0, XYAPP, WARN_LOG, "udp_seq_id=%d, rai=%d", udp_seq_id, g_udp_send_rai[udp_seq_id]);
	if (udp_seq_id != -1)
	{
		xy_printf(0, XYAPP, WARN_LOG, "g_udp_send_seq=%d,rai=%d", g_udp_send_seq[udp_seq_id], g_udp_send_rai[udp_seq_id]);
	}
	ps_ipdata_info.data_type = 0;
	ps_ipdata_info.data_len = p->tot_len;

	//used for UDP local sequence for 3GPP reliable transmission
	if (udp_seq_id != -1 && g_udp_send_seq[udp_seq_id] != 0)
	{
		ps_ipdata_info.sequence = g_udp_send_seq[udp_seq_id];		
	}
	else
	{	
		ps_ipdata_info.sequence = 0;	
	}
	
	ip_packet_information_print(user_ipdata, p->tot_len, 0);

#if XY_WIRESHARK
	wireshark_forward_format_print(user_ipdata, p->tot_len, 0);
#endif

	API_Send_Data_2_PS(ps_ipdata_info.cid, ps_ipdata_info.rai, ps_ipdata_info.data_type,
					   ps_ipdata_info.data_len, user_ipdata, ps_ipdata_info.sequence);
END:
	if (udp_seq_id != -1)
	{
		g_udp_send_rai[udp_seq_id] = RAI_NULL;
		g_udp_send_seq[udp_seq_id] = 0;
	}
	return ERR_OK;
}

void proc_downlink_packet(proxy_downlink_data_t* downlink_data)
{
	xy_assert(downlink_data != NULL);
	xy_printf(0,XYAPP, WARN_LOG, "recv ipdata from nas!!");

	unsigned char cid = downlink_data->cid;
	int len = downlink_data->len;
	char* rcv_data = downlink_data->data;

	if (is_sleep_locked(LPM_DEEPSLEEP) == 0)
	{
		xy_printf(0,XYAPP, WARN_LOG, "worklock have free,but recv downlink packet!!!");
	}

	ip_packet_information_print(rcv_data, len, 1);

#if XY_WIRESHARK
	wireshark_forward_format_print((void *)rcv_data, len, 1);
#endif

	send_ps_packet_to_tcpip(rcv_data, len, cid);
	xy_printf(0,XYAPP, WARN_LOG, "send ps packet to tcpip len:%d", len);
}

err_t netif_ps_output_ipv4(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr)
{
	UNUSED_ARG(ipaddr);
	return send_ip_packet_to_ps_net(netif,p);
}

err_t netif_ps_output_ipv6(struct netif *netif, struct pbuf *p, const ip6_addr_t *ipaddr)
{
	UNUSED_ARG(ipaddr);
	return send_ip_packet_to_ps_net(netif,p);
}

// 建议支持ipv6业务时，mtu取值在1280-1500字节，仅支持ipv4业务时，取值在128-1500
static uint16_t ps_netif_get_mtu()
{
	// 获取顺序：PS配置 -> 平台出厂NV -> 默认值
	uint16_t mtu = DEFAULT_MTU_LENGTH;
	uint16_t ps_mtu = 0;
	if (g_working_cid != INVALID_CID && xy_get_ipv4_mtu(g_working_cid, &ps_mtu) == ATC_AP_TRUE && ps_mtu > 0)
	{
		mtu = ps_mtu;
		xy_printf(0, XYAPP, WARN_LOG, "get mtu val from ps:%d", mtu);
	}
	else if (g_softap_fac_nv->mtu)
	{
		mtu = g_softap_fac_nv->mtu;
		xy_printf(0, XYAPP, WARN_LOG, "get mtu val from facnv:%d", mtu);
	}
	else
	{
		xy_printf(0, XYAPP, WARN_LOG, "get mtu val from defconfig:%d", mtu);
	}
	return mtu;
}

err_t ps_netif_init(struct netif* netif)
{
	PsNetifInfo *pdpinfo = (PsNetifInfo *)(netif->state);
#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

    netif->name[0] = 'x';
    netif->name[1] = 'y';

	netif->mtu = ps_netif_get_mtu();	
    netif->linkoutput = NULL;
	netif->hwaddr_len = NETIF_MAX_HWADDR_LEN;
	
#if LWIP_IPV4
	if (pdpinfo->ip_type == D_PDP_TYPE_IPV4 || pdpinfo->ip_type == D_PDP_TYPE_IPV4V6)
	{
    	netif->output = (netif_output_fn)netif_ps_output_ipv4;
	}
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
if (pdpinfo->ip_type == D_PDP_TYPE_IPV6 || pdpinfo->ip_type == D_PDP_TYPE_IPV4V6)
{
	netif->output_ip6 = (netif_output_ip6_fn)netif_ps_output_ipv6;
	if (resume_ipv6_addr(&pdpinfo->ip6_local))
	{
		netif_set_ip6_autoconfig_enabled(netif, 0);
		//设置可用v6地址
		netif_ip6_addr_set(netif, 0, &pdpinfo->ip6_local);
		netif_ip6_addr_set_state(netif, 0, IP6_ADDR_VALID);
		ip6_addr_t v6_addr = {0};
		memcpy(&v6_addr, g_softap_var_nv->ipv6_addr, sizeof(g_softap_var_nv->ipv6_addr));
		netif_ip6_addr_set(netif, 1, &v6_addr);
		netif_ip6_addr_set_state(netif, 1, IP6_ADDR_PREFERRED);
	}
	else
	{
		netif_set_ip6_autoconfig_enabled(netif, 1);
		netif_ip6_addr_set(netif, 0, &pdpinfo->ip6_local);
		netif_ip6_addr_set_state(netif, 0, IP6_ADDR_VALID);
		/*
		 * 入网入库版本测试用例不允许开机后发RS包
		 * ra_timeout nv设置为65535时,不发送RS包,一直等待接收RA包
		 */
		if (g_softap_fac_nv->ra_timeout == 0xFFFF)
		{
			netif->rs_count = 0;
		}
		else
		{
			//ra timeout nv设置为0时，默认超时时间200ms
			int delay = ((g_softap_fac_nv->ra_timeout == 0) ? 200 : g_softap_fac_nv->ra_timeout * 1000);
			if (delay > 0)
			{
				tcpip_timeout(delay, nd6_ra_timeout_handler, netif);
				nd6_ra_timer_flag = 1;
			}
			else
			{
				netif->rs_count = LWIP_ND6_MAX_MULTICAST_SOLICIT;
				nd6_ra_timer_flag = 0;
			}
		}
		nd6_rs_timeout_flag = 0;
	}
	
}
#endif /* LWIP_IPV6 */
    return ERR_OK;
}

void add_netif(uint8_t cid, struct netif *ps_eth)
{	
	int i = 0;
	
	xy_mutex_acquire(g_ps_if_mutex, osWaitForever);	
	for (; i < PS_PDP_CID_MAX; i++)
	{
		if (ps_if[i].ps_eth == NULL && ps_if[i].cid == INVALID_CID)
		{
			ps_if[i].ps_eth = ps_eth;
			ps_if[i].cid = cid;
			xy_mutex_release(g_ps_if_mutex);
			return;
		}
	}

	xy_assert(0);
}

void delete_netif(uint8_t cid)
{
	int i = 0;

	xy_mutex_acquire(g_ps_if_mutex, osWaitForever);	
	for (; i < PS_PDP_CID_MAX; i++)
	{
		if (ps_if[i].cid == cid)
		{
			//主控线程处理完pdp激活消息后，pdpinfo会被释放，lwip内部如果使用会出现野指针，因此额外申请了一块内存，需单独释放
			if (ps_if[i].ps_eth->state != NULL)
			{
				xy_free(ps_if[i].ps_eth->state);
				ps_if[i].ps_eth->state = NULL;
			}
			if (ps_if[i].ps_eth != NULL)
			{
				xy_free(ps_if[i].ps_eth);
				ps_if[i].ps_eth = NULL;
			}
			ps_if[i].cid = INVALID_CID;
			xy_mutex_release(g_ps_if_mutex);
			return;
		}
	}
	xy_assert(0);
}

struct netif* find_active_netif()
{
	int i = 0;
	struct netif* active_netif = NULL;

	xy_mutex_acquire(g_ps_if_mutex, osWaitForever);	
	for (; i < PS_PDP_CID_MAX; i++)
	{
		if (ps_if[i].ps_eth != NULL)
			active_netif = ps_if[i].ps_eth;
	}
	xy_mutex_release(g_ps_if_mutex);

	return active_netif;
}

bool is_netif_active(uint8_t cid)
{
	int i = 0;
	bool ret = false;

	xy_mutex_acquire(g_ps_if_mutex, osWaitForever);
	for (; i < PS_PDP_CID_MAX; i++)
	{
		if (ps_if[i].cid == cid && ps_if[i].ps_eth != NULL)
			ret = true;
	}
	xy_mutex_release(g_ps_if_mutex);

	return ret;
}

/*lwip线程，当网口状态变化时调用，深睡唤醒场景下，当IPV4/IPV6地址发生变化时，才调用psNetifEventInd通知应用层*/
void netif_status_callback_proc(struct netif *netif)
{
	if (netif->flags & NETIF_FLAG_UP)
	{
		char* ipaddr  = xy_malloc(XY_IPADDR_STRLEN_MAX);
		char* ipaddr1 = xy_malloc(XY_IPADDR_STRLEN_MAX);
		if (!ip4_addr_isany(netif_ip4_addr(netif)) && netif_ip6_addr_state(netif, 0) == IP6_ADDR_INVALID)
		{
			/* IPv4 Only */
			s_Netif_IpType = IPV4_TYPE;
			ipaddr_ntoa_r(netif_ip_addr4(netif), ipaddr, XY_IPADDR_STRLEN_MAX);
			xy_printf(0, XYAPP, WARN_LOG, "[netif cb]only ipv4 and addr:%s[0x%X]", ipaddr, g_softap_var_nv->ipv4_addr);
			if (g_softap_var_nv->ipv4_addr != netif_ip4_addr(netif)->addr)
			{
				psNetifEventInd(EVENT_PSNETIF_IPV4_VALID);
			}
		}
		else if (ip4_addr_isany(netif_ip4_addr(netif)) && netif_ip6_addr_state(netif, 1) == IP6_ADDR_PREFERRED)
		{
			/* IPv6 Only */
			s_Netif_IpType = IPV6_TYPE;
			ipaddr_ntoa_r(netif_ip_addr6(netif, 1), ipaddr, XY_IPADDR_STRLEN_MAX);
			xy_printf(0, XYAPP, WARN_LOG, "[netif cb]ipv6 only and addr:%s[0x%X%X]", ipaddr, g_softap_var_nv->ipv6_addr[14], g_softap_var_nv->ipv6_addr[15]);
			if (memcmp(g_softap_var_nv->ipv6_addr, netif_ip6_addr(netif, 1), sizeof(g_softap_var_nv->ipv6_addr)))
			{
				psNetifEventInd(EVENT_PSNETIF_IPV6_VALID);
			}
		}
		else if (!ip4_addr_isany(netif_ip4_addr(netif)) && netif_ip6_addr_state(netif, 1) == IP6_ADDR_PREFERRED)
		{
			/* IPv4v6 */
			s_Netif_IpType = IPV46_TYPE;
			ipaddr_ntoa_r(netif_ip_addr4(netif), ipaddr, XY_IPADDR_STRLEN_MAX);
			ipaddr_ntoa_r(netif_ip_addr6(netif, 1), ipaddr1, XY_IPADDR_STRLEN_MAX);
			xy_printf(0, XYAPP, WARN_LOG, "[netif cb]ipv4v6 valid, ipv4:%s[0x%X],ipv6:%s[0x%X%X]", ipaddr, g_softap_var_nv->ipv4_addr, ipaddr1, g_softap_var_nv->ipv6_addr[14], g_softap_var_nv->ipv6_addr[15]);
			if ( (g_softap_var_nv->ipv4_addr != netif_ip4_addr(netif)->addr) || memcmp(g_softap_var_nv->ipv6_addr, netif_ip6_addr(netif, 1), sizeof(g_softap_var_nv->ipv6_addr)) )
			{			
				psNetifEventInd(EVENT_PSNETIF_IPV4V6_VALID);
			}
		}
		else if (netif_ip6_addr_state(netif, 0) == IP6_ADDR_VALID)
		{
			if (netif_ip6_addr_state(netif, 1) == IP6_ADDR_TENTATIVE && nd6_rs_timeout_flag == 1)
			{
				/* rs包发送超时后，再次收到RA包，需要重置nd6_rs_timeout_flag，否则IPv6地址状态变化时，会错误上报 */
				nd6_rs_timeout_flag = 0;
			}

			if (nd6_rs_timeout_flag == 1)
			{
				if (ip4_addr_isany(netif_ip4_addr(netif)))
				{
					//发送rs包超时，上报无可用ip地址消息
					s_Netif_IpType = IPV6PREPARING_TYPE;
					xy_printf(0, XYAPP, WARN_LOG, "[netif cb]ipv6 only,but rs send timeout");
					psNetifEventInd(EVENT_PSNETIF_INVALID);
				}
				else
				{
					s_Netif_IpType = IPV4_IPV6PREPARING_TYPE;
					ipaddr_ntoa_r(netif_ip_addr4(netif), ipaddr, XY_IPADDR_STRLEN_MAX);
					xy_printf(0, XYAPP, WARN_LOG, "[netif cb]ipv4v6, but rs send timeout, use ipv4:%s[0x%X]", ipaddr, g_softap_var_nv->ipv4_addr);
					if (g_softap_var_nv->ipv4_addr != netif_ip4_addr(netif)->addr)
					{
						psNetifEventInd(EVENT_PSNETIF_IPV4_VALID);
					}
				}
			}
			else
			{
				if (ip4_addr_isany(netif_ip4_addr(netif)))
				{
					//发送rs包超时，上报无可用ip地址消息
					s_Netif_IpType = IPV6PREPARING_TYPE;
					xy_printf(0, XYAPP, WARN_LOG, "[netif cb]ipv6 only, ipv6 addr preparing");
				}
				else
				{
					s_Netif_IpType = IPV4_IPV6PREPARING_TYPE;
					xy_printf(0, XYAPP, WARN_LOG, "[netif cb]ipv4v6, ipv6 addr preparing");
				}
			}
		}

		/** 
		 * 深睡唤醒后，当网卡可用时释放信号量，通知xy_tcpip_is_ok等网路状态判断接口 
		 * 标志位的作用是避免深睡唤醒后，执行网络去激活，再次激活，不需要使用信号量
		 */
		if (netif_is_wakeup_suitation())
		{
			osSemaphoreRelease(g_net_ok_sem);
		}

		/* 保存ipv6地址到易变nv，用于深睡唤醒恢复 */
		if (netif_ip6_addr_state(netif, 1) == IP6_ADDR_PREFERRED)
		{
			memcpy(g_softap_var_nv->ipv6_addr, netif_ip6_addr(netif, 1), sizeof(ip6_addr_t));
		}
		// 保存IPv4地址，若出现深睡IPv4地址保持不变的情况，不通知应用层
		memcpy(&g_softap_var_nv->ipv4_addr, netif_ip4_addr(netif), sizeof(g_softap_var_nv->ipv4_addr));

		xy_free(ipaddr);
		xy_free(ipaddr1);
	}
	else
	{
		// 去激活前删除可能存在的nd6 ra定时器，避免超时后访问非法地址
		sys_untimeout(nd6_ra_timeout_handler, netif);
		s_Netif_IpType = IP_TYPE_INVALID;
		memset(g_softap_var_nv->ipv6_addr, 0, sizeof(ip6_addr_t));
		xy_printf(0, XYAPP, WARN_LOG, "[netif cb]netif down");
		psNetifEventInd(EVENT_PSNETIF_INVALID);
	}
}

//pdp激活
void ps_netif_activate(PsNetifInfo *pdp_info)
{
	struct netif *netif_new;

	user_dns_config(pdp_info->dns, pdp_info->ip_type);

	netif_new = xy_malloc(sizeof(struct netif));
	memset(netif_new, 0, sizeof(struct netif));
	add_netif(pdp_info->workingCid, netif_new);

	//主控线程处理完pdp激活消息后，pdpinfo会被释放，lwip内部如果使用会出现野指针，需要额外申请一块内存
	PsNetifInfo *pdp_info_backup = (PsNetifInfo *)xy_malloc(sizeof(PsNetifInfo));
	memcpy(pdp_info_backup, pdp_info, sizeof(PsNetifInfo));

	netifapi_netif_add(netif_new, &pdp_info->ip4, NULL, NULL, (void *)(pdp_info_backup), ps_netif_init, tcpip_input);
	netif_set_status_callback(netif_new, netif_status_callback_proc);
	netifapi_netif_set_default(netif_new);	
	netifapi_netif_set_link_up(netif_new);
	netifapi_netif_set_up(netif_new);
}

//pdp去激活
int ps_netif_deactivate(uint8_t cid)
{
	struct ps_netif *netif_tmp = find_netif_by_cid(cid);
	if (netif_tmp == NULL)
	{
		xy_printf(0,XYAPP, WARN_LOG, "deactive cid:%u pdp fail", cid);
		return XY_ERR;
	}

	netifapi_netif_remove(netif_tmp->ps_eth);
	delete_netif(cid);

	return XY_OK;
}

bool resume_ipv6_addr(ip6_addr_t* ip6_addr)
{
	if (g_ipv6_resume_flag)
	{
		//check ipv6 后64位是否变化
		g_ipv6_resume_flag = 0;
		ip6_addr_t ipv6_addr = {0};
		memcpy(&ipv6_addr, g_softap_var_nv->ipv6_addr, sizeof(g_softap_var_nv->ipv6_addr));
		if (ip6_addr_nethostcmp(&ipv6_addr, ip6_addr))
		{
			return true;
		}
	}
	return false;
}

int xy_get_netif_iptype()
{
	return s_Netif_IpType;
}

void psNetifEventInd(PsStateChangeEvent eventId)
{
	if (g_netif_callback_list == NULL)
		return;

	osMutexAcquire(g_netif_callbacklist_mutex, osWaitForever);

	//执行回调
	psNetifEventCallbackList_T *tmp = g_netif_callback_list;
	while (tmp != NULL)
	{
		if (tmp->callback != NULL && (eventId & tmp->eventGroup))
			tmp->callback(eventId);
		tmp = tmp->next;
	}

	osMutexRelease(g_netif_callbacklist_mutex);
}

void user_dns_config(ip_addr_t *dns_ipaddr, uint8_t ip_type)
{
	uint8_t dns_index = 0;
    ip_addr_t dns_default[DNS_MAX_SERVERS] = {0};

    if (XY_ERR == cloud_read_file(DNS_SERVER_ADDR_NVM_FILE_NAME, (void *)&dns_default[0], sizeof(ip_addr_t) * DNS_MAX_SERVERS))
    {
        // 这里需保证保证文件系统里有默认dns配置
		xy_printf(0, PLATFORM, WARN_LOG, "[user_dns_config]nvm dns read fail");
		if (ipaddr_aton(XY_DEFAULT_V4_PRIDNS, &dns_default[DEFAULT_V4_PRIDNS_INDEX]))
			dns_setserver(DEFAULT_V4_PRIDNS_INDEX, &dns_default[DEFAULT_V4_PRIDNS_INDEX]);

		if (ipaddr_aton(XY_DEFAULT_V4_SECDNS, &dns_default[DEFAULT_V4_SECDNS_INDEX]))
			dns_setserver(DEFAULT_V4_SECDNS_INDEX, &dns_default[DEFAULT_V4_SECDNS_INDEX]);
	
		if (ipaddr_aton(XY_DEFAULT_V6_PRIDNS, &dns_default[DEFAULT_V6_PRIDNS_INDEX]))
			dns_setserver(DEFAULT_V6_PRIDNS_INDEX, &dns_default[DEFAULT_V6_PRIDNS_INDEX]);

		if (ipaddr_aton(XY_DEFAULT_V6_SECDNS, &dns_default[DEFAULT_V6_SECDNS_INDEX]))
			dns_setserver(DEFAULT_V6_SECDNS_INDEX, &dns_default[DEFAULT_V6_SECDNS_INDEX]);	 
    }
	else
	{
		xy_printf(0, PLATFORM, WARN_LOG, "[user_dns_config]nvm dns read ok");
	}

	/* OPENCPU文件系统读取无效，直接根据协议栈上报及代码默认写死的dns进行配置 */
    {
        switch (ip_type)
        {
            case D_PDP_TYPE_IPV4:
                if (!ip_addr_cmp(&dns_ipaddr[DEFAULT_V4_PRIDNS_INDEX], IP4_ADDR_ANY))
                    xy_dns_set2(&dns_ipaddr[DEFAULT_V4_PRIDNS_INDEX], dns_index++, false);
                else if (!ip_addr_isany(&dns_default[DEFAULT_V4_PRIDNS_INDEX]))
                    xy_dns_set2(&dns_default[DEFAULT_V4_PRIDNS_INDEX], dns_index++, false);
                if (!ip_addr_cmp(&dns_ipaddr[DEFAULT_V4_SECDNS_INDEX], IP4_ADDR_ANY))
                    xy_dns_set2(&dns_ipaddr[DEFAULT_V4_SECDNS_INDEX], dns_index++, false);
                else if (!ip_addr_isany(&dns_default[DEFAULT_V4_SECDNS_INDEX]))
                    xy_dns_set2(&dns_default[DEFAULT_V4_SECDNS_INDEX], dns_index++, false);
            break;
            case D_PDP_TYPE_IPV6:
                if (!ip_addr_cmp(&dns_ipaddr[DEFAULT_V6_PRIDNS_INDEX], IP6_ADDR_ANY))
                    xy_dns_set2(&dns_ipaddr[DEFAULT_V6_PRIDNS_INDEX], dns_index++, false);
                else if (!ip_addr_isany(&dns_default[DEFAULT_V6_PRIDNS_INDEX]))
                    xy_dns_set2(&dns_default[DEFAULT_V6_PRIDNS_INDEX], dns_index++, false);
                if (!ip_addr_cmp(&dns_ipaddr[DEFAULT_V6_SECDNS_INDEX], IP6_ADDR_ANY))
                    xy_dns_set2(&dns_ipaddr[DEFAULT_V6_SECDNS_INDEX], dns_index++, false);
                else if (!ip_addr_isany(&dns_default[DEFAULT_V6_SECDNS_INDEX]))
                    xy_dns_set2(&dns_default[DEFAULT_V6_SECDNS_INDEX], dns_index++, false);
            break;
            case D_PDP_TYPE_IPV4V6:
                if (!ip_addr_cmp(&dns_ipaddr[DEFAULT_V4_PRIDNS_INDEX], IP4_ADDR_ANY))
                    xy_dns_set2(&dns_ipaddr[DEFAULT_V4_PRIDNS_INDEX], dns_index++, false);
                else if (!ip_addr_isany(&dns_default[DEFAULT_V4_PRIDNS_INDEX]))
                    xy_dns_set2(&dns_default[DEFAULT_V4_PRIDNS_INDEX], dns_index++, false);
                if (!ip_addr_cmp(&dns_ipaddr[DEFAULT_V4_SECDNS_INDEX], IP4_ADDR_ANY))
                    xy_dns_set2(&dns_ipaddr[DEFAULT_V4_SECDNS_INDEX], dns_index++, false);
                else if (!ip_addr_isany(&dns_default[DEFAULT_V4_SECDNS_INDEX]))
                    xy_dns_set2(&dns_default[DEFAULT_V4_SECDNS_INDEX], dns_index++, false);
                if (!ip_addr_cmp(&dns_ipaddr[DEFAULT_V6_PRIDNS_INDEX], IP6_ADDR_ANY))
                    xy_dns_set2(&dns_ipaddr[DEFAULT_V6_PRIDNS_INDEX], dns_index++, false);
                else if (!ip_addr_isany(&dns_default[DEFAULT_V6_PRIDNS_INDEX]))
                    xy_dns_set2(&dns_default[DEFAULT_V6_PRIDNS_INDEX], dns_index++, false);
                if (!ip_addr_cmp(&dns_ipaddr[DEFAULT_V6_SECDNS_INDEX], IP6_ADDR_ANY))
                    xy_dns_set2(&dns_ipaddr[DEFAULT_V6_SECDNS_INDEX], dns_index++, false);
                else if (!ip_addr_isany(&dns_default[DEFAULT_V6_SECDNS_INDEX]))
                    xy_dns_set2(&dns_default[DEFAULT_V6_SECDNS_INDEX], dns_index++, false);
            break;                 
            default:
            break;
        }
    }
}

#define DNS_SERVER_ADDR_EMPTY(addr) ((addr) == NULL || strlen(addr) == 0)
void dns_server_init(void)
{
	ip_addr_t dns_default[DNS_MAX_SERVERS] = {0};

	if (XY_ERR == cloud_read_file(DNS_SERVER_ADDR_NVM_FILE_NAME, (void *)&dns_default[0], sizeof(ip_addr_t)*DNS_MAX_SERVERS))
	{
		if (  DNS_SERVER_ADDR_EMPTY(XY_DEFAULT_V4_PRIDNS) || DNS_SERVER_ADDR_EMPTY(XY_DEFAULT_V4_SECDNS) \ 
		   || DNS_SERVER_ADDR_EMPTY(XY_DEFAULT_V6_PRIDNS) || DNS_SERVER_ADDR_EMPTY(XY_DEFAULT_V6_SECDNS))
		{
			// 四个DNS服务器IP地址必须全部设好
			return;
		}

		memset(dns_default, 0, sizeof(ip_addr_t)*DNS_MAX_SERVERS);

		if (ipaddr_aton(XY_DEFAULT_V4_PRIDNS, &dns_default[DEFAULT_V4_PRIDNS_INDEX]))
			dns_setserver(DEFAULT_V4_PRIDNS_INDEX, &dns_default[DEFAULT_V4_PRIDNS_INDEX]);

		if (ipaddr_aton(XY_DEFAULT_V4_SECDNS, &dns_default[DEFAULT_V4_SECDNS_INDEX]))
			dns_setserver(DEFAULT_V4_SECDNS_INDEX, &dns_default[DEFAULT_V4_SECDNS_INDEX]);
	
		if (ipaddr_aton(XY_DEFAULT_V6_PRIDNS, &dns_default[DEFAULT_V6_PRIDNS_INDEX]))
			dns_setserver(DEFAULT_V6_PRIDNS_INDEX, &dns_default[DEFAULT_V6_PRIDNS_INDEX]);

		if (ipaddr_aton(XY_DEFAULT_V6_SECDNS, &dns_default[DEFAULT_V6_SECDNS_INDEX]))
			dns_setserver(DEFAULT_V6_SECDNS_INDEX, &dns_default[DEFAULT_V6_SECDNS_INDEX]);	 

        cloud_save_file(DNS_SERVER_ADDR_NVM_FILE_NAME, (void *)&dns_default[0], sizeof(ip_addr_t)*DNS_MAX_SERVERS);
	}
}

bool netif_is_wakeup_suitation()
{
	if (Is_WakeUp_From_Dsleep() && CONVERT_RTCTICK_TO_MS((get_utc_tick() - g_sys_start_time)) < NETIF_WAKEUP_WAIT_MAX)
	{
		return true;
	}
	return false;
}

void netif_regist_init()
{
	g_out_OOS_sem = osSemaphoreNew(1, 0, NULL);
	g_net_ok_sem = osSemaphoreNew(1, 0, NULL);
    osMutexAttr_t mutex_attr = {0};
    mutex_attr.attr_bits = osMutexRecursive;
	g_ps_if_mutex = osMutexNew(&mutex_attr);
	g_netif_callbacklist_mutex = osMutexNew(NULL);
	g_cloud_fs_mutex = osMutexNew(NULL);
	g_ipv6_resume_flag = (Is_WakeUp_From_Dsleep()) ? 1 : 0;
}
