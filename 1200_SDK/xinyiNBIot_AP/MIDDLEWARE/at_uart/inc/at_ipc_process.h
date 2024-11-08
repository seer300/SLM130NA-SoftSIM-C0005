/*
 * at_ipc_process.h
 */


#pragma once

#include <stdint.h>
#include "at_utils.h"
#include "xy_at_api.h"
#include "at_uart.h"

typedef struct
{
	struct List_t	*next;
	int             len;
	char			data[0];	//申请内存时需要多申请一个字节来保存'\0'
}AtCmdList_t;


/*用户无需关注！*/
typedef struct
{
	void *at_buf;
	uint32_t at_size;
}IPC_At_Data;

/*用户无需关注！指示AT请求的源头，仅对异步AT机制有意义*/
typedef enum
{
    AT_FROM_SYNC_API = 0, //收到同步AP应用所发AT，AT_Send_And_Get_Rsp
    AT_FROM_EXT_MCU,      //收到外部MCU所发AT
    AT_FROM_ASYN_API,     //收到异步AP应用所发AT，Send_AT_Req或at_uart_write
	AT_FROM_BLE,          //收到对端蓝牙发的AT
} CP_AT_SOURCE_E;



/* 标志当前是否有异步AT命令正在执行 */
extern int g_async_at_doing;

extern uint8_t get_cmee_mode();

At_status_type AT_Send_To_CP(void *data_addr, uint32_t datalen,CP_AT_SOURCE_E source);

void clear_at_info();


void IPC_Set_User_Handle(void *pFun);

int CP_Special_AT_Proc(void *at_str);

