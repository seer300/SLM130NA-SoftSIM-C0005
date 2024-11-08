/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#include "ipc_msg.h"
#include "hw_memmap.h"
#include "xy_memmap.h"
#include "xy_utils.h"
#include "dump_flash.h"
#include "dump.h"
#include "prcm.h"

#define IpcMsg_FAILURE -1
#define IpcMsg_SUCCESS 0
#define IPCMSG_ALIGN 0x04
#define SRAM_AP_CP_OFFSET  0x00000000
#define IPCMSG_BUFF_SIZE ALIGN_IPCMSG(((IPCMSG_BUF_LEN - (sizeof(T_RingBuf) << 1)) >> 1),IPCMSG_ALIGN)
/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
typedef struct _T_IpcMsg_Resource
{
	IpcMsg_ChID Ch_ID;
	unsigned int block_size;
	unsigned int block_cnt;
	unsigned int direction : 2; //lowest order bit indicate whether exist downlink direction(CP->AP) or not,secondary low order bit indicate whether exist uplink direction(AP->CP) or not;1:open,0:close
	unsigned int reserved : 30;
} T_IpcMsg_Resource;

/*******************************************************************************
 *						   Global variable definitions						   *
 ******************************************************************************/
T_IpcMsg_Resource IpcMsg_Resource_Tbl[IpcMsg_Channel_MAX] =
{
	{IpcMsg_Normal, IPCMSG_BUFF_SIZE, 1, 3, 0},
};

volatile int g_all_ipc_channel_init = 0;
T_IpcMsg_ChInfo g_IpcMsg_ChInfo[IpcMsg_Channel_MAX] = {0};

/*******************************************************************************
 *						Global function implementations 					   *
 ******************************************************************************/
__RAM_FUNC int Is_Ringbuf_Empty(T_RingBuf *ringbuf)
{
	if (ringbuf == NULL)
		return true;

	unsigned int Write_Pos = ringbuf->Write_Pos;
	unsigned int Read_Pos = ringbuf->Read_Pos;

	if (Write_Pos == Read_Pos)
		return true;
	else
		return false;
}

int Is_Ringbuf_ChFreeSpace(T_RingBuf *ringbuf_send, unsigned int size)
{
	if (ringbuf_send == NULL)
		return false;

	unsigned int Write_Pos = ringbuf_send->Write_Pos;
	unsigned int Read_Pos = ringbuf_send->Read_Pos;
	unsigned int ringbuf_size = ringbuf_send->size;
	/* |+++wp-----rp+++| */
	if (Write_Pos < Read_Pos)
	{

		if ((Read_Pos - Write_Pos) > size)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{

		/* |+++rp-----sp+++| */
		if ((ringbuf_size - Write_Pos + Read_Pos) > size)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

void ipcmsg_ringbuf_read(T_RingBuf *RingBuf_rcv, void *__dest, unsigned int __data_len, unsigned int __buf_len)
{
	unsigned int read_pos = RingBuf_rcv->Read_Pos;
	unsigned int write_pos = RingBuf_rcv->Write_Pos;
	unsigned int Base_Addr = RingBuf_rcv->base_addr;
	/* |+++wp-----rp+++| */
	if ((write_pos < read_pos) && (__buf_len > RingBuf_rcv->size - read_pos))
	{
		if (__data_len > RingBuf_rcv->size - read_pos)
		{
			memcpy(__dest, (void *)(Base_Addr + read_pos), RingBuf_rcv->size - read_pos);
			memcpy((void *)((unsigned int)__dest + RingBuf_rcv->size - read_pos), (void *)Base_Addr, __data_len - RingBuf_rcv->size + read_pos);
		}
		else
			memcpy(__dest, (void *)(Base_Addr + read_pos), __data_len);
		RingBuf_rcv->Read_Pos = ALIGN_IPCMSG(__buf_len - RingBuf_rcv->size + read_pos, IPCMSG_ALIGN);
	}
	/* |---rp+++++wp---| */
	else
	{
		memcpy(__dest, (void *)(Base_Addr + read_pos), __data_len);
		RingBuf_rcv->Read_Pos += ALIGN_IPCMSG(__buf_len, IPCMSG_ALIGN); //(__n+__n%2);
	}

	if (RingBuf_rcv->Read_Pos >= RingBuf_rcv->size)
		RingBuf_rcv->Read_Pos = (RingBuf_rcv->Read_Pos % RingBuf_rcv->size);
}

void ipcmsg_ringbuf_write(T_RingBuf *RingBuf_send, void *__src, unsigned int __n)
{
	unsigned int read_pos = RingBuf_send->Read_Pos;
	unsigned int write_pos = RingBuf_send->Write_Pos;
	unsigned int Base_Addr = RingBuf_send->base_addr;
	/* |---rp+++++wp---| */
	if ((write_pos >= read_pos) && (__n > RingBuf_send->size - write_pos))
	{
		memcpy((void *)(Base_Addr + write_pos), __src, RingBuf_send->size - write_pos);
		memcpy((void *)(Base_Addr), (void *)((unsigned int)__src + RingBuf_send->size - write_pos), __n - RingBuf_send->size + write_pos);
		RingBuf_send->Write_Pos = ALIGN_IPCMSG(__n - RingBuf_send->size + write_pos, IPCMSG_ALIGN);
	}
	else /* |+++wp-----rp+++| */
	{
		memcpy((void *)(Base_Addr + write_pos), __src, __n);
		RingBuf_send->Write_Pos += ALIGN_IPCMSG(__n, IPCMSG_ALIGN); //(__n+__n%2);
	}

	if (RingBuf_send->Write_Pos >= RingBuf_send->size)
		RingBuf_send->Write_Pos = (RingBuf_send->Write_Pos % RingBuf_send->size);
}

long IpcMsg_Mutex_Create(IpcMsg_MUTEX *pMutexId)
{
	long OSAdp_ret = IpcMsg_FAILURE;

	if (NULL == pMutexId)
	{
		return IpcMsg_FAILURE;
	}

	pMutexId->pMutex = (unsigned long)xSemaphoreCreateMutex(); /* Create Mutex */
	if (0 != pMutexId->pMutex)

	{
		OSAdp_ret = IpcMsg_SUCCESS;
	}

	return OSAdp_ret;
}

long IpcMsg_Mutex_Lock(IpcMsg_MUTEX *pMutexId, unsigned long xTicksToWait)
{
	long OSAdp_ret = IpcMsg_FAILURE;

	if (NULL == pMutexId)
	{

		return IpcMsg_FAILURE;
	}

	OSAdp_ret = (long)xSemaphoreTake((QueueHandle_t)pMutexId->pMutex, xTicksToWait);

	if (OSAdp_ret == pdTRUE)

		OSAdp_ret = IpcMsg_SUCCESS;
	else
		OSAdp_ret = IpcMsg_FAILURE;

	return OSAdp_ret;
}

long IpcMsg_Mutex_Unlock(IpcMsg_MUTEX *pMutexId)
{
	if (NULL == pMutexId)
	{

		return IpcMsg_FAILURE;
	}

	xSemaphoreGive(pMutexId->pMutex);

	return IpcMsg_SUCCESS;
}

long IpcMsg_Semaphore_Create(IpcMsg_SEMAPHORE *pSemaphoreId)
{
	long OSAdp_ret = IpcMsg_FAILURE;

	if (NULL == pSemaphoreId)
	{

		return IpcMsg_FAILURE;
	}

	pSemaphoreId->pSemaphore = (unsigned long)xSemaphoreCreateBinary();
	if ((void *)0 != pSemaphoreId->pSemaphore)

	{
		OSAdp_ret = IpcMsg_SUCCESS;
	}

	return OSAdp_ret;
}

long IpcMsg_Semaphore_Take(IpcMsg_SEMAPHORE *pSemaphoreId, unsigned long xTicksToWait)
{
	long OSAdp_ret = IpcMsg_FAILURE;

	if (NULL == pSemaphoreId)
	{

		return IpcMsg_FAILURE;
	}

	OSAdp_ret = (long)xSemaphoreTake((QueueHandle_t)pSemaphoreId->pSemaphore, xTicksToWait);

	if (OSAdp_ret == pdTRUE)
		OSAdp_ret = IpcMsg_SUCCESS;
	else
		OSAdp_ret = IpcMsg_FAILURE;

	return OSAdp_ret;
}

long IpcMsg_Semaphore_Give(IpcMsg_SEMAPHORE *pSemaphoreId)
{
	//BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
	if (NULL == pSemaphoreId)
	{
		return IpcMsg_FAILURE;
	}
	xSemaphoreGive(pSemaphoreId->pSemaphore);
	return IpcMsg_SUCCESS;
}

__RAM_FUNC void IpcMsg_Semaphore_Give_ISR(IpcMsg_SEMAPHORE *pSemaphoreId)
{
	BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
	if (NULL == pSemaphoreId)
	{
		return;
	}
	if (xSemaphoreGiveFromISR(pSemaphoreId->pSemaphore, &pxHigherPriorityTaskWoken) == pdPASS)
	{
		portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
	}
}

static unsigned int get_ipcmsg_buf_len(unsigned int ch_id)
{
	int i;
	for (i = 0; i < IpcMsg_Channel_MAX; i++)
	{
		if (ch_id == IpcMsg_Resource_Tbl[i].Ch_ID)
			return (IpcMsg_Resource_Tbl[i].block_cnt) * (IpcMsg_Resource_Tbl[i].block_size);
	}
	xy_assert(!"[IPCMSG]get_ipcmsg_buf_len fail !");
	return 0;
}

__RAM_FUNC uint32_t Ipc_SetInt(void)
{
	PRCM_CpApIntWkupTrigger();
	//可能存在双核同时写，所以写完要判断一下，核间buff是否为空
	if(Is_Ringbuf_Empty(g_IpcMsg_ChInfo[IpcMsg_Normal].RingBuf_rcv) != true)
	{
		return 1;
	}

	return 0;
}

__RAM_FUNC uint32_t icm_buf_check(void)
{
	int ret = 0;
	int i;

	if (IpcMsg_check_init_flag(g_IpcMsg_ChInfo[0].RingBuf_send) == false)
		return ret;

	for(i = 0; i<IpcMsg_Channel_MAX; i++)
	{
		osCoreEnterCritical();
		//AP死机通知CP核导dump
	    if(HWREGB(BAK_MEM_AP_DO_DUMP_FLAG))
	    {
	    	xy_assert(0);
	    }

		if(Is_Ringbuf_Empty(g_IpcMsg_ChInfo[i].RingBuf_rcv) != true)
		{
			ret = 1;
			if(osCoreGetState() == osCoreInInterrupt)
				IpcMsg_Semaphore_Give_ISR(&(g_IpcMsg_ChInfo[i].read_sema));
			else
				IpcMsg_Semaphore_Give(&(g_IpcMsg_ChInfo[i].read_sema));
		}
		osCoreExitCritical();
	}

	return ret;
}

__RAM_FUNC void WAKEUP_Handler(void)
{
#if RUNTIME_DEBUG
	extern uint32_t xy_runtime_get_enter(void);
	uint32_t time_enter = xy_runtime_get_enter();
#endif
	// clear aonprcm cp wakeup int
	AONPRCM->WAKUP_INT0  = 0x10;

	if(PRCM_ApCpIntWkupGet() != 0){
		PRCM_ApCpIntWkupClear();
		extern void check_force_stop_cp(void);
		check_force_stop_cp();
		
		icm_buf_check();
	}

#if RUNTIME_DEBUG
	extern void xy_runtime_get_exit(uint32_t id, uint32_t time_enter);
	xy_runtime_get_exit(WAKEUP_IRQn, time_enter);
#endif
}


//check all ipcmsg channel init flag by AP, true:success,false:failed
__RAM_FUNC int IpcMsg_check_init_flag(T_RingBuf *pMsg)
{
	int result = false;
	if (pMsg == NULL)
	{
		result = false;
		goto END;
	}

	if (g_all_ipc_channel_init)
	{
		result = true;
		goto END;
	}

	osCoreEnterCritical();

	if (*(unsigned int *)((unsigned long)ALIGN_IPCMSG(IPCMSG_BUF_BASE_ADDR, IPCMSG_ALIGN)) == get_ipcmsg_buf_len(IpcMsg_Normal))
	{
		result = true;
		g_all_ipc_channel_init = 1;
	}
	else
	{
		result = false;
		g_all_ipc_channel_init = 0;
	}
	osCoreExitCritical();
END:
	return result;
}

void ringbuf_ctl_init(T_RingBuf *pringbuf_ctl, T_RingBuf *pIpcMsg_ChInfo)
{
	pringbuf_ctl->base_addr = pIpcMsg_ChInfo->base_addr + SRAM_AP_CP_OFFSET;
	pringbuf_ctl->size = pIpcMsg_ChInfo->size;
	pringbuf_ctl->Read_Pos = pIpcMsg_ChInfo->Read_Pos;
	pringbuf_ctl->Write_Pos = pIpcMsg_ChInfo->Write_Pos;
}

int IpcMsg_Read(T_IpcMsg_Msg *pMsg, unsigned long xTicksToWait)
{
	T_RingBuf ringbuf_rcv = {0};
	T_IpcMsg_Head tmpMsgHeader;
	unsigned int result_len = 0;

	//cp,AP ipcmsg must be inited
	if (IpcMsg_check_init_flag(g_IpcMsg_ChInfo[0].RingBuf_rcv) == false)
		return -1;

	if (Is_Ringbuf_Empty(g_IpcMsg_ChInfo[0].RingBuf_rcv) == true)
	{
		IpcMsg_Semaphore_Take(&(g_IpcMsg_ChInfo[0].read_sema), xTicksToWait);
	}
	taskENTER_CRITICAL();

	if (Is_Ringbuf_Empty(g_IpcMsg_ChInfo[0].RingBuf_rcv) == true)
	{
		taskEXIT_CRITICAL();
		return 0;
	}
	ringbuf_ctl_init(&ringbuf_rcv, g_IpcMsg_ChInfo[0].RingBuf_rcv);
	taskEXIT_CRITICAL();

	/* get msg header */
	ipcmsg_ringbuf_read(&ringbuf_rcv, &tmpMsgHeader, sizeof(T_IpcMsg_Head), sizeof(T_IpcMsg_Head));

	if (pMsg->len < (unsigned int)(tmpMsgHeader.data_len))
	{
		//taskEXIT_CRITICAL();
		xy_assert(0);
		return -1;
	}
	pMsg->id = tmpMsgHeader.id;
	result_len = tmpMsgHeader.data_len;
	ipcmsg_ringbuf_read(&ringbuf_rcv, pMsg->buf, result_len, ALIGN_IPCMSG(result_len + sizeof(T_IpcMsg_Head), IPCMSG_ALIGN) - sizeof(T_IpcMsg_Head));

	taskENTER_CRITICAL();
	g_IpcMsg_ChInfo[0].RingBuf_rcv->Read_Pos = ringbuf_rcv.Read_Pos;

	taskEXIT_CRITICAL();

	return result_len;
}

int IpcMsg_Write(T_IpcMsg_Msg *pMsg)
{
	unsigned int size = 0;
	T_RingBuf ringbuf_send = {0};
	T_IpcMsg_Head tmpMsgHeader;

	//cp,AP ipcmsg must be inited
	if (IpcMsg_check_init_flag(g_IpcMsg_ChInfo[0].RingBuf_send) == false)
		return -1;

	size = pMsg->len + sizeof(T_IpcMsg_Head);
	size = ALIGN_IPCMSG(size, IPCMSG_ALIGN);

	/*IPC通道自身没有对每条消息的长度做出限制，此处设置该宏值仅仅是为了方便接收时固定缓存上限。有效数据长度不得超过123*/
	//xy_assert(pMsg->len!=0 && pMsg->len<IPCMSG_SINGLE_BUFF_MAX_SIZE-sizeof(T_IpcMsg_Head));
	xy_assert(pMsg->len<IPCMSG_SINGLE_BUFF_MAX_SIZE-sizeof(T_IpcMsg_Head));

	IpcMsg_Mutex_Lock(&(g_IpcMsg_ChInfo[0].write_mutex), IPC_WAITFROEVER);
	taskENTER_CRITICAL();

	ringbuf_ctl_init(&ringbuf_send, g_IpcMsg_ChInfo[0].RingBuf_send);

	taskEXIT_CRITICAL();
	if (Is_Ringbuf_ChFreeSpace(&ringbuf_send, size) == true)
	{

		tmpMsgHeader.id = (unsigned short)(pMsg->id);
		tmpMsgHeader.data_len = (unsigned short)(pMsg->len);

		ipcmsg_ringbuf_write(&ringbuf_send, (void *)&tmpMsgHeader, sizeof(T_IpcMsg_Head));
		if (pMsg->len)
			ipcmsg_ringbuf_write(&ringbuf_send, pMsg->buf, size - sizeof(T_IpcMsg_Head));
	}
	else
	{
		IpcMsg_Mutex_Unlock(&(g_IpcMsg_ChInfo[0].write_mutex));
		//xy_assert(0);
		return -1;
	}

	taskENTER_CRITICAL();
	g_IpcMsg_ChInfo[0].RingBuf_send->Write_Pos = ringbuf_send.Write_Pos;

	taskEXIT_CRITICAL();

	if(Ipc_SetInt())
	{
		icm_buf_check();
	}

	IpcMsg_Mutex_Unlock(&(g_IpcMsg_ChInfo[0].write_mutex));

	return (pMsg->len);
}

void IpcMsg_init()
{
	int i, size;
	unsigned long IpcMsg_Addr_tmp = (unsigned long)ALIGN_IPCMSG(IPCMSG_BUF_BASE_ADDR, IPCMSG_ALIGN);

	for (i = 0; i < IpcMsg_Channel_MAX; i++)
	{
		size = get_ipcmsg_buf_len(i);

		if ((IpcMsg_Resource_Tbl[i].direction & 0x1) == 1)
		{
			g_IpcMsg_ChInfo[i].RingBuf_send = (T_RingBuf *)IpcMsg_Addr_tmp;
			//ringbuf_ctl_init(&g_ringbuf_ctl[i].send_crl,g_IpcMsg_ChInfo[i].RingBuf_send);
			IpcMsg_Addr_tmp += ALIGN_IPCMSG((sizeof(T_RingBuf) + size), IPCMSG_ALIGN);
			IpcMsg_Mutex_Create(&(g_IpcMsg_ChInfo[i].write_mutex));
		}

		if ((IpcMsg_Resource_Tbl[i].direction & 0x2) == 2)
		{
			g_IpcMsg_ChInfo[i].RingBuf_rcv = (T_RingBuf *)IpcMsg_Addr_tmp;
			//ringbuf_ctl_init(&g_ringbuf_ctl[i].rcv_crl,g_IpcMsg_ChInfo[i].RingBuf_rcv);
			IpcMsg_Addr_tmp += ALIGN_IPCMSG((sizeof(T_RingBuf) + size), IPCMSG_ALIGN);
			IpcMsg_Semaphore_Create(&(g_IpcMsg_ChInfo[i].read_sema));
		}

		g_IpcMsg_ChInfo[i].flag = i;

		if (IpcMsg_Addr_tmp - IPCMSG_BUF_BASE_ADDR > IPCMSG_BUF_LEN)
			xy_assert(0);
	}
}
