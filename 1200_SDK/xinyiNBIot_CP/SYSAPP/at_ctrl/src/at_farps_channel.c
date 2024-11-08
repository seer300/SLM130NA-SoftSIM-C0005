/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_com.h"
#include "at_config.h"
#include "at_ctl.h"
#include "at_uart.h"
#include "factory_nv.h"
#include "ipc_msg.h"
#include "low_power.h"
#include "oss_nv.h"
#include "xy_at_api.h"
#include "xy_passthrough.h"
#include "xy_rtc_api.h"
#include "xy_system.h"
#include "at_worklock.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define RECVED_TAIL_CHAR(ctx) 		(*(ctx->g_farps_rcv_mem + ctx->g_have_rcved_len - 1))
#define AT_RX_PULL_ERR_THRESHOLD 	(5000)

/*******************************************************************************
 *						  Local variable definitions						   *
 ******************************************************************************/

/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
int realloc_farps_rcv_mem(int len, at_context_t *ctx)
{
	char *new_mem;
	xy_assert(ctx->g_farps_rcv_mem != NULL);

	new_mem = xy_malloc(len);
	if (ctx->g_have_rcved_len != 0)
		memcpy(new_mem, ctx->g_farps_rcv_mem, ctx->g_have_rcved_len);
	xy_free(ctx->g_farps_rcv_mem);
	ctx->g_farps_rcv_mem = new_mem;
	ctx->g_rcv_buf_len = len;
	return 0;
}


bool single_atcmd_proc(at_context_t *ctx)
{
	/*每接收到一条AT命令，都必须更新启动延迟锁。否则云通信中有sleep操作会释放调度进入idle深睡*/
	if(g_softap_fac_nv->deepsleep_delay!=0 && !at_strncasecmp(ctx->g_farps_rcv_mem, "AT+NPSMR?", strlen("AT+NPSMR?")))
		at_delaylock_act();
	
	//debug打印,用于确认at命令接收情况。
	xy_printf(0, PLATFORM, WARN_LOG, "at recv from fd:%d,len:%d,%s", ctx->fd, ctx->g_have_rcved_len, ctx->g_farps_rcv_mem);

	if (ctx->state & RCV_REQ_NOW)
	{
		SEND_ERR_RSP_TO_EXT(ATERR_CHANNEL_BUSY, ctx);
		char *atstr = xy_malloc(80);
		char new_prefix[10] = {0};
		memcpy(new_prefix, ctx->g_farps_rcv_mem, 9);
		if (strlen(ctx->at_cmd_prefix) != 0)
			snprintf(atstr, 80, "\r\n+DBGINFO:FARPS BUSY:%s,new:%s,fd:%d\r\n", ctx->at_cmd_prefix, new_prefix, ctx->fd);
		else
			snprintf(atstr, 80, "\r\n+DBGINFO:at_ctl block,new:%s,fd:%d\r\n", new_prefix, ctx->fd);

		send_debug_by_at_uart(atstr);
		xy_free(atstr);
		return 0;
	}

	ctx->state |= RCV_REQ_NOW;

	//发送给at_ctl线程处理，g_farps_rcv_mem指向at数据字符串，末尾有'\0',g_have_rcved_len不包含'\0'
	send_msg_2_atctl(AT_MSG_RCV_STR_FROM_FARPS, ctx->g_farps_rcv_mem, ctx->g_have_rcved_len, ctx);

	return 1;
}

int atstr_correct_check(at_context_t *ctx)
{
	if(ctx->g_have_rcved_len < 3)
		return 0;
	return 1;
}

char *find_end_char(at_context_t *ctx)
{
	//\r结尾
	if(*(ctx->g_farps_rcv_mem + ctx->g_have_rcved_len-1) == '\r')
		return  ctx->g_farps_rcv_mem + ctx->g_have_rcved_len-1;
	//\r\n结尾，将\n去除
	else if(*(ctx->g_farps_rcv_mem + ctx->g_have_rcved_len-2) == '\r')
	{
		*(ctx->g_farps_rcv_mem + ctx->g_have_rcved_len-1) = '\0';
		ctx->g_have_rcved_len = ctx->g_have_rcved_len-1;
		return  ctx->g_farps_rcv_mem + ctx->g_have_rcved_len-1;
	}
	else
		return  NULL;
}


void at_recv_from_log(uint32_t id,char *buf, unsigned int data_len)
{
	at_context_t *ctx = search_at_context(id);
	xy_assert(ctx != NULL);

	//透传模式
	if (g_at_passthr_hook != NULL && g_at_passthr_hook(buf, data_len) == 1)
		return;
	
	/* 以\r作为尾部特征，进而后续尾部的\n要丢弃。另外有些外部at工具命令结束符为\r\n\0, 因此也需要考虑\0的情况 */
	if (ctx->g_have_rcved_len == 0 && *buf == '\n')
	{		
		if (data_len < 3)
		{
			/* 数据小于3直接丢弃 */
			return;
		}
		else
		{
			buf++;
			data_len--;
		}
	}

	if (ctx->g_farps_rcv_mem == NULL)
	{
		ctx->g_farps_rcv_mem = xy_malloc(data_len + 1);
		ctx->g_rcv_buf_len = data_len + 1;
	}

	if (ctx->g_have_rcved_len + data_len > AT_RX_PULL_ERR_THRESHOLD)
	{
		xy_printf(0, PLATFORM, WARN_LOG, "AT rx pin pull down,and recv all zero!!!ERROR!!!");
		send_debug_by_at_uart("+DBGINFO:AT RX PIN pull down ERROR\r\n");
		goto END_PROC;
	}

	if ((unsigned long)(ctx->g_rcv_buf_len) < (unsigned long)(ctx->g_have_rcved_len) + data_len + 1)
		realloc_farps_rcv_mem(ctx->g_rcv_buf_len + data_len, ctx);
	memcpy(ctx->g_farps_rcv_mem + ctx->g_have_rcved_len, buf, data_len);
	ctx->g_have_rcved_len += data_len;
	*(ctx->g_farps_rcv_mem + ctx->g_have_rcved_len) = '\0';

	if (find_end_char(ctx) != NULL)
	{
		//对AT命令前缀进行小写改大写，并检查是否为脏数据
		if (atstr_correct_check(ctx) == 0)
		{
			SEND_ERR_RSP_TO_EXT(ATERR_DROP_MORE, ctx);
		}
		else if (ctx->g_have_rcved_len > AT_RECV_MAX_LENGTH)
		{
			SEND_ERR_RSP_TO_EXT(ATERR_PARAM_INVALID, ctx);
		}
		else
		{
			single_atcmd_proc(ctx);
		}
	}
	else
	{	
        return;
	}

END_PROC:
	if (ctx->g_farps_rcv_mem != NULL)
		xy_free(ctx->g_farps_rcv_mem);
	ctx->g_farps_rcv_mem = NULL;
	ctx->g_have_rcved_len = 0;
	ctx->g_rcv_buf_len = 0;
}

void at_recv_from_ap(AT_SRC_FD at_fd, char *buf, unsigned int data_len)
{
	at_context_t *ctx = search_at_context(at_fd);
	xy_assert(ctx != NULL && buf != NULL);


	//透传模式
	if ((ctx->fd == FARPS_AP_ASYNC || ctx->fd == FARPS_AP_EXT) && g_at_passthr_hook != NULL && g_at_passthr_hook(buf, data_len) == 1)
	{
		xy_free(buf);
		return;
	}

	if (ctx->g_farps_rcv_mem == NULL)
	{
		ctx->g_farps_rcv_mem = buf;
		ctx->g_rcv_buf_len = ctx->g_have_rcved_len = data_len;
	}
	else
		xy_assert(0);

	if (find_end_char(ctx) != NULL)
	{
		
		//对AT命令前缀进行小写改大写，并检查是否为脏数据
		if (atstr_correct_check(ctx) == 0)
		{
			SEND_ERR_RSP_TO_EXT(ATERR_DROP_MORE, ctx);
		}
		else if (ctx->g_have_rcved_len > AT_RECV_MAX_LENGTH)
		{
			SEND_ERR_RSP_TO_EXT(ATERR_PARAM_INVALID, ctx);
		}
		else
		{
			single_atcmd_proc(ctx);
		}
	}
	else
	{
		SEND_ERR_RSP_TO_EXT(ATERR_DROP_MORE, ctx);
	}

	xy_free(ctx->g_farps_rcv_mem);
	ctx->g_farps_rcv_mem = NULL;
	ctx->g_have_rcved_len = 0;
	ctx->g_rcv_buf_len = 0;
}

bool at_send_to_log(void* at_ctx, void *buf, int size)
{
	//xy_assert(size!=0 && at_ctx!=NULL && osCoreGetState()!=osCoreInCritical); //不能在锁中断状态下调用该接口！
	at_context_t *ctx = search_at_context(((at_context_t *)(at_ctx))->fd);

	//返回OK或者错误,以及接收到透传模式退出字符的时候处理！
	if (Is_Result_AT_str((char *)buf))
	{
		//update err no
		ctx->error_no = Get_AT_errno(buf);
		reset_ctx(ctx);
	}

	xy_printf(0,PLATFORM, WARN_LOG, "at_send_to_log:%s", (char *)buf);
	diag_at_response_output((char *)buf, size);

	return 1;
}

/*AP核本地的同步和异步两个虚拟AT通道的发送接口*/
bool at_send_to_ap(void* at_ctx, void *buf, int size)
{
	//xy_assert(at_ctx != NULL && size != 0 && osCoreGetState() != osCoreInCritical); //不能在锁中断状态下调用该接口！
	at_context_t *ctx = search_at_context(((at_context_t *)(at_ctx))->fd);
	uint8_t isResult = Is_Result_AT_str((char *)buf);

	if (isResult)
	{
		ctx->error_no = Get_AT_errno((char *)buf);
		reset_ctx(ctx);
	}

	xy_printf(0,PLATFORM, WARN_LOG, "at_send_to_ap:%s,%d", buf, ctx->fd);
	at_write_to_AP(ctx->fd, (char *)buf, size, isResult);

	/* URC缓存上报 */
#if URC_CACHE
	at_report_urc_cache(at_ctx);
#endif /* URC_CACHE */

	return 1;
}

/*only used for sending debug string to AT UART,maybe drop string when in lock int state.
such as "+DBGINFO:",normal log can not send by it*/
void send_debug_by_at_uart(char *buf)
{
	if (HWREGB(BAK_MEM_XY_DUMP) == 0)
		return;

	write_to_at_uart(buf, strlen(buf));
}
