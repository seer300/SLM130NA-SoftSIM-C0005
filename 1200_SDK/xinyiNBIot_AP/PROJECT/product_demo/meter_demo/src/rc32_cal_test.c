#include <string.h>
#include "xy_system.h"
#include "xy_timer.h"
#include "ap_watchdog.h"
#include "hal_adc.h"
#include "xy_printf.h"
#include "basic_config.h"
#include "at_uart.h"
#include "user_config.h"
#include "xy_event.h"
#include "mcu_adapt.h"

/************************************************************************************************************
***  重点测试RC32的周期性校准，考虑随机启停CP核的压力测试，也可以用于功耗开销测试。
*************************************************************************************************************/


__RAM_FUNC void XY_TIMER_rc32_cal_Cback(void)
{
    Set_RC32K_Cali_Event();
	Send_AT_to_Ext("XY_TIMER_rc32_cal_Cback arriverd\r\n");  
}


void set_rc32_cal_xy_timer(int period_sec)
{
	Timer_AddEvent(XY_TIMER_TEST, period_sec*1000, XY_TIMER_rc32_cal_Cback, 1);
}

__RAM_FUNC void Lptimer_rc32_cal_Cback(void)
{
    Set_RC32K_Cali_Event();
	Send_AT_to_Ext("Lptimer_rc32_cal_Cback arrived\r\n"); 
}

void set_rc32_cal_lptimer(int period_sec)
{
	McuLptimerSetPeriod(period_sec*1000);
	McuLptimerIrqReg(Lptimer_rc32_cal_Cback);
	McuLptimerEn();
}

/************************************************************************************
* @brief  周期性进行RC32校准，可以与CP启停压力测试。格式：                  AT+DEMOCFG=RC32,<timer_type>,<RC32 cal period sec>
* @param  val1: timer_type，定时器类型选择，0：xytimer,1:lptimer
* @param  val2：校准的周期时长设置，单位为秒
* @note   动态启停CP，可以用命令AT+DEMOCFG=BITMAP,1或AT+DEMOCFG=BITMAP,3
************************************************************************************/
int rc32_cal_test_set(char **prsp_cmd,int val1,int val2,int val3,int val4,int val5)
{
	UNUSED_ARG(prsp_cmd);
	UNUSED_ARG(val3);
	UNUSED_ARG(val4);
	UNUSED_ARG(val5);
	if(val1 == 0 && val2!=0)
	{
		set_rc32_cal_xy_timer(val2);
		xy_printf("\r\nrc32_cal_test_set,xy_timer cycle: %d\r\n", val2);
	}
	else if(val1 == 1 && val2!=0)
	{
		set_rc32_cal_lptimer(val2);
		xy_printf("\r\nrc32_cal_test_set,lptimer cycle: %d\r\n", val2);
	}
	else
	{
		xy_printf("\r\nrc32_cal_test_set  ERROR!\r\n");
		return XY_ERR_PARAM_INVALID;
	}
	return XY_OK;
}




