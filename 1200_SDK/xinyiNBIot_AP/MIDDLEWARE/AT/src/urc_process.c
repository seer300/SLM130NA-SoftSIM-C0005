/*******************************************************************************
 *如果产品不支持DRX/eDRX模式下的下行远程控制，为了节省内存空间，可以将该文件注释掉，其中
 URC_Regit_Init、Match_URC_Cmd、URC_Process三个函数需要打桩主控，否则编译不过*
 ******************************************************************************/
#include "xy_list.h"
#include "sys_ipc.h"
#include "xy_system.h"
#include "xy_lpm.h"
#include "at_process.h"
#include "system.h"
#include "hal_def.h"
#include "ap_watchdog.h"
#include "hw_types.h"
#include "xy_printf.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include "basic_config.h"
#include "urc_process.h"

#if XY_AT_CTL

typedef void (*urc_proc)(char *urc_paramlist);

typedef struct
{
	char		*at_prefix;
	urc_proc	proc;
}urc_regist_t;

typedef struct
{
	struct List_t	*next;
	urc_proc		proc;
	char			param[0];	//申请内存时需要多申请一个字节来保存'\0'
}UrcList_t;

/* CP核发送来的URC格式的AT命令 */
ListHeader_t urc_list = {0};

__WEAK void URC_NNMI_Proc(char *paramlist)
{
	UNUSED_ARG(paramlist);
}

__WEAK void URC_CTLWRECV_Proc(char *paramlist)
{
	UNUSED_ARG(paramlist);
}
__WEAK void URC_MIPLREAD_Proc(char *paramlist)
{
	UNUSED_ARG(paramlist);
}

__WEAK void URC_MIPLWRITE_Proc(char *paramlist)
{
	UNUSED_ARG(paramlist);
}

__WEAK void URC_MIPLEXECUTE_Proc(char *paramlist)
{
	UNUSED_ARG(paramlist);
}

__WEAK void URC_MIPLPARAMETER_Proc(char *paramlist)
{
	UNUSED_ARG(paramlist);
}

__WEAK void URC_QMTRECV_Proc(char *paramlist)
{
	UNUSED_ARG(paramlist);
}

__WEAK void URC_QMTSTAT_Proc(char *paramlist)
{
	UNUSED_ARG(paramlist);
}

__WEAK void URC_NSONMI_Proc(char *paramlist)
{
	UNUSED_ARG(paramlist);
}

__WEAK void URC_NSOCLI_Proc(char *paramlist)
{
	UNUSED_ARG(paramlist);
}

/*用户可以在此执行无卡的容错操作，例如调用Send_Rai接口触发CP核进入深睡，或者调用Stop_CP强行下电CP*/
__WEAK void URC_SIMST_Proc(char *paramlist)
{
	UNUSED_ARG(paramlist);
}

/*处理CP核上报"POWERDOWN" URC*/
__WEAK void URC_CP_SYSDOWN_Proc(char *paramlist)
{
	UNUSED_ARG(paramlist);
}


/* 用户在此添加需要处理的URC前缀及hook，不需要的主动上报务必不要注册，否则严重影响性能。建议使用at_parse_param接口解析具体的参数值！*/
const urc_regist_t urc_reg_list[] = {
	{"NNMI", URC_NNMI_Proc},
	{"MIPLREAD", URC_MIPLREAD_Proc},
	{"MIPLWRITE", URC_MIPLWRITE_Proc},
	{"MIPLEXECUTE", URC_MIPLEXECUTE_Proc},
	{"MIPLPARAMETER", URC_MIPLPARAMETER_Proc},
	{"NSOCLI", URC_NSOCLI_Proc},
	{"NSONMI", URC_NSONMI_Proc},
	{"QMTRECV", URC_QMTRECV_Proc},
	{"QMTSTAT", URC_QMTSTAT_Proc},
	//{"SIMST", URC_SIMST_Proc},
	{"CTLWRECV", URC_CTLWRECV_Proc},
	//{"POWERDOWN", URC_CP_SYSDOWN_Proc},
	{0,0}
};

/**
  * @brief  定制长度字符串匹配，通常用于前缀的匹配，客户无需关注
  * @param  source   字符串首地址，通常为AT命令的参数首地址
  * @param  substr   待匹配的字符串，通常为前缀，如“CGEV”
  * @param  n        从source头部开始匹配的字节数
  * @return  匹配成功后返回下一个地址，一般为参数头部；否则返回NULL，如AT_Strnstr(“\r\n+CGEV:0,2,5”,“+CGEV:”,8),返回值为"0,2,5"首地址
  * @warning 一般客户无需关注
  */
__OPENCPU_FUNC char * AT_Strnstr(char * source, char * substr, unsigned int n)
{
	char *ret;
	char *end = xy_malloc(n+4); //头部跳过“\r\n+”，尾部补0

	memcpy(end,source,n+3);		//末尾保留\0
	*(end+n+3) = '\0';
	ret = strstr(end,substr);
	if(ret == NULL)
	{
		xy_free(end);
		return NULL;
	}
	xy_free(end);
	return (char *)(source+(ret-end)+strlen(substr));
}

__OPENCPU_FUNC int Match_URC_Cmd(char *at_buf)
{
	urc_regist_t *p_urc_reg_list = (urc_regist_t *)urc_reg_list;

	while(p_urc_reg_list->at_prefix != 0)
	{
		char *param_head;
		uint32_t param_len = 0;

		if((param_head = AT_Strnstr(at_buf, p_urc_reg_list->at_prefix, strlen(p_urc_reg_list->at_prefix))) != NULL)
		{
			UrcList_t *pxlist;

			do{
				param_head++;
			}while((*param_head == ':') || (*param_head == ' '));

			if((*param_head == '\r') && (*(param_head+1) == '\n'))
			{
				param_len = 0;
			}
			else
			{
				param_len = strlen(param_head);
			}	

			pxlist = xy_malloc(sizeof(UrcList_t) + param_len+1);
			pxlist->next = NULL;
			pxlist->proc = p_urc_reg_list->proc;

			if(param_len != 0)
				strcpy(pxlist->param, param_head);

			ListInsert((List_t *)pxlist, &urc_list);
			set_event(EVENT_AT_URC);
			return 1;
		}
		p_urc_reg_list++;
	}
	return 0;
}
void Process_CP_URC(void)
{
	UrcList_t *use_node;
	while((use_node = (UrcList_t *)ListRemove(&urc_list)) != NULL)
	{
		use_node->proc(use_node->param);
		xy_free(use_node);
		use_node = NULL;
	}
	clear_event(EVENT_AT_URC);
}


/*该函数每次唤醒都执行，需要放在RAM上执行，否则运行时间过长影响功耗*/
__RAM_FUNC void CP_URC_Process(void)
{
	if(!is_event_set(EVENT_AT_URC))
		return;

	Process_CP_URC();
}
#else

__RAM_FUNC void CP_URC_Process(void)
{}
#endif
