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


//*AT+QPOWD=x
int at_QPOWD_req(char *at_buf, char **prsp_cmd)
{
	if (g_cmd_type == AT_CMD_REQ)
	{
		int powd = -1;
		if (at_parse_param("%d", at_buf, &powd) != XY_OK || powd < 0 || powd > 2)
			return (XY_ERR_PARAM_INVALID);

		if (powd == 2)
		{
			Send_AT_to_Ext("\r\nOK\r\n");
			xy_Soft_Reset(SOFT_RB_BY_NRB);
		}
		else
			return XY_FORWARD;
	}
	else if (g_cmd_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(32);
		snprintf(*prsp_cmd, 32, "\r\n+QPOWD: (0-2)\r\n\r\nOK\r\n");
	}
	else
		return (XY_ERR_PARAM_INVALID);
	return XY_OK;
}

/**
 * AT+QRST=<mode>
 * AT+QRST=?
*/
int at_QRST_req(char *at_buf, char **prsp_cmd)
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
		snprintf(*prsp_cmd, 32, "\r\n+QRST: (1)\r\n\r\nOK\r\n");
	}
	else
		return  (XY_ERR_PARAM_INVALID);

	return XY_OK;
}



