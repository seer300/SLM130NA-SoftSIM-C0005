/*
 * @Description: ping demo
 */

#include "xy_printf.h"
#include "user_config.h"
#include "at_process.h"
#include "xy_cp.h"
#include "xy_system.h"
#include "at_uart.h"
#include "xy_event.h"
#include "cloud_process.h"
#include "xy_utils.h"

typedef enum
{
	PING_BOOTCP = 0,
	PING_QUERY_NET_STATE = 1,
	PING_REQ_SEND,
	PING_WAIT_RSP,
	PING_SUCC,
	PING_ERR_PROCESS,
}PING_STATE;


uint32_t g_random_stopcp_time = 0;
uint32_t g_stopcp_flag = 0;
PING_STATE pingState = PING_BOOTCP;

void Set_StopCP_timer(AP_TIMER_EVENT timer_ID, uint32_t sec_offset, TimeEventCallback callback)
{
	uint32_t off_msec;

	srand(xy_seed());

	off_msec = (uint32_t)(1 + (rand() % (sec_offset*1000)));

	Timer_AddEvent(timer_ID, off_msec, callback, 0);
}


__RAM_FUNC void Ping_Stop_CP_Timeout(void)
{
	g_stopcp_flag = 1;
	Send_AT_to_Ext("stop cp\r\n");
}

void Do_Stop_CP()
{
	uint32_t time_out = 0;
	At_status_type  at_ret = -1;
	char rsp[30];

	if(g_cloud_cfg->cp_mode == 0)
	{
		Stop_CP(0);
	}
	else if(g_cloud_cfg->cp_mode == 1)
	{
		Send_Rai();
	}
	else if(g_cloud_cfg->cp_mode == 2)
	{
		set_event(EVENT_PING_PROCESS);
		at_ret = Send_AT_Req("AT+CPOF=0\r\n",0);
		if(at_ret != XY_OK)
		{
			Send_AT_to_Ext("\r\nAT+CPOF=0 FAIL!\r\n");
			pingState = PING_ERR_PROCESS;
		}
	}
	else if(g_cloud_cfg->cp_mode == 3)
	{
		srand(xy_seed());
		time_out = (uint32_t)((rand() % 30)*1000);//构建0~30秒随机入参进行stopcp
		sprintf(rsp,"PING STOPCP WAIT_TIME:%ld\r\n",time_out);
		Send_AT_to_Ext(rsp);
		
		Stop_CP(time_out);
	}
	else
	{
		//CP核自动进深睡
	}
}

__RAM_FUNC void Ping_Process(void)
{
	At_status_type  at_ret = -1;

	if(g_stopcp_flag == 1)
	{
		Do_Stop_CP();
		
		Send_AT_to_Ext("Force stop CP!\r\n");
		g_stopcp_flag = 0;
	}

	if(is_event_set(EVENT_PING_PROCESS))
	{
		switch (pingState)
		{
			case PING_BOOTCP:
			{
				Boot_CP(WAIT_CP_BOOT_MS);

				if(g_random_stopcp_time >0)
				{
					Set_StopCP_timer(STOP_CP_TEST_TIMER, g_random_stopcp_time, Ping_Stop_CP_Timeout);
				}
				pingState = PING_QUERY_NET_STATE;
				break;
			}

			case PING_QUERY_NET_STATE:
			{

				at_ret = xy_wait_tcpip_ok(WAIT_CGATT_TIMEOUT);

				if(at_ret == XY_OK)
				{
					pingState = PING_REQ_SEND;
				}
				else
				{
					Send_AT_to_Ext("\r\n NPING NET ERROR!\r\n");
					pingState = PING_ERR_PROCESS;
				}
				break;
			}

			case PING_REQ_SEND:
			{
				at_ret = Send_AT_Req("AT+NPING=221.229.214.202,1024,1,10,0\r\n", 20);

				if(at_ret != XY_OK)
				{
					Send_AT_to_Ext("\r\nNPING REQ FAIL!\r\n");
					pingState = PING_ERR_PROCESS;
				}
				else
				{
					pingState = PING_WAIT_RSP;
				}

				break;
			}
			case PING_WAIT_RSP:
			{
				at_ret = Get_AT_Rsp("statistics: ping num:1, reply:1", NULL);
				if(at_ret == XY_OK)
				{
					Send_AT_to_Ext("\r\nNPING success,do none!\r\n");
					pingState = PING_SUCC;
				}
				else if(at_ret > 0)
				{
					Send_AT_to_Ext("\r\nNPING RSP FAIL!\r\n");
					pingState = PING_ERR_PROCESS;
				}

				break;
			}
			case PING_SUCC:
			{
				pingState = PING_BOOTCP;

				clear_event(EVENT_PING_PROCESS);

				Do_Stop_CP();
				break;
			}
			case PING_ERR_PROCESS:
			{
				Send_AT_to_Ext("\r\n PING_ERR_PROCESS!\r\n");
				pingState = PING_BOOTCP;
				clear_event(EVENT_PING_PROCESS);
				Stop_CP(0);
				break;
			}
			default:
			{
				Send_AT_to_Ext("\r\n PING STATE ERROR!\r\n");
				xy_assert(0);
			}
				
		}
	}
}