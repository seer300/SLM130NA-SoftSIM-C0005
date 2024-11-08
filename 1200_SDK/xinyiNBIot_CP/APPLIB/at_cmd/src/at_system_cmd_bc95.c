#if VER_BC95
/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_system_cmd.h"
#include "xy_at_api.h"
#include "factory_nv.h"
#include "oss_nv.h"
#include "softap_nv.h"
#include "low_power.h"
#include "xy_rtc_api.h"
#include "xy_system.h"
#include "xy_flash.h"
#include "csp.h"
#include "at_uart.h"
#include "at_com.h"
#include "xy_fs.h"

/*******************************************************************************
 *						Global function implementations 					   *
 ******************************************************************************/
// AT+NITZ=<mode>[,<save>]
int at_NITZ_BC95_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		int temp = 0;

		if (at_parse_param("%d(0-1)", at_buf, &temp) != AT_OK)
		{
			return (ATERR_PARAM_INVALID);
		}

		g_NITZ_mode = temp;
		g_softap_var_nv->g_NITZ = g_softap_fac_nv->g_NITZ = temp;
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(20);
		sprintf(*prsp_cmd, "+NITZ:%d", g_NITZ_mode);
	}
#if (AT_CUT != 1)
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(32);
		snprintf(*prsp_cmd, 32, "+NITZ:(0,1)");
	}
#endif
	else
		return (ATERR_PARAM_INVALID);
	return AT_END;
}

// AT+CCLK=<yy/MM/dd,hh:mm:ss>[<±zz>]    as 19/03/30,09:28:56+32
// AT+CCLK?   +CCLK:[<yy/MM/dd,hh:mm:ss>[<±zz>]]   19/03/30,09:28:56+32
int at_CCLK_BC95_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{

		char date[16] = {0};
		char time[16] = {0};
		int zone_sec = 0;
		xy_wall_clock_t wall_time = {0};

		if (at_parse_param("%s,%s", at_buf, date, time) == AT_OK)
		{
			if (date[0] == 0 || time[0] == 0)
			{
				return ATERR_PARAM_INVALID;
			}

			if (g_NITZ_mode == 1)
			{
				return ATERR_NOT_ALLOWED;
			}

			if (convert_wall_time(date, time, &wall_time, &zone_sec) == XY_OK)
				Set_UT_Time(&wall_time, zone_sec);
			else
				return (ATERR_PARAM_INVALID);
		}
		else
			return (ATERR_PARAM_INVALID);
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		// if never attached,now_wall_time is local time,not UT time,maybe is 2018/10/1
		if (g_softap_var_nv->wall_time_ms == 0)
		{
			return AT_END;
		}

		*prsp_cmd = xy_malloc(60);

		int zone_sec = 0;
		RTC_TimeTypeDef rtctime = {0};
		char zone[4] = {0};
		char positive_zone;

		if (g_softap_var_nv->g_zone >= 0)
		{
			positive_zone = g_softap_var_nv->g_zone;
			if (g_softap_var_nv->g_zone >= 10)
			{
				sprintf(zone, "+%2d", positive_zone);
			}
			else
			{
				sprintf(zone, "+%02d", positive_zone);
			}
		}
		else
		{
			positive_zone = 0 - g_softap_var_nv->g_zone;
			if (g_softap_var_nv->g_zone <= -10)
			{
				sprintf(zone, "-%2d", positive_zone);
			}
			else
			{
				sprintf(zone, "-%02d", positive_zone);
			}
		}
		if (g_softap_var_nv->g_zone != 0)
		{
			zone_sec = (int)g_softap_var_nv->g_zone * 15 * 60;
			get_universal_timer(&rtctime, zone_sec);
		}
		else
		{
			get_universal_timer(&rtctime, 0);
		}
		int tm_year_diff = rtctime.wall_clock.tm_year - 2000;
		if (g_softap_var_nv->g_zone == 0)
		{
			if (tm_year_diff >= 0)
				sprintf(*prsp_cmd, "+CCLK:%02d/%02lu/%02lu,%02lu:%02lu:%02lu+00", tm_year_diff, rtctime.wall_clock.tm_mon, rtctime.wall_clock.tm_mday,
						rtctime.wall_clock.tm_hour, rtctime.wall_clock.tm_min, rtctime.wall_clock.tm_sec);
			else
				sprintf(*prsp_cmd, "+CCLK:%02d/%02lu/%02lu,%02lu:%02lu:%02lu+00", 100 + tm_year_diff, rtctime.wall_clock.tm_mon, rtctime.wall_clock.tm_mday,
						rtctime.wall_clock.tm_hour, rtctime.wall_clock.tm_min, rtctime.wall_clock.tm_sec);
		}
		else
		{
			if (tm_year_diff >= 0)
				sprintf(*prsp_cmd, "+CCLK:%02d/%02lu/%02lu,%02lu:%02lu:%02lu%s", tm_year_diff, rtctime.wall_clock.tm_mon, rtctime.wall_clock.tm_mday,
						rtctime.wall_clock.tm_hour, rtctime.wall_clock.tm_min, rtctime.wall_clock.tm_sec, zone);
			else
				sprintf(*prsp_cmd, "+CCLK:%02d/%02lu/%02lu,%02lu:%02lu:%02lu%s", 100 + tm_year_diff, rtctime.wall_clock.tm_mon, rtctime.wall_clock.tm_mday,
						rtctime.wall_clock.tm_hour, rtctime.wall_clock.tm_min, rtctime.wall_clock.tm_sec, zone);
		}
	}
	else if (g_req_type == AT_CMD_ACTIVE)
	{
		return ATERR_PARAM_INVALID;
	}
	return AT_END;
}

int at_QSCLK_BC95_req(char *at_buf, char **prsp_cmd)
{
	static uint8_t qslck_mode = 1;
	if (g_req_type == AT_CMD_REQ)
	{
		uint8_t mode = 0;

		if (at_parse_param("%1d(0-2)", at_buf, &mode) != AT_OK)
		{
			return (ATERR_PARAM_INVALID);
		}
		set_qsclk_lock(mode);
		qslck_mode = mode;
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(32);
		snprintf(*prsp_cmd, 32, "%d", qslck_mode);
	}
#if (AT_CUT != 1)
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(32);
		snprintf(*prsp_cmd, 32, "(0-2)");
	}
#endif
	else
	{
		return ATERR_PARAM_INVALID;
	}
	return AT_END;
}
#endif