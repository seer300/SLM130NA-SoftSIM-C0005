#include "basic_config.h"
#include "user_config.h"

/**************************************************************************************************************************************************************
 * @brief 对于OPENCPU产品形态，不建议CP核执行云的保存和自动UPDATE，以防止私自唤醒对AP核产生干扰，建议出厂NV参数save_cloud、CLOUDALIVE、set_CP_rtc三者皆设为0；close_drx_dsleep设为1
 *************************************************************************************************************************************************************/


#if CLOUDTYPE==5
/**
* @file       Send_By_Ctwing.c
* @brief      该源文件执行的是通过ctlw的标准AT命令进行数据的发送,以及下行控制数据的接收处理。
*
* @attention  上产线时，务必关注相关NV参数是否设置正确：set_tau_rtc=0；save_cloud=1；keep_cloud_alive=0；open_log=0；off_debug=1；
* @attention  使用该demo前，确保终端已经设置了imei号，且该imei号在云平台上已成功注册！
* @warning    在开卡时设置TAU(T3412)时长大于业务通信周期，以节省功耗和流量！！！
* @warning    禁止客户需要该源文件，若有特别需求，请联系芯翼研发，否则不做支持！
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xy_printf.h"
#include "at_process.h"
#include "data_gather.h"
#include "cloud_process.h"
#include "xy_cp.h"
#include "err_process.h"


//CTWING 超时重传耗时155s，所以超时设为159s
#define AT_CTLW_RESPONSE_TIMEOUT 159

//CTWING 查询类AT超时时间10s
#define  SYNC_RESPONSE_TIMEOUT    10

int g_ctlw_at_step = 0;


typedef enum
{
	AT_CLOUD_INIT = 0,
	AT_SEND_CTLWGETSTATUS = AT_CLOUD_INIT,
	AT_WAIT_CTLWREG,
	AT_WAIT_UPDATE_RSP,
	AT_WAIT_SEND_RSP,
	AT_CLOUD_READY,   //表示可以开始远程数据通信
}At_CTLW_STATE;


/* CTWING的下行数据，可以在此解析控制配置参数，并更新到g_Control_Context->user_config中,同时可能需要通过I2C等操控外设 */
__WEAK void URC_CTLWRECV_Proc(char *paramlist)
{
	//解析CTLWRECV:<mode>,<recv_data>示例，解析出来的recv_data用户自己处理;
	int mode = -1;
	char *recv_data = NULL;
	at_parse_param("%d,%p", paramlist, &mode, &recv_data);
	xy_printf("mode=%d,recv_data=%s\n",mode,recv_data);
	//Save_User_Config();
}

//获取待发送的数据，组装为对应的AT命令发送出去;若数据尚未准备好，则继续等待
At_status_type AT_Send_By_Ctlw(void *SendDataAddr,uint32_t SendDataLen)
{
	At_status_type  at_ret = XY_OK;
	char *SendDataHex = xy_malloc(SendDataLen * 2 + 1);
	char *at_cmd = xy_malloc(50 + SendDataLen * 2 + 1);
	memset(SendDataHex, 0, SendDataLen * 2 + 1);
	memset(at_cmd, 0, 50 + SendDataLen * 2 + 1);

	//将读取到的所有数据转换成十六进制形式的字符串
	bytes2hexstr((uint8_t *)(SendDataAddr), SendDataLen, SendDataHex, (SendDataLen * 2 + 1));

	//把数据填充到相应的AT命令中
	snprintf(at_cmd, 50 + (SendDataLen) * 2, "AT+CTLWSEND=%s,1\r\n", (char *)SendDataHex);
	xy_free(SendDataHex);
	//发送AT命令并等待发送结果
	at_ret = Send_AT_Req(at_cmd, AT_CTLW_RESPONSE_TIMEOUT);
	
	xy_free(at_cmd);

	return at_ret;
}

At_status_type Send_Data_By_Ctlw_Asyc(void *data,int len,int *send_ret)
{
	At_status_type  at_ret = -1;

	switch (g_ctlw_at_step)
	{
		case AT_SEND_CTLWGETSTATUS:
		{
			uint32_t session_state = -1;

			//第1步:查询ctwing当前注册状态
			at_ret = AT_Send_And_Get_Rsp("AT+CTLWGETSTATUS=0\r\n",SYNC_RESPONSE_TIMEOUT, "+CTLWGETSTATUS: 0,", "%d", &session_state);
			if(at_ret != XY_OK)
				break;
			
			if(session_state == 1)//云当前会话状态是未登录，发起注册
			{
				//第2步:设置电信物联网平台服务器地址
				at_ret = AT_Send_And_Get_Rsp("AT+CTLWSETSERVER=0,0,221.229.214.202\r\n",SYNC_RESPONSE_TIMEOUT, NULL, NULL);

				if(at_ret != XY_OK)
					break;
				
				//第3步:设置生命周期lifetime
				char at_ctlwsetlt[40] = {0};
				snprintf(at_ctlwsetlt, 40, "AT+CTLWSETLT=%d\r\n",CLOUD_LIFETIME);
				at_ret = AT_Send_And_Get_Rsp(at_ctlwsetlt, SYNC_RESPONSE_TIMEOUT, NULL, NULL);
				
				if(at_ret != XY_OK)
					break;
	
				//第4步:设置终端认证模式为IMEI
				at_ret = AT_Send_And_Get_Rsp("AT+CTLWSETMOD=3,2\r\n", SYNC_RESPONSE_TIMEOUT, NULL, NULL);
				
				if(at_ret != XY_OK)
					break;

				//第5步：向平台发起注册
				at_ret = Send_AT_Req("AT+CTLWREG\r\n", AT_CTLW_RESPONSE_TIMEOUT);
				
				if(at_ret != XY_OK)
					break;
			
				g_ctlw_at_step = AT_WAIT_CTLWREG;
				break;
			}
			else if(session_state == 4)//当前会话状态为已登录，跳转至ready态
			{
				g_ctlw_at_step = AT_CLOUD_READY;
			}
			break;
		}
		case AT_WAIT_CTLWREG:
		{
			at_ret = Get_AT_Rsp("+CTLW: lwevent,0,0\r\n", NULL);

			if(at_ret != XY_OK)
				break;
			//注册完成进入ready态
			g_ctlw_at_step = AT_CLOUD_READY;	
			break;
		}
		case AT_CLOUD_READY:
		{
			if(is_event_set(EVENT_CLOUD_UPDATE))
			{
				at_ret = Send_AT_Req("AT+CTLWUPDATE\r\n", AT_CTLW_RESPONSE_TIMEOUT);
				
				if(at_ret != XY_OK)
					break;
				
				g_ctlw_at_step = AT_WAIT_UPDATE_RSP;
				break;
			}

			if(is_event_set(EVENT_CLOUD_SEND) && data != NULL)
			{
				at_ret = AT_Send_By_Ctlw(data,len);
				
				if(at_ret != XY_OK)
					break;

				g_ctlw_at_step = AT_WAIT_SEND_RSP;					
			}

			break;
		}

		case AT_WAIT_UPDATE_RSP:
		{
			at_ret = Get_AT_Rsp("+CTLW: update,0,", NULL);

			if(at_ret != XY_OK)
				break;

			if(is_event_set(EVENT_CLOUD_UPDATE))
			{
				clear_event(EVENT_CLOUD_UPDATE);
				xy_printf("update success!\n");
			}
			g_ctlw_at_step = AT_CLOUD_READY;
			break;
		}

		case AT_WAIT_SEND_RSP:
		{
			int send_result = -1;
			int msgid = -1;
			at_ret = Get_AT_Rsp("+CTLW: send,","%d,%d", &send_result, &msgid);

			if(at_ret != XY_OK)
				break;
				
			if(send_result == 0 && msgid > 0)
			{
				*send_ret = 0;
				g_ctlw_at_step = AT_CLOUD_READY;
			}
			else	//若发送失败，判为网路异常，建议客户容错，例如深睡一段时间后再发送
			{
				*send_ret = 1;
				xy_printf("send falied!\n");
				g_ctlw_at_step = AT_CLOUD_INIT;
			}
			break;
		}
		default:
			break;
	}

	return at_ret;
}

#endif