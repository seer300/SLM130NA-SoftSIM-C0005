#if AT_SOCKET
#if VER_BC25
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "oss_nv.h"
#include "at_socket.h"
#include "at_com.h"
#include "at_utils.h"
#include "at_tcpip_api.h"
#include "at_socket_context.h"
#include "at_socket_passthr.h"
#include "xy_at_api.h"
#include "xy_net_api.h"
#include "at_socket_api.h"
#include "cloud_proxy.h"



void socket_config_init(void)
{
	g_data_send_mode	 = (g_softap_fac_nv->sock_data_format & 0x01) >> 0;
	g_data_recv_mode	 = (g_softap_fac_nv->sock_data_format & 0x02) >> 1;
	g_recv_data_viewmode = (g_softap_fac_nv->sock_data_format & 0x04) >> 2;
	g_show_length_mode	 = (g_softap_fac_nv->sock_data_format & 0x08) >> 3;

	if(Is_WakeUp_From_Dsleep() == 0)
		g_softap_var_nv->sock_open_time = 36;
}

/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/
int at_QICFG_BC25_req(char *at_buf, char **prsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
        char option[13] = {0};

        if (at_parse_param("%12s", at_buf, option) != XY_OK)
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
			return AT_END;
		}

        // 查询/设置发送/接收数据的格式：AT+QICFG="dataformat"[,<send_data_format>,<recv_data_format>] 
        if (!strcasecmp(option, "dataformat"))
        {
            int send_mode = -1, recv_mode = -1; 

            if (at_parse_param(",%d[0-1],%d[0-1]", at_buf, &send_mode, &recv_mode) != XY_OK)
			{
				*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
				return AT_END;
			}

			// sendmode和recvmode都不输入表示查询
			if(send_mode == -1 && recv_mode == -1)
			{
				*prsp_cmd = xy_malloc(64);
				snprintf(*prsp_cmd, 64, "\r\n+QICFG: \"dataformat\",%d,%d\r\n\r\nOK\r\n", g_data_send_mode, g_data_recv_mode);
				set_at_tcpip_err(TCPIP_OP_OK);
				return AT_END;
			}

			if (send_mode != -1)
			{
				g_data_send_mode = send_mode;
				// send_data_format暂只支持文本字符串格式
				g_softap_fac_nv->sock_data_format = (send_mode << 0) | (g_softap_fac_nv->sock_data_format & 0xFE);
			}
			if (recv_mode != -1)
			{
				g_data_recv_mode = recv_mode;
				g_softap_fac_nv->sock_data_format = (recv_mode << 1) | (g_softap_fac_nv->sock_data_format & 0xFD);
			}
			SAVE_FAC_PARAM(sock_data_format);
		}
        // 查询/设置接收数据的输出格式：AT+QICFG="viewmode"[,<view_mode>]
        else if (!strcasecmp(option, "viewmode"))
        {
            int view_mode = -1;

            if (at_parse_param(",%d[0-1]", at_buf, &view_mode) != XY_OK)
			{
				*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
				return AT_END;
			}
			// viewmode不输入表示查询
            if (view_mode != -1)
            {
                //Todo:保存nv
                g_recv_data_viewmode = view_mode;
                g_softap_fac_nv->sock_data_format = (view_mode << 2) | (g_softap_fac_nv->sock_data_format & 0xFB);
                SAVE_FAC_PARAM(sock_data_format);
            }
			else
			{
				*prsp_cmd = xy_malloc(64);
				snprintf(*prsp_cmd, 64, "\r\n+QICFG: \"viewmode\",%d\r\n\r\nOK\r\n", g_recv_data_viewmode);
			}
		}
        // 查询/设置缓存模式下是否显示可选长度参数：AT+QICFG="showlength"[,<show_length_mode>]
        else if (!strcasecmp(option, "showlength"))
        {
            int show_length_mode = -1;

            if (at_parse_param(",%d[0-1]", at_buf, &show_length_mode) != XY_OK)
			{
				*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
				return AT_END;
			}

			// showlength不输入表示查询
            if (show_length_mode != -1)
            {
                //Todo:保存nv
                g_show_length_mode = show_length_mode;
                g_softap_fac_nv->sock_data_format = (show_length_mode << 3) | (g_softap_fac_nv->sock_data_format & 0xF7);
                SAVE_FAC_PARAM(sock_data_format);
            }
			else
			{
				*prsp_cmd = xy_malloc(64);
				snprintf(*prsp_cmd, 64, "\r\n+QICFG: \"showlength\",%d\r\n\r\nOK\r\n", g_show_length_mode);
			}
		}
        // 查询/设置 TCP 连接的超时时间AT+QICFG="open_time"[,<open_time>]
        else if (!strcasecmp(option, "open_time"))
        {
            int open_time = -1;

            if (at_parse_param(",%d[1-36]", at_buf, &open_time) != XY_OK)
			{
				*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
				return AT_END;
			}

            if (open_time == -1)
            {
                /* open_time不输入表示查询 */
                *prsp_cmd = xy_malloc(64);
                snprintf(*prsp_cmd, 64, "\r\n+QICFG: \"open_time\",%d\r\n\r\nOK\r\n", g_softap_var_nv->sock_open_time);
            }
            else
            {
                //Todo:不保存nv
                g_softap_var_nv->sock_open_time = open_time;
            }
        }
        else
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
			return AT_END;
		}
    }
    else if (g_req_type == AT_CMD_TEST)
    {
		int n = 0;

        *prsp_cmd = xy_malloc(128);
        n += sprintf(*prsp_cmd + n, "\r\n+QICFG: \"dataformat\",(0,1),(0,1)");
		n += sprintf(*prsp_cmd + n, "\r\n+QICFG: \"viewmode\",(0,1)");
		n += sprintf(*prsp_cmd + n, "\r\n+QICFG: \"showlength\",(0,1)");
		n += sprintf(*prsp_cmd + n, "\r\n+QICFG: \"open_time\",(1,36)");
		n += sprintf(*prsp_cmd + n, "\r\nOK\r\n");
    }
    else
	{
		*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
		return AT_END;
	}  

	set_at_tcpip_err(TCPIP_OP_OK);
    return AT_END;
}

// AT+QIOPEN=<contextID>,<connectID>,<service_type>,"<IP_address>/<domain_name>",<remote_port>[,<local_port>[,<access_mode>[,<protocol_type>]]]
int at_QIOPEN_BC25_req(char *at_buf, char **prsp_cmd)
{
	socket_resume();
    if (g_req_type == AT_CMD_REQ)
    {
        char service_type[13] = {0};
        socket_create_param_t open_para = {0};
        
        if (at_parse_param("%1d(1-3),%1d(0-5),%13s(),%51p(),%2d(1-65535),%2d[0-65535],%1d[0-1],%1d[0-1]", at_buf, &open_para.cid, &open_para.id, service_type, &open_para.remote_ip, &open_para.remote_port, &open_para.local_port, &open_para.access_mode, &open_para.af_type) != XY_OK)
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
			return AT_END;
		}  

        if (!strcasecmp(service_type, "TCP"))
            open_para.service_type = SOCKET_TCP;
        else if (!strcasecmp(service_type, "UDP"))
            open_para.service_type = SOCKET_UDP;
        else if (!strcasecmp(service_type, "TCP LISTENER"))
            open_para.service_type = SOCKET_TCP_LISTENER;
        else
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
			return AT_END;
		} 
		
		// 做服务端时，必须指定本地端口
		if (open_para.service_type == SOCKET_TCP_LISTENER && open_para.local_port == 0)
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_NotAllowed);
			return AT_END;
		} 			
		int ip_type = xy_get_IpAddr_type(open_para.remote_ip);
		if ((ip_type == IPV4_TYPE && open_para.af_type) || (ip_type == IPV6_TYPE && !open_para.af_type))
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
			return AT_END;
		} 

		if (!xy_tcpip_is_ok())
		{	
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_PdpOpen);
			return AT_END;
		}		
        // 对应socket id已在使用中
        if (is_socketId_valid(open_para.id))
		{		
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_SockInuse);
			return AT_END;
		}
        socket_open_async(&open_para);
    }
    else if (g_req_type == AT_CMD_TEST)
    {
        *prsp_cmd = xy_malloc(128);
        snprintf(*prsp_cmd, 128, "\r\n+QIOPEN: (1-3),(0-5),\"TCP/UDP/TCP LISTENER\",\"<IP_address>/<domain_name>\",<remote_port>[,<local_port>[,(0-1)[,(0-1)]]]\r\n\r\nOK\r\n");
    }
    else
	{
		*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
		return AT_END;
	} 

	set_at_tcpip_err(TCPIP_OP_OK);
    return AT_END;
}

// AT+QICLOSE=<connectID>
int at_QICLOSE_BC25_req(char *at_buf, char **prsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
        int sock_id;

        if (at_parse_param("%d(0-5)", at_buf, &sock_id) != XY_OK)
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
			return AT_END;
		}  

        /* 对应socket id已在使用中 */
        if (socket_close(sock_id) != XY_OK)
		{	
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_SockClosed);
			return AT_END;
		}	
    }
    else if (g_req_type == AT_CMD_TEST)
    {
        *prsp_cmd = xy_malloc(32);
        sniprintf(*prsp_cmd, 32, "\r\n+QICLOSE: (0-5)\r\n\r\nOK\r\n");
    }
    else
    {
		*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
		return AT_END;
    }

	set_at_tcpip_err(TCPIP_OP_OK);	
    return AT_END;
}

//若<query_type>=0,AT+QISTATE=<query_type>,<contextID>
//若<query_type>=1,AT+QISTATE=<query_type>,<connectID>
int at_QISTATE_BC25_req(char *at_buf, char **prsp_cmd)
{
    int str_len = 0;
	char *rsp = NULL;
	socket_resume();
    if (g_req_type == AT_CMD_REQ)
    {
        int query_type = 0, id = 0;

        if (at_parse_param("%d(0-1),%d(0-5)", at_buf, &query_type, &id) != XY_OK)
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
			return AT_END;
		}
		// 若<query_type>为0，AT+QISTATE=<query_type>,<contextID> 查询指定PDP场景下的连接状态
		if (query_type == 0)
        {
            rsp = xy_malloc(640);
            str_len = socket_get_status_info_by_cid(id, rsp);
        }
        // 若<query_type>为1，AT+QISTATE=<query_type>,<connectID> 查询指定Socket服务连接状态
        else
        {
            rsp = xy_malloc(96);
            str_len = socket_get_status_info(id, rsp);
        }
		if (str_len > 0)
		{
			str_len += sprintf(rsp + str_len, "\r\n");
			send_urc_to_ext_NoCache(rsp, str_len);
		}
		xy_free(rsp);
    }
	// [+QISTATE: <connectID>,<service_type>,<host>,<remote_port>,<local_port>,<socket_state>,<contextID>,<access_mode>]
    else if (g_req_type == AT_CMD_QUERY)
    {
		rsp = xy_malloc(640);
		str_len = socket_get_all_status_info(rsp);
		if (str_len > 0)
		{
			str_len += sprintf(rsp + str_len, "\r\n");
			send_urc_to_ext_NoCache(rsp, str_len);
		}
		xy_free(rsp);
    }
    else if (g_req_type == AT_CMD_TEST)
    {

    }	
    else
	{
		*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
		return AT_END;
	}	

	set_at_tcpip_err(TCPIP_OP_OK);	
    return AT_END;
}

// 非数据模式下发送定长数据 AT+QISEND=<connectID>,<send_length>,"<data>"
// 数据模式下发送不定长数据 AT+QISEND=<connectID>
// 数据模式下发送定长数据   AT+QISEND=<connectID>,<send_length>
// 查询已发送、已应答，以及已发送但未应 答数据的总长度 AT+QISEND=<connectID>,0
int at_QISEND_BC25_req(char *at_buf, char **prsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
		socket_send_param_t param = {0}; 
		param.udp_connectd = 1;
		param.data_len = -1;
		socket_resume();
        if (at_parse_param("%1d(0-5),%d[0-1441],%p,%1d[0-2]", at_buf, &param.id, &param.data_len, &param.data, &param.rai_type) != AT_OK)
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
			return AT_END;
		}
        // 对应socket id未在使用中
        if (!is_socketId_valid(param.id))
		{		
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_NotAllowed);
			return AT_END;
		}
        if (param.data_len)
        {
			int ret = XY_OK;

			if (param.data != NULL)
			{
				// tcp listener不支持发送data
				if (g_socket_ctx[param.id]->service_type == SOCKET_TCP_LISTENER)
				{
					*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_NotAllowed);
					return AT_END;
				}
				param.data_str_len = strlen(param.data);
				if (param.data_str_len != param.data_len)
				{
					*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
					return AT_END;
				}				
				ret = socket_send(&param);
				if (ret == XY_OK)
				{	
					set_at_tcpip_err(TCPIP_OP_OK);
					*prsp_cmd = xy_malloc(32);
					sprintf(*prsp_cmd, "\r\nOK\r\n\r\nSEND OK\r\n");	
				}
				else
					*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_SockWrite);

				return AT_END;
			}
			param.data_echo = is_echo_mode();
			param.passthr_data_mode = ASCII_STRING;
			ret = socket_enter_passthr_mode(&param);
			if (ret == XY_OK)
			{
				set_at_tcpip_err(TCPIP_OP_OK);
				return AT_ASYN;
			}
			else
			{
				*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_NotAllowed);
				return AT_END;
			}				
        }
		else
		{
			socket_context_t *ctx = g_socket_ctx[param.id];
			if (ctx->sended_size)
			{
				*prsp_cmd = xy_malloc(128);
				sniprintf(*prsp_cmd, 128, "\r\n+QISEND: %d,%d,%d\r\n\r\nOK\r\n", ctx->sended_size, ctx->acked_size, ctx->sended_size - ctx->acked_size);
			}
			else
			{
				*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_NotAllowed);
				return AT_END;				
			}	
		}
    }
    else if (g_req_type == AT_CMD_TEST)
    {
        *prsp_cmd = xy_malloc(64);
        snprintf(*prsp_cmd, 64, "\r\n+QISEND: (0-5),(0-1440),\"<data>\",(0-2)\r\n\r\nOK\r\n");
    }
    else
	{
		*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
		return AT_END;
	}

	set_at_tcpip_err(TCPIP_OP_OK);	
    return AT_END;
}

/* AT+QIRD=<connectID>,<read_length> */
int at_QIRD_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		socket_readbuffer_param_t param = {0};

		if (at_parse_param("%d(0-5),%d(1-1024)", at_buf, &param.id, &param.want_len) != XY_OK)
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
			return AT_END;
		}
        // 对应socket id未在使用中
        if (!is_socketId_valid(param.id))
		{		
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_SockClosed);
			return AT_END;
		}
        // tcp listener不支持使用该AT指令读取
		if (g_socket_ctx[param.id]->service_type == SOCKET_TCP_LISTENER)
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_NotAllowed);
			return AT_END;
		}

		// 读取数据正常的情况下，此长度应该等于实际读取出来的数据长度
        if (socket_read_buffer(&param) == XY_OK)
		{	
            int str_len = 0; 
			char *report = NULL;

			if (g_data_recv_mode == HEX_ASCII_STRING)
            	report = xy_malloc2(128 + (param.want_len << 1)); 
			else
				report = xy_malloc2(128 + param.want_len); 
			if (report == NULL)
			{
				xy_free(param.data);
				xy_free(param.remote_ip);
				*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_NoMemory);
				return AT_END;				
			}
			// 返回：+QIRD: <read_actual_length>[,<remaining_length>][<CR><LF>/,]<data>	
            if (g_show_length_mode == 0)	
			    str_len += sprintf(report, "\r\n+QIRD: %d", param.read_len);
            else
                str_len += sprintf(report, "\r\n+QIRD: %d,%d", param.read_len, param.remaining_len);
			// 接收数据的输出方式设置为1时，输出数据内容“<CR><LF><data>”，否则为“,<data>”
			if (g_recv_data_viewmode == 0)
				str_len += sprintf(report + str_len, "\r\n");
			else
				str_len += sprintf(report + str_len, ",");
			if (g_data_recv_mode == HEX_ASCII_STRING)
			{
				memcpy(report + str_len, param.data, param.read_len << 1);
				str_len += (param.read_len << 1);
			}
			else
			{
				memcpy(report + str_len, param.data, param.read_len);
				str_len += param.read_len;
			}
			str_len += sprintf(report + str_len, "\r\n");
			send_urc_to_ext_NoCache(report, str_len);
			xy_free(report);
			xy_free(param.data);
			xy_free(param.remote_ip);				
            xy_printf(0, XYAPP, INFO_LOG, "[%s]read socket len:%d", __FUNCTION__, str_len);
		}
		else
		{
			*prsp_cmd = xy_malloc(32);
			snprintf(*prsp_cmd, 32, "\r\n+QIRD: 0\r\n\r\nOK\r\n");
		}	
	}
	else if (g_req_type == AT_CMD_TEST)
	{	
		//  +QIRD: (支持的<connectID>范围),(支持的<read_data_len>范围)
		*prsp_cmd = xy_malloc(64);
		snprintf(*prsp_cmd, 64, "\r\n+QIRD: (0-5),(1-1024)\r\n\r\nOK\r\n");
	}
	else
	{
		*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
		return AT_END;
	}

	set_at_tcpip_err(TCPIP_OP_OK);
	return AT_END;
}

// AT+QISENDEX=<connectID>[,<send_length>,"<hex_string>"]  TCP LISTENER不支持此AT命令
int at_QISENDEX_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		socket_send_param_t param = {0};
		param.udp_connectd = 1;
		param.data_len = -1;

		socket_resume();
		if (at_parse_param("%1d(0-5),%d[1-1440],%p[],%1d[0-2]", at_buf, &param.id, &param.data_len, &param.data, &param.rai_type) != XY_OK)
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
			return AT_END;
		}

        // 对应socket id未在使用中
        if (!is_socketId_valid(param.id))
		{		
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_SockClosed);
			return AT_END;
		}
        // tcp listener不支持使用该AT指令
		if (g_socket_ctx[param.id]->service_type == SOCKET_TCP_LISTENER)
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_NotAllowed);
			return AT_END;
		}

        if (param.data != NULL)
        {
			param.data_str_len = strlen(param.data);
			if (param.data_len != (param.data_str_len >> 1))
			{
				*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
				return AT_END;				
			}
			
			g_data_send_mode = HEX_ASCII_STRING;
			int ret = socket_send(&param);
			if (ret != XY_OK)
				*prsp_cmd = AT_TCPIP_ERR((ret == XY_Err_Parameter) ? TCPIP_Err_Parameter : TCPIP_Err_SockWrite);
			else
			{	
				set_at_tcpip_err(TCPIP_OP_OK);
				*prsp_cmd = xy_malloc(32);
				sprintf(*prsp_cmd, "\r\nOK\r\n\r\nSEND OK\r\n");	
			}
			g_data_send_mode = ASCII_STRING;
			return AT_END;	
        }
        else
        {
			param.data_echo = is_echo_mode();			
			param.passthr_data_mode = HEX_ASCII_STRING;
			int ret = socket_enter_passthr_mode(&param);
			if (ret == XY_OK)
			{
				set_at_tcpip_err(TCPIP_OP_OK);
				return AT_ASYN;
			}
			else
			{
				*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_NotAllowed);
				return AT_END;
			}        
        }
	}
	else if (g_req_type == AT_CMD_TEST)
	{	
		*prsp_cmd = xy_malloc(64);
		snprintf(*prsp_cmd, 64, "\r\n+QISENDEX: (0-5),(1-1440),\"<hex_string>\",(0-2)\r\n\r\nOK\r\n");
	}
	else
		*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);

	set_at_tcpip_err(TCPIP_OP_OK);
	return AT_END;
}

// AT+QISWTMD=<connectID>,<access_mode>
int at_QISWTMD_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		int sock_id = -1, mode = -1;
		
		if (at_parse_param("%d(0-5),%d(0-1)", at_buf, &sock_id, &mode) != XY_OK)
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
			return AT_END;
		}
        // 对应socket id未在使用中
        if (!is_socketId_valid(sock_id))
		{		
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_SockClosed);
			return AT_END;
		}
		g_socket_ctx[sock_id]->access_mode = mode;
	}
	else if (g_req_type == AT_CMD_TEST)
	{	
		// +QISWTMD: (支持的<connectID>范围),(支持的<access_mode>范围)
		*prsp_cmd = xy_malloc(64);
		snprintf(*prsp_cmd, 64, "\r\n+QISWTMD: (0-5),(0-1)\r\n\r\nOK\r\n");
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		
	}
	else
		*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
		
	set_at_tcpip_err(TCPIP_OP_OK);
	return AT_END;	
}

int at_SOCKCLZ_BC25_URC(int id, bool isquit)
{
	if (isquit)
		send_urc_to_ext("\r\nCLOSE OK\r\n", strlen("\r\nCLOSE OK\r\n"));
	else
	{
		int state = socket_get_state(id);
		if (state == SOCKET_STATE_CONNECTED)
		{
			char rsp[32];
			snprintf(rsp, 32, "\r\n+QIURC: \"closed\",%d\r\n", id);
			send_urc_to_ext(rsp, strlen(rsp));
		}
	}
   
    return XY_OK;
}

int at_SOCKNMI_BC25_URC(int sock_id, uint32_t len, char *data, void *remoteinfo)
{
	char urc_str[64];
    int access_mode = g_socket_ctx[sock_id]->access_mode;

	xy_printf(0, PLATFORM_AP, INFO_LOG, "[%s-%d]recv:%d,%d,%d\n", __FUNCTION__, __LINE__, access_mode, sock_id, len);
	if (access_mode == SOCKET_BUFFER_MODE)
	{
	    if (add_new_rcv_data_node(sock_id, len, data, (struct sockaddr_storage *)remoteinfo) == XY_OK)
		{
			// 如果在透传发送模式则退出
			if (g_socket_passthr_info != NULL)
				return XY_OK;

            if (g_socket_ctx[sock_id]->data_list->next == NULL)
            {
				if (g_show_length_mode)
					snprintf(urc_str, 64, "\r\n+QIURC: \"recv\",%d,%d\r\n", sock_id, len);				
				else
					snprintf(urc_str, 64, "\r\n+QIURC: \"recv\",%d\r\n", sock_id);				
				send_urc_to_ext(urc_str, strlen(urc_str));	
            }
	    }
		else
		{
			snprintf(urc_str, 64, "\r\n+QIURC: \"recv\",%d,\"buff full\"\r\n", sock_id);							
			send_urc_to_ext(urc_str, strlen(urc_str));			
		}		
	}
	else if (access_mode == SOCKET_DIRECT_SPIT_MODE)
	{		
		int str_len = 0;
		char *report_buf = NULL;
		int service_type = g_socket_ctx[sock_id]->service_type;

		// 需要输出16进制格式的数据内容时，预先申请2倍的数据长度的内存空间，用来存放十六进制格式的字符串
		if (g_data_recv_mode == HEX_ASCII_STRING)
			report_buf = xy_malloc2(128 + (len << 1));
		else
			report_buf = xy_malloc2(128 + len);

		if (report_buf == NULL)
			return XY_Err_NoMemory;

		if (g_show_length_mode == 0)
			str_len = sprintf(report_buf, "\r\n+QIURC: \"recv\",%d", sock_id);
		else
			str_len = sprintf(report_buf, "\r\n+QIURC: \"recv\",%d,%d", sock_id, len);

		// view_mode为0时，格式为：data header\r\ndata；为1时，格式为：data header,data
		if (g_recv_data_viewmode == 0)
			str_len += sprintf(report_buf + str_len, "\r\n");
		else
			str_len += sprintf(report_buf + str_len, ",");

		// 接收数据格式为16进制模式
		if (g_data_recv_mode == HEX_ASCII_STRING)
		{
			if (bytes2hexstr(data, len, report_buf + str_len, ((len << 1) + 1)) == -1)
			{
				xy_free(report_buf);
				return XY_ERR;
			}
			str_len = str_len + (len << 1);
		}
		else
		{
			memcpy(report_buf + str_len, data, len);
			str_len = str_len + len;
		}

		str_len += sprintf(report_buf + str_len, "\r\n");

		send_urc_to_ext(report_buf, str_len); 
		xy_free(report_buf);
	}
	else
		xy_assert(0);
}

int at_SOCKDATAMODE_EXIT_BC25_URC(void *arg)
{
	UNUSED_ARG(arg);
	if (g_socket_passthr_info->result == XY_OK)
	{
		send_rsp_at_to_ext("\r\nOK\r\n\r\nSEND OK\r\n");
	}
	else
	{
		send_rsp_at_to_ext("\r\nSEND FAIL\r\n");
	}

    return XY_OK;
}

//+QIURC: "incoming",<connectID>,<serverID>,<remoteIP>,<remote_port>
int at_SOCKINCOMING_URC(int sock_id, uint8_t server_id, char *ip, uint16_t port)
{
	char report_buf[64] = {0};
    snprintf(report_buf, 64, "\r\n+QIURC: \"incoming\",%d,%d,\"%s\",%d\r\n", sock_id, server_id, ip, port);
    send_urc_to_ext(report_buf, strlen(report_buf));

    return XY_OK;
}

//+QIURC: "incoming full"
int at_SOCKINCOMEFULL_URC(void)
{
	char report_buf[32] = {0};
    snprintf(report_buf, 32, "\r\n+QIURC: \"incoming full\"\r\n");
    send_urc_to_ext(report_buf, strlen(report_buf));

    return XY_OK;
}

#endif /* VER_BC25 */
#endif /* AT_SOCKET */