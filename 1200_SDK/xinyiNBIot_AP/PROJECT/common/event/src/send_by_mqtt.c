#include "basic_config.h"
#include "user_config.h"
#if CLOUDTYPE==3
/**
* @file       send_by_mqtt.c
* @brief      该源文件执行的是通过mqtt的标准AT命令进行数据的发送。\n
* @attention  使用该demo前，确保已在云平台上已成功注册设备！\n
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


//mqtt服务器ip，端口号配置，用户可设
#define MQTT_NET_CONFIG       "AT+QMTOPEN=0,mqtt.ctwing.cn,1883\r\n"

//mqtt服务器认证信息设置，采用特征串认证，用户可设
#define MQTT_AUTHINFO_CONFIG  "AT+QMTCONN=0,15091954MQTT_DEVICES,mqtt_test,YyQtmZales4HSxtf2v6s9ouEGADGbQOGxbH0bAywXUM\r\n"

//mqtt测试主题设置，用户可设
#define MQTT_TEST_TOPIC       "data_report"

#define AT_MQTT_LOCALRSP_TIMEOUT   10
#define AT_MQTT_RESPONSE_TIMEOUT   60

int g_mqtt_at_step = 0;

//+QMTSTAT:<TCP_connectID>,<err_code>
//在 URC_Regit_Init 中注册添加即可
//用户根据该主动上报自行处理数据，示例中仅打印结果
__WEAK void URC_QMTSTAT_Proc(char *paramlist)
{
	int mqtt_id = -1;
	int err_code = -1;
    at_parse_param("%d,%d", paramlist, &mqtt_id, &err_code);

    xy_printf("mqtt id[%d] current state:%d\n", mqtt_id, err_code);
}


//+QMTRECV: <TCP_connectID>,<msgID>,<topic>,<payload>
//在 URC_Regit_Init 中注册添加即可
//用户根据该主动上报自行处理数据，示例中仅打印结果，用户可以根据实际使用在该接口内部调用 Send_Rai() 快速释放 RAI，但是需要确保之后无下行报文
__WEAK void URC_QMTRECV_Proc(char *paramlist)
{
	char *topic = NULL;
	char *payload = NULL;

	topic = xy_malloc(strlen(paramlist)+ 1);
	payload = xy_malloc(strlen(paramlist)+ 1);
	at_parse_param(",,%s,%s", paramlist, topic, payload);
	xy_printf("recv downdata, topic:%s, data:%s\n", topic, payload);
	xy_free(topic);
	xy_free(payload);
}


typedef enum
{
	AT_CLOUD_INIT = 0,
	AT_MQTT_INIT = AT_CLOUD_INIT,
	AT_WAIT_QMTOPEN,
	AT_WAIT_QMTCONN,
	AT_MQTT_READY,   //表示可以开始远程数据通信
	AT_WAIT_PUB_INPUT_IND, //透传数据输入指示符
	AT_WAIT_PUB_RSP	
}At_MQTT_STATE;


At_status_type Send_Data_By_Mqtt_Asyc(void *data,int len,int *send_ret)
{
	At_status_type  at_ret = -1;
	
	switch (g_mqtt_at_step)
	{
		case AT_MQTT_INIT:
		{
			//配置 keep_alive 时间（可选：用户可以在此进行其他的配置，否则采用默认的配置）
			at_ret = AT_Send_And_Get_Rsp("AT+QMTCFG=\"keepalive\",0,3600\r\n", AT_MQTT_LOCALRSP_TIMEOUT, NULL, NULL);
			if(at_ret != XY_OK)
				break;

			//进行tcp连接
			at_ret = Send_AT_Req(MQTT_NET_CONFIG, AT_MQTT_RESPONSE_TIMEOUT);
			if(at_ret != XY_OK)
				break;
			
			g_mqtt_at_step = AT_WAIT_QMTOPEN;
			break;
		}
		case AT_WAIT_QMTOPEN:
		{
			int result = -1;
			
			at_ret = Get_AT_Rsp("+QMTOPEN: 0,", "%d", &result);
			if(at_ret != XY_OK)
				break;
			
			if(result == 0)
			{
				at_ret = Send_AT_Req(MQTT_AUTHINFO_CONFIG, AT_MQTT_RESPONSE_TIMEOUT);
				if(at_ret != XY_OK)
					break;
				
				g_mqtt_at_step = AT_WAIT_QMTCONN;
			}
			else
			{
				//TCP 连接失败，判为网路异常，建议客户容错
				at_ret = XY_ERR_CONN_NOT_CONNECTED;
				xy_printf("open falied!\n");
				g_mqtt_at_step = AT_MQTT_INIT;	
			}
			break;
		}
		case AT_WAIT_QMTCONN:
		{
			int result = -1;
			int retcode = -1;

			at_ret = Get_AT_Rsp("+QMTCONN: 0,", "%d,%d", &result, &retcode);
			if(at_ret != XY_OK)
				break;

			if((result == 0) && (retcode == 0))
			{
				g_mqtt_at_step = AT_MQTT_READY;
			}
			else
			{
				//MQTT 连接失败，建议客户容错
				at_ret = XY_ERR_CONN_NOT_CONNECTED;
				xy_printf("connect falied!\n");
				g_mqtt_at_step = AT_MQTT_INIT;	
			}
			break;
		}
		//能够进行数据发送 ready 态
		case AT_MQTT_READY:
		{
			if(is_event_set(EVENT_CLOUD_SEND) && data != NULL)
			{
				int topic_len = strlen(MQTT_TEST_TOPIC);
				char *at_mqtt_pub = xy_malloc(48 + topic_len);
			  
				//测试采用 QOS 等级为 1，所以 msgId 必须大于0
				snprintf(at_mqtt_pub, 48 + topic_len, "AT+QMTPUB=0,1,1,0,%s,%ld\r\n", MQTT_TEST_TOPIC, len);
			
				//发送AT命令并等待透传数据输入指示符 “>”
				at_ret = Send_AT_Req(at_mqtt_pub, AT_MQTT_RESPONSE_TIMEOUT);
				xy_free(at_mqtt_pub);

				if(at_ret != XY_OK)
					break;

				g_mqtt_at_step = AT_WAIT_PUB_INPUT_IND;		
				break;	
			}
			
			//若没有事可做，退回初始态
			if(!is_event_set(EVENT_CLOUD_SEND))
			{
				//关闭MQTT通信
				at_ret = AT_Send_And_Get_Rsp("AT+QMTDISC=0\r\n", AT_MQTT_LOCALRSP_TIMEOUT, NULL, NULL);
				if(at_ret != XY_OK)
					break;
				
				g_mqtt_at_step = AT_MQTT_INIT;
			}
			break;
		}
		case AT_WAIT_PUB_INPUT_IND:
		{
			at_ret = Get_AT_Rsp("\r\n>", NULL);

			if(at_ret != XY_OK)
				break;

			//发送透传数据并等待发送结果
			Send_Passthr_Data_Req(data, len, AT_RESPONSE_TIMEOUT);
			g_mqtt_at_step = AT_WAIT_PUB_RSP;	
			break;
 		}
		case AT_WAIT_PUB_RSP:
		{
			int pub_result = -1;

			at_ret = Get_AT_Rsp("+QMTPUB: 0,1,", "%d", &pub_result);
			if(at_ret != XY_OK)
				break;

			//如果发送成功则继续下一次发送
			if(pub_result == 0)
			{
				*send_ret = 0;
				g_mqtt_at_step = AT_MQTT_READY;
			}
			else //若发送失败，判为网路异常，建议客户容错，例如深睡一段时间后再发送
			{
				*send_ret = 1;
				g_mqtt_at_step = AT_MQTT_INIT;
			}
			break;
		}
		default:
			break;
	}
	return at_ret;
}
#endif
