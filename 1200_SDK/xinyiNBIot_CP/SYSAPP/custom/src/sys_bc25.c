
#if VER_BC25

#include "xy_at_api.h"
#include "at_com.h"
#include "xy_system.h"
#include "softap_nv.h"
#include "at_ctl.h"
#include "rtc_tmr.h"
#include "low_power.h"


void Sys_Up_URC_25()
{
	char at_str[32] = {0};
	if (!is_urc_drop())
	{
		if (Is_WakeUp_From_Dsleep())
		{
			if (g_softap_var_nv->powerdown_flag == 0 && g_softap_var_nv->wakup_URC == 1)
				snprintf(at_str, 32, "\r\n+QATWAKEUP\r\n");
			else if (g_softap_var_nv->powerdown_flag == 1)
			{
				snprintf(at_str, 32, "\r\nRDY\r\n");
				g_softap_var_nv->powerdown_flag = 0;
			}
		}
		else
			snprintf(at_str, 32, "\r\nRDY\r\n");

		uint8_t str_len = strlen(at_str);
		if(str_len!=0)
			send_urc_to_ext_NoCache(at_str, str_len);
	}
}

extern void send_powerdown_urc_to_ext(void *buf, uint32_t size);
void Sys_Down_URC_25(void)
{
	if(is_urc_drop())
		return;
	if(g_softap_var_nv->powerdown_flag == 0 && g_softap_var_nv->wakup_URC == 1)
	{
		char at_str[32] = {0};
		snprintf(at_str, 32, "\r\n+QATSLEEP\r\n");
		send_powerdown_urc_to_ext(at_str, strlen(at_str));
	}
}

#endif
