/**
 * @file at_proxy.c
 * @brief 
 * @version 1.0
 * @date 2022-07-07
 * @copyright Copyright (c) 2022  芯翼信息科技有限公司
 * 
 */

#include "xy_system.h"
#include "at_config.h"
#include "at_ctl.h"
#include "at_context.h"

typedef struct at_proxy_msg
{
    int id;
    int size;       //消息数据长度
    char data[0];   //消息数据
} at_proxy_msg_t;

/*******************************************************************************
 *						   Global variable definitions				           *
 ******************************************************************************/
osMessageQueueId_t at_proxy_msg_q = NULL;
osMutexId_t at_proxy_mutex = NULL;


bool proc_at_proxy_req(at_proxy_msg_t *msg)
{
	//Step1: 参数检测
	xy_assert(msg != NULL && msg->data != NULL);
	at_msg_t *at_msg = (at_msg_t *)msg->data;
	at_context_t *proxy_ctx = at_msg->ctx;
	xy_assert(proxy_ctx != NULL && proxy_ctx->at_proc != NULL);
	g_at_proxy_ctx = proxy_ctx;
	g_req_type = proxy_ctx->at_type;
	char *rsp_cmd = NULL;

	//Step2: 处理at命令
	int ret = proxy_ctx->at_proc((at_msg->data + at_msg->offset), &rsp_cmd);
	xy_printf(0,PLATFORM, WARN_LOG, "proxy task deal with [%s] done", proxy_ctx->at_cmd_prefix);

	//Step3: 如果需3gpp处理，转发给PS
	if (AT_FORWARD == ret)
	{
		forward_req_at_proc(at_msg);
	}
	else if (AT_ASYN == ret)
	{
		//参考forward_req_at_proc,将原始上下文信息赋予NEARPS_USER_FD对应的at上下文fwd_ctx字段
		at_context_t *fwd_ctx = search_at_context(NEARPS_USER_FD);
		xy_assert(fwd_ctx != NULL);
		//获取fwd->ctx的前缀，需注意，原始AT命令的前缀在此前已解析并做过非0判断，此处无需再次解析和判断，直接memcpy即可
		memcpy(fwd_ctx->at_cmd_prefix, at_msg->ctx->at_cmd_prefix, strlen(at_msg->ctx->at_cmd_prefix));
		proxy_ctx->fwd_ctx = fwd_ctx;
		fwd_ctx->fwd_ctx = proxy_ctx;
	}
	else
	{
		//框架组应答AT命令
		if (rsp_cmd == NULL)
		{
			//若返回错误码，则框架自行组装“+ERROR:”
			if (ret > AT_ROUTE_MAX)
				rsp_cmd = AT_ERR_BUILD(ret);
			else
			{
				xy_assert(ret == AT_END);
				rsp_cmd = at_ok_build();
			}
		}
		/*具体AT命令应答时，尾部不添加\r\n，则由框架自动添加前缀头和OK结果码*/
		else if(strstr(rsp_cmd+strlen(rsp_cmd)-2,"\r\n") == NULL)
		{
			char *temp = xy_malloc(strlen(rsp_cmd)+strlen(proxy_ctx->at_cmd_prefix)+15);

			/*有前缀，仅尾部自动添加OK。通常用于前缀不规则的URC上报*/
			if(*rsp_cmd=='\r')
			{
				sprintf(temp,"%s\r\n\r\nOK\r\n",rsp_cmd);
			}
			/*添加头部\r\n和尾部OK，通常用于冒号后无空格的特殊命令*/
			else if(*rsp_cmd=='+')
			{
				sprintf(temp,"\r\n%s\r\n\r\nOK\r\n",rsp_cmd);
			}
			else   /*添加前缀和OK，标准的URC上报*/
			{
				sprintf(temp,"\r\n+%s: %s\r\n\r\nOK\r\n",proxy_ctx->at_cmd_prefix,rsp_cmd);
			}
			
			xy_free(rsp_cmd);
			rsp_cmd = temp;
		}

		//Step5: 调用farps_write接口将at返回信息输出到uart或者ap
		at_write_by_ctx(proxy_ctx, rsp_cmd, strlen(rsp_cmd));
	}

	if (rsp_cmd != NULL)
		xy_free(rsp_cmd);

	return 1;
}

void at_proxy_proc(void)
{
    at_proxy_msg_t *msg = NULL;
    while (1)
    {
        osMessageQueueGet(at_proxy_msg_q, (void *)(&msg), NULL, osWaitForever);
        switch (msg->id)
        {
        case AT_PROXY_MSG_CMD_PROC:
            proc_at_proxy_req((at_proxy_msg_t *)msg);
            break;
        default:
            break;
        }
        xy_free(msg);
    }
}

void send_msg_2_at_proxy(int id, void *buff, int len)
{
	xy_mutex_acquire(at_proxy_mutex, osWaitForever);
	xy_assert(at_proxy_msg_q != NULL);

	at_proxy_msg_t *msg = NULL;
	msg = xy_malloc(sizeof(at_proxy_msg_t) + len);
	msg->id = id;
	msg->size = len;

	if (buff != NULL)
		memcpy(msg->data, buff, len);
	osMessageQueuePut(at_proxy_msg_q, &msg, 0, osWaitForever);
	xy_mutex_release(at_proxy_mutex);
	return;
}

void at_proxy_init(void)
{
	at_proxy_msg_q = osMessageQueueNew(10, sizeof(void *), NULL);
	at_proxy_mutex = osMutexNew(NULL);
	
	osThreadAttr_t thread_attr = {0};
	thread_attr.name        = AT_PROXY_THREAD_NAME;
	thread_attr.priority    = AT_PROXY_THREAD_PRIO;
	thread_attr.stack_size  = AT_PROXY_THREAD_STACKSIZE;
	osThreadNew((osThreadFunc_t)(at_proxy_proc), NULL, &thread_attr);
}