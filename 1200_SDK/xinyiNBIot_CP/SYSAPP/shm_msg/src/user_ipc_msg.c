#include "user_ipc_msg.h"
#include "mem_adapt.h"
#include "ipc_msg.h"
#include "xy_log.h"
#include "xy_system.h"

int send_usr_msg(int mid,void *data,uint32_t datalen)
{
	void *send_ptr = NULL;
	uint32_t send_len = 0;

	if(data != NULL)
	{
		uint8_t *send_buff = xy_malloc(datalen + 1);

		memcpy(send_buff, data, (datalen + 1));
		*(send_buff + datalen) = '\0';
		xy_cache_writeback(send_buff,datalen);

		send_len = 4;
		send_ptr = (char *)Address_Translation_CP_To_AP((unsigned int)send_buff);
		
		add_zero_copy_sum((uint32_t)send_ptr,(unsigned int)datalen);
	}

	return shm_msg_write(&send_ptr, send_len, mid);
}

/*当前处理示例为回显*/
void usr_msg_test1(void *param)
{
	if(param == NULL)
	{
		PrintLog(0, PLATFORM, WARN_LOG, "RECV is NULL");
		send_usr_msg(ICM_USER_MSG1,NULL,0);
		return;
	}
	char *out_ptr = (char *)Address_Translation_AP_To_CP((unsigned int)(param));
	PrintLog(0, PLATFORM, WARN_LOG, "RECV:%s", out_ptr);
	send_usr_msg(ICM_USER_MSG1,out_ptr,strlen(out_ptr));
}

const usr_msg_t g_usr_msg_list[] = {

	{ICM_USER_MSG1, usr_msg_test1},
	{0, 0}
};


/*在inter_core_msg_entry里调用，返回1表示用户扩展消息，在此函数中处理完毕即可*/
int process_rcved_usr_msg(int mid, void *param)
{
	usr_msg_t *node = (usr_msg_t *)g_usr_msg_list;
	
	if(mid < ICM_USER_BASE || mid >= ICM_USER_MAX)
		return 0;
	
	while(node->mid != 0)
	{
		if(node->mid == mid)
		{
			/*该内存为对方核申请的内存空间，需要跨核释放*/
			node->proc(param);
			if(param != NULL)
			{
				shm_msg_write(&param, 4, ICM_MEM_FREE);
			}
			return 1;
		}
		else
			node++;
	}
	return 0;
}