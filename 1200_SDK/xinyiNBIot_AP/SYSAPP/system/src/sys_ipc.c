/**
 * @file hal_ipc.c
 * @brief
 * @version 1.0
 * @date 2021-07-10
 * @copyright Copyright (c) 2021  芯翼信息科技有限公司
 *
 */
#include "sys_ipc.h"
#include "xy_memmap.h"
#include "hw_types.h"
#include "system.h"
#include "hw_uart.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "xinyi2100.h"
#include "at_uart.h"
#include "xy_printf.h"
#include "xy_system.h"
#include "user_ipc_msg.h"
#include "zero_copy.h"
#include "sys_proc.h"
#include "xy_fota.h"
#include "at_ipc_process.h"
/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
/**
 * @brief	Users don't need to care
 */
typedef struct
{
  unsigned short data_len;
  unsigned short data_flag;
} IPC_MessageHead;

/**
 * @brief	Users don't need to care
 */
typedef struct
{
  unsigned int size;
  unsigned int base_addr;
  unsigned int Write_Pos;
  unsigned int Read_Pos;
} IPC_Buffer_CB;

/**
 * @brief	Users don't need to care
 */
typedef struct
{
  IPC_Buffer_CB *sendBuffer;
  IPC_Buffer_CB *recieveBuffer;
} IPC_CB;



/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define IPCMSG_ALIGN                 0x04
#define ALIGN_IPCMSG(size, align)    (((unsigned int)size + align - 1U) & (~(align - 1U)))
#define IPCMSG_BUFF_SIZE 			 ALIGN_IPCMSG(((IPCMSG_BUF_LEN - (sizeof(IPC_Buffer_CB) << 1)) >> 1), IPCMSG_ALIGN)


/*******************************************************************************
 *                       Global variable declarations                          *
 ******************************************************************************/
IPC_CB g_IpcMsg_Channel = {0};

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/

__OPENCPU_FUNC bool Is_Ringbuf_Empty(IPC_Buffer_CB *ringbuf)
{
	if(ringbuf == NULL)
		return true;

	unsigned int Write_Pos = ringbuf->Write_Pos;
	unsigned int Read_Pos = ringbuf->Read_Pos;

	if (Write_Pos == Read_Pos)
		return true;
	else
		return false;
}


__OPENCPU_FUNC bool Is_IPC_SendBuf_Empty(void)
{
	if (Is_Ringbuf_Empty(g_IpcMsg_Channel.sendBuffer) != true)
	{
		return false;
	}
	return true;
}

__OPENCPU_FUNC bool Is_IPC_RecvBuf_Empty(void)
{
	if (Is_Ringbuf_Empty(g_IpcMsg_Channel.recieveBuffer) != true)
	{
		return false;
	}
	return true;
}

__OPENCPU_FUNC bool Is_RingBuf_Write_Access(IPC_Buffer_CB *ringbuf, unsigned int size)
{
	if(ringbuf == NULL)
		return false;

	unsigned int Write_Pos = ringbuf->Write_Pos;
	unsigned int Read_Pos = ringbuf->Read_Pos;
	unsigned int ringbuf_size = ringbuf->size;
	/* |+++wp-----rp+++| */
	if (Write_Pos < Read_Pos)
	{
    	if ((Read_Pos - Write_Pos) > size){
			return true;
		}
		else{
			return false;
		}
	}
	else{

		/* |---rp+++++wp---| */
		if ((ringbuf_size - Write_Pos + Read_Pos ) > size){
			return true;
		}
		else{
			return false;
		}
	}
}

__OPENCPU_FUNC static void IPC_ReadBuffer(IPC_Buffer_CB* recieveBuffer, void *dest, unsigned int __data_len,unsigned int __buf_len)
{
	unsigned int read_pos = recieveBuffer->Read_Pos;
	unsigned int write_pos = recieveBuffer->Write_Pos;
	unsigned int Base_Addr = recieveBuffer->base_addr;
	unsigned int tmp;
	/* |+++wp-----rp+++| */
	if((write_pos < read_pos)&&(__buf_len > recieveBuffer->size - read_pos))
	{
		if(__data_len > recieveBuffer->size - read_pos)
		{
			memcpy(dest, (void *)(Base_Addr + read_pos), recieveBuffer->size - read_pos);
			memcpy((void *)((unsigned int)dest + recieveBuffer->size - read_pos), (void *)Base_Addr, __data_len - recieveBuffer->size + read_pos);
		}
		else
			memcpy(dest, (void *)(Base_Addr + read_pos), __data_len);

		tmp = ALIGN_IPCMSG(__buf_len - recieveBuffer->size + read_pos ,IPCMSG_ALIGN);
		memcpy(&recieveBuffer->Read_Pos,&tmp,4);
	}
	/* |---rp+++++wp---| */
	else
	{
		memcpy(dest, (void*)(Base_Addr + read_pos), __data_len);
		tmp = recieveBuffer->Read_Pos + ALIGN_IPCMSG(__buf_len,IPCMSG_ALIGN);
		memcpy(&recieveBuffer->Read_Pos,&tmp,4);
	}

	if(recieveBuffer->Read_Pos >= recieveBuffer->size)
	{
		tmp = (recieveBuffer->Read_Pos % recieveBuffer->size);
		memcpy(&recieveBuffer->Read_Pos,&tmp,4);
	}
}

__OPENCPU_FUNC static void IPC_WriteBuffer(IPC_Buffer_CB* sendBuffer, void *__src, unsigned int __n)
{
	unsigned int read_pos = sendBuffer->Read_Pos;
	unsigned int write_pos = sendBuffer->Write_Pos;
	unsigned int Base_Addr = sendBuffer->base_addr;
	unsigned int tmp;
	/* |---rp+++++wp---| */
	if((write_pos >= read_pos) && (__n > sendBuffer->size - write_pos) )
	{
		memcpy((void*)(Base_Addr + write_pos), __src, sendBuffer->size - write_pos);
		memcpy((void*)(Base_Addr), (void*)((unsigned int)__src + sendBuffer->size - write_pos), __n - sendBuffer->size + write_pos);
		tmp = ALIGN_IPCMSG(__n- sendBuffer->size + write_pos,IPCMSG_ALIGN);
		memcpy(&sendBuffer->Write_Pos,&tmp,4);
	}
	else/* |+++wp-----rp+++| */
	{
		memcpy((void*)(Base_Addr + write_pos), __src, __n);
		tmp = sendBuffer->Write_Pos + ALIGN_IPCMSG(__n,IPCMSG_ALIGN);
		memcpy(&sendBuffer->Write_Pos,&tmp,4);
	}

	if(sendBuffer->Write_Pos >= sendBuffer->size)
	{
		tmp = (sendBuffer->Write_Pos % sendBuffer->size);
		memcpy(&sendBuffer->Write_Pos,&tmp,4);
	}
}

__OPENCPU_FUNC int32_t IPC_ReadMessage(IPC_Message *pMsg)
{
	IPC_Buffer_CB recieveBuffer = {0};
	IPC_MessageHead tmpMsgHeader = {0};
	int32_t result_len = 0;

	memcpy(&recieveBuffer, g_IpcMsg_Channel.recieveBuffer, sizeof(IPC_Buffer_CB));

	if (Is_Ringbuf_Empty(&recieveBuffer) == true)
	{
		return -1;
	}

	/* get msg header */
	IPC_ReadBuffer(&recieveBuffer, &tmpMsgHeader, sizeof(IPC_MessageHead), sizeof(IPC_MessageHead));
	if (pMsg->len < tmpMsgHeader.data_len)
	{
		xy_assert(0);
		return -1;
	}
	pMsg->id = tmpMsgHeader.data_flag;
	result_len = tmpMsgHeader.data_len;
	IPC_ReadBuffer(&recieveBuffer, pMsg->buf, tmpMsgHeader.data_len, ALIGN_IPCMSG(tmpMsgHeader.data_len + sizeof(IPC_MessageHead), IPCMSG_ALIGN) - sizeof(IPC_MessageHead));

	g_IpcMsg_Channel.recieveBuffer->Read_Pos = recieveBuffer.Read_Pos;

	return result_len;
}

__OPENCPU_FUNC int32_t IPC_WriteMessage(IPC_Message *pMsg)
{
	uint32_t size = 0;
	IPC_Buffer_CB sendBuffer = {0};

	IPC_MessageHead tmpMsgHeader;

	/*IPC通道自身没有对每条消息的长度做出限制，此处设置该宏值仅仅是为了方便接收时固定缓存上限。有效数据长度不得超过123*/
	//xy_assert(pMsg->len!=0 && pMsg->len<IPCMSG_SINGLE_BUFF_MAX_SIZE - sizeof(IPC_MessageHead));
	xy_assert(pMsg->len<IPCMSG_SINGLE_BUFF_MAX_SIZE - sizeof(IPC_MessageHead));

	DisablePrimask();

	size = pMsg->len + sizeof(IPC_MessageHead);
	size = ALIGN_IPCMSG(size, IPCMSG_ALIGN);

	memcpy(&sendBuffer, g_IpcMsg_Channel.sendBuffer, sizeof(IPC_Buffer_CB));

	if (Is_RingBuf_Write_Access(&sendBuffer, size) == true)
	{
		tmpMsgHeader.data_flag = (unsigned short)(pMsg->id);
		tmpMsgHeader.data_len = (unsigned short)(pMsg->len);

		IPC_WriteBuffer(&sendBuffer, (void *)&tmpMsgHeader, sizeof(IPC_MessageHead));

		if (pMsg->len)
		{
			IPC_WriteBuffer(&sendBuffer, pMsg->buf, size - sizeof(IPC_MessageHead));
		}
	}
	else
	{
#if MODULE_VER
		EnablePrimask();
		return -1;
#else

		EnablePrimask();
		g_errno = XY_ERR_IPC_FAIL;
		return -1;
#endif
	}

	g_IpcMsg_Channel.sendBuffer->Write_Pos = sendBuffer.Write_Pos;

	int32_t value = (int32_t)pMsg->len;

	EnablePrimask();

	//触发IPC中断
    //CP睡眠状态时，log信息不触发唤醒CP，防止出现始终无法睡眠情况;但当log信息积压到一定数目时，避免耗完核间信息空间，还是需要唤醒CP继续打印
    #define LOG_NOT_TRIGGER_CP_WAKEUP_MAXNUM 5
    static uint8_t s_icm_log_not_trigger_cp_wakeup = 0;
    if( (pMsg->id == ICM_AP_LOG) && (CP_IS_DEEPSLEEP() || CP_IS_STANDBY()) )
    {
        s_icm_log_not_trigger_cp_wakeup++;
        if(s_icm_log_not_trigger_cp_wakeup >= LOG_NOT_TRIGGER_CP_WAKEUP_MAXNUM)
        {
            s_icm_log_not_trigger_cp_wakeup = 0;
            PRCM_ApCpIntWkupTrigger();
        }
    }
    else
    {
        s_icm_log_not_trigger_cp_wakeup = 0;
        PRCM_ApCpIntWkupTrigger();
    }
	
	return (value);
}

__OPENCPU_FUNC void IPC_Init_WAKEUP_DSLEEP(void)
{
	unsigned long IpcMsg_Addr_tmp = (unsigned long)ALIGN_IPCMSG(IPCMSG_BUF_BASE_ADDR,IPCMSG_ALIGN);

	g_IpcMsg_Channel.recieveBuffer = (IPC_Buffer_CB*)IpcMsg_Addr_tmp;

	IpcMsg_Addr_tmp += ALIGN_IPCMSG((sizeof(IPC_Buffer_CB) + IPCMSG_BUFF_SIZE),IPCMSG_ALIGN);

	g_IpcMsg_Channel.sendBuffer = (IPC_Buffer_CB*)IpcMsg_Addr_tmp;
}

__OPENCPU_FUNC void IPC_Init(void)
{
	unsigned long IpcMsg_Addr_tmp = (unsigned long)ALIGN_IPCMSG(IPCMSG_BUF_BASE_ADDR,IPCMSG_ALIGN);

	g_IpcMsg_Channel.recieveBuffer = (IPC_Buffer_CB*)IpcMsg_Addr_tmp;
	g_IpcMsg_Channel.recieveBuffer->base_addr = IpcMsg_Addr_tmp + sizeof(IPC_Buffer_CB);
	g_IpcMsg_Channel.recieveBuffer->size = IPCMSG_BUFF_SIZE;
	g_IpcMsg_Channel.recieveBuffer->Read_Pos = 0;
	g_IpcMsg_Channel.recieveBuffer->Write_Pos = 0;
	IpcMsg_Addr_tmp += ALIGN_IPCMSG((sizeof(IPC_Buffer_CB) + IPCMSG_BUFF_SIZE),IPCMSG_ALIGN);

	g_IpcMsg_Channel.sendBuffer = (IPC_Buffer_CB*)IpcMsg_Addr_tmp;
	g_IpcMsg_Channel.sendBuffer->base_addr = IpcMsg_Addr_tmp + sizeof(IPC_Buffer_CB);
	g_IpcMsg_Channel.sendBuffer->size = IPCMSG_BUFF_SIZE;
	g_IpcMsg_Channel.sendBuffer->Read_Pos = 0;
	g_IpcMsg_Channel.sendBuffer->Write_Pos = 0;
	IpcMsg_Addr_tmp += ALIGN_IPCMSG((sizeof(IPC_Buffer_CB) + IPCMSG_BUFF_SIZE),IPCMSG_ALIGN);

	if(IpcMsg_Addr_tmp - IPCMSG_BUF_BASE_ADDR > IPCMSG_BUF_LEN)
		xy_assert(0);
}

extern void CP_Fota_proc(uint32_t state);
extern void ipc_proc_Async_AT(void *data);
extern void ipc_proc_Sync_AT(void *data);
extern void ipc_proc_copy_URC(void *data);
extern void Proc_Ext_At(void *ext_at);

/*该函数运行在FLASH上，进而AP核写FLASH期间必须关中断*/
__OPENCPU_FUNC uint8_t IPC_ProcessEvent(void)
{
	int32_t ret = -1;
	IPC_Message Msg = {0};
	char rcv_buf[IPCMSG_SINGLE_BUFF_MAX_SIZE - sizeof(IPC_MessageHead)];

	while(1)
	{
		/*IPC通道自身没有对每条消息的长度做出限制，此处设置该宏值仅仅是为了方便接收时固定缓存上限。有效数据长度不得超过123*/
		Msg.len = IPCMSG_SINGLE_BUFF_MAX_SIZE - sizeof(IPC_MessageHead);
		Msg.buf = rcv_buf;
		Msg.id = 0;
		ret = IPC_ReadMessage(&Msg);

		if(ret < 0)
		{
			return 0;
		}
		/*对于空消息，为了防止脏数据，强行清零*/
		else if(ret == 0)
		{
			memset(rcv_buf, 0, 8);
		}
		/*用户自定义的扩展跨核消息，需自行打开宏*/
#if USER_IPC_MSG
		if(process_rcved_usr_msg(Msg.id,(void *)(*((uint32_t *)rcv_buf))) == 1)
			continue;
#endif
		switch(Msg.id)
		{
			case ICM_MEM_FREE:
				Delet_ZeroCopy_Buf((void *)(HWREG(rcv_buf)));
				break;
#if (XY_AT_CTL || BLE_EN)
			case ICM_AT_SYNC:
				ipc_proc_Sync_AT(rcv_buf);
				break;
#endif
			 /*Send_AT_Req与Get_AT_Rsp异步模式AT交互的处理，包含URC;或者AP核用户自定义AT框架，自行读取CP核AT命令*/
			case ICM_AT_ASYNC:				
			{
				ipc_proc_Async_AT(rcv_buf);
				break;
			}
			case ICM_COPY_AT://用于CP非零拷贝方式向AP上报"POWERDOWN" URC
				{
					ipc_proc_copy_URC(rcv_buf);
				}
				break;
			 case ICM_FLASHWRT_NOTICE:
				 {
					extern void wait_cp_flash_write_done(void);
				 	wait_cp_flash_write_done();
				 }
			 	break;

			// case ICM_WEB_LOG:
			// 	Proc_Web_Log_Cmd(rcv_buf);
			// 	break;

			case ICM_SOFT_RESET:
				/*CP核软复位API接口，如FOTA重启等*/
				{
					extern void Proc_Cp_Reset_Req(uint32_t *reset_reason);
					Proc_Cp_Reset_Req((uint32_t *)rcv_buf);
				}
				break;
			case ICM_CHANGE_CP_STATE:
				{
					CP_Fota_proc(*((uint32_t *)rcv_buf));
				}
				break;
			default:
#if XY_DUMP
				xy_assert(0);
#endif
				break;
		}
	}
}
