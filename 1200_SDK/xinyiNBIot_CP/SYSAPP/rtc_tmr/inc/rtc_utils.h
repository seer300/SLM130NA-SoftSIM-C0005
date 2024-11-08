
#pragma once
/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include <stdint.h>

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define SEC_PER_MIN  ((uint64_t)60)
#define SEC_PER_HOUR ((uint64_t)60 * SEC_PER_MIN)
#define SEC_PER_DAY  ((uint64_t)24 * SEC_PER_HOUR)

// 判断year是否是闰年
#define CLOCK_IS_LEAPYEAR(year) (((year)%400) ? (((year)%100) ? (((year)%4)?0:1) : 0) : 1)

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
typedef struct 
{	
	uint32_t rtc_cal;
	uint32_t rtc_timer;
	uint32_t rtc_cnt;
} rtc_reg_t;

typedef struct
{
	uint32_t tm_sec;  /* Seconds (0-59) */
  	uint32_t tm_min;  /* Minutes (0-59) */
  	uint32_t tm_hour; /* Hours (0-23) */
  	uint32_t tm_mday; /* Day of the month (1-31) */
  	uint32_t tm_mon;  /* Month (0-11) */
  	uint32_t tm_year; /* Years */
} wall_clock_t;

/**
* @brief RTC通用时间结构体
*/
typedef struct
{	
	wall_clock_t wall_clock;
	uint32_t tm_msec;                     // Millisecond
	uint32_t tm_wday;	                  // Day of the week (1-7)
	uint32_t tm_yday;	                  // Day of the year (1-365),always ignore
	uint32_t tm_isdst;	                  // Non-0 if daylight savings time is in effect,always ignore
} RTC_TimeTypeDef;

/**
 * Claculate the days before a specified month,month 0-based.
 *
 * @param[in]   month      The specified month.
 * @param[in]   year      Leapyear check.
 * @return      Sum days.
 */
int clock_days_before_month(int month, bool leapyear);