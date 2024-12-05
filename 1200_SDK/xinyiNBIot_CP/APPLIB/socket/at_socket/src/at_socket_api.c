/**
 * @file socket_api.c
 * @brief 
 * @version 1.0
 * @date 2022-06-14
 * @copyright Copyright (c) 2022  芯翼信息科技有限公司
 * 
 */
#include "at_socket.h"
#include "at_socket_api.h"
#include "xy_at_api.h"
#include "at_socket_context.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "at_socket_passthr.h"
#include "xy_passthrough.h"
#include "xy_socket_api.h"
#include "at_tcpip_api.h"
#include "softap_nv.h"


typedef struct socket_err_map
{
    int lwip_socket_err;
    int socket_err;
} socket_err_map_t;

const socket_err_map_t socket_err_table[] = {
    {ENOMEM, XY_Err_NoMemory},			/* Out of memory error.	  	*/
    {ENOBUFS, XY_Err_NoMemory},			/* Buffer error.			*/
	{EWOULDBLOCK, XY_Err_Timeout},		/* Timeout				  	*/
    {EHOSTUNREACH, XY_Err_DnsFail},		/* Routing problem.		  	*/
	{EINPROGRESS, XY_Err_InProgress},	/* Operation in progress	*/
    {EINVAL, XY_Err_Parameter},			/* Illegal value. 		  	*/
    {EADDRINUSE, XY_Err_AddrInUse},		/* Address in use.		 	*/
	{EALREADY, XY_Err_NotAllowed},		/* Already connecting.	  	*/
    {EISCONN, XY_Err_NotAllowed},		/* Conn already established.*/
	{ENOTCONN, XY_Err_SockNoConn},		/* Not connected. 		  	*/
	{-1, XY_Err_SockNoConn},			/* Low-level netif error	*/
	{ECONNABORTED, XY_Err_SockNoConn},	/* Connection aborted.	  	*/
	{ECONNRESET, XY_Err_SockNoConn},	/* Connection reset.		*/
	{EIO, XY_Err_Parameter},			/* Illegal argument.		*/
	{EOOS, XY_Err_NoConnected},			/* +CGEV:OOS				*/
	{EFLOWCTL, XY_Err_NotAllowed},			/* +CGEV:OOS				*/
};

osTimerId_t g_at_sock_tmr = NULL;
static void socket_open_task(socket_create_param_t *arg)
{
	char prsp[64] = {0};
	int ret = socket_open(arg);
     
    snprintf(prsp, 64, "\r\n+QIOPEN: %d,%d\r\n", arg->id, ret);
    send_urc_to_ext(prsp, strlen(prsp));

	if (osTimerIsRunning(g_at_sock_tmr))
		osTimerStop(g_at_sock_tmr);

	xy_free(arg->remote_ip);
    xy_free(arg);
    osThreadExit();
}

void at_sock_timeout_cb(void)
{
	uint8_t sock_id = *((uint8_t *)osTimerGetCallbackArgs(g_at_sock_tmr));
	socket_close(sock_id);
}

void socket_open_async(socket_create_param_t *arg)
{
	socket_create_param_t *open_arg = (socket_create_param_t *)xy_malloc(sizeof(socket_create_param_t));
	memcpy(open_arg, arg, sizeof(socket_create_param_t));
	open_arg->remote_ip = xy_malloc(strlen(arg->remote_ip) + 1);
	strcpy(open_arg->remote_ip, arg->remote_ip);

	if (g_at_sock_tmr == NULL)
	{
		osTimerAttr_t timer_attr = {0};
		timer_attr.name = "at_sock_tmr";
		g_at_sock_tmr = osTimerNew((osTimerFunc_t)at_sock_timeout_cb, osTimerOnce, &open_arg->id, &timer_attr);
	}

#if VER_260Y
	osTimerStart(g_at_sock_tmr, 60 * 1000);
#elif VER_BC25
	osTimerStart(g_at_sock_tmr, ((140 + g_softap_var_nv->sock_open_time) * 1000));
#endif

	osThreadAttr_t thread_attr = {0};
    thread_attr.name = "sck_open";
    thread_attr.priority = osPriorityNormal1;
    thread_attr.stack_size = osStackShared;
	osThreadNew((osThreadFunc_t)(socket_open_task), open_arg, &thread_attr);
}

int socket_get_status_info(int sock_id, char *prsp_cmd)
{	
    socket_context_t *ctx = g_socket_ctx[sock_id];
    if (ctx == NULL)
        return 0;
	
	int str_len;
	// 存储service type转换后的字符串
	char service_type_str[16] = {0};

	if (ctx->service_type == SOCKET_TCP)
		snprintf(service_type_str, 16, "TCP");
	else if (ctx->service_type == SOCKET_UDP)
		snprintf(service_type_str, 16, "UDP");
	else if (ctx->service_type == SOCKET_TCP_LISTENER)
		snprintf(service_type_str, 16, "TCP LISTENER");
	else
        return 0;

	//+QISTATE: <connectID>,<service_type>,<IP_address>,<remote_port>,<local_port>,<socket_state>,<contextID>,<serverID>,<access_mode>,<AT_port>
	str_len = sprintf(prsp_cmd, "\r\n+QISTATE: %d,\"%s\",\"%s\",%d,%d,%d,%d,%d", sock_id, service_type_str, ctx->remote_ip, 
						ctx->remote_port, ctx->local_port_ori, ctx->state, ctx->cid, ctx->access_mode);

	return str_len;   
}

// 每个PDP场景下都可建立多个socket context上下文
int socket_get_status_info_by_cid(int cid, char *prsp_cmd)
{	
	int str_len = 0;

	for (int sock_id = 0; sock_id < SOCK_NUM; sock_id++)
	{
		if (g_socket_ctx[sock_id] != NULL && (g_socket_ctx[sock_id]->cid == cid))
			str_len += socket_get_status_info(sock_id, prsp_cmd + str_len);
	}

	return str_len;
}

int socket_get_all_status_info(char *prsp_cmd)
{	
	int str_len = 0;

	for (int sock_id = 0; sock_id < SOCK_NUM; sock_id++)
	{
		if (g_socket_ctx[sock_id] != NULL)
			str_len += socket_get_status_info(sock_id, prsp_cmd + str_len);
	}

	return str_len;
}

int covert_to_socket_errcode(int lwip_socket_err)
{
    int i = 0;
    for (i = 0; i < (sizeof(socket_err_table) / sizeof(socket_err_map_t)); i++)
    {
        if (socket_err_table[i].lwip_socket_err == lwip_socket_err)
            return socket_err_table[i].socket_err;
    }
    return XY_ERR;
}

int socket_open(socket_create_param_t *arg)
{
    if (g_socket_ctx[arg->id] != NULL)
        return TCPIP_Err_SockInuse;

    int fd = -1, ret = TCPIP_OP_OK;
    struct addrinfo hint = {0};
    struct addrinfo *result = NULL;
    struct sockaddr_storage bind_addr = {0};    

    if (arg->service_type == SOCKET_UDP)
    {
        hint.ai_protocol = IPPROTO_UDP;
        hint.ai_socktype = SOCK_DGRAM;
    }
    else
    {
        hint.ai_protocol = IPPROTO_TCP;
        hint.ai_socktype = SOCK_STREAM;
    }

    hint.ai_family = (arg->af_type == 1) ? AF_INET6 : AF_UNSPEC;
    uint8_t strPort[7] = {0};
    snprintf(strPort, sizeof(strPort) - 1, "%d", arg->remote_port);
    if (getaddrinfo(arg->remote_ip, strPort, &hint, &result) != 0)
        return TCPIP_Err_DnsFail;

	if (result->ai_family == AF_INET)
	{
		((struct sockaddr_in *)&bind_addr)->sin_family = AF_INET;
		((struct sockaddr_in *)&bind_addr)->sin_port = htons(arg->local_port);
        if (arg->service_type == SOCKET_TCP_LISTENER)
        {
            ip4_addr_t ip4addr = {0};
            inet_aton(arg->remote_ip, &ip4addr);
		    inet_addr_from_ip4addr(&(((struct sockaddr_in *)&bind_addr)->sin_addr), &ip4addr);            
        }        	
	}
	else if (result->ai_family == AF_INET6)
	{
		((struct sockaddr_in6 *)&bind_addr)->sin6_family = AF_INET6;
		((struct sockaddr_in6 *)&bind_addr)->sin6_port = htons(arg->local_port);
        if (arg->service_type == SOCKET_TCP_LISTENER)
        {
            ip6_addr_t ip6addr = {0};
            inet6_aton(arg->remote_ip, &ip6addr);
		    inet6_addr_from_ip6addr(&(((struct sockaddr_in6 *)&bind_addr)->sin6_addr), &ip6addr);
        }        
	}
	else
	{
		xy_printf(0, XYAPP, WARN_LOG, "[%s]ai_family err:%d,%d", __FUNCTION__, arg->id, result->ai_family);
        ret = TCPIP_Err_DnsFail;
		goto EXIT;
	}

	xy_printf(0, XYAPP, WARN_LOG, "socket_open_task create,ai_family:%d,ai_socktype:%d,ai_protocol:%d", result->ai_family, result->ai_socktype, result->ai_protocol);
    /* 创建socket */
    if ((fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) == -1)
    {
		xy_printf(0, XYAPP, WARN_LOG, "socket_open_task create err:%d", errno);
        ret = TCPIP_Err_SockCreate;
		goto EXIT;
    }
	xy_printf(0, XYAPP, WARN_LOG, "socket_open_task band,fd:%d,band addr:0x%x,band addr len:%d", fd, bind_addr, sizeof(bind_addr));
    /* 绑定socket */
    if (bind(fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) == -1)
    {
        xy_printf(0, XYAPP, WARN_LOG, "socket_open_task sock bind errno:%d", errno);
        ret = TCPIP_Err_SockBind;
        goto EXIT;
    }

    socket_context_t *ctx = xy_malloc(sizeof(socket_context_t));
    memset(ctx, 0, sizeof(socket_context_t));
    ctx->fd             = fd;
	ctx->cid            = arg->cid;
    ctx->recv_ctl       = 1;
    ctx->sock_id        = arg->id;
    ctx->service_type   = arg->service_type;
    ctx->access_mode    = arg->access_mode;
	ctx->af_type        = (result->ai_family == AF_INET6) ? 1 : 0;
	ctx->remote_port    = arg->remote_port;
    ctx->local_port     = arg->local_port;
    ctx->local_port_ori = arg->local_port;
    ctx->net_type       = (arg->service_type == SOCKET_UDP) ? 1 : 0;
    ctx->remote_ip = xy_malloc(strlen(arg->remote_ip) + 1);
    strcpy(ctx->remote_ip, arg->remote_ip);

    memcpy(ctx->ai_addr, result->ai_addr, 28);
    reset_socket_seq_state(arg->id);
	xy_printf(0, XYAPP, WARN_LOG, "\r\nsocket open fd:%d,remote ip:%s,remote port:%d\r\n", ctx->fd, ctx->remote_ip, ctx->remote_port);
    /* bind时如果local port为0，lwip会随机为local分配一个port，此处获取一下pcb中实际记录的local port */
    /* local port为0情况下, bind完成后需要获取lwip内部分配的local port */
    xy_socket_local_info(fd, NULL, &ctx->local_port); 
    ctx->state = SOCKET_STATE_CREATED;
	SOCKET_CTX_LOCK();
    g_socket_ctx[arg->id] = ctx;
	SOCKET_CTX_UNLOCK();

    if (arg->service_type == SOCKET_UDP)
	    socket_setprop_nonblock(ctx->fd);
    ctx->state = SOCKET_STATE_CONNECTING;

	xy_printf(0, PLATFORM, INFO_LOG, "socket open with addr:%s,%s,%s", ipaddr_ntoa(result->ai_addr), ip4addr_ntoa(result->ai_addr), ip6addr_ntoa(result->ai_addr));
	// 本地Socket作为客户端时，需要做connect
	if(arg->service_type == SOCKET_TCP || arg->service_type == SOCKET_UDP)
	{
		xy_printf(0, XYAPP, WARN_LOG, "socket_open_task connect,fd:%d,ai addr:0x%x,ai addr len:%d", fd, result->ai_addr, result->ai_addrlen);
		// TODO：UDP做connect的情况下，不适用于服务器回复包所带的IP地址发生变化的情况
		if (connect(fd, result->ai_addr, result->ai_addrlen) == -1)
		{
			ret = TCPIP_Err_SockConnect;
			xy_printf(0, XYAPP, WARN_LOG, "socket_open_task sock connect err:%d", errno);
			goto EXIT;
		}
		ctx->state = SOCKET_STATE_CONNECTED;	
	}
	else if (arg->service_type == SOCKET_TCP_LISTENER)
	{
		if (listen(fd, SOCK_NUM - 1) != 0)
		{
			ret = TCPIP_Err_SockListen;
			xy_printf(0, PLATFORM_AP, WARN_LOG, "socket_open_task listen err:%d,%d", arg->id, errno);
			goto EXIT;
		}

		ctx->state = SOCKET_STATE_CONNECTED;
	}
	else
	{
		ret = XY_Err_Parameter;
		xy_printf(0, PLATFORM_AP, WARN_LOG, "socket_open_task service type err:%d,%d", arg->id, arg->service_type);
		goto EXIT;		
	}
    /* 保存udp socket上下文到文件系统,用于edrx/drx下行数据云恢复 */
    update_socket_infos_to_fs(arg->id, true);
    /* 建立socket数据收发线程 */
    start_at_socket_recv_thread();
	// 若监测到PDP去激活，释放socket相关资源
	// reg_data_call_status_cb(arg->cid, EVENT_PS_INVALID, netif_down_close_socket);

EXIT:
    freeaddrinfo(result);
    if (ret != TCPIP_OP_OK)
    {
        close(fd);
        del_socket_ctx_by_index(arg->id, 0);
    }
        
    return ret;
}

// EC600N对标，从最大ID号开始遍历，一般使用时从最小的开始使用
int get_socket_unused_id(void)
{
	SOCKET_CTX_LOCK();
	for (int i = SOCK_NUM - 1; i >= 0; --i)
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


int socket_accept(int sock_fd)
{
	uint8_t serverID;
    
	if (get_socket_id_by_fd(sock_fd, &serverID) == 0)
    {
        xy_printf(0, XYAPP, WARN_LOG, "[%s]don't find ctx:%d", __FUNCTION__, sock_fd);
        return TCPIP_Err_SockClosed;
    }
		
	struct sockaddr_storage client_addr;
	int client_addr_len = sizeof(struct sockaddr_storage);

	if ((sock_fd = accept(sock_fd, &client_addr, &client_addr_len)) == -1)
    {
        xy_printf(0, XYAPP, WARN_LOG, "[%s]accept fail:%d,%d,%d", __FUNCTION__, sock_fd, errno, serverID);
        return TCPIP_Err_SockAccept;
    }
		
	int sock_id = get_socket_unused_id(); 
	// socket服务ID已经使用完
	if (sock_id == -1)
    {
        close(sock_fd);
#if VER_BC25
		at_SOCKINCOMEFULL_URC();
#endif
		xy_printf(0, XYAPP, WARN_LOG, "[%s]not can use id:%d,%d", __FUNCTION__, sock_fd, serverID);
        return TCPIP_Err_SockAccept;  
    }
	
	socket_context_t *socket_ctx = xy_malloc(sizeof(socket_context_t));
	memset(socket_ctx, 0, sizeof(socket_context_t));			
	// 如果<service_type>是"TCP  LISTENER"，那么模块作为TCP服务器使用。接受到一个新的TCP连接后，新建TCP服务类型为“TCP INCOMING”
	socket_ctx->service_type    = SOCKET_TCP_INCOMING;
	socket_ctx->net_type        = IPPROTO_TCP; // TCP
	socket_ctx->af_type         = g_socket_ctx[serverID]->af_type;
	socket_ctx->fd              = sock_fd;
	socket_ctx->sock_id         = (uint8_t)sock_id;
	socket_ctx->cid             = g_socket_ctx[serverID]->cid;
	socket_ctx->access_mode     = g_socket_ctx[serverID]->access_mode;
	socket_ctx->local_port_ori  = g_socket_ctx[serverID]->local_port_ori;
	socket_ctx->remote_ip = xy_malloc(XY_IPADDR_STRLEN_MAX);
    inet_ntop(AF_INET, &(((struct sockaddr_in *)&client_addr)->sin_addr), socket_ctx->remote_ip, XY_IPADDR_STRLEN_MAX);
	socket_ctx->remote_port = ntohs(((struct sockaddr_in *)&client_addr)->sin_port);    

	// local port为0情况下, bind完成后需要获取lwip内部分配的local port
	xy_socket_local_info(sock_fd, NULL, &socket_ctx->local_port);
	
	SOCKET_CTX_LOCK();
	g_socket_ctx[sock_id] = socket_ctx;	
	socket_ctx->state = SOCKET_STATE_CONNECTED;
	SOCKET_CTX_UNLOCK();

#if VER_BC25
	// report incoming urc
	at_SOCKINCOMING_URC(sock_id, serverID, socket_ctx->remote_ip, socket_ctx->remote_port);
#endif

    return TCPIP_OP_OK;
}

///-------------------------处理超时,异步消息------------------------------------
osThreadId_t g_socket_timeout_handle = NULL;
osMessageQueueId_t g_socket_timeout_q = NULL;

osTimerId_t socket_waiting_timer = NULL;
SOCK_TIMEOUT_STATUS g_socket_timeout_status;
int                 g_socket_id;
int                 g_sequence;

extern void http_ctl_timeout_callback(int socket_id);
//extern void mqtt_ctl_timeout_callback(int socket_id);

void socket_timer_calback(void)
{
#if 0    
    char *report_buf = xy_malloc(64);

    snprintf(report_buf, 32, "\r\nsocket_timer_calback,stat:%d\r\n",g_socket_timeout_status);
	write_to_at_uart(report_buf,strlen(report_buf));
    xy_free(report_buf);
#endif

    if (g_socket_timeout_status == SOCK_TCP_CTL_TIMEOUT) {
        // socket_context_t* ctx = g_socket_ctx[g_socket_id];
        socket_close(g_socket_id);
    } else if (g_socket_timeout_status == SOCK_UDP_CTL_TIMEOUT) {
        char *report_buf = xy_malloc(32);
        
        snprintf(report_buf, 32, "+NSOSTR:%d,%d,0", g_socket_id,g_sequence);
        send_urc_to_ext(report_buf, strlen(report_buf));
 
        xy_free(report_buf);

    } else if (g_socket_timeout_status == SOCK_HTTP_CTL_TIMEOUT) {
        http_ctl_timeout_callback(g_socket_id);
    } else if (g_socket_timeout_status == SOCK_MQTT_CTL_TIMEOUT) {
        //mqtt_ctl_timeout_callback(g_socket_id);
    }
    if (socket_waiting_timer != NULL) {
        osTimerDelete(socket_waiting_timer);
	    socket_waiting_timer = NULL;
    }
}

void socket_conn_timeout_task(void)
{
    struct sock_tm_msg *rcv_msg = NULL;	
    osTimerAttr_t timer_attr = {0};
	//char *report_buf;
		
    while(1) {
		osMessageQueueGet(g_socket_timeout_q, &rcv_msg, NULL, osWaitForever);
        //vTaskDelay(100);
        #if 0
        report_buf = xy_malloc(32);
        sprintf(report_buf, "\r\nsocket_conn_timeout_task:%d/%d\r\n",rcv_msg->msg_id,rcv_msg->socket_id);
        write_to_at_uart(report_buf,strlen(report_buf));
        xy_free(report_buf);
        #endif
        
        switch (rcv_msg->msg_id)
		{
		    case SOCK_TCP_CTL_TIMEOUT:
            case SOCK_HTTP_CTL_TIMEOUT:
            case SOCK_MQTT_CTL_TIMEOUT:
            case SOCK_UDP_CTL_TIMEOUT:
                g_socket_timeout_status = rcv_msg->msg_id;
                g_socket_id = rcv_msg->socket_id;
                g_sequence = rcv_msg->sequence;
                
                timer_attr.name = "socket_tmr1";
                if (socket_waiting_timer != NULL) {
                    osTimerDelete(socket_waiting_timer);
                }
            	socket_waiting_timer = osTimerNew((osTimerFunc_t)(socket_timer_calback), osTimerOnce, NULL, &timer_attr);
		        osTimerStart(socket_waiting_timer, rcv_msg->delay_ms);

                break;
            case SOCK_CANCEL_CHECK:
                g_socket_timeout_status = SOCK_CANCEL_CHECK;
                if (socket_waiting_timer != NULL) {
                    osTimerDelete(socket_waiting_timer);
	                socket_waiting_timer = NULL;
                }
                break;
            default:
                break;
        }
        //xy_free(rcv_msg);
    }

	osThreadExit();
}

void call_socket_delay(SOCK_TIMEOUT_STATUS msg_type, int sockid,int delay_ms,int sequence)
{
	struct sock_tm_msg *msg =NULL;
	
    if ((g_socket_timeout_q == NULL) || (g_socket_timeout_handle == NULL)) {
        init_socket_timeout();
        vTaskDelay(50);
    }

	msg = xy_malloc(sizeof(struct sock_tm_msg));
	msg->msg_id = msg_type;
    msg->socket_id = sockid;
    msg->delay_ms = delay_ms;
    msg->sequence = sequence;

    if (g_socket_timeout_q != NULL) {
	    osMessageQueuePut(g_socket_timeout_q, &msg, 0, osNoWait);
    } else {
        xy_free(msg);
    }
}


void init_socket_timeout(void)
{
	osThreadAttr_t thread_attr = {0};
		
    if(g_socket_timeout_q == NULL) {
		g_socket_timeout_q = osMessageQueueNew(20, sizeof(void *), NULL);
    }

    if (g_socket_timeout_handle == NULL) {
        thread_attr.name	   = "socket_timeout";
        thread_attr.priority   = osPriorityNormal;
        thread_attr.stack_size = 0x400;
        
        g_socket_timeout_handle= osThreadNew ((osThreadFunc_t)(socket_conn_timeout_task),NULL,&thread_attr);
    }
}

//------------------------------------------------------------------------------

int socket_create(socket_create_param_t* arg)
{
    xy_assert(arg != NULL);

    int sock_fd = -1;
    int sock_id = -1;
    int af_type;
    struct sockaddr_storage bind_addr = {0};

    init_socket_timeout();

    if ((sock_id = get_socket_avail_id()) < 0)
    {
        return -1;
    }

    if (arg->proto != IPPROTO_UDP && arg->proto != IPPROTO_TCP)
    {
        return -1;
    }
	
	if (arg->af_type != AF_INET && arg->af_type != AF_INET6)
	{
		// 创建socket时必须指定socket类型是V4或V6
		return -1;
	}

    if (arg->remote_ip != NULL)
    {
        if (strlen(arg->remote_ip) == 0 || arg->remote_port < 0 || arg->remote_port > 65535)
        {
            return -1;
        }
        int dns_ret = 0;
        struct addrinfo hint = {0};
        struct addrinfo *result = NULL;
        hint.ai_protocol = arg->proto;
        hint.ai_socktype = (arg->proto == IPPROTO_UDP) ? SOCK_DGRAM : SOCK_STREAM;
        hint.ai_family = arg->af_type;
        uint8_t strPort[7] = {0};
        snprintf(strPort, sizeof(strPort) - 1, "%d", arg->remote_port);

        if ((dns_ret = getaddrinfo(arg->remote_ip, strPort, &hint, &result)) != 0)
        {
            xy_printf(0,XYAPP, WARN_LOG, "[socket create]getaddrinfo err:%d", dns_ret);
            return -1;
        }

        if ((sock_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) == -1)
        {
            xy_printf(0,XYAPP, WARN_LOG, "[socket create]create err:%d", errno);
            freeaddrinfo(result);
            return -1;
        }
        else
        {
            af_type = (result->ai_family == AF_INET6) ? 1 : 0;
        }

        freeaddrinfo(result);
    }
    else
    {
        if (arg->proto == IPPROTO_UDP)
        {
            sock_fd = socket(arg->af_type, SOCK_DGRAM, IPPROTO_UDP);
        }
        else
        {
            sock_fd = socket(arg->af_type, SOCK_STREAM, IPPROTO_TCP);
        }

        if (sock_fd == -1)
        {
            xy_printf(0,XYAPP, WARN_LOG, "[socket create]create err:%d", errno);
            return -1;
        }
        else
        {
            af_type = (arg->af_type == AF_INET6) ? 1 : 0;
        }
    }

    /* 设置bind addr prop */
    if (af_type == 1)
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
    socket_ctx->net_type = (arg->proto == IPPROTO_UDP) ? 1 : 0;
    socket_ctx->af_type = af_type;
    socket_ctx->fd = sock_fd;
    socket_ctx->sock_id = sock_id;
    socket_ctx->local_port_ori = arg->local_port;
    socket_ctx->cid = arg->cid;
    socket_ctx->recv_ctl = arg->recv_ctrl;
    /* local port为0情况下, bind完成后需要获取lwip内部分配的local port */
    xy_socket_local_info(sock_fd, NULL, &socket_ctx->local_port);

    g_socket_ctx[sock_id] = socket_ctx;
    reset_socket_seq_state(sock_id);

    /* 保存udp socket上下文到文件系统,用于edrx/drx下行数据云恢复 */
    update_socket_infos_to_fs(sock_id, true);

    socket_set_state(sock_id, SOCKET_STATE_CREATED);
    xy_printf(0,XYAPP, WARN_LOG, "[socket create]create success id(%d)", sock_id);
	
    if (arg->proto == IPPROTO_UDP)
    {
        //udp set socket recv/send non-block
        socket_setprop_nonblock(get_socket_fd(sock_id));
    }

	/* 启动socket recv线程 */
	start_at_socket_recv_thread();
	
    return sock_id;
}

int socket_connect(socket_conn_param_t* arg)
{
    xy_assert(arg != NULL);
    int8_t id = arg->id;

    if (!xy_tcpip_is_ok())
    {
        xy_printf(0,XYAPP, WARN_LOG, "[sock conn] net not avail");
        return XY_Err_NoConnected;
    }

    if (!is_socketId_valid(id))
    {
        xy_printf(0,XYAPP, WARN_LOG, "[sock conn]socket id(%d) invalid", arg->id);
        return XY_Err_Parameter;
    }

    socket_context_t* ctx = g_socket_ctx[id];
    struct addrinfo *result = NULL;

    /* 检测remote ipaddr字符串地址有效性 */
//    if ( (ctx->af_type == 0 && (xy_IpAddr_Check(arg->remote_ip, IPV4_TYPE) == 0)) ||
//         (ctx->af_type == 1 && (xy_IpAddr_Check(arg->remote_ip, IPV6_TYPE) == 0)) )
//    {
//        xy_printf(0,XYAPP, WARN_LOG, "[sock conn]socket id(%d) ipcheck invalid", id);
//        return XY_Err_Parameter;
//    }

    if (ctx->remote_ip != NULL)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[sock conn]socket id(%d) remoteip non-null(%s)", id, ctx->remote_ip);
        return XY_Err_Parameter;
    }

    ctx->remote_port = arg->remote_port;
    ctx->remote_ip = xy_malloc(strlen(arg->remote_ip) + 1);
    strcpy(ctx->remote_ip, arg->remote_ip);

    int dns_ret;
    struct addrinfo hint = {0};
    if (ctx->af_type == 1)
        hint.ai_family = AF_INET6;
    else
        hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_DGRAM;
    char port_str[7] = {0};
    snprintf(port_str, sizeof(port_str) - 1, "%d", ctx->remote_port);
    if ((dns_ret = getaddrinfo(ctx->remote_ip, port_str, &hint, &result)) != 0)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[sock conn]socket id(%d) getaddrinfo err:%d", id, dns_ret);
        return XY_Err_DnsFail;
    }

    socket_set_state(ctx->sock_id, SOCKET_STATE_CONNECTING);
    
    if (connect(ctx->fd, result->ai_addr, result->ai_addrlen) == -1)
    {
        freeaddrinfo(result);
        xy_printf(0,XYAPP, WARN_LOG, "[sock conn]socket id(%d) connect err:%d", id, errno);
        return covert_to_socket_errcode(errno);
    }

    /* 设置NON-BLOCK属性 */
    socket_setprop_nonblock(ctx->fd);

    socket_set_state(ctx->sock_id, SOCKET_STATE_CONNECTED);
    xy_printf(0,XYAPP, WARN_LOG, "[sock conn]socket id(%d) connect success", ctx->sock_id);
    freeaddrinfo(result);
    return XY_OK;
}

static void socket_connect_task(socket_conn_param_t *arg)
{
	char prsp[64] = {0};
	int ret = socket_connect(arg);
	at_SOCKCONN_URC(ret);

	xy_free(arg->remote_ip);
    xy_free(arg);
    osThreadExit();
}

void socket_connnect_async(socket_conn_param_t* arg)
{
	socket_conn_param_t *connect_arg = (socket_conn_param_t *)xy_malloc(sizeof(socket_conn_param_t));
	memcpy(connect_arg, arg, sizeof(socket_conn_param_t));
	connect_arg->remote_ip = xy_malloc(strlen(arg->remote_ip) + 1);
	strcpy(connect_arg->remote_ip, arg->remote_ip);

	osThreadAttr_t thread_attr = {0};
    thread_attr.name = "sck_connect";
    thread_attr.priority = osPriorityNormal1;
    thread_attr.stack_size = osStackShared;
	osThreadNew((osThreadFunc_t)(socket_connect_task), connect_arg, &thread_attr);
}

int socket_send(socket_send_param_t *arg)
{
    xy_assert(arg != NULL);

    int8_t id = arg->id;
    uint32_t pre_sn = 0;
    char *send_data = NULL;
    struct sockaddr_storage sockaddr = {0};
    int sockaddr_len = 0;
    int ret = XY_OK;

    if (!is_socketId_valid(id))
    {
        xy_printf(0,XYAPP, WARN_LOG, "[sock send]socket id(%d) invalid", id);
        return XY_Err_Parameter;
    }
 
    if (!check_socket_sendlen(arg->data_str_len, arg->data_len))
    {
        return XY_Err_Parameter;
    }

    if (check_socket_seq(id, arg->sequence) == 1)
    {
        return XY_Err_Parameter;
    }
    else
    {
        xy_printf(0, XYAPP, WARN_LOG, "[sock send]socket id(%d) seq:%d", id, arg->sequence);
    }

    /* TCP */
    if (g_socket_ctx[id]->net_type == 0)
    {
        if (arg->sequence > 0)
        {
            ioctl(g_socket_ctx[id]->fd, FIOREADSN, &pre_sn);
        }
    }
    /* UDP */
    else
    {
        /* udp已经执行connect操作， remote addr和port已知 */
        if (arg->udp_connectd == 0)
        {
            /* 检测remote ipaddr字符串地址有效性 */
            if ( (g_socket_ctx[id]->af_type == 0 && (xy_IpAddr_Check(arg->remote_ip, IPV4_TYPE) == 0)) ||
                 (g_socket_ctx[id]->af_type == 1 && (xy_IpAddr_Check(arg->remote_ip, IPV6_TYPE) == 0)) )
            {
                return XY_Err_Parameter;
            }

            g_socket_ctx[id]->remote_port = arg->remote_port;

            if (g_socket_ctx[id]->af_type == 1) //ipv6
            {
                ((struct sockaddr_in6 *)(&sockaddr))->sin6_family = AF_INET6;
                ((struct sockaddr_in6 *)(&sockaddr))->sin6_port = htons(g_socket_ctx[id]->remote_port);
                if (1 != inet_pton(AF_INET6, arg->remote_ip, &(((struct sockaddr_in6 *)(&sockaddr))->sin6_addr)))
                {
                    return XY_Err_Parameter;
                }
            }
            else
            {
                ((struct sockaddr_in *)(&sockaddr))->sin_family = AF_INET;
                ((struct sockaddr_in *)(&sockaddr))->sin_port = htons(g_socket_ctx[id]->remote_port);
                if (1 != inet_pton(AF_INET, arg->remote_ip, &(((struct sockaddr_in *)(&sockaddr))->sin_addr)))
                {
                    return XY_Err_Parameter;
                }
            }

            if (g_socket_ctx[id]->remote_ip != NULL)
                xy_free(g_socket_ctx[id]->remote_ip);
            g_socket_ctx[id]->remote_ip = xy_malloc(strlen(arg->remote_ip) + 1);
            strcpy(g_socket_ctx[id]->remote_ip, arg->remote_ip);
            sockaddr_len = sizeof(sockaddr);
        }
    }

	if (g_data_send_mode == HEX_ASCII_STRING)
	{
		send_data = xy_malloc2(arg->data_len);

		if(send_data == NULL)
		{
			ret = XY_Err_NoMemory;
			goto ERR_PROC;
		}
		if (hexstr2bytes(arg->data, arg->data_len * 2, send_data, arg->data_len) == -1)
		{
            ret = XY_Err_Parameter;
			xy_free(send_data);
            goto ERR_PROC;
		}
        if (sockaddr_len > 0)
            ret = sendto2(g_socket_ctx[id]->fd, send_data, arg->data_len, arg->flag, (struct sockaddr *)&sockaddr, sockaddr_len, arg->sequence, arg->rai_type);
        else
            ret = send2(g_socket_ctx[id]->fd, send_data, arg->data_len, arg->flag, arg->sequence, arg->rai_type);
		xy_free(send_data);
    }
	else if (g_data_send_mode == ASCII_STRING)
	{
        if (sockaddr_len > 0)
            ret = sendto2(g_socket_ctx[id]->fd, arg->data, arg->data_len, arg->flag, (struct sockaddr *)&sockaddr, sockaddr_len, arg->sequence, arg->rai_type);
        else
            ret = send2(g_socket_ctx[id]->fd, arg->data, arg->data_len, arg->flag, arg->sequence, arg->rai_type);
    }

    if (ret <= 0)
	{
		xy_printf(0,XYAPP, WARN_LOG, "socket[%d-%d] send errno:%d", id, g_socket_ctx[id]->fd, errno);
		if ((errno == EHOSTUNREACH && arg->net_type == 1) || (errno == ENOTCONN && arg->net_type == 0) )
		{
			ret = covert_to_socket_errcode(errno);
            goto ERR_PROC;
		}
		else
		{
			if (arg->sequence > 0)
			{
                g_socket_ctx[id]->sequence_state[arg->sequence - 1] = SEND_STATUS_FAILED;
            }
            ret = XY_ERR;
            goto ERR_PROC;
        }
	}
    else
    {
        ret = XY_OK;
    }
    
    if (arg->sequence > 0)
    {
        add_sninfo_node(id, arg->data_len, arg->sequence, pre_sn);
        g_socket_ctx[id]->sequence_state[arg->sequence - 1] = SEND_STATUS_SENDING;
    }
    g_socket_ctx[id]->sended_size += arg->data_len;
    goto OK_PROC;

ERR_PROC:
    /* 手动指定remote addr和port的情况下，异常情况需要释放已申请的remote addr内存 */
    if (arg->udp_connectd == 0 && g_socket_ctx[id] != NULL && g_socket_ctx[id]->remote_ip != NULL)
    {
        xy_free(g_socket_ctx[id]->remote_ip);
        g_socket_ctx[id]->remote_ip = NULL;
    }
OK_PROC:
    return ret;
}

int socket_close(int sock_id)
{
    if (!is_socketId_valid(sock_id))
    {
        xy_printf(0,XYAPP, WARN_LOG, "[sock close]socket id(%d) invalid", sock_id);
        return XY_Err_Parameter;
    }

    socket_context_t* ctx = g_socket_ctx[sock_id];

    if (ctx->fd >= 0)
    {
    	/* 置标志位后,在at_sock_recv_thread中完成关闭socket操作 */
        ctx->quit = 1;
        xy_printf(0,XYAPP, WARN_LOG, "[sock close]socket id(%d) quit flag set 1!!!", sock_id);
    }
    else
    {
        return XY_Err_Parameter;
    }

    socket_set_state(sock_id, SOCKET_STATE_CLOSING);

    return XY_OK;
}

void socket_set_state(int id, int state)
{
    if (state < SOCKET_STATE_CREATED || state >= SOCKET_STATE_MAX)
        return;
    if (!is_socketId_valid(id))
    {
        return;
    }
    g_socket_ctx[id]->state = state;
    xy_printf(0,XYAPP, WARN_LOG, "socket id(%d) set state:%d", id, state);
    return;
}

int socket_get_state(int id)
{
    if (!is_socketId_valid(id))
        return SOCKET_STATE_CLOSED;

    return g_socket_ctx[id]->state;
}

int socket_setprop_rcvtimeout(int fd, int timeout)
{
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
}

int socket_setprop_nonblock(int fd)
{
    int fl = fcntl(fd, F_GETFL, 0);
	return fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}


int socket_get_remaining_buffer_len(int id)
{
	if (g_socket_ctx[id] != NULL && g_socket_ctx[id]->data_list != NULL)
	{
		return g_socket_ctx[id]->data_list->len;
	}
	return 0;
}

int socket_read_buffer(socket_readbuffer_param_t *arg)
{
    xy_assert(arg != NULL);
    SOCKET_CTX_LOCK();
    int id = arg->id;
    recv_data_node_t *temp = g_socket_ctx[id]->data_list;

	if (temp != NULL)
	{
		arg->data = xy_malloc2(temp->len * 2 + 1);
		if (arg->data == NULL)
		{
			SOCKET_CTX_UNLOCK();
			return XY_ERR;
		}
        arg->remote_ip = xy_malloc(DNS_MAX_NAME_LENGTH);
		struct sockaddr_storage *remote_info = temp->sockaddr_info;
		if (g_socket_ctx[id]->af_type == 1)
		{
			struct sockaddr_in6 *addr6 = temp->sockaddr_info;
			inet_ntop(AF_INET6, &(addr6->sin6_addr), arg->remote_ip, DNS_MAX_NAME_LENGTH);	
			arg->remote_port = ntohs(addr6->sin6_port);
		}
		else
		{			
			struct sockaddr_in *addr4 = temp->sockaddr_info;
			inet_ntop(AF_INET, &(addr4->sin_addr), arg->remote_ip, DNS_MAX_NAME_LENGTH);
			arg->remote_port = ntohs(addr4->sin_port);
		}

		if (temp->len <= arg->want_len)
		{
            arg->read_len = temp->len;

			if (g_data_recv_mode == HEX_ASCII_STRING)
			{
				bytes2hexstr(temp->data, temp->len, arg->data, temp->len * 2 + 1);
			}
			else if (g_data_recv_mode == ASCII_STRING)
			{
				memcpy(arg->data, temp->data, temp->len);
				*(arg->data + temp->len) = '\0';
			}
			else
            {
                if(arg->remote_ip != NULL)
                xy_free(arg->remote_ip);
                if(arg->data != NULL)
                    xy_free(arg->data);
                SOCKET_CTX_UNLOCK();
                return XY_Err_Parameter;
            }

            g_socket_ctx[id]->data_list = temp->next;
            if (temp->data)
                xy_free(temp->data);
            if (temp->sockaddr_info)
                xy_free(temp->sockaddr_info);
            xy_free(temp);

            arg->remaining_len = socket_get_remaining_buffer_len(id);


			if (g_at_sck_report_mode == BUFFER_WITH_HINT && g_socket_ctx[id]->data_list != NULL)
			{
                /* 上报下一个节点的数据信息，例如+NSONMI: */
                arg->nsonmi_rpt = 1;
			}
			else if (g_socket_ctx[id]->data_list == NULL)
			{
				g_socket_ctx[id]->firt_recv = 0;
			}
		}
		else
		{
			char* remain_data = xy_malloc2(temp->len - arg->want_len + 1);
			if (remain_data == NULL)
			{
                if (arg->remote_ip != NULL)
                    xy_free(arg->remote_ip);                
				if (arg->data)
					xy_free(arg->data);
				SOCKET_CTX_UNLOCK();
				return XY_ERR;
			}
            memcpy(remain_data, temp->data + arg->want_len, temp->len - arg->want_len);
			*(remain_data + temp->len - arg->want_len) = '\0';

			if (g_data_recv_mode == HEX_ASCII_STRING)
			{
				bytes2hexstr(temp->data, arg->want_len, arg->data, arg->want_len * 2 + 1);
			}
			else if (g_data_recv_mode == ASCII_STRING)
			{
				memcpy(arg->data, temp->data, arg->want_len);
				*(arg->data + arg->want_len) = '\0';
			}

			xy_free(temp->data);

			temp->data = remain_data;
			temp->len = temp->len - arg->want_len;

            arg->read_len = arg->want_len;
            arg->remaining_len = socket_get_remaining_buffer_len(id);
		}
	}
	else
	{
        SOCKET_CTX_UNLOCK();
        return XY_ERR;
    }

	SOCKET_CTX_UNLOCK();   
    return XY_OK;
}

int socket_enter_passthr_mode(socket_send_param_t *param)
{
    int sockaddr_len = 0;

	if (g_socket_passthr_info != NULL)
	{
		return XY_Err_NotAllowed;
	}
	if (param->data_len > AT_SOCKET_MAX_DATA_LEN)
	{
		return XY_Err_NotAllowed;
	}
	g_socket_passthr_info = xy_malloc(sizeof(socket_passthr_info_t));
	g_socket_passthr_info->socket_id = param->id;
	g_socket_passthr_info->rai_flag = param->rai_type;
	g_socket_passthr_info->sequence = param->sequence;
	g_socket_passthr_info->result = XY_OK;
	g_socket_passthr_info->send_length = 0;
	g_socket_passthr_info->data_send_mode = param->passthr_data_mode;
    g_socket_passthr_info->data_echo = param->data_echo;
	if (get_socket_net_type(param->id) == 1)
	{
		// udp
		g_socket_passthr_info->pre_sn = 0;
        g_socket_passthr_info->udp_connect = param->udp_connectd;
        /* udp已经执行connect操作， remote addr和port已知 */
        if (param->udp_connectd == 0)
		{			
			if (get_socket_af_type(param->id) == 1) //ipv6
			{
				((struct sockaddr_in6*)(&g_socket_passthr_info->sock_addr))->sin6_family = AF_INET6;
				((struct sockaddr_in6*)(&g_socket_passthr_info->sock_addr))->sin6_port = htons(param->remote_port);
				if (1 != inet_pton(AF_INET6, param->remote_ip, &(((struct sockaddr_in6 *)(&g_socket_passthr_info->sock_addr))->sin6_addr)))
				{
					return XY_Err_Parameter;
				}
			}
			else
			{
				((struct sockaddr_in*)(&g_socket_passthr_info->sock_addr))->sin_family = AF_INET;
				((struct sockaddr_in*)(&g_socket_passthr_info->sock_addr))->sin_port = htons(param->remote_port);
				if (1 != inet_pton(AF_INET, param->remote_ip, &(((struct sockaddr_in *)(&g_socket_passthr_info->sock_addr))->sin_addr)))
				{
					return XY_Err_Parameter;
				}
			}
		}
	}
	else
	{
		// tcp
		g_socket_passthr_info->pre_sn = get_socket_pre_sn(param->id, param->sequence);
	}
	
	if (param->data_len > 0)
	{
		if (g_socket_passthr_info->data_send_mode == HEX_ASCII_STRING)
			passthr_fixed_buff_len = param->data_len * 2;
		else if (g_socket_passthr_info->data_send_mode == ASCII_STRING)
			passthr_fixed_buff_len = param->data_len;
		xy_enterPassthroughMode((app_passthrough_proc)socket_fixed_length_passthr_proc, (app_passthrough_exit)socket_fixed_length_passthr_exit);
	}
	else
	{
		xy_enterPassthroughMode((app_passthrough_proc)socket_unfixed_length_passthr_proc, (app_passthrough_exit)socket_unfixed_length_passthr_exit);
	}
#if VER_BC25
    send_urc_to_ext_NoCache("\r\n> ",strlen("\r\n> "));
#elif VER_260Y
    send_urc_to_ext_NoCache("\r\n>\r\n",strlen("\r\n>\r\n"));
#else
	send_urc_to_ext_NoCache("\r\n>",strlen("\r\n>"));
#endif
	return XY_OK;
}

void socket_pack_qsosrsp(int *socket_ids, char *rsp_fmt, char **prsp_cmd)
{
	int current_size = 100;
	int rspok_size = strlen("\r\nOK\r\n") + 1;
	int maxUrc_size = strlen("\r\n+NQSOS:9,255\r\n"); //NQSOS URC上报的最大长度
	uint32_t i = 0;
	uint32_t j = 0;
	
	//初始分配100字节，后续动态realloc，避免静态分配可能造成的内存浪费
	*prsp_cmd = xy_malloc(current_size);
	**prsp_cmd = '\0';
	for (i = 0; i < SOCK_NUM; i++)
	{
		if (is_socketId_valid(socket_ids[i]))
		{
			uint8_t id = (uint8_t)socket_ids[i];
			for (j = 0; j < SEQUENCE_MAX; j++)
			{
				if (g_socket_ctx[id]->sequence_state[j] == SEND_STATUS_SENDING)
				{
					int offset = strlen(*prsp_cmd);
					//计算满足NQSOS命令正常返回的长度，需包含所有上报的NQSOS信息及OK信息的长度
					//如果不包含OK信息长度,可能导致OK信息无法返回，出现8007错误
					int valid_len = offset + rspok_size + maxUrc_size + 1;
					if (valid_len > current_size)
					{
						char *new_lloc = xy_malloc(current_size + 100);
						current_size += 100;
						xy_printf(0,XYAPP, WARN_LOG, "uart_send:%s\r\n", current_size);
						memcpy(new_lloc, *prsp_cmd, offset);
						xy_free(*prsp_cmd);
						*prsp_cmd = new_lloc;
					}
					snprintf(*prsp_cmd + offset, 20, rsp_fmt, id, j + 1);
				}
			}
		}
	}

}

void socket_debug(int id)
{
    if (!is_socketId_valid(id) || HWREGB(BAK_MEM_XY_DUMP) == 0)
    {
        return;
    }
    socket_context_t* ctx = g_socket_ctx[id];
    char* socket_debug = xy_malloc(128);

    sprintf(socket_debug, "\r\nsocket id:%d,af_type:%d,proto:%d,cid:%d,state:%d\r\n",
            ctx->sock_id, ctx->af_type, ctx->net_type, ctx->cid, ctx->state);
    send_debug_by_at_uart(socket_debug);
    memset(socket_debug, 0, 128);
    if (ctx->remote_ip != NULL)
    {
        sprintf(socket_debug, "\r\nsocket fd:%d,remote ip:%s,remote port:%d\r\n", ctx->fd, ctx->remote_ip, ctx->remote_port);
    }
    else
    {
        sprintf(socket_debug, "\r\nsocket fd:%d\r\n", ctx->fd);
    }
    
    send_debug_by_at_uart(socket_debug);
    memset(socket_debug, 0, 128);
    sprintf(socket_debug, "\r\nlocal port:%d,local port ori:%d,sended size:%d,acked size:%d,quit flag:%d\r\n",
            ctx->local_port, ctx->local_port_ori, ctx->sended_size, ctx->acked_size, ctx->quit);
    send_debug_by_at_uart(socket_debug);
    xy_free(socket_debug);
    return;
}
