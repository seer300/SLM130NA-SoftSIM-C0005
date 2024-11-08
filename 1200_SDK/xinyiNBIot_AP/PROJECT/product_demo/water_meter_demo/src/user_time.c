#include "user_time.h"
#include "utc.h"
#include "user_i2c.h"
#include "user_config.h"
#include "xy_event.h"
#include "sys_rc32k.h"
#include "user_adc.h"
#include "user_NB_app.h"

uint8_t const DayTab[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; //月时间常量
RTC_TimeTypeDef Time = {0};
RTC_TimeTypeDef spi_test_time = {0};
uint8_t g_eeprom_buf[256] = {0};
uint8_t g_eeprom_peri = 20;
uint8_t g_send_peri = 5;

// __RAM_FUNC void Set_Stop_CP_Event(void)
// {
//     set_event(EVENT_LPTIMER);
// }

__RAM_FUNC void User_RTC_Refresh(void)
{
    uint8_t DayNum;
    char rsp[64] = {0};
    volatile int16_t test_temperature;
    static uint32_t adc_flag = 0;
	static uint32_t E2_min_old = 0;
	static uint32_t send_min_old = 0;

	if(Time.tm_year == 0)
		Get_Current_UT_Time(&Time);
	else
		Get_UT_Time_Fast(&Time,XY_TIMER_TIMING_PERIOD);

    test_temperature = g_user_temperature;
    g_user_temperature = Get_Temperature();    
    g_user_vbat = Get_VBAT();
    
    if((g_user_vbat > 5500) || (g_user_vbat < 2000))
    {
        Send_AT_to_Ext("VBAT ERROR!");
        xy_assert(0); 
    }
    if((g_user_temperature > 150) || (g_user_temperature < -40))
    {
        Send_AT_to_Ext("TEMPETATURE ERROR!");
        xy_assert(0);
    }
    if((abs(test_temperature - g_user_temperature) > 30) && adc_flag)
    {
        Send_AT_to_Ext("TEMPETATURE MATCH ERROR!");
        xy_assert(0);        
    }
    adc_flag = 1;

    sprintf(rsp, "\r\nTIME:%ld/%ld,%ld:%ld:%ld;VBAT:%d;temp:%d\r\n",Time.tm_mon, Time.tm_mday,Time.tm_hour,Time.tm_min,Time.tm_sec,g_user_vbat,g_user_temperature);
    Send_AT_to_Ext(rsp);

    UTC_WDT_Refresh(USER_UTC_WDT_TIME);

    if((Time.tm_min%g_eeprom_peri) == 0 && Time.tm_min != E2_min_old)
    {
    	E2_min_old = Time.tm_min;
		
        memcpy(&g_eeprom_buf[0],&Time,40);//填入时间戳

        memcpy(&g_eeprom_buf[40],&g_user_temperature, 2);//填入温度

        memcpy(&g_eeprom_buf[42],&g_user_vbat, 2);//填入VBAT                         

        memcpy(&g_eeprom_buf[44],&g_user_info, 168);//填入用户信息
#if DRIVER_TEST
        Write_EEPROM_With_Page(0,&g_eeprom_buf[0],212); 
#endif
        Set_RC32K_Cali_Event();//校准RC       
    }
    if(((Time.tm_min+(Time.tm_hour*60))%g_send_peri) == 0  && (Time.tm_min+(Time.tm_hour*60)) != send_min_old)
    {
    	send_min_old = (Time.tm_min+(Time.tm_hour*60));
		
        Get_Current_UT_Time(&Time);
        Trigger_Send_Proc();
    }
    
    Key_Check();//红外检测计时
}