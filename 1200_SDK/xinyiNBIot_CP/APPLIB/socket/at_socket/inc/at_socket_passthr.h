/**
 * @file at_socket_passthr.h
 * @brief socket透传代码。芯翼主要支持BC95 socket透传以及socket ppp透传方式。
 * 需要注意的是BC95类型的socket透传是有结束符表示数据包接收完毕的，而socket ppp方式以csp fifo为空来判定用户数据发送结束。
 * 推荐用户使用BC95 socket透传方式，socket ppp方式仅供参考
 * @version 1.0
 * @date 2022-03-08
 * @copyright Copyright (c) 2022  芯翼信息科技有限公司
 * 
 */
#pragma once

#include "xy_utils.h"
#include "lwip/sockets.h"

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
typedef struct socket_passthr_info
{
	uint8_t socket_id;		//socket标识，一般由NSOCR命令创建返回
	uint8_t rai_flag;		//rai标识
	uint8_t sequence;		//sequence,取值[0-255]，0表示不上报数据发送状态
	uint8_t result;			//socket透传结果标识，用于退出透传时根据返回值来上报不同的URC信息
	uint32_t pre_sn;		//tcp sn号，udp为0
	uint32_t send_length;	//发送的数据长度
	uint8_t	data_send_mode;	//发送数据的格式
	uint8_t udp_connect;
	uint8_t data_echo;
	uint8_t reserved;
	struct sockaddr_storage sock_addr;	//远端服务器socket addr
} socket_passthr_info_t;

/*******************************************************************************
 *                       Global variable declarations                          *
 ******************************************************************************/
extern socket_passthr_info_t* g_socket_passthr_info; //记录socket的全局信息，用于透传线程

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
/* 固定长度socket透传处理接口 */
void socket_fixed_length_passthr_proc(char *buf, uint32_t len);
/* 固定长度socket透传退出定制接口 */
void socket_fixed_length_passthr_exit(void);
/* 非固定长度socket透传处理接口，以CTRLZ字符结束或者ESC取消发送 */
void socket_unfixed_length_passthr_proc(char *buf, uint32_t len);
/* 非固定长度socket透传退出定制接口 */
void socket_unfixed_length_passthr_exit(void);
