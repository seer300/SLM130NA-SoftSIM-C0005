
/*****************************************************************************************************************************	
 * @brief  该工程为气表参考DEMO
 * @warning OPENCPU开发时，注意Boot_CP调用点、低功耗相关外设的使用细节、User_Init_FastRecovery的实现、看门狗的使用、容错策略等，细节参考相关DEMO工程
 * @note  用户编译gas_meter_demo工程时，请根据自身产品形态，设置好define.cmake中XY_SOC_VER和VER_BC95等重要宏值
 ****************************************************************************************************************************/

#include "sys_rc32k.h"
#include "urc_process.h"
#include "user_debug.h"
#include "user_cloud.h"
#include "user_config.h"
#include "user_eeprom.h"
#include "user_uart.h"
#include "user_gpio.h"
#include "user_watchdog.h"
#include "user_fota.h"
#include "user_key.h"
#include "user_timer.h"
#include "user_adc.h"
#include "user_lcd.h"
#include "vmcu.h"

/*仅log串口需要在此执行去初始化和初始化，其他的外设建议按需启停，即XXX_Init-->传输-->XXX_Deinit*/
__RAM_FUNC void User_Init_FastRecovery(void)
{
    // 按客户硬件设计重新映射AP Jlink引脚
    AP_Jlink_Reset(GPIO_PAD_NUM_10, GPIO_PAD_NUM_9);
    CP_Jlink_Reset(GPIO_PAD_NUM_30, GPIO_PAD_NUM_31);

    // 初始化printf
    jk_printf_uart_Init();

    // 设置4s看门狗
    UserWatchdogInit(WDT_TIMEOUT);
}

extern uint32_t xy_timer_overflow_test(uint32_t overflowsec);
extern void rtc_init_test(uint32_t newyear,uint32_t countdown);

__RAM_FUNC int main(void)
{
    SystemInit();

    Into_Dslp_Hook_Regist(User_Flash_Sleep_Work);

    FastRecov_Init_Hook_Regist(User_Init_FastRecovery);

    User_Hook_Regist();

    //utc看门狗去初始化
    UTC_WDT_Deinit();

    // 未使用GPIO初始化
    UnusedGpioInit();

    // 初始化printf
    jk_printf_uart_Init();

    // 按客户硬件设计重新映射AP Jlink引脚
    AP_Jlink_Reset(GPIO_PAD_NUM_10,GPIO_PAD_NUM_9);

    // lcd初始化
    VmcuLcdInit();

    // 初始化按键检测
    UserKeyInit();

    //lptimer设置1s定时器，周期唤醒
    UserLptimerInit();

    // 产线 测试用LPUart初始化
    UserFactUartInit();

    xy_timer_overflow_test(XY_TIMER_OFSEC);
    // rtc_init_test(RTC_NEWYEAR,RTC_COUNTDOWN);

    // 设置4s看门狗
    UserWatchdogInit(WDT_TIMEOUT);

    jk_printf("power on:%d,%d\r\n", Get_Boot_Reason(), Get_Boot_Sub_Reason());

    while(1)
    {
        // 云通信
        DO_Send_Data_By_JK();

        // 处理URC上报
        CP_URC_Process();

        // 周期性进行ADC检测
        UserAdcFunc();

        // 处理按键操作
        UserKeyFunc();

        // lcd显示处理
        UserLcdFunc();

        // eeprom读写测试
        UserEepromTest();

#if (BAN_WRITE_FLASH==0)
        // flash读写测试
        UserFlashFunc();
#endif

        // 处理产线串口和红外串口收发数据
        UserUartFunc();

        // 开关阀操作
        UserValveFunc();

        // RC32K校准
        RC32k_Cali_Process();

        // CSP2工作期间打开深睡锁不进深睡，开阀过程中打开深睡锁不进深睡
        VmcuSleep(1);
    }
}
