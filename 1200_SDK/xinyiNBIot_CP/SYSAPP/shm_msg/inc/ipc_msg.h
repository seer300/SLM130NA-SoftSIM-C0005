#pragma once

#include "cmsis_os2.h"
#include <stdbool.h>


#define IPC_WAITFROEVER portMAX_DELAY

/*IPC通道自身没有对每条消息的长度做出限制，此处设置该宏值仅仅是为了方便接收时固定缓存上限。有效数据长度不得超过123*/
#define IPCMSG_SINGLE_BUFF_MAX_SIZE     128
#define ALIGN_IPCMSG(size, align) (((unsigned int)size + align - 1) & (~(align - 1)))

#define ICM_ZERO_COPY_LIST_ENABLE       1        //开启CP发送至AP的零拷贝消息链表

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/

typedef enum
{
	CP_STOP = 0,		/*!< 通知CP核执行停止运行*/
	CP_RAI,				/*!< 通知CP核执行RAI流程，以加速回落idle态*/
	CP_CFUN,
	CP_MAX,
} ICM_CP_MsgID;


typedef enum
{
	ICM_FLASHWRITE = 0,	 /*!< AP核通知CP核执行写flash动作，期间会挂起AP核 */
	ICM_FLASHWRT_NOTICE, /*!< CP核执行flash本地写操作之前，通过该消息通知AP核挂起，以防止退出XIP模式时AP核发生异常 */
	ICM_CHANGE_CP_STATE, /*!< AP与CP之间进行状态信息传递，AP发送RAI/STOP等命令给CP；CP发送FOTA状态给AP*/
	ICM_MEM_FREE,		 /*!< 用于释放核间通讯申请的内存，通常为AT命令零拷贝*/
	ICM_AT_ASYNC,		 /*!< AP核本地的异步AT命令机制使用，即Send_AT_Req与Get_AT_Rsp异步模式，或者纯AT虚拟通道模式at_uart_write*/
	ICM_AT_SYNC,		 /*!< AT_Send_And_Get_Rsp应答结果AT字符串，不包含URC，要求中间结果必须与OK作为一条命令发送过来*/
	ICM_AT_EXT,			 /*!< 外部MCU通过AT物理串口进行的AT命令发送*/
	ICM_NV_WRITE,        /*!< AP通知CP核执行NV某参数的保存*/
	ICM_APAT_DELAY,      /*!< AP的AT命令通知CP刷新延迟锁*/
	ICM_SOFT_RESET,		 /*!< 用于软件复位*/
	ICM_AP_LOG,          /*!< 当CP核启动后，AP核的log经CP核的logview输出*/
	ICM_COPY_AT,         /*用于CP非零拷贝方式上报URC,如"POWERDOWN"*/
	ICM_MAX, /*!< MAX*/
} ICM_flag;

typedef enum
{
	IpcMsg_Normal = 0, //see  ICM_flag
	IpcMsg_Channel_MAX,
} IpcMsg_ChID;

typedef struct _T_IpcMsg_Head
{
	unsigned short data_len;
	unsigned short id;
} T_IpcMsg_Head;

typedef struct _T_RingBuf
{
	unsigned int size;
	unsigned int base_addr;
	unsigned int Write_Pos;
	unsigned int Read_Pos;
} T_RingBuf;

typedef struct
{
	unsigned long pMutex;
} IpcMsg_MUTEX;

typedef struct
{
	unsigned long pSemaphore;
} IpcMsg_SEMAPHORE;

typedef struct _T_IpcMsg
{
	T_RingBuf *RingBuf_send;
	T_RingBuf *RingBuf_rcv;
	unsigned int flag;
	IpcMsg_MUTEX write_mutex;
	IpcMsg_MUTEX read_mutex;
	IpcMsg_SEMAPHORE read_sema;
} T_IpcMsg_ChInfo;

typedef struct _T_IpcMsg_Msg
{
	unsigned int id;
	void *buf;
	unsigned int len;
} T_IpcMsg_Msg;

/*CP发送零拷贝消息给AP数据链表结构体*/
typedef struct zero_copy_list_s
{
    struct zero_copy_list_s *next;
    uint32_t msg_addr;       //消息地址
    unsigned int msg_len;    //消息长度
	unsigned int msg_file;    //消息长度
	unsigned int msg_line;    //消息长度
	const char *tskName;     //当前线程
} zero_copy_list_t;

typedef struct
{
    zero_copy_list_t *head;
    zero_copy_list_t *tail;
    int pending_num;
} zero_copy_info_t;

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/

extern int g_zero_copy_sum;
extern zero_copy_info_t *zero_copy_info;

void add_zero_copy_sum(uint32_t addr, unsigned int len);
bool shm_msg_write(void* buf, int len, unsigned int msg_type);
int IpcMsg_Read(T_IpcMsg_Msg *pMsg, unsigned long xTicksToWait);
int IpcMsg_Write(T_IpcMsg_Msg *pMsg);
int IpcMsg_check_init_flag(T_RingBuf *pMsg);
void icm_task_init();
uint32_t icm_buf_check(void);
uint32_t Ipc_SetInt(void);