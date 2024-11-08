/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_com.h"
#include "at_context.h"
#include "at_ctl.h"
#include "xy_at_api.h"
#include "xy_rtc_api.h"
#include "xy_system.h"
#include "xy_fota.h"

/*******************************************************************************
 *						   Global variable definitions						   *
 ******************************************************************************/
char g_req_type = AT_CMD_REQ;
at_context_t * g_at_proxy_ctx = NULL;

/*******************************************************************************
 *						   Local variable definitions						   *
 ******************************************************************************/
osTimerId_t at_wait_farps_rsp_timer = NULL;

/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/


/*仅供内部调试用*/
static void wait_nearps_rsp_timeout_proc(osTimerId_t timerId)
{
    xy_assert(timerId != NULL);
	xy_printf(0,PLATFORM, WARN_LOG, "at_send_wait_rsp with timer: [%p] timeout!!!", timerId);

    osMessageQueueId_t queue_Id = at_related_queue_4_user(timerId);

    xy_assert(queue_Id != NULL);

    //超时重置forward context,避免超时定时器生效同时协议栈也上报返回，at_ctl处理协议栈上报时因为fwd_ctx指向异常而踩内存
    at_context_t *fwd_ctx = search_at_context(NEARPS_CP_FD);
    if(fwd_ctx != NULL && fwd_ctx->fwd_ctx != NULL)
    {
        reset_ctx(fwd_ctx);
    }

    char *data = AT_ERR_BUILD(ATERR_WAIT_RSP_TIMEOUT);
    unsigned int size = strlen(data);
    struct at_fifo_msg *msg = xy_malloc(sizeof(struct at_fifo_msg) + size + 1);
    memcpy(msg->data, data, size);
    osMessageQueuePut(queue_Id, &msg, 0, osWaitForever);
    xy_free(data);
    return;
}


static char *find_first_print_char(char *at_str, int len)
{
	char *str = at_str;
	while ((*str < '!' || *str > '}') && str < at_str + len)
	{
		str++;
	}
	return (str < at_str + len ? str : NULL);
}

/*仅供内部调试使用*/
static int get_handle_rst_vp(osMessageQueueId_t rcv_fifo, char *prefix, char *info_fmt, const va_list* vp)
{
	int errno = 0;
    int ret = -1;
    char *end_tail;
    char *str_param;
	char *maohao=NULL;
    int rf_at = 0;
    do
    {
        struct at_fifo_msg *rcv_msg = NULL;
        osMessageQueueGet(rcv_fifo, &rcv_msg, NULL, osWaitForever);
        char *rsp_at = rcv_msg->data;

        xy_printf(0,PLATFORM, WARN_LOG, "user rsp:%s\r\n", rsp_at);
        if (rsp_at == NULL)
            xy_assert(0);

		if (info_fmt != NULL && prefix != NULL)
		{
            if (at_strcasecmp(prefix, "RF"))
                rf_at = 1;
			//标准URC，前缀匹配成功
	        if (rf_at || ((str_param = at_prefix_strstr(rsp_at, prefix)) != NULL))
	        {
                if (!rf_at)
                {
                    xy_assert(*str_param == ':' || *str_param == '=' || *str_param == '^');
                    str_param++;
                }
                else
                {
                    /* 射频命令中间结果必须和结果码同时返回，并且不携带+RF前缀 */
                    str_param = rsp_at;
                    while (*str_param == '\r' || *str_param == '\n')
                    {
                        str_param++;
                    }
                }
                
	            if ((end_tail = strchr(str_param, '\r')) != NULL)
	                *end_tail = '\0';
	            else
	                xy_assert(0);
                ret = parse_param(info_fmt, str_param, 0, NULL, AT_PARAM_PARSE_DEFAULT, (va_list *)(vp));
                *end_tail = '\r';
	        }
			//不标准的URC，即中间结果没有前缀，检查到没冒号或逗号前没冒号，可以直接解析参数,如3,"do:it"
            else if ((str_param = find_first_print_char(rsp_at, strlen(rsp_at))) != NULL \
                && ((maohao = strchr(str_param, ':')) == NULL || (maohao > strchr(str_param, ','))))
            {
            	char *ok_head = strstr(str_param, "OK\r\n");
            	end_tail = strchr(str_param, '\r');
	            if (ok_head != str_param && end_tail != NULL)
	            {
	                *end_tail = 0;
					ret = parse_param(info_fmt, str_param, 0, NULL, AT_PARAM_PARSE_DEFAULT, (va_list*)(vp));
	            	*end_tail = '\r';
	            }
	            else if (end_tail == NULL)
	                xy_assert(0);
	        }
		}
		//没有中间结果
        if (ret == -1)
        {
            if (Is_AT_Rsp_OK(rsp_at))
                ret = AT_OK;
            else if((errno=Get_AT_errno(rsp_at)) != 0)
                ret = errno;
        }
		//中间结果解析正确，但可能还未收到最终结果码，需要继续接收
        else if (ret == AT_OK && !Is_Result_AT_str(rsp_at))
        {
            ret = -1;
        }
   
        xy_free(rcv_msg);
    
    } while (ret == -1);

    return ret;
}



//对于整形参数，%d后面加()表示参数必选；[]表示参数可选；且参数值一定为正整形，括号内可选通过-来设置上下限值，上限值可缺省
AT_ERRNO_E at_parse_param(char *fmt, char *buf, ...)
{
	int ret = AT_OK;
	va_list vl;
	va_start(vl,buf);

	char *quot_end = NULL;
	while (*buf == ' ')
		buf++;

	//参数被""包住的情况
	if (*buf == '"')
	{
		//跳过左“
		buf++;
		char *first_comma = strchr(buf, ',');
		char *second_quote = strchr(buf, '"');
		if (first_comma != NULL && first_comma < second_quote) //不只一个参数，并且第一个参数无引号，则整个buf被""包住
		{
			//找到右”
			quot_end = strchr(buf + strlen(buf) - 3, '"');
			if (quot_end != NULL)
			{
				*quot_end = 0;
				ret = parse_param(fmt, buf, 0, NULL, AT_PARAM_PARSE_DEFAULT, &vl);
			}
			else
				goto normal_proc;
		}
		else
		{
			normal_proc:
			//回退左",直接解析
			buf--;
			ret = parse_param(fmt, buf, 0, NULL, AT_PARAM_PARSE_DEFAULT, &vl);
		}
	}
	else
		ret = parse_param(fmt, buf, 0, NULL, AT_PARAM_PARSE_DEFAULT, &vl);

	va_end(vl);
	return ret;
}

AT_ERRNO_E at_parse_param_escape(char *fmt, char *buf, int* parse_num, ...)
{
	int ret = AT_OK;
	va_list vl;
	va_start(vl, parse_num);
	*parse_num = 0;
	ret = parse_param(fmt, buf, 0, parse_num, AT_PARAM_PARSE_ESC, &vl);
	va_end(vl);
	return ret;
}

/*xy_proxy线程中调用at_send_wait_rsp接口处理扩展AT命令，必须是同步AT应答，即返回值为AT_END，且中间结果必须与最终结果码连在一起*/
int process_from_at_proxy(char *at_str,char *info_fmt,const va_list* vp)
{
	int i = 0;
	int errno = 0;
	char *rsp_cmd = NULL;
	char *str_param;
	char *end_tail;
	char at_cmd_prefix[AT_CMD_PREFIX] = {0};
	struct at_serv_proc_e *at_basic;
    int rf_at = 0;

    char *at_param = at_get_prefix_and_param(at_str, at_cmd_prefix, &g_req_type);

	if (!strlen(at_cmd_prefix))
	{
		return  -1;
	}
		
	for (i = 0; g_at_basic_req != NULL && (g_at_basic_req + i)->at_prefix != 0; i++)		
	{
		at_basic = g_at_basic_req + i;
		if (at_strcasecmp(at_cmd_prefix, at_basic->at_prefix))
		{
            if (at_strcasecmp(at_cmd_prefix, "RF"))
                rf_at = 1;
            /*要求中间结果和结果码必须体现在rsp_cmd里，否则无法截获解析*/
			int ret = at_basic->proc(at_param, &rsp_cmd);

			/*xy_proxy线程中调用at_send_wait_rsp接口触发AT请求处理，若非3GPP命令，定制化处理，且返回值必须为AT_END,不得再转发二次处理*/
			xy_assert(ret == AT_END);

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
			
			//标准URC，前缀匹配成功
	        if (rf_at || ((str_param = at_prefix_strstr(rsp_cmd, at_cmd_prefix)) != NULL))
	        {
                if (!rf_at)
                {
                    xy_assert(*str_param == ':' || *str_param == '=' || *str_param == '^');
                    str_param++;
                }
                else
                {
                    /* 射频命令中间结果必须和结果码同时返回，并且不携带+RF前缀 */
                    str_param = rsp_cmd;
                    while (*str_param == '\r' || *str_param == '\n')
                    {
                        str_param++;
                    }
                }

                if ((end_tail = strchr(str_param, '\r')) != NULL)
	                *end_tail = '\0';
	            else
	                xy_assert(0);
                ret = parse_param(info_fmt, str_param, 0, NULL, AT_PARAM_PARSE_DEFAULT, (va_list *)(vp));
                *end_tail = '\r';
	        }
			else
			{
				if (Is_AT_Rsp_OK(rsp_cmd))
	                ret = AT_OK;
	            else if((errno=Get_AT_errno(rsp_cmd)) != 0)
	                ret = errno;
				else
					ret = ATERR_XY_ERR;
			}
			xy_free(rsp_cmd);
			return  ret;
		}
	}
	return  -1;
}



/**
 *替代旧的at_ReqAndRsp_to_ps命令；3GPP相关AT命令，请使用xy_atc_interface_call
 *ram req_at 入参,值为AT命令字符串，例"AT+RF=GRFIT",不可使用局部变量
 *info_fmt   入参,值为AT命令返回值格式，例"%d,%s",不可使用局部变量
 */
int at_send_wait_rsp(char *req_at, char *info_fmt, int timeout, ...)
{
    int at_errno = -1;
    int from_proxy = 0;
    int userFd = AT_FD_INVAIL;
    char *prefix = xy_malloc(AT_CMD_PREFIX);
    at_context_t *ctx = NULL;
    osTimerId_t timer_Id = NULL;
    osMessageQueueId_t queue_Id = NULL;
	osTimerAttr_t timer_attr = {0};
    va_list vp;
    va_start(vp, timeout);

    //Step1: deal with abnormal situation
    xy_printf(0,PLATFORM, WARN_LOG, "user req:%s", req_at);

    //发送AT命令后该接口处于阻塞状态，不可在软定时器和RTC的回调中使用该接口！
    char *curTaskName = (char *)(osThreadGetName(osThreadGetId()));
    xy_printf(0,PLATFORM, WARN_LOG, "current task:%s", curTaskName);
    if (strstr(curTaskName, "Tmr Svc") || strstr(curTaskName, RTC_TMR_THREAD_NAME) ||
        strstr(curTaskName, AT_CTRL_THREAD_NAME))
    {
        xy_assert(0);
    }
    at_get_prefix_and_param(req_at, prefix, NULL);

    if (strcmp(prefix, "") == 0)
    {
        at_errno = ATERR_INVALID_PREFIX;
        goto END_PROC;
    }

    //Step2: get available at context fd for user
    if (strstr(curTaskName, AT_PROXY_THREAD_NAME))
    {
    	/*at_proxy中调用该接口触发平台扩展AT命令，在此处定制处理，但只能处理同步扩展AT命令的应答*/
    	if((at_errno = process_from_at_proxy(req_at,info_fmt,&vp)) != -1)
    	{
    		goto END_PROC;
    	}
        from_proxy = 1;
    }

	timer_attr.name = "wait_nearps";
    timer_Id = osTimerNew((osTimerFunc_t)(wait_nearps_rsp_timeout_proc), osTimerOnce, NULL, &timer_attr);
    xy_assert(timer_Id != NULL);
    xy_printf(0,PLATFORM, WARN_LOG, "create timer: [%p] for user:%s", timer_Id, curTaskName);

    queue_Id = osMessageQueueNew(10, sizeof(void *), NULL);
    xy_assert(queue_Id != NULL);
    xy_printf(0,PLATFORM, WARN_LOG, "create queue: [%p] for user:%s", queue_Id, curTaskName);

    //Step3: create a context associated with current task
    ctx = get_avail_atctx_4_user(from_proxy);
    xy_assert(ctx != NULL);
    userFd = ctx->fd;
    xy_printf(0,PLATFORM, WARN_LOG, "get available at fd: [%d] for user", userFd);
    ctx->user_queue_Id = queue_Id;
    ctx->user_ctx_tmr = timer_Id;

    //Step5: start timer here
    at_errno = -1;
    if (timeout < 30)
    {
        timeout = 30;
    }
    osTimerStart(timer_Id, timeout * 1000);
    
    //Step6: clear queue and send msg to at ctl
    at_errno = send_msg_2_atctl(AT_MSG_RCV_STR_FROM_FARPS, req_at, strlen(req_at), ctx);
    xy_assert(at_errno == AT_OK);
    at_errno = get_handle_rst_vp(queue_Id, prefix, info_fmt, &vp);
    xy_printf(0,PLATFORM, WARN_LOG, "USER_RSP:%d\r\n", at_errno);
    xy_assert(at_errno != -1);

END_PROC:
    //Step1: delete the related queue
    if (queue_Id != NULL)
    {
        xy_printf(0,PLATFORM, WARN_LOG, "delete queue fd: [%p]", queue_Id);
    	void *elem = NULL;
        while (osMessageQueueGet(queue_Id, &elem, NULL, 0) == osOK)
        {
        	xy_free(elem);
        }
    	osMessageQueueDelete(queue_Id);
        queue_Id = NULL;
    }
    //Step2: delete the related timer
    if (timeout > 0 && timer_Id != NULL)
    {
        osTimerDelete(timer_Id);
        timer_Id = NULL;
    }
    //Step3: delete the user context which register in the global context dictionary
    if (ctx != NULL && userFd >= FARPS_USER_MIN && userFd <= FARPS_USER_MAX)
    {
        deregister_at_context(userFd);
        xy_free(ctx);
        ctx = NULL;
    }
    //Step4: if err occured,notify ext,such as uart or ap core
    if (at_errno != AT_OK)
    {
		char *at_str = xy_malloc(32);
        snprintf(at_str, 32, "\r\n+DBGINFO:AT_ERROR:%d\r\n", at_errno);
        send_debug_by_at_uart(at_str);
		xy_free(at_str);
    }
    va_end(vp);
	xy_free(prefix);
    return at_errno;
}

void send_rsp_at_to_ext(void *data)
{
    if (strstr(osThreadGetName(osThreadGetId()), AT_PROXY_THREAD_NAME))
    {
        /* at proxy线程中不得调用,而应该通过出参返回给AT框架进行发送 */
        xy_assert(0);
    }
    send_msg_2_atctl(AT_MSG_RCV_STR_FROM_NEARPS, data, strlen(data), &user_app_ctx);
}

char *at_err_build_info(int err_no, char *file, int line)
{
    UNUSED_ARG(line);
	char *at_str = xy_malloc(64);

	if ((get_cmee_mode() == 0) || (err_no == 0))
		snprintf(at_str, 64, "\r\nERROR\r\n");
	else if (get_cmee_mode() == 1)
		snprintf(at_str, 64, "\r\n+CME ERROR:%d\r\n", err_no);
	else
		snprintf(at_str, 64, "\r\n+CME ERROR:%s\r\n", get_at_err_string(err_no));

	if (HWREGB(BAK_MEM_XY_DUMP) == 1)
	{
		char xy_file[20] = {0};
		if (strlen(file) >= 20)
			memcpy(xy_file, file + strlen(file) - 19, 19);
		else
			memcpy(xy_file, file, strlen(file));
		xy_printf(0,PLATFORM, WARN_LOG,"DebugInfo:%s,line:%d,file:%s\r\n", get_at_err_string(err_no), line, xy_file);
	}
	return at_str;
}


typedef struct
{
    int sys_err;
    int at_err;
} err_sys_2_at_t;

//system错误码与AT返回值匹配
static const err_sys_2_at_t sys_at_err_table[] = {
    {XY_OK                 , AT_OK},
    {XY_ERR                , ATERR_XY_ERR},
    {XY_Err_Timeout        , ATERR_WAIT_RSP_TIMEOUT},
    {XY_Err_NoConnected    , ATERR_NOT_NET_CONNECT},
    {XY_Err_Parameter      , ATERR_PARAM_INVALID},
    {XY_Err_NoMemory       , ATERR_XY_ERR},
    {XY_Err_NotAllowed     , ATERR_NOT_ALLOWED},
    {XY_Err_LowVbat        , ATERR_XY_ERR},
    {XY_Err_Reserved       , ATERR_XY_ERR},
};

//根据系统错误码获取AT返回值
AT_ERRNO_E at_get_errno_by_syserr(xy_ret_Status_t err_code)
{
    int i;
    for (i = 0; i < (int)(sizeof(sys_at_err_table) / sizeof(sys_at_err_table[0])); ++i)
    {
        if (sys_at_err_table[i].sys_err == err_code)
            return sys_at_err_table[i].at_err;
    }

    return ATERR_XY_ERR;
}
