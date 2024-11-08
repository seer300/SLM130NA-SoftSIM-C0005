/**
 * @file   
 * @brief  此文件是为了方便模组客户通过基础平台的IPC核间通信机制，搭建两个核业务层面的消息交互机制
 * @warning 不建议客户进行核间消息的扩展开发，优先推荐使用AT命令方式进行核间交互。
 * @warning 若使用核间扩展消息机制，必须在IPC_ProcessEvent里打开USER_IPC_MSG宏。
 *
  ******************************************************************************
 */

#pragma once

#include <stdint.h>
#include "sys_ipc.h"


/**
 * @brief	用户自定义的核间消息类型枚举
 * @warning 用户可以改名，但不得调整取值范围
 */
typedef enum
{
	ICM_USER_BASE = 20,	 
	ICM_USER_MSG1 = ICM_USER_BASE,
	ICM_USER_MSG2,
	ICM_USER_MSG3,
	ICM_USER_MSG4,
	ICM_USER_MAX,			
} User_Msg_E;


/*用户无需关心param内存释放，平台后台会进行释放*/
typedef void (*RcvedMsgFunc)(void *param);


typedef struct
{
	int           mid;   //see  User_Msg_E
	RcvedMsgFunc  proc;
}usr_msg_t;

/**
 * @brief 供用户进行自定义消息的发送，核间零拷贝思想，返回0表示发送失败。
 */
int send_usr_msg(int mid, void *data, uint32_t datalen);


/**
 * @brief 用户无需关心！在IPC_ProcessEvent里调用，返回1表示为用户扩展消息，函数内部会执行相应的注册函数。
 * @warning 若使用核间扩展消息机制，必须在IPC_ProcessEvent里打开USER_IPC_MSG宏
 */
int process_rcved_usr_msg(int mid, void *param);


