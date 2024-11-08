#include "basic_config.h"
#include "user_config.h"
#if CLOUDTYPE==4
/**
* @file       send_by_http.c
* @brief      该源文件执行的是通过http的标准AT命令进行数据的发送。\n
* @warning   由于私有云开发周期长，请客户使用时务必先跟芯翼研发交流清楚，否则无法支持！
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "system.h"
#include "xy_printf.h"
#include "data_gather.h"
#include "cloud_process.h"
#include "at_process.h"
#include "err_process.h"


//http 服务器 ip，端口号配置，用户可设
#define HTTP_NET_CONFIG  "AT+HTTPCREATE=http://139.224.131.190:3000\r\n"

#define AT_HTTP_LOCALRSP_TIMEOUT   10
#define AT_HTTP_RESPONSE_TIMEOUT   60

int g_http_at_step = 0;

typedef enum
{
	AT_CLOUD_INIT = 0,
	AT_HTTP_INIT = AT_CLOUD_INIT,
	AT_HTTP_READY, //表示可以开始远程数据通信
	AT_WAIT_SEND_RSP,
}At_HTTP_STATE;
   
At_status_type AT_Send_by_Http(void *data,int len)
{
	At_status_type  at_ret = -1;
	char *SendDataHex = NULL;
	char *at_http_content = NULL;

	//配置头部信息（可选：根据发送的数据格式等进行配置，用户可设，以下采用 text/plain 格式进行发送数据，实际使用时需要使用合适的数据格式）
	at_ret = AT_Send_And_Get_Rsp("AT+HTTPHEADER=0,436f6e74656e742d547970653a20746578742f706c61696e0d0a,1\r\n", AT_HTTP_LOCALRSP_TIMEOUT, NULL, NULL);
	if(at_ret != XY_OK)
		goto exit;

	//配置头部信息（可选：关闭长连接，用户可设，根据实际环境进行配置）
	at_ret = AT_Send_And_Get_Rsp("AT+HTTPHEADER=0,436f6e6e656374696f6e3a20636c6f73650d0a,1\r\n", AT_HTTP_LOCALRSP_TIMEOUT, NULL, NULL);
	if(at_ret != XY_OK)
		goto exit;

	SendDataHex = xy_malloc(len * 2 + 1);
	at_http_content = xy_malloc(50 + len * 2);
	
	//将需要发送的数据转换成十六进制形式的字符串
	bytes2hexstr((uint8_t *)(data), len, SendDataHex, len * 2 + 1);

	//如果采集的数据可能存在转义字符等，所以上面将其转为十六进制形式的字符串，并且字符串形式发送，需要服务端配合重新将收到的十六进制形式的字符串转为普通字符
	snprintf(at_http_content, 50 + len * 2, "AT+HTTPCONTENT=0,%s,0\r\n", (char *)SendDataHex);
	
	///配置内容数据，把数据填充到相应的AT命令中
	at_ret = AT_Send_And_Get_Rsp(at_http_content, AT_HTTP_LOCALRSP_TIMEOUT, NULL, NULL);

	xy_free(SendDataHex);
	xy_free(at_http_content);
	
	if(at_ret != XY_OK)
		goto exit;

	//发送请求，上报采集数据，其中的 path 根据实际使用HTTP服务器进行更改；
	//示例中通过检查HTTP RESPONSE中 responseCode 进行检测请求是否被正确处理， HTTP/1.1 200 OK 表示服务器已经正确处理该请求，
	//或者服务器也可返回特殊字段用于判断，但是该字段必须处于回复报文的结尾；
	at_ret = Send_AT_Req("AT+HTTPSEND=0,1,\"/users/h\"\r\n", AT_HTTP_RESPONSE_TIMEOUT);

exit:
	return at_ret;
}

At_status_type Send_Data_By_Http_Asyc(void *data,int len,int *send_ret)
{
	At_status_type  at_ret = -1;

	switch (g_http_at_step)
	{
		case AT_HTTP_INIT:
		{
			//创建HTTP实例
			at_ret = AT_Send_And_Get_Rsp(HTTP_NET_CONFIG, AT_HTTP_LOCALRSP_TIMEOUT, "+HTTPCREATE:0\r\n", NULL, NULL);
			if(at_ret != XY_OK)
				break;
			
			g_http_at_step = AT_HTTP_READY;
			break;
		}
		//能够进行数据发送 ready 态
		case AT_HTTP_READY:
		{
			if(is_event_set(EVENT_CLOUD_SEND) && data != NULL)
			{
				at_ret = AT_Send_by_Http(data, len);
					
				g_http_at_step = AT_WAIT_SEND_RSP;
				break;	
			}
			
			//若没有事可做，退回初始态
			if(!is_event_set(EVENT_CLOUD_SEND))
			{
				//关闭基础通信套件实例
				at_ret = AT_Send_And_Get_Rsp("AT+HTTPCLOSE=0\r\n", AT_HTTP_LOCALRSP_TIMEOUT, NULL, NULL);
				if(at_ret != XY_OK)
					break;

				g_http_at_step = AT_HTTP_INIT;
			}
			break;
		}
		case AT_WAIT_SEND_RSP:
		{
			int responseCode = -1;
				
			at_ret = Get_AT_Rsp("+HTTPNMIH:0,","%d", &responseCode);
			if(at_ret != XY_OK)
				break;

			if(responseCode == 200)
			{
				*send_ret = 0;
				g_http_at_step = AT_HTTP_READY;
			}
			//若发送失败，判为网路异常，建议客户容错，例如深睡一段时间后再发送
			else
			{
				xy_printf("send falied!\n");
				*send_ret = 1;
				g_http_at_step = AT_HTTP_INIT;
			}
			break;
		}
		default:
			break;
	}
	return at_ret;
}
#endif
