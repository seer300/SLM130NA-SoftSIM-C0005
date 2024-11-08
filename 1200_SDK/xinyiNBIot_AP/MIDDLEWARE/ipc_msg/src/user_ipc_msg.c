/**
 * @file
 * @brief  此文件是为了方便模组客户通过基础平台的IPC核间通信机制，搭建两个核业务层面的消息交互机制
 * @warning 不建议客户进行核间消息的扩展开发，优先推荐使用AT命令方式进行核间交互。
 * @warning 若使用核间扩展消息机制，必须在IPC_ProcessEvent里打开USER_IPC_MSG宏。
 *
 ******************************************************************************
 */
#include "user_ipc_msg.h"
#include "sys_ipc.h"
#include "hw_types.h"
#include "system.h"
#include "xy_system.h"
#include "zero_copy.h"

/*用户无需关心param内存释放，平台后台会进行释放*/
__RAM_FUNC void usr_msg_test1(void *param)
{
	if(param != NULL)
	{
		xy_printf("usr_msg_test1:%s\r\n", (char *)param);
	}
	else
	{
		xy_printf("usr_msg_test1:NULL\r\n");
	}
}

const usr_msg_t g_usr_msg_list[] = {

	{ICM_USER_MSG1, usr_msg_test1},
	{0, 0}
};

/*在IPC_ProcessEvent里调用，返回1表示用户扩展消息，在此函数中处理完毕即可*/
int process_rcved_usr_msg(int mid, void *param)
{
	usr_msg_t *node = (usr_msg_t *)g_usr_msg_list;
	IPC_Message pMsg  = {ICM_MEM_FREE, &param, 4};
	
	if (mid < ICM_USER_BASE || mid >= ICM_USER_MAX)
		return 0;

	while (node->mid != 0)
	{
		if (node->mid == mid)
		{
			node->proc(param);
			if(param != NULL)
			{
				/*该内存为对方核申请的内存空间，需要跨核释放*/
				IPC_WriteMessage(&pMsg);
			}
			return 1;
		}
		else
			node++;
	}
	return 0;
}

extern bool CP_Is_Alive(void);
int send_usr_msg(int mid, void *data, uint32_t datalen)
{
	IPC_Message pMsg = {0};
	void *msg_data = NULL;

	if (CP_Is_Alive() == false)
	{
		return 0;
	}

	if (datalen != 0)
	{
		
		msg_data = xy_malloc(datalen + 1);
		memcpy(msg_data, data, datalen);
		*((char *)msg_data + datalen) = '\0';
		Insert_ZeroCopy_Buf(msg_data);
		pMsg.len = 4;
		pMsg.buf = &msg_data;
	}
	else
	{
		pMsg.len = 0;
		pMsg.buf = NULL;
	}
	
	pMsg.id = mid;

	if (IPC_WriteMessage(&pMsg) < 0)
	{
		if(datalen != 0)
		{
			Delet_ZeroCopy_Buf(msg_data);
		}
		return 0;
	}

	return 1;
}
