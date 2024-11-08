#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "xy_timer.h"
#include "utc_watchdog.h"
#include "user_time.h"
#include "user_config.h"
#include "user_adc.h"
#include "xy_fota.h"
#include "user_i2c.h"


/*即将执行FOTA下载和FLASH保存，AP核可执行私有动作。也可以在User_SoftReset_Hook中执行私有动作*/
__RAM_FUNC void User_Fota_Proc_Hook(uint32_t state)
{	  
	/*即将执行差分包下载与FLASH保存，会长时间挂起AP核，可执行保存E2等紧急事务，并开启看门狗*/
    if(state == 1)
    {
        g_user_temperature = Get_Temperature();
        
        g_user_vbat = Get_VBAT();

        memcpy(&g_eeprom_buf[0],&Time,40);//填入时间戳

        memcpy(&g_eeprom_buf[40],&g_user_temperature, 2);//填入温度

        memcpy(&g_eeprom_buf[42],&g_user_vbat, 2);//填入VBAT                         

        memcpy(&g_eeprom_buf[44],&g_user_info, 168);//填入用户信息
#if DRIVER_TEST
        Write_EEPROM_With_Page(0,(uint8_t *)&g_eeprom_buf[0],212); 
#endif
	    // HAL_LPTIM_Stop(&Lptim1ContinuousHandle);

	    // HAL_LPTIM_DeInit(&Lptim1ContinuousHandle);

	    Timer_DeleteEvent(XY_TIMER_RTC);

        UTC_WDT_Refresh(USER_FOTA_UTC_WDT_TIME);
     
        clear_all_event();

        extern void Clear_Cloud_State();
        Clear_Cloud_State();

        xy_printf("\r\nready fota\r\n");
    }
	/*FOTA流程异常中止结束，用户可恢复先前暂停的事务*/
    else
    {
        User_Config_Init();

        xy_printf("\r\nfota done\r\n");
    }
}


void User_SoftReset_Hook(Soft_Reset_Type soft_reset_type)
{
	xy_printf("\r\nReady to reset:%d\r\n", soft_reset_type);

	if(soft_reset_type == SOFT_RB_BY_FOTA)
	{
		xy_printf("\r\nFOTA reset\r\n");
	}
}



void User_Hook_Regist(void)
{
	/*即将执行FOTA下载和FLASH保存，AP核可执行私有动作。失败后需要恢复私有动作*/
	Fota_Proc_Hook_Regist(User_Fota_Proc_Hook);

	/*差分包校验通过后，即将重启进入差分升级流程。AP核用户可执行私有动作，但不能执行擦写FLASH动作*/
	Pre_Reset_Hook_Regist(User_SoftReset_Hook);
}
