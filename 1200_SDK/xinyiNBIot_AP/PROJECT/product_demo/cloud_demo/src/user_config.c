#include <string.h>
#include "hal_timer.h"
#include "user_config.h"
#include "urc_process.h"
#include "xy_ftl.h"
#include "data_gather.h"
#include "at_process.h"
#include "xy_system.h"
#include "cloud_process.h"
#include "xy_printf.h"
#include "at_uart.h"
#include "err_process.h"
#include "at_cmd_regist.h"
#include "hal_lptimer.h"


CLOUD_CFG_T  *g_cloud_cfg = (CLOUD_CFG_T  *)BAK_MEM_CLOUDTEST;

extern uint32_t g_random_stopcp_time;//ping业务过程中是否开启随机时间点关闭CP核;0:不开启业务过程中随机时间点关闭CP核,大于0:开启业务过程中随机时间点关闭CP核,随机时间范围(1~random_stopcp_time)

uint8_t 	*g_sending_buf = NULL;
uint32_t	g_reserved_len = 0;  //还剩多少字节未发送

uint32_t Get_Data_To_Send(uint32_t max_len, void **addr, uint32_t *len)
{
	if(g_cloud_cfg->data_len == 0)
		return  0;

	*len = (g_reserved_len < max_len ? g_reserved_len : max_len) ;
	g_reserved_len -= *len;
	
	*addr = g_sending_buf;
	
	return  *len;
}

/*发送失败，继续原来的数据发送*/
void Update_Info_By_Send_Result(uint32_t send_succ)
{
	(void)(send_succ);
}

__RAM_FUNC void Send_Data_Timeout(void)
{
	xy_printf("Send_Data_Timeout\n");
	set_event(EVENT_CLOUD_SEND);
	g_reserved_len = g_cloud_cfg->data_len*8;
}

__RAM_FUNC void Send_Update_Timeout(void)
{
	xy_printf("Send_Update_Timeout\n");
	set_event(EVENT_CLOUD_UPDATE);
	Set_RC32K_Cali_Event();
}


__RAM_FUNC void Send_Ping_Timeout(void)
{
	xy_printf("Send_Ping_Timeout\n");
	set_event(EVENT_PING_PROCESS);
}

/**
 * @brief  AT+DEMOCFG=CLOUD,<data_len>,<data_period>,<update_period>    
 *         其中，period单位为分钟,data_len为8字节整数倍。
 *         AT+DEMOCFG=PING,<ping_period>,<CP_mode>[,<stop cp random second>]
 *         其中，period单位为分钟。CP_mode为PING结束后CP核操作模式，0:stop_CP(0);1:Send_Rai；2:AT+CPOF;3:stop_CP随机停；其他值，让CP核自行深睡
 *         <stop cp random second>，可选参数，指示boot_CP后随机强行停CP核的最大秒数间隔
 */
int at_DEMOCFG_req(char *param,char **rsp_cmd)
{
    if (g_cmd_type == AT_CMD_QUERY) //查询类
	{
        *rsp_cmd = xy_malloc(40);
		snprintf(*rsp_cmd,40, "\r\n+DEMOCFG:%d,%d,%d,%ld\r\n\r\nOK\r\n",g_cloud_cfg->data_len,g_cloud_cfg->data_period,g_cloud_cfg->update_period, g_random_stopcp_time);
	}
    else if (g_cmd_type == AT_CMD_REQ) //设置类
	{
		char cmd[10] = {0};
		
		if (at_parse_param("%10s", param, cmd) != XY_OK)
		{
			return XY_ERR_PARAM_INVALID;			
		}

		if (!strcmp(cmd, "CLOUD"))
		{
			at_parse_param(",%d,%d,%d,",param,&g_cloud_cfg->data_len,&g_cloud_cfg->data_period,&g_cloud_cfg->update_period);
			
			if(g_sending_buf == NULL)
			{
				g_sending_buf = xy_malloc(g_cloud_cfg->data_len*8);
			}
			if(g_cloud_cfg->update_period != 0)
			{
				Timer_AddEvent(TIMER_CLOUDUPDATE,(g_cloud_cfg->update_period*60*1000), Send_Update_Timeout, 1);
			}
			else
			{
				Timer_DeleteEvent(TIMER_CLOUDUPDATE);
			}

			if(g_cloud_cfg->data_period!=0 && g_cloud_cfg->data_len!=0)
			{
				Timer_AddEvent(TIMER_SENDDATA,(g_cloud_cfg->data_period*60*1000), Send_Data_Timeout, 1);
			}
			else
			{
				Timer_DeleteEvent(TIMER_SENDDATA);
			}
		}
		else if (!strcmp(cmd, "PING"))
		{
			at_parse_param(",%d,%d,%d,",param,&g_cloud_cfg->ping_period,&g_cloud_cfg->cp_mode, &g_random_stopcp_time);

			if(g_cloud_cfg->ping_period != 0)
			{
				Timer_AddEvent(PING_TIMER,(g_cloud_cfg->ping_period*60*1000), Send_Ping_Timeout, 1);
			}
			else
			{
				Timer_DeleteEvent(PING_TIMER);
			}
		}
	}
	else
	{
		return XY_ERR_PARAM_INVALID;
	}
	return XY_OK;
}