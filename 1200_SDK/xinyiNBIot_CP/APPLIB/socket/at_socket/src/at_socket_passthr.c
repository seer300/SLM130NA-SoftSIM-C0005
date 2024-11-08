/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#if AT_SOCKET
#include "at_ctl.h"
#include "at_passthrough.h"
#include "at_socket_context.h"
#include "at_socket_passthr.h"
#include "xy_socket_api.h"
#include "xy_at_api.h"
#include "xy_net_api.h"
#include "xy_utils.h"

/*******************************************************************************
 *						   Global variable definitions						   *
 ******************************************************************************/
socket_passthr_info_t* g_socket_passthr_info = NULL;

/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
/*************SOCKET BC95 START ****************/
int socket_passthr_send(uint8_t id, char *data, uint32_t data_len, uint8_t seqno, uint8_t rai_type, uint32_t presn)
{
	int ret = -1;
	if (!is_socketId_valid(id))
	{
		return XY_ERR;
	}

	socket_context_t *ctx = g_socket_ctx[id];

	if (g_socket_passthr_info->data_send_mode == HEX_ASCII_STRING)
	{
		if (data_len % 2 != 0)
		{
			return XY_ERR;
		}
		char *hex_stream_data = xy_malloc(data_len / 2);
		if (hexstr2bytes(data, data_len, hex_stream_data, data_len / 2) == -1)
		{
			xy_free(hex_stream_data);
			return XY_ERR;
		}
		if (ctx->net_type == 0 || g_socket_passthr_info->udp_connect)
		{
			ret = send2(ctx->fd, hex_stream_data, data_len / 2, 0, seqno, rai_type);
		}
		else
		{
			ret = sendto2(ctx->fd, hex_stream_data, data_len/2, 0, (struct sockaddr *)&g_socket_passthr_info->sock_addr, sizeof(struct sockaddr_storage), seqno, rai_type);
		}
		xy_free(hex_stream_data);
	}
	else if (g_socket_passthr_info->data_send_mode == ASCII_STRING)
	{
		if (ctx->net_type == 0 || g_socket_passthr_info->udp_connect)
		{
			ret = send2(ctx->fd, data, data_len, 0, seqno, rai_type);
		}
		else
		{
			ret = sendto2(ctx->fd, data, data_len, 0, (struct sockaddr *)&g_socket_passthr_info->sock_addr, sizeof(struct sockaddr_storage), seqno, rai_type);
		}
	}

	if (ret <= 0)
	{
		xy_printf(0,XYAPP, WARN_LOG, "socket[%d] passthr send errno:%d", ctx->sock_id, errno);
		if (seqno != 0)
		{
			ctx->sequence_state[seqno - 1] = SEND_STATUS_FAILED;
		}
		return XY_ERR;
	}

	ctx->sended_size += ret;
	g_socket_passthr_info->send_length = ret;
	if (seqno != 0)
	{
		add_sninfo_node(ctx->sock_id, ret, seqno, presn);
		ctx->sequence_state[g_socket_passthr_info->sequence - 1] = SEND_STATUS_SENDING;
	}
	return XY_OK;
}

void socket_fixed_length_passthr_proc(char *buf, uint32_t len)
{
	xy_printf(0,XYAPP, WARN_LOG, "socket fixed recv len:%d", len);
	if (g_socket_passthr_info == NULL)
		xy_assert(0);
    if (passthr_fixed_buff_len == 0)
        return;

    //申请内存
    if (passthr_rcv_buff == NULL)
    {
        passthr_rcv_buff = xy_malloc(passthr_fixed_buff_len);
    }
	if (g_socket_passthr_info->data_echo)
		send_urc_to_ext_NoCache(buf, len);
    if ((passthr_rcvd_len + len) < passthr_fixed_buff_len)
    {
		memcpy(passthr_rcv_buff + passthr_rcvd_len, buf, len);
        passthr_rcvd_len += len;
    }
	else
	{
		if (passthr_rcvd_len < passthr_fixed_buff_len)
		{ 
			memcpy(passthr_rcv_buff + passthr_rcvd_len, buf, passthr_fixed_buff_len - passthr_rcvd_len);
			passthr_rcvd_len = passthr_fixed_buff_len;
		}

        //数据发送
		if (socket_passthr_send(g_socket_passthr_info->socket_id, passthr_rcv_buff, passthr_fixed_buff_len,
								g_socket_passthr_info->sequence, g_socket_passthr_info->rai_flag, g_socket_passthr_info->pre_sn) != XY_OK)
		{
			g_socket_passthr_info->result = XY_ERR;
		}

EXIT_PASSTHR:
		//退出透传
		xy_exitPassthroughMode();
	}
}

void socket_fixed_length_passthr_exit(void)
{
	//释放内存
	if (passthr_rcv_buff != NULL)
	{
		xy_free(passthr_rcv_buff);
		passthr_rcv_buff = NULL;
	}

	passthr_rcvd_len = 0;
	passthr_fixed_buff_len = 0;

	at_SOCKDATAMODE_EXIT_URC(NULL);

	if (g_socket_passthr_info != NULL)
	{
		xy_free(g_socket_passthr_info);
		g_socket_passthr_info = NULL;
	}
}

void socket_unfixed_length_passthr_proc(char *buf, uint32_t len)
{
	xy_printf(0,XYAPP, WARN_LOG, "socket passthr unfixed recv len:%d", len);
	if (g_socket_passthr_info == NULL)
		xy_assert(0);
	
	passthr_get_unfixedlen_data(buf, len);
	
	if (buf[len - 1] == PASSTHR_ESC)	/* ESC取消发送  */
	{
		goto EXIT_PASSTHR;
	}
	else if (buf[len - 1] == PASSTHR_CTRLZ) /* CtrlZ发送数据 */
	{
		if (passthr_rcvd_len == 1)
		{
			/* 只收到CtrlZ一个字节数据,报错 */
			g_socket_passthr_info->result = XY_ERR;
			goto EXIT_PASSTHR;
		}
		if (socket_passthr_send(g_socket_passthr_info->socket_id, passthr_rcv_buff, passthr_rcvd_len - 1,
									g_socket_passthr_info->sequence, g_socket_passthr_info->rai_flag, g_socket_passthr_info->pre_sn) != XY_OK)
		{
			g_socket_passthr_info->result = XY_ERR;
		}
	}
	else
	{
		if (g_socket_passthr_info->data_echo)
			send_urc_to_ext_NoCache(buf, len);
		
		if ((g_socket_passthr_info->data_send_mode == HEX_ASCII_STRING && passthr_rcvd_len >= AT_SOCKET_MAX_DATA_LEN * 2) ||
				(g_socket_passthr_info->data_send_mode == ASCII_STRING && passthr_rcvd_len >= AT_SOCKET_MAX_DATA_LEN))
		{
			if (g_socket_passthr_info->data_send_mode == HEX_ASCII_STRING)
			{
				passthr_rcvd_len = AT_SOCKET_MAX_DATA_LEN * 2;
			}
			else
			{
				passthr_rcvd_len = AT_SOCKET_MAX_DATA_LEN;
			}

			/* 已接收的数据大于AT_SOCKET_MAX_DATA_LEN时，自动发送数据，需要注意的是，十六进制模式下，接收的最大数据为AT_SOCKET_MAX_DATA_LEN * 2 */
			if (socket_passthr_send(g_socket_passthr_info->socket_id, passthr_rcv_buff, passthr_rcvd_len,
										g_socket_passthr_info->sequence, g_socket_passthr_info->rai_flag, g_socket_passthr_info->pre_sn) != XY_OK)
			{
				g_socket_passthr_info->result = XY_ERR;
			}
		}
		else
		{
			//未收到ctrl+z or esc,继续接收
			return;
		}
	} 

EXIT_PASSTHR:
	xy_exitPassthroughMode();
}

void socket_unfixed_length_passthr_exit(void)
{
	//释放内存
	if (passthr_rcv_buff != NULL)
	{
		xy_free(passthr_rcv_buff);
		passthr_rcv_buff = NULL;
	}

	passthr_rcvd_len = 0;
	passthr_fixed_buff_len = 0;

	at_SOCKDATAMODE_EXIT_URC(NULL);

	if (g_socket_passthr_info != NULL)
	{
		xy_free(g_socket_passthr_info);
		g_socket_passthr_info = NULL;
	}
}
/*************SOCKET BC95 END ****************/

#endif //AT_SOCKET
