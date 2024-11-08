#include "xy_list.h"
#include "xy_system.h"
#include "at_process.h"
#include "err_process.h"
#include "xy_cp.h"



/* 指示当前PDP是否已激活 */
int g_tcpip_ok;

/* 查询是否已PDP激活的开始时刻点，一旦知悉最终结果，将会被清零 */
uint64_t g_cgatt_start_tick = 0;

At_status_type xy_wait_tcpip_ok_asyc(int wait_sec)
{
    At_status_type at_ret = XY_OK;

	uint32_t cgatt = 0;

	if(g_cgatt_start_tick == 0)
		g_cgatt_start_tick = Get_Tick();

	/*此处需要考虑扫频、小区驻留、attach等各种流程的总忍耐时长*/
	if(wait_sec==0 || !Check_Ms_Timeout(g_cgatt_start_tick,(uint32_t)(wait_sec*1000)))
	{
		at_ret = AT_Send_And_Get_Rsp("AT+CGATT?\r\n", 10, "+CGATT:","%d",&cgatt);
		
		if(at_ret == XY_OK && cgatt == 0)
		{
			if(wait_sec==0)
			{
				g_cgatt_start_tick = 0;
				at_ret = XY_ERR_NOT_NET_CONNECT;
			}
			else
				at_ret = XY_ATTACHING;
		}
		else if(at_ret == XY_OK && cgatt == 1)
		{
			g_cgatt_start_tick = 0;
		}
	}
	else
	{
		g_cgatt_start_tick = 0;
		at_ret = XY_ERR_NOT_NET_CONNECT;
	}
	xy_printf("xy_wait_tcpip_ok_asyc  errno=%d\n",at_ret);
	
	return  at_ret;
}

/*慎用！超时等待TCPIP网路建立成功，会阻塞main主线程一段时间，造成其他事件不能及时处理*/
At_status_type xy_wait_tcpip_ok(int wait_sec)
{
	At_status_type at_ret = XY_OK;
	int cgatt = -1;
	int repeat_count = wait_sec*2;

	if (CP_Is_Alive() == false)
		return XY_ERR_NOT_ALLOWED;
	
repeat_query:
	at_ret = AT_Send_And_Get_Rsp("AT+CGATT?\r\n", AT_RESPONSE_TIMEOUT, "+CGATT:", "%d", &cgatt);
	if(at_ret != XY_OK)
		return XY_ERR_NOT_NET_CONNECT;

	if(cgatt != 1)
	{
		repeat_count--;
		if(repeat_count > 0)
		{
			HAL_Delay(500);
			goto repeat_query;
		}
		else
		{
			return XY_ERR_NOT_NET_CONNECT;
		}
	}
	else
		return XY_OK;
}

/*用户可以在此执行无卡的容错操作，例如调用Send_Rai接口触发CP核进入深睡，或者调用Stop_CP强行下电CP*/
void URC_SIMST_Proc(char *paramlist)
{
    int simcard_state = -1;
    at_parse_param("%d", paramlist, &simcard_state);
    xy_printf("SIMST:%d\n", simcard_state);
}

