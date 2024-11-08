#if VER_BC25
/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#include "at_hardware_cmd.h"
#include "xy_at_api.h"
#include "xy_system.h"
#include "oss_nv.h"
#include "xy_utils.h"
#include "at_com.h"
#include "at_uart.h"
#include "low_power.h"
#include "gpio.h"
#include "adc.h"

/**
  * @brief  ADC测试相关的AT命令处理函数
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  */

int at_ADC_BC25_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(40);
		snprintf(*prsp_cmd, 40, "\r\n+QADC: 1,%d\r\n\r\nOK\r\n",get_adc_value_incpcore(ADC_AUX_ADC2));
	}
#if (AT_CUT != 1)
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(40);
		snprintf(*prsp_cmd, 40, "\r\n+QADC: (0,1),(0-4500)\r\n\r\nOK\r\n");
	}
#endif
	else
	{
		return ATERR_PARAM_INVALID;
	}

	return AT_END;
}

/**
  * @brief  查询电源电压值相关的AT命令处理函数
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  */

int at_CBC_BC25_req(char *at_buf, char **prsp_cmd)
{
	uint32_t vol;
	uint32_t bcs = 0;	//电池状态
	uint32_t bcl = 0;	//电池电量百分比

	if (g_req_type == AT_CMD_ACTIVE)
	{
		*prsp_cmd = xy_malloc(50);
		vol = xy_getVbat();
		snprintf(*prsp_cmd, 50, "%d,%d,%d",bcs,bcl,vol);
	}
#if (AT_CUT != 1)
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(50);
		snprintf(*prsp_cmd, 50, "(0-5),(0-100),<voltage>");
	}
#endif
	else
	{
		return ATERR_PARAM_INVALID;
	}

	return AT_END;
}


/**
  * @brief  +QLEDMODE   
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * 
  * @attention 1、此指令用于LED网络指示灯启动与关闭，具体闪烁规则请参考net_led_proc.c。
  * 		   2、此代码中GPIO9为示例使用，具体请根据硬件设计调整。
  * 		   3、开关LED灯，重启生效。
*/

extern void net_led_stop(void);
int at_LED_BC25_req(char *at_buf, char **prsp_cmd)
{
	volatile int32_t led_enable = -1;
	if (g_req_type == AT_CMD_REQ)
	{
		if (at_parse_param("%d(0-1)", at_buf, &led_enable) != AT_OK)
			return ATERR_PARAM_INVALID;

		if (led_enable == 0)  //关闭LED网络指示灯
		{
			net_led_stop();
			g_softap_fac_nv->led_pin = 0xFF;
			SAVE_FAC_PARAM(led_pin);
		}
		else//打开LED网络指示灯
		{
			g_softap_fac_nv->led_pin = GPIO_PAD_NUM_9; 
			SAVE_FAC_PARAM(led_pin);
		}
	}
#if (AT_CUT != 1)
	else if (g_req_type == AT_CMD_TEST) //AT+XXX=?
	{
		*prsp_cmd = xy_malloc(30);
		snprintf(*prsp_cmd, 30, "+QLEDMODE: (0,1)");
	}
#endif
	else if (g_req_type == AT_CMD_QUERY) //AT+XXX?
	{
		*prsp_cmd = xy_malloc(30);
		snprintf(*prsp_cmd, 30, "+QLEDMODE: %d",((g_softap_fac_nv != NULL) && (g_softap_fac_nv->led_pin <= GPIO_PAD_NUM_63)) ? 1 : 0);
	}

	return AT_END;
}
#endif