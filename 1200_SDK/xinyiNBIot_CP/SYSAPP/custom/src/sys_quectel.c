#if VER_BC95

#include "xy_at_api.h"
#include "at_com.h"
#include "xy_system.h"
#include "softap_nv.h"
#include "at_ctl.h"
#include "rtc_tmr.h"
#include "low_power.h"


/*1表示进入PSM状态，含OSS等异常，需要上报URC*/
int g_npsmr_status = 0; 

void Sys_Up_URC_95()
{
    char *at_str = xy_malloc(64);

    if (!Is_WakeUp_From_Dsleep())
    {
        snprintf(at_str, 64, "\r\n%s\r\nNeul\r\nOK\r\n", at_get_power_on_string());
        send_urc_to_ext_NoCache(at_str, strlen(at_str));
    }

	xy_free(at_str);

	//异常深睡唤醒均上报NPSMR0
    if (Is_WakeUp_From_Dsleep() && g_softap_var_nv->ps_deepsleep_state == 0)
    {
        if(g_softap_fac_nv->g_NPSMR_enable == 1 && g_npsmr_status == 1)
            send_urc_to_ext("+NPSMR:0", strlen("+NPSMR:0"));

        g_npsmr_status = 0;
    }

	xy_printf(0,PLATFORM, WARN_LOG, "+DBGINFO:BootR %d,SubR 0x%x,PS state %d,REG %x %x\r\n", Get_Boot_Reason(), Get_Boot_Sub_Reason(),g_softap_var_nv->ps_deepsleep_state,HWREG(0x40000000),HWREG(0x40000008));
}

extern T_LPM_INFO Ps_Lpminfo;
extern void send_powerdown_urc_to_ext(void *buf, uint32_t size);
void Sys_Down_URC_95(void)
{
    //非edrx和cfun0的深睡，上报NPSMR1
    if(g_softap_var_nv->ps_deepsleep_state != 2 && g_softap_var_nv->ps_deepsleep_state != 3)
    {
        if(g_softap_fac_nv->g_NPSMR_enable == 1 && g_npsmr_status == 0)
        {
           send_powerdown_urc_to_ext("\r\n+NPSMR:1\r\n", strlen("\r\n+NPSMR:1\r\n"));
        }
        g_npsmr_status = 1;
    }

    if(HWREGB(BAK_MEM_XY_DUMP) == 1)
    {
        uint64_t rtc_msec_cur;
        uint64_t next_rtc_ms;
        char *sys_down_str = NULL;
        rtc_msec_cur = get_utc_tick();

        if(HWREGB(BAK_MEM_CP_SET_RTC) == 0)
        {
            next_rtc_ms = RTC_ALARM_INVALID;
        }
        else
        {
            next_rtc_ms = Get_CP_ALARM_RAM();
        }

        if (g_softap_var_nv->ps_deepsleep_state == 2)
        {
            send_debug_by_at_uart("+DBGINFO:POWERDOWN drx/edrx\r\n");
            return;
        }

        sys_down_str = xy_malloc(48);
        if(next_rtc_ms != 0 && next_rtc_ms != RTC_ALARM_INVALID)
        {
            uint32_t next_rtc_sec = (uint32_t)(CONVERT_RTCTICK_TO_MS((Transform_Num64_To_Ms(next_rtc_ms) - rtc_msec_cur) / 1000));
            if(g_softap_var_nv->next_PS_sec==0xFFFFFFFF)
            {
                sprintf(sys_down_str,"+DBGINFO:POWERDOWN:-1,%lu\r\n", next_rtc_sec);
            }
            else if(g_softap_var_nv->next_PS_sec!=0 && g_softap_var_nv->next_PS_sec > rtc_msec_cur / configTICK_RATE_HZ)
            {
                sprintf(sys_down_str,"+DBGINFO:POWERDOWN:%lu,%lu\r\n",(uint32_t)(CONVERT_RTCTICK_TO_MS(g_softap_var_nv->next_PS_sec-rtc_msec_cur/configTICK_RATE_HZ)), next_rtc_sec);
            }
            else
            {
                sprintf(sys_down_str,"+DBGINFO:POWERDOWN:0,%lu\r\n", next_rtc_sec);
            }
        }
        else
        {
            if(g_softap_var_nv->next_PS_sec==0xFFFFFFFF)
            {
                sprintf(sys_down_str,"+DBGINFO:POWERDOWN:-1,-1\r\n");
            }
            else if(g_softap_var_nv->next_PS_sec!=0 && g_softap_var_nv->next_PS_sec > rtc_msec_cur / configTICK_RATE_HZ )
            {
                sprintf(sys_down_str,"+DBGINFO:POWERDOWN:%lu,-1\r\n",(uint32_t)(CONVERT_RTCTICK_TO_MS(g_softap_var_nv->next_PS_sec-rtc_msec_cur/configTICK_RATE_HZ)));
            }
            else
            {
                sprintf(sys_down_str,"+DBGINFO:POWERDOWN:0,-1\r\n");
            }
        }

		xy_printf(0, PLATFORM, WARN_LOG,"URC: %s",sys_down_str);
		
        if(!is_urc_drop())
			send_powerdown_urc_to_ext(sys_down_str, strlen(sys_down_str));
		
        xy_free(sys_down_str);

    }

}

#endif /* VER_BC95 */
