
#if VER_260Y

#include "xy_at_api.h"
#include "at_com.h"
#include "xy_system.h"
#include "softap_nv.h"
#include "at_ctl.h"
#include "rtc_tmr.h"
#include "low_power.h"

void Sys_Up_URC_260()
{
	char at_str[40] = {0};
	if (!is_urc_drop())
	{
		if (Is_WakeUp_From_Dsleep())
		{
			if (g_softap_fac_nv->off_dsevent == 0)
				snprintf(at_str, 40, "\r\n+QNBIOTEVENT: \"EXIT DEEPSLEEP\"\r\n");
		}
		else
			snprintf(at_str, 40, "\r\nRDY\r\n");

		uint8_t str_len = strlen(at_str);
		if(str_len!=0)
			send_urc_to_ext_NoCache(at_str, str_len);
	}
}

extern void send_powerdown_urc_to_ext(void *buf, uint32_t size);
void Sys_Down_URC_260(void)
{
	if(is_urc_drop())
		return;
	if(g_softap_fac_nv->off_dsevent == 0)
	{
		char at_str[40] = {0};
		snprintf(at_str, 40, "\r\n+QNBIOTEVENT: \"ENTER DEEPSLEEP\"\r\n");
		send_powerdown_urc_to_ext(at_str, strlen(at_str));
	}
}
#endif
