#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "xinyi2100.h"
#include "xy_printf.h"
#include "hal_gpio.h"
#include "uart.h"
#include "hal_lpuart.h"
#include "at_uart.h"
#include "at_process.h"
#include "at_hardware_cmd.h"
#include "at_cmd_regist.h"
#include "xy_flash.h"
#include "mcu_adapt.h"
#include "at_uart.h"
#include "auto_baudrate.h"


uint8_t g_setipr_flag = 0;	//收到AT+IPR=xx命令后先回OK再设置新的波特率

/**
 * AT+QRST=<mode>
 * AT+QRST=?
*/
int at_QRST_req2(char *at_buf, char **prsp_cmd)
{
	if (g_cmd_type == AT_CMD_REQ)
	{
		int mode = -1;

		if (at_parse_param("%d", at_buf, &mode) != XY_OK || mode != 1)
			return XY_ERR_PARAM_INVALID;
		Send_AT_to_Ext("\r\nOK\r\n");

		xy_Soft_Reset(SOFT_RB_BY_NRB);
	}
	else if (g_cmd_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(32);
		snprintf(*prsp_cmd, 32, "(1)");
	}
	else
		return  (XY_ERR_PARAM_INVALID);

	return XY_OK;
}

void set_ipr(uint32_t baud_rate)
{
#if (AT_LPUART == 1)
	if (baud_rate == 0)
	{
		HAL_LPUART_DeInit(&g_at_lpuart);
		g_at_lpuart.Init.BaudRate = AutoBaudDetection();
	}
	at_uart_config(g_at_lpuart.Init.BaudRate, 0); // 切换波特率
	if (baud_rate == 0)
		HAL_LPUART_Init(&g_at_lpuart);
	set_standby_by_rate(g_at_lpuart.Init.BaudRate);
#endif
}

/**
 * @brief  AT串口配置(波特率)
 * @param  param:
 * @param  prsp_cmd:
 * @retval AT_END
 * @arg 请求类AT命令：AT+IPR=<baud_rate>
 *      @arg <baud_rate>,可配置波特率：0,4800,9600,19200,38400,57600,115200,230400,460800,921600;0表示启用波特率自适应
 *		 若设置的波特率高于9600，则同时修改NV关闭standby
 * @arg 查询类AT命令：AT+IPR?
 * @arg 测试类AT命令：AT+IPR=?
 * 
 * @attention 波特率大于9600则关闭standby,小于等于9600则打开standby。
 * @attention LPUART只能在小于等于9600波特率情况下，才能保证唤醒后不丢AT命令；建议客户不要选用高于9600波特率。
 */
int at_IPR_req2(char  *param, char **rsp_cmd)
{
	uint32_t baud_rate = 9600;
	if (g_cmd_type == AT_CMD_TEST)  //测试类
	{
		*rsp_cmd = xy_malloc(200);
		sprintf(*rsp_cmd, "(1200,2400,4800,9600,19200,38400,57600,115200),(0,4800,9600,19200,38400,57600,115200,230400,460800,921600)]");
	}
	else if (g_cmd_type == AT_CMD_QUERY) //查询类
	{
		*rsp_cmd = xy_malloc(40);
		sprintf(*rsp_cmd, "%d", (READ_FAC_NV(uint16_t,at_uart_rate) & 0x1ff) * 2400);
	}
	else if (g_cmd_type == AT_CMD_REQ) //设置类
	{
		at_parse_param("%d", param, &baud_rate);

		uint8_t valid_baud_flag = 0;

		uint32_t valid_baud[] = {0, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};

		for (uint32_t i = 0; i < sizeof(valid_baud) / sizeof(valid_baud[0]); i++)
		{
			if (baud_rate == valid_baud[i])
			{
				valid_baud_flag = 1;
				break;
			}
		}

		if ((valid_baud_flag == 0))
			return XY_ERR_PARAM_INVALID;

		g_setipr_flag = 1;
#if (AT_LPUART == 1)
		g_at_lpuart.Init.BaudRate = baud_rate;
#endif
		baud_rate = baud_rate / 2400;
		WRITE_FAC_PARAM(at_uart_rate, &baud_rate);
	}
	return XY_OK;
}


