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

/*REBOOT期间仅上报REBOOTING，不上报普通的AT命令*/
int g_Rebooting = 0;

/*******************************************************************************
 *						Global function implementations 					   *
 ******************************************************************************/
//AT+CPOF
int at_CPOF_req(char *at_buf,char **prsp_cmd)
{
	if (g_req_type == AT_CMD_ACTIVE)
	{
		xy_fast_power_off();
	}
	else if (g_req_type == AT_CMD_REQ)
	{
		int abnormal_off = 0;
		if (at_parse_param("%d(0-2)", at_buf, &abnormal_off) != AT_OK)
		{
			return  (ATERR_PARAM_INVALID);
		}

		else
		{
			xy_fast_power_off();
		}
	}
#if (AT_CUT != 1)
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(40);

		snprintf(*prsp_cmd, 40, "+CPOF:(0,1)");
	}
#endif
	else
		return  (ATERR_PARAM_INVALID);
	return AT_END;
}

/*AT+ASSERTCP=<val>*/
int at_ASSERT_req(char *at_buf, char **prsp_cmd)
{
	int cmd = 0;
	xy_printf(0,XYAPP, WARN_LOG, "TEST ASSERT AND DUMP!");
	if(g_req_type == AT_CMD_ACTIVE)
	{
		xy_assert(0);
	}
	else if (g_req_type == AT_CMD_REQ)
	{
		at_parse_param("%d,", at_buf, &cmd);
		
		if(cmd == 0)
		{
			xy_assert(0);
		}
		/*人为构造hardfault*/
		else
		{
			return ((ser_req_func)(g_req_type))(NULL,NULL);
		}
	}
	else
		return  (ATERR_PARAM_INVALID);

	return AT_END;
	
}

//AT+NITZ=<mode>[,<save>]
int at_NITZ_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		int temp = 0;
		int save_NITZ = 1;

		if (at_parse_param("%d(0-1),%d[0-1]", at_buf, &temp, &save_NITZ) != AT_OK)
		{
			return  (ATERR_PARAM_INVALID);
		}

		g_NITZ_mode = temp;

		//if (save_NITZ == 1)
		{
			g_softap_fac_nv->g_NITZ = g_NITZ_mode;
			SAVE_FAC_PARAM(g_NITZ);	
		}
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
		return  (ATERR_PARAM_INVALID);
	return AT_END;
}


//AT+CCLK=<yy/MM/dd,hh:mm:ss>[<±zz>]    as 19/03/30,09:28:56+32
//AT+CCLK?   +CCLK:[<yy/MM/dd,hh:mm:ss>[<±zz>]]   19/03/30,09:28:56+32
int at_CCLK_req(char *at_buf, char **prsp_cmd)
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
		//if never attached,now_wall_time is local time,not UT time,maybe is 2018/10/1
        if (g_softap_var_nv->wall_time_ms == 0)
        {
			return ATERR_NOT_ALLOWED;
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
                sprintf(zone, "+%1d", positive_zone);
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
                sprintf(zone, "-%1d", positive_zone);
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
                sprintf(*prsp_cmd, "+CCLK:%02d/%02lu/%02lu,%02lu:%02lu:%02lu+0", tm_year_diff, rtctime.wall_clock.tm_mon, rtctime.wall_clock.tm_mday,
                        rtctime.wall_clock.tm_hour, rtctime.wall_clock.tm_min, rtctime.wall_clock.tm_sec);
            else
                sprintf(*prsp_cmd, "+CCLK:%02d/%02lu/%02lu,%02lu:%02lu:%02lu+0", 100 + tm_year_diff, rtctime.wall_clock.tm_mon, rtctime.wall_clock.tm_mday,
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


/*AT+STANDBY*/
osTimerId_t g_off_standby_timer = NULL;

//AT+STANDBY 关闭standby超时回调
void off_standby_timer_callback(void)
{
	g_softap_fac_nv->lpm_standby_enable = 1;
	osTimerDelete(g_off_standby_timer);
	g_off_standby_timer = NULL;
}

/**
 * @brief 动态开关STANDBY
 * @note at_format :AT+STANDBY=<enable>[,time];
 * @note at_param explain:
 * enable:是否打开standby开关 1：打开 0：关闭 ;time:秒，关闭STANDY时间，仅在enable取值为0时有效
 * @return int 
 */
int at_STANDBY_req(char *at_buf, char **prsp_cmd)
{
	int32_t enable = 0;
	int32_t off_keep_time = -1;

	if (g_req_type == AT_CMD_REQ)
	{
		if(at_parse_param("%d(0|1),%d[0-2147483648]", at_buf, &enable, &off_keep_time) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		if(enable == 0 && off_keep_time < 0)
		{
			return ATERR_PARAM_INVALID;
		}

		if(enable == 0)
		{
			osTimerAttr_t timer_attr = {0};
			g_softap_fac_nv->lpm_standby_enable = 0;//close standby

			if (off_keep_time > 0)
			{
				if (g_off_standby_timer != NULL)
				{
					osTimerDelete(g_off_standby_timer);
					g_off_standby_timer = NULL;
				}

				timer_attr.name = "off_standby";
				g_off_standby_timer = osTimerNew((osTimerFunc_t)(off_standby_timer_callback), osTimerOnce, NULL, &timer_attr);

				osTimerStart(g_off_standby_timer, off_keep_time * 1000);
			}
		}
		else
		{
			g_softap_fac_nv->lpm_standby_enable = 1;//open standby
			if(g_off_standby_timer != NULL)
			{
				osTimerDelete(g_off_standby_timer);
				g_off_standby_timer = NULL;
			}
		}
	}
	else
		return ATERR_PARAM_INVALID;

	return AT_END;
}