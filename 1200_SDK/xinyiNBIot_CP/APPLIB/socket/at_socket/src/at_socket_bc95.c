/**
 * @file at_socket_quectel.c
 * @brief 
 * @version 1.0
 * @date 2022-06-21
 * @copyright Copyright (c) 2022  芯翼信息科技有限公司
 * 
 */
#if AT_SOCKET

#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "oss_nv.h"
#include "at_socket.h"
#include "at_com.h"
#include "at_utils.h"
#include "at_socket_context.h"
#include "at_socket_passthr.h"
#include "xy_at_api.h"
#include "xy_net_api.h"
#include "at_socket_api.h"
#include "cloud_proxy.h"
#include "at_tcpip_api.h"


#if VER_BC95
/* AT+NSOCR=<type>,<protocol>,<listenport>[,<receive control>,<af type>,<ip addr>] */
int at_SOCKCFG_BC95_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		char type[7] = {0};
		char af_type[10] = {0};
		char ipaddr[XY_IPADDR_STRLEN_MAX] = {0};
	    int sockid = -1;
		socket_create_param_t create_param = {0};
		create_param.recv_ctrl = 1;

		if (at_parse_param("%7s,%d,%2d(0-65535),%1d[0-6],%10s,%47s", 
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

        if (strlen(ipaddr) > 0)
        {
            if (create_param.af_type == AF_INET6)
            {
                if (xy_IpAddr_Check(ipaddr, IPV6_TYPE) == 0)
                {
					return ATERR_PARAM_INVALID;
                }
            }
            else
            {
                if (xy_IpAddr_Check(ipaddr, IPV4_TYPE) == 0)
                {
					return ATERR_PARAM_INVALID;
                }
            }
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
		snprintf(*prsp_cmd, 24, "\r\n%d", sockid);
		
		return AT_END;
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

int at_SOCKCONN_BC95_req(char *at_buf, char **prsp_cmd)
{
	//char *report_buf = xy_malloc(32);

	if (g_req_type == AT_CMD_REQ)
	{
		char remote_ip[XY_IPADDR_STRLEN_MAX] = {0};
		socket_conn_param_t param = {0};

		if (at_parse_param("%d(0-),%47s,%2d(1-65535)", at_buf, &param.id, remote_ip, &param.remote_port) != AT_OK)
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

		//snprintf(report_buf, 32, "\r\nOK\r\n");
		//write_to_at_uart(report_buf,strlen(report_buf));
		//memset(report_buf,0,32);

		call_socket_delay(SOCK_TCP_CTL_TIMEOUT,param.id,SOCKT_TIMEOUT_30S,0);

		if (socket_connect(&param) != XY_OK)
		{
			call_socket_delay(SOCK_CANCEL_CHECK,param.id,SOCKT_TIMEOUT_30S,0);
			//xy_free(report_buf);
			return ATERR_NOT_NET_CONNECT;
		}
		call_socket_delay(SOCK_CANCEL_CHECK,param.id,SOCKT_TIMEOUT_30S,0);

		if (g_softap_fac_nv->sock_async != 0)
		{
			*prsp_cmd = xy_malloc(48);
			snprintf(*prsp_cmd, 48, "\r\nOK\r\n\r\n+NSOCO:%d\r\n", param.id);
		}

		return AT_END;
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

/* AT+NSOSD=<socket>,<length>,<data>[,<flag>[,<sequence>]] */
int at_SOCKSEND_BC95_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		int rai_flag = 0;
		socket_send_param_t param = {0};

		if(is_Uplink_FlowCtl_Open())
			return ATERR_NOT_ALLOWED;

		/* BC95 sequence不可输入0 */
		if (at_parse_param("%1d(0-),%l,%p,%d[0|256|512|1024],%1d[1-255]", at_buf, &param.id, &param.data_len, &param.data, &rai_flag, &param.sequence) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		if(param.data != NULL)
			param.data_str_len = strlen(param.data);

		if (get_socket_net_type(param.id) != 0)
		{
			return ATERR_NOT_ALLOWED;
		}

		// 标志转换 0x100->0, 0x200->1, 0x400->2, 未设置时置为0
		param.rai_type = (int8_t)((rai_flag == 0) ? RAI_NULL : ((rai_flag >> 9) & 0xFF));

		if (param.sequence != 0) {
			call_socket_delay(SOCK_TCP_CTL_TIMEOUT,param.id,SOCKT_TIMEOUT_30S,param.sequence);
		}
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

int at_SOCKSENT_BC95_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		char remote_addr[XY_IPADDR_STRLEN_MAX] = {0};
		socket_send_param_t param = {0};

		if(is_Uplink_FlowCtl_Open())
			return ATERR_NOT_ALLOWED;

 		if (at_parse_param("%1d(0-),%47s,%2d(0-65535),%l,%p,%1d[1-255]", at_buf, &param.id, remote_addr, &param.remote_port,
							&param.data_len, &param.data, &param.sequence) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		if(param.data != NULL)
			param.data_str_len = strlen(param.data);

		socket_resume();
		if (get_socket_net_type(param.id) != 1)
		{
			return ATERR_NOT_ALLOWED;
		}

		param.remote_ip = remote_addr;
		param.udp_connectd = 0;
		if (param.sequence != 0) {
			call_socket_delay(SOCK_UDP_CTL_TIMEOUT,param.id,SOCKT_TIMEOUT_30S,param.sequence);
		}
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

int at_SOCKSENTF_BC95_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		char remote_addr[XY_IPADDR_STRLEN_MAX] = {0};
		int rai_flag = 0;
		socket_send_param_t param = {0};

		if(is_Uplink_FlowCtl_Open())
			return ATERR_NOT_ALLOWED;

		/* BC95 sequence不可输入0 */
		if (at_parse_param("%1d(0-),%47s,%2d(0-65535),%d[0|256|512|1024],%l,%p,%1d[1-255]", at_buf, &param.id, remote_addr,
							&param.remote_port, &rai_flag, &param.data_len, &param.data, &param.sequence) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		if(param.data != NULL)
			param.data_str_len = strlen(param.data);

		socket_resume();
		if (get_socket_net_type(param.id) != 1)
		{
			return ATERR_NOT_ALLOWED;
		}

		// 标志转换 0x100->0, 0x200->1, 0x400->2, 未设置时置为0
		param.rai_type = (int8_t)((rai_flag == 0) ? RAI_NULL : ((rai_flag >> 9) & 0xFF));
		param.remote_ip = remote_addr;
		param.udp_connectd = 0;

		if (param.sequence != 0) {
			call_socket_delay(SOCK_UDP_CTL_TIMEOUT,param.id,SOCKT_TIMEOUT_30S,param.sequence);
		}
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

int at_SOCKRF_BC95_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		uint8_t id = 0;
		uint32_t req_len = 0;

		if (at_parse_param("%1d(0-),%d(1-)", at_buf, &id, &req_len) != AT_OK || id >= SOCK_NUM)
		{
			return ATERR_PARAM_INVALID;
		}

        if (req_len > AT_SOCKET_MAX_DATA_LEN)
        {
			return ATERR_PARAM_INVALID;
        }

		if (!is_socketId_valid(id))
		{
			return ATERR_NOT_ALLOWED;
		}

        if (socket_get_remaining_buffer_len(id) == 0)
        {
			return AT_END;
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

int at_SOCKCLZ_BC95_req(char *at_buf, char **prsp_cmd)
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
		return AT_ASYN;
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

int at_SOCKQSOS_BC95_req(char *at_buf, char **prsp_cmd)
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

        /* eg:AT+NQSOS=0,1,2,3,4,5,6,7-->error */
        if (at_strnchr(at_buf, ',', SOCK_NUM) != NULL)
        {
			return ATERR_PARAM_INVALID;
        }

        if (at_parse_param("%d(0-6),%d[0-6],%d[0-6],%d[0-6],%d[0-6],%d[0-6],%d[0-6]", at_buf, &socket_ids[0], &socket_ids[1],
                  &socket_ids[2], &socket_ids[3], &socket_ids[4], &socket_ids[5], &socket_ids[6]) != AT_OK)
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

		char rsp_fmt[] = "\r\n+NQSOS:%d,%d";

		socket_pack_qsosrsp(socket_ids, rsp_fmt, prsp_cmd);

		if (strlen(*prsp_cmd) == 0)
			snprintf(*prsp_cmd, 20, "\r\nOK\r\n");

		return AT_END;
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		int i = 0;
        int socket_ids[SOCK_NUM] = {0};
		char rsp_fmt[] = "\r\n+NQSOS:%d,%d";
        for (i = 0; i < SOCK_NUM; i++)
		{
			socket_ids[i] = i;
		}
		socket_pack_qsosrsp(socket_ids, rsp_fmt, prsp_cmd);
		
		if (strlen(*prsp_cmd) == 0)
			snprintf(*prsp_cmd, 20, "\r\nOK\r\n");

		return AT_END;
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}

int at_SOCKSTAT_BC95_req(char *at_buf, char **prsp_cmd)
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
			snprintf(*prsp_cmd + strlen(*prsp_cmd), 160, "\r\n+NSOSTATUS:%d,%d", socket_id, status);
		}

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

/* AT+NSOSDEX=<socket>,<flag>,<sequece>,<length>,<data> */
/* only for tcp*/
int at_SOCKSENDEX_BC95_req(char *at_buf, char **prsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
	{
		int rai_flag = 0;
		socket_send_param_t param = {0};
        uint8_t is_passthr_mode = 0;

		if(is_Uplink_FlowCtl_Open())
			return ATERR_NOT_ALLOWED;

		if (at_strnchr(at_buf, ',', 4) != NULL)
		{
			if (at_parse_param("%1d(0-),%d(0|256|512|1024),%1d(0-255),%l,%p",
				at_buf, &param.id, &rai_flag, &param.sequence, &param.data_len, &param.data) != AT_OK)
			{
				return ATERR_PARAM_INVALID;
			}			
		}
		else
		{
			/* 透传类型AT指令中不包含data参数 AT+NSOSDEX=<socket>,<flag>,<sequece>,[length]				 
			   若长度不设置为不定长度透传,若设置为指定长度透传       	*/
			if (at_parse_param("%1d(0-),%d(0|256|512|1024),%1d(0-255),%d",
				at_buf, &param.id, &rai_flag, &param.sequence, &param.data_len) != AT_OK)
			{
				return ATERR_PARAM_INVALID;
			}			
			/* 需进入透传模式 */
            is_passthr_mode = 1;
        }

		if(param.data != NULL)
			param.data_str_len = strlen(param.data);

        if (get_socket_net_type(param.id) != 0)
		{
			//not tcp, err
			return ATERR_NOT_ALLOWED;
		}

		param.rai_type = (int8_t)((rai_flag == 0) ? RAI_NULL : ((rai_flag >> 9) & 0xFF));

		if (param.sequence != 0) {
			call_socket_delay(SOCK_TCP_CTL_TIMEOUT,param.id,SOCKT_TIMEOUT_30S,param.sequence);
		}
        /* 透传模式 */
        if (is_passthr_mode == 1)
        {
        	int xy_ret;
			// 移远透传数据格式为ASCII码
			param.passthr_data_mode = ASCII_STRING;
			xy_ret = socket_enter_passthr_mode(&param);			
			if (XY_OK == xy_ret)
			{
				return AT_ASYN;
			}
			else
			{
				return at_get_errno_by_syserr(xy_ret);
			}
		}

		
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

/* AT+NSOSTEX=<socket>,<remote_addr>,<remote_port>,<sequence>,<length>,<data> */
/* only for udp */
int at_SOCKSENTEX_BC95_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		char remote_addr[XY_IPADDR_STRLEN_MAX] = {0};
		socket_send_param_t param = {0};
        uint8_t is_passthr_mode = 0;
		
        param.rai_type = RAI_NULL;

		if(is_Uplink_FlowCtl_Open())
			return ATERR_NOT_ALLOWED;

		if (at_strnchr(at_buf, ',', 5) != NULL)
		{
			if (at_parse_param("%1d(0-),%47s,%2d(0-65535),%1d(0-255),%l,%p", at_buf, &param.id, remote_addr,
								 &param.remote_port, &param.sequence, &param.data_len, &param.data) != AT_OK)
			{
				return ATERR_PARAM_INVALID;
			}
		}
		else
		{
			/*  透传类型AT指令中不包含data参数 AT+NSOSTEX=<socket>,<remote_addr>,<remote_port>,<sequence>,[length]			  
				若长度不设置为不定长度透传,若设置为指定长度透传			 */
			if (at_parse_param("%1d(0-),%47s,%2d(0-65535),%1d(0-255),%d", at_buf, &param.id, remote_addr,
								 &param.remote_port, &param.sequence, &param.data_len) != AT_OK)
			{
				return ATERR_PARAM_INVALID;
			}			 
			/* 需进入透传模式 */
			is_passthr_mode = 1;
		}
		
		if(param.data != NULL)
			param.data_str_len = strlen(param.data);

		socket_resume();
		if (get_socket_net_type(param.id) != 1)
		{
			//not udp, err
			return ATERR_NOT_ALLOWED;
		}

        param.remote_ip = remote_addr;
		param.udp_connectd = 0;
		
		if (param.sequence != 0) {
			call_socket_delay(SOCK_UDP_CTL_TIMEOUT,param.id,SOCKT_TIMEOUT_30S,param.sequence);
		}

        /* 透传模式 */
        if (is_passthr_mode == 1)
        {
        	int xy_ret;
			// 移远透传数据格式为ASCII码
			param.passthr_data_mode = ASCII_STRING;
			xy_ret = socket_enter_passthr_mode(&param);
			if (XY_OK == xy_ret)
			{
				return AT_ASYN;
			}
			else
			{
				return at_get_errno_by_syserr(xy_ret);
			}
		}

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

/* AT+NSOSTFEX=<socket>,<remote_addr>,<remote_port>,<flag>,<sequence>,<length>,<data> */
/* only for udp */
int at_SOCKSENTFEX_BC95_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		char remote_addr[XY_IPADDR_STRLEN_MAX] = {0};
		socket_send_param_t param = {0};
        uint8_t is_passthr_mode = 0;
		int rai_flag = 0;

		if(is_Uplink_FlowCtl_Open())
			return ATERR_NOT_ALLOWED;

		if (at_strnchr(at_buf, ',', 6) != NULL)
		{
			if (at_parse_param("%1d(0-),%47s,%2d(0-65535),%d[0|256|512|1024],%1d(0-255),%l,%p", at_buf, &param.id, remote_addr,
								 &param.remote_port, &rai_flag, &param.sequence, &param.data_len, &param.data) != AT_OK)
			{
				return ATERR_PARAM_INVALID;
			}
		}
		else
		{
		 	/*  透传类型AT指令中不包含data参数 AT+NSOSTFEX=<socket>,<remote_addr>,<remote_port>,<flag>,<sequence>,[length] 		   
			 	若长度不设置为不定长度透传,若设置为指定长度透传 		  */
			if (at_parse_param("%1d(0-),%47s,%2d(0-65535),%d[0|256|512|1024],%1d(0-255),%d", at_buf, &param.id, remote_addr,
								 &param.remote_port, &rai_flag, &param.sequence, &param.data_len) != AT_OK)
			{
				return ATERR_PARAM_INVALID;
			}
		 	/* 需进入透传模式 */
		 	is_passthr_mode = 1;
		}

		if(param.data != NULL)
			param.data_str_len = strlen(param.data);
			
		socket_resume();
		if (get_socket_net_type(param.id) != 1)
		{
			//not udp, err
			return ATERR_NOT_ALLOWED;
		}

		// 标志转换 0x100->0, 0x200->1, 0x400->2, 未设置时置为0
		param.rai_type = (int8_t)((rai_flag == 0) ? RAI_NULL : ((rai_flag >> 9) & 0xFF));

        param.remote_ip = remote_addr;
		param.udp_connectd = 0;

		if (param.sequence != 0) {
			call_socket_delay(SOCK_UDP_CTL_TIMEOUT,param.id,SOCKT_TIMEOUT_30S,param.sequence);
		}

		/* 透传模式 */
        if (is_passthr_mode == 1)
        {
        	int xy_ret;
			// 移远透传数据格式为ASCII码
			param.passthr_data_mode = ASCII_STRING;
			xy_ret =  socket_enter_passthr_mode(&param);
			if (XY_OK == xy_ret)
			{
				return AT_ASYN;
			}
			else
			{
				return at_get_errno_by_syserr(xy_ret);
			}
		}

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

int at_SOCKCLZ_BC95_URC(int id, bool isquit)
{
	char *report_buf = xy_malloc(32);
	if (isquit)
	{
		snprintf(report_buf, 32, "\r\nOK\r\n");
		send_rsp_at_to_ext(report_buf);
	}
	// else if (get_socket_net_type(id) == 0)
	else  // 不管是TCP 还是 UDP 连接，都需要上报连接断开
	{
		snprintf(report_buf, 32, "+NSOCLI:%d", id);
		send_urc_to_ext(report_buf, strlen(report_buf));
	}
	xy_free(report_buf);
	return XY_OK;
}

int at_SOCKCONN_BC95_URC(int id, int err)
{
	char *at_rsp = NULL;
	
	if (err == XY_OK)
	{
		at_rsp = at_ok_build();
		send_rsp_at_to_ext(at_rsp);

		char *conn_urc = xy_malloc(48);
		snprintf(conn_urc, 48, "\r\n+NSOCO:%d\r\n", id);
		send_urc_to_ext(conn_urc, strlen(conn_urc));
		xy_free(conn_urc);
	}
	else
	{
		at_rsp = AT_ERR_BUILD(err);
		send_rsp_at_to_ext(at_rsp);
	}
	xy_free(at_rsp);
	return XY_OK;
}
#endif /* VER_BC95 */
#endif /* AT_SOCKET */
