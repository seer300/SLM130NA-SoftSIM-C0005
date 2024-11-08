#include "utc.h"
#include "system.h"
/**
  * @brief  stop the UTC timer.
  * @retval None
  */
void UTC_TimerStop(void)
{
    UTC->CTRL |= UTC_CTRL_TIMER_STOP;
}

/**
  * @brief  run the UTC timer.
  * @retval None
  */
void UTC_TimerRun(void)
{
    UTC->CTRL &= (~UTC_CTRL_TIMER_STOP);
}

/**
  * @brief  stop the UTC calendar.
  * @retval None
  */
void UTC_CalStop(void)
{
    UTC->CTRL |= UTC_CTRL_CAL_STOP;
}

/**
  * @brief  run the UTC calendar.
  * @retval None
  */
void UTC_CalRun(void)
{
    UTC->CTRL &= (~UTC_CTRL_CAL_STOP);
}

/**
  * @brief  stop the UTC 10ms/1s divider.
  * @retval None
  */
void UTC_DivStop(void)
{
    UTC->CTRL |= UTC_CTRL_DIVCNT;
}

/**
  * @brief  Enable the UTC 10ms/1s divider.
  * @retval None
  */
void UTC_DivEn(void)
{
    UTC->CTRL &= (~UTC_CTRL_DIVCNT);
}

/**
  * @brief  set the UTC hour mode.
  * @param  ulMode is the hour mode as following:
  *     @arg UTC_HOUR_MODE_12H
  *     @arg UTC_HOUR_MODE_24H
  * @retval None
  */
void UTC_HourModeSet(uint32_t ulMode)
{
    UTC->HOUR_MODE = ulMode;
}

/**
  * @brief  get the UTC hour mode.
  * @retval the UTC hour mode
  */
uint32_t UTC_HourModeGet(void)
{
    return UTC->HOUR_MODE;
}

/**
  * @brief  set the UTC timer.
  * @param  ulAMPM is 0:AM or 1:PM.
  * @param  ulHour is hours value.
  * @param  ulMin is minutes value.
  * @param  ulSec is seconds value.
  * @param  ulMinSec is hundredths of a second value.
  * @retval None
  */
void UTC_TimerSet(uint32_t ulAMPM, uint32_t ulHour, uint32_t ulMin, uint32_t ulSec, uint32_t ulMinSec)
{
    uint8_t ucHourHigh;
    uint8_t ucHourLow;
    uint8_t ucMinHigh;
    uint8_t ucMinLow;
    uint8_t ucSecHigh;
    uint8_t ucSecLow;
    uint8_t ucMinSecHigh;
    uint8_t ucMinSecLow;

    ucHourHigh = ulHour / 10;
    ucHourLow  = ulHour % 10;

    ucMinHigh  = ulMin / 10;
    ucMinLow   = ulMin % 10;

    ucSecHigh  = ulSec / 10;
    ucSecLow   = ulSec % 10;

    ucMinSecHigh = ulMinSec / 10;
    ucMinSecLow  = ulMinSec % 10;

    UTC->TIMER = (UTC->TIMER & 0x80000000) | (ulAMPM << UTC_TIMER_PM_Pos) |
                      (ucHourHigh   << UTC_TIMER_MRT_Pos) | (ucHourLow   << UTC_TIMER_MRU_Pos) |
                      (ucMinHigh    << UTC_TIMER_MT_Pos)  | (ucMinLow    << UTC_TIMER_MU_Pos) |
                      (ucSecHigh    << UTC_TIMER_ST_Pos)  | (ucSecLow    << UTC_TIMER_SU_Pos) |
                      (ucMinSecHigh << UTC_TIMER_HT_Pos)  | (ucMinSecLow << UTC_TIMER_HU_Pos);
}

/**
  * @brief  get the UTC timer change status.
  * @retval the UTC timer change flag
  */
uint32_t UTC_TimerChangeGet(void)
{
    return ((UTC->TIMER & UTC_TIMER_CH_Msk) >> UTC_TIMER_CH_Pos);
}

/**
  * @brief  get the UTC timer.
  * @param  ulAMPM is 0:AM or 1:PM.
  * @param  ulHour is hours value.
  * @param  ulMin is minutes value.
  * @param  ulSec is seconds value.
  * @param  ulMinSec is hundredths of a second value.
  * @param  ulRegData is the regdata to export.
  * @retval None
  */
void UTC_TimerGet(uint8_t *ulAMPM, uint8_t *ulHour, uint8_t *ulMin, uint8_t *ulSec, uint8_t *ulMinSec, uint32_t ulRegData)
{
    uint32_t ulReg;

    if(ulRegData)
		ulReg = ulRegData;
	else
    	ulReg = UTC->TIMER;

    *ulAMPM = (ulReg & UTC_TIMER_PM_Msk) >> UTC_TIMER_PM_Pos;

    *ulHour = ((ulReg & UTC_TIMER_MRT_Msk) >> UTC_TIMER_MRT_Pos) * 10 + ((ulReg & UTC_TIMER_MRU_Msk) >> UTC_TIMER_MRU_Pos);

    *ulMin = ((ulReg & UTC_TIMER_MT_Msk) >> UTC_TIMER_MT_Pos) * 10 + ((ulReg & UTC_TIMER_MU_Msk) >> UTC_TIMER_MU_Pos);

    *ulSec = ((ulReg & UTC_TIMER_ST_Msk) >> UTC_TIMER_ST_Pos) * 10 + ((ulReg & UTC_TIMER_SU_Msk) >> UTC_TIMER_SU_Pos);

    *ulMinSec = ((ulReg & UTC_TIMER_HT_Msk) >> UTC_TIMER_HT_Pos) * 10 + ((ulReg & UTC_TIMER_HU_Msk) >> UTC_TIMER_HU_Pos);
}

/**
  * @brief  set the UTC timer alarm.
  * @param  ulAMPM is 0:AM or 1:PM.
  * @param  ulHour is hours value.
  * @param  ulMin is minutes value.
  * @param  ulSec is seconds value.
  * @param  ulMS is hundredths of a second value.
  * @retval None
  */
void UTC_TimerAlarmSet(uint32_t ulAMPM, uint32_t ulHour, uint32_t ulMin, uint32_t ulSec, uint32_t ulMS)
{
    uint8_t ucHourHigh;
    uint8_t ucHourLow;
    uint8_t ucMinHigh;
    uint8_t ucMinLow;
    uint8_t ucSecHigh;
    uint8_t ucSecLow;
    uint8_t ucMinSecHigh;
    uint8_t ucMinSecLow;
    uint32_t ulClkCnt;
    ucHourHigh = ulHour / 10;
    ucHourLow  = ulHour % 10;

    ucMinHigh  = ulMin / 10;
    ucMinLow   = ulMin % 10;

    ucSecHigh  = ulSec / 10;
    ucSecLow   = ulSec % 10;

    ucMinSecHigh = (ulMS/10) / 10;
    ucMinSecLow  = (ulMS/10) % 10;

    UTC->ALARM_TIMER = (ulAMPM << UTC_ALARM_TIMER_PM_Pos)  |
                            (ucHourHigh   << UTC_ALARM_TIMER_MRT_Pos) | (ucHourLow   << UTC_ALARM_TIMER_MRU_Pos) |
                            (ucMinHigh    << UTC_ALARM_TIMER_MT_Pos)  | (ucMinLow    << UTC_ALARM_TIMER_MU_Pos) |
                            (ucSecHigh    << UTC_ALARM_TIMER_ST_Pos)  | (ucSecLow    << UTC_ALARM_TIMER_SU_Pos) |
                            (ucMinSecHigh << UTC_ALARM_TIMER_HT_Pos)  | (ucMinSecLow << UTC_ALARM_TIMER_HU_Pos);
	ulClkCnt = (ulMS%10)*32;
	if(ulClkCnt < 160)
		UTC->ALARM_CLK_CNT = ((1<<15)|(ulClkCnt + 256));
	else
		UTC->ALARM_CLK_CNT = ((1<<15)|(ulClkCnt - 160));
}


/**
  * @brief  set the UTC calendar.
  * @param  ulCentury is century value.
  * @param  ulYear is year value.
  * @param  ulMonth is month value.
  * @param  ulDate is date value.
  * @param  ulDay is day value.
  * @retval None
  */
void UTC_CalSet(uint32_t ulCentury, uint32_t ulYear, uint32_t ulMonth, uint32_t ulDate, uint32_t ulDay)
{
    uint8_t ucCenturyHigh;
    uint8_t ucCenturyLow;
    uint8_t ucYearHigh;
    uint8_t ucYearLow;
    uint8_t ucMonthHigh;
    uint8_t ucMonthLow;
    uint8_t ucDateHigh;
    uint8_t ucDateLow;

    ucCenturyHigh = ulCentury / 10;
    ucCenturyLow  = ulCentury % 10;

    ucYearHigh = ulYear / 10;
    ucYearLow  = ulYear % 10;

    ucMonthHigh = ulMonth / 10;
    ucMonthLow  = ulMonth % 10;

    ucDateHigh = ulDate / 10;
    ucDateLow  = ulDate % 10;

    UTC->CAL = (UTC->CAL & 0x80000000) |
                                (ucCenturyHigh << UTC_CAL_CT_Pos) | (ucCenturyLow << UTC_CAL_CU_Pos) |
                                (ucYearHigh    << UTC_CAL_YT_Pos) | (ucYearLow    << UTC_CAL_YU_Pos) |
                                (ucMonthHigh   << UTC_CAL_MT_Pos) | (ucMonthLow   << UTC_CAL_MU_Pos) |
                                (ucDateHigh    << UTC_CAL_DT_Pos) | (ucDateLow    << UTC_CAL_DU_Pos) |
                                (ulDay         << UTC_CAL_DAY_Pos);

    UTC->STATUS |= UTC_STATUS_VALID_CAL_Msk;
}

/**
  * @brief  get the UTC calendar change status.
  * @retval the UTC calendar change flag
  */
uint32_t UTC_CalChangeGet(void)
{
    return ((UTC->CAL & UTC_CAL_CH_Msk) >> UTC_CAL_CH_Pos);
}

/**
  * @brief  get the UTC calendar.
  * @param  ulCentury is century value.
  * @param  ulYear is year value.
  * @param  ulMonth is month value.
  * @param  ulDate is date value.
  * @param  ulDay is day value.
  * @param  ulRegData is the regdata for export.
  * @retval None
  */
void UTC_CalGet(uint8_t *ulCentury, uint8_t *ulYear, uint8_t *ulMonth, uint8_t *ulDate, uint8_t *ulDay, uint32_t ulRegData)
{
    uint32_t ulReg;

    if(ulRegData) {
        ulReg = ulRegData;
    } else {
        ulReg = UTC->CAL;
    }

    *ulCentury = ((ulReg & UTC_CAL_CT_Msk) >> UTC_CAL_CT_Pos) * 10 + ((ulReg & UTC_CAL_CU_Msk) >> UTC_CAL_CU_Pos);

    *ulYear = ((ulReg & UTC_CAL_YT_Msk) >> UTC_CAL_YT_Pos) * 10 + ((ulReg & UTC_CAL_YU_Msk) >> UTC_CAL_YU_Pos);

    *ulMonth = ((ulReg & UTC_CAL_MT_Msk) >> UTC_CAL_MT_Pos) * 10 + ((ulReg & UTC_CAL_MU_Msk) >> UTC_CAL_MU_Pos);

    *ulDate = ((ulReg & UTC_CAL_DT_Msk) >> UTC_CAL_DT_Pos) * 10 + ((ulReg & UTC_CAL_DU_Msk) >> UTC_CAL_DU_Pos);

    *ulDay = (ulReg & UTC_CAL_DAY_Msk) >> UTC_CAL_DAY_Pos;
}

/**
  * @brief  set the UTC calendar alarm.
  * @param  ulMonth is month value.
  * @param  ulDate is date value.
  * @retval None
  */
void UTC_CalAlarmSet(uint32_t ulMonth, uint32_t ulDate)
{
    uint8_t ucMonthHigh;
    uint8_t ucMonthLow;
    uint8_t ucDateHigh;
    uint8_t ucDateLow;

    ucMonthHigh = ulMonth / 10;
    ucMonthLow  = ulMonth % 10;

    ucDateHigh = ulDate / 10;
    ucDateLow  = ulDate % 10;

    UTC->ALARM_CAL = (ucMonthHigh << UTC_ALARM_CAL_MT_Pos) | (ucMonthLow << UTC_ALARM_CAL_MU_Pos) |
                          (ucDateHigh  << UTC_ALARM_CAL_DT_Pos) | (ucDateLow  << UTC_ALARM_CAL_DU_Pos);
}

/**
  * @brief  get the UTC calendar alarm.
  * @param  ulMonth is month value.
  * @param  ulDate is date value.
  * @retval None
  */
void UTC_CalAlarmGet(uint8_t *ulMonth, uint32_t *ulDate)
{
    uint32_t ulReg;

    ulReg = UTC->ALARM_CAL;

    *ulMonth = ((ulReg & UTC_ALARM_CAL_MT_Msk) >> UTC_ALARM_CAL_MT_Pos) * 10 + ((ulReg & UTC_ALARM_CAL_MU_Msk) >> UTC_ALARM_CAL_MU_Pos);

    *ulDate = ((ulReg & UTC_ALARM_CAL_DT_Msk) >> UTC_ALARM_CAL_DT_Pos) * 10 + ((ulReg & UTC_ALARM_CAL_DU_Msk) >> UTC_ALARM_CAL_DU_Pos);
}

/**
  * @brief  Enable the UTC alarm.
  * @param  ulAlarmFlags can be any combination of the following values:
  *     @arg UTC_ALARM_MIN_SEC
  *     @arg UTC_ALARM_SEC
  *     @arg UTC_ALARM_MIN
  *     @arg UTC_ALARM_HOUR
  *     @arg UTC_ALARM_DATE
  *     @arg UTC_ALARM_MONTH
  *     @arg UTC_ALARM_ALL
  * @note   This register is used to set the fields that can trigger an alarm.
            Setting a bit enables the corresponding time units as a trigger event.
            The alarm triggering causes an event to be generated which is set in the event register.
  * @retval None
  */
void UTC_AlarmEnable(uint32_t ulAlarmFlags)
{
    UTC->ALARM_EN |= ulAlarmFlags;
}

/**
  * @brief  Disable the UTC alarm.
  * @param  ulAlarmFlags can be any combination of the following values:
  *     @arg UTC_ALARM_MIN_SEC
  *     @arg UTC_ALARM_SEC
  *     @arg UTC_ALARM_MIN
  *     @arg UTC_ALARM_HOUR
  *     @arg UTC_ALARM_DATE
  *     @arg UTC_ALARM_MONTH
  *     @arg UTC_ALARM_ALL
  * @retval None
  */
void UTC_AlarmDisable(uint32_t ulAlarmFlags)
{
    UTC->ALARM_EN &= (~ulAlarmFlags);
}

/**
  * @brief  Enable the UTC interrupt.
  * @param  ulIntFlags can be any combination of the following values:
  *     @arg UTC_INT_MIN_SEC
  *     @arg UTC_INT_SEC
  *     @arg UTC_INT_MIN
  *     @arg UTC_INT_HOUR
  *     @arg UTC_INT_DATE
  *     @arg UTC_INT_MONTH
  *     @arg UTC_INT_ALARM
  *     @arg UTC_INT_ALL
  * @retval None
  */
void UTC_IntEnable(uint32_t ulIntFlags)
{
    UTC->INT_EN |= ulIntFlags;
}

/**
  * @brief  Disable the UTC interrupt.
  * @param  ulIntFlags can be any combination of the following values:
  *     @arg UTC_INT_MIN_SEC
  *     @arg UTC_INT_SEC
  *     @arg UTC_INT_MIN
  *     @arg UTC_INT_HOUR
  *     @arg UTC_INT_DATE
  *     @arg UTC_INT_MONTH
  *     @arg UTC_INT_ALARM
  *     @arg UTC_INT_ALL
  * @retval None
  */
void UTC_IntDisable(uint32_t ulIntFlags)
{
    UTC->INT_DIS |= ulIntFlags;
}

/**
  * @brief  set mask the UTC interrupt.
  * @param  ulIntFlags can be any combination of the following values:
  *     @arg UTC_INT_MIN_SEC
  *     @arg UTC_INT_SEC
  *     @arg UTC_INT_MIN
  *     @arg UTC_INT_HOUR
  *     @arg UTC_INT_DATE
  *     @arg UTC_INT_MONTH
  *     @arg UTC_INT_ALARM
  *     @arg UTC_INT_ALL
  * @retval None
  */
void UTC_IntMaskSet(uint32_t ulIntMask)
{
    UTC->INT_MSK |= ulIntMask;
}

/**
  * @brief  get the mask of the UTC interrupt.
  * @retval utc interrupt mask
  */
uint32_t UTC_IntMaskGet(void)
{
    return UTC->INT_MSK;
}

/**
  * @brief  set the UTC interrupt status.
  * @param  ulIntFlags can be any combination of the following values:
  *     @arg UTC_INT_MIN_SEC
  *     @arg UTC_INT_SEC
  *     @arg UTC_INT_MIN
  *     @arg UTC_INT_HOUR
  *     @arg UTC_INT_DATE
  *     @arg UTC_INT_MONTH
  *     @arg UTC_INT_ALARM
  *     @arg UTC_INT_ALL
  * @retval None
  */
void UTC_IntStatusSet(uint32_t ulIntFlags)
{
    UTC->INT_STAT |= ulIntFlags;
}

/**
  * @brief  get the UTC interrupt status and clear the interrupt.
  * @retval the UTC interrupt status
  */
uint32_t UTC_IntStatusGet(void)
{
    return UTC->INT_STAT;
}

/**
  * @brief  get the UTC valid status.
  * @note   This register contains 'data valid' flags for data that has been input to the UTC.
            Invalid entry checking is performed after data is written in and when data is copied back
            from the counters to the register map. The approprite field bit is set if the data is valid.
            The counter will not start if any of the valid flags is not set.
  *     @arg UTC_STATUS_VALID_TIMER_Msk
  *     @arg UTC_STATUS_VALID_CAL_Msk
  *     @arg UTC_STATUS_VALID_ALARM_TIMER_Msk
  *     @arg UTC_STATUS_VALID_ALARM_CAL_Msk
  * @retval the UTC valid status
  */
uint32_t UTC_ValidStatusGet(void)
{
    return UTC->STATUS;
}

/**
  * @brief  get the UTC interrupt status and clear the interrupt.
  * @param  ulKeepUTC is as following:
  *     @arg   1: the time and calendar registers and any other registers which directly affect or
                  are affected by the time and calendar register are NOT reset when the APB reset signals are applied.
  *     @arg   0: The APB reset signal will reset every register except the keep UTC and control registers.
  * @retval None
  */
void UTC_KeepRun(uint32_t ulKeepUTC)
{
    UTC->KEEP_UTC = ulKeepUTC;
}

/**
  * @brief  set the UTC clock count.
  * @param  ulClkCnt is the UTC clock count value.
  * @note   When DIVCNT at Control register is set, write to this register would update the divcounter of generating 10ms/1s pulse initial value;
            when DIVCNT bit isn't set, write to this register doesn't take effect
            When xtal32768_sel=0; Only lower 9 bits are valid, higher bits are 0; otherwise all the 15 bits are valid
  * @retval None
  */
void UTC_ClkCntSet(uint32_t ulClkCnt)
{
    if(ulClkCnt < 160)
        UTC->CLK_CNT = ulClkCnt + 256;
    else
        UTC->CLK_CNT = ulClkCnt - 160;
}

/**
  * @brief  get the UTC clock count.
  * @param  ulRegData is the regdata for export.
  * @note   clock counter value showes the current counter for the 32KHz clock, RTC minimum accuracy 10ms when xtal32768_sel=0;
  * @retval the current counter for the 32KHz clock
  */
uint32_t UTC_ClkCntGet(uint32_t ulRegData)
{
	volatile uint32_t reg_value;
	if(ulRegData)
		reg_value = ulRegData;
	else
		reg_value = UTC->CLK_CNT;

	reg_value &= 0x1FF;
	if(reg_value & 0x100)
		return (reg_value - 256);
	else
		return (reg_value + 160);
}

uint32_t UTC_ClkCntConvert(uint32_t ulRegData)
{
	uint32_t reg_value;

	reg_value = ulRegData & 0x1FF;

	if(reg_value & 0x100)
		return (reg_value - 256);
	else
		return (reg_value + 160);

}

/**
  * @brief  Registers an interrupt handler for UTC interrupt.
  * @param  g_pRAMVectors is the global interrupt vectors table.
  * @param  pfnHandler is a pointer to the function to be called when the UTC interrupt occurs.
  * @retval None
  */
void UTC_IntRegister(uint32_t *g_pRAMVectors, void (*pfnHandler)(void))
{
    IntRegister(INT_UTC, g_pRAMVectors, pfnHandler);

    IntEnable(INT_UTC);
}

/**
  * @brief  Unregisters an interrupt handler for the UTC interrupt.
  * @param  g_pRAMVectors is the global interrupt vectors table.
  * @retval None
  */
void UTC_IntUnregister(uint32_t *g_pRAMVectors)
{
	IntDisable(INT_UTC);

	IntUnregister(INT_UTC, g_pRAMVectors);
}

/**
  * @brief  get the xtal 32768 selection.
  * @retval the xtal 32768 selection.
  */
uint8_t UTC_32768Get(void)
{
    return (UTC->SEL_32768  & 0x01);
}

void UTC_AlarmCntCheckEnable(void)
{
    UTC->ALARM_CLK_CNT |= UTC_ALARM_CNT_CHECK_EN_Msk;
}

void UTC_AlarmCntCheckDisable(void)
{
    UTC->ALARM_CLK_CNT &= ~UTC_ALARM_CNT_CHECK_EN_Msk;
}

void UTC_AlarmCntCFG(uint32_t ulCnt)
{
        UTC->ALARM_CLK_CNT = (UTC->ALARM_CLK_CNT & ~(UTC_ALARM_CNT_32768_CFG_Msk)) | ulCnt;

}

uint32_t UTC_AlarmCntGet(void)
{
        return (UTC->ALARM_CLK_CNT & UTC_ALARM_CNT_32768_CFG_Msk);
}


/**
  * @brief  config the UTC_WDT control register.
  * @param  ulConfig is the configuration of the UTC_WDT control.
  *   This parameter can be any combination of the following values:
  *     @arg UTC_WDT_CTRL_EN_Msk:           UTC_WDT enable
  *     @arg UTC_WDT_CTRL_MODE_LONG:        long timer mode
  *     @arg UTC_WDT_CTRL_MODE_SHORT:       short timer mode
  *     @arg UTC_WDT_CTRL_RESET_Msk:        reset enable
  *     @arg UTC_WDT_CTRL_REPEAT_EN_Msk:    repeat enable
  *     @arg UTC_WDT_CTRL_REPEAT_MAX_VALUE: repeat to maximum value
  *     @arg UTC_WDT_CTRL_REPEAT_REG_VALUE: repeat to register value
  *     @arg UTC_WDT_CTRL_INT_EN_Msk:       interrupt enable
  *     @arg UTC_WDT_CTRL_CLOCK_TICK_Msk:   clktick mode enable
  *     @arg UTC_WDT_CTRL_DATA_VALID_Msk:   longtimer data valid enable
  * @retval None
  */
void UTC_WDTCtrlConfig(uint32_t ulConfig)
{
    UTC->WDT_CTRL = ulConfig;
}

/**
  * @brief  Enable the UTC_WDT.
  * @retval None
  */
void UTC_WDTEnable(void)
{
    UTC->WDT_CTRL |= UTC_WDT_CTRL_EN_Msk;
}

/**
  * @brief  Disable the UTC_WDT.
  * @retval None
  */
void UTC_WDTDisable(void)
{
    UTC->WDT_CTRL &= ~UTC_WDT_CTRL_EN_Msk;
}

/**
  * @brief  set the UTC_WDT long timer data valid.
  * @retval None
  */
void UTC_WDTLongTimerDataValid(void)
{
    UTC->WDT_CTRL |= UTC_WDT_CTRL_DATA_VALID_Msk;
}

/**
  * @brief  set the UTC_WDT long timer data invalid.
  * @retval None
  */
void UTC_WDTLongTimerDataInvalid(void)
{
    UTC->WDT_CTRL &= ~UTC_WDT_CTRL_DATA_VALID_Msk;
}

/**
  * @brief  config the UTC_WDT tickclock accuracy.
  * @param  ucAccuracy is accuracy of the UTC_WDT tickclock as following:
  *     @arg 0: utc_clk /2;
  *     @arg 1: utc_clk /4;
  *     @arg 2: utc_clk /8;
  *     @arg 3: utc_clk /16;
  *     @arg 4: utc_clk /32;
  *     @arg 5: utc_clk /64;
  *     @arg 6: utc_clk/128;
  *     @arg 7: per second;
  *     @arg 8: per ten seconds;
  *     @arg 9: per minute;
  *     @arg a: per ten minutes.
  * @retval None
  */
void UTC_WDTTickConfig(uint8_t ucAccuracy)
{
    UTC->WDT_TICK_CONFIG = (UTC->WDT_TICK_CONFIG & ~(UTC_WDT_TICK_CONFIG_ACCURACY_Msk)) | (ucAccuracy << UTC_WDT_TICK_CONFIG_ACCURACY_Pos);
}

/**
  * @brief  set the UTC_WDT time data of long timer mode.
  * @param  ulTime is the UTC_WDT time data of long timer mode.
  * @note   When long timer mode this registers [31:0] is the time point for watchdog timer generate interrupt.
  * @retval None
  */
void UTC_WDTLongTimerDataSet(uint32_t ulTime)
{
    UTC->WDT_TIMER_DATA = ulTime;
}

/**
  * @brief  set the UTC_WDT time data of short timer mode.
  * @param  ucStartValue is the UTC_WDT time data of short timer mode.
  * @note   When long timer mode this registers is the time for watchdog timer generate interrupt.
            This register used both long timer mode and short timer mode.
            Short timer mode: time data[3:0] short timer mode start value. Counting frequency is utc_clk /128. It's maximum value is 16*128(utc_clk cycles).
  * @retval None
  */
void UTC_WDTShortTimerDataSet(uint8_t ucStartValue)
{
    UTC->WDT_TIMER_DATA = (UTC->WDT_TIMER_DATA & ~(UTC_WDT_TIMER_DATA_START_VALUE_Msk)) | (ucStartValue << UTC_WDT_TIMER_DATA_START_VALUE_Pos);
}

/**
  * @brief  set the UTC_WDT time data of short timer mode.
  * @param  ucActive is the UTC_WDT time data of short timer mode.
  * @note   Long timer mode: time data[8];
            Short timer mode: If short timer down count after system wake up.
            1: start after wake up.
            0: ignore wake up. Down counting after write new start data.
  * @retval None
  */
void UTC_WDTStartAfterWakeupSet(uint8_t ucActive)
{
    UTC->WDT_TIMER_DATA = (UTC->WDT_TIMER_DATA & ~(UTC_WDT_START_AFTER_WAKEUP_Msk)) | (ucActive << UTC_WDT_START_AFTER_WAKEUP_Pos);
}

/**
  * @brief  set the UTC_WDT calendar data of short timer mode.
  * @param  ulCalendar is the UTC_WDT calendar data of short timer mode.
  * @note   Long timer mode generate interrupt calendar data.
  * @retval None
  */
void UTC_WDTCalendarDataSet(uint32_t ulCalendar)
{
    UTC->WDT_CALENDAR_DATA = ulCalendar;
}

/**
  * @brief  read the UTC_WDT interrupt status.
  * @retval the UTC_WDT interrupt status
  */
uint32_t UTC_WDTIntStatusGet(void)
{
    return UTC->WDT_INT_STAT;
}

/**
  * @brief  clear the UTC_WDT interrupt.
  * @param  ucInt is the UTC_WDT interrupt bits as following:
  *     @arg UTC_WDT_INT_STAT_Msk
  * @retval None
  */
void UTC_WDTClearInt(uint8_t ucInt)
{
    UTC->WDT_INT_STAT |= ucInt;
}

/**
  * @brief  set the UTC_WDT interrupt mask.For 1200 B0.
  * @param  WATCHDOG_INT_MASK This parameter can be one of the following values:
  *         @arg @ref UTC_WDT_WATCHDOGCTRL_WATCHDOG_INT_MASK_NONE
  *         @arg @ref UTC_WDT_WATCHDOGCTRL_WATCHDOG_INT_MASK_SECOND
  *         @arg @ref UTC_WDT_WATCHDOGCTRL_WATCHDOG_INT_MASK_MINUTE
  *         @arg @ref UTC_WDT_WATCHDOGCTRL_WATCHDOG_INT_MASK_AMPM_HOUR
  *         @arg @ref UTC_WDT_WATCHDOGCTRL_WATCHDOG_INT_MASK_MONTH
  *         @arg @ref UTC_WDT_WATCHDOGCTRL_WATCHDOG_INT_MASK_DAY
  *         @arg @ref UTC_WDT_WATCHDOGCTRL_WATCHDOG_INT_MASK_CENTURY_YEAR
  * @retval None
  */
void UTC_WDTSetWatchdogIntMask(uint32_t WATCHDOG_INT_MASK)
{
	UTC->WDT_CTRL = ((UTC->WDT_CTRL) & (~UTC_WDT_WATCHDOGCTRL_WATCHDOG_INT_MASK)) | WATCHDOG_INT_MASK;
}

/**
 * @brief //世纪，年，月，日，星期，Long Timer Mode 星期不匹配
 * 
 * @param ulCentury 
 * @param ulYear 
 * @param ulMonth 
 * @param ulDate 
 * @param ulDay 
 */
void UTCWDT_Long_Timer_CalSet(uint32_t ulCentury, uint32_t ulYear, uint32_t ulMonth, uint32_t ulDate, uint32_t ulDay)
{
    uint8_t ucCenturyHigh;
    uint8_t ucCenturyLow;
    uint8_t ucYearHigh;
    uint8_t ucYearLow;
    uint8_t ucMonthHigh;
    uint8_t ucMonthLow;
    uint8_t ucDateHigh;
    uint8_t ucDateLow;
    
    ucCenturyHigh = ulCentury / 10;
    ucCenturyLow  = ulCentury % 10;
    
    ucYearHigh = ulYear / 10;
    ucYearLow  = ulYear % 10;
    
    ucMonthHigh = ulMonth / 10;
    ucMonthLow  = ulMonth % 10;
    
    ucDateHigh = ulDate / 10;
    ucDateLow  = ulDate % 10;

    HWREG(UTC_WDT_CALENDAR_DATA) =	(ucCenturyHigh << UTC_CAL_CT_Pos) | (ucCenturyLow << UTC_CAL_CU_Pos) | 
                                	(ucYearHigh    << UTC_CAL_YT_Pos) | (ucYearLow    << UTC_CAL_YU_Pos) | 
                                	(ucMonthHigh   << UTC_CAL_MT_Pos) | (ucMonthLow   << UTC_CAL_MU_Pos) | 
                                	(ucDateHigh    << UTC_CAL_DT_Pos) | (ucDateLow    << UTC_CAL_DU_Pos) | 
                                	(ulDay         << UTC_CAL_DAY_Pos);
}

/**
 * @brief   AMPM,时，分，秒，10ms，Long Timer Mode , 10ms不匹配，设置时，Longtimer_data_valid（0x44-bit7）,要先清零，然后设置时间，然后置1；
 * @param ulAMPM 
 * @param ulHour 
 * @param ulMin 
 * @param ulSec 
 * @param ulMinSec 
 */
void UTCWDT_Long_Timer_TimerSet(uint32_t ulAMPM, uint32_t ulHour, uint32_t ulMin, uint32_t ulSec, uint32_t ulMinSec)
{
    uint8_t ucHourHigh;
    uint8_t ucHourLow;
    uint8_t ucMinHigh;
    uint8_t ucMinLow;
    uint8_t ucSecHigh;
    uint8_t ucSecLow;
    uint8_t ucMinSecHigh;
    uint8_t ucMinSecLow;

	//UTC_WDTLongTimerDataInvalid();

    
    ucHourHigh = ulHour / 10;
    ucHourLow  = ulHour % 10;
    
    ucMinHigh  = ulMin / 10;
    ucMinLow   = ulMin % 10;
    
    ucSecHigh  = ulSec / 10;
    ucSecLow   = ulSec % 10;
    
    ucMinSecHigh = ulMinSec / 10;
    ucMinSecLow  = ulMinSec % 10;

    HWREG(UTC_WDT_TIMER_DATA) =	(ulAMPM << UTC_TIMER_PM_Pos) | 
                      			(ucHourHigh   << UTC_TIMER_MRT_Pos) | (ucHourLow   << UTC_TIMER_MRU_Pos) | 
                      			(ucMinHigh    << UTC_TIMER_MT_Pos)  | (ucMinLow    << UTC_TIMER_MU_Pos) | 
                      			(ucSecHigh    << UTC_TIMER_ST_Pos)  | (ucSecLow    << UTC_TIMER_SU_Pos) | 
                      			(ucMinSecHigh << UTC_TIMER_HT_Pos)  | (ucMinSecLow << UTC_TIMER_HU_Pos);

    //UTC_WDTLongTimerDataValid();
}

const uint8_t Month_days[13]={0,31,28,31,30,31,30,31,31,30,31,30,31};

/*判断是否为闰年*/
uint8_t UTC_WDT_IsLeapYear(uint32_t year)
{
    if(((year%4==0)&&(year%100!=0))||(year%400==0))
    {
        return 1;
    }
    return 0;
}

//返回月的天数
uint8_t UTC_WDT_CountMonthDays(uint32_t year, uint8_t ulMonth)
{
    if(ulMonth == 2)
    {
        return (28 + UTC_WDT_IsLeapYear(year));
    }
    else
    {
        return Month_days[ulMonth];
    }
}


void UTC_WDTLongTimerIncrementFeedSecond(uint32_t IncSec)
{
    uint8_t ulAMPM;    //24小时制，固定为0;
    uint8_t ulHour;
    uint8_t ulMin;
    uint8_t ulSec;
    uint8_t ulMinSec;	//wdt 不匹配

    uint32_t tmp_Sec;      //秒     增加后的 中间值;
    uint32_t tmp_Min;      //分钟    增加后的 中间值;
    uint32_t tmp_Hour;     //小时    增加后的 中间值;

    UTC_WDTLongTimerDataInvalid();

    if(IncSec > (24*60*60 -1))
    {
        IncSec = (24*60*60 -1);
    }

    UTC_TimerGet(&ulAMPM, &ulHour, &ulMin, &ulSec, &ulMinSec, 0);

    tmp_Sec = ulSec + IncSec;     
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
//    utc_cnt_delay(1);

    UTCWDT_Long_Timer_TimerSet(ulAMPM,ulHour,ulMin,ulSec,ulMinSec);
    UTC_WDTLongTimerDataValid();
}



/*低精度的延时函数，以utc cnt为计数单位（约30us），即32K计数周期，适用于对延时准确度不太高的场景。
*只依赖硬件utc cnt计数变化，不依赖任何全局变量参数，不受APCore频率变化影响，更适合驱动层面使用（内部硬件状态变化有些也是基于32K时钟频率）。
*传入参数为32K时钟周期数量，延时时间为delay_cnt/32K频率~（delay_cnt+1）/32K频率。如 utc_cnt_delay(10)，实际延时10~11个utccnt。
*另外该延时函数功耗相对较低，在功耗敏感的场景且精确度要求不太高的情况下，可以使用该函数延时。
*如果需要高精度的延时，请使用delay_func_us*/
void utc_cnt_delay(uint32_t delay_cnt)
{
    unsigned long i;
	unsigned long cnt;

	//add 1 to make sure delay>delay_cnt
    for(i = 0; i < delay_cnt+1; i++)
    {
		cnt = HWREG(UTC_CLK_CNT);
		while(cnt == HWREG(UTC_CLK_CNT));
    }
}


/**
 * Judge if the year is leapyear.
 *
 * @param[in]   year      The year to judge.
 * @return      1 or 0.
 */
__FLASH_FUNC int clock_is_leapyear(int year)
{
	return (year % 400) ? ((year % 100) ? ((year % 4) ? 0 : 1) : 0) : 1;
}

static const uint16_t g_daysbeforemonth[13] =
{
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};

/**
 * Claculate the days before a specified month,month 0-based.
 *
 * @param[in]   month      The specified month.
 * @param[in]   year      Leapyear check.
 * @return      Sum days.
 */
__FLASH_FUNC int clock_days_before_month(int month, int leapyear)
{
	int retval = g_daysbeforemonth[month];

	if (month >= 2 && leapyear)
	{
	  retval++;
	}

	return retval;
}

/**
 * Convert calendar to utc time.
 *
 * @param[in]   year      The specified year.
 * @param[in]   month      The specified month.
 * @param[in]   day      The specified day.
 * @return      Sum days.
 */
__FLASH_FUNC uint32_t clock_calendar_to_utc(int year, int month, int day)
{
	uint32_t days;

	/* Years since epoch in units of days (ignoring leap years). */
	days = (uint32_t)((year - 1970) * 365);
	/* Add in the extra days for the leap years prior to the current year. */
	days += (uint32_t)((year - 1969) >> 2);
	/* Add in the days up to the beginning of this month. */
	days += (uint32_t)clock_days_before_month(month, clock_is_leapyear(year));
	/* Add in the days since the beginning of this month (days are 1-based). */
	days += (uint32_t)(day - 1);
	/* Then convert the seconds and add in hours, minutes, and seconds */
	return days;
}


__FLASH_FUNC int clock_dayoftheweek(int mday, int month, int year)
{
  if((month == 1) || (month == 2))
	{
		month += 12;
		year--;
	}

	return (mday + 2*month + 3*(month+1)/5 + year + year/4 - year/100 + year/400) % 7 + 1;
}

__FLASH_FUNC void clock_utc_to_calendar(uint64_t days, int *year, int *month, int *day)
{
	int  value;
	int  min;
	int  max;
	int  leapyear;

	uint64_t  tmp;

	/* There is one leap year every four years, so we can get close with the
	* following:
	*/
	value   = (int)(days / (4 * 365 + 1));       /* Number of 4-years periods since the epoch */
	days   -= (uint64_t)(value * (4 * 365 + 1)); /* Remaining days */
	value <<= 2;                                 /* Years since the epoch */

	/* Then we will brute force the next 0-3 years */
	for (;;)
	{
		/* Is this year a leap year (we'll need this later too) */
		leapyear = clock_is_leapyear(value + 1970);

		/* Get the number of days in the year */
		tmp = (leapyear ? 366 : 365);

		/* Do we have that many days? */
		if (days >= tmp)
		{
			/* Yes.. bump up the year */
			value++;
			days -= tmp;
		}
		else
		{
			/* Nope... then go handle months */
			break;
		}
	}

	/* At this point, value has the year and days has number days into this year */
	*year = 1970 + value;

	/* Handle the month (zero based) */
	min = 0;
	max = 11;

	do {
			/* Get the midpoint */
			value = (min + max) >> 1;

			/* Get the number of days that occurred before the beginning of the month
			* following the midpoint.
			*/
			tmp = (uint64_t)clock_days_before_month(value + 1, leapyear);

			/* Does the number of days before this month that equal or exceed the
			* number of days we have remaining?
			*/
			if (tmp > days)
			{
				/* Yes.. then the month we want is somewhere from 'min' and to the
				* midpoint, 'value'.  Could it be the midpoint?
				*/
				tmp = (uint64_t)clock_days_before_month(value, leapyear);
				if (tmp > days)
				{
					/* No... The one we want is somewhere between min and value-1 */
					max = value - 1;
				}
				else
				{
					/* Yes.. 'value' contains the month that we want */
					break;
				}
			}
			else
			{
				/* No... The one we want is somwhere between value+1 and max */
				min = value + 1;
			}

			/* If we break out of the loop because min == max, then we want value
			* to be equal to min == max.
			*/
			value = min;
		} while (min < max);

	/* The selected month number is in value. Subtract the number of days in the
	* selected month
	*/
	days -= (uint64_t)clock_days_before_month(value, leapyear);

	/* At this point, value has the month into this year (zero based) and days has
	* number of days into this month (zero based)
	*/
	*month = value + 1; /* 1-based */
	*day   = (int)(days + 1);  /* 1-based */
}

// 根据year/mon/day/hour/min/sec，获取相对1970/1/1的毫秒偏移量
uint64_t xy_mktime(RTC_TimeTypeDef *tp)
{
   int msec,sec,min,hour,mday,mon,year;

   uint64_t ret;

   msec = (int)tp->tm_msec,
   sec  = (int)tp->tm_sec;
   min  = (int)tp->tm_min;
   hour = (int)tp->tm_hour;
   mday = (int)tp->tm_mday;
   mon  = (int)tp->tm_mon;
   year = (int)tp->tm_year;

   if(sec==0&&min==0&&hour==0&&mday==0&&mon==0&&year==0)
   		return 0;

    /* 1..12 -> 11,12,1..10 */
    if (0 >= (int) (mon -= 2))
	{
        mon += 12;  /* Puts Feb last since it has leap day */
        year -= 1;
    }

	ret = (((((uint64_t) \
          (year/4 - year/100 + year/400 + 367*mon/12 + mday) \
          + (uint64_t)year*365 - 719499) * 24 \
		  + (uint64_t)hour) * 60              \
		  + (uint64_t)min)  * 60              \
		  + (uint64_t)sec)  * 1000            \
		  + (uint64_t)msec;

    return ret;
}

// 根据相对1970/1/1的毫秒偏移量，获取year/mon/day/hour/min/sec
__FLASH_FUNC void xy_gmtime(const uint64_t msec, RTC_TimeTypeDef *result)
{
  uint32_t year  = 0;
  uint32_t month = 0;
  uint32_t day   = 0;
  uint32_t hour  = 0;
  uint32_t min   = 0;
  uint32_t sec   = 0;

  uint64_t epoch = 0;
  uint64_t jdn   = 0;

  /* Get the seconds since the EPOCH */
  epoch = (msec)/1000;

  /* Convert to days, hours, minutes, and seconds since the EPOCH */
  jdn    = epoch / SEC_PER_DAY;
  epoch -= SEC_PER_DAY * jdn;

  hour   = (uint32_t)(epoch / SEC_PER_HOUR);
  epoch -= SEC_PER_HOUR * hour;

  min    = (uint32_t)epoch / SEC_PER_MIN;
  epoch -= SEC_PER_MIN * min;

  sec    = (uint32_t)epoch;

  /* Convert the days since the EPOCH to calendar day */
  clock_utc_to_calendar(jdn, (int *)&year, (int *)&month, (int *)&day);

  /* Then return the struct tm contents */
  result->tm_year  = year; /* Relative to 1900 */
  result->tm_mon   = month;   /* one-based */
  result->tm_mday  = day;         /* one-based */
  result->tm_hour  = hour;
  result->tm_min   = min;
  result->tm_sec   = sec;
  result->tm_msec  = (uint32_t)((msec)%1000);

  result->tm_wday  = (uint32_t)clock_dayoftheweek((int)day, (int)month, (int)year);
  result->tm_yday  = day + (uint32_t)clock_days_before_month((int)(result->tm_mon-1), clock_is_leapyear((int)year));
  result->tm_isdst = 0;
}
