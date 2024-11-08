/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/

#if AT_SOCKET

#include "at_socket_context.h"
#include "at_com.h"
#include "at_socket.h"
#include "at_socket_passthr.h"
#include "factory_nv.h"
#include "lwip/api.h"
#include "lwip/apps/sntp.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "main_proxy.h"
#include "net_app_resume.h"
#include "at_socket_api.h"
#include "xy_at_api.h"
#include "xy_atc_interface.h"
#include "xy_flash.h"
#include "xy_net_api.h"
#include "xy_utils.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/		 	
#define SOCKET_SNINFO_LOCK()        osMutexAcquire(g_sninfo_mux, osWaitForever)
#define SOCKET_SNINFO_UNLOCK()      osMutexRelease(g_sninfo_mux)

/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/
uint8_t g_show_length_mode = 0;
uint8_t g_recv_data_viewmode = 0;
int g_at_sck_report_mode = BUFFER_WITH_HINT;

#if VER_BC25 || VER_260Y
uint8_t g_data_recv_mode = ASCII_STRING;
uint8_t g_data_send_mode = ASCII_STRING;
#else
uint8_t g_data_recv_mode = HEX_ASCII_STRING;
uint8_t g_data_send_mode = HEX_ASCII_STRING;
#endif

osThreadId_t g_at_socket_rcv_thd = NULL;
osMutexId_t g_socket_mux = NULL;
osMutexId_t g_sninfo_mux = NULL;
socket_context_t *g_socket_ctx[SOCK_NUM];
sock_sn_node_t sn_info = {0};
socket_udp_info_t *g_socket_udp_info = NULL;

/*******************************************************************************
 *						Global function implementations 					   *
 ******************************************************************************/
/**
 * @brief 设置socket sequence的默认状态，用于socket初始化
 */
void reset_socket_seq_state(uint8_t sock_id)
{
	SOCKET_CTX_LOCK();
	int i = 0;
	for (i = 0; i < SOCK_NUM; i++)
	{
		if (g_socket_ctx[i] != NULL && g_socket_ctx[i]->sock_id == sock_id)
		{
			int j = 0;
			for (j = 0; j < SEQUENCE_MAX; j++)
			{
				g_socket_ctx[i]->sequence_state[j] = SEND_STATUS_UNUSED;
			}
		}
	}
	SOCKET_CTX_UNLOCK();
}

bool is_socketId_valid(int socket_id)
{
	if (socket_id < 0 || socket_id >= SOCK_NUM)
		return false;
	SOCKET_CTX_LOCK();
	if (g_socket_ctx[socket_id] != NULL && g_socket_ctx[socket_id]->sock_id == socket_id)
	{
		SOCKET_CTX_UNLOCK();
		return true;
	}
	SOCKET_CTX_UNLOCK();
	return false;
}

/**
 * @brief 根据socket fd查找对应socket 上下文id
 * @return -1表示未找到对应socket上下文
 */
int get_socket_id_by_fd(int fd, uint8_t* id)
{
	SOCKET_CTX_LOCK();
	int i = 0;
	for (i = 0; i < SOCK_NUM; i++)
	{
		if (g_socket_ctx[i] != NULL && g_socket_ctx[i]->fd == fd)
		{
			SOCKET_CTX_UNLOCK();
			*id = i;
			return XY_OK;
		}
	}
	SOCKET_CTX_UNLOCK();
	return XY_ERR;
}

int get_socket_net_type(int id)
{
	if (!is_socketId_valid(id))
		return -1;
	else
		return g_socket_ctx[id]->net_type;
}

int get_socket_af_type(int id)
{
	if (!is_socketId_valid(id))
		return -1;
	else
		return g_socket_ctx[id]->af_type;
}

int get_socket_fd(int id)
{
	if (!is_socketId_valid(id))
		return -1;
	else
		return g_socket_ctx[id]->fd;
}

int get_socket_seq_state(int id, int seq)
{
	if (!is_socketId_valid(id) || seq <= 0 || seq > SEQUENCE_MAX)
		return -1;
	else
		return g_socket_ctx[id]->sequence_state[seq - 1];
}

int get_socket_pre_sn(int id, int sequence)
{
	int pre_sn = 0;
	if (sequence != 0)
	{
		ioctl(g_socket_ctx[id]->fd, FIOREADSN, &pre_sn);
	}
	return pre_sn;
}

void del_allsnnode_by_socketid(uint8_t id)
{
	SOCKET_SNINFO_LOCK();
    sock_sn_node_t *cur_nod, *pre;
	cur_nod = (&sn_info)->next;
    pre = &sn_info;

    while (cur_nod != NULL)
    {
		if (cur_nod->socket_id == id)
		{
			g_socket_ctx[id]->sequence_state[cur_nod->sequence - 1] = SEND_STATUS_FAILED;	
			at_SOCKSTR_URC(id, cur_nod->sequence, g_socket_ctx[id]->sequence_state[cur_nod->sequence - 1]);
            pre->next = cur_nod->next;
    		xy_free(cur_nod);
			cur_nod = pre->next;
			continue;
    	}
        pre = cur_nod;
        cur_nod = cur_nod->next;
    }
    SOCKET_SNINFO_UNLOCK();
}

/**
 * @brief 根据socket 上下文索引删除socket 上下文
 */
int del_socket_ctx_by_index(uint8_t idx, bool isquit)
{
	SOCKET_CTX_LOCK();
	if (g_socket_ctx[idx] != NULL)
	{
		xy_printf(0, PLATFORM, INFO_LOG, "[%s]del:%d,%d,%d\n", __FUNCTION__, idx, isquit, g_socket_ctx[idx]->state);
        uint8_t net_type = g_socket_ctx[idx]->net_type;
        close(g_socket_ctx[idx]->fd);
		struct rcv_data_nod *nod_next = NULL;
		struct rcv_data_nod *temp = g_socket_ctx[idx]->data_list;
		while (temp != NULL)
		{
			nod_next = temp->next;
			SOCKET_SAFE_FREE(temp->data);
			SOCKET_SAFE_FREE(temp->sockaddr_info);
			SOCKET_SAFE_FREE(temp);
			temp = nod_next;
		}
		if (g_socket_ctx[idx]->remote_ip != NULL)
			xy_free(g_socket_ctx[idx]->remote_ip);

		del_allsnnode_by_socketid(g_socket_ctx[idx]->sock_id);
		at_SOCKCLZ_URC(g_socket_ctx[idx]->sock_id, isquit);
		xy_free(g_socket_ctx[idx]);
        g_socket_ctx[idx] = NULL;
		update_socket_infos_to_fs(idx, false);
        SOCKET_CTX_UNLOCK();
        return XY_OK;
    }
    else
    {
        SOCKET_CTX_UNLOCK();
        return XY_ERR;
    }
}

int add_new_rcv_data_node(uint8_t skt_id, int rcv_len, char *buf, struct sockaddr_storage *sockaddr_info)
{
    int len_sum = 0;
	uint8_t node_num = 0;
    struct rcv_data_nod *temp;
    struct rcv_data_nod *rcv_nod = xy_malloc(sizeof(struct rcv_data_nod));
	rcv_nod->data = (char *)xy_malloc(rcv_len);
	memcpy(rcv_nod->data, buf, rcv_len);
    rcv_nod->len = rcv_len;
    rcv_nod->sockaddr_info = sockaddr_info;
    rcv_nod->next = NULL;

    SOCKET_CTX_LOCK();
    temp = g_socket_ctx[skt_id]->data_list;
    if (temp == NULL)
        g_socket_ctx[skt_id]->data_list = rcv_nod;
    else
    {
        while (temp->next != NULL)
        {
            len_sum += temp->len;
            temp = temp->next;
			node_num++;
        }
        len_sum += temp->len;
        if (len_sum + rcv_len < SOCK_RCV_BUF_MAX && node_num <= AT_SOCKET_RCV_NODE_MAX)
            temp->next = rcv_nod;
        else
        {
            /* 缓存超过4000且未取出或者节点个数大于AT_SOCKET_RCV_NODE_MAX,丢弃新收到的数据*/
			SOCKET_SAFE_FREE(rcv_nod->data);
            SOCKET_SAFE_FREE(rcv_nod);
			SOCKET_SAFE_FREE(sockaddr_info);
            // xy_free(buf);
            SOCKET_CTX_UNLOCK();
            return XY_ERR;
        }
    }
    SOCKET_CTX_UNLOCK();
    return XY_OK;
}

int get_socket_avail_id()
{
	SOCKET_CTX_LOCK();
	int i;
	for (i = 0; i < SOCK_NUM; i++)
	{
		if (g_socket_ctx[i] == NULL)
		{
			SOCKET_CTX_UNLOCK();
			return i;
		}
	}
	SOCKET_CTX_UNLOCK();
	return -1;
}

int is_all_socket_exit()
{
	SOCKET_CTX_LOCK();
	int i = 0;
	for (i = 0; i < SOCK_NUM; i++)
	{
		if (g_socket_ctx[i] != NULL)
		{
			SOCKET_CTX_UNLOCK();
			return 0;
		}
	}
	SOCKET_CTX_UNLOCK();
	return 1;
}

bool is_socket_need_backup()
{
    int i = 0;
    for (i = 0; i < SOCK_NUM; i++)
        if (g_socket_ctx[i] != NULL && g_socket_ctx[i]->net_type == 1)
            return true;
    return false;
}
void clear_socket_backup(void)
{
	if(!Is_WakeUp_From_Dsleep())
		cloud_remove_file(SOCKET_SESSION_NVM_FILE_NAME);
}

int update_socket_infos_to_fs(uint8_t sockid, bool add)
{
	if (g_socket_udp_info == NULL)
	{
		g_socket_udp_info = cloud_malloc(SOCKET_SESSION_NVM_FILE_NAME);
	}

	if (add)
	{
		if (g_socket_ctx[sockid] == NULL || g_socket_ctx[sockid]->net_type == 0 || g_socket_ctx[sockid]->quit == 1)
		{
			return XY_ERR;
		}

		g_socket_udp_info->udp_socket[sockid].local_port_ori = g_socket_ctx[sockid]->local_port_ori;
		g_socket_udp_info->udp_socket[sockid].remote_port = g_socket_ctx[sockid]->remote_port;
		g_socket_udp_info->udp_socket[sockid].local_port = g_socket_ctx[sockid]->local_port;
		g_socket_udp_info->udp_socket[sockid].recv_ctl = g_socket_ctx[sockid]->recv_ctl;
		g_socket_udp_info->udp_socket[sockid].access_mode = g_socket_ctx[sockid]->access_mode;
		g_socket_udp_info->udp_socket[sockid].socket_idx = (int8_t)(g_socket_ctx[sockid]->sock_id);
		g_socket_udp_info->udp_socket[sockid].af_type = g_socket_ctx[sockid]->af_type;
	    g_socket_udp_info->udp_socket[sockid].data_mode = (uint8_t)g_at_sck_report_mode;
		memcpy(g_socket_udp_info->udp_socket[sockid].ai_addr, g_socket_ctx[sockid]->ai_addr, 28);
	}
	else
	{
		memset(&g_socket_udp_info->udp_socket[sockid], 0, sizeof(socket_udp_context_t));
		g_socket_udp_info->udp_socket[sockid].socket_idx = (int8_t)-1;
	}

	if (cloud_save_file(SOCKET_SESSION_NVM_FILE_NAME, (void *)g_socket_udp_info, sizeof(socket_udp_info_t)) != XY_OK)
	{
		send_debug_by_at_uart("+DBGINFO:socket backup fail!!\r\n");
		return XY_ERR;
	}
	else
	{
		send_debug_by_at_uart("+DBGINFO:socket backup success!!\r\n");
		return XY_OK;
	}
}

int restore_socket_infos_from_fs()
{
    if (g_socket_udp_info == NULL)
    {
        g_socket_udp_info = cloud_malloc(SOCKET_SESSION_NVM_FILE_NAME);
    }

    return cloud_read_file(SOCKET_SESSION_NVM_FILE_NAME, g_socket_udp_info, sizeof(socket_udp_info_t));
}

void add_sninfo_node(uint8_t id, uint32_t len, uint8_t sequence, uint32_t pre_sn)
{
	if (!is_socketId_valid(id))
	{
		xy_assert(0);
	}
	sock_sn_node_t *cur_head, *pre, *node;

	SOCKET_SNINFO_LOCK();
	cur_head = &sn_info;
    pre = cur_head;
    while (cur_head != NULL)
    {
        pre = cur_head;
        cur_head = cur_head->next;
    }
	node = xy_malloc(sizeof(sock_sn_node_t));
	memset(node, 0, sizeof(sock_sn_node_t));
	node->data_len = len;
	node->per_sn = pre_sn;
	node->sequence = sequence;
	node->socket_id = id;
	node->net_type = g_socket_ctx[id]->net_type;
	pre->next = node;
	SOCKET_SNINFO_UNLOCK();
}

void del_sninfo_node(uint8_t id, uint8_t sequence)
{
	SOCKET_SNINFO_LOCK();
	sock_sn_node_t *cur_node, *pre;
	cur_node = (&sn_info)->next;
    pre = &sn_info;

    while (cur_node != NULL)
    {
		if (cur_node->socket_id == id && cur_node->sequence == sequence)
		{
            pre->next = cur_node->next;
    		xy_free(cur_node);
			SOCKET_SNINFO_UNLOCK();
    		return;
    	}
        pre = cur_node;
        cur_node = cur_node->next;
    }
	SOCKET_SNINFO_UNLOCK();
}

int find_match_udp_node(uint8_t id, uint8_t sequence)
{
	SOCKET_SNINFO_LOCK();
	sock_sn_node_t *cur_node = (&sn_info)->next;

    while (cur_node != NULL)
    {
		if (cur_node->net_type == 1 && cur_node->socket_id == id && cur_node->sequence == sequence)
		{
			SOCKET_SNINFO_UNLOCK();
			return 1;
		}
		cur_node = cur_node->next;
    }
	SOCKET_SNINFO_UNLOCK();
	return 0;
}

static int find_match_tcp_node(uint8_t ctx_id, uint16_t ack_len)
{
	SOCKET_SNINFO_LOCK();
	sock_sn_node_t *cur_node = (&sn_info)->next;
	sock_sn_node_t* pre_node = &sn_info;
    uint8_t socket_id = g_socket_ctx[ctx_id]->sock_id;

    while (cur_node != NULL)
    {
        if (cur_node->net_type == 0 && cur_node->socket_id == socket_id)
        {
            if (ack_len == cur_node->data_len)
            {
               uint8_t sequence = cur_node->sequence;
               g_socket_ctx[ctx_id]->sequence_state[sequence - 1] = SEND_STATUS_SUCCESS;
			   g_socket_ctx[ctx_id]->acked_size += ack_len;
			   pre_node->next = cur_node->next;
			   xy_free(cur_node);
			   at_SOCKSTR_URC(socket_id, sequence, g_socket_ctx[ctx_id]->sequence_state[sequence - 1]);
			   SOCKET_SNINFO_UNLOCK();
               return XY_OK;
            }
            /* nagle算法生效或者tcp unsent数据长度大于发送窗口时，可能出现粘包 */
            else if (ack_len > cur_node->data_len)
            {
                uint8_t sequence = cur_node->sequence;
                g_socket_ctx[ctx_id]->sequence_state[sequence - 1] = SEND_STATUS_SUCCESS;
				g_socket_ctx[ctx_id]->acked_size += cur_node->data_len;
                ack_len -= cur_node->data_len;
				pre_node->next = cur_node->next;
				xy_free(cur_node);
                cur_node = pre_node->next;
				at_SOCKSTR_URC(socket_id, sequence, g_socket_ctx[ctx_id]->sequence_state[sequence - 1]);
                continue;
            }
            else if (ack_len < cur_node->data_len)
            {
                cur_node->data_len -= ack_len;
				SOCKET_SNINFO_UNLOCK();
				return XY_OK;
            }
        }
        cur_node = cur_node->next;
    }
	SOCKET_SNINFO_UNLOCK();
	return XY_OK;
}

int check_socket_seq(uint8_t id, uint8_t sequence_num)
{
	if (sequence_num == 0)
		return 0;

	SOCKET_SNINFO_LOCK();
	sock_sn_node_t *cur_node = (&sn_info)->next;

	while (cur_node != NULL)
	{
		if (cur_node->net_type == g_socket_ctx[id]->net_type && cur_node->socket_id == id && cur_node->sequence == sequence_num)
		{
			SOCKET_SNINFO_UNLOCK();
			return 1;
		}
		cur_node = cur_node->next;
	}
	SOCKET_SNINFO_UNLOCK();
	return 0;
}

void netconn_ack_notify(void *arg, unsigned long ackno, unsigned short ack_len)
{
	struct netconn *conn = (struct netconn *)arg;

	tcp_ack_info_t info = {0};
	info.socket = (int16_t)conn->socket;
	info.ack_no = ackno;
    info.ack_len = ack_len;
	send_msg_2_proxy(PROXY_MSG_TCP_ACK, &info, sizeof(tcp_ack_info_t));
}

int proc_tcp_ack(int fd, uint32_t ackno, uint16_t ack_len)
{
	uint8_t id = 0;

	xy_printf(0,XYAPP, WARN_LOG, "proc_tcp_ack socket fd:%d, ackno:%d, ack_len:%d", fd, ackno, ack_len);

	if (get_socket_id_by_fd(fd, &id) != XY_OK)
	{
		xy_printf(0,XYAPP, WARN_LOG, "tcpack can not match conn_socket");
		return XY_ERR;
	}
#if VER_BC25 || VER_260Y
	g_socket_ctx[id]->acked_size += ack_len;
#else
	find_match_tcp_node(id, ack_len);
#endif
	return XY_OK;
}

int proc_tcp_accept(int fd)
{
	extern void socket_accept(int sock_fd);
	socket_accept(fd);

	return XY_OK;
}

void sockaddr_to_string(struct sockaddr  *ai_addr, char *ip_addr, size_t len)
{
	if (ai_addr->sa_family == AF_INET)
	{
		struct sockaddr_in *addr = (struct sockaddr_in *)ai_addr;
		inet_ntop(AF_INET, &addr->sin_addr, ip_addr, len);
	}
	else if (ai_addr->sa_family == AF_INET6)
	{
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)ai_addr;
		inet_ntop(AF_INET6, &addr6->sin6_addr, ip_addr, len);
	}
}

int resume_udp_socket(socket_create_param_t* arg)
{
    xy_assert(arg != NULL);

    int sock_fd = -1;
    int sock_id = arg->id;
    int af_type;
    struct sockaddr_storage bind_addr = {0};

    if (sock_id < 0)
    {
        return -1;
    }

    if (arg->proto != IPPROTO_UDP)
    {
        return -1;
    }
	
	af_type = (arg->af_type == 1) ? AF_INET6 : AF_INET;
	
	xy_printf(0, XYAPP, WARN_LOG, "socket_resume create,ai_family:%d,ai_socktype:%d,ai_protocol:%d", af_type, SOCK_DGRAM, IPPROTO_UDP);
    sock_fd = socket(af_type, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_fd == -1)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[socket create]create err:%d", errno);
        return -1;
    }

    /* 设置bind addr prop */
    if (arg->af_type == 1)
    {
        ((struct sockaddr_in6 *)&bind_addr)->sin6_family = AF_INET6;
        ((struct sockaddr_in6 *)&bind_addr)->sin6_port = htons(arg->local_port);
        if (arg->local_ip != NULL)
        {
            if (inet_pton(AF_INET6, arg->local_ip, &(((struct sockaddr_in6 *)&bind_addr)->sin6_addr)) == -1)
            {
                xy_printf(0,XYAPP, WARN_LOG, "[sock bind]ipv6 inet_pton fail");
                close(sock_fd);
                return -1;
            }
        }
    }
    else
    {
        ((struct sockaddr_in *)&bind_addr)->sin_family = AF_INET;
        ((struct sockaddr_in *)&bind_addr)->sin_port = htons(arg->local_port);
        if (arg->local_ip != NULL)
        {
            if (inet_pton(AF_INET, arg->local_ip, &(((struct sockaddr_in *)&bind_addr)->sin_addr)) == -1)
            {
                xy_printf(0,XYAPP, WARN_LOG, "[sock bind]ipv4 inet_pton fail");
                close(sock_fd);
                return -1;
            }
        }
    }
	xy_printf(0, XYAPP, WARN_LOG, "socket_resume band,fd:%d,band addr:0x%x,band addr len:%d", sock_fd, bind_addr, sizeof(bind_addr));
    /* do bind */
    if (bind(sock_fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) == -1)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[sock bind]bind err:%d", errno);
        close(sock_fd);
        return -1;
    }

    /* 初始化socket context上下文 */
    socket_context_t *socket_ctx = xy_malloc(sizeof(socket_context_t));
	memset(socket_ctx, 0, sizeof(socket_context_t));

	socket_ctx->fd             = sock_fd;
	socket_ctx->cid            = arg->cid;
	socket_ctx->recv_ctl       = 1;
	socket_ctx->sock_id        = sock_id;
	socket_ctx->service_type   = SOCKET_UDP;
	socket_ctx->access_mode    = arg->access_mode;
    socket_ctx->af_type        = arg->af_type;
	socket_ctx->remote_port    = arg->remote_port;
	socket_ctx->local_port     = arg->local_port_ori;
    socket_ctx->local_port_ori = arg->local_port_ori;
    socket_ctx->net_type       = 1;
#if VER_BC25 || VER_260Y
	memcpy(socket_ctx->ai_addr, arg->remote_ip, 28);
	socket_ctx->remote_ip = xy_malloc(XY_IPADDR_STRLEN_MAX);
	sockaddr_to_string(socket_ctx->ai_addr, socket_ctx->remote_ip, XY_IPADDR_STRLEN_MAX);
#endif
    /* local port为0情况下, bind完成后需要获取lwip内部分配的local port */
    xy_socket_local_info(sock_fd, NULL, &socket_ctx->local_port);

	SOCKET_CTX_LOCK();
    g_socket_ctx[sock_id] = socket_ctx;
	SOCKET_CTX_UNLOCK();
    reset_socket_seq_state(sock_id);
	
    //udp set socket recv/send non-block
    socket_setprop_nonblock(get_socket_fd(sock_id));
#if VER_BC25 || VER_260Y
	xy_printf(0, XYAPP, WARN_LOG, "socket_resume connect,fd:%d,ai addr:0x%x,ai addr len:%d", sock_fd, socket_ctx->ai_addr, 28);
	// TODO：UDP做connect的情况下，不适用于服务器回复包所带的IP地址发生变化的情况
	if (connect(sock_fd, socket_ctx->ai_addr, 28) == -1)
	{
		xy_printf(0, PLATFORM, INFO_LOG, "UDP resume connect fail, addr:%s", socket_ctx->remote_ip);
		xy_printf(0, XYAPP, WARN_LOG, "socket_open_task sock connect err:%d", errno);
		close(sock_fd);
        del_socket_ctx_by_index(sock_id, 0);
		return -1;
	}
	xy_printf(0, PLATFORM, INFO_LOG, "UDP resume connect success, addr:%s", socket_ctx->remote_ip);
	socket_ctx->state = SOCKET_STATE_CONNECTED;
#endif
    return 0;
}

void resume_socket_app()
{
    static uint8_t g_backup_udp_once = 0;
    if (g_at_socket_rcv_thd != NULL)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[resume_socket_app] fail--rcv thread is running!");
        goto END;
    }
    if (g_socket_udp_info == NULL || *(unsigned int *)g_socket_udp_info == 0xFFFFFFFF)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[resume_socket_app] fail--socket udp info is null");
        goto END;
    }

    if (g_backup_udp_once == 0)
    {
        int res = -1;
        int fd = -1;
        int idx = 0;
        int i = 0;
        socket_create_param_t *sock_param[SOCK_NUM] = {NULL};

        for (i = 0; i < SOCK_NUM; i++)
        {
			idx = g_socket_udp_info->udp_socket[i].socket_idx;
			xy_assert(idx < SOCK_NUM);

			if (idx == (int8_t)-1 || g_socket_ctx[idx] != NULL|| g_socket_udp_info->udp_socket[i].local_port == 0)
			{
				xy_printf(0,XYAPP, WARN_LOG, "socket resume debug:%d", i);
				continue;
			}
            xy_printf(0,XYAPP, WARN_LOG, "[resume_socket_app] udp_socket[%d] local port:%d", i, g_socket_udp_info->udp_socket[i].local_port);
            sock_param[i] = xy_malloc(sizeof(socket_create_param_t));
			memset(sock_param[i], 0, sizeof(socket_create_param_t));

            sock_param[i]->id = idx;
			sock_param[i]->local_port_ori = g_socket_udp_info->udp_socket[i].local_port_ori;
            sock_param[i]->local_port = g_socket_udp_info->udp_socket[i].local_port;
			sock_param[i]->remote_port = g_socket_udp_info->udp_socket[i].remote_port;
            sock_param[i]->proto = IPPROTO_UDP;
            sock_param[i]->af_type = g_socket_udp_info->udp_socket[i].af_type;
            sock_param[i]->recv_ctrl = g_socket_udp_info->udp_socket[i].recv_ctl;
			sock_param[i]->access_mode = g_socket_udp_info->udp_socket[i].access_mode;
#if VER_BC25 || VER_260Y
			sock_param[i]->remote_ip = xy_malloc(28);
			memcpy(sock_param[i]->remote_ip, g_socket_udp_info->udp_socket[i].ai_addr, 28);
#endif
			g_at_sck_report_mode = g_socket_udp_info->udp_socket[i].data_mode;

			if ((resume_udp_socket(sock_param[i])) != 0)
			{
				xy_printf(0, XYAPP, WARN_LOG, "[resume_socket_app] udp_nack_err:%d,g_socket_ctx[%d] create fail!", res, i);
			}
#if VER_BC25 || VER_260Y
			xy_free(sock_param[i]->remote_ip);
#endif
			xy_free(sock_param[i]);
		}

        if (is_all_socket_exit())
            goto END;
		
		/* 启动socket recv线程 */
		start_at_socket_recv_thread();
    }
END:
    g_backup_udp_once = 1;
    return;
}

//when DRX/eDRX period,user may start other new cloud connect,so must care conflict
bool socket_resume(void)
{
	static bool sock_resume = false;
	if (sock_resume || Is_WakeUp_From_Dsleep() == 0)
		return true;
	sock_resume = true;

    /* 从文件系统中读取socket上下文 */
    if (restore_socket_infos_from_fs() == XY_OK)
	{
		/* 恢复udp socket上下文 */
		resume_socket_app();
		send_debug_by_at_uart("+DBGINFO:[SOCKET] RECOVERY\r\n");
		return true;
	}

    return false;
}

/**
 * @brief 检测新创建的socket有效性
 * @param  flag  0:检测主机端口号和网络类型是否与socket上下文记录重复;1:检测服务端口/地址/网络类型是否与socket上下文记录重复
 * @return XY_OK 表示新建socket有效；XY_ERR表示新建socket与已知socket发生冲突
 */
int check_socket_valid(uint8_t net_type, uint16_t remote_port, char *remote_ip, uint16_t local_port, int flag)
{
	SOCKET_CTX_LOCK();
	int i;
	if (flag)
	{
		for (i = 0; i < SOCK_NUM; i++)
		{
			if (g_socket_ctx[i] != NULL)
			{
				if (g_socket_ctx[i]->net_type == net_type && g_socket_ctx[i]->remote_port == remote_port && !strcmp(remote_ip, g_socket_ctx[i]->remote_ip))
				{
					SOCKET_CTX_UNLOCK();
					return XY_ERR;
				}
			}
		}
	}
	else
	{
		for (i = 0; i < SOCK_NUM; i++)
		{
			if (g_socket_ctx[i] != NULL)
			{
				if (g_socket_ctx[i]->net_type == net_type && g_socket_ctx[i]->local_port == local_port && local_port != 0)
				{
					SOCKET_CTX_UNLOCK();
					return XY_ERR;
				}
			}
		}
	}
	SOCKET_CTX_UNLOCK();
	return XY_OK;
}

bool check_socket_sendlen(uint32_t str_data_size, uint32_t data_len)
{
    if (data_len > AT_SOCKET_MAX_DATA_LEN || data_len == 0)
    {
        return false;
    }

    if (g_data_send_mode == HEX_ASCII_STRING && 2 * data_len != str_data_size)
    {
        return false;
    }

    if (g_data_send_mode == ASCII_STRING && data_len != str_data_size)
    {
        return false;
    }

    return true;
}

//+IPSEQUENCE:<seq>,<status>
void urc_UDPSN_Callback(unsigned long eventId, void *param, int paramLen)
{
	UNUSED_ARG(eventId);
	xy_assert(paramLen == sizeof(ATC_MSG_IPSN_IND_STRU));
	ATC_MSG_IPSN_IND_STRU *ipsn_urc = (ATC_MSG_IPSN_IND_STRU*)param;
	unsigned short seq_soc_num = 0;
	char *sequence_rsp = NULL;
	char send_status = 0;
	unsigned short socket_fd = 0;
	uint8_t id = 0;
	unsigned short seq_num = 0;

	seq_soc_num = ipsn_urc->usIpSn;
	send_status = ipsn_urc->ucStatus;

	socket_fd = (unsigned short)((seq_soc_num & 0XFF00) >> 8);
	seq_num = (unsigned short)(seq_soc_num & 0X00FF);
    xy_printf(0, XYAPP, WARN_LOG, "socket_fd:%d, seq_num:%d, status:%d", socket_fd, seq_num, send_status);

    //socket context proc
	if (get_socket_id_by_fd(socket_fd, &id) != XY_OK)
		return;
	if (seq_num > SEQUENCE_MAX || seq_num <= 0)
	{
		xy_printf(0,XYAPP, WARN_LOG, "UDPSEQUENCE ERR!!!");
		return;
	}

	if (find_match_udp_node(id, (unsigned char)seq_num) == 0)
	{
		xy_printf(0,XYAPP, WARN_LOG, "find no match udp mode!!!");
		return;
	}

	g_socket_ctx[id]->sequence_state[seq_num - 1] = send_status;
	del_sninfo_node(id, seq_num);
	at_SOCKSTR_URC(id, seq_num, g_socket_ctx[id]->sequence_state[seq_num - 1]);
}

void at_socket_init()
{
	osMutexAttr_t mutex_attr = {0};
	mutex_attr.attr_bits = osMutexRecursive;
	g_socket_mux = osMutexNew(&mutex_attr);
	g_sninfo_mux = osMutexNew(NULL);
	xy_atc_registerPSEventCallback(D_XY_PS_REG_EVENT_IPSN, urc_UDPSN_Callback);
	regist_socket_callback();
#if VER_BC25 || VER_260Y
	extern void socket_config_init(void);
	socket_config_init();
#endif
}

#endif //AT_SOCKET