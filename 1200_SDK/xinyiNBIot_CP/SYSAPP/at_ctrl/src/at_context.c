/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_context.h"
#include "at_com.h"
#include "at_ctl.h"
#include "xy_utils.h"

/*******************************************************************************
 *						   Global variable definitions						   *
 ******************************************************************************/

/*PS通道上下文，属于nearps通道属性*/
at_context_t nearps_ctx = {0};

/*应用扩展AT命令服务端上下文，仅适用于AT_ASYN异步扩展AT命令，属于nearps通道属性*/
at_context_t user_app_ctx = {0};

/*AP核蓝牙通道对应的虚拟通道*/
at_context_t ap_ble_ctx = {0};

/*log物理通道应的虚拟通道*/
at_context_t log_ctx = {0};

/*AP核本地的异步API接口对应的虚拟通道*/
at_context_t ap_async_ctx = {0};

/*AP核本地的同步API接口对应的虚拟通道*/
at_context_t ap_sync_ctx = {0};

/*AP核外接的uart物理通道在CP核的虚拟通道，其中AT命令的发送直接在CP核执行写uart物理通道*/
at_context_t ap_ext_ctx = {0};

at_context_dict_t *g_at_context_dict = NULL;
osMutexId_t at_ctx_dict_m = NULL;

/*******************************************************************************
 *						Global function implementations 					   *
 ******************************************************************************/
void reset_ctx(at_context_t *ctx)
{
	memset(ctx->at_cmd_prefix, 0, AT_CMD_PREFIX);
	ctx->at_type = AT_CMD_INVALID;
	ctx->at_proc = NULL;
	ctx->fwd_ctx = NULL;
	if (ctx->at_param != NULL)
	{
		xy_free(ctx->at_param);
		ctx->at_param = NULL;
	}
	ctx->retrans_count = 0;
	g_CombCmd_Doing = 0;
	ctx->state = REQ_IDLE;
}

bool register_at_context(at_context_t *ctx)
{
	xy_assert(ctx != NULL);

	xy_mutex_acquire(at_ctx_dict_m, osWaitForever);

	at_context_dict_t *temp = NULL;
	if (g_at_context_dict == NULL)
	{
		temp = xy_malloc(sizeof(at_context_dict_t));
		temp->pre = NULL;
		temp->node = ctx;
		temp->next = NULL;
		g_at_context_dict = temp;
		xy_mutex_release(at_ctx_dict_m);
		return 1;
	}

	at_context_dict_t *dict = g_at_context_dict;
	if (dict->node->fd == ctx->fd)
	{
		xy_mutex_release(at_ctx_dict_m);
		return 0;
	}
	while (dict->next != NULL)
	{
		if (dict->node->fd == ctx->fd)
		{
			xy_mutex_release(at_ctx_dict_m);
			return 0;
		}
		dict = dict->next;
	};
	temp = xy_malloc(sizeof(at_context_dict_t));
	temp->node = ctx;
	temp->pre = dict;
	temp->next = NULL;
	dict->next = temp;
	xy_mutex_release(at_ctx_dict_m);
	return 1;
}

bool deregister_at_context(int fd)
{
	xy_assert(g_at_context_dict != NULL &&
			  ((fd >= AT_XY_FD_MIN && fd < AT_XY_FD_MAX) || (fd >= FARPS_USER_MIN && fd <= FARPS_USER_MAX)));

	osMutexAcquire(at_ctx_dict_m, osWaitForever);
	at_context_dict_t *temp = NULL;
	temp = g_at_context_dict;
	do
	{
		if (temp->node->fd == fd)
		{
			xy_printf(0,PLATFORM, WARN_LOG, "deregister at context fd[%d] from at dict", fd);
			if (temp->next != NULL)
			{
				temp->next->pre = temp->pre;
			}
			temp->pre->next = temp->next;
			xy_free(temp);
			temp = NULL;
			osMutexRelease(at_ctx_dict_m);
			return 1;
		}
		else
		{
			temp = temp->next;
		}

	} while (temp != NULL);

	osMutexRelease(at_ctx_dict_m);
	return 0;
}

at_context_t *search_at_context(int fd)
{
	osMutexAcquire(at_ctx_dict_m, osWaitForever);

	xy_assert(g_at_context_dict != NULL &&
			  ((fd >= AT_XY_FD_MIN && fd < AT_XY_FD_MAX) || (fd >= FARPS_USER_MIN && fd <= FARPS_USER_MAX)));

	at_context_dict_t *temp = NULL;
	temp = g_at_context_dict;
	do
	{
		if (temp->node->fd == fd)
		{
			osMutexRelease(at_ctx_dict_m);
			return temp->node;
		}
		else
		{
			temp = temp->next;
		}
	} while (temp != NULL);

	osMutexRelease(at_ctx_dict_m);
	return NULL;
}

at_context_t* get_avail_atctx_4_user(int from_proxy)
{
	osMutexAcquire(at_ctx_dict_m, osWaitForever);

	at_context_t *ctx = NULL;
	int fd;

	if (from_proxy == 1)
	{
		/*创建at_proxy的上下文*/
		if (search_at_context(FARPS_USER_PROXY) == NULL)
		{
			ctx = xy_malloc(sizeof(at_context_t));
			memset(ctx, 0, sizeof(at_context_t));
			ctx->position = FAR_PS;
			ctx->fd = FARPS_USER_PROXY;
			xy_assert(register_at_context(ctx) != 0);
		}
		goto END_GET_CTX;
	}

	for (fd = FARPS_USER_PROXY + 1; fd <= FARPS_USER_MAX; fd++)
	{
		if (search_at_context(fd) == NULL)
		{
			ctx = xy_malloc(sizeof(at_context_t));
			memset(ctx, 0, sizeof(at_context_t));
			ctx->position = FAR_PS;
			ctx->fd = fd;
			xy_assert(register_at_context(ctx) != 0);
			goto END_GET_CTX;
		}
	}
END_GET_CTX:
	osMutexRelease(at_ctx_dict_m);
	return ctx;
}

osMessageQueueId_t at_related_queue_4_user(osTimerId_t timerId)
{
	osMutexAcquire(at_ctx_dict_m, osWaitForever);

	at_context_dict_t *temp = NULL;
	temp = g_at_context_dict;
	do
	{
		if (temp->node->user_ctx_tmr == timerId)
		{
			osMutexRelease(at_ctx_dict_m);
			return temp->node->user_queue_Id;
		}
		else
		{
			temp = temp->next;
		}
	} while (temp != NULL);

	osMutexRelease(at_ctx_dict_m);
	return NULL;
}

bool is_at_cmd_processing()
{
	if (HWREGB(BAK_MEM_LPUART_USE) == 1 && is_at_lpuart_doing())
	{
		return true;
	}
	else if (ap_async_ctx.state & RCV_REQ_NOW)
	{
		return true;
	}
	else if (ap_sync_ctx.state & RCV_REQ_NOW)
	{
		return true;
	}
	else if (ap_ble_ctx.state & RCV_REQ_NOW)
	{
		return true;
	}
	else
	{
		return false;
	}
}
