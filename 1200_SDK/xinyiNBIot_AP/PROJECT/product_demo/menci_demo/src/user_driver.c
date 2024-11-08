#include <string.h>
#include "system.h"
#include "hal_gpio.h"
#include "basic_config.h"
#include "data_gather.h"
#include "user_config.h"
#include "xy_system.h"
#include "xy_timer.h"
#include "at_process.h"
#include "xy_printf.h"
#include "hal_lptimer.h"

/* 采集数据的GPIO */
#define USER_CHECK_PIN GPIO_PAD_NUM_6

/****************************************初始化相关*********************************************/
/**
 * @brief   GPIO引脚的初始化，将引脚配置为输入上拉
 */
__RAM_FUNC void User_Peripherals_Init(void)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};

	gpio_init.Pin = USER_CHECK_PIN;
	gpio_init.Mode = GPIO_MODE_INPUT;
	gpio_init.Pull = GPIO_PULL_UP;
	HAL_GPIO_Init(&gpio_init);
}

/****************************************数据相关*********************************************/

// 发送报警数据Time的超时回调，用户可以在此根据开关门状态配置蜂鸣器是否响铃
__RAM_FUNC void Send_Alarm_Data_Time_Timeout()
{
	// 读取CHECK脚电平状态
	if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(USER_CHECK_PIN))
	{
		set_event(EVENT_GATHER);
		//此处表示开门，用户可以在此根据开关门状态配置蜂鸣器是否响铃
	}
	else
	{
		// 关门状态下，删除此Time
		Timer_DeleteEvent(TIME_ALARM_SEND_DATA);

		//此处表示关门，用户可以在此根据开关门状态配置蜂鸣器是否响铃
	}
}

/* 设置发送报警数据的Time，例如门常开不关情况下，每隔10秒上传一次报警数据，用户可以在超时回调Send_Alarm_Data_Time_Timeout里根据开关门状态配置蜂鸣器是否响铃 */
__RAM_FUNC void Set_Send_Alarm_Data_Time(void)
{
	Timer_AddEvent(TIME_ALARM_SEND_DATA, g_Control_Context->user_config.send_alarm_ctl, Send_Alarm_Data_Time_Timeout, 1);
}


/*单次采集的临时数据全局空间，至于采集哪些参数，用户自行决定*/
Gathered_data_t g_once_data = {0};


/**
 * @brief 获取当前采集的数据，包括开关门信息和当前时间，内存空间由用户决定
 * @warning 此函数接口内部不得执行CP核任何AT命令，因为此时CP核尚未启动！
 */
__RAM_FUNC void Get_Gathered_Data(void **gather_data, uint32_t *gather_len)
{
	// 读取CHECK脚电平状态，若开门动作
	if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(USER_CHECK_PIN))
	{
		g_once_data.data = 1; // 表示开门
		xy_printf("\r\nThe door is open!\r\n");

		// 门常开不关状态下，若发送报警数据的Time不存在，则重设发送报警数据的Time，直至关门为止
		if (-1 == Timer_GetEventLeftTime(TIME_ALARM_SEND_DATA))
		{
			Set_Send_Alarm_Data_Time();
		}

		//用户可以在此根据开关门状态配置蜂鸣器是否响铃
	}
	//若关门动作
	else
	{
		g_once_data.data = 0;
		xy_printf("\r\nThe door is closed!\r\n");
		//用户可以在此根据开关门状态配置蜂鸣器是否响铃
	}

	Get_Current_UT_Time(&g_once_data.time);

	*gather_data = &g_once_data;
	*gather_len = sizeof(Gathered_data_t);

#if FLASH_OPTEST
	char rsp[64] = {0};
    sprintf(rsp, "\r\nTIME:%d/%d,%d:%d:%d;\r\n",g_once_data.time.tm_mon, g_once_data.time.tm_mday,g_once_data.time.tm_hour,g_once_data.time.tm_min,g_once_data.time.tm_sec);
    Send_AT_to_Ext(rsp);
#endif
}

#if FLASH_OPTEST

__RAM_FUNC void HAL_LPTIM1_Callback(void)
{
	//Send_AT_to_Ext("LPTIM timeout,EVENT_GATHER set\n");
	set_event(EVENT_GATHER); 
}

void Lptimer_PeriodicTiming_Init(uint32_t ms)
{
	/* 设置lptimer的工作周期，单位ms */
	HAL_LPTIM_HandleTypeDef Lptim1ContinuousHandle = {0};

	Lptim1ContinuousHandle.Instance = HAL_LPTIM1;    
	Lptim1ContinuousHandle.Init.Mode = HAL_LPTIM_MODE_CONTINUOUS;
    
	HAL_LPTIM_Stop(&Lptim1ContinuousHandle);
	HAL_LPTIM_DeInit(&Lptim1ContinuousHandle);
	HAL_LPTIM_SetTimeout(&Lptim1ContinuousHandle, ms);
    
	HAL_LPTIM_Init(&Lptim1ContinuousHandle);    
	HAL_LPTIM_Start(&Lptim1ContinuousHandle);
}

#endif
