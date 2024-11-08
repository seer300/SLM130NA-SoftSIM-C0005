/**
 * @file master.c
 * @brief master使用AT口与PC机串口工具交互，将收到的AT命令与进行解析，然后与从机进行交互
 * @version 0.2
 * @date 2022-10-08
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hal_lpuart.h"
#include "master.h"
#include "at_process.h"
#include "at_cmd_regist.h"
#include "xy_system.h"
#include "xy_timer.h"
#include "xy_cp.h"

#if (DRIVER_TEST == 1)


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



int at_MASTER_req(char *param, char **rsp_cmd)
{
    char *pTmp = NULL;
    CuSuite *suite = NULL;
    CuString *output = NULL;

	if (g_cmd_type == AT_CMD_QUERY) //查询类: AT+XXX?
	{
        *rsp_cmd = xy_malloc(10);
        sprintf(*rsp_cmd, "\r\nOK\r\n");
    }
    else if (g_cmd_type == AT_CMD_REQ) //设置类: AT+XXX=param
    {
        if ((strstr((const char *)param, "Test")) != NULL)
        {
            //主机初始化主从MCU通信使用的UART
            MasterSlave_UART_Init();

            //申请CuSuite和CuString有关内存
            suite = CuSuiteNew();
            output = CuStringNew();

            //解析时钟源切换参数，1：切换HRC与PLL时钟；0：使用当前版本默认时钟，不切换
            if ((pTmp = strstr((const char *)param, "Switchclksys")) != NULL)
            {
                if ( (Switchclksys =get_num_from_cmd((uint8_t*)pTmp)) == 1)
                {
                    Timer_AddEvent(TIMER_LP_USER2, 20*1000, CLK_SYS_Switch_Test, 1);;
                }
            }
 
            //解析DebugLevel
            if ((pTmp = strstr((const char *)param, "DebugLevel")) != NULL)
            {
                if ((InputDebugLevel = get_num_from_cmd((uint8_t*)pTmp)) < 0)
                {
                    InputDebugLevel = 0;
                }
            }
            else
            {
                InputDebugLevel = 0;
            }

            //解析TestTimes
            if ((pTmp = strstr((const char *)param, "TestTimes")) != NULL)
            {
                if ((TestTimes = get_num_from_cmd((uint8_t*)pTmp)) < 0)
                {
                    TestTimes = 1;
                }
            }
            else
            {
                TestTimes = 1;
            }

            //解析具体的外设模块并挂在CuTest上
            if(strstr((const char *)param, "<UART>") != NULL)
            {
                Debug_Print_Str(DEBUG_LEVEL_1, "\r\n<UART>\r\n");
                SUITE_ADD_TEST(suite, uart_master_test);
            }
            if(strstr((const char *)param, "<I2C>") != NULL)
            {
                Debug_Print_Str(DEBUG_LEVEL_1, "\r\n<I2C>\r\n");    
                SUITE_ADD_TEST(suite, i2c_master_test);
            }
            if(Switchclksys == 0) //下述外设测试时会受时钟源切换的影响，故仅在时钟源不变时进行测试
            {
                if(strstr((const char *)param, "<SPI>") != NULL)
                {
                    Debug_Print_Str(DEBUG_LEVEL_1, "\r\n<SPI>\r\n");
                    SUITE_ADD_TEST(suite, spi_master_test);
                }
                if (strstr((const char *)param, "<CSP_SPI>") != NULL)
                {
                    Debug_Print_Str(DEBUG_LEVEL_1, "\r\n<CSP_SPI>\r\n");
                    SUITE_ADD_TEST(suite, csp_spi_master_test);
                }
            
                if (strstr((const char *)param, "<CSP_UART>") != NULL)
                {
                    Debug_Print_Str(DEBUG_LEVEL_1, "\r\n<CSP_UART>\r\n");
                    SUITE_ADD_TEST(suite, csp_uart_master_test);
                }
            }

            //运行CuTest，开始跑驱动测试
            Debug_Print_Str(DEBUG_LEVEL_0, "\r\n+driver test begin , every case test times:%ld\r\n", TestTimes);
            CuSuiteRun(suite);
            CuSuiteSummary(suite, output);
            CuSuiteDetails(suite, output);
            Output_Print_Str(output->buffer);
            Debug_Print_Str(DEBUG_LEVEL_0, "\r\n+driver test end , every case test times:%ld\r\n", TestTimes);

            //释放CuSuite和CuString有关内存
            CuStringDelete(output);
            CuSuiteDelete(suite);

            //AT命令回复OK
            *rsp_cmd = xy_malloc(10);
            sprintf(*rsp_cmd, "\r\nOK\r\n");
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
#endif /* #if (DRIVER_TEST == 1) */
