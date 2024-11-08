/*****************************************************************************************************************************	 
 * user_gpio.c
 ****************************************************************************************************************************/

#include "user_debug.h"
#include "mcu_adapt.h"
#include "user_gpio.h"
#include "vmcu.h"
#include "user_timer.h"
#include "type_adapt.h"

static uint32_t s_10ms_timer_cnt = 0;
__TYPE_IRQ_FUNC static void UserValveTimerCallback(void)
{
    s_10ms_timer_cnt++;

    // 开阀计时1400ms后停止
    if (s_10ms_timer_cnt >= SWITCHING_VALVE_HOLD_TIME/10)
    {
        //开阀完成允许进深睡
        LPM_UNLOCK(VALVE_DEEPSLEEP_LOCK);

        VmcuGpioWrite(51, 0);
        Time_EventStop(TIMER_LP_USER4);
        jk_printf("switching valve stop\r\n");
        VmcuTimerStop(2);
        s_10ms_timer_cnt = 0;
    }
}

// 开阀
void UserValveStart(void)
{
    //开阀期间不允许进深睡
	LPM_LOCK(VALVE_DEEPSLEEP_LOCK);

    jk_printf("switching valve start\r\n");
    VmcuGpioModeSet(48, 0x00);
    VmcuGpioWrite(48, 1);
    VmcuGpioModeSet(51, 0x00);
    VmcuGpioWrite(51, 0);
    HAL_Delay(200);
    VmcuGpioWrite(48, 0);
    VmcuGpioWrite(51, 1);
    s_10ms_timer_cnt = 0;
    Timer_AddEvent(TIMER_LP_USER4, 10, UserValveTimerCallback, 1);
    
#if CHECK_BAT_WHEN_BOOT_CP
    // 打开timer2，2ms定时监测碱电
    UserTimer2Init();
#endif
}

// 关阀
__TYPE_IRQ_FUNC void UserValveStop(void)
{
    VmcuGpioModeSet(48, 0x00);
    VmcuGpioModeSet(51, 0x00);
    VmcuGpioWrite(51, 0);
    VmcuGpioWrite(48, 1);
    HAL_Delay(200);
    VmcuGpioWrite(48, 0);
}

void UserValveFunc(void)
{
    // 模拟关开阀动作，先关阀，再开阀
    if(is_event_set(USER_VALVE_EVENT))
    {
        clear_event(USER_VALVE_EVENT);

        UserValveStart();
    }
}

/*******************************************************************************************************/
/*******************************************************************************************************/
/*********************************    其他gpio默认低电平    *********************************************/
/*******************************************************************************************************/
/*******************************************************************************************************/

void UnusedGpioInit(void)
{
    int8_t num=0;

    //gpio0~7、wkp123，深睡保持。其他不保持

    //1
    //num=46;
    //VmcuGpioModeSet(46, 0x00);  //debug时，pin46用作xy_printf的Tx
    //VmcuGpioWrite(46, 0);
    num=48;
    VmcuGpioModeSet(num, 0x00); //SWCLK
    VmcuGpioWrite(num, 0);
    num=49;
    VmcuGpioModeSet(num, 0x00); //SWIO
    VmcuGpioWrite(num, 0);
    num=50;
    VmcuGpioModeSet(num, 0x00); 
    VmcuGpioWrite(num, 0);
    num=51;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);

    //2
    num=43;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=42;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=35;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=31;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=27;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);

    //3
    num=4;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=3;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 1);      //TX--高关
    num=7;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=0;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=6;
    VmcuGpioModeSet(num, 0x11);  //SIM_SE        
    num=5;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);

    //4
    num=8;
    VmcuGpioModeSet(8, 0x00); 
    VmcuGpioWrite(8, 0);
    num=9;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=11;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);

    //5
    num=23;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=22;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=21;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=24;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=14;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=20;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=45;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=44;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);

    //6
    num=54;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=53;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=52;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=47;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=36;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=41;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=30;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=28;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=25;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=26;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);

    //7
    num=201;
    VmcuGpioModeSet(num, 0x11);  //key 0x11  Mode_In_DI
    //VmcuGpioWrite(num, 0);
    num=202;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=203;
    VmcuGpioModeSet(num, 0x11);  //ALARM
    //VmcuGpioWrite(num, 0);
    num=2;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=1;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);

    //8
    num=13;
    VmcuGpioModeSet(13, 0x00); 
    VmcuGpioWrite(13, 0);
    num=12;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);
    num=10;
    VmcuGpioModeSet(num, 0x00);
    VmcuGpioWrite(num, 0);

    num=7;
    VmcuGpioModeSet(7, 0x11); 
}