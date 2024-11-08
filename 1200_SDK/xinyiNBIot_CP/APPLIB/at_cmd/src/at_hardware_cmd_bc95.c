#if VER_BC95
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
//#include "hal_adc.h"
/**
  * @brief 芯片当前信息查询
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval int
  */
int at_CHIPINFO_BC95_req(char *at_buf, char **prsp_cmd)
{
	char cmd[5] = {0};

	if (g_req_type == AT_CMD_REQ)
	{
		if (at_parse_param("%5s", at_buf, cmd) != AT_OK)
			return ATERR_PARAM_INVALID;

		if (at_strcasecmp(cmd, "ALL")) //VBAT,TEMP
		{
			*prsp_cmd = xy_malloc(65);
			snprintf(*prsp_cmd, 65, "+QCHIPINFO:%s,%d\r\n+QCHIPINFO:%s,%d", "TEMP", xy_getempera(), "VBAT", xy_getVbat());
		}

		else if (at_strcasecmp(cmd, "TEMP")) //TEMP
		{
			*prsp_cmd = xy_malloc(40);
			snprintf(*prsp_cmd, 40, "+QCHIPINFO:%s,%d", "TEMP", xy_getempera());
		}

		else if (at_strcasecmp(cmd, "VBAT")) //VBAT
		{
			*prsp_cmd = xy_malloc(40);
			snprintf(*prsp_cmd, 40, "+QCHIPINFO:%s,%d", "VBAT", xy_getVbat());
		}

		else
			return ATERR_PARAM_INVALID;
	}
#if (AT_CUT != 1)
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(40);
		snprintf(*prsp_cmd, 40, "+QCHIPINFO:(%s,%s,%s)", "ALL", "TEMP", "VBAT");
	}
#endif
	return AT_END;
}


/**
  * @brief  ADC测试相关的AT命令处理函数
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  */

int at_ADC_BC95_req(char *at_buf, char **prsp_cmd)
{
	int adc_Mode = 0;
	int16_t voltageADC;


	if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(30);
		snprintf(*prsp_cmd, 30, "+QADC:(0,1)");
	}
	else if (g_req_type == AT_CMD_REQ)
	{
		if (at_parse_param("%d(0-1)", at_buf, &adc_Mode) != AT_OK)
			return ATERR_PARAM_INVALID;
		
		if (adc_Mode == 0) {
			voltageADC = xy_getVbat();
		} else if (adc_Mode == 1) {
			voltageADC =  get_adc_value_incpcore(ADC_AUX_ADC2);
		} else {
			return ATERR_PARAM_INVALID;
		}
		*prsp_cmd = xy_malloc(25);
		snprintf(*prsp_cmd, 25, "+QADC:%d,%d",adc_Mode,voltageADC);
	}

	return AT_END;
}

/**
  * @brief  +ZCONTLED   
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * 
  * @attention 1、此指令用于LED网络指示灯启动与关闭，具体闪烁规则请参考net_led_proc.c。
  * 		   2、此代码中GPIO9为示例使用，具体请根据硬件设计调整。
  * 		   3、开关LED灯，重启生效。
*/

extern void net_led_stop(void);
int at_LED_BC95_req(char *at_buf, char **prsp_cmd)
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
			g_softap_fac_nv->led_pin = GPIO_PAD_NUM_5; 
			SAVE_FAC_PARAM(led_pin);

			//xy_Soft_Reset(SOFT_RB_BY_NRB);
		}
	}
#if (AT_CUT != 1)
	else if (g_req_type == AT_CMD_TEST) //AT+XXX=?
	{
		*prsp_cmd = xy_malloc(30);
		snprintf(*prsp_cmd, 30, "+QLEDMODE:(0,1)");
	}
#endif
	else if (g_req_type == AT_CMD_QUERY) //AT+XXX?
	{
		*prsp_cmd = xy_malloc(30);
		snprintf(*prsp_cmd, 30, "+QLEDMODE:%d",((g_softap_fac_nv != NULL) && (g_softap_fac_nv->led_pin <= GPIO_PAD_NUM_63)) ? 1 : 0);
	}
	return AT_END;
}
#endif
