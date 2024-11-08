#pragma once

/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "utc.h"
#include "xy_utils.h"
#include "rtc_utils.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define RTC_TMR_THREAD_NAME "rtc_tmr"
#define RTC_TMR_THREAD_PRIO osPriorityAboveNormal1
#define RTC_ALARM_INVALID 	0xFFFFFFFFFFFFFFFF
#define RTC_NEXT_OFFSET_INVAILD  0xFFFFFFFFFFFFFFFF

extern volatile unsigned int g_freq_32k;
#define XY_UTC_CLK                        (g_freq_32k)

/*将毫秒转化为RTC的tick数,常用于RTC的offset定时设置*/
#define CONVERT_MS_TO_RTCTICK(msec)   ((uint64_t)(msec)*XY_UTC_CLK/32000ULL)

/*将获取的RTC的TICK偏差转换成实际的毫秒偏差，通常用于tick差值的转换*/
#define CONVERT_RTCTICK_TO_MS(tick)      ((uint64_t)(tick)*32000ULL/XY_UTC_CLK)

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
/**
* @brief RTC 超时回调  
*/
typedef void (*rtc_timeout_cb_t)(void);

/**
* @brief 周期性RTC标志枚举
*/
typedef enum
{	
	RTC_NOT_RELOAD_FLAG = 0,              // 一次性RTC
	RTC_PERIOD_AUTO_RELOAD_FLAG = 1	      // 周期性RTC
} RTC_RELOAD_FLAGTypeDef;

/**
* @brief 用来保存RTC当前时间寄存器CAL/TIMER/CNT的值
*/
typedef struct
{
	uint32_t tm_cal;                      // CAL寄存器值
	uint32_t tm_timer;                    // TIMER寄存器值
	uint32_t tm_cnt;	                  // CNT寄存器值
} RTC_CalTimerCntTypeDef;

/**
* @brief RTC事件链表节点信息结构体
*/
typedef struct rtc_event_info
{
	uint8_t timer_id;			          // RTC事件ID，当为-1表示无效  
	uint8_t rtc_reload;                   // 周期性RTC标志位，详见RTC_RELOAD_FLAGTypeDef
	uint8_t padding[2];                   // unused  

	uint32_t rtc_alarm64_low;             // CAL/TIMER/CNT时间寄存器合并后的64位数的低32位  	
	uint32_t rtc_alarm64_high;            // CAL/TIMER/CNT时间寄存器合并后的64位数的高32位  
	
	uint32_t rtc_period_msoffset;         // 周期性RTC的毫秒偏移量，非周期性RTC可设置该值为0   
	rtc_timeout_cb_t callback;            // RTC回调函数，RTC定时到期被调用  
} rtc_event_info_t;

/*******************************************************************************
 *                       Global variable declarations                          *
 ******************************************************************************/
extern osSemaphoreId_t g_rtc_sem;
extern osMutexId_t rtc_mutex;
extern struct rtc_event_info *cp_rtc_event_arry;

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
int rtc_get_register(rtc_reg_t * rtc_reg);

/*获取RTC年月日寄存器的count值,仅限底层使用，不能用xy_gmtime_r进行转换*/
uint64_t rtc_get_cnt(void);

uint64_t rtc_time_calculate_process(rtc_reg_t *rtc_reg, RTC_TimeTypeDef *rtctime);

int rtc_event_add_by_offset(uint8_t timer_id, uint32_t msec_offset, rtc_timeout_cb_t callback, uint8_t rtc_reload);

int rtc_event_add_by_global(uint8_t timer_id, uint64_t rtc_msec, rtc_timeout_cb_t callback);

/*获取RTC寄存器中年月日对应的毫秒数tick值，该值不是严格的ms粒度，与32K精度相关，不能用于世界时间的直接加减*/
uint64_t rtc_get_tick_ms();


//根据时区重置本地世界世界，更新快照信息
int reset_universal_timer(RTC_TimeTypeDef *rtctime,int zone_sec);

// 获取当前世界时间，必须保证上电后已经ATTACH成功过，才能正确获取
uint64_t get_cur_UT_ms();

/*入参zone_sec为0时，获取当前世界时间；zone_sec如果填入当地时区，则获取的是当前格林尼治时间*/
int get_universal_timer(RTC_TimeTypeDef *rtctime,int zone_sec);

uint64_t Get_CP_ALARM_RAM();

void Set_CP_ALARM_RAM(uint64_t rtcmsc);

int rtc_set_alarm_by_cnt(uint64_t ullutccnt);

void restore_RTC_list(void);

void rtc_event_refresh();

uint64_t Transform_Num64_To_Ms(uint64_t rtc_cal_time_cnt_num64);


typedef struct
{
    uint32_t     frame_ms;                   
    uint32_t     freq_num;               
    uint16_t     cell_id;                  
} PhyFrameInfo;


/*应对永不深睡场景，周期性触发帧更新快照*/
void set_snapshot_update_timer();

/*phy在上电或小区变更时，上报帧信息，以通知主线程更新快照信息，以维持精度*/
void update_snapshot_by_frame(PhyFrameInfo *frame_info);

void set_frame_update_flag();

void set_snapshot_by_wtime(uint64_t wall_time_ms);


