/**
* @file
* @brief      本DEMO为水表运行模型。
*
* @warning    务必设置正确的NV参数
*
* @note		  1.本demo主要涉及SPI，I2C，LPUART，ADC，GPIO的使用。
*			  2.本demo配置如下：
*			  (XY_SOC_VER 2)
*			  (VER_BC95=1)
*			  (XY_DEBUG=0)
*             (XY_DUMP=1)
*			  (CP_USED_APRAM=35)
*			  (XY_AT_CTL=1)
*			  (AT_LPUART=1)
*			  (MPU_EN=0) 
*             (CONFIG_DYN_LOAD_SELECT 0)
*             且factory_nv中的_32K_CLK_MODE须为2（强制使用RC）
*			  3.本demo的测试命令为AT+DEMOCFG,具体含义如下
*               1）AT+DEMOCFG=SPI 查询SPI通讯的flash内存储的世界时间
*               2）AT+DEMOCFG=EEPROM,1 查询EEPROM内存储的世界时间
*               3）AT+DEMOCFG=EEPROM,0,x 设置x分钟写一次EEPROM
*               4）AT+DEMOCFG=SEND,0,x 设置x分钟触发一次云业务
*               5）AT+DEMOCFG=SEND,1 立即触发数据发送
*               5）AT+DEMOCFG=VERSION 获取版本信息
*			  4.本demo只有在红外打开的情况下才能进行AT命令的收发,将GPIO_PAD_NUM_7拉高1秒以上即可
*             5.本demo可选测PSM流程或者StopCP流程，在user_NB_app.h内配置 MODE_PSM 即可
*             6.本demo可选测长短周期工作模式和短周期工作模式，在user_config.h内配置 SLEEP_PERIOD_MODE 即可
*/

#include "hal_gpio.h"
#include "urc_process.h"
#include "user_config.h"
#include "at_process.h"
#include "xy_cp.h"
#include "xy_lpm.h"
#include "xy_system.h"
#include "at_uart.h"
#include "sys_rc32k.h"
#include "utc.h"
#include "xy_event.h"
#include "user_gpio.h"
#include "user_time.h"
#include "user_init.h"
#include "utc_watchdog.h"
#include "user_NB_app.h"
#include "xy_fota.h"

/*非AON区域的外设(CSP1,CSP2,CSP3,TIMER2,ADC,UART2,I2C1,I2C2,SPI)必须按需启停，即XXX_Init-->传输-->XXX_Deinit，不得轻易在此函数中初始化，否则功耗会抬高*/
__RAM_FUNC void User_Init_FastRecovery(void)
{
/*供用户排查快速恢复唤醒后有没有flash上运行的代码,对于低概率的事件容许运行在flash，可以通过Flash_mpu_Unlock临时放行*/
	Flash_mpu_Lock();
}

#if AT_LPUART
extern void reset_uart_recv_buf(void);
__RAM_FUNC void User_Lpuart_Func(void)
{
	if(!g_hw_init)//红外未打开，不允许使用上位机下位机交互
	{
		reset_uart_recv_buf();
		return;
	}
	else
	{
		at_uart_recv_and_process();
	}
}
#endif

__RAM_FUNC void XY_TIMER_Work()
{	
	if(is_event_set(EVENT_XY_TIMER))
	{
        clear_event(EVENT_XY_TIMER);
        
        User_RTC_Refresh();//用户日历系统刷新，业务推进
	}
}

__RAM_FUNC void Flash_Sleep_Work()
{
	static uint16_t flash_protect = 0;
	static uint64_t write_mark = 0;
	uint8_t wbuf1[512] = {0};
	uint32_t fota_flash_addr;
	uint32_t fota_flash_len;

	write_mark++;

	if(write_mark%240 == 0)
	{
		for(int i = 0 ; i < 512; i++)
		{
			wbuf1[i] = (uint8_t)i;
		}

		xy_OTA_flash_info(&fota_flash_addr,&fota_flash_len);

		if(xy_Flash_Write((USER_FLASH_BASE+(flash_protect*512)), wbuf1, 512) == false)
		{
			Send_AT_to_Ext("\r\nFS W FAIL\r\n");
			xy_assert(0);
		}

		if(xy_Flash_Write((fota_flash_addr+(flash_protect*512)), wbuf1, 512) == false)
		{
			Send_AT_to_Ext("\r\nFS W FAIL\r\n");
			xy_assert(0);
		}

		flash_protect++;

		if(((flash_protect+1)*512) > USER_FLASH_LEN_MAX)
		{
			flash_protect = 0;
		}

		if((uint32_t)((flash_protect+1)*512) > fota_flash_len)
		{
			flash_protect = 0;
		}

		Send_AT_to_Ext("\r\nFS W\r\n");

		write_mark = 0;
	}
}

void Fota_Reboot_Proc(void)
{
	At_status_type at_ret = XY_OK;

	at_ret = AT_Send_And_Get_Rsp("AT+CGATT=1\r\n",10, NULL, NULL);
	if(at_ret != XY_OK)
	{
		xy_assert(0);
	}
	Send_AT_to_Ext("\r\nFota Confirm\r\n");	
}

extern void User_Hook_Regist(void);
__RAM_FUNC int main(void)
{
    SystemInit();

    FastRecov_Init_Hook_Regist(User_Init_FastRecovery);

	Into_Dslp_Hook_Regist(Flash_Sleep_Work);

	User_Hook_Regist();

    //用户基础信息获取与用户的CP初始化设置
    User_Info_Init();

    //IO初始状态初始化
    User_Gpio_Init();

    //周期性事件初始化，Time日历全局初始化
	User_Config_Init();

    Send_AT_to_Ext("\r\nWMD work!\r\n");	

    //使用UTC_WDT全局看门狗，深睡保持，每秒回调内喂狗
    UTC_WDT_Init(USER_UTC_WDT_TIME);

	while(1)
	{
#if AT_LPUART
		User_Lpuart_Func();//保留芯翼AT框架，模拟客户上位机下位机通讯
#endif
		XY_TIMER_Work();//客户日历系统维护及业务推进

		Send_Data_By_Cloud_WM();
		
		CP_URC_Process();

		RC32k_Cali_Process();

		Enter_LowPower_Mode(LPM_DSLEEP);
	}
}