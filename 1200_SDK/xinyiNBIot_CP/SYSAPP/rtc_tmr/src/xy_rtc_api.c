#include "softap_nv.h"
#include "xy_rtc_api.h"
#include "at_tcpip_api.h"

void Set_UT_Time(xy_wall_clock_t *wall_clock,int zone_sec)
{
	RTC_TimeTypeDef rtctime = {0};
	
	rtctime.wall_clock = *wall_clock;

	reset_universal_timer(&rtctime, zone_sec);
}

//get beijing year/mon/day/hour,and not care zone;if not attached,return 0
bool Get_Current_UT_Time(xy_wall_clock_t *wall_clock)
{
	RTC_TimeTypeDef rtctime = {0};

	//if never attached,now_wall_time is local time,not UT time,maybe is 2018/10/1
	if (1 == get_universal_timer(&rtctime, 0))
	{
		*wall_clock = rtctime.wall_clock;

		return 1;
	}
	else
	{
		return 0;
	}
}


/*if set 15:00--18:00 everyday to do user work,then call xy_rtc_set_by_day(RTC_TIMER_USER1,15*60*60,3*60*60)*/
bool xy_rtc_set_by_day(uint8_t timer_id, rtc_timeout_cb_t callback, uint32_t sec_start, int sec_span)
{
	int sec_offset;
	xy_wall_clock_t now_wall_time = {0};

	//if never attached,now_wall_time is local time,not UT time,maybe is 2018/10/1
	if (!Get_Current_UT_Time(&now_wall_time))
		return 0;

	sec_offset = (int)(sec_start - (now_wall_time.tm_hour * 3600 + now_wall_time.tm_min * 60 + now_wall_time.tm_sec));

	if(sec_offset <= 0)
		sec_offset += 3600*24;

	if(sec_span == 0)
		sec_span = 1;

	//set rand work time  to reduce BSS stress
	xy_rtc_timer_create(timer_id, sec_offset+(xy_rand()%sec_span), callback, 0);
	return 1;
}

/*if set sunday 15:00--20:00 every week to do user work,then call xy_rtc_set_by_week(RTC_TIMER_USER1,7,15*60*60,5*60*60)*/
/*day_week  is 1-7*/
bool xy_rtc_set_by_week(uint8_t timer_id, rtc_timeout_cb_t callback, int day_week, uint32_t sec_start, int sec_span)
{
	int sec_offset;
	RTC_TimeTypeDef now_wall_time = {0};

	//if never attached,now_wall_time is local time,not UT time,maybe is 2018/10/1
	if(get_universal_timer(&now_wall_time, 0) == 0)
		return 0;

	sec_offset = (int)((day_week - now_wall_time.tm_wday) * 3600 * 24 + sec_start - (now_wall_time.wall_clock.tm_hour * 3600 + now_wall_time.wall_clock.tm_min * 60 + now_wall_time.wall_clock.tm_sec));

	if(sec_offset <= 0)
		sec_offset += 7*3600*24;

	if(sec_span == 0)
		sec_span = 1;
	
	//set rand work time  to reduce BSS stress
	xy_rtc_timer_create(timer_id, sec_offset+(xy_rand()%sec_span), callback, 0);
	return 1;
}

/*如果是北京时间，zone_sec赋值为(8*60*60)*/
bool Set_UT_by_ntp(char *ser_name, int timeout, int zone_sec)
{
	ntp_query_param_t arg = {0};

	arg.host = ser_name;
	arg.timeout = timeout - 1;
	/* 平台内部使用默认值20秒；因NB速率低，如果设置超时时间，建议该值不得小于20秒 */
	if (arg.timeout < 10)
		arg.timeout = 20;

	if (0 != query_ntp(&arg))
	{
		xy_printf(0,PLATFORM, WARN_LOG, "error!set rtc by ntp,fail get time!");
		return 0;
	}

	reset_universal_timer(&arg.rtctime, zone_sec);

	return 1;
}

//19/03/30,09:28:56+32---->xy_wall_clock+zone
int convert_wall_time(char *date, char *time, xy_wall_clock_t *wall_time, int *zone_sec)
{
	char *tag;
	char *next_tag;
	uint8_t month_table[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	tag = date;
	next_tag = strchr(tag, '/');
	if (next_tag == NULL)
		return XY_ERR;
	*next_tag = '\0';

	int temp_tag = (int)strtol(tag,NULL,10);

#if VER_260Y
	if (temp_tag >= 2000 && temp_tag < 2100)
		wall_time->tm_year = temp_tag;
#else
	if (temp_tag >= 0 && temp_tag < 70)
		wall_time->tm_year = 2000 + temp_tag;
	else if (temp_tag >= 70 && temp_tag <= 99)
		wall_time->tm_year = 1900 + temp_tag;
#endif
	else
		return XY_ERR;

	//闰年
	if ((0 == wall_time->tm_year % 4 && 0 != wall_time->tm_year % 100) || (0 == wall_time->tm_year % 400))
		month_table[2] = 29;
	
	tag = next_tag + 1;
	next_tag = strchr(tag, '/');
	if (next_tag == NULL)
		return XY_ERR;
	*next_tag = '\0';
	wall_time->tm_mon = (int)strtol(tag,NULL,10);

	if (wall_time->tm_mon > 12 || wall_time->tm_mon <= 0)
		return XY_ERR;

	tag = next_tag + 1;
	wall_time->tm_mday = (int)strtol(tag,NULL,10);

	if (wall_time->tm_mday <= 0 || wall_time->tm_mday > month_table[wall_time->tm_mon])
		return XY_ERR;

	tag = time;
	next_tag = strchr(tag, ':');
	if (next_tag == NULL)
		return XY_ERR;
	*next_tag = '\0';
	wall_time->tm_hour = (int)strtol(tag,NULL,10);

	if (wall_time->tm_hour >= 24)
		return XY_ERR;

	tag = next_tag + 1;
	next_tag = strchr(tag, ':');
	if (next_tag == NULL)
		return XY_ERR;
	*next_tag = '\0';
	wall_time->tm_min = (int)strtol(tag,NULL,10);
	if (wall_time->tm_min >= 60)
		return XY_ERR;

	tag = next_tag + 1;

	wall_time->tm_sec = (int)strtol(tag, NULL, 10);
	if (wall_time->tm_sec >= 60)
		return XY_ERR;

	//+zone
	if (((next_tag = strchr(tag, '+')) != NULL) || ((next_tag = strchr(tag, '-')) != NULL))
	{
		char zone[4] = {0};

		memcpy(zone, next_tag, 3);

		if (next_tag[0] == '+')
		{
#if VER_BC95
            if ((int)strtol(next_tag + 1, NULL, 10) > 96)
                return XY_ERR;
#else
            if ((int)strtol(next_tag + 1, NULL, 10) > 48)
                return XY_ERR;
#endif
            *zone_sec = ((int)strtol(next_tag + 1, NULL, 10)) * 15 * 60;
        }
        else if (next_tag[0] == '-')
        {
#if VER_BC95
            if ((int)strtol(next_tag + 1, NULL, 10) > 96)
                return XY_ERR;
#else
            if ((int)strtol(next_tag + 1, NULL, 10) > 48)
                return XY_ERR;
#endif
            *zone_sec = 0 - ((int)strtol(next_tag + 1, NULL, 10)) * 15 * 60;

        }
		g_softap_var_nv->g_zone = (int)strtol(zone, NULL, 10);
    }
#if !VER_BC25
	//not +zone
	else
	{
		return XY_ERR;
	}
#endif
	return XY_OK;
}