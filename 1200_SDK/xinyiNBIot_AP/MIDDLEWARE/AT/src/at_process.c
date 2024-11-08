#include "xy_list.h"
#include "xy_system.h"
#include "sys_ipc.h"
#include "at_process.h"
#include "system.h"
#include "hal_def.h"
#include "ap_watchdog.h"
#include "hw_types.h"
#include "xy_printf.h"
#include "urc_process.h"
#include "xy_cp.h"
#include "at_CP_api.h"
#include "at_uart.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>
#include "at_ipc_process.h"
#include "xy_event.h"
#include "driver_utils.h"

/* CP核发送来同步AT结果码*/
ListHeader_t sync_ipc_at_list = {0};
int g_async_at_doing = 0;

char *Get_AT_Rsp_Until(uint32_t *msec)
{
	void *at_response = NULL;
	AtCmdList_t *use_node = NULL;
	uint32_t start_tick = 0;

	if(msec != NULL)
		start_tick = Get_Tick();
	
	do{
		//为AT_Send_And_Get_Rsp同步接口调用
		if(msec != NULL)
			use_node = (AtCmdList_t *)ListRemove(&sync_ipc_at_list);
		else//异步非阻塞接口使用
			use_node = (AtCmdList_t *)ListRemove(&asyn_ipc_at_list);

		if(use_node != NULL)
		{
			at_response = xy_malloc(use_node->len + 1);
				
			strcpy(at_response, use_node->data);
			xy_free(use_node);
			use_node = NULL;
			break;
		}

		if(msec==NULL ||  Check_Ms_Timeout(start_tick, *msec))  
			break;
	}while(1);

	if(msec != NULL)
		*msec = Convert_Tick_to_Ms((uint32_t)(start_tick + Convert_Ms_to_Tick(*msec) - Get_Tick()));

	return at_response;
}



int parse_param(char *fmt_parm, char *buf, int is_ESC, va_list *vl);

/*发送AT请求，并接收中间上报及结果码；如果用户需要解析具体的参数值，必须赋值urc_param和urc_param_len*/
At_status_type AT_Send_And_Get_Rsp(char *at_req, int timeout, char *urc_prefix, char *rsp_fmt, ...)
{
	int errno = 0;
	int ret = XY_OK;
	char *recv_str = NULL;
	char *rsp_str = NULL;
	char *param_head = NULL;
	uint32_t remain_time = (uint32_t)(timeout * 1000);

	xy_assert(!IS_IRQ_MODE() && timeout!=0);
	ListFreeAll(&sync_ipc_at_list);

	ret = AT_Send_To_CP((void *)at_req, strlen(at_req),AT_FROM_SYNC_API);

	if(ret == XY_OK)
	{
		va_list vl;
		va_start(vl,rsp_fmt);

		while(1)
		{
			if(g_errno != 0)
			{
				ret = g_errno;
				break;
			}
			recv_str = Get_AT_Rsp_Until(&remain_time);

			if(recv_str == NULL)
			{
				ret = XY_ERR_WAIT_RSP_TIMEOUT;
				break;
			}

			rsp_str = recv_str;
			/*跳过头部字符*/
			while(*rsp_str=='\r' || *rsp_str=='\n' || *rsp_str==' ')
				rsp_str++;
			
			if(urc_prefix == NULL && Is_AT_Rsp_OK(rsp_str))
			{
				break;
			}
			else if((errno = Get_AT_errno(rsp_str)))
			{
				ret = errno;
				break;
			}
			else if(urc_prefix != NULL)
			{
                if ((rsp_fmt != NULL) && (strcmp(rsp_fmt, "%a") == 0 || strcmp(rsp_fmt, "%A") == 0))
                {
                    strcpy((char *)va_arg(vl, int), rsp_str);
                    break;
                }
				else if((param_head = fast_Strstr(rsp_str, urc_prefix)) != NULL)
				{
					if(rsp_fmt != NULL)
					{
						ret = parse_param(rsp_fmt, param_head, 0, &vl);
					}
				}
				/*这两条URC的特点是没有前缀，且第一个字符为数字,直接解析参数*/
#if(VER_BC95 == 1)				
				else if(isdigit((int)*rsp_str) && (at_strstr(urc_prefix,"+NSORF:")||at_strstr(urc_prefix,"+NSOCR:")||at_strstr(urc_prefix,"+CIMI:")))
#else
				else if(isdigit((int)*rsp_str) && (at_strstr(urc_prefix,"+NSORF:")||at_strstr(urc_prefix,"+NSOCR:")))
#endif				
				{
					if(rsp_fmt != NULL)
					{
						ret = parse_param(rsp_fmt, rsp_str, 0, &vl);
					}
				}
				//识别到OK才退出，防止URC到了OK还没到又发送下一条命令
				if(Is_AT_Rsp_OK(rsp_str))
				{
					break;	
				}
			}
			xy_free(recv_str);
		}
		if(recv_str != NULL)
			xy_free(recv_str);
		va_end(vl);
	}
	
	xy_printf("AT_Send_And_Get_Rsp:%s,errno=%d,time=%d",at_req,ret,(int)(Get_Tick()/1000));
	return ret;
}

/* 1表示等待对端AT应答结果超时 */
int  g_Rsp_Timeout = 0;

__RAM_FUNC void Wait_Rsp_Timeout(void)
{
	xy_printf("Wait_Rsp_Timeout:%d\n",(int)(Get_Tick()/1000));
	g_Rsp_Timeout = 1;

}

void Set_Wait_Rsp_Timeout(int timeout_sec)
{
	xy_assert(timeout_sec != 0);
	
	DisableInterrupt();		//防止设置瞬间Time超时改变g_Rsp_Timeout的值
	g_Rsp_Timeout = 0;
	Timer_AddEvent(TIMER_WAIT_AT_RSP,(timeout_sec*1000),Wait_Rsp_Timeout, 0);
	EnableInterrupt();
	
}

At_status_type Send_AT_Req(char *at_req, int timeout_sec)
{

	At_status_type ret = XY_OK;

	/*防止之前链表中有残留的AT命令*/
	ListFreeAll(&asyn_ipc_at_list);

	/*仅用于OPENCPU场景。涉及CP核总体硬件的操作，只能放在AP核此处来执行。且当前不支持回复"\r\nOK\r\n",若需要回复，需要定制下*/
	if(CP_Special_AT_Proc(at_req) == 1)
		return  XY_OK;
	
	ret = AT_Send_To_CP((void *)at_req, strlen(at_req),AT_FROM_ASYN_API);
	
	xy_printf("Send_AT_Req:%s",at_req);

	if(ret != XY_OK)
	{
		xy_printf("ret=%d,time=%d",ret,(int)(Get_Tick()/1000));
		return  ret;
	}

    g_async_at_doing = 1;
	
	if(timeout_sec != 0)
    	Set_Wait_Rsp_Timeout(timeout_sec);
	
	return  XY_OK;
}

/*仅发送透传数据*/
At_status_type Send_Passthr_Data_Req(void *data_addr, uint32_t datalen, int timeout_sec)
{
	xy_printf("Passthrough %d data to CP\r\n", (int)datalen);
	At_status_type ret = AT_Send_To_CP(data_addr, datalen,AT_FROM_ASYN_API);

	if(ret != XY_OK)
		return ret;

	g_async_at_doing = 1;
	DisableInterrupt();
	g_Rsp_Timeout = 0;
	Timer_AddEvent(TIMER_WAIT_AT_RSP,(timeout_sec*1000),Wait_Rsp_Timeout, 0);
	EnableInterrupt();
	return  XY_OK;
}

//返回XY_NO_RSP表示尚未等待CP核应答结果，需要退出到main主线程继续执行
At_status_type Get_AT_Rsp(char *urc_prefix, char *rsp_fmt, ...)
{
	int errno = 0;
	int ret = XY_WAITING_RSP;
	if(g_Rsp_Timeout == 1)
	{
		ret = XY_ERR_WAIT_RSP_TIMEOUT;
	}
	else if(g_errno != 0)
		ret = g_errno;
	else
	{
		char *rsp_str = NULL;
		char *param_head = NULL;
		while((rsp_str = Get_AT_Rsp_Until(NULL)) != NULL)
		{
			xy_printf("Get_AT_Rsp:%s",rsp_str);
			
			//识别OK
			if(urc_prefix == NULL && Is_AT_Rsp_OK(rsp_str))
			{
				ret = XY_OK;
				break;
			}
			else if((errno = Get_AT_errno(rsp_str)))
			{
				ret = errno;
				break;
			}
			//匹配URC中间结果
			else if(urc_prefix != NULL)
			{
				if((param_head = fast_Strstr(rsp_str, urc_prefix)) != NULL)
				{
					if(rsp_fmt != NULL)
					{
						va_list vl;
						va_start(vl,rsp_fmt);
						if(!strcmp(rsp_fmt, "%a"))
						{
							*((int *)(va_arg(vl, int))) = (int)rsp_str;
							ret = XY_OK;
							goto END;
						}
						else
						{
							ret = parse_param(rsp_fmt, param_head, 0, &vl);
						}
						va_end(vl);
					}
					else
					{
						ret = XY_OK;
					}
					break;
				}
			}
			xy_free(rsp_str);
		}

		if(rsp_str != NULL)
			xy_free(rsp_str);
	}

END:

	if(ret != XY_WAITING_RSP)
	{
		Timer_DeleteEvent(TIMER_WAIT_AT_RSP);
		ListFreeAll(&asyn_ipc_at_list);
        g_async_at_doing = 0;
    }
	if(ret > XY_OK)
	{
		if(urc_prefix != NULL)
		{
			xy_printf("Get_AT_Rsp  ERROR!!!:%s,errno=%d,time=%d",urc_prefix,ret,(int)(Get_Tick()/1000));
		}
		else
		{
			xy_printf("Get_AT_Rsp  ERROR!!!:errno=%d,time=%d",ret,(int)(Get_Tick()/1000));
		}
	}

	return ret;
}
