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

/* IP包头插入实时信息的开关；该信息伴随云数据实时上传，但不会保存到本地缓存中，与采集数据不关联 */
#define    INSERT_HEAD_INFO   0


__RAM_FUNC static void Again_Send_Time_Timeout(void)
{
	xy_printf("Again send rtc timeout\n");
	set_event(EVENT_CLOUD_SEND);
}

/* 设置容错处理的下一次重新尝试发送的Time */
static void Set_Again_Send_Time()
{
	/*如果没有产品Time存在，则需要设置下一次重新尝试发送Time，以防止设备永远不唤醒工作*/
	if(-1 == Timer_GetEventLeftTime(TIME_ALARM_SEND_DATA))
	{
		Timer_AddEvent(TIME_AGAIN_SEND_DATA, AGAIN_SEND_MS, Again_Send_Time_Timeout, 0);
	}
}

/* 外部触发的WAKEUP_PIN中断处理函数，正常工作态也可触发此中断；通常为开关门瞬间或SOS按键触发 */
__RAM_FUNC void test_WakeupEn_Wakeup_Callback(void)
{
	xy_printf("wakeup trigger\n");
	set_event(EVENT_GATHER);
}

/* 异常时，相比默认的异常处理，添加了一个异常时重新设置一个发送的定时器的行为，客户可根据实际需求修改 */
int User_Err_Process(int errno)
{
	if(errno > XY_OK)
		xy_printf("User_Err_Process:%d\n", errno);
	switch(errno)
	{
		case  XY_WAITING_RSP:  //尚未等到结果码，继续在main主线程中循环等待
		case  XY_OK:      //等到OK结果码，正常处理
			break;


		//FOTA期间，AP核一般可以继续执行远程通信，客户自行考虑设计
		case  XY_ERR_DOING_FOTA:
			break;

		//用户可以对特别关注的错误单独执行定制策略
		default:
			CP_Err_Process();
			break;
	}

	if(errno != XY_OK && errno != XY_WAITING_RSP)
	{
		Set_Again_Send_Time();
		Update_Info_By_Send_Result(0);
	}
	return  errno;
}

__RAM_FUNC void Send_Update_Timeout(void)
{
	xy_printf("update time timeout\n");
	set_event(EVENT_CLOUD_UPDATE);
}

#if PERIOD_TEST

__RAM_FUNC void Period_Test_Timeout(void)
{
	xy_printf("period test timeout\n");
	set_event(EVENT_PERIOD_TEST);
}

#endif

//用户私有配置类信息初始化，通常在非深睡唤醒时调用，读取后保存在retention内存区
void User_Config_Init(void)
{
	xy_ftl_regist((void *)USER_NV_BASE,USER_NV_LEN);
	
	extern pFunType_void p_Wkupen_WakeupCallback;
	p_Wkupen_WakeupCallback = test_WakeupEn_Wakeup_Callback;

	if(Get_Boot_Reason() != WAKEUP_DSLEEP)
	{
		if(xy_ftl_read(USER_NV_BASE, &(g_Control_Context->user_config), sizeof(user_config_t))!=1)
		{
			/* 若读取配置信息失败，使用默认配置 */
			g_Control_Context->user_config.send_alarm_ctl = USER_ALARM_SEND_MS;
		}
		else
		{
			/* 判断配置是否有效，无效则使用默认配置 */
			if(g_Control_Context->user_config.send_alarm_ctl == 0)
			{
				g_Control_Context->user_config.send_alarm_ctl = USER_ALARM_SEND_MS;
			}
		}

		//BACKUP_MEM首地址2字节用于存储已使用的长度，将该地址的值赋值为0
		g_Control_Context->bkmem_saved_size = 0;
		g_Control_Context->flash_write_pos = 0;
		g_Control_Context->flash_read_pos = 0;

		//云保活周期性TIME,14：00 - 18：00 随机时间。不建议用户开启保活功能，建议注掉该函数
#if FLASH_OPTEST
		Timer_Set_By_Day(TIMER_CLOUDUPDATE, Send_Update_Timeout, 14*3600, 4*3600);
#endif

#if PERIOD_TEST
		Timer_AddEvent(TIME_PERIOD_TEST, USER_PERIOD_TEST_SHORT, Period_Test_Timeout, 0);
#endif	
	}
}

//保存用户私有数据到flash中，通常由云下行控制报文触发，如远程修改某些配置参数等
void Save_User_Config(void)
{
	if(xy_ftl_write(USER_NV_BASE,(void *)&g_Control_Context->user_config, sizeof(user_config_t)) == 0)
	{
		xy_printf("xy_ftl_write error\n");
		xy_assert(0);
	}
}

/* IP数据包头部数据信息，静态全局方式存储,需要准实时发送给云端 */
Packet_Head_t  g_once_head = {0};

/**
 * @brief 配置和查询DEMO的配置参数。
 *  查询：发送: AT+DEMOCFG?，返回：\r\n+MENCI_DEMO SEND_ALARM:%ld\r\n\r\nOK\r\n,其中%ld表示控制门常开不关等异常情况下发送报警数据的时间。
 * 
 *  设置：发送：AT+DEMOCFG=xx,其中xx表示要设置的时间，返回：\r\nOK\r\n 表示成功，其余表示失败。
 */
int at_DEMOCFG_req(char *param,char **rsp_cmd)
{
    if (g_cmd_type == AT_CMD_QUERY) //查询类
	{
        *rsp_cmd = xy_malloc(40);
		snprintf(*rsp_cmd,40, "\r\n+MENCI_DEMO SEND_ALARM:%ld\r\n\r\nOK\r\n", g_Control_Context->user_config.send_alarm_ctl);
	}
    else if (g_cmd_type == AT_CMD_REQ) //设置类
	{
		int32_t send_ms = -1;
		at_parse_param("%d",param,&send_ms);
		if(send_ms == -1)
		{
			return XY_ERR_PARAM_INVALID;
		}
		g_Control_Context->user_config.send_alarm_ctl = send_ms;
		Save_User_Config();
		Send_AT_to_Ext("\r\nOK\r\n");
		xy_Soft_Reset(SOFT_RB_BY_AP_USER);
	}
	else
	{
		return XY_ERR_PARAM_INVALID;
	}
	return XY_OK;
}

