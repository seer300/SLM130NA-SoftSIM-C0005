/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "ipc_msg.h"
#include "xy_utils.h"
#include "hw_types.h"
#include "at_ctl.h"
#include "xy_system.h"
#include "oss_nv.h"
#include "dump_flash.h"
#include "xy_ps_api.h"
#include "main_proxy.h"
#include "user_ipc_msg.h"
#include "at_worklock.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/


/*******************************************************************************
 *						   Global variable definitions						   *
 ******************************************************************************/
volatile unsigned int time_begin = 0; //debug info
volatile unsigned int time_end = 0;	  //debug info
osTimerId_t g_ipc_timer = NULL;
/*******************************************************************************
 *						Global function implementations 					   *
 ******************************************************************************/

/* CP发送零拷贝消息给AP后，不能立即进入深睡，否则会因为深睡RAM掉电而造成AP核访问RAM异常 */
int g_zero_copy_sum = 0;
osMutexId_t icm_zero_copy_m = NULL;

zero_copy_info_t *zero_copy_info = NULL;  //CP发送至AP的零拷贝消息链表

/* CP发送零拷贝消息给AP后，不能立即进入深睡，否则会因为深睡RAM掉电而造成AP核访问RAM异常 */
void add_zero_copy_sum(uint32_t addr, unsigned int len)
{
	xy_mutex_acquire(icm_zero_copy_m, osWaitForever);
	g_zero_copy_sum++;

//开启此宏定义时，保存CP发送AP的零拷贝消息至链表，供内部调试查看
#if ICM_ZERO_COPY_LIST_ENABLE     
	if(zero_copy_info == NULL)   //初始化
	{
		zero_copy_info = (zero_copy_info_t*)xy_malloc(sizeof(zero_copy_info_t));
		memset(zero_copy_info, 0, sizeof(zero_copy_info_t));
	}

	zero_copy_list_t *icm_msg = NULL;

	icm_msg = (zero_copy_list_t*)xy_malloc(sizeof(zero_copy_list_t));
    if (icm_msg == NULL)
    {
        xy_mutex_release(icm_zero_copy_m);
	    return;
    }
	memset(icm_msg, 0x00, sizeof(zero_copy_list_t));

    icm_msg->msg_addr = addr;
    icm_msg->msg_len = len;
	icm_msg->tskName = osThreadGetName(osThreadGetId());
	if (zero_copy_info->tail == NULL)
    {
        zero_copy_info->head = icm_msg;
        zero_copy_info->tail = icm_msg;
    }
    else
    {
        zero_copy_info->tail->next = icm_msg;
        zero_copy_info->tail = icm_msg;
    }

    zero_copy_info->pending_num++;
	xy_printf(0,PLATFORM,WARN_LOG,"ICM_ZERO_COPY_MSG, addr:%x, len:%d, current_thread:%s", icm_msg->msg_addr, icm_msg->msg_len, icm_msg->tskName);
#endif

	xy_mutex_release(icm_zero_copy_m);
	return;
}


/* CP发送零拷贝消息给AP后，不能立即进入深睡，否则会因为深睡RAM掉电而造成AP核访问RAM异常 */
void sub_zero_copy_sum()
{
	xy_mutex_acquire(icm_zero_copy_m, osWaitForever);
	g_zero_copy_sum--;
	xy_assert(g_zero_copy_sum >= 0);

#if ICM_ZERO_COPY_LIST_ENABLE 
	zero_copy_list_t *icm_msg;

	if (zero_copy_info->pending_num > 0 && zero_copy_info->head != NULL)
    {
        icm_msg = zero_copy_info->head;
        zero_copy_info->head = zero_copy_info->head->next;

        //xy_printf(0,PLATFORM,WARN_LOG,"clear ICM_ZERO_COPY_MSG, addr:%x, len:%d, current_thread:%s", icm_msg->msg_addr, icm_msg->msg_len, icm_msg->tskName);
        
		zero_copy_info->pending_num--;
        if (zero_copy_info->pending_num == 0)
        {
            zero_copy_info->tail = NULL;
        }

        xy_free(icm_msg);
    }
#endif

	xy_mutex_release(icm_zero_copy_m);
}

bool shm_msg_write(void *buf, int len, unsigned int msg_type)
{
	T_IpcMsg_Msg pMsg = {0};
	unsigned int send_num = 0;
	volatile unsigned int cycle = 0;

	pMsg.buf = buf;
	pMsg.id = msg_type;
	pMsg.len = len;

	while (IpcMsg_Write(&pMsg) < 0)
	{
		if (send_num == 0) //debug info
			time_begin = osKernelGetTickCount();
		send_num++;
		if (send_num > 200) //debug info
		{
			time_end = osKernelGetTickCount();
			xy_assert(0); //10s assert
		}
		//delay 50ms,have tested
		for (cycle = 0; cycle < 200000; cycle++);
	}

	return 1;
}

void icm_ap_log(void *ap_log_buf)
{
	char *log_buf = NULL;

	log_buf = (char *)Address_Translation_AP_To_CP((unsigned int)ap_log_buf);

	/*通过logview输出AP的log*/
	xy_printf(0,AP_CORE_LOG, WARN_LOG,"%s",log_buf);

	shm_msg_write(&ap_log_buf, 4, ICM_MEM_FREE);
}

void icm_ap_gnss_log(void *ap_log_buf)
{
	char *log_buf = NULL;

	log_buf = (char *)Address_Translation_AP_To_CP((unsigned int)ap_log_buf);

	/*通过logview输出AP的GNSS位置信息码流，最终由logview进行提取*/
	PrintGnssLog(WARN_LOG,"%s",log_buf);

	shm_msg_write(&ap_log_buf, 4, ICM_MEM_FREE);
}


void send_Fota_stat_msg(uint32_t fota_state)
{
	shm_msg_write(&fota_state,4,ICM_CHANGE_CP_STATE);
}

void inter_core_msg_entry()
{
	int read_len = -1;
	T_IpcMsg_Msg Msg;
	char *rcv_buf = xy_malloc(IPCMSG_SINGLE_BUFF_MAX_SIZE-sizeof(T_IpcMsg_Head));

	while (1)
	{
		/*IPC通道自身没有对每条消息的长度做出限制，此处设置该宏值仅仅是为了方便接收时固定缓存上限。有效数据长度不得超过123*/
		Msg.len = IPCMSG_SINGLE_BUFF_MAX_SIZE-sizeof(T_IpcMsg_Head);
		Msg.buf = rcv_buf;
		read_len = IpcMsg_Read(&Msg, IPC_WAITFROEVER);

		/*对于空数据，为了防止脏数据，强制清零*/
		if(read_len == 0)
		{
			memset(rcv_buf, 0, 8);
		}	


		if(process_rcved_usr_msg(Msg.id, (void *)*((uint32_t *)(rcv_buf))) == 1)
		{
			continue;
		}


		if (read_len > 0)
		{
			switch (Msg.id)
			{
				case ICM_FLASHWRITE:
				{
					ipc_flash_process(rcv_buf);
					break;
				}

				case ICM_CHANGE_CP_STATE:
				{
					switch(*(uint8_t *)rcv_buf)
					{
						case CP_STOP:
						{
							HWREGB(BAK_MEM_AP_STOP_CP_REQ) = 1;
							//停CP核必须先软件停3GPP工作，因为rf等寄存器可能处于不稳定态，造成软重启后会有异常
							xy_cfun_excute(NET_CFUN5);

						    extern osThreadId_t g_flash_TskHandle;
						    volatile eTaskState stat = eBlocked;
						again:
							// 等待DMA Channel 0-3 空闲
						    if(g_flash_TskHandle)
								stat = eTaskGetState((TaskHandle_t)g_flash_TskHandle);
							while ((DMAChannelGetStartStatus(FLASH_DMA_CHANNEL) && DMAChannelTransferRemainCNT(FLASH_DMA_CHANNEL)) \
									|| stat == eRunning || stat == eReady)
							{
								osDelay(50);
								if(g_flash_TskHandle)
									stat = eTaskGetState((TaskHandle_t)g_flash_TskHandle);
							}

							// 锁中断防止高优先级线程抢占
							taskENTER_CRITICAL();

							// 再次判断DMA是否空闲，防止锁中断瞬间发生线程切换
							if(g_flash_TskHandle)
								stat = eTaskGetState((TaskHandle_t)g_flash_TskHandle);
							if ( !(DMAChannelGetStartStatus(FLASH_DMA_CHANNEL) && DMAChannelTransferRemainCNT(FLASH_DMA_CHANNEL)) \
									 && stat == eBlocked)
							{
								
								// 死循环等待AP关闭CP
								while (1)
								{
									//clear pending
									NVIC->ICPR[0] = 0xFFFFFFFF;
									NVIC->ICPR[1] = 0xFFFFFFFF;
									SCB->SCR |= 0x04;
									// 双核握手
									HWREGB(BAK_MEM_AP_STOP_CP_REQ) = 2;
									__asm__ ("WFI");
								}
									
							}
							else
							{
								taskEXIT_CRITICAL();
								goto again;
							}

							break;
						}

						case CP_RAI:
						{
							xy_send_rai();
							break;
						}
						case CP_CFUN:
						{
							xy_cfun_excute((int)*((uint8_t *)rcv_buf+1));
							break;
						}
						
						default:
						{
							xy_assert(0);
							break;
						}
					}
					break;
				}
				case ICM_AP_LOG:
				{
					icm_ap_log((void *)*((uint32_t *)(rcv_buf)));
					break;
				}	
				case ICM_AT_ASYNC:
				case ICM_AT_SYNC:
				case ICM_AT_EXT:
				{
					icm_at_msg_recv(Msg.id, (zero_msg_t *)rcv_buf);
					break;
				}

				case ICM_MEM_FREE:
				{
					xy_free((void *)Address_Translation_AP_To_CP(*(unsigned int *)rcv_buf));
					sub_zero_copy_sum();
					break;
				}

				case ICM_NV_WRITE:
				{
					send_msg_2_proxy(PROXY_WRITE_AP_NV_PARAM,(void *)rcv_buf,read_len);
					break;
				}

				case ICM_APAT_DELAY:
				{
					at_delaylock_act();
					break;
				}

				default:
				{
					if (HWREGB(BAK_MEM_XY_DUMP) == 1)
						xy_assert(0);
					else
						xy_printf(0,PLATFORM, WARN_LOG, "recv unknown msg :%d",Msg.id);
					break;
				}
			}
		}
	}
}

void icm_task_init()
{
	HWREGB(COREPRCM_BASE + 0x20) |= 0x01;

	icm_zero_copy_m = osMutexNew(NULL);	
	osThreadAttr_t thread_attr = {0};
	
	thread_attr.name = "inter_core_msg";
	thread_attr.priority = osPriorityAboveNormal1;
	thread_attr.stack_size = osStackShared; 	//0x800->osStackShared
	osThreadNew(inter_core_msg_entry, NULL, &thread_attr);
}
