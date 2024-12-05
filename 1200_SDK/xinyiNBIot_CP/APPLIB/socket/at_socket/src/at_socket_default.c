/**
 * @file at_socket_default.c
 * @version 1.0
 * @date 2022-06-22
 * @copyright Copyright (c) 2022  芯翼信息科技有限公司
 * 
 */
#include "at_socket.h"
#include "at_socket_context.h"
#include "attribute.h"
#include "lwip/sockets.h"
#include "at_socket_api.h"
#include "xy_at_api.h"
#include "xy_utils.h"
#include "at_socket_passthr.h"
#include "xy_passthrough.h"

/* AT+NSOCR=<type>,<protocol>,<listenport>[,<receive control>,<af type>,<ip addr>] */
int at_SOCKCFG_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		char type[7] = {0};
		char af_type[10] = {0};
		char ipaddr[XY_IPADDR_STRLEN_MAX] = {0};
		int sockid = -1;
		socket_create_param_t create_param = {0};
		create_param.recv_ctrl = 1;

		if (at_parse_param("%7s,%d,%2d(0-65535),%1d[0-1],%10s,%47s", 
			at_buf, type, &create_param.proto, &create_param.local_port, &create_param.recv_ctrl, af_type, ipaddr) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		if (at_strcasecmp(af_type, "AF_INET6"))
		{
			create_param.af_type = AF_INET6;
		}
		else if (at_strcasecmp(af_type, "AF_INET") || strlen(af_type) == 0)
		{
			create_param.af_type = AF_INET;
		}
		else
		{
			return ATERR_PARAM_INVALID;
		}

		if ((create_param.proto != IPPROTO_UDP || !at_strcasecmp(type, "DGRAM")) &&
			(create_param.proto != IPPROTO_TCP || !at_strcasecmp(type, "STREAM")))
		{
			//check protocol and type param
			return ATERR_PARAM_INVALID;
		}

		if ((sockid = socket_create(&create_param)) == -1)
		{
			return ATERR_PARAM_INVALID;
		}

		*prsp_cmd = xy_malloc(24);
		snprintf(*prsp_cmd, 24, "+NSOCR:%d", sockid);
		return AT_END;
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

/* AT+NSOCO=<socket>,<remote_addr>,<remote_port> */
/* only for tcp */
int at_SOCKCONN_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		char remote_ip[XY_IPADDR_STRLEN_MAX] = {0};
		socket_conn_param_t param = {0};

		if (at_parse_param("%d(0-),%40s,%2d(1-65535)", at_buf, &param.id, remote_ip, &param.remote_port) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		if (!strcmp(remote_ip, ""))
		{
			return ATERR_PARAM_INVALID;
		}

		if (get_socket_net_type(param.id) == 1)  //udp
		{
			return ATERR_NOT_ALLOWED;
		}

		param.remote_ip = remote_ip;

		if (socket_connect(&param) != XY_OK)
		{
			return ATERR_NOT_NET_CONNECT;
		}
		
		return AT_END;	
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

/* AT+NSOSD=<socket>,<length>,<data>[,<flag>[,<sequence>]] */
/* only for tcp*/
int at_SOCKSEND_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		int rai_flag = 0;
		socket_send_param_t param = {0};
		
		if(is_Uplink_FlowCtl_Open())
			return ATERR_NOT_ALLOWED;

		if (at_parse_param("%1d(0-),%l,%p,%d[0|256|512|1024],%1d[0-255]", at_buf, &param.id, &param.data_len, &param.data, &rai_flag, &param.sequence) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		if(param.data != NULL)
			param.data_str_len = strlen(param.data);

		if (get_socket_net_type(param.id) != 0)
		{
			//not tcp, err
			return ATERR_NOT_ALLOWED;
		}

		// 标志转换 0x100->0, 0x200->1, 0x400->2, 未设置时置为0
		param.rai_type = (int8_t)((rai_flag == 0) ? RAI_NULL : ((rai_flag >> 9) & 0xFF));

		if (socket_send(&param) != XY_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		*prsp_cmd = xy_malloc(32);
		snprintf(*prsp_cmd, 32, "\r\n%d,%d", param.id, param.data_len);
		
		return AT_END;
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

/* AT+NSOSDEX=<socket>,<flag>,<sequece>,<length>,<data> */
/* only for tcp*/
int at_SOCKSENDEX_req(char *at_buf, char **prsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
	{
		int rai_flag = 0;
		socket_send_param_t param = {0};
		
		if(is_Uplink_FlowCtl_Open())
			return ATERR_NOT_ALLOWED;

		if (at_parse_param("%1d(0-),%d(0|256|512|1024),%1d(0-255),%l,%p",
			at_buf, &param.id, &rai_flag, &param.sequence, &param.data_len, &param.data) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		if(param.data != NULL)
			param.data_str_len = strlen(param.data);

        if (get_socket_net_type(param.id) != 0)
		{
			//not tcp, err
			return ATERR_NOT_ALLOWED;
		}

		param.rai_type = (int8_t)((rai_flag == 0) ? RAI_NULL : ((rai_flag >> 9) & 0xFF));

        if (socket_send(&param) != XY_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		*prsp_cmd = xy_malloc(32);
		snprintf(*prsp_cmd, 32, "\r\n%d,%d", param.id, param.data_len);
	
		return AT_END;
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

/* AT+NSOST=<socket>,<remote_addr>,<remote_port>,<length>,<data>[,<sequence>]*/
/* only for udp */
int at_SOCKSENT_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		char remote_addr[XY_IPADDR_STRLEN_MAX] = {0};
		socket_send_param_t param = {0};

		if(is_Uplink_FlowCtl_Open())
			return ATERR_NOT_ALLOWED;

		if (at_parse_param("%1d(0-),%47s,%2d(0-65535),%l,%p,%1d[0-255]", at_buf, &param.id, remote_addr,
							 &param.remote_port, &param.data_len, &param.data, &param.sequence) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		if(param.data != NULL)
			param.data_str_len = strlen(param.data);

		if (get_socket_net_type(param.id) != 1)
		{
			//not udp, err
			return ATERR_NOT_ALLOWED;
		}

		param.remote_ip = remote_addr;
		param.udp_connectd = 0;

		if (socket_send(&param) != XY_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		*prsp_cmd = xy_malloc(32);
		snprintf(*prsp_cmd, 32, "+NSOST:%d,%d", param.id, param.data_len);
		
		return AT_END;
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

/* AT+NSOSTEX=<socket>,<remote_addr>,<remote_port>,<sequence>,<length>,<data> */
/* only for udp */
int at_SOCKSENTEX_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		char remote_addr[XY_IPADDR_STRLEN_MAX] = {0};
		socket_send_param_t param = {0};
		
        param.rai_type = RAI_NULL;

		if(is_Uplink_FlowCtl_Open())
			return ATERR_NOT_ALLOWED;

		if (at_parse_param("%1d(0-),%47s,%2d(0-65535),%1d(0-255),%l,%p", at_buf, &param.id, remote_addr,
							 &param.remote_port, &param.sequence, &param.data_len, &param.data) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}
						 
		if(param.data != NULL)
			param.data_str_len = strlen(param.data);

		if (get_socket_net_type(param.id) != 1)
		{
			//not udp, err
			return ATERR_NOT_ALLOWED;
		}

        param.remote_ip = remote_addr;
		param.udp_connectd = 0;        

		if (socket_send(&param) != XY_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		*prsp_cmd = xy_malloc(32);
		snprintf(*prsp_cmd, 32, "\r\n%d,%d", param.id, param.data_len);
		
		return AT_END;
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

/* AT+NSOSTF=<socket>,<remote_addr>,<remote_port>,<flag>,<length>,<data>[,<sequence>] */
/* only for udp */
int at_SOCKSENTF_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		char remote_addr[XY_IPADDR_STRLEN_MAX] = {0};
		int rai_flag = 0;
		socket_send_param_t param = {0};

		if(is_Uplink_FlowCtl_Open())
			return ATERR_NOT_ALLOWED;

		if (at_parse_param("%1d(0-),%47s,%2d(0-65535),%d[0|256|512|1024],%l,%p,%1d[0-255]", at_buf, &param.id, remote_addr,
							  &param.remote_port, &rai_flag, &param.data_len, &param.data, &param.sequence) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}
						  
		if(param.data != NULL)
			param.data_str_len = strlen(param.data);

		if (get_socket_net_type(param.id) != 1)
		{
			//not udp, err
			return ATERR_NOT_ALLOWED;
		}

		// 标志转换 0x100->0, 0x200->1, 0x400->2, 未设置时置为0
		param.rai_type = (int8_t)((rai_flag == 0) ? RAI_NULL : ((rai_flag >> 9) & 0xFF));
		
		param.remote_ip = remote_addr;
		param.udp_connectd = 0;

		if (socket_send(&param) != XY_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		*prsp_cmd = xy_malloc(32);
		snprintf(*prsp_cmd, 32, "+NSOSTF:%d,%d", param.id, param.data_len);
		
		return AT_END;
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}	
}

/* AT+NSOSTFEX=<socket>,<remote_addr>,<remote_port>,<flag>,<sequence>,<length>,<data> */
/* only for udp */
int at_SOCKSENTFEX_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		char remote_addr[XY_IPADDR_STRLEN_MAX] = {0};
		socket_send_param_t param = {0};
		int rai_flag = 0;

		if(is_Uplink_FlowCtl_Open())
			return ATERR_NOT_ALLOWED;

		if (at_parse_param("%1d(0-),%47s,%2d(0-65535),%d[0|256|512|1024],%1d(0-255),%l,%p", at_buf, &param.id, remote_addr,
							 &param.remote_port, &rai_flag, &param.sequence, &param.data_len, &param.data) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		if(param.data != NULL)
			param.data_str_len = strlen(param.data);

		if (get_socket_net_type(param.id) != 1)
		{
			//not udp, err
			return ATERR_NOT_ALLOWED;
		}

		// 标志转换 0x100->0, 0x200->1, 0x400->2, 未设置时置为0
		param.rai_type = (int8_t)((rai_flag == 0) ? RAI_NULL : ((rai_flag >> 9) & 0xFF));

        param.remote_ip = remote_addr;
		param.udp_connectd = 0;

		if (socket_send(&param) != XY_OK)
		{
			return ATERR_PARAM_INVALID;
		}
		
		*prsp_cmd = xy_malloc(32);
		snprintf(*prsp_cmd, 32, "\r\n%d,%d", param.id, param.data_len);
		
		return AT_END;
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

/* AT+NSOCL=<socket> */
int at_SOCKCLZ_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		int sock_id = 0;

		if (at_parse_param("%d(0-)", at_buf, &sock_id) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		if (socket_close(sock_id) != XY_OK)
		{
			return ATERR_NOT_ALLOWED;
		}
		return AT_END;
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

/* AT+NSOSTATUS=<socket> */
int at_SOCKSTAT_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		uint8_t id = 0;
		uint8_t status = 0;
		if (at_parse_param("%1d(0-)", at_buf, &id) != AT_OK || id >= SOCK_NUM)
		{
			return ATERR_PARAM_INVALID;
		}
		
		status = is_socketId_valid(id) ? 0 : 1;

		*prsp_cmd = xy_malloc(48);
		snprintf(*prsp_cmd, 48, "+NSOSTATUS:%d,%d", id, status);

		return AT_END;
	}
	else if (g_req_type == AT_CMD_ACTIVE)
	{
		*prsp_cmd = xy_malloc(160);
		uint8_t socket_id = 0;
		uint8_t status = 0;
		
		**prsp_cmd = '\0';
		for (socket_id = 0; socket_id < SOCK_NUM; socket_id++)
		{
			status = is_socketId_valid(socket_id) ? 0 : 1;		
			snprintf(*prsp_cmd + strlen(*prsp_cmd), 160, "\r\n+NSOSTATUS:%d,%d\r\n", socket_id, status);
		}
		snprintf(*prsp_cmd + strlen(*prsp_cmd), 160, "\r\nOK\r\n");
		return AT_END;
	}
#if (AT_CUT!=1)
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(48); 
		snprintf(*prsp_cmd, 48, "+NSOSTATUS:(0-%d)", SOCK_NUM - 1);
		return AT_END;
	}
#endif
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

/* AT+NSONMI=<mode> */
int at_SOCKNMI_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		int report_mode_temp = -1;
		if (at_parse_param("%d(0-3)", at_buf, &report_mode_temp) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}
		g_at_sck_report_mode = report_mode_temp;
		return AT_END;
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(24);
		snprintf(*prsp_cmd, 24, "+NSONMI:%d", g_at_sck_report_mode);
		return AT_END;
    }
#if (AT_CUT!=1)
    else if (g_req_type == AT_CMD_TEST)
    {
        *prsp_cmd = xy_malloc(32);
        snprintf(*prsp_cmd, 32, "+NSONMI:(0,1,2,3)");
		return AT_END;
    }
#endif
    else
	{
		return ATERR_PARAM_INVALID;
	}
}

/* AT+SEQUENCE=<socket>,<sequence> */
int at_SOCKSEQ_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
        uint8_t seq_num = 0;
        uint8_t id = 0;
        if (at_parse_param("%1d(0-),%1d(1-255)", at_buf, &id, &seq_num) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		if (!is_socketId_valid(id))
		{
			return ATERR_PARAM_INVALID;
		}

		*prsp_cmd = xy_malloc(32);
        snprintf(*prsp_cmd, 32, "\r\n%d", get_socket_seq_state(id, seq_num));
		return AT_END;
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

/* AT+NSORF=<socket>,<req_length> */
int at_SOCKRF_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		uint8_t id = 0;
		uint32_t req_len = 0;

		if (at_parse_param("%1d(0-),%d(1-)", at_buf, &id, &req_len) != AT_OK || id >= SOCK_NUM)
		{
			return ATERR_PARAM_INVALID;
		}

		if (!is_socketId_valid(id))
		{
			return ATERR_NOT_ALLOWED;
		}

		if (socket_get_remaining_buffer_len(id) == 0)
		{
			return ATERR_NOT_ALLOWED;
		}

		socket_readbuffer_param_t param = {0};
		param.id = id;
		param.want_len = req_len;

		if (socket_read_buffer(&param) == XY_OK)
		{
			*prsp_cmd = xy_malloc2(param.read_len * 2 + 120);
			if (*prsp_cmd == NULL)
			{
				if (param.remote_ip != NULL)
				{
					xy_free(param.remote_ip);
				}
				if (param.data != NULL)
				{
					xy_free(param.data);
				}
				return ATERR_NO_MEM;
			}

			sprintf(*prsp_cmd, "\r\n%d,%s,%d,%d,%s,%d", id, param.remote_ip, param.remote_port, param.read_len, param.data, param.remaining_len);
			if (param.nsonmi_rpt == 1)
			{
				/* 一条消息处理完成后,还有下一条需要处理,使用异步方式发送新的 +NSONMI: 通知 */
				osThreadAttr_t thread_attr = {0};
				thread_attr.name = "rfnmi_rpt";
				thread_attr.priority = osPriorityNormal;
				thread_attr.stack_size = osStackShared;
				osThreadNew((osThreadFunc_t)(at_SOCKRFNMI_URC), id, &thread_attr);
			}
			if (param.remote_ip != NULL)
			{
				xy_free(param.remote_ip);
			}
			if (param.data != NULL)
			{
				xy_free(param.data);
			}		

			return AT_END;
		}
		else
		{
			return ATERR_NOT_ALLOWED;
		}
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

/* AT+NQSOS=<socket>[,<socket>][,<socket>...] */
int at_SOCKQSOS_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		uint32_t i = 0;
		uint32_t j = 0;
		int socket_ids[SOCK_NUM] = {0};
		for (i = 0; i < SOCK_NUM; i++)
		{
			socket_ids[i] = -1;
		}

		/* eg:AT+NQSOS=0,1,2-->error */
		if (at_strnchr(at_buf, ',', SOCK_NUM) != NULL)
		{
			return ATERR_PARAM_INVALID;
		}

		if (at_parse_param("%d(0-1),%d[0-1]", at_buf, &socket_ids[0], &socket_ids[1]) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}

        /* socket参数有效性检测 */
		for (i = 0; i < SOCK_NUM; i++)
		{
			for (j = i + 1; j < SOCK_NUM; j++)
			{
                /* socket参数重复检测 eg:AT+NQSOS=0,1,2,2 */
				if (socket_ids[i] != -1 && socket_ids[i] == socket_ids[j])
				{
					return ATERR_PARAM_INVALID;
				}
			}
		}


		char rsp_fmt[] = "\r\n+NQSOS:%d,%d\r\n";

		socket_pack_qsosrsp(socket_ids, rsp_fmt, prsp_cmd);

		snprintf(*prsp_cmd + strlen(*prsp_cmd), 20, "\r\nOK\r\n");
		
		return AT_END;
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		int i = 0;
        int socket_ids[SOCK_NUM] = {0};
		char rsp_fmt[] = "\r\n+NQSOS:%d,%d\r\n";
        for (i = 0; i < SOCK_NUM; i++)
		{
			socket_ids[i] = i;
		}
		socket_pack_qsosrsp(socket_ids, rsp_fmt, prsp_cmd);

		snprintf(*prsp_cmd + strlen(*prsp_cmd), 20, "\r\nOK\r\n");
		
		return AT_END;
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

int at_SOCKDTMD_req(char *at_buf, char **prsp_cmd)
{
    UNUSED_ARG(at_buf);
    UNUSED_ARG(prsp_cmd);
    return ATERR_NOT_ALLOWED;
}

int at_SOCKSTR_Default_URC(int id, uint16_t seqno, int8_t state)
{
    char *report = (char *)xy_malloc(32);
    snprintf(report, 32, "+NSOSTR:%d,%d,%d", id, seqno, state);
    send_urc_to_ext(report, strlen(report));
    xy_free(report);
    return XY_OK;
}

void at_SOCKRFNMI_Default_URC(void *arg)
{
	uint8_t id = (uint8_t)arg;
	char *urc_str = xy_malloc(48);
	snprintf(urc_str, 48, "+NSONMI:%d,%d", id, socket_get_remaining_buffer_len(id));
	send_urc_to_ext(urc_str, strlen(urc_str));
	xy_free(urc_str);
	osThreadExit();
}

int at_SOCKCLZ_Default_URC(int id, bool isquit)
{
	UNUSED_ARG(isquit);
    char *report_buf = xy_malloc(32);

    snprintf(report_buf, 32, "+NSOCLI:%d", id);
	send_urc_to_ext(report_buf, strlen(report_buf));
   
    xy_free(report_buf);
    return XY_OK;
}

int at_SOCKDATAMODE_EXIT_Default_URC(void *arg)
{
	UNUSED_ARG(arg);
	if (g_socket_passthr_info->result == XY_OK)
	{
		char* info = xy_malloc(32);
		snprintf(info, 32, "\r\n%d,%d\r\n\r\nOK\r\n", g_socket_passthr_info->socket_id, g_socket_passthr_info->send_length);
		send_rsp_at_to_ext(info);
		xy_free(info);
	}
	else
	{
		send_rsp_at_to_ext("\r\nERROR\r\n");
	}

    return XY_OK;
}

int at_SOCKNMI_Default_URC(int id, uint32_t read_len, char *buf, void *remoteinfo)
{
    xy_assert(buf != NULL);
    char *rsp_cmd = NULL;
    char *temp_buf = NULL;
    struct sockaddr_storage* remote_info = (struct sockaddr_storage*)remoteinfo;

    if (g_at_sck_report_mode == HINT_WITH_REMOTE_INFO)
    {
        char *remote_ip = xy_malloc(40);
		int port;
		if (remote_info->ss_family == AF_INET6)
		{
			struct sockaddr_in6 * addr6 = remote_info;
			inet_ntop(AF_INET6, &(addr6->sin6_addr), remote_ip, 40);
			port = ntohs(addr6->sin6_port);
		}
		else
		{
			struct sockaddr_in * addr4 = remote_info;
			inet_ntop(AF_INET, &(addr4->sin_addr), remote_ip, 40);
			port = ntohs(addr4->sin_port);
		}
        if (g_data_recv_mode == ASCII_STRING) /* 文本格式 */
        {
            int size = 0;
            rsp_cmd = xy_malloc2(80 + read_len);
			if (rsp_cmd == NULL)
			{
				xy_free(remote_ip);
				xy_free(remote_info);
				return XY_ERR;
			}
            snprintf(rsp_cmd, 80 + read_len, "\r\n+NSONMI:%d,%s,%d,%d,", id, remote_ip, port, read_len);
            size += strlen(rsp_cmd);
            memcpy(rsp_cmd + size, buf, read_len);
            size += read_len;
            memcpy(rsp_cmd + size, "\r\n", 2);
            size += 2;
            send_urc_to_ext_NoCache(rsp_cmd, size);
            xy_free(rsp_cmd);
            xy_free(remote_ip);
            // xy_free(buf);
            xy_free(remote_info);
			
            return XY_OK;
        }
        else /* 十六进制格式 */
        {
            temp_buf = xy_malloc2(read_len * 2 + 1);
			if (temp_buf == NULL)
			{
				xy_free(remote_ip);
				xy_free(remote_info);
				return XY_ERR;
			}
            bytes2hexstr(buf, read_len, temp_buf, read_len * 2 + 1);
            rsp_cmd = xy_malloc2(80 + read_len * 2);
 			if (rsp_cmd == NULL)
			{
				xy_free(remote_ip);
				xy_free(remote_info);
				xy_free(temp_buf);
				return XY_ERR;
			}
			snprintf(rsp_cmd, 80 + read_len * 2, "+NSONMI:%d,%s,%d,%d,%s", g_socket_ctx[id]->sock_id, remote_ip, port, read_len, temp_buf);
        }

        xy_free(remote_ip);
        // xy_free(buf);
        xy_free(remote_info);
    }
    else if (g_at_sck_report_mode == HINT_NO_REMOTE_INFO)
    {
        if (g_data_recv_mode == ASCII_STRING) /* 文本格式 */
        {
            int size = 0;
            rsp_cmd = xy_malloc2(80 + read_len);
			if (rsp_cmd == NULL)
			{
				xy_free(remote_info);
				return XY_ERR;
			}
            snprintf(rsp_cmd, 80 + read_len, "\r\n+NSONMI:%d,%d,", id, read_len);
            size += strlen(rsp_cmd);
            memcpy(rsp_cmd + size, buf, read_len);
            size += read_len;
            memcpy(rsp_cmd + size, "\r\n", 2);
            size += 2;
            send_urc_to_ext_NoCache(rsp_cmd, size);
            xy_free(rsp_cmd);
            // xy_free(buf);
            xy_free(remote_info);
            return XY_OK;
        }
        else
        {
            temp_buf = xy_malloc2(read_len * 2 + 1);
			if (temp_buf == NULL)
			{
				xy_free(remote_info);
				return XY_ERR;
			}
            bytes2hexstr(buf, read_len, temp_buf, read_len * 2 + 1);
            rsp_cmd = xy_malloc2(80 + read_len * 2);
			if (rsp_cmd == NULL)
			{
				xy_free(temp_buf);
				xy_free(remote_info);
				return XY_ERR;
			}
			snprintf(rsp_cmd, 80 + read_len * 2, "+NSONMI:%d,%d,%s", id, read_len, temp_buf);
        }
        // xy_free(buf);
        xy_free(remote_info);
    }
    else if (add_new_rcv_data_node(id, read_len, buf, remote_info) == XY_OK)
    {
        //+NSONMI:<socket>,<length>
        if (g_at_sck_report_mode == BUFFER_WITH_HINT || g_at_sck_report_mode == BUFFER_NO_HINT)
        {
            xy_printf(0,XYAPP, WARN_LOG, "socket[%d] recv %d length downlink data!!!", id, read_len);
            if (g_at_sck_report_mode == BUFFER_WITH_HINT && g_socket_ctx[id]->firt_recv == 0)
            {
                rsp_cmd = xy_malloc(36);
                snprintf(rsp_cmd, 36, "+NSONMI:%d,%d", id, read_len);
                SOCKET_CTX_LOCK();
                g_socket_ctx[id]->firt_recv = 1;
                SOCKET_CTX_UNLOCK();
            }
        }
    }
    else
    {
        rsp_cmd = xy_malloc(60);
        snprintf(rsp_cmd, 60, "+NSONMI:drop %d bytes pkt", read_len);
    }

    if (temp_buf != NULL)
        xy_free(temp_buf);
    if (rsp_cmd != NULL)
    {
        send_urc_to_ext(rsp_cmd, strlen(rsp_cmd));
        xy_free(rsp_cmd);
    }
    return XY_OK;
}
