#include <string.h>
#include <stdio.h>
#include "hal_timer.h"
#include "user_config.h"
#include "xy_system.h"
#include "hal_lptimer.h"
#include "user_time.h"
#include "at_CP_api.h"
#include "user_i2c.h"
#include "user_spi.h"
#include "at_cmd_regist.h"
#include "xy_at_api.h"
#include "at_uart.h"
#include "user_adc.h"
#include "user_NB_app.h"

HAL_LPTIM_HandleTypeDef Lptim1ContinuousHandle = {0};
User_Info_t g_user_info = {0};
uint8_t g_send_mark = 0;
int16_t g_user_temperature = 25;
int16_t g_user_vbat = 3600;

__RAM_FUNC void lptimer_callback(void)
{
#if DRIVER_TEST
	User_SPI_Func();//调用SPI存储当前时间
#endif

	Key_Func();//干簧管检测  
}

__RAM_FUNC void XY_TIMER_Timeout(void)
{
    set_event(EVENT_XY_TIMER);
}

void Lptimer_PeriodicTiming_Init(uint32_t ms)
{
	Lptim1ContinuousHandle.Instance = HAL_LPTIM1;
    
	Lptim1ContinuousHandle.Init.Mode = HAL_LPTIM_MODE_CONTINUOUS;
    
	HAL_LPTIM_Stop(&Lptim1ContinuousHandle);

	HAL_LPTIM_DeInit(&Lptim1ContinuousHandle);

	HAL_LPTIM_SetTimeout(&Lptim1ContinuousHandle, ms);
    
	HAL_LPTIM_Init(&Lptim1ContinuousHandle);
    
	HAL_LPTIM_Start(&Lptim1ContinuousHandle);
}

void User_Config_Init(void)
{
	//获取Time日历
	Get_Current_UT_Time(&Time);
	//获取的时间存入E2
#if DRIVER_TEST
	Write_EEPROM_With_Page(0,(uint8_t *)&Time,40);
#endif
    //RTC 1S唤醒一次，工作约10ms
	Timer_AddEvent(XY_TIMER_RTC, XY_TIMER_TIMING_PERIOD, XY_TIMER_Timeout, 1);   //RTC中断

	//lptimer周期定时
	McuLptimerSetPeriod(LPTIMER_TIMING_PERIOD);
	McuLptimerIrqReg(lptimer_callback);
	McuLptimerEn();
}

void User_Info_Init(void)
{
	At_status_type at_ret = XY_OK;

	Boot_CP(WAIT_CP_BOOT_MS);

	xy_printf("user info start\r\n");
	
	Set_Water_Meter_User_Config();

	at_ret = xy_wait_tcpip_ok(USER_WAIT_SEC);
	if(at_ret != XY_OK)
	{
		xy_assert(0);
	}

	at_ret = AT_Send_And_Get_Rsp("AT+CPINFO\r\n",10, "+CPINFO:", "%s,%s,%s,%s,%s,%s", &g_user_info.object_Id,
																						&g_user_info.Revision, 
																						&g_user_info.user_imei,  
																						&g_user_info.user_imsi, 
																						&g_user_info.user_usim, 
																						&g_user_info.user_apn);
	if(at_ret != XY_OK)
	{
		xy_assert(0);
	}

	at_ret = AT_Send_And_Get_Rsp("AT+CFUN=0\r\n",10, NULL, NULL);
	if(at_ret != XY_OK)
	{
		xy_assert(0);
	}

	Stop_CP2(10,1);														
}

int at_DEMOCFG_req(char *at_buf, char **prsp_cmd)
{
	(void)prsp_cmd;
	if (g_cmd_type == AT_CMD_REQ)
	{
		char cmd[10] = {0};
		char rsp[50] = {0};
		uint32_t val1 = 0;
		uint32_t val2 = 0;

		at_parse_param("%s,%d,%d",at_buf,cmd,&val1,&val2);

		if (!strcmp(cmd, "SPI"))
		{
            sprintf(rsp, "\r\nSPI:%ld/%ld,%ld:%ld:%ld\r\n",spi_test_time.tm_mon, spi_test_time.tm_mday,spi_test_time.tm_hour,spi_test_time.tm_min,spi_test_time.tm_sec);
            Send_AT_to_Ext(rsp);
		}
		else if(!strcmp(cmd, "EEPROM"))
		{
			if(val1 == 0)
            {
                g_eeprom_peri  = val2;//E2存储周期，单位分钟
            }
            else if(val1 == 1)
            {
				RTC_TimeTypeDef Test_time = {0};
                Read_EEPROM_With_Page(0,(uint8_t *)&Test_time,40);
                sprintf(rsp, "\r\nEEPROM:%ld/%ld,%ld:%ld:%ld\r\n",Test_time.tm_mon, Test_time.tm_mday,Test_time.tm_hour,Test_time.tm_min,Test_time.tm_sec);
			    Send_AT_to_Ext(rsp);
            }
		}
		else if(!strcmp(cmd, "SEND"))
		{
			if(val1 == 0)
            {
                g_send_peri  = val2;//发送周期，单位分钟
            }
			else if(val1 == 1)
			{
				Trigger_Send_Proc();
			}
		}
		else if(!strcmp(cmd, "VERSION"))
		{
			if(val1 == 0)
            {
			    Send_AT_to_Ext("\r\nVERSION:XY1200SV1000\r\n");
            }
		}
		else
		{
			return XY_ERR_PARAM_INVALID;
		}
		
    	return XY_OK;
	}

	else
	{
		return XY_ERR_PARAM_INVALID;
	}
}

