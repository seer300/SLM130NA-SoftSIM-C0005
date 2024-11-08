/**
* @file
* @brief      该DEMO模拟门磁类产品，产品模型为开关门时通过WAKEUP-PIN触发唤醒及中断，然后通过读取GPIO电平识别开关门状态，并立即上报；对于开门状态，会开启周期定时器循环上报云平台，直至再次关门后停止周期上报。\n
*             若无线通信失败，需要缓存开关门相关信息数据及时间戳，待下次通信恢复后一并上报。             各种配置可以通过云下发下来更新，并重启后仍然有效。
*
* @warning    务必设置正确的NV参数
*
* @note		  1.宏AT_UART和XY_DEBUG打开时，ap的log口与AT口一致且为lpuart，端口固定为GPIO3和GPIO4，xy_Print接口能够输出日志。
*			  2.宏AT_UART关闭，XY_DEBUG打开时，ap的log口为用户配置io口，xy_Print接口能够输出日志。
*			  3.宏AT_UART打开，XY_DEBUG关闭时，ap的AT口配置为lpuart，但是xy_Print接口失效。
*			  4.宏AT_UART和宏XY_DEBUG都关闭时,无串口初始化,xy_Print接口失效。
*/
#include "xy_system.h"
#include "ap_watchdog.h"
#include "xy_printf.h"
#include "hal_gpio.h"
#include "xy_ftl.h"
#include "urc_process.h"
#include "user_config.h"
#include "cloud_process.h"
#include "data_gather.h"
#include "xy_cp.h"
#include "xy_lpm.h"
#include "at_uart.h"
#include "sys_rc32k.h"

__RAM_FUNC void Save_Data(void)
{
	//判断bkmem中数据存储是否到达阈值，若达到则回写到flash中
	if(is_event_set(EVENT_SAVE_DATA))
	{
		xy_printf("save retmem data into flash\n");
		//将bkmem中的数据保存到flash中，以防bkmem中的数据溢出
		Save_Bkmem2Flash();

		clear_event(EVENT_SAVE_DATA);
	}
}

/*采集类事件一般为秒级高频率事件，需要放在RAM上执行，否则运行时间过长影响功耗*/
__RAM_FUNC void Gather_Data()
{
	if(is_event_set(EVENT_GATHER))
	{
		void *gather_data;
		uint32_t gather_len;

		clear_event(EVENT_GATHER);

		/*gather_data由用户静态申请内存，调用者不负责释放内存空间*/
		Get_Gathered_Data(&gather_data,&gather_len);

		Save_Data_2_BakMem(gather_data,gather_len);

#if FLASH_OPTEST
		if(g_Control_Context->flash_saved_size > 1000  /*|| g_Control_Context->bkmem_saved_size > 0*/)
		{
			/* 采集完数据后，开始执行上传操作 */
			set_event(EVENT_CLOUD_SEND);
			Send_AT_to_Ext("\r\nsend data\r\n");
		}
#else 
		if(g_Control_Context->flash_saved_size > 0  || g_Control_Context->bkmem_saved_size > 0)
		{
			/* 采集完数据后，开始执行上传操作 */
			set_event(EVENT_CLOUD_SEND);
		}
#endif 
	}
}
#if PERIOD_TEST
extern void Period_Test_Timeout(void);
__RAM_FUNC void Period_Test()
{
	if(is_event_set(EVENT_PERIOD_TEST))
	{
		clear_event(EVENT_PERIOD_TEST);
		static int st_count = 0;
		st_count++;
		if(st_count%2)
		{
			Timer_AddEvent(TIME_PERIOD_TEST, USER_PERIOD_TEST_LONG, Period_Test_Timeout, 0);
		}
		else
		{
			Timer_AddEvent(TIME_PERIOD_TEST, USER_PERIOD_TEST_SHORT, Period_Test_Timeout, 0);
		}
	}
}
#endif


/*非AON区域的外设(CSP1,CSP2,CSP3,TIMER2,ADC,UART2,I2C1,I2C2,SPI)必须按需启停，即XXX_Init-->传输-->XXX_Deinit，不得轻易在此函数中初始化，否则功耗会抬高*/
__RAM_FUNC void User_Init_FastRecovery(void)
{
/*供用户排查快速恢复唤醒后有没有flash上运行的代码,对于低概率的事件容许运行在flash，可以通过Flash_mpu_Unlock临时放行*/
	Flash_mpu_Lock();

	/*快速恢复后看门狗失效，需用户自行重启看门狗*/
	//AP_WDT_Init(AP_WDT_WORKMODE_RESET, 30);
	//Soft_Watchdog_Init(30*60);

    User_Peripherals_Init();
}

__RAM_FUNC int main(void)
{
    SystemInit();

    FastRecov_Init_Hook_Regist(User_Init_FastRecovery);

	/* 软看门狗，通常用于远程通信相关业务异常的容错。芯片的监控必须依靠AP_WDT_Init */
	Soft_Watchdog_Init(30*60);

	User_Peripherals_Init();

	User_Config_Init();

#if FLASH_OPTEST
    Lptimer_PeriodicTiming_Init(LPTIMER_TIMING_PERIOD);
#endif
	Send_AT_to_Ext("menci_demo start\r\n");

	while(1)
	{
		if(is_event_set(EVENT_CLOUD_SEND) || is_event_set(EVENT_CLOUD_UPDATE) || is_event_set(EVENT_BOOT_CP))
			Boot_CP(WAIT_CP_BOOT_MS);
	
		at_uart_recv_and_process();

		Gather_Data();

		Save_Data();

#if PERIOD_TEST
		Period_Test();
#endif	

		Send_Data_By_Cloud();

		CP_URC_Process();

		RC32k_Cali_Process();
		
	    Enter_LowPower_Mode(LPM_DSLEEP);
	}
}


