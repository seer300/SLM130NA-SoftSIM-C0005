/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_ps_cmd.h"
#include "at_com.h"
#include "at_ctl.h"
#include "atc_ps.h"
#include "factory_nv.h"
#include "low_power.h"
#include "main_proxy.h"
#include "net_api_priv.h"
#include "oss_nv.h"
#include "ps_netif_api.h"
#include "softap_nv.h"
#include "xy_at_api.h"
#include "xy_atc_interface.h"
#include "xy_net_api.h"
#include "xy_ps_api.h"
#include "xy_system.h"
#include "at_tcpip_api.h"
#include "lwip/ip_addr.h"
#include "lwip/sockets.h"
#include "xy_socket_api.h"
#include "xy_utils_hook.h"

/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/
int g_Sim_Is_Valid = 1;
uint8_t g_OOS_flag = 0;
osSemaphoreId_t g_out_OOS_sem = NULL;

/*******************************************************************************
 *                       Local function implementations	                   *
 ******************************************************************************/
static bool send_null_udp_paket(char *remote_ip,unsigned short remote_port)
{	
	int s = -1;
	struct sockaddr_in remote_sockaddr = {0};

	s = socket(AF_INET/*srv.family*/, SOCK_DGRAM, IPPROTO_UDP);

	if (s < 0)
	{
		xy_printf(0, XYAPP, WARN_LOG, "xyari create null udp socket fail");
		return false;
	}

	remote_sockaddr.sin_family = AF_INET;
	remote_sockaddr.sin_port = htons(remote_port);
	if(1 != inet_aton(remote_ip, &remote_sockaddr.sin_addr))
	{
		close(s);
		xy_printf(0, XYAPP, WARN_LOG, "xyari open null udp socket fail");
		return false;
	}
	
	xy_printf(0, XYAPP, WARN_LOG, "xyrai open null udp socket success");

	if (sendto2(s, "A", 1, 0, (struct sockaddr *)&remote_sockaddr, sizeof(struct sockaddr_in), 0, 1) < 0)
	{
		xy_printf(0, XYAPP, WARN_LOG, "send_errno:%d", errno);
		close(s);
		return false;
	}

	xy_printf(0, XYAPP, WARN_LOG, "xyrai null udp send success");
	close(s);
	xy_printf(0, XYAPP, WARN_LOG, "xyrai close null udp socket success");

	return true;
}



/*******************************************************************************
 *                       Global function implementations	                   *
 ******************************************************************************/
int at_ECHOMODE_req(char *at_buf, char **prsp_cmd)
{
    if (g_req_type == AT_CMD_ACTIVE)
    {
		uint8_t echo_mode = 0;
        if (at_parse_param("%1d(0-1)", at_buf, &echo_mode) != AT_OK)
        {
			return ATERR_PARAM_INVALID;
        }
#if VER_BC95
		/* 移远版本设置后立即生效并保存NV */
        g_softap_fac_nv->echo_mode = echo_mode;
        SAVE_FAC_PARAM(echo_mode);
#endif /* VER_BC95 */

#if VER_BC25
		g_softap_var_nv->echo_mode = echo_mode;
#endif
		set_echo_mode(echo_mode);
    }
	else
	{
		return ATERR_PARAM_INVALID;
	}
	return AT_END;
}

int at_CMEE_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		uint8_t cmee_mode = 0;
		if (at_parse_param("%1d(0-2)", at_buf, &cmee_mode) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}

#if VER_BC25
		g_softap_var_nv->cmee_mode = cmee_mode;
#elif VER_BC95
		g_softap_fac_nv->cmee_mode = cmee_mode;
		SAVE_FAC_PARAM(cmee_mode);
#endif
		set_cmee_mode(cmee_mode);
	}
		else if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(32);
#if VER_BC25
		snprintf(*prsp_cmd, 32, "\r\n+CMEE: %d\r\n\r\nOK\r\n", get_cmee_mode());
#else
		snprintf(*prsp_cmd, 32, "\r\n+CMEE:%d\r\n\r\nOK\r\n", get_cmee_mode());
#endif
	}
#if (AT_CUT != 1)
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(32);
#if VER_BC25
		snprintf(*prsp_cmd, 32, "\r\n+CMEE: (0-2)\r\n\r\nOK\r\n");
#else
		snprintf(*prsp_cmd, 32, "\r\n+CMEE:(0-2)\r\n\r\nOK\r\n");
#endif
	}
#endif
	else
	{
		return ATERR_PARAM_INVALID;
	}
	return AT_END;
}

//+CGATT？命令，根据netif状态上报网络结果,以解决TCPIP启动需要的耗时，确保客户收到应答后IP网络已好
int at_CGATT_req(char *at_buf, char **prsp_cmd)
{
	UNUSED_ARG(at_buf);
	if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(32);
		if (g_factory_nv->tNvData.tNasNv.ucAtHeaderSpaceFlg == 1 || VER_BC25)
			sprintf(*prsp_cmd, "\r\n+CGATT: %d\r\n\r\nOK\r\n", psnetif_is_ok());
		else
			sprintf(*prsp_cmd, "\r\n+CGATT:%d\r\n\r\nOK\r\n", psnetif_is_ok());
		return AT_END;
	}
	else
		return AT_FORWARD;
}

int at_NSET_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		char module[24] = {0};
		uint8_t para = 0;

		if (at_parse_param("%24s", at_buf, module) != AT_OK)
        {
			return AT_FORWARD;
        }

	    if ((at_strcasecmp(module, "STORAGE_OPER")))
		{
			if(at_parse_param(",%1d()", at_buf, &para))
			{
				return AT_FORWARD;			
			}

			if (para != 0)
			{
				if(para == 1) //电信入库模式下才进行cdp入库指令设置
				{
#if TELECOM_VER
					extern void cdp_storage_nv_init();
					cdp_storage_nv_init();
#endif
				}
				
				g_softap_fac_nv->ra_timeout = 0xFFFF;
				SAVE_FAC_PARAM(ra_timeout);
			}
			else
			{
				g_softap_fac_nv->ra_timeout = 0;
				SAVE_FAC_PARAM(ra_timeout);
			}	
		}
	}
	return AT_FORWARD;
}

//+CGEV: ME PDN DEACT 0
void urc_CGEV_Callback(unsigned long eventId, void *param, int paramLen)
{
	UNUSED_ARG(eventId);
	xy_assert(paramLen == sizeof(ATC_MSG_CGEV_IND_STRU));
	ATC_MSG_CGEV_IND_STRU *cgev_urc = (ATC_MSG_CGEV_IND_STRU*)param;
	
	uint8_t cid = cgev_urc->stCgevPara.ucCid;
	xy_printf(0,XYAPP, WARN_LOG, "cgev urc eventId:%d,cid:%d", cgev_urc->ucCgevEventId, cgev_urc->stCgevPara.ucCid);

	switch(cgev_urc->ucCgevEventId)
	{
		case D_ATC_CGEV_ME_PDN_ACT:
		{
#if (VER_BC95)
			extern int g_npsmr_status;

		    if (g_softap_var_nv->ps_deepsleep_state == 4)       //CFUN1&&CGATT0状态进深睡后,pdp激活时上报npsmr0
            {
                if(g_softap_fac_nv->g_NPSMR_enable == 1 && g_npsmr_status == 1)
                    send_urc_to_ext("+NPSMR:0", strlen("+NPSMR:0"));

                g_npsmr_status = 0;
            }
#endif
			g_OOS_flag = 0;
		}
			break;
		case D_ATC_CGEV_NW_PDN_DEACT:
		case D_ATC_CGEV_ME_PDN_DEACT:
		{
			g_OOS_flag = 0;
			if (g_ipv6_resume_flag == 1)
			{
				/* 深睡唤醒后，如果小区发生变化，协议栈可能会进入OOS状态，进而用户可能会执行CFUN0/1操作
				 * 此时需要将ipv6地址恢复标志位复位，避免执行CFUN1操作后，ipv6地址继续执行深睡恢复流程
				 */
				g_ipv6_resume_flag = 0;
				xy_printf(0, XYAPP, WARN_LOG, "ipv6 resume flag reset");
			}
			if (g_working_cid != INVALID_CID && cid != g_working_cid)
			{
				xy_printf(0, XYAPP, WARN_LOG, "current cid is not working cid and return");
				if (is_netif_active(cid))
					send_msg_2_proxy(PROXY_MSG_PS_PDP_DEACT, &cid, sizeof(cid));
				return;
			}
			g_working_cid = INVALID_CID;
			send_msg_2_proxy(PROXY_MSG_PS_PDP_DEACT, &cid, sizeof(cid));
		}
			break;
		case D_ATC_CGEV_OOS:
		{
			g_OOS_flag = 1;
			psNetifEventInd(EVENT_PSNETIF_ENTER_OOS);
		}
			break;
		case D_ATC_CGEV_IS:
		{
			g_OOS_flag = 0;
			osSemaphoreRelease(g_out_OOS_sem);
			psNetifEventInd(EVENT_PSNETIF_ENTER_IS);
		}
			break;
		default:
			break;
	}
}

/**
 * +XYIPDNS: 
 *<cid_num>,<cid>,<PDP_type>[,<PDP_address>,"",<primary_dns>,<secondary_dns>],
 *[<cid>,<PDP_type>[,<PDP_address>,"",<primary_dns>,<secondary_dns>]]
*/
void urc_XYIPDNS_Callback(unsigned long eventId, void *param, int paramLen)
{
	UNUSED_ARG(eventId);
	xy_assert(param != NULL && paramLen == sizeof(ATC_MSG_XYIPDNS_IND_STRU));
	ATC_MSG_XYIPDNS_IND_STRU *xyipdns_urc = (ATC_MSG_XYIPDNS_IND_STRU*)param;

	PsNetifInfo pdp_info = {0}; 
	pdp_info.ip_type = xyipdns_urc->stPara.ucPdpType;
	g_working_cid = pdp_info.workingCid = xyipdns_urc->stPara.ucCid;

	xy_printf(0, XYAPP, WARN_LOG, "xyipdns urc cid:%d, type:%d", pdp_info.workingCid, pdp_info.ip_type);

	switch (pdp_info.ip_type)
	{
		case D_PDP_TYPE_IPV4:
		{
			memcpy(&pdp_info.ip4, (ip4_addr_t *)xyipdns_urc->stPara.aucIPv4Addr, sizeof(xyipdns_urc->stPara.aucIPv4Addr));

			if (ip4_addr_isany_val(*ip_2_ip4(&pdp_info.ip4)))
			{
				xy_printf(0, XYAPP, WARN_LOG, "ipv4 active but ipv4 addr is any");
				goto error;
			}

			pdp_info.dns[0].type = IPADDR_TYPE_V4;
			pdp_info.dns[1].type = IPADDR_TYPE_V4;
			memcpy(&pdp_info.dns[0].u_addr.ip4, (ip4_addr_t *)xyipdns_urc->stPara.stDnsAddr.ucPriDnsAddr_IPv4, sizeof(xyipdns_urc->stPara.stDnsAddr.ucPriDnsAddr_IPv4));
			memcpy(&pdp_info.dns[1].u_addr.ip4, (ip4_addr_t *)xyipdns_urc->stPara.stDnsAddr.ucSecDnsAddr_IPv4, sizeof(xyipdns_urc->stPara.stDnsAddr.ucSecDnsAddr_IPv4));
		}
		break;
		case D_PDP_TYPE_IPV6:
		{
			memcpy(&pdp_info.ip6_local, (ip6_addr_t*)(xyipdns_urc->stPara.aucIPv6Addr), sizeof(xyipdns_urc->stPara.aucIPv6Addr));

			if (ip6_addr_isany(ip_2_ip6(&pdp_info.ip6_local)))
			{
				xy_printf(0, XYAPP, WARN_LOG, "ipv6 active but ipv4 addr is any");
				goto error;
			}

			pdp_info.dns[2].type = IPADDR_TYPE_V6;
			memcpy(&pdp_info.dns[2].u_addr.ip6, (ip6_addr_t*)(xyipdns_urc->stPara.stDnsAddr.ucPriDnsAddr_IPv6), sizeof(xyipdns_urc->stPara.stDnsAddr.ucPriDnsAddr_IPv6));
			pdp_info.dns[3].type = IPADDR_TYPE_V6;
			memcpy(&pdp_info.dns[3].u_addr.ip6, (ip6_addr_t*)(xyipdns_urc->stPara.stDnsAddr.ucSecDnsAddr_IPv6), sizeof(xyipdns_urc->stPara.stDnsAddr.ucSecDnsAddr_IPv6));
		}
		break;
		case D_PDP_TYPE_IPV4V6:
		{
			memcpy(&pdp_info.ip4, (ip4_addr_t *)(xyipdns_urc->stPara.aucIPv4Addr), sizeof(xyipdns_urc->stPara.aucIPv4Addr));
			memcpy(&pdp_info.ip6_local, (ip6_addr_t *)(xyipdns_urc->stPara.aucIPv6Addr), sizeof(xyipdns_urc->stPara.aucIPv6Addr));

			if (ip4_addr_isany_val(*ip_2_ip4(&pdp_info.ip4)) || ip6_addr_isany(ip_2_ip6(&pdp_info.ip6_local)))
			{
				xy_printf(0, XYAPP, WARN_LOG, "ipv46 active but ipv4 or ipv6 addr is any");
				goto error;
			}
			
			pdp_info.dns[0].type = IPADDR_TYPE_V4;
			memcpy(&pdp_info.dns[0].u_addr.ip4, (ip4_addr_t *)(xyipdns_urc->stPara.stDnsAddr.ucPriDnsAddr_IPv4), sizeof(xyipdns_urc->stPara.stDnsAddr.ucPriDnsAddr_IPv4));
			pdp_info.dns[1].type = IPADDR_TYPE_V4;
			memcpy(&pdp_info.dns[1].u_addr.ip4, (ip4_addr_t *)(xyipdns_urc->stPara.stDnsAddr.ucSecDnsAddr_IPv4), sizeof(xyipdns_urc->stPara.stDnsAddr.ucSecDnsAddr_IPv4));
			pdp_info.dns[2].type = IPADDR_TYPE_V6;
			memcpy(&pdp_info.dns[2].u_addr.ip6, (ip6_addr_t *)(xyipdns_urc->stPara.stDnsAddr.ucPriDnsAddr_IPv6), sizeof(xyipdns_urc->stPara.stDnsAddr.ucPriDnsAddr_IPv6));
			pdp_info.dns[3].type = IPADDR_TYPE_V6;
			memcpy(&pdp_info.dns[3].u_addr.ip6, (ip6_addr_t *)(xyipdns_urc->stPara.stDnsAddr.ucSecDnsAddr_IPv6), sizeof(xyipdns_urc->stPara.stDnsAddr.ucSecDnsAddr_IPv6));
		}
			break;
		default:
			break;
	}

	/*触发物理层进行帧信息上报，以同步快照信息*/
	set_frame_update_flag();
	
	send_msg_2_proxy(PROXY_MSG_PS_PDP_ACT, &pdp_info, sizeof(PsNetifInfo));

error:
	
	return;
}

//^SIMST:<n>
void urc_SIMST_Callback(unsigned long eventId, void *param, int paramLen)
{
	UNUSED_ARG(eventId);
	xy_assert(paramLen == sizeof(ATC_MSG_SIMST_IND_STRU));
	ATC_MSG_SIMST_IND_STRU *simst_urc = (ATC_MSG_SIMST_IND_STRU*)param;

	xy_printf(0,XYAPP, WARN_LOG, "sim urc status:%d", simst_urc->ucSimStatus);
	
	if(simst_urc->ucSimStatus == 0)
	{
		g_Sim_Is_Valid = 0;
		xy_printf(0,XYAPP, WARN_LOG, "no sim card");
	}
	else
	{
		g_Sim_Is_Valid = 1;
	}

	/* 初始化默认dns server */
	send_msg_2_proxy(PROXY_MSG_DNS_INIT, NULL, 0);
}

//+CTZEU:<tz>,<dst>,[<utime>]   +CTZEU:+32,0,2019/03/29,21:12:56
void urc_CTZEU_Callback(unsigned long eventId, void *param, int paramLen)
{
	UNUSED_ARG(eventId);
	xy_assert(paramLen == sizeof(ATC_MSG_LOCALTIMEINFO_IND_STRU));
	ATC_MSG_LOCALTIMEINFO_IND_STRU *ctzeu_urc = (ATC_MSG_LOCALTIMEINFO_IND_STRU*)param;
	int zone_sec = 0;
	xy_wall_clock_t wall_time = {0};
	uint8_t zone = 0;
	uint8_t DayLightTime = 0;

	wall_time.tm_year = 2000 + (ctzeu_urc->stPara.aucUtAndLtz[0] & 0x0F) * 10 + ((ctzeu_urc->stPara.aucUtAndLtz[0] & 0xF0) >> 4);
	wall_time.tm_mon = (ctzeu_urc->stPara.aucUtAndLtz[1] & 0x0F) * 10 + ((ctzeu_urc->stPara.aucUtAndLtz[1] & 0xF0) >> 4);
	wall_time.tm_mday = (ctzeu_urc->stPara.aucUtAndLtz[2] & 0x0F) * 10 + ((ctzeu_urc->stPara.aucUtAndLtz[2] & 0xF0) >> 4);
	wall_time.tm_hour = (ctzeu_urc->stPara.aucUtAndLtz[3] & 0x0F) * 10 + ((ctzeu_urc->stPara.aucUtAndLtz[3] & 0xF0) >> 4);
	wall_time.tm_min = (ctzeu_urc->stPara.aucUtAndLtz[4] & 0x0F) * 10 + ((ctzeu_urc->stPara.aucUtAndLtz[4] & 0xF0) >> 4);
	wall_time.tm_sec = (ctzeu_urc->stPara.aucUtAndLtz[5] & 0x0F) * 10 + ((ctzeu_urc->stPara.aucUtAndLtz[5] & 0xF0) >> 4);

	zone = (ctzeu_urc->stPara.ucLocalTimeZone & 0x07) * 10 + ((ctzeu_urc->stPara.ucLocalTimeZone & 0xF0) >> 4);
	if(ctzeu_urc->stPara.ucNwDayltSavTimFlg == D_ATC_FLAG_TRUE)
	{
		DayLightTime = (ctzeu_urc->stPara.ucNwDayltSavTim & 3) * 4;
	}

	if((ctzeu_urc->stPara.ucLocalTimeZone & 0x08) == 0)
	{
		//zone += DayLightTime;
		g_softap_var_nv->g_zone = zone;
	}
	else
	{
		//zone -= DayLightTime;
		g_softap_var_nv->g_zone = -1 * zone;
	}

	zone_sec = (int)g_softap_var_nv->g_zone * 15 * 60;

	xy_printf(0,XYAPP, WARN_LOG, "+CTZEU:%1d,%1d,%d/%d/%d,%d:%d:%d\r\n",\
		zone,DayLightTime,wall_time.tm_year,wall_time.tm_mon,wall_time.tm_mday,wall_time.tm_hour,wall_time.tm_min,wall_time.tm_sec);
#if VER_BC95
	if(g_softap_fac_nv->g_NITZ == 1)
#endif
	    Set_UT_Time(&wall_time, zone_sec);
}

#if (VER_BC95)
void urc_NPSMR_Callback(unsigned long eventId, void *param, int paramLen)
{
    UNUSED_ARG(eventId);
    xy_assert(paramLen == sizeof(ATC_MSG_MNBIOTEVENT_IND_STRU));
    ATC_MSG_MNBIOTEVENT_IND_STRU *npsmr_urc = (ATC_MSG_MNBIOTEVENT_IND_STRU*)xy_malloc(sizeof(ATC_MSG_MNBIOTEVENT_IND_STRU));
    memcpy(npsmr_urc, param, paramLen);

    xy_printf(0, XYAPP, WARN_LOG, "PS_psm status %d", npsmr_urc->ucPsmState);

    if(npsmr_urc->ucPsmState == 1)  //EXIT PSM
    {
    	extern int g_npsmr_status;
        if(g_softap_fac_nv->g_NPSMR_enable == 1 && g_npsmr_status == 1)
            send_urc_to_ext("+NPSMR:0", strlen("+NPSMR:0"));

        g_npsmr_status = 0;
    }

    xy_free(npsmr_urc);
}
#endif



//AT+XYRAI=<remote IP>,<remote port>,send null packet and rai=1
/*由于该接口未考虑3GPP状态，可能造成无效的建链发空包，不建议用户使用！*/
int at_XYRAI_req(char *at_buf,char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		char remote_ip[20] = {0};
		unsigned short remote_port = 0;
		
		if(!xy_tcpip_v4_is_ok()) 
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
			return AT_END;
		}
		if((at_parse_param("%20s,%2d", at_buf, remote_ip, &remote_port) != AT_OK) || remote_port == 0 || !strcmp(remote_ip, ""))
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
			return AT_END;
		}
		g_null_udp_rai = 1;
		if (!send_null_udp_paket(remote_ip, remote_port))
		{
			g_null_udp_rai = 0;
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
		}
	}
	else if(g_req_type == AT_CMD_ACTIVE)
	{
		/*PS提供的接口，考虑了空闲态下不触发RAI。而之前空包会触发IDLE下重新建链发送RAI，造成无效功耗开销*/
		xy_send_rai();
	}
	else
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	}

	return AT_END;
}

int at_CPINFO_req(char *at_buf,char **prsp_cmd)
{
	if(g_req_type == AT_CMD_ACTIVE)
	{
		char *user_imei = xy_malloc(16);
		char *user_imsi = xy_malloc(16);
		char *user_usim = xy_malloc(24);
		char *user_apn = xy_malloc(100);
		memset(user_imei, 0, 16);
		memset(user_imsi, 0, 16);
		memset(user_usim, 0, 24);
		memset(user_apn, 0, 100);

		if(!xy_get_IMEI(user_imei,16)) 
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			goto END;
		}

		if(!xy_get_IMSI(user_imsi,16)) 
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			goto END;
		}
		
		if(!xy_get_NCCID(user_usim,24)) 
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			goto END;
		}

		if(!xy_get_PDP_APN(user_apn,100,0)) 
		{
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_ALLOWED);
			goto END;
		}

		*prsp_cmd = xy_malloc(256);
		sprintf(*prsp_cmd, "\r\n+CPINFO:%s,%s,%s,%s,%s,%s\r\n\r\nOK\r\n", MODULE_VER_STR, PRODUCT_VER, user_imei, user_imsi, user_usim, user_apn);

END:
		xy_free(user_imei);
		xy_free(user_imsi);
		xy_free(user_usim);
		xy_free(user_apn);
	}
	else
	{
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	}

	return AT_END;
}
int at_ngsearfcn_req(char *at_buf,char **prsp_cmd)
{
	ATC_MSG_NUESTATS_CNF_STRU* pNueStatsCnf;
	unsigned char              aucSelectPlmn[7] = { 0 };

	if(g_req_type == AT_CMD_ACTIVE)
	{
		pNueStatsCnf = (ATC_MSG_NUESTATS_CNF_STRU*)AtcAp_Malloc(sizeof(ATC_MSG_NUESTATS_CNF_STRU));
		if(ATC_AP_TRUE != xy_atc_interface_call("AT+NUESTATS=RADIO\r\n", (func_AppInterfaceCallback)NULL, (void*)pNueStatsCnf))
		{
			xy_free(pNueStatsCnf);
			return ATC_AP_FALSE;
		}
		
    	if((ATC_NUESTATS_TYPE_RADIO == pNueStatsCnf->type) && (0xFFFFFFFF != pNueStatsCnf->stRadio.current_plmn))
    	{
			*prsp_cmd = xy_malloc(256);
			AtcAp_IntegerToPlmn(pNueStatsCnf->stRadio.current_plmn, aucSelectPlmn);
			sprintf(*prsp_cmd, "\r\n+NGSEARFCN:%s,%d\r\n\r\nOK\r\n", 
					aucSelectPlmn,pNueStatsCnf->stRadio.last_earfcn_value);
		} else {
			*prsp_cmd = AT_ERR_BUILD(ATERR_NOT_NET_CONNECT);
		}
	}else if(g_req_type == AT_CMD_TEST) {
		*prsp_cmd = xy_malloc(256);
		sprintf(*prsp_cmd, "\r\nOK\r\n");
	} else {
		*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);
	}
	
	return AT_END;
}

extern bool g_flow_ctl;
void urc_FlowCtl_Cb(unsigned long eventId, void *param, int paramLen)
{
	UNUSED_ARG(eventId);
	xy_assert(paramLen == sizeof(ATC_MSG_IPSN_IND_STRU));

	g_flow_ctl = ((ATC_MSG_IPSN_IND_STRU*)param)->ucFlowCtrlStatus;
}

void ps_urc_register_callback_init()
{
	xy_atc_registerPSEventCallback(D_XY_PS_REG_EVENT_SIMST, urc_SIMST_Callback);
	xy_atc_registerPSEventCallback(D_XY_PS_REG_EVENT_XYIPDNS, urc_XYIPDNS_Callback);
	xy_atc_registerPSEventCallback(D_XY_PS_REG_EVENT_CGEV, urc_CGEV_Callback);
	xy_atc_registerPSEventCallback(D_XY_PS_REG_EVENT_LOCALTIMEINFO, urc_CTZEU_Callback);
	xy_atc_registerPSEventCallback(D_XY_PS_REG_EVENT_IPSN, urc_FlowCtl_Cb);

#if (VER_BC95)
	xy_atc_registerPSEventCallback(D_XY_PS_REG_EVENT_MNBIOTEVENT, urc_NPSMR_Callback);
#endif
}
