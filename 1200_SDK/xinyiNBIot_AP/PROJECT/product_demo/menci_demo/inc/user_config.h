/*
 * @file
 * @brief      用户根据自己的产品模型修改此处宏定义，不建议修改云通信相关代码。
 * @attention  上产线时，务必关注相关NV参数是否设置正确：set_tau_rtc=0；save_cloud=1；keep_cloud_alive=0；open_log=0；off_debug=1；
 */

#pragma once

#include "xy_timer.h"
#include "xy_memmap.h"

/*长短周期深睡唤醒测试 */
#define PERIOD_TEST                   		(0)


/*云通信协议类型：0,CDP;1,ONENET;2,socket;3,mqtt;4,http;5,ctwing;*/
#define  CLOUDTYPE                 			0

/* 存储器类型：0：使用外部存储器  1：使用内部存储器（flash）*/
#define SAVE_IN_FLASH						1

/* 容错处理再次重新尝试发送的时间，单位毫秒 */
#define AGAIN_SEND_MS           			(30*60*1000)

/* 发送报警数据的时间，单位毫秒，例如门常开不关情况下，每隔一段时间上传报警数据 */
#define USER_ALARM_SEND_MS           		(10*1000)

#define USER_PERIOD_TEST_SHORT           	(500)
#define USER_PERIOD_TEST_LONG           	(5*1000)

#define TIME_AGAIN_SEND_DATA          		TIMER_LP_USER1
#define TIME_ALARM_SEND_DATA          		TIMER_LP_USER2
#define TIME_PERIOD_TEST          			TIMER_LP_USER3
#define  TIMER_CLOUDUPDATE  				TIMER_LP_USER4

#define EVENT_GATHER						EVENT_USER_DEFINE1 	       
#define	EVENT_SAVE_DATA						EVENT_USER_DEFINE2	
#define EVENT_PERIOD_TEST					EVENT_USER_DEFINE3


/* 用户NV，用于保存用户配置信息;最大长度为USER_FLASH_LEN_MAX，不得越界 */
#define  USER_NV_BASE            			USER_FLASH_BASE
#define  USER_NV_LEN             			0x1000

/*  用于保存用户采集数据的flash环形缓存地址空间；最大长度为USER_FLASH_LEN_MAX，不得越界 */
#define RINGBUF_FLASH_BASE       			(USER_NV_BASE + USER_NV_LEN)
#define RINGBUF_FLASH_SIZE       			(uint32_t)(7*1024)

/*门磁demo，可在BAN_WRITE_FLASH==0时，进行高频随机写flash测试，默认关闭，测试时需打开此宏*/
#define	FLASH_OPTEST      					1     


typedef struct
{
	uint32_t        magic; /* 考虑到用户数据可能因为断电、写满覆盖等原因造成的部分脏数据，建议客户每次采集的数据通过魔术数字进行隔离，以便识别有效数据块 */
	RTC_TimeTypeDef time;  /* 时间戳 */
	uint8_t         data;  /* 门磁开关门数据 */
} Gathered_data_t;

/*IP数据包头部数据信息，如3GPP状态信息等;受INSERT_HEAD_INFO控制*/
typedef struct
{
	uint8_t     rssi;
	uint8_t     ber;
	uint8_t     padding[2];
}Packet_Head_t;

typedef struct
{
	uint32_t send_alarm_ctl;         // 控制门常开不关等异常情况下发送报警数据的时间	
}user_config_t;

/*单次采集的临时数据全局空间，至于采集哪些参数，用户自行决定*/
extern Gathered_data_t g_once_data;

/* 用户配置初始化 */
void User_Config_Init(void);

/* 用户外设初始化 */
void User_Peripherals_Init();

/* 获取需要存储的数据 */
void Get_Gathered_Data(void **gather_data,uint32_t *gather_len);

/* IP数据包发送时，需要携带在头部的信息获取，如CSQ等3GPP实时信息等 */
void Get_Packet_Head_Info(void **head, uint32_t *head_len);

void Abort_Send_Process();

#if FLASH_OPTEST
#define LPTIMER_TIMING_PERIOD				(10000)
void Lptimer_PeriodicTiming_Init(uint32_t ms);
#endif		


