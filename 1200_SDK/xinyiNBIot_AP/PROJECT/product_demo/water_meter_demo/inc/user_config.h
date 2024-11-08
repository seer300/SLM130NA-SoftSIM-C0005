#pragma once
#include "basic_config.h"
#include "xy_timer.h"
#include "hal_lptimer.h"

/*设为1，则工作周期为5秒；设为0，则工作周期为1秒内*/
#define SLEEP_PERIOD_MODE           (0)

/* 设置UTC_WDT的工作周期，单位s */
#define USER_UTC_WDT_TIME			(5)

/* 设置UTC_WDT的工作周期，单位s */
#define USER_FOTA_UTC_WDT_TIME	    (1800)

/* 设置阻塞等待联网的最大等待时长，单位s */
#define USER_WAIT_SEC			    (120)

#if (SLEEP_PERIOD_MODE)
#define LPTIMER_TIMING_PERIOD		(6000)
#define XY_TIMER_TIMING_PERIOD		(5000)
#else
#define LPTIMER_TIMING_PERIOD		(600)
#define XY_TIMER_TIMING_PERIOD		(1000)
#endif

#define XY_TIMER_RTC        		TIMER_LP_USER4

/*是否进行外设测试，0不进行；1进行，此时需要正确的接入E2与Norflash，否则会断言*/
#define DRIVER_TEST					0        

//事件宏重定义
#define EVENT_XY_TIMER				EVENT_USER_DEFINE2
#define EVENT_LPTIMER				EVENT_USER_DEFINE3
#define EVENT_CTL					EVENT_USER_DEFINE4

typedef struct
{
	unsigned char object_Id[6];
	uint8_t padding[2];

	unsigned char Revision[6];
	uint8_t padding1[2];

	unsigned char user_imei[15];
	uint8_t padding2[1];

	unsigned char user_imsi[15];
	uint8_t padding3[1];

	unsigned char user_usim[20]; 

	unsigned char user_apn[100]; 

}User_Info_t;//长度168


extern User_Info_t g_user_info;
extern HAL_LPTIM_HandleTypeDef Lptim1ContinuousHandle;
extern uint8_t g_send_mark;
extern int16_t g_user_temperature;
extern int16_t g_user_vbat;

void User_Config_Init(void);

void User_Info_Init(void);

int at_DEMOCFG_req(char *at_buf, char **prsp_cmd);