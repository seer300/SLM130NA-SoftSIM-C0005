/**
 * @brief  该工程默认为上电就加载CP核的传统数据模式，用户可以参考其他DEMO，实现OPENCPU产品相关代码
 * @warning OPENCPU开发时，注意Boot_CP调用点、低功耗相关外设的使用细节、User_Init_FastRecovery的实现、看门狗的使用、容错策略等，细节参考相关DEMO工程
 * @note  用户编译default工程时，请根据自身产品形态，设置好define.cmake中XY_SOC_VER和VER_BC95等重要宏值
 */
#include "hal_gpio.h"
#include "xy_printf.h"
#include "xy_cp.h"
#include "xy_lpm.h"
#include "at_uart.h"
#include "auto_baudrate.h"
#include "urc_process.h"
#include "xy_system.h"
#include "xy_svd.h"
#include "ap_watchdog.h"
#include "basic_config.h"
#include "sys_rc32k.h"
#include "app_init.h"
#include "mpu_protect.h"

#define WDT_TIEMOUT_SEC                (2*60)

/*非AON区域的外设(CSP1,CSP2,CSP3,TIMER2,ADC,UART2,I2C1,I2C2,SPI)必须按需启停，即XXX_Init-->传输-->XXX_Deinit，不得轻易在此函数中初始化，否则功耗会抬高*/
__RAM_FUNC void User_Init_FastRecovery(void)
{
/*供用户排查快速恢复唤醒后有没有flash上运行的代码,对于低概率的事件容许运行在flash，可以通过Flash_mpu_Unlock临时放行*/
	Flash_mpu_Lock();

	/*快速恢复后看门狗失效，需用户自行重启看门狗*/
	//AP_WDT_Init(AP_WDT_WORKMODE_RESET, 30);
	//Soft_Watchdog_Init(30*60);
}


/*UTC全局看门狗时长，用户自行设置时长。喂狗频率过快会造成功耗抬升*/
#define UTC_WATCHDOG_TIME   (3*60)


__RAM_FUNC int main(void)
{     
    SystemInit();

    FastRecov_Init_Hook_Regist(User_Init_FastRecovery);

#if MODULE_VER
	AP_WDT_Init(AP_WDT_WORKMODE_RESET, WDT_TIEMOUT_SEC);
#else

	/*OPENCPU,使用UTC_WDT全局看门狗，深睡保持。由用户在while(1)里周期性喂狗，但不能过于频繁，否则功耗会增加 */
    UTC_WDT_Init(UTC_WATCHDOG_TIME);
#endif



	/*section方式注册的用户开机初始化函数执行*/
	User_Startup_Init();

	while (1)
    {
    
#if MODULE_VER
		AP_WDT_Refresh(WDT_TIEMOUT_SEC);
         /*传统模组形态默认上电就启动CP核，用户可以自行改为按需加载CP核*/
        if(Boot_CP(WAIT_CP_BOOT_MS) == 0)
        {
            xy_assert(0);
        }
#else
		/*喂狗耗时长，频率过快会造成功耗抬高*/
		UTC_WDT_Refresh(UTC_WATCHDOG_TIME);

		/*各种事件处理*/
        if(is_event_set(EVENT_BOOT_CP))
            Boot_CP(WAIT_CP_BOOT_MS);
		
#endif

        at_uart_recv_and_process();


        CP_URC_Process();

        RC32k_Cali_Process();

        Enter_LowPower_Mode(LPM_DSLEEP);

    }
}

