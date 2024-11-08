/*****************************************************************************************************************************	 
 * user_fota.c
 ****************************************************************************************************************************/

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "xy_timer.h"
#include "utc_watchdog.h"
#include "user_config.h"
#include "xy_event.h"
#include "xy_cp.h"
#include "xy_system.h"
#include "vmcu.h"
#include "err_process.h"
#include "xy_fota.h"
#include "user_debug.h"
#include "xy_flash.h"

/*即将执行FOTA下载和FLASH保存，AP核可执行私有动作。也可以在User_SoftReset_Hook中执行私有动作*/
__RAM_FUNC void User_Fota_Proc_Hook(uint32_t state)
{
	/*即将执行差分包下载与FLASH保存，会长时间挂起AP核，可执行保存E2等紧急事务，并开启看门狗*/
	if (state == 1)
	{
		UTC_WDT_Init(30 * 60); // 初始化看门狗兜底fota
		clear_all_event();
		Clear_Info_of_CP();

		//测试代码，fota初始化阶段AP核写flash
		uint8_t flash_write_data[16] = {0};
		snprintf((char*)flash_write_data, 16, "fota write test");
		xy_assert(xy_Flash_Write(USER_FLASH_BASE, flash_write_data, 16));
		jk_printf("\r\nready to fota\r\n");
	}
	/*FOTA流程异常中止结束，用户可恢复先前暂停的事务*/
	else
	{
		uint8_t flash_read_data[16] = {0};
		xy_assert(xy_Flash_Read(USER_FLASH_BASE, flash_read_data, 16));
		jk_printf("\r\nfota done,read user data from flash:%s\r\n", flash_read_data);
	}
}

/*差分包校验通过后，即将重启进入差分升级流程。AP核用户可执行私有动作*/
void User_SoftReset_Hook(Soft_Reset_Type soft_reset_type)
{
	jk_printf("\r\nReady to reset:%d\r\n", soft_reset_type);

	if(soft_reset_type == SOFT_RB_BY_FOTA)
	{
		uint8_t flash_read_data[16] = {0};
		xy_assert(xy_Flash_Read(USER_FLASH_BASE, flash_read_data, 16));
		jk_printf("\r\nFOTA reset,read user data from flash:%s\r\n",flash_read_data);
	}
}


void User_Hook_Regist(void)
{
	/*即将执行FOTA下载和FLASH保存，AP核可执行私有动作。失败后需要恢复私有动作*/
	Fota_Proc_Hook_Regist(User_Fota_Proc_Hook);

	/*差分包校验通过后，即将重启进入差分升级流程。AP核用户可执行私有动作，但不能执行擦写FLASH动作*/
	Pre_Reset_Hook_Regist(User_SoftReset_Hook);
}

__RAM_FUNC void User_Flash_Sleep_Work(void)
{
	static uint16_t flash_protect = 0;
	static uint16_t write_mark = 0;
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
			jk_printf("\r\nFS W FAIL\r\n");
			xy_assert(0);
		}

		if(xy_Flash_Write((fota_flash_addr+(flash_protect*512)), wbuf1, 512) == false)
		{
			jk_printf("\r\nFS W FAIL\r\n");
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

		jk_printf("\r\nFS W\r\n");

        write_mark = 0;
	}
}