#include "hw_memmap.h"
#include "hw_utc.h"
#include "xy_rtc_api.h"

static const uint16_t g_daysbeforemonth[13] =
{
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};

/**
 * Claculate the days before a specified month,month 0-based.
 *
 * @param[in]   month      The specified month.
 * @param[in]   year      Leapyear check.
 * @return      Sum days.
 */
int clock_days_before_month(int month, bool leapyear)
{
	int retval = g_daysbeforemonth[month];

	if (month >= 2 && leapyear) 
  {
	  retval++;
	}
	return retval;
}

/**
 * Convert calendar to utc time.
 *
 * @param[in]   year      The specified year.
 * @param[in]   month      The specified month.
 * @param[in]   day      The specified day.
 * @return      Sum days.
 */
uint32_t clock_calendar_to_utc(int year, int month, int day)
{
	uint32_t days;

	/* Years since epoch in units of days (ignoring leap years). */
	days = (year - 1970) * 365;
	/* Add in the extra days for the leap years prior to the current year. */
	days += (year - 1969) >> 2;
	/* Add in the days up to the beginning of this month. */
	days += (uint32_t)clock_days_before_month(month, CLOCK_IS_LEAPYEAR(year));
	/* Add in the days since the beginning of this month (days are 1-based). */
	days += day - 1;
	/* Then convert the seconds and add in hours, minutes, and seconds */
	return days;
}

static int clock_dayoftheweek(int mday, int month, int year)
{
  if((month == 1) || (month == 2)) 
  {  
		month += 12;  
		year--;  
	}  
	
	return (mday + 2*month + 3*(month+1)/5 + year + year/4 - year/100 + year/400) % 7 + 1;  
}

static void clock_utc_to_calendar(uint64_t days, int *year, int *month, int *day)
{
  int  value;
  int  min;
  int  max;
  uint64_t  tmp;
  bool leapyear;

  /* There is one leap year every four years, so we can get close with the
   * following:
   */
  value   = days  / (4 * 365 + 1); /* Number of 4-years periods since the epoch */
  days   -= value * (4 * 365 + 1); /* Remaining days */
  value <<= 2;                     /* Years since the epoch */

  /* Then we will brute force the next 0-3 years */

  for (;;) 
  {
      /* Is this year a leap year (we'll need this later too) */
      leapyear = CLOCK_IS_LEAPYEAR(value + 1970);

      /* Get the number of days in the year */
      tmp = (leapyear ? 366 : 365);

      /* Do we have that many days? */
      if (days >= tmp) 
      {
        /* Yes.. bump up the year */
        value++;
        days -= tmp;
      }
      else 
      {
        /* Nope... then go handle months */
        break;
      }
  }

  /* At this point, value has the year and days has number days into this year */
  *year = 1970 + value;

  /* Handle the month (zero based) */
  min = 0;
  max = 11;

  do{
      /* Get the midpoint */
      value = (min + max) >> 1;

      /* Get the number of days that occurred before the beginning of the month
       * following the midpoint.
       */
      tmp = clock_days_before_month(value + 1, leapyear);

      /* Does the number of days before this month that equal or exceed the
       * number of days we have remaining?
       */
      if (tmp > days) 
      {
          /* Yes.. then the month we want is somewhere from 'min' and to the
           * midpoint, 'value'.  Could it be the midpoint?
           */
          tmp = clock_days_before_month(value, leapyear);
          if (tmp > days) 
          {
            /* No... The one we want is somewhere between min and value-1 */
            max = value - 1;
          }
          else 
          {
            /* Yes.. 'value' contains the month that we want */
            break;
          }
        }
      else 
      {
        /* No... The one we want is somwhere between value+1 and max */
        min = value + 1;
      }

      /* If we break out of the loop because min == max, then we want value
       * to be equal to min == max.
       */
      value = min;
    } while (min < max);

  /* The selected month number is in value. Subtract the number of days in the
   * selected month
   */
  days -= clock_days_before_month(value, leapyear);

  /* At this point, value has the month into this year (zero based) and days has
   * number of days into this month (zero based)
   */
  *month = value + 1; /* 1-based */
  *day   = days + 1;  /* 1-based */
}

/*from 1970/1/1, msec*/
uint64_t xy_mktime(RTC_TimeTypeDef *tp)
{
  uint32_t msec,sec,min,hour,mday,mon,year;
  uint64_t ret;

  msec = tp->tm_msec,
  sec  = tp->wall_clock.tm_sec;
  min  = tp->wall_clock.tm_min;
  hour = tp->wall_clock.tm_hour;
  mday = tp->wall_clock.tm_mday;
  mon  = tp->wall_clock.tm_mon;
  year = tp->wall_clock.tm_year;

  if ((sec | min | hour | mday | mon | year) == 0)
    return 0;

  /* 1..12 -> 11,12,1..10 */
  if (0 >= (int)(mon -= 2))
  {
    mon += 12; /* Puts Feb last since it has leap day */
    year -= 1;
  }

  ret = (((((uint64_t)(year / 4 - year / 100 + year / 400 + 367 * mon / 12 + mday) +
            (uint64_t)year  * 365  - 719499) * 24 +
            (uint64_t)hour) * 60   +
            (uint64_t)min)  * 60   +
            (uint64_t)sec)  * 1000 +
            (uint64_t)msec;

  return ret;
} 

// 将相对1970年1月1日的毫秒偏移转换为RTC通用时间结构体格式
void xy_gmtime_r(const uint64_t msec, RTC_TimeTypeDef *result)
{
  uint64_t jdn;
  uint64_t epoch;  
  uint32_t hour;
  uint32_t min;
  uint32_t sec;
  uint32_t year =0;
  uint32_t month = 0;
  uint32_t day = 0;

  // 获取相对于1970年1月1日的秒偏移
  epoch = (msec) / 1000;

  // 减去天、小时、分钟，只得到秒数
  jdn    = epoch / SEC_PER_DAY;
  epoch -= SEC_PER_DAY * jdn;

  hour   = epoch / SEC_PER_HOUR;
  epoch -= SEC_PER_HOUR * hour;

  min    = (uint32_t)epoch / SEC_PER_MIN;
  epoch -= SEC_PER_MIN * min;

  sec    = epoch;

  // 获取天、小时、分钟、秒日历格式的时间
  clock_utc_to_calendar(jdn, (int *)&year, (int *)&month, (int *)&day);

  result->wall_clock.tm_year  = (uint32_t)year;
  result->wall_clock.tm_mon   = (uint32_t)month;   
  result->wall_clock.tm_mday  = (uint32_t)day;        
  result->wall_clock.tm_hour  = (uint32_t)hour;
  result->wall_clock.tm_min   = (uint32_t)min;
  result->wall_clock.tm_sec   = (uint32_t)sec;
  result->tm_msec  = (uint32_t)((msec) % 1000);

  result->tm_wday  = clock_dayoftheweek(day, month, year);
  result->tm_yday  = day + clock_days_before_month(month-1, CLOCK_IS_LEAPYEAR(year));
  result->tm_isdst = 0;
}
