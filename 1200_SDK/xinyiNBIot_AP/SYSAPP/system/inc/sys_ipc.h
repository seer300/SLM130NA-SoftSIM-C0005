/**
 * @file    hal_ipc.h
 * @brief   此文件包含核间通信相关的变量，枚举，结构体定义，函数声明等.
 * @version 1.0
 *******************************************************************************
 * @attention
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 * http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

#pragma once

#include <stdint.h>

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/


/*IPC通道自身没有对每条消息的长度做出限制，此处设置该宏值仅仅是为了方便接收时固定缓存上限。有效数据长度不得超过123*/
#define IPCMSG_SINGLE_BUFF_MAX_SIZE  128

/**
 * @brief	核间消息类型枚举
 */
typedef enum
{
	ICM_FLASHWRITE = 0,	 /*!< AP核通知CP核执行写flash动作，期间会挂起AP核 */
	ICM_FLASHWRT_NOTICE, /*!< CP核执行flash本地写操作之前，通过该消息通知AP核挂起，以防止退出XIP模式时AP核发生异常 */
	ICM_CHANGE_CP_STATE, /*!< AP与CP之间进行状态信息传递，AP发送RAI/STOP等命令给CP；CP发送FOTA状态给AP*/
	ICM_MEM_FREE,		 /*!< 用于释放核间通讯申请的内存，通常为AT命令零拷贝*/
	ICM_AT_ASYNC,		 /*!< AP核本地的异步AT命令机制使用，即Send_AT_Req与Get_AT_Rsp异步模式，或者纯AT虚拟通道模式at_uart_write*/
	ICM_AT_SYNC,		 /*!< AT_Send_And_Get_Rsp应答结果AT字符串，不包含URC，要求中间结果必须与OK作为一条命令发送过来*/
	ICM_AT_EXT,			 /*!< 外部MCU通过LPUART发送AT请求给AP核，CP核将应答报文或URC经LPUART发送给MCU*/
	ICM_NV_WRITE,        /*!< AP通知CP核执行NV某参数的保存*/
	ICM_APAT_DELAY,		 /*!< AP的AT命令通知CP刷新延迟锁*/
	ICM_SOFT_RESET,		 /*!< CP核业务执行的软复位API接口消息*/
	ICM_AP_LOG,          /*!< 当CP核启动后，AP核的log通过CP核的logview输出*/
	ICM_COPY_AT,         /*!< 用于非零拷贝方式上报URC,如"POWDOWN"*/
	ICM_MAX,			 /*!< MAX*/
} IPC_MessageID;

/**
 * @brief	核间消息结构体
 */
typedef struct
{
	IPC_MessageID id; /*!< 核间消息标志位 */
	void *buf;		  /*!< 指向核间数据区域的指针 */
	unsigned int len; /*!< 核间消息长度 */
} IPC_Message;

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
 

/**
 * @brief	写核间消息，内部动态初始化核间通道
 * @param	pMsg 核间消息结构体
 * @return	函数执行状态
 * @retval  -1       ：函数执行失败
 * @retval  int32_t  ：函数执行成功，返回成功写入的数据长度，单位为字节
 */
int32_t IPC_WriteMessage(IPC_Message *pMsg);

/**
 * @brief	核间消息处理函数.在中断中被调用.
 * @return	函数执行状态
 * @retval  1	：表示函数执行成功
 */
uint8_t IPC_ProcessEvent(void);

/**
 * @brief	核间通道初始化.
 */
void IPC_Init(void);

void IPC_NoticeDump(void);




