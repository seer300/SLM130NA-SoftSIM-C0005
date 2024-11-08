
#include "xy_system.h"
#include "ap_watchdog.h"
#include "xy_printf.h"
#include "hal_gpio.h"
#include "xy_ftl.h"
#include "urc_process.h"
#include "user_config.h"
#include "cloud_process.h"
#include "data_gather.h"
#include "xy_cp.h"
#include "xy_lpm.h"
#include "at_CP_api.h"
#include "err_process.h"
#include "xy_memmap.h"

/**************************************************************************************************************************************************************
 * @brief 对于OPENCPU产品形态，不建议CP核执行云的保存和自动UPDATE，以防止私自唤醒对AP核产生干扰，建议出厂NV参数save_cloud、CLOUDALIVE、set_CP_rtc三者皆设为0；close_drx_dsleep设为1
 *************************************************************************************************************************************************************/
 
/*识别远程通信事件后，取数据进行远程通信。若通信异常，需用户自行进行容错策略*/
At_status_type DO_Send_Data_By_Cloud(void)
{
	At_status_type at_ret = XY_OK;
	int send_ret = -1;  //0表示发送成功，其他正值表示发送失败

	/*用静态全局记录待发送的数据，容许多次调用该函数进行数据发送*/
	static void *SendData = NULL;
	static uint32_t SendLen = 0;
	

    if (CP_Is_Alive() == false)
	{
		Boot_CP(WAIT_CP_BOOT_MS);
	}


	/*进行PDP激活成功与否的查询，若未成功，则不能继续云操作*/
	if(g_tcpip_ok == 0)
	{
		//异步方式超时等待PDP激活成功
		at_ret = xy_wait_tcpip_ok_asyc(WAIT_CGATT_TIMEOUT);
		
		if(at_ret == XY_OK)	
		{
			g_tcpip_ok = 1;
		}
		else if(at_ret == XY_ATTACHING)	/* 正在attach，继续等待 */
		{
			return at_ret;
		}
		else/* 其他错误 */
		{
			User_Err_Process(at_ret);
			return at_ret;
		}
	}

	if(is_event_set(EVENT_CLOUD_SEND) && SendLen == 0)
	{
#if CLOUDTYPE==0
		if(Get_Data_To_Send(CDP_ONCE_SEND_LEN, &SendData, &SendLen) == 0)
#elif CLOUDTYPE==1
		if(Get_Data_To_Send(ONENET_ONCE_SEND_LEN, &SendData, &SendLen) == 0)
#elif CLOUDTYPE==2
		if(Get_Data_To_Send(SOCKET_SEND_MAX_LENGTH, &SendData, &SendLen) == 0)
#elif CLOUDTYPE==3
		if(Get_Data_To_Send(MQTT_ONCE_SEND_LEN, &SendData, &SendLen) == 0)
#elif CLOUDTYPE==4
		if(Get_Data_To_Send(HTTP_SEND_DATA_MAX, &SendData, &SendLen) == 0)
#elif CLOUDTYPE==5
		if(Get_Data_To_Send(CTLW_ONCE_SEND_LEN, &SendData, &SendLen) == 0)
#endif
		/*无待发送数据，清空事件后返回*/
		{
			clear_event(EVENT_CLOUD_SEND);

			xy_printf("Send_Data_By_Cloud Finish time=%d\n",(int)(Get_Tick()/1000));

#if ((CLOUDTYPE != 3) && (CLOUDTYPE != 4))
			if(!is_event_set(EVENT_CLOUD_UPDATE))
			{
				/*为了加速CP的下电深睡，用户根据产品的下行数据突发属性自行选择对应接口*/
				//Send_Rai();
				//Stop_CP(WAIT_CP_STOP_MS);	
                		
				//数据发送完成后，重设云状态机
				Reset_Cloud_State();
				return  XY_OK;
			}
#endif
		}
	}
	

	
#if CLOUDTYPE==0
	at_ret = Send_Data_By_Cdp_Asyc(SendData,SendLen,&send_ret);
#elif CLOUDTYPE==1
	at_ret = Send_Data_By_Onenet_Asyc(SendData,SendLen,&send_ret);
#elif CLOUDTYPE==2
	at_ret = Send_Data_By_Socket(SendData,SendLen,&send_ret);
#elif CLOUDTYPE==3
	at_ret = Send_Data_By_Mqtt_Asyc(SendData,SendLen,&send_ret);
#elif CLOUDTYPE==4
	at_ret = Send_Data_By_Http_Asyc(SendData,SendLen,&send_ret);
#elif CLOUDTYPE==5
	at_ret = Send_Data_By_Ctlw_Asyc(SendData,SendLen,&send_ret);
#endif
	/*发送成功*/
	if(send_ret == 0)
	{
		Update_Info_By_Send_Result(1);
		SendData = NULL;
		SendLen = 0;
	}
	/*发送失败*/
	else if(send_ret == 1)
	{
		Update_Info_By_Send_Result(0);
		User_Err_Process(at_ret);
		SendData = NULL;
		SendLen = 0;
	}
	/*AT命令相关返回值容错*/
	else if(at_ret > XY_OK)
	{
		Update_Info_By_Send_Result(0);
		User_Err_Process(at_ret);
		SendData = NULL;
		SendLen = 0;
	}

	return at_ret;
}


/*该函数每次唤醒都执行，需要放在RAM上执行，否则运行时间过长影响功耗*/
__RAM_FUNC At_status_type Send_Data_By_Cloud(void)
{
	At_status_type at_ret = XY_OK;
	
	//识别是否需要远程发送数据，例如RTC超时，或者flash中缓存的数据超过阈值
	if(is_event_set(EVENT_CLOUD_SEND) || is_event_set(EVENT_CLOUD_UPDATE))
	{
		at_ret = DO_Send_Data_By_Cloud();
	}

	return at_ret;
}

/*云业务状态机初始化*/
void Reset_Cloud_State()
{
/*云状态机复位*/
#if CLOUDTYPE==0
	g_cdp_at_step = 0;
#elif CLOUDTYPE==1
	g_onenet_at_step = 0;
#elif CLOUDTYPE==2
	g_socket_at_step = 0;
#elif CLOUDTYPE==3
	g_mqtt_at_step = 0;
#elif CLOUDTYPE==4
	g_http_at_step = 0;
#elif CLOUDTYPE==5
	g_ctlw_at_step = 0;
#endif
}


