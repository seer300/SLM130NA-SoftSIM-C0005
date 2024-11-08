/** 
* @file        xy_rtc_api.h
* @brief       供用户设置RTC定时器事件，同时还提供了接口供用户设置和获取当前世界时间. \n
*	  RTC定时器目前只有一个硬件资源，平台通过软件方式实现了多个RTC ID，目前能给用户使用的为RTC_TIMER_USER1和RTC_TIMER_USER2。\n
*   RTC定时器在深睡时不掉电，超时后能够唤醒芯片执行RTC中断。
* @attention   RTC定时器事件，只支持相对时间的设置方式，如半小时唤醒一次；不支持年月日的设置方式,请根据当前世界时间自行转换为秒偏移。 \n
*    对于通过AT+NRB或xy_reboot进行的正常软重启，平台仍然保持了RTC定时器和当前世界时间有效. \n
*    对于PIN_RESET、assert、watchdog等异常重启，RTC会被重新复位，用户任务每次上电初始化时，需要识别当前用户RTC是否存在，若不存在，必须重设RTC定时器。
* @warning	 平台维护了与基站同步的世界时间，前提是NB终端attach小区成功，若未attach成功，则本地的世界时间是从2000/1/1开始的,该值由AP设置。 \n
*   如果用户需要正确的世界时间，需要确保attach成功后，再使用RTC接口功能。 \n
*   RTC采用的是32K晶振，精度低，连续运行若干天后，会造成偏差变大，若用户需要较精确的世界时间，可以自行在合适的时机重新触发小区attach。 \n
*   !!!非深睡唤醒场景下，之前设置的RTC全部无效!!!
*/

#pragma once

#include <stdint.h>
#include "rtc_tmr.h"

/**
* @brief CP核RTC总个数，目前只能为7个，用户可用的RTC ID，目前仅支持2个，用户不得增减ID，否则会造成平台内存越界
*/
typedef enum
{
	RTC_TIMER_CP_BASE,
	RTC_TIMER_CP_LPM = RTC_TIMER_CP_BASE, // PSM/TAU
	RTC_TIMER_CMCCDM,                     // CMCC DM keep alive
	RTC_TIMER_CTWING,	                    // CTWING update RTC
	RTC_TIMER_ONENET,                     // onenet update RTC
	RTC_TIMER_CDP,                        // CDP update RTC
	RTC_TIMER_CP_USER1,
	RTC_TIMER_CP_USER2,
	RTC_TIMER_CP_END
} RTC_TIMER_CP_ID;

/**
* @brief 挂钟时间结构体
* @param tm_sec  秒   (0-59)
* @param tm_min  分钟 (0-59)
* @param tm_hour 小时 (0-23) 
* @param tm_mday 每月对应的日期 (1-31)
* @param tm_mon  月份 (0-11)
* @param tm_year 年份 
*/
#define xy_wall_clock_t wall_clock_t

/**
  * @brief   设置某ID超时RTC事件
  * @param   timer_id	      ID
  * @param   sec_offset	    next timeout offset by seconds
  * @param   callback	      timeout callback
  * @param   rtc_reload	periodic RTC event flag. if 1, the event is periodic RTC event.
  * @note    对于周期性的定时功能，必须设置随机的超时时刻点，以减轻对运营商基站的运营压力，建议使用xy_rtc_set_by_day或xy_rtc_set_by_week接口
  * @warning if period <1 hour,must connect XINYIN AE!!!
  * @attention  超时回调函数不得使用阻塞机制，且代码不能运行太长事件，以免影响其他RTC的运行时序
  */
#define xy_rtc_timer_create(timer_id,sec_offset,callback,rtc_reload) rtc_event_add_by_offset(timer_id,(sec_offset)*1000,callback,rtc_reload)

/**
  * @brief   获取最快超时的RTC ALARM事件节点信息
  * @return  详见rtc_event_info_t
  * @note    timer_id   RTC事件ID，当为-1表示无效  
  */
rtc_event_info_t *xy_rtc_get_next_event();

/**
  * @brief   用于删除某一个RTC事件
  * @return  timer_id   RTC事件ID，当为-1表示无效 
  * @note    if period <1 hour,must connect XINYIN AE!!! 
  */
void xy_rtc_timer_delete(char timer_id);


/**
  * @brief   获取某用户定时器ID的下一次超时的秒数偏移。
  * @return  seconds  -1表示未设置该ID；0表示已超时
  * @note    该接口常用于异常重启等保护使用，即重启后查看是否有效，若无效，则重设该定时器。 \n
  *          客户在设置RTC事件时，必须充分考虑各种异常造成的事件未设置成功问题。
  */
int32_t xy_rtc_next_offset_by_ID(uint8_t timer_id);


/*******************************************************************************************************************************
 * @brief   设置每天某个时间段的随机TIME定时事件。不具备周期性，需用户在callback自行再次设置。
 * @param   timer_id, @ref AP_TIMER_EVENT,只能选用深睡有效的ID
 * @param   sec_start 起始时间，单位秒，从00:00:00开始计算，例如上午08:30对应8*3600 + 30*60 = 30600 秒
 * @param   sec_span  随机的秒级跨度，若需要设置定点的定时器，值为0即可。例如设置下午2:30到4:00的一个随机定时器，则填值(90*60)
 * @note    为了减轻基站的通信压力，每个终端的远程通信时刻点应具有随机性。若用户想设置周期性行为，需要自行在callback中调用该接口重设该定时ID
 * @warning    使用该接口必须确保之前已经加载过CP核且PDP激活成功过，即已经获取过最新的世界时间；否则返回false
 ******************************************************************************************************************************/
bool xy_rtc_set_by_day(uint8_t timer_id, rtc_timeout_cb_t callback, uint32_t sec_start, int sec_span);

/*******************************************************************************************************************************
 * @brief   设置每周某个时间段的随机TIME事件，非周期性。不具备周期性，需用户在callback自行再次设置。
 * @param   timer_id, @ref AP_TIMER_EVENT,只能选用深睡有效的ID
 * @param   day_week, 星期(1-7)
 * @param   sec_start，起始时间，单位秒，从00:00:00开始计算，例如上午08:30对应8*3600 + 30*60 = 30600 秒
 * @param   sec_span   随机的秒级跨度，若需要设置定点的定时器，值为0即可。例如设置下午2:30到4:00的一个随机定时器，则填值(90*60)
 * @note    为了减轻基站的通信压力，每个终端的远程通信时刻点应具有随机性。若用户想设置周期性行为，需要自行在callback中调用该接口重设该定时ID
 * @warning    使用该接口必须确保之前已经加载过CP核且PDP激活成功过，即已经获取过最新的世界时间；否则返回false
 *******************************************************************************************************************************/
bool xy_rtc_set_by_week(uint8_t timer_id, rtc_timeout_cb_t callback, int day_week, uint32_t sec_start, int sec_span);

/**
  * @brief   将相对秒数(与1970/1/1比较) 转化为世界时间格式。
  * @note   
  */
void xy_gmtime_r(const uint64_t msec, RTC_TimeTypeDef *result);

/**
  * @brief  将世界时间转化为相对秒数(与1970/1/1比较)，通常与Get_Current_UT_Time配套使用 
  * @note   返回值为毫秒
  */
uint64_t xy_mktime(RTC_TimeTypeDef *tp);

/**
  * @brief   用户手工设置当前世界时间，不建议用户使用！
  * @param   rtctime	   universal time,such as 2020/10/1 12:0:0
  * @param   zone_sec	   zone sec,such as beijing is 8*60*60,always is 0
  * @note    目前芯翼平台支持两种世界时间的更新途径，一种是3GPP在attach时后台更新当前世界时间；\n
  *  另一种是通过Get_UT_by_ntp获取当前网络世界时间。
  */
void Set_UT_Time(xy_wall_clock_t *rtctime, int zone_sec);

/**
  * @brief   获取NTP网络时间，并更新到本地，以同步本地的世界时间
  * @param   ser_name, ntp服务器地址,NULL表示默认选用ntp1.aliyun.com
  * @param   timeout, 单位秒，填0时，平台内部使用默认值20秒；因NB速率低，建议该值不得小于20秒
  *	@param   zone_sec ,本地时区秒数，例如北京时间为(8*60*60)
  * @warning 使用该接口必须确保之前已经PDP激活成功过，否则返回XY_ERR；若timeout超时，则会返回XY_Err_Timeout错误
  * @note    该接口与Get_UT_by_ntp的差异仅在于时区的设置，如果不关心时区问题，不建议使用该接口
  */
bool Set_UT_by_ntp(char *ser_name, int timeout, int zone_sec);

/**
  * @brief   获取本地的当前世界时间,可以与xy_mktime配套使用。若未设置过(如未attach成功过)，则返回失败
  * @param   rtctime	   wall time,such as 2020/10/1 12:0:0
  * @attention  该接口未考虑时区问题，获取的就是当地的年月日时间，例如北京时间。该接口不能用于带时区功能的转换
  * @warning  软复位/OPENCPU的boot_cp等操作，该区域之前内容仍然有效，无需等待attach成功
  */
bool Get_Current_UT_Time(xy_wall_clock_t *rtctime);

/**
  * @brief   解析CCLK或QCCLK等命令时使用，将输入的日期、时间字符串（时区）字符串转换成墙上时间结构体和子时区字符串
  * @param   date	输入参数，字符串格式日期，例如"2020/12/30"或"20/12/30"
  * @param   time	输入参数，"12:00:00+32"或"12:00:00"
  * @param   wall_time 输出参数，墙上时间，ref@wall_clock_t
  * @param   zone_sec 输出参数，子时区字符串
  * 
  * @attention  该接口未考虑时区问题，获取的就是当地的年月日时间，例如北京时间。该接口不能用于带时区功能的转换
  * @warning  软复位/OPENCPU的boot_cp等操作，该区域之前内容仍然有效，无需等待attach成功
  */
int convert_wall_time(char *date, char *time, xy_wall_clock_t *wall_time, int *zone_sec);


