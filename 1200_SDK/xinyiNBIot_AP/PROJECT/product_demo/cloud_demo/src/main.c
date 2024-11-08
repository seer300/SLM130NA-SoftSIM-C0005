/**
* @file
* @brief      该DEMO为云业务交互模型。
*             各种配置可以通过云下发下来更新，并重启后仍然有效。
*
* @warning    务必设置正确的NV参数
*
* @note		  1.宏AT_UART和XY_DEBUG打开时，ap的log口与AT口一致且为lpuart，端口固定为GPIO3和GPIO4，xy_Print接口能够输出日志。
*			  2.宏AT_UART关闭，XY_DEBUG打开时，ap的log口为用户配置io口，xy_Print接口能够输出日志。
*			  3.宏AT_UART打开，XY_DEBUG关闭时，ap的AT口配置为lpuart，但是xy_Print接口失效。
*			  4.宏AT_UART和宏XY_DEBUG都关闭时,无串口初始化,xy_Print接口失效。
*/


#include "ap_watchdog.h"
#include "xy_printf.h"
#include "hal_gpio.h"
#include "xy_ftl.h"
#include "urc_process.h"
#include "cloud_process.h"
#include "data_gather.h"
#include "user_config.h"
#include "at_process.h"
#include "xy_cp.h"
#include "xy_lpm.h"
#include "xy_system.h"
#include "at_uart.h"
#include "sys_rc32k.h"


/*非AON区域的外设(CSP1,CSP2,CSP3,TIMER2,ADC,UART2,I2C1,I2C2,SPI)必须按需启停，即XXX_Init-->传输-->XXX_Deinit，不得轻易在此函数中初始化，否则功耗会抬高*/
__RAM_FUNC void User_Init_FastRecovery(void)
{
/*供用户排查快速恢复唤醒后有没有flash上运行的代码,对于低概率的事件容许运行在flash，可以通过Flash_mpu_Unlock临时放行*/
	Flash_mpu_Lock();

	/*快速恢复后看门狗失效，需用户自行重启看门狗*/
	//AP_WDT_Init(AP_WDT_WORKMODE_RESET, 30);
	//Soft_Watchdog_Init(30*60);
}

extern __RAM_FUNC void Ping_Process(void);
__RAM_FUNC int main(void)
{
    SystemInit();

    FastRecov_Init_Hook_Regist(User_Init_FastRecovery);

	/*产品用户必须设计看门狗机制*/
	//AP_WDT_Init(AP_WDT_WORKMODE_RESET, 30);
	//Soft_Watchdog_Init(30*60);

	xy_printf("cloud_demo start\n");

	while(1)
	{
		/*低概率事件容许运行flash代码*/
		Flash_mpu_Unlock();
		
        if(is_event_set(EVENT_CLOUD_SEND) || is_event_set(EVENT_CLOUD_UPDATE) || is_event_set(EVENT_BOOT_CP))
            Boot_CP(WAIT_CP_BOOT_MS);

		at_uart_recv_and_process();
		
		Ping_Process();

		Send_Data_By_Cloud();

		CP_URC_Process();

		RC32k_Cali_Process();

		Flash_mpu_Lock();

		Enter_LowPower_Mode(LPM_DSLEEP);
	}
}
