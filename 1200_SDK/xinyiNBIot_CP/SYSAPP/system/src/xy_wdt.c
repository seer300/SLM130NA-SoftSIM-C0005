/*
 * xy_wdt.c
 *
 *  Created on: 2022年4月11日
 *      Author: jiangzj
 */
#include "hw_memmap.h"
#include "hw_prcm.h"
#include "hw_watchdog.h"
#include "xy_wdt.h"
#include "xy_utils.h"
#include "xinyi_hardware.h"
#include "watchdog.h"
#include "prcm.h"
#include "attribute.h"
#include "rtc_tmr.h"
#include "xy_rtc_api.h"

uint32_t g_wdt_refresh = 0;

void WDT_Handler(void){
	xy_assert(0);
}

/**
  * @brief  Disables the watchdog timer interrupt.
  * @param  ulBase is the base address of the watchdog timer module.
  * @retval None
  */
void WatchdogIntDisable(unsigned long ulBase)
{
    HWREG(ulBase + WDT_CTL) &= ~WDT_CTL_INT_EN;
}

//if softreset,retension mem will reset,so use watchdog to keep retension mem
uint32_t wdt_init(void)
{
	if(g_softap_fac_nv->watchdog_enable)
	{
		PRCM_ClockEnable(CORE_CKG_CTL_CP_WDT_EN);

		WatchdogDisable(CP_WDT_BASE);

//		if(0 == g_softap_fac_nv->off_debug)
//		{
			WatchdogResetDisable(CP_WDT_BASE);
			WatchdogIntEnable(CP_WDT_BASE);
			WatchdogReadClearInt(CP_WDT_BASE);

			NVIC_IntRegister(WDT_IRQn, WDT_Handler, 0);
//		}
//		else
//		{
//			WatchdogIntDisable(CP_WDT_BASE);
//			WatchdogResetEnable(CP_WDT_BASE);
//		}

		WatchdogReloadSet(CP_WDT_BASE, WDT_COUNT_CLK * WDT_TIME_MS);
		WatchdogEnable(CP_WDT_BASE);
	}
	else
	{
		PRCM_ClockEnable(CORE_CKG_CTL_CP_WDT_EN);

		WatchdogDisable(CP_WDT_BASE);

		PRCM_ClockDisable(CORE_CKG_CTL_CP_WDT_EN);
	}

	return 1;
}

__RAM_FUNC uint32_t wdt_refresh(void)
{
	if(WatchdogRunning(CP_WDT_BASE)){

		osCoreEnterCritical();

		WatchdogReloadSet(CP_WDT_BASE, WDT_COUNT_CLK * WDT_TIME_MS);

		osCoreExitCritical();
	}
	return 1;
}

/*仅用于cp异常情况*/
__RAM_FUNC void cp_utcwdt_init(uint32_t timeout_sec)
{
    RTC_TimeTypeDef rtctime;

    UTCWDTDisable();
    UTCWDTCtrlConfig(UTC_WDT_CTRL_RESET_Msk | UTC_WDT_CTRL_MODE_LONG | UTC_WDT_MASK_MON_Msk | UTC_WDT_MASK_DATE_Msk | UTC_WDT_MASK_YEAR_Msk);
    cp_utcwdt_refresh(timeout_sec);
    UTCWDTEnable();
    AONPRCM->RST_CTRL |= 0x04;//utc reset en
}

__RAM_FUNC void cp_utcwdt_deinit(void)
{
	if(!Is_OpenCpu_Ver() && (UTC->WDT_CTRL & UTC_WDT_CTRL_EN_Msk))
	{
		UTC->WDT_CTRL = 0;
		AONPRCM->RST_CTRL &= ~0x04;//utc reset dis
		g_wdt_refresh = 1;
	}
}

//只允许设置24小时以内
__RAM_FUNC void cp_utcwdt_refresh(uint32_t timeout_sec)
{
	//仅模组形态，执行utcwdt喂狗操作
	if(g_wdt_refresh || (!Is_OpenCpu_Ver() && (UTC->WDT_CTRL & UTC_WDT_CTRL_EN_Msk)))
	{
		uint8_t ulAMPM;    //24小时制，固定为0;
		uint8_t ulHour;
		uint8_t ulMin;
		uint8_t ulSec;
		uint8_t ulMinSec;	//wdt 不匹配

		uint32_t tmp_Sec;      //秒     增加后的 中间值;
		uint32_t tmp_Min;      //分钟    增加后的 中间值;
		uint32_t tmp_Hour;     //小时    增加后的 中间值;

		osCoreEnterCritical();

		UTCWDTLongTimerDataInvalid();
//	    utc_cnt_delay(2);//bug9804

		if(timeout_sec >= (24*60*60-1))
		{
			timeout_sec = (24*60*60-1);
		}

		UTCTimerGet(&ulAMPM, &ulHour, &ulMin, &ulSec, &ulMinSec, 0);

		tmp_Sec = ulSec + timeout_sec;
		if(tmp_Sec < 60)                                 //ulSec+IncSec             如果秒        加后的值小于60，直接计算即可
		{
			ulSec = tmp_Sec;
		}
		else                //秒    超过60，要进位 分钟
		{
			tmp_Min = ulMin + tmp_Sec/60;
			if(tmp_Min < 60)                             //ulMin + tmp_Sec/60;      如果分钟        加后的值小于60，直接计算即可
			{
				ulSec = tmp_Sec%60;
				ulMin = tmp_Min;
			}
			else            //分钟 超过60，要进位 小时
			{
				tmp_Hour = ulHour + tmp_Min/60;
				if(tmp_Hour < 24)                        //ulHour + tmp_Min/60;      如果小时        加后的值小于24，直接计算即可
				{
					ulSec = tmp_Sec%60;
					ulMin = tmp_Min%60;
					ulHour = tmp_Hour;
				}
				else        //小时 超过24，要进位 天
				{
					ulSec = tmp_Sec%60;
					ulMin = tmp_Min%60;
					ulHour = tmp_Hour%24;
				}
			}
		}

		UTCWDT_Long_Timer_TimerSet(ulAMPM,ulHour,ulMin,ulSec,ulMinSec);
		UTCWDTLongTimerDataValid();

		osCoreExitCritical();
	}
}

void Disable_All_WDT(void)
{
	WatchdogDisable(CP_WDT_BASE);
	cp_utcwdt_deinit();
}
