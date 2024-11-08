

#if AT_SOCKET

#include "at_socket.h"
#include "at_socket_context.h"
#include "lwip/sockets.h"
#include "xy_net_api.h"
#include "xy_utils.h"

void netif_down_close_socket(uint32_t eventId)
{
    UNUSED_ARG(eventId);
	int idx = 0;
	for (idx = 0; idx < SOCK_NUM; idx++)
	{
		if (g_socket_ctx[idx] != NULL && g_socket_ctx[idx]->fd >= 0)
		{
#if VER_BC95
			// 移远需求，网络不好时，AT创建的UDP socket不关闭
			if (g_socket_ctx[idx]->net_type == 1)
				continue;
#endif
			g_socket_ctx[idx]->is_deact = 1;
			xy_printf(0,XYAPP, WARN_LOG, "socket quit set 1!!!");
		}
	}
}

void at_sock_recv_thread(void)
{
    int ret = 0;
    uint8_t i = 0;
    int max_fd;
	fd_set readfds, exceptfds;
	struct timeval tv;
    tv.tv_sec = SOCK_RECV_TIMEOUT;
    tv.tv_usec = 0;
    char *buf = (char *)xy_malloc(AT_SOCKET_MAX_DATA_LEN);
	xy_reg_psnetif_callback(EVENT_PSNETIF_INVALID, netif_down_close_socket);

    while (1)
    {
        FD_ZERO(&readfds); 
		FD_ZERO(&exceptfds);
        max_fd = -1;

        for (i = 0; i < SOCK_NUM; i++) 
		{
			if (g_socket_ctx[i] != NULL && g_socket_ctx[i]->quit == 1 && g_socket_ctx[i]->fd >= 0)
			{
                del_socket_ctx_by_index(i, true);
                continue;
			}
            else if (g_socket_ctx[i] != NULL && g_socket_ctx[i]->is_deact == 1 && g_socket_ctx[i]->fd >= 0)
            {
                del_socket_ctx_by_index(i, false);
                continue;
			}
			else if (g_socket_ctx[i] != NULL && g_socket_ctx[i]->quit != 1 && g_socket_ctx[i]->fd >= 0) 
			{
                if (max_fd < g_socket_ctx[i]->fd)
                {
					max_fd=g_socket_ctx[i]->fd;
				}
				FD_SET(g_socket_ctx[i]->fd, &readfds);
				FD_SET(g_socket_ctx[i]->fd, &exceptfds);
			}
		}

		if (max_fd < 0) 
		{
            goto out;
		}

        ret = select(max_fd + 1, &readfds, NULL, &exceptfds, &tv);
        if (ret < 0)
		{
			if (errno == EBADF)
			{
				xy_assert(0);
			}
			else
			{
				continue;
			}
		}
		else if (ret == 0)
        {
            //we assume select timeouted here
            continue;
        }

        for (i = 0; i < SOCK_NUM; i++)
        {
            if (g_socket_ctx[i] == NULL)
                continue;

            if (g_socket_ctx[i] != NULL && FD_ISSET(g_socket_ctx[i]->fd, &exceptfds))
            {
                xy_printf(0,XYAPP, WARN_LOG, "except exist,force to close socket[%d]", i);
                del_socket_ctx_by_index(i, false);
                continue;
			}
            if (g_socket_ctx[i] != NULL && !FD_ISSET(g_socket_ctx[i]->fd, &readfds))
            {
                continue;
            }

            // int len = 0;
			int read_len = 0;
			// char *buf = NULL;
			struct sockaddr_storage *remote_info = NULL;
			socklen_t fromlen = sizeof(struct sockaddr_storage);

			// ioctl(g_socket_ctx[i]->fd, FIONREAD, &len);  //get the size of received data
            // xy_printf(0,XYAPP, WARN_LOG, "socket[%d] fionread len:%d", i, len);
            // if (len == 0)
            // {
            //     xy_printf(0,XYAPP, WARN_LOG, "socket[%d] recv 0 BYTES,force to close socket", i);
            //     del_socket_ctx_by_index(i, true);
			// 	continue;
            // }

            // buf = xy_malloc(len+1);

			remote_info = xy_malloc(sizeof(struct sockaddr_storage));
            read_len = recvfrom(g_socket_ctx[i]->fd, buf, AT_SOCKET_MAX_DATA_LEN, MSG_DONTWAIT, (struct sockaddr *)remote_info, &fromlen);
            xy_printf(0,XYAPP, WARN_LOG, "socket[%d] recv len:%d", i, read_len);
			if (read_len < 0)
            {
                if (errno == EWOULDBLOCK) // time out
                {
                	// xy_free(buf);
					xy_free(remote_info);
                    continue;
                }
                else
                {
                    xy_printf(0,XYAPP, WARN_LOG, "socket[%d] read - BYTES,force to close socket", i);
                    del_socket_ctx_by_index(i, false);
					// xy_free(buf);
					xy_free(remote_info);
                    continue;
                }
            }
            else if (read_len == 0)
            {
                xy_printf(0,XYAPP, WARN_LOG, "socket[%d]read 0 BYTES,force to close socket", i);
                del_socket_ctx_by_index(i, false);
				// xy_free(buf);
				xy_free(remote_info);
				continue;
            }

            if (g_socket_ctx[i]->recv_ctl == 0)
            {
				xy_printf(0,XYAPP, WARN_LOG, "socket[%d]recv_ctl is 0, recv %d length downlink data!!!", i, read_len);
            	add_new_rcv_data_node(i, read_len, buf, remote_info);
				continue;
			}

            at_SOCKNMI_URC(i, read_len, buf, remote_info);
        }
    }
out:
    xy_free(buf);
    xy_deReg_psnetif_callback(EVENT_PSNETIF_INVALID, netif_down_close_socket);
    xy_printf(0,XYAPP, WARN_LOG, "socket recv thread exit!!!");
    g_at_socket_rcv_thd = NULL;
    osThreadExit();
}

void start_at_socket_recv_thread(void)
{
	if (g_at_socket_rcv_thd == NULL)
	{
		osThreadAttr_t thread_attr = {0};
		thread_attr.name = "sck_rcv";
		thread_attr.priority = osPriorityNormal1;
		thread_attr.stack_size = osStackShared;
		g_at_socket_rcv_thd = osThreadNew((osThreadFunc_t)(at_sock_recv_thread), NULL, &thread_attr);
	}
}

#endif //AT_SOCKET
