/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#include <string.h>
#include <stdlib.h>
#include "prcm.h"
#include "gpio.h"
#include "hal_gpio.h"
#include "xy_at_api.h"
#include "xy_system.h"
#include "at_cmd_regist.h"

extern uint8_t g_wdt_enable;

/*耗时开销大，主要用于监控CP核异常等。模组形态在AP核设置，CP核会执行喂狗；OPENCPU形态运行CP核期间在AP核设置总的工作时长，CP核不会执行喂狗*/
__FLASH_FUNC void UTC_WDT_Init(uint32_t sec)
{
	if(g_wdt_enable == 0)
		return;
	
	if(sec != 0)
	{
        /*此处关中断，是为了防止喂狗期间切出去执行中断时出现卡死，造成看门狗无效*/
        DisablePrimask();
	    UTC_WDTDisable();
	    UTC_WDTCtrlConfig(UTC_WDT_CTRL_RESET_Msk | UTC_WDT_CTRL_MODE_LONG | UTC_WDT_MASK_MON_Msk | UTC_WDT_MASK_DATE_Msk | UTC_WDT_MASK_YEAR_Msk);
        UTC_WDTLongTimerIncrementFeedSecond(sec);
	    UTC_WDTEnable();
	    AONPRCM->RST_CTRL |= 0x04;//utc reset en
        EnablePrimask();
	}
}

/*此处关中断，是为了防止喂狗期间切出去执行中断时出现卡死，造成看门狗无效*/
void UTC_WDT_Refresh(uint32_t sec)
{
	if(g_wdt_enable == 0)
		return;
    
    DisablePrimask();
    UTC_WDTLongTimerIncrementFeedSecond(sec);
    EnablePrimask();
}


/*接口耗时大，通常用于时长为小时以上级别的看门狗设置与喂狗*/
void UTC_WDT_Refresh2(uint32_t sec)
{
	RTC_TimeTypeDef rtctime;
	if(g_wdt_enable == 0)
		return;
	
	xy_gmtime(RTC_Get_Global_Byoffset(sec*1000),&rtctime);
    UTC_WDTLongTimerDataInvalid();
    utc_cnt_delay(2);//bug9804
    UTCWDT_Long_Timer_CalSet(rtctime.tm_year/100,rtctime.tm_year%100,rtctime.tm_mon,rtctime.tm_mday,rtctime.tm_wday);
	UTCWDT_Long_Timer_TimerSet(0,rtctime.tm_hour,rtctime.tm_min,rtctime.tm_sec,rtctime.tm_msec/10);
    UTC_WDTLongTimerDataValid();
}


/*接口耗时大，通常用于时长为小时以上级别的看门狗设置与喂狗*/
void UTC_WDT_Init2(uint32_t sec)
{
	if(g_wdt_enable == 0)
		return;
	
	if(sec != 0)
	{
	    UTC_WDTDisable();
	    UTC_WDTCtrlConfig(UTC_WDT_CTRL_RESET_Msk | UTC_WDT_CTRL_MODE_LONG);
		
		UTC_WDT_Refresh2(sec);
		
	    UTC_WDTEnable();
	    AONPRCM->RST_CTRL |= 0x04;//utc reset en
	}
}

void UTC_WDT_Deinit(void)
{
    UTC->WDT_CTRL = 0;
    AONPRCM->RST_CTRL &= ~0x04;//utc reset dis
}


/*由于AP_WDT复位不彻底，进而在一级boot中强行跳转执行UTC WDT，执行芯片全局复位*/
void UTC_WDT_Func(void)
{
	// if((AONPRCM->AONGPREG0 & 0x43) == 0x43)
	// {
	// 	//8kmem的开启flash电源
	// 	AONPRCM->FLASH_VCC_IO_CTRL = (AONPRCM->FLASH_VCC_IO_CTRL & 0xF0) | 0x2;
	// }
	// else if((AONPRCM->AONGPREG0 & 0x42) == 0x42)
	{
		//AONPRCM->AONGPREG0 = 0;
		//ap wdt触发复位后，执行utc wdt造成全局复位
		//COREPRCM->CKG_CTRL_L |= 0x20;//utc时钟
		AONPRCM->RST_CTRL = 0x04;//stop cp,utc reset en
        UTC->WDT_CTRL = 0;
		//UTC->CTRL = 0x07;
		UTC->WDT_CTRL = 0x05;
		UTC->WDT_TIMER_DATA = 0x01;
		//UTC->CTRL = 0x03;
		while(1);//wait utc wdt reset
	}
}