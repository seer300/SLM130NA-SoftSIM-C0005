/**
 * @brief 定时相关API，具体的ID可选支持是否深睡保持，以满足不同客户需要；目前仅支持设置40天内的定时器
 * 
 * @warning 建议使用LPTIMER产生秒级心跳时钟(参见McuLptimerSetPeriod())，并使用Get_UT_Time_Fast接口进行世界时间的更新，以保证实时性同时能够有效减少运行时长。
 *          xy_timer.h中给出的是clocktick硬件timer，软件上实现了多事件遍历，实时性不能保证，不宜作为心跳包使用。
 * 
 * @note xy_timer定时器使用clocktick硬件，clocktick时钟源为32K，分频系数为32. 最小精度为1ms。
 *       
 * @note xy_timer误差
 *       (1)精度误差：RC32K的精度误差为 ±350ppm ，XTAL32K的精度误差为 ±90ppm
 *       (2)温漂误差：32K时钟源由于温度变化会导致频率偏移，一天的误差在2秒内
 *       (3)由于该定时器通过软件实现了多ID机制，,如果对精度要求高的，如毫秒级周期性定时器，建议专用LPTIMER或TIMER2，参见LptimerSetPeriod()
 *       (4)periodic周期性定时器，是软件维护的，在处理完所有超时事件后设置下一个定时器的，进而无法保证周期性定时器的精度，与用户超时回调耗时耦合很大。
 *       (5)clock tick深睡或软复位仍然维持计数，硬复位后重新计数。
 */
#pragma once
#include "utc.h"
#include "driver_utils.h"


/* 不能设置超过40天的时长 */
#define MAX_TIME_PERIOD_MS                  ((uint32_t)40*24*60*60*1000)  //0XCDFE6000

#define NEXT_TIMER_TICK_INVAILD         ((uint64_t)0xFFFFFFFFFFFFFFFF)        //CLK TIMER最近的超时时刻点无效


typedef void (*TimeEventCallback)(void);


/* @brief	TIMER事件号，分为两类：一类是深睡仍工作的，目前只支持6个；另一类是深睡后就无效的，个数用户可扩展。*/
typedef enum
{
	/* 总共6个深睡能保持的定时器，该宏值大小不得修改 */
    /* These timers(6) can be maintained in DEEPSLEEP. The macro value cannot be modified */
	TIMER_LP_BASE=0,
	TIMER_LP_USER1=TIMER_LP_BASE,            /*!< TIME用户深睡仍有效事件1,用户可以自行改宏名(注：模组形态时被系统占用，用户不可使用) */
	TIMER_LP_USER2,         				 /*!< TIME用户深睡仍有效事件2,用户可以自行改宏名*/
	TIMER_LP_USER3,                          /*!< TIME用户深睡仍有效事件3,用户可以自行改宏名 */
	TIMER_LP_USER4,                          /*!< TIME用户深睡仍有效事件4,用户可以自行改宏名 */
	TIMER_LP_USER5,                          /*!< TIME用户深睡仍有效事件5,用户可以自行改宏名 */
	TIMER_LP_USER6, 						 /*!< TIME用户深睡仍有效事件5,用户可以自行改宏名 */
	TIMER_LP_END = 6,						 /*!< 总共6个深睡能保持的定时器，该宏值大小不得修改 */

	/* 深睡无效的定时器，不够用的话，用户可自行添加ID */
	/* These timers will be invalid in DEEPSLEEP. If it is not enough, the user can add ID by himself */
	TIMER_NON_LP_BASE = TIMER_LP_END,	   /*!< 深睡无效的TIMER事件基地址，该值大小严禁修改 */
	TIMER_WAIT_AT_RSP = TIMER_NON_LP_BASE, /*!< 内部占用，等待CP核AT应答结果超时定时器,深睡无效 */
	TIMER_SOFT_WDT,                          /*!< 软看门狗，供客户业务容错用的，不能百分百保证生效。芯片总体的监控必须使用硬看门狗，具体参看AP_WDT_Init */
	TIMER_AT_STANDBY,                        /*!< 波特率大于9600时,设置锁STANDBY的超时定时器 */
	TIMER_NON_LP_BLE,                        /*!< BLE等待确认的超时定时器 */
	TIMER_BLE_AT_END,                        /*!< BLE等待AT结尾'\r'的定时器 */
	TIMER_NON_LP_USER1,                      /*!< TIME用户深睡无效事件1,用户可以自行改宏名 */
	TIMER_NON_LP_USER2,                      /*!< TIME用户深睡无效事件2,用户可以自行改宏名 */
	TIMER_NON_LP_USER3,                      /*!< TIME用户深睡无效事件3,用户可以自行改宏名 */
	TIMER_NON_LP_END                         /*!< 深睡无效的定时器大小，用户可自行扩展 */
} AP_TIMER_EVENT;






/******************************************************************************************************************************
 * @brief 设置毫秒级定时器，并立即生效.
 * @param timer_ID		  用户必须根据定时是否深睡仍需有效来选择正确的ID号
 * @param msec_offset	  TIME事件延时时间，单位为毫秒(此值不能超过40天.相对当前时间，延时时间到期后调用回调函数.
 * @param callback		  TIME回调函数
 * @param periodic	      周期性标志位，为1时表示周期性，由系统后台保证周期性触发调用callback
 * @warning  该接口为简化用户使用而设计，如果用户想分步进行定时器设置，请使用Time_NewEvent->Time_EventStart两个接口。
 * @attention 由于周期性是超时后软件设置下一个时刻点，而超时处理耗时可能达到毫秒级别，造成周期性不准。如果对精度要求高的，如毫秒级周期性定时器，建议专用LPTIMER，参见LptimerSetPeriod()
 ******************************************************************************************************************************/
bool Timer_AddEvent(AP_TIMER_EVENT timer_ID, uint32_t msec_offset, TimeEventCallback callback, uint8_t periodic);


/******************************************************************************************************************************
 * @brief 设置毫秒级定时器，但不立即生效，需要用户主动调用Time_EventStart
 * @param timer_ID		  用户必须根据定时是否深睡仍需有效来选择正确的ID号
 * @param msec_offset	  TIME事件延时时间，单位为毫秒(此值不能超过40天.相对当前时间，延时时间到期后调用回调函数.
 * @param callback		  TIME回调函数
 * @param periodic	      周期性标志位，为1时表示周期性，由系统后台保证周期性触发调用callback
 * @warning  调用该接口必须确保该ID定时器之前未被创建过，否则会出现紊乱。
 * @attention 由于周期性是超时后软件设置下一个时刻点，而超时处理耗时可能达到毫秒级别，造成周期性不准。如果对精度要求高的，如毫秒级周期性定时器，建议专用LPTIMER，参见LptimerSetPeriod()
 ******************************************************************************************************************************/
bool Time_NewEvent(AP_TIMER_EVENT timer_ID, uint32_t msec_offset, TimeEventCallback callback, uint8_t periodic);

/******************************************************************************************************************************
 * @brief 启动定时器
 * @param timer_ID		  TIME事件ID
 * @warning  Timer_AddEvent接口内部已经自动启动，无需再调用该接口
 ******************************************************************************************************************************/
bool Time_EventStart(AP_TIMER_EVENT timer_ID);

/******************************************************************************************************************************
 * @brief 停止定时器
 * @param timer_ID		  TIME事件ID
 * @warning  用户必须根据定时是否深睡仍需有效来选择正确的ID号；该定时精度为32K
 * @return bool值
 ******************************************************************************************************************************/
bool Time_EventStop(AP_TIMER_EVENT timer_ID);

/******************************************************************************************************************************
 * @brief  删除某timer事件ID
 ******************************************************************************************************************************/
bool Timer_DeleteEvent(AP_TIMER_EVENT timer_ID);

/******************************************************************************************************************************
 * @brief  获取某TIMER事件距离到期剩余时长，非周期性。
 * @return uint32_t	 剩余时长，单位为ms;-1表示此TIME不存在或已超时；0表示恰好超时
 ******************************************************************************************************************************/
int Timer_GetEventLeftTime(AP_TIMER_EVENT timer_ID);

/*******************************************************************************************************************************
 * @brief   设置每天某个时间段的随机TIME定时事件。不具备周期性，需用户在callback自行再次设置。
 * @param   timer_id, @ref AP_TIMER_EVENT,只能选用深睡有效的ID
 * @param   sec_start，起始时间，单位秒，从00:00:00开始计算，例如上午08:30对应8*3600 + 30*60 = 30600 秒
 * @param   sec_span  随机的秒级跨度，若需要设置定点的定时器，值为0即可。例如设置下午2:30到4:00的一个随机定时器，则填值(90*60)
 * @note    为了减轻基站的通信压力，每个终端的远程通信时刻点应具有随机性。若用户想设置周期性行为，需要自行在callback中调用该接口重设该定时ID
 * @warning    使用该接口必须确保之前已经加载过CP核且PDP激活成功过，即已经获取过最新的世界时间；否则返回false
 ******************************************************************************************************************************/
bool Timer_Set_By_Day(AP_TIMER_EVENT timer_ID, TimeEventCallback callback, int sec_start, int sec_span);

/*******************************************************************************************************************************
 * @brief   设置每周某个时间段的随机TIME事件，非周期性。不具备周期性，需用户在callback自行再次设置。
 * @param   timer_id, @ref AP_TIMER_EVENT,只能选用深睡有效的ID
 * @param   day_week, 星期(1-7)
 * @param   sec_start，起始时间，单位秒，从00:00:00开始计算，例如上午08:30对应8*3600 + 30*60 = 30600 秒
 * @param   sec_span   随机的秒级跨度，若需要设置定点的定时器，值为0即可。例如设置下午2:30到4:00的一个随机定时器，则填值(90*60)
 * @note    为了减轻基站的通信压力，每个终端的远程通信时刻点应具有随机性。若用户想设置周期性行为，需要自行在callback中调用该接口重设该定时ID
 * @warning    使用该接口必须确保之前已经加载过CP核且PDP激活成功过，即已经获取过最新的世界时间；否则返回false
 *******************************************************************************************************************************/
bool Timer_Set_By_Week(AP_TIMER_EVENT timer_ID, TimeEventCallback callback, int day_week, int sec_start, int sec_span);

/*******************************************************************************************************************************
 * @brief	获取当前世界时间（当前为北京时间）。该接口耗时长，如果秒级周期调用，建议通过LPTIMER自行维护心跳周期，然后通过Get_UT_Time_Fast()接口获取当前时间
 * @param   rtctime RTC通用时间格式(年/月/日 时:分:秒），包含在RTC_TimeTypeDef结构体中
 * @return  bool值，1表示有时间快照，能够获取最新世界时间；0表示无快照，获取的是RTC本地寄存器的世界时间，从2000.1.1开始计时的
 * @attention  设置世界时间有两个途径，一个是在CP核，根据3GPP或SNTP后台自动设置；另一个是在AP核，由用户手工设置。两个设置途径是按照时间顺序覆盖式设置的。
 * @note    由于32K时钟频率，再加上温度变化造成的频偏影响，一天的误差在10秒左右。
 * @note    系统上电原因为SOFT_RESET(NRB/xy_Soft_Reset)或WAKEUP_DSLEEP两个场景，仍然可以获取之前的世界时间，前提是之前曾获取过有效的世界时间。
 *******************************************************************************************************************************/
bool Get_Current_UT_Time(RTC_TimeTypeDef *rtctime);

/*******************************************************************************************************************************
 * @brief	获取当前格林威治时间（GMT）。
 * @param   rtctime RTC通用时间格式(年/月/日 时:分:秒），包含在RTC_TimeTypeDef结构体中
 * @return  bool值，1表示有时间快照，能够获取最新格林威治时间；0表示无快照，获取的是RTC本地寄存器的世界时间，从2000.1.1开始计时的
 * @note    由于32K时钟频率，再加上温度变化造成的频偏影响，一天的误差在10秒左右。
 * @note    系统上电原因为SOFT_RESET(NRB/xy_Soft_Reset)或WAKEUP_DSLEEP两个场景，仍然可以获取之前的GMT时间，前提是之前曾获取过有效的世界时间。
 *******************************************************************************************************************************/
bool Get_GMT_Time(RTC_TimeTypeDef *rtctime);

/*******************************************************************************************************************************
 * @brief	供用户根据自己的工作周期定时器来自行维护本地世界时间。该接口比Get_Current_UT_Time运行耗时更短。
 * @param   time    RTC通用时间(年/月/日 时:分:秒）,初始值通常由Get_Current_UT_Time获取
 * @param   ms_diff 用户工作的周期定时器心跳时长，ms；不得超过60秒，即最大值为60000
 * @attention  该接口当前未维护星期几和绝对日期，即tm_wday和tm_yday不可用
 * @note    该接口存在的价值就是Get_Current_UT_Time耗时太长，影响功耗。进而提供简化接口，以降低功耗。
 * 具体使用时，用户先通过Get_Current_UT_Time()获取当前世界时间，然后记录到全局中，后续每次心跳超时中断事件中，调用Get_UT_Time_Fast()来更新当前世界时间即可。
 *******************************************************************************************************************************/
void Get_UT_Time_Fast(RTC_TimeTypeDef *time,uint32_t ms_diff);

/*******************************************************************************************************************************
 * @brief	由用户手工设置世界时间(北京时间)。当出厂NV参数g_NITZ为1时，一旦ATTACH成功，则由芯翼后台自动设置最新世界时间。
 * @param   rtctime，参看RTC_TimeTypeDef
 * @attention  设置世界时间有两个途径，一个是在CP核，根据3GPP或SNTP后台自动设置；另一个是在AP核，由用户手工设置。两个设置途径是按照时间顺序覆盖式设置的。
 * @warning    若使用该接口，必须将出厂NV参数g_NITZ设置为0，即不容许芯翼后台自行更新世界时间快照
 *******************************************************************************************************************************/
void Set_UT_Time(RTC_TimeTypeDef *rtctime);

