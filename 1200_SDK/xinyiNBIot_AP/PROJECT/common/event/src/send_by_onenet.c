#include "basic_config.h"
#include "user_config.h"

/**************************************************************************************************************************************************************
 * @brief 对于OPENCPU产品形态，不建议CP核执行云的保存和自动UPDATE，以防止私自唤醒对AP核产生干扰，建议出厂NV参数save_cloud、CLOUDALIVE、set_CP_rtc三者皆设为0；close_drx_dsleep设为1
 *************************************************************************************************************************************************************/


#if CLOUDTYPE==1
/**
* @file       AT_Send_by_Onenet.c
* @brief      该源文件执行的是通过onenet的标准AT命令进行数据的发送,以及下行控制数据的接收处理。
*
* @attention  上产线时，务必关注相关NV参数是否设置正确：set_tau_rtc=0；save_cloud=1；keep_cloud_alive=0；open_log=0；off_debug=1；
* @attention  使用该demo前，确保终端已经设置了imei号，且该imei号在云平台上已成功注册！
* @warning    在开卡时设置TAU(T3412)时长大于业务通信周期，以节省功耗和流量！！！
* @warning    禁止客户需要该源文件，若有特别需求，请联系芯翼研发，否则不做支持！
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "xy_printf.h"
#include "user_config.h"
#include "data_gather.h"
#include "cloud_process.h"
#include "at_process.h"
#include "err_process.h"

//ONENET 超时重传耗时124s，所以超时设为128s
#define AT_ONENET_RESPONSE_TIMEOUT  128
#define AT_ONNET_LOCALRSP_TIMEOUT   10
#define RESULT_204_CHANGED 			2
#define RESULT_400_BADREQUEST 		11
#define RESULT_401_UNAUTHORIZED 	12
#define RESULT_404_NOTFOUND 		13
#define RESULT_405_NOTALLOWED 		14
#define EVENT_NOTIFY_SUCCESS		6

//用户可设置——默认使用引导模式接入onenet平台
#if VER_BC95
#define ONENET_NET_CONFIG  			"AT+MIPLCONFIG=1,183.230.40.39,5683\r\n"
#else
#define ONENET_CREATE_BY_CONFHEX    "AT+MIPLCREATE=69,130045f10003f20037040011800005434d494f540000000000123138332e3233302e34302e33393a35363833000f41757468436f64653a3b50534b3a3bf30008e400c80000,0,69,0\r\n"
#endif


typedef struct
{
	int msgId;
	int objId;
	int insId;
	int resId;
}onenet_msg_t;

int g_onenet_at_step = 0;

//以object 3311为例,处理读数据;在URC_Regit_Init中注册添加即可
void URC_MIPLREAD_Proc(char *paramlist)
{
	onenet_msg_t msg = {0};
	At_status_type at_ret = XY_OK;

	at_parse_param(",%d,%d,%d,%d", paramlist, &msg.msgId, &msg.objId, &msg.insId, &msg.resId);
	xy_printf("[URC_MIPLREAD]msgId:%d, objId:%d, insId:%d, resId:%d \r\n",msg.msgId, msg.objId, msg.insId, msg.resId);
	char *at_response = xy_malloc(100);

	//用户根据实际情况填写上报值
	switch(msg.resId) {
	case 5850:	//bool 上报值为1
		snprintf(at_response, 100, "AT+MIPLREADRSP=0,%d,1,%d,%d,%d,5,1,1,0,0\r\n", msg.msgId, msg.objId, msg.insId, msg.resId);
		at_ret = AT_Send_And_Get_Rsp(at_response,AT_RESPONSE_TIMEOUT,  NULL, NULL);
		break;
	case 5851:	//int 上报值为3611
		snprintf(at_response, 100, "AT+MIPLREADRSP=0,%d,1,%d,%d,%d,3,2,3611,0,0\r\n", msg.msgId, msg.objId, msg.insId, msg.resId);
		at_ret = AT_Send_And_Get_Rsp(at_response, AT_RESPONSE_TIMEOUT,NULL,  NULL);
		break;
	case 5706:	//string 上报值为abcde12345
		snprintf(at_response, 100, "AT+MIPLREADRSP=0,%d,1,%d,%d,%d,1,10,abcde12345,0,0\r\n", msg.msgId, msg.objId, msg.insId, msg.resId);
		at_ret = AT_Send_And_Get_Rsp(at_response, AT_RESPONSE_TIMEOUT,NULL, NULL);
		break;
	case 5805:	//float 上报值为12.256
		snprintf(at_response, 100, "AT+MIPLREADRSP=0,%d,1,%d,%d,%d,4,4,12.256,0,0\r\n", msg.msgId, msg.objId, msg.insId, msg.resId);
		at_ret = AT_Send_And_Get_Rsp(at_response,AT_RESPONSE_TIMEOUT, NULL, NULL);
		break;
	default:
		xy_printf("resId wrong!\r\n");
		break;
	}

	if(at_ret != XY_OK)
	{
		xy_printf("AT+MIPLREADRSP failed!\r\n");
	}
	xy_free(at_response);
	return;
}

/* onenet的下行"写"数据，可以在此解析控制配置参数，并更新到g_Control_Context->user_config中,同时可能需要通过I2C等操控外设 */
void URC_MIPLWRITE_Proc(char *paramlist)
{
    //解析+MIPLWRITE: <ref>,<msgid>,<objectid>,<instanceid>,<resourceid>,<valuetype>,<len>,<value>,<flag>,<index>
    int msgId = -1, objId = -1, insId = -1, resId = -1, len = -1;
    char *str = NULL;
    At_status_type at_ret = XY_OK;
    char *at_response = xy_malloc(36);

    at_parse_param(",%d,%d,%d,%d,%p,%d", paramlist, &msgId, &objId, &insId, &resId, &str, &len);
    xy_printf("[URC_MIPWRITE]msgId:%d, objId:%d, insId:%d, resId:%d \r\n",msgId, objId, insId, resId);

    //value  目前平台下行数据只支持不透明型,可根据此下发数据更改用户配置项
    xy_printf("[URC_MIPWRITE]opaque:%s\r\n",str);

    //发送回复写入成功到平台AT+MIPLWRITERSP=<ref>,<msgid>,<result>[,<raimode>]
    snprintf(at_response, 36, "AT+MIPLWRITERSP=0,%d,2\r\n", msgId);
    at_ret = AT_Send_And_Get_Rsp(at_response,AT_RESPONSE_TIMEOUT,  NULL, NULL);

    if(at_ret != XY_OK)
	{
        xy_printf("[URC_MIPWRITE]AT+MIPLWRITERSP failed!\r\n");
	}
    xy_free(at_response);

    //Save_User_Config();
}

/* onenet的下行"执行"数据，可以在此解析控制配置参数，并更新到g_Control_Context->user_config中,同时可能需要通过I2C等操控外设 */
void URC_MIPLEXECUTE_Proc(char *paramlist)
{
    //解析+MIPLEXECUTE: <ref>,<msgid>,<objectid>,<instanceid>,<resourceid>[,<len>,<arguments>]
    int msgId = -1, objId = -1, insId = -1, resId = -1, len = -1;
    char *str = NULL;
    At_status_type at_ret = XY_OK;
    char *at_response = xy_malloc(36);

    at_parse_param(",%d,%d,%d,%d,%d,%p", paramlist, &msgId, &objId, &insId, &resId, &len, &str);
    xy_printf("[URC_MIPEXECUTE]msgId:%d, objId:%d, insId:%d, resId:%d \r\n",msgId, objId, insId, resId);

    if(len > 0)
    {
        //value  可根据此下发数据更改用户配置项
        xy_printf("[URC_MIPEXECUTE]arguments:%s\r\n",str);
    }
    //发送回复执行成功到平台AT+MIPLEXECUTERSP=<ref>,<msgid>,<result>[,<raimode>]
    snprintf(at_response, 36, "AT+MIPLEXECUTERSP=0,%d,2\r\n", msgId);
    at_ret = AT_Send_And_Get_Rsp(at_response, AT_RESPONSE_TIMEOUT, NULL, NULL);

    if(at_ret != XY_OK)
	{
        xy_printf("[URC_MIPEXECUTE]AT+MIPLEXECUTERSP failed!\r\n");
	}
    xy_free(at_response);

    //Save_User_Config();
}

/* onenet的下行"参数控制"数据，可以在此解析控制配置参数，并更新到g_Control_Context->user_config中,同时可能需要通过I2C等操控外设 */
void URC_MIPLPARAMETER_Proc(char *paramlist)
{
    //解析+MIPLPARAMETER: <ref>,<msgid>,<objectid>,<instanceid>,<resourceid>,<len>,<parameter>
    int msgId = -1, objId = -1, insId = -1, resId = -1, len = -1;
    char *str = NULL;
    At_status_type at_ret = XY_OK;
    char *at_response = xy_malloc(36);

    at_parse_param(",%d,%d,%d,%d,%d,%p", paramlist, &msgId, &objId, &insId, &resId, &len, &str);
    xy_printf("[URC_MIPLPARAMETER]msgId:%d, objId:%d, insId:%d, resId:%d \r\n",msgId, objId, insId, resId);
    xy_printf("[URC_MIPLPARAMETER]parameter:%s\r\n",str);

    //发送回复AT+MIPLPARAMETERRSP=<ref>,<msgid>,<result>[,<raimode>]
    snprintf(at_response, 36, "AT+MIPLPARAMETERRSP=0,%d,2\r\n", msgId);
    at_ret = AT_Send_And_Get_Rsp(at_response, AT_RESPONSE_TIMEOUT, NULL, NULL);
	
    if(at_ret != XY_OK)
	{
        xy_printf("[URC_MIPLPARAMETER]AT+MIPLPARAMETERRSP failed!\r\n");
	}
    xy_free(at_response);

    //Save_User_Config();
}

typedef enum
{
	AT_CLOUD_INIT = 0,
	AT_GET_RMLIFETIME = AT_CLOUD_INIT,
	AT_WAIT_OPEN,
#if !VER_BC95
	AT_WAIT_DISCOVER,
#endif
	AT_WAIT_UPDATE_RSP,
	AT_SEND_DATA,
	AT_WAIT_SEND_RSP,
	AT_CLOUD_READY,   //表示处于待发送数据态，若内部检查无数据发送，则会退出大循环
}At_ONENET_STATE;


//获取待发送的数据，组装为对应的AT命令发送出去;若数据尚未准备好，则继续等待
At_status_type AT_Send_by_Onenet(void *SendDataAddr,uint32_t SendDataLen)
{
	At_status_type	at_ret = XY_OK;
	char *SendDataHex = xy_malloc(SendDataLen * 2 + 1);
	char *at_cmd = xy_malloc(75 + SendDataLen * 2 + 1);
	memset(SendDataHex, 0, SendDataLen * 2 + 1);
	memset(at_cmd, 0, 75 + SendDataLen * 2 + 1);

	//将读取到的所有数据转换成十六进制形式的字符串
	bytes2hexstr((uint8_t *)(SendDataAddr), SendDataLen, SendDataHex, (SendDataLen * 2 + 1));

	//把数据填充到相应的AT发送命令中          修改时需注意：对象实例属性等信息应与添加对象命令匹配，且带ackid才会有发送结果上报
	snprintf(at_cmd, 75 + (SendDataLen) * 2, "AT+MIPLNOTIFY=0,0,3311,0,5706,2,%ld,%s,0,0,123\r\n", (SendDataLen), (char *)SendDataHex);
	xy_free(SendDataHex);
	//发送AT命令并等待发送结果
	at_ret = Send_AT_Req(at_cmd, AT_ONENET_RESPONSE_TIMEOUT);

 	xy_free(at_cmd);

	return at_ret;
}


At_status_type Send_Data_By_Onenet_Asyc(void *data,int len,int *send_ret)
{
	At_status_type  at_ret = -1;

	switch (g_onenet_at_step)
	{
		case AT_GET_RMLIFETIME:
		{
		    xy_printf("Send_Data_By_Cloud Start time=%d\n",(int)(Get_Tick()/1000));
		    int remain_lifetime = -1;
			//查询onenet lifetime剩余时间
		    at_ret = AT_Send_And_Get_Rsp("AT+ONETRMLFT\r\n", AT_ONNET_LOCALRSP_TIMEOUT, "+ONETRMLFT:", "%d", &remain_lifetime);

			//无剩余lifetime，需重新注册
            if(remain_lifetime < 0)
            {
#if VER_BC95
                //配置observe自动应答
                at_ret = AT_Send_And_Get_Rsp("AT+MIPLCONFIG=3,1\r\n", AT_ONNET_LOCALRSP_TIMEOUT, NULL, NULL);
                if(at_ret != XY_OK)
                    break;

                //设置服务器IP和port
                at_ret = AT_Send_And_Get_Rsp(ONENET_NET_CONFIG, AT_ONNET_LOCALRSP_TIMEOUT, NULL, NULL);
                if(at_ret != XY_OK)
                    break;

                //创建onenet 通讯套件
                at_ret = AT_Send_And_Get_Rsp("AT+MIPLCREATE\r\n", AT_ONNET_LOCALRSP_TIMEOUT, NULL, NULL);
                if(at_ret != XY_OK)
                    break;
#else
                //创建onenet通讯套件
                at_ret = AT_Send_And_Get_Rsp(ONENET_CREATE_BY_CONFHEX, AT_ONNET_LOCALRSP_TIMEOUT, NULL, NULL);
                if(at_ret != XY_OK)
                    break;
#endif

                //添加object  按用户实际使用的object修改，注意属性个数应大于0
                at_ret = AT_Send_And_Get_Rsp("AT+MIPLADDOBJ=0,3311,1,\"1\",4,3\r\n", AT_ONNET_LOCALRSP_TIMEOUT, NULL, NULL);
                if(at_ret != XY_OK)
                    break;

                //开始向平台注册
                char at_miplopen[40] = {0};
                snprintf(at_miplopen, 40, "AT+MIPLOPEN=0,%d,%d\r\n", CLOUD_LIFETIME, AT_ONENET_RESPONSE_TIMEOUT);
                at_ret = Send_AT_Req(at_miplopen, AT_ONENET_RESPONSE_TIMEOUT);
                if(at_ret != XY_OK)
                    break;

                g_onenet_at_step = AT_WAIT_OPEN;
            }
            else
            {
                //剩余lifetime有效，进入可以发数据的ready态
                g_onenet_at_step = AT_CLOUD_READY;
            }
            break;
		}
#if VER_BC95
		case AT_WAIT_OPEN:
#else
        case AT_WAIT_OPEN:
        {
            char at_miplobserve_value[12] = {0};
            at_ret = Get_AT_Rsp("+MIPLOBSERVE:",",%s", at_miplobserve_value);
            if(at_ret != XY_OK)
                break;

            char *at_miplobserversp = xy_malloc(65);
            //获取套件对象属性 修改时需注意：属性个数应与添加object对象时的属性个数相等
            snprintf(at_miplobserversp, 65, "AT+MIPLOBSERVERSP=0,%s,1\r\n", at_miplobserve_value);
            at_ret = Send_AT_Req(at_miplobserversp, AT_ONENET_RESPONSE_TIMEOUT);
            xy_free(at_miplobserversp);
            if(at_ret != XY_OK)
                break;

            g_onenet_at_step = AT_WAIT_DISCOVER;
            break;
        }
        case AT_WAIT_DISCOVER:
#endif
		{
			char at_mipldiscover_value[10] = {0};
			at_ret = Get_AT_Rsp("+MIPLDISCOVER:",",%s", at_mipldiscover_value);
			if(at_ret != XY_OK)
				break;
			
			char *at_mipldiscoverrsp = xy_malloc(65);
			//获取套件对象属性 修改时需注意：属性个数应与添加object对象时的属性个数相等
			snprintf(at_mipldiscoverrsp, 65, "AT+MIPLDISCOVERRSP=0,%s,1,19,\"5850;5851;5706;5805\"\r\n", at_mipldiscover_value);
			at_ret = AT_Send_And_Get_Rsp(at_mipldiscoverrsp, AT_ONNET_LOCALRSP_TIMEOUT, NULL, NULL);
			xy_free(at_mipldiscoverrsp);
			if(at_ret != XY_OK)
				break;

			g_onenet_at_step = AT_CLOUD_READY;
			break;
		}
		//当前处在待发送数据态，若没有任何数据待发送，将会在此退出大循环
		case AT_CLOUD_READY:
		{
			if(is_event_set(EVENT_CLOUD_UPDATE))
			{
				//触发UPDATE
				char *at_miplupdate = xy_malloc(50);
				snprintf(at_miplupdate, 50, "AT+MIPLUPDATE=0,%d,0\r\n", CLOUD_LIFETIME);
				at_ret = Send_AT_Req(at_miplupdate, AT_ONENET_RESPONSE_TIMEOUT);
				xy_free(at_miplupdate);
				if(at_ret != XY_OK)
					break;
				
				g_onenet_at_step = AT_WAIT_UPDATE_RSP;
				break;
			}

			if(is_event_set(EVENT_CLOUD_SEND) && data != NULL)
			{
				at_ret = AT_Send_by_Onenet(data,len);
				if(at_ret != XY_OK)
					break;

				g_onenet_at_step = AT_WAIT_SEND_RSP;
			}

			break;
		}

		case AT_WAIT_UPDATE_RSP:
		{
#if VER_BC95
			at_ret = Get_AT_Rsp("+MIPLEVENT: 0,11\r\n" , NULL);
#else
			at_ret = Get_AT_Rsp("+MIPLEVENT:0,11\r\n" , NULL);
#endif
			if(at_ret != XY_OK)
				break;

			if(is_event_set(EVENT_CLOUD_UPDATE))
			{
				clear_event(EVENT_CLOUD_UPDATE);
				xy_printf("update success!\n");
			}
			//进入待发送数据态，以检查是否需要发送数据
			g_onenet_at_step = AT_CLOUD_READY;

			break;
		}
		case AT_WAIT_SEND_RSP:
		{
			int result = -1;
#if VER_BC95
			at_ret = Get_AT_Rsp("\r\n+MIPLEVENT: 0,2", "%d", &result);
#else
			at_ret = Get_AT_Rsp("\r\n+MIPLEVENT:0,2", "%d", &result);
#endif
			if(at_ret != XY_OK)
				break;

			//如果发送成功则继续下一次发送
			if(result == EVENT_NOTIFY_SUCCESS)
			{
				*send_ret = 0;
				g_onenet_at_step = AT_CLOUD_READY;
			}
			//若发送失败，判为网路异常，建议客户容错，例如深睡一段时间后再发送
			else
			{
				*send_ret = 1;
				xy_printf("send falied! result[%d]\n", result);
				g_onenet_at_step = AT_CLOUD_INIT;
			}
			break;
		}
		default:
			break;
	}

	return at_ret;
}

#endif
