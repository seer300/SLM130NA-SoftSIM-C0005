
#include "xy_at_api.h"
#include "factory_nv.h"
#include "softap_nv.h"
#include "oss_nv.h"
#include "xy_rtc_api.h"
#include "at_worklock.h"


uint8_t g_qslck_mode = 1;

int at_QCFG_260Y_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		uint8_t sub_param[20] = {0};
		int32_t value = -1;
		if (at_parse_param("%20s,%d", at_buf, sub_param, &value) != AT_OK || strlen(sub_param) == 0)
			return (ATERR_PARAM_INVALID);

		if (at_strcasecmp(sub_param, "logbaudrate"))
		{
			uint32_t log_rate_bund[] = {921600,9600,19200,38400,57600,115200,380400,460800};
			if (value == -1)
			{
				*prsp_cmd = xy_malloc(48);
				snprintf(*prsp_cmd, 48, "\"logbaudrate\",%ld", log_rate_bund[g_softap_fac_nv->log_rate]);
			}
			else
			{
				uint8_t index = 0;
				for (; index < sizeof(log_rate_bund) / sizeof(uint32_t); index++)
				{
					if (value == log_rate_bund[index])
					{
						g_softap_fac_nv->log_rate = index;
						SAVE_FAC_PARAM(log_rate);
						return AT_END;
					}
				}
				return ATERR_PARAM_INVALID;
			}
		}
		else if (at_strcasecmp(sub_param, "slplocktimes"))
		{
			if (value == -1)
			{
				*prsp_cmd = xy_malloc(48);
				snprintf(*prsp_cmd, 48, "\"slplocktimes\",%d", g_softap_fac_nv->deepsleep_delay);
			}
			else if (value >= 0 && value <= 30)
			{
				g_softap_fac_nv->deepsleep_delay = value;
				SAVE_FAC_PARAM(deepsleep_delay);
			}
			else
				return ATERR_PARAM_INVALID;
		}
		else if (at_strcasecmp(sub_param, "dsevent"))
		{
			if (value == -1)
			{
				*prsp_cmd = xy_malloc(48);
				snprintf(*prsp_cmd, 48, "\"dsevent\",%d", !g_softap_fac_nv->off_dsevent);
			}
			else if(value == 0 || value == 1)
			{
				g_softap_fac_nv->off_dsevent = !value;
				SAVE_FAC_PARAM(off_dsevent);
			}
			else
				return ATERR_PARAM_INVALID;
		}
		else if (at_strcasecmp(sub_param, "wakeupRXD"))
		{
			if (value == -1)
			{
				*prsp_cmd = xy_malloc(48);
				snprintf(*prsp_cmd, 48, "\"wakeupRXD\",%d", !g_softap_fac_nv->off_wakeupRXD);
			}
			else if(value == 0 || value == 1)
			{
				g_softap_fac_nv->off_wakeupRXD = !value;
				SAVE_FAC_PARAM(off_wakeupRXD);
				return AT_END;
			}
			else
				return ATERR_PARAM_INVALID;
		}
		else
			return AT_FORWARD;
	}
	else if (g_req_type == AT_CMD_QUERY || g_req_type == AT_CMD_TEST)
	{
		return AT_FORWARD;
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
	return AT_END;
}

int at_QSCLK_260Y_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		uint8_t mode = 0;

		if (at_parse_param("%1d(0-2)", at_buf, &mode) != AT_OK)
		{
			return (ATERR_PARAM_INVALID);
		}
		set_qsclk_lock(mode);
		g_qslck_mode = mode;
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(32);
		snprintf(*prsp_cmd, 32, "%d", g_qslck_mode);
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


// AT+CCLK=<yy/MM/dd,hh:mm:ss>[<±zz>]    as 19/03/30,09:28:56+32
// AT+CCLK?   +CCLK:[<yy/MM/dd,hh:mm:ss>[<±zz>]]   19/03/30,09:28:56+32
int at_CCLK_260Y_req(char *at_buf, char **prsp_cmd)
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
		*prsp_cmd = xy_malloc(60);

		int zone_sec = 0;
		RTC_TimeTypeDef rtctime = {0};
		char zone[4] = {0};
		char positive_zone;

		if (g_softap_var_nv->g_zone >= 0)
		{
			positive_zone = g_softap_var_nv->g_zone;
			sprintf(zone, "+%2d", positive_zone);
		}
		else
		{
			positive_zone = 0 - g_softap_var_nv->g_zone;
			sprintf(zone, "-%2d", positive_zone);
		}

		zone_sec = (int)g_softap_var_nv->g_zone * 15 * 60;
		if(get_universal_timer(&rtctime, zone_sec) == 0)
			rtc_timer_read(&rtctime);

		if (g_softap_var_nv->g_zone == 0)
			sprintf(*prsp_cmd, "\"%04d/%02lu/%02lu,%02lu:%02lu:%02lu+00\"", rtctime.wall_clock.tm_year, rtctime.wall_clock.tm_mon, rtctime.wall_clock.tm_mday,
					rtctime.wall_clock.tm_hour, rtctime.wall_clock.tm_min, rtctime.wall_clock.tm_sec);
		else
			sprintf(*prsp_cmd, "\"%04d/%02lu/%02lu,%02lu:%02lu:%02lu%s\"", rtctime.wall_clock.tm_year, rtctime.wall_clock.tm_mon, rtctime.wall_clock.tm_mday,
					rtctime.wall_clock.tm_hour, rtctime.wall_clock.tm_min, rtctime.wall_clock.tm_sec, zone);
	}
	else if (g_req_type == AT_CMD_ACTIVE)
	{
		return ATERR_PARAM_INVALID;
	}
	return AT_END;
}
