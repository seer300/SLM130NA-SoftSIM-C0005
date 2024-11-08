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
  * @brief 芯片当前信息查询
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval int
  */
int at_CHIPINFO_req(char *at_buf, char **prsp_cmd)
{
	char cmd[5] = {0};

	if (g_req_type == AT_CMD_REQ)
	{
		if (at_parse_param("%5s", at_buf, cmd) != AT_OK)
			return ATERR_PARAM_INVALID;

		if (at_strcasecmp(cmd, "ALL")) //VBAT,TEMP
		{
			*prsp_cmd = xy_malloc(65);
			snprintf(*prsp_cmd, 65, "+MCHIPINFO:%s,%d\r\n+MCHIPINFO:%s,%d", "TEMP", xy_getempera(), "VBAT", xy_getVbat());
		}

		else if (at_strcasecmp(cmd, "TEMP")) //TEMP
		{
			*prsp_cmd = xy_malloc(40);
			snprintf(*prsp_cmd, 40, "+MCHIPINFO:%s,%d", "TEMP", xy_getempera());
		}

		else if (at_strcasecmp(cmd, "VBAT")) //VBAT
		{
			*prsp_cmd = xy_malloc(40);
			snprintf(*prsp_cmd, 40, "+MCHIPINFO:%s,%d", "VBAT", xy_getVbat());
		}

		else
			return ATERR_PARAM_INVALID;
	}
#if (AT_CUT != 1)
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(40);
		snprintf(*prsp_cmd, 40, "+MCHIPINFO:(%s,%s,%s)", "ALL", "TEMP", "VBAT");
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

int at_ADC_req(char *at_buf, char **prsp_cmd)
{
	int adc_Mode = 0;

	
	if (g_req_type == AT_CMD_QUERY || g_req_type == AT_CMD_ACTIVE)
	{
		*prsp_cmd = xy_malloc(80);
		snprintf(*prsp_cmd, 80, "+ZADC:TEMP,%d,VBAT,%d,ADC2,%d,TSENSOR,%d",xy_getempera(),xy_getVbat(),get_adc_value_incpcore(ADC_AUX_ADC2),get_adc_value_incpcore(ADC_TSENSOR));
	}
#if (AT_CUT != 1)
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(30);

		if( g_ADCVref == ADC_VREF_VALUE_1P5V)	  //精度好
			snprintf(*prsp_cmd, 30, "+ZADC:(0,1500)");
		else if( g_ADCVref == ADC_VREF_VALUE_2P2V)//受供电影响,实际量程可能不到到2.2V，精度较差
			snprintf(*prsp_cmd, 30, "+ZADC:(0,2200)");
		else if (g_ADCRange == ADC_RANGE_VBAT)	  //测量范围0-VBAT,最高耐压5.5V，精度较差
			snprintf(*prsp_cmd, 30, "+ZADC:(0,VBAT)");
	}
#endif
	else if (g_req_type == AT_CMD_REQ)
	{
		if (at_parse_param("%d(0-0)", at_buf, &adc_Mode) != AT_OK)
			return ATERR_PARAM_INVALID;

		uint16_t cal_vol = get_adc_value_incpcore(ADC_AUX_ADC2);//默认读ADC2通道的电压
		*prsp_cmd = xy_malloc(25);
		snprintf(*prsp_cmd, 25, "+ZADC:%d", (cal_vol > xy_getVbat())? xy_getVbat() : cal_vol);
	}

	return AT_END;
}


/** 
  * @brief  VBAT电压测试AT命令处理函数，显示结果为VBAT电压（单位mV）
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 测试AT命令：AT+VBAT=?
  */
int at_VBAT_req(char *at_buf, char **prsp_cmd)
{
	unsigned int voltage;

	(void)at_buf;

	if (g_req_type == AT_CMD_TEST || g_req_type == AT_CMD_QUERY || g_req_type == AT_CMD_ACTIVE)
	{
		voltage = xy_getVbat();

		*prsp_cmd = xy_malloc(30);
		snprintf(*prsp_cmd, 30, "\r\n+VBAT:%d mV\r\n\r\nOK\r\n", voltage);
	}
	else if (g_req_type == AT_CMD_REQ)
		return ATERR_PARAM_INVALID;

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
int at_LED_req(char *at_buf, char **prsp_cmd)
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

			//xy_Soft_Reset(SOFT_RB_BY_NRB);
		}
	}
#if (AT_CUT != 1)
	else if (g_req_type == AT_CMD_TEST) //AT+XXX=?
	{
		*prsp_cmd = xy_malloc(30);
		snprintf(*prsp_cmd, 30, "+ZCONTLED:(0,1)");
	}
#endif
	else if (g_req_type == AT_CMD_QUERY) //AT+XXX?
	{
		*prsp_cmd = xy_malloc(30);
		snprintf(*prsp_cmd, 30, "+ZCONTLED:%d", ((g_softap_fac_nv != NULL) && (g_softap_fac_nv->led_pin <= GPIO_PAD_NUM_63)) ? 1 : 0);
	}

	return AT_END;
}




