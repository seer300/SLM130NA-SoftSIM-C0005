/**
 * @file        main.c
 * @brief       主入口
 */
#include <stdio.h>
#include <string.h>
#include "hal_gpio.h"
#include "hal_csp.h"
#include "slave.h"
#include "at_process.h"
#include "at_cmd_regist.h"
#include "xy_system.h"
#include "at_uart.h"
#include "xy_cp.h"

#if (DRIVER_TEST == 2)

static uint8_t CmdData[200] = {0};
uint32_t id = 0, size = 0;
uint32_t ArgStruct[1024] = {0};


__RAM_FUNC void CLK_SYS_Switch_Test()
{
	static  int sum = 0;

    if(sum%2 == 0)
	{
		PRCM_ForceCPOff_Disable();
        delay_func_us(100);
        Ap_HclkDivSet(10);
		SIDO_NORMAL_INIT();
		START_PLL();
        Sys_Clk_Src_Select(SYSCLK_SRC_PLL);
	}
	else
	{
		HRC_Clk_Src_Select(RC26MCLKDIV);
		Sys_Clk_Src_Select(SYSCLK_SRC_HRC);
		Xtal_Pll_Ctl(SYSCLK_SRC_HRC,LSIO_CLK_SRC);
		PRCM_SlowfreqDivDisable();
	}
	
	sum++;
}

int at_SLAVE_req(char *param,char **rsp_cmd)
{
	char *pTmp = NULL;
	
	if (g_cmd_type == AT_CMD_QUERY) //查询类: AT+XXX?
	{
        *rsp_cmd = xy_malloc(10);
        sprintf(*rsp_cmd, "\r\nOK\r\n");
    }
    else if (g_cmd_type == AT_CMD_REQ) //设置类: AT+XXX=param
    {
		//解析时钟源切换参数，1：切换HRC与PLL时钟；0：使用当前版本默认时钟，不切换
		if ((pTmp = strstr((const char *)param, "Switchclksys")) != NULL)
		{
			if ( (Switchclksys =get_num_from_cmd((uint8_t*)pTmp)) == 1)
			{
				Timer_AddEvent(TIMER_LP_USER2, 20*1000, CLK_SYS_Switch_Test, 1);;
			}
		}
		if ((strstr((const char *)param, "Test")) != NULL)
        {
            MasterSlave_UART_Init();//从机初始化主从MCU通信使用的UART
 
			Send_AT_to_Ext("\r\n+Please send test command to master uart!\r\n\r\nOK\r\n");

			while (1)
			{
				//从机接收主机发过来的数据
				HAL_CSP_Receive_IT(&MasterSlave_UART_Handle, CmdData, sizeof(CmdData), 5);
				while (MasterSlave_RxCplt_Flag != 1);
				MasterSlave_RxCplt_Flag = 0;

				//如果是TIMER驱动测试则单独处理，否则就解析数据到结构体变量ArgStruct中
				if (strstr((const char *)CmdData, "<timer_test>") != NULL)
				{
					// timer_slave_test();
				}
				else
				{
					GetArgStructFromMaster(CmdData, &id, &size, &InputDebugLevel, ArgStruct);
				}

				if (id == (uint32_t)test_UART)
				{
					uart_slave_test((UartArgStruct *)ArgStruct);
				}
				else if (id == (uint32_t)test_I2C)
				{
					i2c_slave_test((I2cArgStruct *)ArgStruct);
				}
				else if(id == (uint32_t)test_SPI)
				{
					spi_slave_test((SpiArgStruct *)ArgStruct);
				}
				else if (id == (uint32_t)test_CSP_UART)
				{
					csp_uart_slave_test((CspUartArgStruct *)ArgStruct);
				}
				else if (id == (uint32_t)test_CSP_SPI)
				{
					csp_spi_slave_test((CspSpiArgStruct *)ArgStruct);
				}
				memset(CmdData, 0, sizeof(CmdData));
			}
		}
		else
		{
			return XY_ERR_PARAM_INVALID;
		}
    }
	else
	{
		return XY_ERR_PARAM_INVALID;
	}
    return XY_OK;
}
#endif /* #if (DRIVER_TEST == 2) */
