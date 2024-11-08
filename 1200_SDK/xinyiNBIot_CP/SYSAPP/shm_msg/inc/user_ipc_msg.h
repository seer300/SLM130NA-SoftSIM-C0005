#pragma once

#include <stdint.h>


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

/*用户无需关系param内存释放，会进行释放*/
typedef void (*RcvedMsgFunc)(void *param);


typedef struct
{
	int           mid;   //see  User_Msg_E
	RcvedMsgFunc  proc;
}usr_msg_t;

/**
 * @brief 用户进行自定义消息的发送，核间零拷贝思想，返回0表示发送失败
 */
int send_usr_msg(int mid,void *data,uint32_t datalen);

/**
 * @brief 用户无需关心！在inter_core_msg_entry里调用，返回1表示为用户扩展消息，函数内部会执行相应的注册函数。
 * @warning 若使用核间扩展消息机制，必须在inter_core_msg_entry里打开USER_IPC_MSG宏
 */
int process_rcved_usr_msg(int mid, void *param);


