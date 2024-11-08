#include "at_CP_api.h"
#include "at_uart.h"
#include "xy_event.h"
#include "xy_timer.h"
#include "xy_cp.h"
#include "basic_config.h"
#include "user_config.h"
#include "xy_utils.h"

/*AT+DEMOCFG=CP,<g_boot_min_sec>,<g_boot_max_sec>,<g_stop_min_sec>,<g_stop_max_sec>,<g_stop_timeout_ms>*/
uint32_t g_boot_min_sec = 60;   /*执行Boot_CP超时定时器随机时长的最小值*/
uint32_t g_boot_max_sec = 120;   /*执行Boot_CP超时定时器随机时长的最大值*/
uint32_t g_stop_min_sec = 10;   /*执行Stop_CP超时定时器随机时长的最小值*/
uint32_t g_stop_max_sec = 30;   /*执行Stop_CP超时定时器随机时长的最大值*/
uint32_t g_stop_timeout_ms = 0; /*Stop_CP(uint32_t wait_ms)入参默认值*/

/*boot_CP后构造长时间的PING动作。AT+DEMOCFG=PING,<g_PING_flag>*/
uint32_t g_PING_flag = 1;

__RAM_FUNC void Boot_CP_Timeout(void)
{
    set_event(EVENT_BOOT_CP);
}

__RAM_FUNC void Stop_CP_Timeout(void)
{
	set_event(EVENT_STOP_CP);
}

/*若想在main主循环内执行随机启停CP核的事件，在主循环中调用该接口即可*/
void Start_CP_Test() 
{
	static uint8_t s_bootcp_flag = 0;

	/*非深睡唤醒的场景上电*/
	if(0 == s_bootcp_flag)
	{
		s_bootcp_flag = 1;
		srand(xy_seed());
		Timer_AddEvent(BOOT_CP_TIMER, ((g_boot_min_sec + rand() % (g_boot_max_sec-g_boot_min_sec))*1000),Boot_CP_Timeout,0);
	}
	else if(is_event_set(EVENT_BOOT_CP))
	{
		clear_event(EVENT_BOOT_CP);
		Boot_CP(WAIT_CP_BOOT_MS);
		Send_AT_to_Ext("boot cp\r\n");
		/*boot_CP后构造长时间的PING动作,以测试RF收发阶段对AP核的影响*/
		if(g_PING_flag == 1)
		{
			if(xy_wait_tcpip_ok(120))
			{
				Send_AT_to_Ext("\r\nPDP fail!\r\n");
			}
			if(AT_Send_And_Get_Rsp("AT+NPING=221.229.214.202,88,5,10,4\r\n",120,"+NPING:", NULL) != XY_OK)
				Stop_CP(0);
			else
				AT_Send_And_Get_Rsp("AT+CPOF=2\r\n",10,NULL, NULL);
		}
		Timer_AddEvent(BOOT_CP_TIMER, ((g_boot_min_sec + rand() % (g_boot_max_sec - g_boot_min_sec)) * 1000), Boot_CP_Timeout, 0);
	}
}

void Stop_CP_Test(void)
{
	static uint8_t s_stopcp_flag = 0;

	/*非深睡唤醒的场景上电*/
	if(0 == s_stopcp_flag)
	{
		s_stopcp_flag = 1;
		Timer_AddEvent(STOP_CP_TIMER, ((g_stop_min_sec + rand() % (g_stop_max_sec - g_stop_min_sec)) * 1000), Stop_CP_Timeout, 0);
	}
	else if (is_event_set(EVENT_STOP_CP))
	{
		clear_event(EVENT_STOP_CP);

		Stop_CP(g_stop_timeout_ms);
		xy_printf("stop_CP(%d)!\r\n", g_stop_timeout_ms);

		Timer_AddEvent(STOP_CP_TIMER, ((g_stop_min_sec + rand() % (g_stop_max_sec - g_stop_min_sec)) * 1000), Stop_CP_Timeout, 0);
	}
}

