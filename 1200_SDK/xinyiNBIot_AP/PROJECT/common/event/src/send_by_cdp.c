
#include "basic_config.h"
#include "user_config.h"

/**************************************************************************************************************************************************************
 * @brief 对于OPENCPU产品形态，不建议CP核执行云的保存和自动UPDATE，以防止私自唤醒对AP核产生干扰，建议出厂NV参数save_cloud、CLOUDALIVE、set_CP_rtc三者皆设为0；close_drx_dsleep设为1
 *************************************************************************************************************************************************************/


#if CLOUDTYPE==0
/**
* @file        AT_Send_By_Cdp.c
* @brief      该源文件执行的是通过cdp的标准AT命令进行数据的发送,以及下行控制数据的接收处理。
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
#include "user_config.h"


//CDP 超时重传耗时92s，所以超时设为96s
#define AT_CDP_RESPONSE_TIMEOUT 96

int g_cdp_at_step = 0;

/* CDP的下行数据，可以在此解析控制配置参数，并更新到g_Control_Context->user_config中,同时可能需要通过I2C等操控外设 */
__WEAK  void URC_NNMI_Proc(char *paramlist)
{
    //解析NNMI:<data_len>,<recv_data>示例，解析出来的recv_data用户自己处理;
    int data_len = -1;
    char *recv_data = NULL;
    at_parse_param("%d,%p", paramlist, &data_len, &recv_data);
    xy_printf("datalen=%d,recv_data=%s\n",data_len,recv_data);
    //Save_User_Config();
}

typedef enum
{
	AT_CLOUD_INIT = 0,
	AT_REQ_QLWEVTIND = AT_CLOUD_INIT,
	AT_WAIT_CDPUPDATE,
	AT_WAIT_QLWSREGIND,
	AT_WAIT_UPDATE_RSP,
	AT_WAIT_SEND_RSP,
	AT_CLOUD_READY,   //表示可以开始远程数据通信
}At_CDP_STATE;


//获取待发送的数据，组装为对应的AT命令发送出去;若数据尚未准备好，则继续等待
At_status_type AT_Send_By_Cdp(void *SendDataAddr,uint32_t SendDataLen)
{
	At_status_type  at_ret = XY_OK;
	char *SendDataHex = xy_malloc(SendDataLen * 2 + 1);
	char *at_cmd = xy_malloc(50 + SendDataLen * 2 + 1);
	
	memset(SendDataHex, 0, SendDataLen * 2 + 1);
	memset(at_cmd, 0, 50 + SendDataLen * 2 + 1);


	//将读取到的所有数据转换成十六进制形式的字符串
	bytes2hexstr((uint8_t *)(SendDataAddr), SendDataLen, SendDataHex, (SendDataLen * 2 + 1));

	//把数据填充到相应的AT命令中
#if VER_BC95
	snprintf(at_cmd, 50 + (SendDataLen) * 2, "AT+QLWULDATAEX=%ld,%s,0x0100\r\n", (SendDataLen), (char *)SendDataHex);
#else 
	snprintf(at_cmd, 50 + (SendDataLen) * 2, "AT+NMGS=%ld,%s\r\n", (SendDataLen), (char *)SendDataHex);
#endif
	xy_free(SendDataHex);
	//发送AT命令并等待发送结果
	at_ret = Send_AT_Req(at_cmd, AT_CDP_RESPONSE_TIMEOUT);
	
	xy_free(at_cmd);
	
	return at_ret;
}

At_status_type Send_Data_By_Cdp_Asyc(void *data,int len,int *send_ret)
{
	At_status_type  at_ret = -1;
	
	switch (g_cdp_at_step)
	{
		case AT_REQ_QLWEVTIND:
		{
			//第1步：查询cdp当前注册状态
#if VER_BC95
			int event_id = -1;
		    at_ret = AT_Send_And_Get_Rsp("AT+QLWEVTIND?\r\n",10, "+QLWEVTIND:1,", "%d", &event_id);
#else
			char at_nmstatus_str[20] = {0};
			at_ret = AT_Send_And_Get_Rsp("AT+NMSTATUS?\r\n",10, "+NMSTATUS:", "%s", at_nmstatus_str);
#endif
			if(at_ret != XY_OK)
				break;

			//若当前未注册状态，重新注册
#if VER_BC95
			if((event_id != 2) && (event_id != 3) && (event_id != 5))
#else
			if(strcmp(at_nmstatus_str, "REGISTERED"))
#endif
			{
				//第2步：设置服务器
				at_ret = AT_Send_And_Get_Rsp("AT+NCDP=221.229.214.202\r\n",10, NULL, NULL);
				if(at_ret != XY_OK)
					break;
				
				//第3步：设置lifetime，时长需要大于数据上报周期
				char at_qcfg[40] = {0};
				snprintf(at_qcfg, 40, "AT+QCFG=\"LWM2M/lifetime\",%d\r\n",CLOUD_LIFETIME);
				at_ret = AT_Send_And_Get_Rsp(at_qcfg, 10, NULL, NULL);
				if(at_ret != XY_OK)
					break;
#if !VER_BC95
				//自研指令需要开启发送成功的主动上报
				at_ret = AT_Send_And_Get_Rsp("AT+NSMI=1\r\n", 10, NULL, NULL);
				if(at_ret != XY_OK)
					break;
#endif
				at_ret = Send_AT_Req("AT+QLWSREGIND=0\r\n", AT_CDP_RESPONSE_TIMEOUT);
				if(at_ret != XY_OK)
					break;

				g_cdp_at_step = AT_WAIT_QLWSREGIND;
			}
			else
			{
				g_cdp_at_step = AT_CLOUD_READY;
			}
			break;
		}
		case AT_WAIT_QLWSREGIND:
		{
			at_ret = Get_AT_Rsp("+QLWEVTIND:3\r\n", NULL);

			if(at_ret != XY_OK)
				break;

			g_cdp_at_step = AT_CLOUD_READY;	//注册完成进入ready态
			break;
		}
		//能够进行数据发送ready态
		case AT_CLOUD_READY:
		{
			if(is_event_set(EVENT_CLOUD_UPDATE))
			{
				at_ret = Send_AT_Req("AT+CDPUPDATE\r\n", AT_CDP_RESPONSE_TIMEOUT);

				if(at_ret != XY_OK)
					break;
				
				g_cdp_at_step = AT_WAIT_UPDATE_RSP;
				break;
			}

			if(is_event_set(EVENT_CLOUD_SEND) && data != NULL)
			{
				at_ret = AT_Send_By_Cdp(data,len);
				
				if(at_ret != XY_OK)
					break;
				
				g_cdp_at_step = AT_WAIT_SEND_RSP;
			}

			break;
		}

		case AT_WAIT_UPDATE_RSP:
		{
			at_ret = Get_AT_Rsp("+QLWEVTIND:2\r\n", NULL);

			if(at_ret != XY_OK)
				break;

			if(is_event_set(EVENT_CLOUD_UPDATE))
			{
				clear_event(EVENT_CLOUD_UPDATE);
				xy_printf("update success!\n");
			}
			g_cdp_at_step = AT_CLOUD_READY;
			break;
		}

		case AT_WAIT_SEND_RSP:
		{
#if VER_BC95
			int send_result = -1;
			at_ret = Get_AT_Rsp("+QLWULDATASTATUS:", "%d", &send_result);
#else
			char send_result[16] = {0};
			at_ret = Get_AT_Rsp("+NSMI:", "%s", send_result);
#endif
			if(at_ret != XY_OK)
				break;

			//如果发送成功则继续下一次发送
#if VER_BC95
			if(send_result == 4)
#else
			if(!strcmp(send_result, "SENT"))
#endif
			{
				
				g_cdp_at_step = AT_CLOUD_READY;
				*send_ret = 0;
			}
			else //若发送失败，判为网路异常，建议客户容错，例如深睡一段时间后再发送
			{
				xy_printf("send falied!\n");
				g_cdp_at_step = AT_CLOUD_INIT;
				*send_ret = 1;
			}
			break;
		}
		default:
			break;
	}
	
	return  at_ret;
}

#endif
