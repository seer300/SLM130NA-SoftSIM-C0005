/**
 * @file xy_passthrough.c
 * @brief 芯翼透传模块，用于通过at uart口进行透传数据传输，目前支持ppp,固定长度，socket tcp及短信模式透传
 * @version 1.0
 * @date 2021-04-29
 * @copyright Copyright (c) 2021  芯翼信息科技有限公司
 * 
 */

#include "xy_passthrough.h"
#include "at_passthrough.h"
#include "xy_at_api.h"
#include "xy_system.h"
#include "xy_memmap.h"

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
typedef struct passthr_msg
{
    uint32_t len;
    char data[0];
} passthr_msg_t;

/*******************************************************************************
 *						   Global variable definitions				           *
 ******************************************************************************/
at_passthrough_hook g_at_passthr_hook = NULL;
app_passthrough_proc g_app_passthr_proc = NULL;
app_passthrough_exit g_app_passthr_exit = NULL;

/*******************************************************************************
 *						   Local variable definitions				           *
 ******************************************************************************/
char *passthr_rcv_buff = NULL;
uint32_t passthr_rcvd_len = 0;
uint32_t passthr_fixed_buff_len = 0;
uint32_t passthr_dynmic_buff_len = 0;
osMutexId_t passthr_msg_m;
osThreadId_t passthr_thread_id = NULL;
osMessageQueueId_t passthr_msg_q = NULL;

/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
void passthr_send_msg(uint32_t len, void* data)
{
    xy_assert(passthr_msg_q != NULL);
    osMutexAcquire(passthr_msg_m, osWaitForever);
    passthr_msg_t *msg = xy_malloc(sizeof(passthr_msg_t) + len);
    msg->len = len;
    if (data != NULL)
        memcpy(msg->data, data, len);
    osMessageQueuePut(passthr_msg_q, &msg, 0, osWaitForever);
    osMutexRelease(passthr_msg_m);
}

void passthrough_process()
{
    passthr_msg_t *msg = NULL;
    while (1)
    {
		osMessageQueueGet(passthr_msg_q, (void *)(&msg), NULL, osWaitForever);

        if (g_app_passthr_proc != NULL)
           g_app_passthr_proc(msg->data, msg->len);
        xy_free(msg);
    }
}

/**
 * @brief  处理从串口接收到的透传码流
 * @note   由于uart接收线程线程栈较小并且不能被长时间阻塞，收到透传码流数据后需要投递到特定的透传线程进行处理
 * @warning 该接口暂不对客户开放，由芯翼内部实现
 */
bool xy_passthr_data_proc_hook(char *buf, uint32_t data_len)
{
	xy_printf(0,PLATFORM, WARN_LOG, "passthr recv from uart len:%d", data_len);
    if (data_len == 0)
        return 0;
    passthr_send_msg(data_len, buf);
    return 1;
}

void set_passthr_shm_flag(uint8_t mode)
{
    HWREGB(BAK_MEM_ATUART_STATE) = (HWREGB(BAK_MEM_ATUART_STATE) & (~(1 << 0))) | (mode << 0);
}

void xy_enterPassthroughMode(app_passthrough_proc proc, app_passthrough_exit exit)
{
    if (proc == NULL)
        xy_assert(0);
    g_app_passthr_exit = exit;
    g_app_passthr_proc = proc;

    //初始化透传消息发送线程
    if (passthr_msg_q == NULL)
        passthr_msg_q = osMessageQueueNew(10, sizeof(void *), NULL);
    if (passthr_msg_m == NULL)
        passthr_msg_m = osMutexNew(NULL);
    if (passthr_thread_id == NULL)
    {
        osThreadAttr_t thread_attr = {0};
        thread_attr.name = "xy_passthr";
        thread_attr.priority = PASSTHR_THD_PROI;
        thread_attr.stack_size = PASSTHR_THD_STACKSIZE;
        passthr_thread_id = osThreadNew((osThreadId_t)passthrough_process, NULL, &thread_attr);
    }

	g_at_passthr_hook = xy_passthr_data_proc_hook;
    set_passthr_shm_flag(1);
}

void xy_exitPassthroughMode()
{
    //执行用户客制化退出处理函数
    if (g_app_passthr_exit != NULL)
        g_app_passthr_exit();

    //重置透彻全局变量
    g_app_passthr_exit = NULL;
    g_app_passthr_proc = NULL;
    g_at_passthr_hook = NULL;
    set_passthr_shm_flag(0);
}

void passthr_get_unfixedlen_data(char *data_buf,uint32_t rev_len)
{
	if (passthr_rcv_buff == NULL)
	{
		passthr_rcv_buff = xy_malloc(PASSTHR_BLOCK_LEN);
		passthr_dynmic_buff_len = PASSTHR_BLOCK_LEN;
	}

	if (passthr_dynmic_buff_len < passthr_rcvd_len + rev_len)
	{
		char block_num = 0; 
		char *new_mem = NULL;
		// 数据长度大于当前申请的内存大小, 重新申请合适大小的内存
		block_num = (passthr_rcvd_len + rev_len)/PASSTHR_BLOCK_LEN + 1;
		new_mem = xy_malloc(PASSTHR_BLOCK_LEN * block_num);
		
		if(passthr_rcvd_len != 0)
			memcpy(new_mem, passthr_rcv_buff, passthr_rcvd_len);
		
		xy_free(passthr_rcv_buff);
		
		passthr_rcv_buff = new_mem;
		passthr_dynmic_buff_len = PASSTHR_BLOCK_LEN * block_num;
	}
	
	memcpy(passthr_rcv_buff + passthr_rcvd_len, data_buf, rev_len);
	passthr_rcvd_len += rev_len;
}
