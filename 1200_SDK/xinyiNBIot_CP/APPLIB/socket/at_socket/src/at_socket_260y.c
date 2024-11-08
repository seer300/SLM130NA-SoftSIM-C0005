#if AT_SOCKET
#if VER_260Y
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
	g_data_send_mode = (g_softap_fac_nv->sock_data_format & 0x01) >> 0;
	g_data_recv_mode = (g_softap_fac_nv->sock_data_format & 0x02) >> 1;
}

/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/
int at_QICFG_260Y_req(char *at_buf, char **prsp_cmd)
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
				SAVE_FAC_PARAM(sock_data_format);
            }
            if (recv_mode != -1)
            {
                g_data_recv_mode = recv_mode;
                g_softap_fac_nv->sock_data_format = (recv_mode << 1) | (g_softap_fac_nv->sock_data_format & 0xFD);
				//Todo:保存nv
            	SAVE_FAC_PARAM(sock_data_format);
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
        n += sprintf(*prsp_cmd + n, "\r\n+QICFG: \"dataformat\",(0,1),(0,1)\r\n");
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

// AT+QIOPEN=<contextID>,<connectID>,<service_type>,"<host>",<remote_port>[,<local_port>[,<access_mode>]]
int at_QIOPEN_260Y_req(char *at_buf, char **prsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
        char service_type[4] = {0};
        socket_create_param_t open_para = {0};
        open_para.access_mode = 1;
        
        if (at_parse_param("%1d(0-0),%1d(0-4),%4s(),%51p(),%2d(1-65535),%2d[0-65535],%1d[1-1]", at_buf, &open_para.cid, &open_para.id, service_type, &open_para.remote_ip, &open_para.remote_port, &open_para.local_port, &open_para.access_mode) != XY_OK)
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
			return AT_END;
		}

        if (!strcasecmp(service_type, "TCP"))
            open_para.service_type = SOCKET_TCP;
        else if (!strcasecmp(service_type, "UDP"))
            open_para.service_type = SOCKET_UDP;
        else
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
        snprintf(*prsp_cmd, 128, "\r\n+QIOPEN: (0-10),(0-4),\"TCP/UDP\",<host>,(1-65535),(0-65535),(0,1)\r\n\r\nOK\r\n");
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
int at_QICLOSE_260Y_req(char *at_buf, char **prsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
        int sock_id;

        if (at_parse_param("%d(0-4)", at_buf, &sock_id) != XY_OK)
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
        sniprintf(*prsp_cmd, 32, "\r\n+QICLOSE: (0-4)\r\n\r\nOK\r\n");
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
int at_QISTATE_260Y_req(char *at_buf, char **prsp_cmd)
{
    int str_len = 0;
	char *rsp = NULL;
	socket_resume();
    if (g_req_type == AT_CMD_REQ)
    {
        int query_type = 0, id = 0;

        if (at_parse_param("%d(0-1),%d(0-4)", at_buf, &query_type, &id) != XY_OK)
		{
			*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
			return AT_END;
		}
		// 若<query_type>为0，AT+QISTATE=<query_type>,<contextID> 查询指定PDP场景下的连接状态
		if (query_type == 0)
        {
			if(id == 0)
			{
				rsp = xy_malloc(640);
				str_len = socket_get_status_info_by_cid(id, rsp);
			}
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
		if(rsp != NULL)
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
        *prsp_cmd = xy_malloc(64);
        sniprintf(*prsp_cmd, 64, "\r\n+QISTATE: 0,(0-10)\r\n+QISTATE: 1,(0-4)\r\n\r\nOK\r\n");
    }	
    else
	{
		*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
		return AT_END;
	}	

	set_at_tcpip_err(TCPIP_OP_OK);	
    return AT_END;
}

// 非数据模式下发送定长数据 AT+QISEND=<connectID>,<send_length>,"<data>"[,<rai_mode>]
// 数据模式下发送不定长数据 AT+QISEND=<connectID>
// 数据模式下发送定长数据   AT+QISEND=<connectID>,<send_length>
// 查询已发送、已应答，以及已发送但未应 答数据的总长度 AT+QISEND=<connectID>,0
int at_QISEND_260Y_req(char *at_buf, char **prsp_cmd)
{
	socket_resume();
    if (g_req_type == AT_CMD_REQ)
    {
		socket_send_param_t param = {0}; 
		param.udp_connectd = 1;
		param.data_len = -1;
        if (at_parse_param("%1d(0-4),%d[0-1024],%p,%1d[0-2]", at_buf, &param.id, &param.data_len, &param.data, &param.rai_type) != AT_OK)
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
				param.data_str_len = strlen(param.data);
				ret = socket_send(&param);
				if (ret == XY_OK)
				{	
					set_at_tcpip_err(TCPIP_OP_OK);
					*prsp_cmd = xy_malloc(32);
					sprintf(*prsp_cmd, "\r\nOK\r\n\r\nSEND OK\r\n");
				}
				else
					*prsp_cmd = AT_TCPIP_ERR((ret == XY_Err_Parameter) ? TCPIP_Err_Parameter: TCPIP_Err_SockWrite);

				return AT_END;
			}
            param.udp_connectd = 1;
			param.passthr_data_mode = g_data_send_mode;
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
				*prsp_cmd = xy_malloc(32);
				sniprintf(*prsp_cmd, 32, "\r\n+QISEND: 0,0,0\r\n\r\nOK\r\n");
				return AT_END;
			}
		}
    }
    else if (g_req_type == AT_CMD_TEST)
    {
        *prsp_cmd = xy_malloc(64);
        snprintf(*prsp_cmd, 64, "\r\n+QISEND: (0-4),(0-1024),<data>,(0-2)\r\n\r\nOK\r\n");
    }
    else
	{
		*prsp_cmd = AT_TCPIP_ERR(TCPIP_Err_Parameter);
		return AT_END;
	}

	set_at_tcpip_err(TCPIP_OP_OK);	
    return AT_END;
}

int at_SOCKCLZ_260Y_URC(int id, bool isquit)
{
    int state = socket_get_state(id);

	if (isquit)
		send_urc_to_ext("\r\nCLOSE OK\r\n", strlen("\r\nCLOSE OK\r\n"));
	else if (state == SOCKET_STATE_CONNECTED)
	{
		char rsp[32];
		snprintf(rsp, 32, "\r\n+QIURC: \"closed\",%d\r\n", id);
		send_urc_to_ext(rsp, strlen(rsp));
	}
   
    return XY_OK;
}

int at_SOCKNMI_260Y_URC(int sock_id, uint32_t len, char *data, void *remoteinfo)
{
	char urc_str[64];
    int access_mode = g_socket_ctx[sock_id]->access_mode;

	xy_printf(0, PLATFORM, INFO_LOG, "[%s-%d]recv:%d,%d,%d\n", __FUNCTION__, __LINE__, access_mode, sock_id, len);
    if (access_mode == SOCKET_DIRECT_SPIT_MODE)
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

		str_len = sprintf(report_buf, "\r\n+QIURC: \"recv\",%d,%d,\"", sock_id, len);
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

		str_len += sprintf(report_buf + str_len, "\"\r\n");

		send_urc_to_ext(report_buf, str_len); 
		xy_free(report_buf);
	}
	else
		xy_assert(0);
}

int at_SOCKDATAMODE_EXIT_260Y_URC(void *arg)
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
#endif
#endif