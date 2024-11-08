#include "basic_config.h"
#include "user_config.h"
#if CLOUDTYPE == 2
/**
 * @file       send_by_socket.c
 * @brief      该源文件执行的是通过socket的标准AT命令进行数据的发送。\n
 * @attention  使用该demo前，确保socket通信可以正常使用\n
 * @warning   部分socket命令("NSOCR""NSORF")中间结果没有前缀，收到相应命令的回复后自动添加前缀以便于读取接口匹配
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xy_printf.h"
#include "data_gather.h"
#include "cloud_process.h"
#include "at_process.h"
#include "err_process.h"



typedef enum
{
	AT_CLOUD_INIT = 0,
	AT_SOCKET_INIT = AT_CLOUD_INIT,
	AT_WAIT_CONNECT,
	AT_WAIT_SEND_RSP,
	AT_SOCKET_READY,   //表示处于待发送数据态，若内部检查无数据发送，则会退出大循环
}AT_SOCKET_STATE;


//用户自行配置端口远端服务地址
#define SOCKET_LOCAL_PORT 				10005
#define SOCKET_REMOTE_IP 				"112.125.89.8"
#define SOCKET_REMOTE_TCP_PORT 			33072
#define SOCKET_REMOTE_UDP_PORT 			33488
#define SYN_SOCK_AT_TIMEOUT             10   //sec
#define SOCKET_RECV_TIMEOUT             30   //sec
#define ASYN_SOCK_AT_TIMEOUT            25   //sec,socket异步AT命令超时时间，tcp连接可能耗时较长，lwip里面是10s超时，会重发一次，一共要20s


int g_socket_at_step = 0;
int g_socket_id = -1;
uint64_t g_sock_send_tick = 0;
int g_sock_recv_data = 0;


/**
 * CP核socket接收模式必须设置为上报下行数据信息且缓存模式
 * +NSONMI:<socketid>,<length>
 * AT+NSORF=<socketid>,<req_length>
 * +NSORF:<socketid>,<ip_addr>,<port>,<length>,<data>,<remaining_length>
 */
void URC_NSONMI_Proc(char *paramlist)
{
	//第1步：解析NSONMI URC
	int socket_id = 0;
	int data_len = 0;
	int read_len = 0;
	at_parse_param("%d,%d", paramlist, &socket_id, &data_len);

	xy_printf("socket recv nsonmi, id:%d, data_length:%d\n", socket_id, data_len);

	//第2步：发送NSORF命令获取缓存数据
	int len = 0;
	char *data_param = NULL;
	At_status_type at_ret = XY_OK;
	char *at_sock_getdata = xy_malloc(48);

	while(data_len)
	{
		read_len = data_len;
		if (read_len > SOCKET_SEND_MAX_LENGTH)
			read_len = SOCKET_SEND_MAX_LENGTH;
			
		snprintf(at_sock_getdata, 48, "AT+NSORF=%d,%d\r\n", socket_id, read_len);
		data_param = xy_malloc(3000);
		
		at_ret = AT_Send_And_Get_Rsp(at_sock_getdata, SYN_SOCK_AT_TIMEOUT, "+NSORF:", ",,,%d,%s",&len,data_param);
		
		if (at_ret != XY_OK)
			return;


		//用户自行处理数据,数据长度为bytes字节数，data_param返回十六进制形式，需做转换
		xy_printf("socket get[%d] len data:%s\n", len, data_param);
		data_len -= len;
		
		xy_free(data_param);
		data_param = NULL;
	}
	// 设置标记位提示已收到数据
	g_sock_recv_data = 1;

	xy_free(at_sock_getdata);
}




int Creat_UDP_Socket(void)
{
	At_status_type	at_ret = XY_OK;
    char *at_cmd = NULL;

	at_cmd = xy_malloc(48);
	
	snprintf(at_cmd, 48, "AT+NSOCR=DGRAM,17,%d,1\r\n", SOCKET_LOCAL_PORT);

	// 创建socket
	at_ret = AT_Send_And_Get_Rsp(at_cmd, SYN_SOCK_AT_TIMEOUT, "+NSOCR:", "%d", &g_socket_id);

	if (at_ret == XY_OK)
	{
		//创建成功设置主动上报模式
		at_ret = AT_Send_And_Get_Rsp("AT+NSONMI=1\r\n", SYN_SOCK_AT_TIMEOUT, NULL, NULL);
	}
	
	free(at_cmd);
	return at_ret;
}

At_status_type AT_Send_by_Socket(void *data,int len)
{
	At_status_type	at_ret = XY_OK;
	char *SendDataHex = NULL;
	char *at_cmd = NULL;
	
    SendDataHex = xy_malloc(len * 2 + 1);
    at_cmd = xy_malloc(50 + len * 2 + 1);
    memset(SendDataHex, 0, len * 2 + 1);
    memset(at_cmd, 0, 50 + len * 2 + 1);

    //将读取到的所有数据转换成十六进制形式的字符串
    bytes2hexstr((uint8_t *)(data), len, SendDataHex, (len * 2 + 1));


	snprintf(at_cmd, 50 + len * 2, "AT+NSOST=%d,%s,%d,%d,%s\r\n",g_socket_id, SOCKET_REMOTE_IP, SOCKET_REMOTE_UDP_PORT, len, (char *)SendDataHex);

	xy_free(SendDataHex);

	at_ret = AT_Send_And_Get_Rsp(at_cmd, SYN_SOCK_AT_TIMEOUT, NULL, NULL);

    xy_free(at_cmd);

	return at_ret;
}

At_status_type Close_Socket(void)
{
	At_status_type  at_ret = -1;
	char *at_cmd = NULL;
	
	at_cmd = xy_malloc(48);
	
	snprintf(at_cmd, 48, "AT+NSOSTF=%d,%s,%d,0x200,1,01\r\n",g_socket_id, SOCKET_REMOTE_IP, SOCKET_REMOTE_UDP_PORT);
	at_ret = AT_Send_And_Get_Rsp(at_cmd, SYN_SOCK_AT_TIMEOUT, NULL, NULL);
	
	if(at_ret != XY_OK)
		goto end;

	snprintf(at_cmd, 48, "AT+NSOCL=%d\r\n",g_socket_id);
	at_ret = AT_Send_And_Get_Rsp(at_cmd, SYN_SOCK_AT_TIMEOUT, NULL, NULL);

	if(at_ret != XY_OK)
		goto end;

	g_socket_id = -1;

end:	
	free(at_cmd);
	return at_ret;
}


At_status_type Send_Data_By_Socket(void *data,int len,int *send_ret)
{
	At_status_type  at_ret = -1;

	switch (g_socket_at_step)
	{
		case AT_SOCKET_INIT:
		{
			char *at_cmd = NULL;
			if (-1 == g_socket_id)
			{
CREATE:
				at_ret = Creat_UDP_Socket();
				if(at_ret != XY_OK)
					break;
				g_socket_at_step = AT_SOCKET_READY;
			}
			else
			{
				//创建过则查询sokcet当前状态
				int socket_status = -1;
				at_cmd = xy_malloc(48);
				snprintf(at_cmd, 48, "AT+NSOSTATUS=%d\r\n", g_socket_id);
				at_ret = AT_Send_And_Get_Rsp(at_cmd, SYN_SOCK_AT_TIMEOUT, "+NSOSTATUS:", ",%d", &socket_status);
				free(at_cmd);
				
				if(at_ret != XY_OK)
					break;

				//socket状态 0:可用 1:不存在
				if (socket_status == 1)
				{
					goto CREATE;
				}
				else
				{
					g_socket_at_step = AT_SOCKET_READY;
				}
			}
			break;
		}

		//当前处在待发送数据态，若没有任何数据待发送，将会在此退出大循环
		case AT_SOCKET_READY:
		{
			if(is_event_set(EVENT_CLOUD_SEND) && data != NULL)
			{
				at_ret = AT_Send_by_Socket(data,len);

				if(at_ret != XY_OK)
					break;
				
				g_socket_at_step = AT_WAIT_SEND_RSP;
				
				if(g_sock_send_tick == 0)
					g_sock_send_tick = xy_get_ms();
			}
						
			
			//若没有事可做，退回初始态
			if(!is_event_set(EVENT_CLOUD_SEND))
			{
				g_socket_at_step = AT_CLOUD_INIT;
			}
			break;
		}
		case AT_WAIT_SEND_RSP:
		{
			if (g_sock_recv_data)
			{
				xy_printf("send succuss!\n");

				at_ret = Close_Socket();
				
				if(at_ret != XY_OK)
					break;
				
				//接收到下行，说明发送成功，关闭socket，等待下次流程
				// 发送计时清空
				g_sock_send_tick = 0;
				g_sock_recv_data = 0;
				
				*send_ret = 0;
				g_socket_at_step = AT_CLOUD_INIT;
			}
			else
			{
				//如果发送上行后，接收下行数据超时，则认为发送失败
				if(xy_get_ms() - g_sock_send_tick > (uint64_t)(SOCKET_RECV_TIMEOUT*1000))
				{
					at_ret = Close_Socket();
					
					if(at_ret != XY_OK)
						break;
				
					*send_ret = 1;
					xy_printf("send falied!\n");
					g_socket_at_step = AT_CLOUD_INIT;					
					g_sock_send_tick = 0;
				}
			}
			break;
		}
		default:
			break;
	}

	return at_ret;
}

#endif
