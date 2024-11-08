#if VER_260Y
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

int at_ADC_260Y_req(char *at_buf, char **prsp_cmd)
{
	 if (g_req_type == AT_CMD_ACTIVE)
	{
		*prsp_cmd = xy_malloc(40);
		snprintf(*prsp_cmd, 40, "0,%d", get_adc_value_incpcore(0));
	}
	else if (g_req_type == AT_CMD_REQ)
	{
		int32_t channel = 0;
		if (at_parse_param("%d(0-12)", at_buf, &channel) != AT_OK)
			return ATERR_PARAM_INVALID;

		*prsp_cmd = xy_malloc(40);
		snprintf(*prsp_cmd, 40, "%d,%d", channel, get_adc_value_incpcore(channel));
	}
#if (AT_CUT != 1)
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(40);
		snprintf(*prsp_cmd, 40, "(0-12)");
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

int at_CBC_260Y_req(char *at_buf, char **prsp_cmd)
{
	uint16_t vol;

	if (g_req_type == AT_CMD_ACTIVE)
	{
		*prsp_cmd = xy_malloc(50);
		vol = xy_getVbat();
		snprintf(*prsp_cmd, 50, "%d", vol);
	}
#if (AT_CUT != 1)
	else if(g_req_type != AT_CMD_TEST)
	{
		return ATERR_PARAM_INVALID;
	}
#endif
	return AT_END;
}
#endif