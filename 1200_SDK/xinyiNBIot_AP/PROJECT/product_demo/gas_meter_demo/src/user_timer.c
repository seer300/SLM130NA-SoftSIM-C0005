/*****************************************************************************************************************************	 
 * user_timer.c
 ****************************************************************************************************************************/

#include "user_debug.h"
#include "mcu_adapt.h"
#include "user_timer.h"
#include "vmcu.h"
#include "user_adc.h"
#include "user_config.h"
#include "xy_event.h"
#include "user_eeprom.h"
#include "user_gpio.h"
#include "user_lcd.h"
#include "user_flash.h"
#include "type_adapt.h"

/*******************************************************************************************************/
/*******************************************************************************************************/
/******************************************  1s的lptimer定时  *******************************************/
/*******************************************************************************************************/
/*******************************************************************************************************/

extern void User_Trigger_RC32K_Cali(void);
__RAM_FUNC void UserLptimer1Callback()
{
    static uint32_t s_printf_lptimer_period = 0;
    static uint32_t s_i2c_test_period = 0;
    static uint32_t s_rc_cali_period = 0;
    static uint32_t s_bootcp_period = 0;
    static uint32_t s_switching_valve_test_period = 0;
    static uint32_t s_bak_power_detect_period = 0;
    static uint32_t s_flash_write_period = 0;

    s_printf_lptimer_period++;
    s_i2c_test_period++;
    s_switching_valve_test_period++;
    s_bak_power_detect_period++;
    s_rc_cali_period++;
    s_flash_write_period++;

    //获取世界时间
    UserGetTime();

    // 10s打印一次
    if (s_printf_lptimer_period == PRINTF_LPTIMER_PERIOD)
    {
        s_printf_lptimer_period = 0;
        jk_printf(" lptimer 1s wakeup\r\n");
    }

#if BOOT_CP_PERIOD_ON
    s_bootcp_period++;
    if (s_bootcp_period == BOOT_CP_PERIOD)
    {
        s_bootcp_period = 0;
        jk_printf("set cloud event\r\n");
        set_event(EVENT_CLOUD_SEND);
    }
#endif

    if (s_bak_power_detect_period == BAK_POWER_DETECT_PERIOD)
    {
        s_bak_power_detect_period = 0;
        set_event(BAK_POWER_DETECT_EVENT);
    }

    if (s_i2c_test_period == IIC_E2ROM_TEST_PERIOD)
    {
        s_i2c_test_period = 0;
        set_event(I2C_EVENT);
    }

    if (s_flash_write_period == FLASH_WRITE_PERIOD)
    {
        s_flash_write_period = 0;
        set_event(FLASH_WRITE_EVENT);
    }

#if SWITCHING_VALVE_PERIOD_ON
    if (s_switching_valve_test_period == SWITCHING_VALVE_TEST_PERIOD)
    {
        s_switching_valve_test_period = 0;
        set_event(USER_VALVE_EVENT);
    }
#endif

    if (s_rc_cali_period == RC_CALI_PERIOD)
    {
        s_rc_cali_period = 0;
        User_Trigger_RC32K_Cali();
        jk_printf("User_Trigger_RC32K_Cali\r\n");
    }
}

void UserLptimerInit(void)
{
    VmcuRtcSet(GAS_TIMER_PERIOD);
    VmcuRtcIrqReg(UserLptimer1Callback);
    VmcuRtcEn();
}

/*******************************************************************************************************/
/*******************************************************************************************************/
/***********************************  2ms硬件Timer检测碱电    *******************************************/
/*******************************************************************************************************/
/*******************************************************************************************************/

static uint16_t s_timer2_adc_alarm_cnt = 0;
static uint16_t s_timer2_printf_cnt = 0;

__RAM_FUNC void UserTimer2Callback()
{
    uint16_t main_power = UserMainPowerDetect();
    // 主电电压低于600mv
    if (main_power < 600)
    {
        s_timer2_adc_alarm_cnt++;
    }
    
    // 模拟扣碱电存储紧急数据,连续监测5次碱电电压异常
    if (s_timer2_adc_alarm_cnt >= 5) 
    {
        Stop_CP2(0, 1);
        UserEepromTest();
        UserValveStop();
        UserTimer2Stop();
        s_timer2_adc_alarm_cnt = 0;
        jk_printf("emergency ADC_AUX_ADC2 = %d\r\n", main_power);
    }

    if (s_timer2_printf_cnt >= 500) 
    {
        s_timer2_printf_cnt = 0;
        jk_printf("2ms ADC_AUX_ADC2 = %d\r\n", main_power);
    }
    s_timer2_printf_cnt++;
}

__TYPE_IRQ_FUNC void UserTimer2Stop(void)
{
    VmcuTimerStop(2);
}

void UserTimer2Init(void)
{
    // 打开timer2，2ms定时监测碱电
    VmcuTimerContinuousInit(2, 2);
    VmcuTimerContinuousIrqReg(2, UserTimer2Callback);
    VmcuTimerStart(2);
    s_timer2_adc_alarm_cnt = 0;
    s_timer2_printf_cnt = 0;
}