#if VER_BC25
#include "xy_at_api.h"
#include "factory_nv.h"
#include "softap_nv.h"
#include "oss_nv.h"
#include "xy_rtc_api.h"
#include "xy_lpm.h"

void save_at_andw_params(void)
{
	g_softap_fac_nv->echo_mode = g_softap_var_nv->echo_mode;
	g_softap_fac_nv->cmee_mode = g_softap_var_nv->cmee_mode;
	g_softap_fac_nv->wakup_URC = g_softap_var_nv->wakup_URC;
	SAVE_SOFTAP_FAC();
}

// AT&W[n]
int at_ANDW_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_ACTIVE)
	{
		save_at_andw_params();
		return AT_FORWARD;
	}
	else if (g_req_type == AT_CMD_REQ)
	{
		int n = 0;

		if (at_parse_param("%d", at_buf, &n) != AT_OK || n != 0)
		{
			return (ATERR_PARAM_INVALID);
		}
		save_at_andw_params();
		return AT_FORWARD;
	}
	else
		return (ATERR_PARAM_INVALID);
}

int at_CCLK_BC25_req(char *at_buf, char **prsp_cmd)
{
	int zone_sec = 0;
	if (g_req_type == AT_CMD_REQ)
	{
		char date[16] = {0};
		char time[16] = {0};
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
		RTC_TimeTypeDef rtctime = {0};
		if (g_softap_var_nv->wall_time_ms == 0)
		{
			rtc_timer_read(&rtctime);
		}
		else
		{
			get_universal_timer(&rtctime, 0);
		}

		int tm_year_diff = rtctime.wall_clock.tm_year - 2000;

		if (tm_year_diff >= 0)
			sprintf(*prsp_cmd, "\"%02d/%02lu/%02lu,%02lu:%02lu:%02lu\"", tm_year_diff, rtctime.wall_clock.tm_mon, rtctime.wall_clock.tm_mday,
					rtctime.wall_clock.tm_hour, rtctime.wall_clock.tm_min, rtctime.wall_clock.tm_sec);
		else
			sprintf(*prsp_cmd, "\"%02d/%02lu/%02lu,%02lu:%02lu:%02lu\"", 100 + tm_year_diff, rtctime.wall_clock.tm_mon, rtctime.wall_clock.tm_mday,
					rtctime.wall_clock.tm_hour, rtctime.wall_clock.tm_min, rtctime.wall_clock.tm_sec);
	}
	else if (g_req_type == AT_CMD_ACTIVE)
	{
		return ATERR_PARAM_INVALID;
	}
	return AT_END;
}


int at_QCCLK_req(char *at_buf, char **prsp_cmd)
{
	int zone_sec = 0;
	if (g_req_type == AT_CMD_REQ)
	{
		char date[16] = {0};
		char time[16] = {0};
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
		RTC_TimeTypeDef rtctime = {0};
		char zone[4] = {0};
		char positive_zone;

		if (g_softap_var_nv->g_zone >= 0)
		{
			positive_zone = g_softap_var_nv->g_zone;
			sprintf(zone, "+%d", positive_zone);
		}
		else
		{
			positive_zone = 0 - g_softap_var_nv->g_zone;
			sprintf(zone, "-%d", positive_zone);
		}

		if (g_softap_var_nv->wall_time_ms == 0)
		{
			rtc_timer_read(&rtctime);
		}
		else
		{
			if (g_softap_var_nv->g_zone != 0)
			{
				zone_sec = (int)g_softap_var_nv->g_zone * 15 * 60;
				get_universal_timer(&rtctime, zone_sec);
			}
			else
			{
				get_universal_timer(&rtctime, 0);
			}
		}

		int tm_year_diff = rtctime.wall_clock.tm_year - 2000;

		if (tm_year_diff >= 0)
			sprintf(*prsp_cmd, "\"%02d/%02lu/%02lu,%02lu:%02lu:%02lu%s\"", tm_year_diff, rtctime.wall_clock.tm_mon, rtctime.wall_clock.tm_mday,
					rtctime.wall_clock.tm_hour, rtctime.wall_clock.tm_min, rtctime.wall_clock.tm_sec, zone);
		else
			sprintf(*prsp_cmd, "\"%02d/%02lu/%02lu,%02lu:%02lu:%02lu%s\"", 100 + tm_year_diff, rtctime.wall_clock.tm_mon, rtctime.wall_clock.tm_mday,
					rtctime.wall_clock.tm_hour, rtctime.wall_clock.tm_min, rtctime.wall_clock.tm_sec, zone);
	}
	else if (g_req_type == AT_CMD_ACTIVE)
	{
		return ATERR_PARAM_INVALID;
	}
	return AT_END;
}

int at_QSCLK_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		// uint8_t mode = 0;

		// if (at_parse_param("%1d(0-2)", at_buf, &mode) != AT_OK)
		// {
		// 	return (ATERR_PARAM_INVALID);
		// }
		// if (g_softap_fac_nv->qsclk_mode != mode)
		// {
		// 	if (mode == 0)
		// 	{
		// 		g_softap_var_nv->qsclk_mode = mode;
		// 		sleep_lock(g_qsclklock_fd, LPM_ALL);
		// 	}
		// 	else if (g_softap_fac_nv->qsclk_mode == 0)
		// 	{
		// 		g_softap_var_nv->qsclk_mode = mode;
		// 		if (mode == 1)
		// 			sleep_unlock(g_qsclklock_fd, LPM_ALL);
		// 		else
		// 			sleep_unlock(g_qsclklock_fd, LPM_STANDBY);
		// 	}
		// 	g_softap_fac_nv->qsclk_mode = mode;
		// 	SAVE_FAC_PARAM(qsclk_mode);
		// }
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		// *prsp_cmd = xy_malloc(32);

		// snprintf(*prsp_cmd, 32, "\r\n+QSCLK:%d\r\n\r\nOK\r\n", g_softap_fac_nv->qsclk_mode);
	}
#if (AT_CUT != 1)
	else if (g_req_type == AT_CMD_TEST)
	{
		// *prsp_cmd = xy_malloc(32);

		// snprintf(*prsp_cmd, 32, "\r\n+QSCLK:(0-2)\r\n\r\nOK\r\n");
	}
#endif
	else
	{
		return ATERR_PARAM_INVALID;
	}
	return AT_END;
}

int at_QATWAKEUP_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		uint8_t wakeup_mode = 0;

		if (at_parse_param("%1d(0-1)", at_buf, &wakeup_mode) != AT_OK)
			return ATERR_PARAM_INVALID;
		g_softap_var_nv->wakup_URC = wakeup_mode;
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(32);
		snprintf(*prsp_cmd, 32, "%d", g_softap_var_nv->wakup_URC);
	}
#if (AT_CUT != 1)
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(32);
		snprintf(*prsp_cmd, 32, "\r\n+QATWAKEUP: (0,1)\r\n\r\nOK\r\n");
	}
#endif
	else
		return ATERR_PARAM_INVALID;
	return AT_END;
}


//AT+QPOWD=
int at_QPOWD_req(char *at_buf,char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		int abnormal_off = 0;
		if (at_parse_param("%d(0-1)", at_buf, &abnormal_off) != AT_OK)
		{
			return  (ATERR_PARAM_INVALID);
		}
		else
		{
			xy_fast_power_off();
			g_softap_var_nv->powerdown_flag = 1;
			if (abnormal_off == 1)
			{
				*prsp_cmd = xy_malloc(32);
				snprintf(*prsp_cmd, 32, "\r\nNORMAL POWER DOWN\r\n");
			}
		}
	}
	else
		return  (ATERR_PARAM_INVALID);
	return AT_END;
}
#endif