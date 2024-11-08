/*
 * at_ipc_process.c
 *
 *  Created on: 
 *      Author: Administrator
 */
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>
#include "xy_cp.h"
#include "at_utils.h"
#include "at_process.h"
#include "at_ipc_process.h"
#include "xy_list.h"
#include "sys_ipc.h"
#include "xy_system.h"
#include "xy_event.h"
#include "zero_copy.h"
#if XY_AT_CTL
#include "at_CP_api.h"
#include "urc_process.h"
#endif


/*AP核本地的异常错误，最终体现到AT命令中，供用户容错*/
int g_errno = 0;

/* 接收到的异步AT命令或非芯翼AT框架的AT接收链表,包括URC */
ListHeader_t asyn_ipc_at_list = {0};



/*清空所有AT命令缓存，通常用于CP异常容错*/
__OPENCPU_FUNC void clear_at_fifo()
{
	ListFreeAll(&asyn_ipc_at_list);
#if XY_AT_CTL
	ListFreeAll(&urc_list);
	ListFreeAll(&sync_ipc_at_list);
#endif
}

/* 检测是否包含“OK”“ERROR”结果码 */
__OPENCPU_FUNC bool Is_Result_AT_str(char *str)
{
	int ret = 0;
	int n = strlen(str);
	char *temp = str;

	//ERROR:XXXX\r\n;减少无效匹配
	if(get_cmee_mode() != 2 && n > 15)
	{
		temp = str+n-15;
	}

	if(strstr(temp,"ERROR") || strstr(temp,"OK\r\n"))
	{
		ret = 1;
	}

	return ret;
}


__OPENCPU_FUNC void Insert_AT_Node(IPC_At_Data *at_str,ListHeader_t *list_head)
{
	AtCmdList_t *pxlist;

	pxlist = xy_malloc(sizeof(AtCmdList_t) + at_str->at_size + 1);

	pxlist->next = NULL;
	pxlist->len = at_str->at_size;
	memcpy(pxlist->data, at_str->at_buf, at_str->at_size);
	*(pxlist->data + at_str->at_size) = '\0';

	ListInsert((List_t *)pxlist,list_head);
}

int g_local_fota = 0;

/******************************************************************************************************************************
  * @brief   通过核间消息发送数据到CP,内部进行零拷贝
  * @param   data    待发送的AT命令
  * @param   source  AT命令的源头
 ******************************************************************************************************************************/
 /******************************************************************************************************************************
  * @brief   Send data to CP through inter core message, and make zero copy internally
  * @param   data:   AT command to be sent
  * @param   source: Source of AT command
 ******************************************************************************************************************************/
At_status_type AT_Send_To_CP(void *data_addr, uint32_t datalen, CP_AT_SOURCE_E source)
{
	xy_assert(data_addr != NULL && datalen != 0);
	IPC_At_Data user_at = {0};

#if MODULE_VER==0
	/*本地FOTA升级，即将复位之前，清除FOTA指示，以防止Fota_Cloud_Process中异常阻塞*/
	if(strncmp(data_addr,"AT+NFWUPD=5",strlen("AT+NFWUPD=5"))==0)
    {
		g_local_fota = 1;
    }
#endif
	if (g_errno != 0)
		return g_errno;

	if (CP_Is_Alive() == false)
	{
		return XY_ERR_CP_NOT_RUN;
	}

	user_at.at_buf = xy_malloc(datalen + 1);
	if (user_at.at_buf == NULL)
	{
		return XY_ERR;
	}
	else
	{
		memcpy(user_at.at_buf, data_addr, datalen);
		*((char *)(user_at.at_buf) + datalen) = '\0';
		user_at.at_size = datalen;
		IPC_Message pMsg = {0};
		pMsg.len = sizeof(IPC_At_Data);
		pMsg.buf = &user_at;
		
		if (source == AT_FROM_SYNC_API)
			pMsg.id = ICM_AT_SYNC;
		else if(source == AT_FROM_ASYN_API)
			pMsg.id = ICM_AT_ASYNC;
		else if(source == AT_FROM_EXT_MCU)
        {
			pMsg.id = ICM_AT_EXT;
            /* 标志外部AT命令已接收准备投递到CP核,用于AT唤醒阶段URC缓存处理，必须在投递到CP前置位 */
            HWREGB(BAK_MEM_ATUART_STATE) = (HWREGB(BAK_MEM_ATUART_STATE) & (~(1 << 4))) | (1 << 4);
        }

		else
		{
			xy_assert(0);
		}

		Insert_ZeroCopy_Buf(user_at.at_buf);

		if (IPC_WriteMessage(&pMsg) < 0)
		{
            if (source == AT_FROM_EXT_MCU)
                HWREGB(BAK_MEM_ATUART_STATE) = (HWREGB(BAK_MEM_ATUART_STATE) & (~(1 << 4))) | (0 << 4);
           
			Delet_ZeroCopy_Buf(user_at.at_buf);
			return XY_ERR_IPC_FAIL;
		}
	}

	return XY_OK;
}

/*供AP核本地用户自定义AT框架进行AT命令发送CP核*/
int at_uart_write(int cid,uint8_t *data_addr,int datalen)
{
	UNUSED_ARG(cid);
	At_status_type ret = XY_OK;

	/*仅用于OPENCPU场景。涉及CP核总体硬件的操作，只能放在AP核此处来执行。且当前不支持回复"\r\nOK\r\n",若需要回复，需要定制下*/
	if(CP_Special_AT_Proc((void *)data_addr) == 1)
	{
		return XY_OK;
	}

	ret = AT_Send_To_CP((void *)data_addr, datalen, AT_FROM_ASYN_API);

	if(ret == XY_OK)
	{
#if (XY_DEBUG == 1)
		char *print_str = xy_malloc(datalen + 1);
		memcpy(print_str,data_addr,datalen);		
		*((char *)(print_str) + datalen) = '\0';
		xy_printf("\r\nat_uart_write CP:%s\r\n", print_str);
		xy_free(print_str);
#endif	
	}
	else
	{
		xy_printf("\r\n at_uart_write Error:%d\r\n", ret);
	}
	return ret;
}

 
/*AT_Send_And_Get_Rsp应答结果AT字符串，不包含URC，要求中间结果必须与OK作为一条命令发送过来*/
__OPENCPU_FUNC void ipc_proc_Sync_AT(void *data)
{
	IPC_At_Data *at_str = (IPC_At_Data *)data;
	IPC_Message pMsg = {ICM_MEM_FREE, &(at_str->at_buf), 4};

	xy_assert(at_str->at_buf != NULL && at_str->at_size != 0);

	//来自CP的所有AT命令皆拷贝一份挂链表，以供用户面Get_AT_Rsp_Until接口去匹配解析
	Insert_AT_Node(at_str,&sync_ipc_at_list);

	IPC_WriteMessage(&pMsg);
}

#if XY_AT_CTL

//CP异常时，AP强行给CP下电，需释放所有的零拷贝地址
__OPENCPU_FUNC void clear_at_info()
{
	g_errno = 0;
	g_cgatt_start_tick = 0;
	g_tcpip_ok = 0;
	g_async_at_doing = 0;

	clear_at_fifo();
	Free_ZeroCopy_Buf();

	/*强行清空CP相关的事件*/
	clear_event(EVENT_AT_URC);
}


/*Send_AT_Req与Get_AT_Rsp异步模式AT交互的处理，包含URC*/
__OPENCPU_FUNC void ipc_proc_Async_AT(void *data)
{
	IPC_At_Data *at_str = (IPC_At_Data *)data;
	xy_assert(at_str->at_buf != NULL && at_str->at_size != 0);

	IPC_Message pMsg = {ICM_MEM_FREE, &(at_str->at_buf), 4};
	//匹配是否是用户关心的URC，若是，则拷贝一份挂链表
	if (!Is_Result_AT_str(at_str->at_buf))
		Match_URC_Cmd(at_str->at_buf);

	/*仅当有异步AT请求正在处理时，才会接收处理CP来的AT命令*/
	if (g_async_at_doing == 1)
	{
		Insert_AT_Node(at_str, &asyn_ipc_at_list);
	}

	IPC_WriteMessage(&pMsg);	
}

//非零拷贝方式，CP向AP上报"POWERDOWN" URC
__OPENCPU_FUNC void ipc_proc_copy_URC(void *data)
{
	xy_assert(data != NULL && strlen(data) != 0);
	Match_URC_Cmd(data);
}

__OPENCPU_FUNC void IPC_Set_User_Handle(void *pFun)
{
	UNUSED_ARG(pFun);
    xy_assert(0);
}


#else

uint32_t g_sync_ipc_atbuf_len = 0;
//CP异常时，AP强行给CP下电，需释放所有的零拷贝地址
__OPENCPU_FUNC void clear_at_info()
{
	g_errno = 0;

	g_sync_ipc_atbuf_len = 0;
	ListFreeAll(&asyn_ipc_at_list);
	Free_ZeroCopy_Buf();
}

typedef void (*pUserAtHandle)(uint8_t data);

pUserAtHandle g_user_at_handle = NULL;

__OPENCPU_FUNC void IPC_Set_User_Handle(void *pFun)
{
    g_user_at_handle = pFun;
	mark_dyn_addr(&g_user_at_handle);
}

extern void ListInsertHead(List_t *pxList,ListHeader_t *p_List_header);
/*供AP核用户自定义AT框架，进行AT虚拟通道的接收读取。若入参空间不够存储完全链表缓存，则按完整的AT命令拷贝，余下的待用户再次调用读接口*/
int at_uart_read(int cid,uint8_t *atstr,int len)
{	
	UNUSED_ARG(cid);

	if(g_sync_ipc_atbuf_len == 0)
		return 0;

	AtCmdList_t *use_node = NULL;
	uint32_t copied_len = 0;
	uint32_t data_len = 0;

	DisablePrimask();
	do{
		use_node = (AtCmdList_t *)ListRemove(&asyn_ipc_at_list);
		if(use_node != NULL)
		{
			/*该长度不包括最后的'\0'*/
			data_len = use_node->len;

			/*空间不够完整的一条AT命令，放弃读取*/
			if(len-copied_len < data_len)
			{
				ListInsertHead((List_t *)use_node,&asyn_ipc_at_list);
				EnablePrimask();
				return copied_len;
			}
			g_sync_ipc_atbuf_len -= data_len;
			memcpy(atstr + copied_len, use_node->data, data_len);
			copied_len += data_len;

			xy_printf("at_uart_read:%s",use_node->data);
			
			xy_free(use_node);
			use_node = NULL;
		}
		else
			break;
	}while(1);

	EnablePrimask();
	return copied_len;
}

/*AP核用户自定义AT框架，通过at_uart_read或用户回调，串行读取CP核的AT上报*/
__OPENCPU_FUNC void ipc_proc_Async_AT(void *data)
{
	IPC_At_Data *at_str = (IPC_At_Data *)data;
	xy_assert(at_str->at_buf != NULL && at_str->at_size != 0);

	IPC_Message pMsg = {ICM_MEM_FREE, &(at_str->at_buf), 4};

	/*供部分用户模拟串口单字节接收场景使用，不会插入内部链表*/
	if(g_user_at_handle != NULL)
	{
		char *p_at_rsp = at_str->at_buf;
		for(uint32_t i = 0; i < at_str->at_size; i++)
		{
			g_user_at_handle(p_at_rsp[i]);
		}
	}
	/*插入链表，供用户调用at_uart_read接口读取*/
	else
	{
		//用户必须在main主函数中调用at_uart_read接口，否则会造成链表满内存泄漏
		Insert_AT_Node(at_str,&asyn_ipc_at_list);

		g_sync_ipc_atbuf_len += at_str->at_size + 1;
	}

	IPC_WriteMessage(&pMsg);

}

//非零拷贝方式，CP向AP上报"POWERDOWN" URC
__OPENCPU_FUNC void ipc_proc_copy_URC(void *data)
{
	xy_assert(data != NULL && strlen(data) != 0);

	if(HWREGB(BAK_MEM_DROP_URC) == 1)
		return;

	/*供部分用户模拟串口单字节接收场景使用，不会插入内部链表*/
	if(g_user_at_handle != NULL)
	{
		char *p_at_rsp = data;

		for(uint32_t i = 0; i < strlen(data); i++)
		{
			g_user_at_handle(p_at_rsp[i]);
		}
	}
	/*插入链表，供用户调用at_uart_read接口读取*/
	else
	{
		//用户必须在main主函数中调用at_uart_read接口，否则会造成链表满内存泄漏
		AtCmdList_t *pxlist = xy_malloc(sizeof(AtCmdList_t) + strlen(data) + 1);
		pxlist->next = NULL;
		pxlist->len = strlen(data);
		memcpy(pxlist->data, data, pxlist->len);
		*(pxlist->data+pxlist->len) = '\0';
		
		ListInsert((List_t *)pxlist,&asyn_ipc_at_list);

		g_sync_ipc_atbuf_len += (pxlist->len)+1;
	}	
}

#endif
