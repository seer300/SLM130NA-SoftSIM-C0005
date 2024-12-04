/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_ctl.h"
#include "at_worklock.h"
#include "at_com.h"
#include "at_passthrough.h"
#include "at_ps_proxy.h"
#include "at_uart.h"
#include "factory_nv.h"
#include "ipc_msg.h"
#include "low_power.h"
#include "oss_nv.h"
#include "xy_system.h"
#include "rtc_tmr.h"

/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/
osMessageQueueId_t at_farps_q = NULL;

/*******************************************************************************
 *						  Local variable definitions						   *
 ******************************************************************************/
osMessageQueueId_t at_msg_q = NULL;
osMutexId_t at_send_2_ctl_m = NULL; //used to protect send_msg_2_at_ctl func visit
osMutexId_t at_forward_m = NULL; 	//used to protect send_msg_2_at_ctl func visit

/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/
void urc_print(void *data ,uint32_t len)
{
	if(data == NULL || len == 0)
		return;
	
	char *urc_str = xy_malloc(64);
	snprintf(urc_str, 64, "%s", (char*)data);
	*(urc_str + 63) = '\0';
	xy_printf(0, PLATFORM, WARN_LOG,"urc:%s",urc_str);
	xy_free(urc_str);
}

/*普通的URC或码流的发送接口，支持URC缓存，不经过at_ctl主框架*/
void send_urc_to_ext(void* str, uint32_t size)
{
	void *data = str;
	
	urc_print(data, size);
	if (is_urc_drop())
	{
		return;
	}
	else
	{
		/*框架识别，自动添加\r\n*/
		if(*((char *)str+size-1) != '\n')
		{
			data = xy_malloc(size+5);
			*((char *)data) = '\0';
			
			strcpy((char *)data,"\r\n");

			strcat((char *)data,(char *)str);

			strcat((char *)data,"\r\n");

			size += 4;
		}
		
        /* AP SYNC通道不传递URC */
#if URC_CACHE
		if (is_at_lpuart_doing() && !g_CombCmd_Doing)
		{
			at_add_urc_cache(&ap_ext_ctx, (char*)data, size);
		}
		else
#endif /* URC_CACHE */
		{
			if(HWREGB(BAK_MEM_DROP_URC) != 2)
			{
				write_to_at_uart((char*)data, size);
			}
		}


#if URC_CACHE
		if ((ap_async_ctx.state & RCV_REQ_NOW) && !g_CombCmd_Doing)
		{
			if(HWREGB(BAK_MEM_DROP_URC) != 1)
			{
				at_add_urc_cache(&ap_async_ctx, (char *)data, size);
			}
		}
		else
#endif /* URC_CACHE */
		{
			if(HWREGB(BAK_MEM_DROP_URC) != 1)
			{
				at_write_to_AP(FARPS_AP_ASYNC, (char*)data, size, 0);
			}
		}

		if(data != str)
			xy_free(data);
	}
}

void send_urc_to_ext2(void* data)
{
	send_urc_to_ext(data,strlen(data));
}

static void proc_from_nearps(at_msg_t *msg)
{
	xy_assert(msg->ctx != NULL && msg->data != NULL);
	char at_prefix[AT_CMD_PREFIX] = {0};
	bool is_result_code = Is_Result_AT_str(msg->data);

	at_get_prefix_for_URC(msg->data, at_prefix);

	//处理短信
	if (msg->ctx->at_cmd_prefix != NULL && is_sms_atcmd(msg->ctx->at_cmd_prefix) && msg->ctx->fwd_ctx != NULL)
	{
		//非透传模式下的数据正常输出
		at_write_by_ctx(msg->ctx->fwd_ctx, msg->data, msg->size);

		if (strstr(msg->data, "> "))
		{
			xy_enterPassthroughMode((app_passthrough_proc)at_sms_passthr_proc, (app_passthrough_exit)at_sms_passthr_exit);
		}
		else if (is_result_code)
		{
			xy_exitPassthroughMode();
		}
		goto end_proc;
	}

	//不包含结果码的URC信息会广播到所有farps端
	if (!is_result_code && strlen(at_prefix) != 0 &&
		(strlen(msg->ctx->at_cmd_prefix) == 0 || strcasecmp(msg->ctx->at_cmd_prefix, at_prefix)))
	{
		xy_printf(0,PLATFORM, WARN_LOG, "[proc_from_nearps] ps urc info [%s]", at_prefix);
		send_urc_to_ext(msg->data, strlen(msg->data));
	}
	else
	{
		//处理at_ReqAndRsp_to_ps接口上报的信息，解阻塞用户线程
		if (msg->ctx->fwd_ctx != NULL && msg->ctx->fwd_ctx->fd >= FARPS_USER_MIN && msg->ctx->fwd_ctx->fd <= FARPS_USER_MAX)
		{
			xy_printf(0,PLATFORM, WARN_LOG, "[proc_from_nearps] response info for user:[%d]", msg->ctx->fwd_ctx->fd);
			at_write_by_ctx(msg->ctx->fwd_ctx, msg->data, msg->size);
		}
		else if (msg->ctx->fwd_ctx != NULL)
		{
			//将at urc信息返回给对应的at上下文
			xy_printf(0,PLATFORM, WARN_LOG, "[proc_from_nearps] response info for sended cmd");
			at_write_by_ctx(msg->ctx->fwd_ctx, msg->data, msg->size);
		}
		else
		{
			if (is_result_code)
				xy_printf(0,PLATFORM, WARN_LOG, "[proc_from_nearps] rcv undefined rsp AT!!!");
			else
			{
				//broadcast for farps entity which can receive broadcast info
				xy_printf(0,PLATFORM, WARN_LOG, "[proc_from_nearps] response info user urc,maybe nping result");
				send_urc_to_ext(msg->data, strlen(msg->data));
			}
		}
	}

end_proc:
	if (is_result_code && msg->msg_id == AT_MSG_RCV_STR_FROM_NEARPS)
	{
		//重置nearps at context
		int err = Get_AT_errno(msg->data);

		if (err != ATERR_CHANNEL_BUSY)
		{
			reset_ctx(msg->ctx);
		}
	}
}

static void proc_from_farps(at_msg_t *msg)
{

	xy_assert(msg->ctx != NULL && msg->msg_id == AT_MSG_RCV_STR_FROM_FARPS);

	special_ps_at_proc(msg->data);

	//Step1: 获取at命令的前缀,参数和命令类型
	char *at_param = at_get_prefix_and_param(msg->data, msg->ctx->at_cmd_prefix,&msg->ctx->at_type);

	//Step2: 判断解析的at前缀是否合法
	char *at_prefix = msg->ctx->at_cmd_prefix;
	if (at_prefix == NULL || !strlen(at_prefix))
	{
		xy_printf(0,PLATFORM, WARN_LOG, "at prefix from farps invalid");
		AT_ERR_BY_CONTEXT(ATERR_INVALID_PREFIX, msg->ctx);
		return;
	}

	//Step3: xy_proxy线程中调用at_send_wait_rsp接口，只能是3GPP相关命令，直接转发给PS
	if (msg->ctx->fd == FARPS_USER_PROXY)
	{	
		xy_printf(0,PLATFORM, WARN_LOG, "at ctl deal with msg from proxy");
		goto FWD_PROC;
	}

	//Step4:芯翼平台基础命令由xy_proxy线程处理
	int i = 0;
	struct at_serv_proc_e *at_basic;
	for (i = 0; g_at_basic_req != NULL && (g_at_basic_req + i)->at_prefix != 0; i++)
	{
		at_basic = g_at_basic_req + i;
		if (at_strcasecmp(at_prefix, at_basic->at_prefix))
		{
			xy_assert(at_basic->proc != NULL);

			/*前缀可能是小写，但URC前缀要求大写*/
			strcpy(at_prefix, at_basic->at_prefix);
			msg->ctx->at_proc = at_basic->proc;
			/* 只传递参数offset，减少拷贝及malloc次数 */
			if (at_param != NULL)
			{
				msg->offset = at_param - msg->data;
				xy_printf(0,PLATFORM, WARN_LOG, "at parma offset:%d", msg->offset);
			}
			else
			{
				msg->ctx->at_param = NULL;
				msg->offset = 0;
			}
			//将当前的at_msg拷贝发送到xy_proxy线程中
			send_msg_2_at_proxy(AT_PROXY_MSG_CMD_PROC, msg, sizeof(at_msg_t) + msg->size + 1);
			xy_printf(0,PLATFORM, WARN_LOG, "[xy_basic] msg [%s] send to proxy task", at_prefix);
			return;
		}
	}

FWD_PROC:
	//Step5: 转发at命令给PS
	forward_req_at_proc(msg);
}

static void init_at_resource(void)
{
	at_msg_q = osMessageQueueNew(50, sizeof(void *), NULL);
	at_farps_q = osMessageQueueNew(10, sizeof(void *), NULL);
	at_send_2_ctl_m = osMutexNew(NULL);
	at_forward_m = osMutexNew(NULL);
	g_at_uart_mux = osMutexNew(NULL);
	osMutexAttr_t mutex_attr = {0};
	mutex_attr.attr_bits = osMutexRecursive;
	at_ctx_dict_m = osMutexNew(&mutex_attr);

	//initial nearps context
	nearps_ctx.fd = NEARPS_CP_FD;
	nearps_ctx.position = NEAR_PS;
	register_at_context(&nearps_ctx);

	//initial log context
	log_ctx.fd = FARPS_LOG_FD;
	log_ctx.position = FAR_PS;
	log_ctx.farps_write = at_send_to_log;
	register_at_context(&log_ctx);
	
	//initial ap context
	ap_async_ctx.fd = FARPS_AP_ASYNC;
	ap_async_ctx.position = FAR_PS;
	ap_async_ctx.urcMutex = osMutexNew(NULL);
	ap_async_ctx.farps_write = at_send_to_ap;
	register_at_context(&ap_async_ctx);

	ap_ext_ctx.fd = FARPS_AP_EXT;
	ap_ext_ctx.position = FAR_PS;
	ap_ext_ctx.urcMutex = osMutexNew(NULL);
	ap_ext_ctx.farps_write = at_send_to_uart;
	register_at_context(&ap_ext_ctx);

	ap_ble_ctx.fd = FARPS_BLE_FD;
	ap_ble_ctx.position = FAR_PS;
	ap_ble_ctx.urcMutex = osMutexNew(NULL);
	ap_ble_ctx.farps_write = at_send_to_ap;
	register_at_context(&ap_ble_ctx);

	//initial ap sync context
	ap_sync_ctx.fd = FARPS_AP_SYNC;
	ap_sync_ctx.position = FAR_PS;
	ap_sync_ctx.urcMutex = osMutexNew(NULL);
	ap_sync_ctx.farps_write = at_send_to_ap;
	register_at_context(&ap_sync_ctx);

	//initial user rsp context
	user_app_ctx.fd = NEARPS_USER_FD;
	user_app_ctx.position = NEAR_PS;
	register_at_context(&user_app_ctx);
}

/*******************************************************************************
 *						Global function implementations 					   *
 ******************************************************************************/



void at_ctl(void)
{
	void *rcv_msg = NULL;
	at_msg_t *msg = NULL;

#if 0// VER_BC95
	extern int g_npsmr_status;

	//BC95V200对标,下列参数深睡保存掉电不保存,设置后不写fac_nv,用易变NV存储
	if(!Is_WakeUp_From_Dsleep())
	{
	    g_softap_var_nv->g_NITZ = g_softap_fac_nv->g_NITZ;
	    g_softap_var_nv->g_NPSMR_enable = g_softap_fac_nv->g_NPSMR_enable;
	}
	else
	{
	    g_softap_fac_nv->g_NITZ = g_softap_var_nv->g_NITZ;
	    g_softap_fac_nv->g_NPSMR_enable = g_softap_var_nv->g_NPSMR_enable;

	    if(g_softap_var_nv->ps_deepsleep_state != 2 && g_softap_var_nv->ps_deepsleep_state != 3)
            g_npsmr_status = 1;
	}
#endif

	if(p_SysUp_URC_Hook != NULL)
		p_SysUp_URC_Hook();

	if(HWREGB(BAK_MEM_RF_MODE) == 1)
		xy_printf(0,PLATFORM, WARN_LOG, "Enter into RF cali mode!");

	/*保存软件异常断言的时间戳*/
	extern void boot_cp_dbg_into();
	boot_cp_dbg_into();


	/* at命令部分全局变量初始化 */
	g_NITZ_mode = g_softap_fac_nv->g_NITZ;

	/*外部MCU触发唤醒，必须启动延迟锁，否则云通信中有sleep操作会释放调度进入idle深睡*/
	sleep_lock_init();

	/*使用软定时器设置周期性帧信息更新快照信息，以维持世界时间精度，深睡后失效*/
	set_snapshot_update_timer();

	while (1)
	{
		osMessageQueueGet(at_msg_q, &rcv_msg, NULL, osWaitForever);

		if (rcv_msg == NULL)
		{
			xy_assert(0);
			continue;
		}
		msg = ((at_msg_t *)rcv_msg);
		switch (msg->msg_id)
		{
		case AT_MSG_RCV_STR_FROM_FARPS:
			proc_from_farps(msg);
			break;
		case AT_MSG_RCV_STR_FROM_NEARPS:
			proc_from_nearps(msg);
			break;

		default:
			break;
		}
		xy_free(msg);
	}
}

void at_init(void)
{
	/* 初始化at框架使用的全局上下文、信号量和队列等资源 */
	init_at_resource();

	/* ps回调注册，该接口必须在内核主线程中执行 */
	ps_urc_register_callback_init();

	/* 创建at_ctl线程 */
	osThreadAttr_t thread_attr 	= {0};
	thread_attr.name 			= AT_CTRL_THREAD_NAME;
	thread_attr.priority 		= AT_CTRL_THREAD_PRIO;
	thread_attr.stack_size 		= AT_CTRL_THREAD_STACKSIZE;
	osThreadNew((osThreadFunc_t)(at_ctl), NULL, &thread_attr);

	at_proxy_init();
}

int send_msg_2_atctl(int msg_id, void *buf, int size, at_context_t* ctx)
{
    osMutexAcquire(at_send_2_ctl_m, osWaitForever);
	xy_assert(ctx != NULL);
    int ret = AT_OK;

    xy_printf(0,PLATFORM, WARN_LOG, "send_msg_2_atctl msg_id: %d, fd: %d", msg_id, ctx->fd);

    if (msg_id == AT_MSG_RCV_STR_FROM_NEARPS || msg_id == AT_MSG_RCV_STR_FROM_FARPS)
	{
		at_msg_t *msg;
		
		msg = xy_malloc(sizeof(at_msg_t) + size + 1);
		msg->msg_id = msg_id;
		msg->size = size;
		msg->ctx = ctx;
		msg->offset = 0;
		if (buf != NULL)
		{
			memcpy(msg->data, buf, size);
			*(msg->data + size) = '\0';
		}
		osMessageQueuePut(at_msg_q, &msg, 0, osWaitForever);
	}
	else
    {
        ret = ATERR_NOT_ALLOWED;
    }

    osMutexRelease(at_send_2_ctl_m);
    return ret;
}

void forward_req_at_proc(at_msg_t *msg)
{
	//Step1: get nearps context firstly
	osMutexAcquire(at_forward_m, osWaitForever);
	at_context_t *fwd_ctx = search_at_context(NEARPS_CP_FD);
	xy_assert(fwd_ctx != NULL);

	//Step2: judge nearps context is on working or not when MCU and AP user send at req at the same time
	if (fwd_ctx->state & SEND_REQ_NOW)
	{
		if (msg->ctx->retrans_count >= AT_RETRANS_MAX_NUM)
		{
			SEND_ERR_RSP_TO_EXT(ATERR_CHANNEL_BUSY, msg->ctx);
			char *at_str = xy_malloc(64);
			char new_prefix[10] = {0};
			memcpy(new_prefix, msg->data, 9);
			snprintf(at_str, 64, "\r\n+DBGINFO:NEARPS BUSY:%s,new:%s\r\n", fwd_ctx->at_cmd_prefix, new_prefix);
			send_debug_by_at_uart(at_str);
			xy_free(at_str);
			reset_ctx(msg->ctx);
		}
		else
		{
			osDelay(AT_RETRANS_DELAY_TIME);
			msg->ctx->retrans_count++;
			xy_printf(0,PLATFORM, WARN_LOG, "atforward conflict, re-send fd:%d,count:%d", msg->ctx->fd, msg->ctx->retrans_count);
			send_msg_2_atctl(msg->msg_id, msg->data, msg->size, msg->ctx);
		}
		osMutexRelease(at_forward_m);
		return;
	}

	//Step3:获取前缀，需注意，调用此接口时，原始AT命令的前缀已解析并做非0判断，此处无需再次解析和判断，直接memcpy即可
	memcpy(fwd_ctx->at_cmd_prefix, msg->ctx->at_cmd_prefix, strlen(msg->ctx->at_cmd_prefix));

	//Step4: mark nearps context state,and link the nearsps context with current context
	fwd_ctx->state |= SEND_REQ_NOW;
	msg->ctx->fwd_ctx = fwd_ctx;
	fwd_ctx->fwd_ctx = msg->ctx;

	//Step5: at命令转发给PS
	xy_printf(0,PLATFORM, WARN_LOG, "send msg [%s] to ps", msg->ctx->at_cmd_prefix);
	at_write_by_ctx(fwd_ctx, msg->data, strlen(msg->data));
	osMutexRelease(at_forward_m);
	return;
}

void send_at_err_to_context(int errno, at_context_t *ctx, char *file, int line)
{
	char *at_errno = at_err_build_info(errno, file, line);
	at_write_by_ctx(ctx, at_errno, strlen(at_errno));
	xy_free(at_errno);
}

bool send_rsp_str_2_app(osMessageQueueId_t queue_fd, char *data, int size)
{
	UNUSED_ARG(size);
    xy_assert(queue_fd != NULL);
    struct at_fifo_msg *msg = xy_malloc(strlen(data) + sizeof(struct at_fifo_msg) + 1);
    strcpy(msg->data, data);
	osMessageQueuePut(queue_fd, &msg, 0, osWaitForever);
    return 1;
}

bool at_write_by_ctx(at_context_t *ctx, void *buf, int size)
{
	int ret = 0;

	xy_assert(size != 0 && ctx != NULL);

	if (ctx->position == NEAR_PS)
	{
		if(ctx->fd == NEARPS_CP_FD) //just cp fd transfer to cp
		{
			/*RF校准模式，PS未初始化，此处直接构造OK结果码，防止平台AT框架有异常*/
			if (HWREGB(BAK_MEM_RF_MODE) == 1)
			{
				at_write_by_ctx(ctx->fwd_ctx, "OK\r\n", strlen("OK\r\n"));
				return 1;
			}
			SendAt2AtcAp((char *)buf, size);
			ret = 1;
		}
	}
	else if (ctx->position == FAR_PS)
	{
		if (ctx->fd >= FARPS_USER_MIN && ctx->fd <= FARPS_USER_MAX)
		{
			//notify the blocked queue which related to the fd
			xy_printf(0,PLATFORM, WARN_LOG, "receive rsp and notify queue: %p", ctx->user_queue_Id);
			ret = send_rsp_str_2_app(ctx->user_queue_Id, (char *)buf, size);
			return ret;
		}

		if (ctx->farps_write != NULL)
		{
			ret = ctx->farps_write(ctx, (char *)buf, size);
		}
		else
		{
			xy_printf(0,PLATFORM,WARN_LOG,"farps write null!!");
		}
	}

	return ret;
}

static char RebootCauseNameItem[][32] = {
	"SECURITY_PMU_POWER_ON_RESET",  //正常断电上电动作,含OPENCPU的stop_CP场景
	"SECURITY_RESET_PIN",           //外部通过RESET-PIN执行的芯片硬复位
	"APPLICATION_AT",               //AT+NRB触发的软重启
	"APPLICATION_RTC",              //RTC触发的深睡唤醒
	"SECURITY_EXTERNAL_PIN",        //外部LPUART唤醒，通常为AT命令唤醒
	"SECURITY_WATCHDOG",            //AP_WDT/UTC_WDT复位，不包括CP核的看门狗复位
	"SECURITY_FOTA_UPGRADE",        //FOTA升级重启
	"APPLICATION_SYSRESETREQ",      //AT+RESET触发的软重启
	"SECURITY_GLOBAL_RESET",        //CP核看门狗 + xy_Soc_Reset + ASSERT异常断言 + SVD
	"SECURITY_SOFT_RESET",          //用户调用xy_Soft_Reset触发的软重启
	"SECURITY_WAKEUP_DSLEEP",       //除了RTC和LPUART之外的其他深睡唤醒，如WAKUP-PIN唤醒等
	"SECURITY_RESET_UNKNOWN"       //未知的异常上电，一般为硬件故障
};

/*系统深睡URC参加Sys_Down_URC*/
void Sys_Up_URC_default()
{
	char *at_str = NULL;

	if (!is_urc_drop())
	{
		at_str = xy_malloc(64);
		//snprintf(at_str, 64, "\r\n+REBOOT_CAUSE_%ld\r\n", at_get_power_on());
		snprintf(at_str, 64, "\r\nREBOOT_CAUSE_%s\r\n", RebootCauseNameItem[at_get_power_on()]);
		send_urc_to_ext_NoCache(at_str, strlen(at_str));
		xy_free(at_str);
	}

	if (g_softap_fac_nv->g_NPSMR_enable == 1)
		send_urc_to_ext_NoCache("\r\n+NPSMR:0\r\n", strlen("\r\n+NPSMR:0\r\n"));

	char *dbg_urc = xy_malloc(64);
	snprintf(dbg_urc, 64, "+DBGINFO:BootR %d,SubR 0x%x,PS state %d,REG %x %x\r\n", Get_Boot_Reason(), Get_Boot_Sub_Reason(),g_softap_var_nv->ps_deepsleep_state,HWREG(0x40000000),HWREG(0x40000008));
	send_debug_by_at_uart(dbg_urc);

	xy_free(dbg_urc);

	xy_printf(0,PLATFORM, WARN_LOG, "+DBGINFO:BootR %d,SubR 0x%x,PS state %d,REG %x %x\r\n", Get_Boot_Reason(), Get_Boot_Sub_Reason(),g_softap_var_nv->ps_deepsleep_state,HWREG(0x40000000),HWREG(0x40000008));
}

void send_err_rsp_2_ext(int err_no, at_context_t* ctx, char *file, int line)
{
	char *err_str = at_err_build_info(err_no, file, line);
	ctx->error_no = err_no;

	if (ctx->fd == FARPS_AP_ASYNC || ctx->fd == FARPS_AP_SYNC || ctx->fd == FARPS_BLE_FD)
	{
		at_write_to_AP(ctx->fd, err_str, strlen(err_str), 1);
	}
	else if(ctx->fd == FARPS_AP_EXT)
	{
		write_to_at_uart(err_str, strlen(err_str));
		/* 清除AP接收AT核间标志位 */
		set_at_lpuart_state(0);
	}
	xy_free(err_str);
}

extern int g_have_sent_downurc;
void icm_at_msg_recv(unsigned int icm_id, zero_msg_t *recv_buf)
{
	unsigned int recvdata_size = 0;
	char *recv_addr_trans = NULL;
	char *recvdata_addr = NULL;

	/*只要收到新的AT命令请求，就必须深睡之前再报DOWN命令给外部*/
	g_have_sent_downurc = 0;

	recvdata_size = recv_buf->size;

	if ((recvdata_size == 0) || (recv_buf->buf == NULL))
		xy_assert(0);

	recv_addr_trans = (char *)Address_Translation_AP_To_CP((unsigned int)(recv_buf->buf));

	recvdata_addr = xy_malloc2(recvdata_size + 1); 
	if (recvdata_addr != NULL)
	{
		//使用memcpy防止接收到的数据没有结束符导致使用strcpy造成踩内存，或接收到的数据结束符被踩再使用strcpy导致重复踩内存
		memcpy(recvdata_addr, recv_addr_trans, recvdata_size);
		*(recvdata_addr + recvdata_size) = '\0';
		shm_msg_write(&(recv_buf->buf), 4, ICM_MEM_FREE);

		if (icm_id == ICM_AT_ASYNC)
			at_recv_from_ap(FARPS_AP_ASYNC, recvdata_addr, recvdata_size);
		else if (icm_id == ICM_AT_SYNC)
			at_recv_from_ap(FARPS_AP_SYNC, recvdata_addr, recvdata_size);
		else
			at_recv_from_ap(FARPS_AP_EXT, recvdata_addr, recvdata_size);
	}
	else  /*堆内存不足，直接丢弃AT命令，并报错*/
	{
		shm_msg_write(&(recv_buf->buf), 4, ICM_MEM_FREE);

		if (icm_id == ICM_AT_ASYNC)
			SEND_ERR_RSP_TO_EXT(ATERR_NO_MEM, &ap_async_ctx);
		else if (icm_id == ICM_AT_SYNC)
			SEND_ERR_RSP_TO_EXT(ATERR_NO_MEM, &ap_sync_ctx);
		else
			SEND_ERR_RSP_TO_EXT(ATERR_NO_MEM, &ap_ext_ctx);
	}
}

/*通过核间消息发送AT给AP核。模组形态，CP核直接写LPUART，不会发送AT命令给AP核，尤其是URC*/
void at_write_to_AP(AT_SRC_FD at_fd, char *buf, unsigned int len, uint8_t isResult)
{
	zero_msg_t response_data = {0};
	char *at_response;
	xy_assert(len != 0);

	//模组版本禁止往AP核异步通道发AT，避免发生URC风暴导致AP内存耗尽死机
	if(!Is_OpenCpu_Ver() && at_fd == FARPS_AP_ASYNC)
		return;

	/*OPENCPU产品，AP核用户不感兴趣的URC，无需发送给AP核*/
	if(isResult!=1 && urc_filter(buf)==1)
		return;
	
	/* 申请内存失败，URC直接丢弃, 若携带结果码则返回8008错误 */
	at_response = xy_malloc2(len + 1);
	if (at_response == NULL)
	{
		xy_printf(0, PLATFORM, WARN_LOG, "at_write_to_AP malloc %d fail!",len);
		if (isResult)
		{
			at_response = AT_ERR_BUILD(ATERR_NO_MEM);
			len = strlen(at_response);
			goto SEND_TO_AP;
		}
		else
		{
			return;
		}
	}
		
	/*构建CP核随机丢应答结果场景，伪造AP核等AT应答超时的异常*/
	if(HWREGB(BAK_MEM_OPEN_TEST) && (xy_rand()%HWREGB(BAK_MEM_OPEN_TEST)==0))
	{
		xy_free(at_response);
		return;
	}
	
	//使用memcpy防止接收到的数据没有结束符导致使用strcpy造成踩内存，或接收到的数据结束符被踩再使用strcpy导致重复踩内存
	memcpy(at_response, buf, len);
	*(at_response+len) = '\0';

SEND_TO_AP:
	xy_cache_writeback(at_response,len);

	response_data.buf = (void *)Address_Translation_CP_To_AP((unsigned int)at_response);
	response_data.size = len;

	void *ap_addr = response_data.buf;

	add_zero_copy_sum((uint32_t)ap_addr, (unsigned int)len);
	if(at_fd == FARPS_AP_ASYNC)
		shm_msg_write(&response_data,sizeof(zero_msg_t), ICM_AT_ASYNC);
	else if(at_fd == FARPS_AP_SYNC)
		shm_msg_write(&response_data,sizeof(zero_msg_t), ICM_AT_SYNC);
	else
		xy_assert(0);
}

void send_powerdown_urc_to_ext(void *buf, uint32_t size)
{
	if(HWREGB(BAK_MEM_DROP_URC) != 2)
	{	
		write_to_uart_for_Dslp((char *)buf, size);
	}

    if (Is_OpenCpu_Ver())
	{
		shm_msg_write(buf, size+1, ICM_COPY_AT);
	}
	return;
}

void send_urc_to_ext_NoCache(void *buf, uint32_t size)
{
	if(HWREGB(BAK_MEM_DROP_URC) != 2)
	{
		write_to_at_uart((char *)buf, size);
	}

	if(HWREGB(BAK_MEM_DROP_URC) != 1)
	{
		at_write_to_AP(FARPS_AP_ASYNC, (char *)buf, size, 0);
	}
	return;
}

/*仅LPUART和AP核异步AT命令两个通道支持URC缓存*/
void at_add_urc_cache(at_context_t *ctx, char *urc, uint32_t size)
{
	xy_assert(urc != NULL && ctx != NULL && ctx->urcMutex != NULL);
    osMutexAcquire(ctx->urcMutex, osWaitForever);
    xy_printf(0, PLATFORM, WARN_LOG, "at[%d] urc cache add size:%d", ctx->fd, size);
    if (ctx->urcData == NULL)
    {
        ctx->urcData = xy_malloc(sizeof(at_urc_cache_t));
        ctx->urcData->urc = xy_malloc2(size);
		if (ctx->urcData->urc == NULL)
		{
			xy_printf(0, PLATFORM, WARN_LOG, "at[%d] urc cache no mem:%d", ctx->fd, size);
			xy_free(ctx->urcData);
			ctx->urcData = NULL;
			goto CACHE_END;
		}
		ctx->urcData->urc_size = size;
		memcpy(ctx->urcData->urc, urc, size);
		ctx->urcData->next = NULL;
		goto CACHE_END;
    }

    at_urc_cache_t* tmp = ctx->urcData;
    while (tmp->next != NULL)
    {
        tmp = tmp->next;
    }

    at_urc_cache_t* new = xy_malloc(sizeof(at_urc_cache_t));
    new->urc = xy_malloc2(size);
	if (new->urc == NULL)
	{
		xy_printf(0, PLATFORM, WARN_LOG, "at[%d] urc cache no mem:%d", ctx->fd, size);
		xy_free(new);
		goto CACHE_END;
	}
	new->urc_size = size;
    memcpy(new->urc, urc, size);
    new->next = NULL;
    tmp->next = new;
CACHE_END:
    osMutexRelease(ctx->urcMutex);
    return;
}

/*仅LPUART和AP核异步AT命令两个通道支持URC缓存*/
void at_report_urc_cache(at_context_t *ctx)
{
	osMutexAcquire(ctx->urcMutex, osWaitForever);
	if (ctx->urcData == NULL || (ctx->state & RCV_REQ_NOW))
	{
		osMutexRelease(ctx->urcMutex);
		return;
	}

	at_urc_cache_t* tmp = ctx->urcData;
    at_urc_cache_t* pre;
    while (tmp != NULL)
    {
        pre = tmp;
        tmp = tmp->next;
        
        if (pre->urc != NULL)
        {
            xy_printf(0, PLATFORM, WARN_LOG, "at[%d] urc cache report size:%d", ctx->fd, pre->urc_size);
            if (ctx->fd == FARPS_AP_ASYNC)
			{
				if(HWREGB(BAK_MEM_DROP_URC) != 1)
				{
					at_write_to_AP(FARPS_AP_ASYNC, (char *)pre->urc, pre->urc_size, 0);
				}
			}
			else
			{
				if(HWREGB(BAK_MEM_DROP_URC) != 2)
				{
				 	write_to_at_uart((char *)pre->urc, pre->urc_size);
				}
			}
            xy_free(pre->urc);
        }
        xy_free(pre);
    }

    ctx->urcData = NULL;
    osMutexRelease(ctx->urcMutex);
    return;
}
