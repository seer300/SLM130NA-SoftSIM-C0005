#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "xy_timer.h"
#include "utc_watchdog.h"
#include "user_config.h"


#define FLASH_TEST_ADDR (USER_FLASH_BASE)
#define FLASH_TEST_LEN  (0x1000)


/*即将执行FOTA下载和FLASH保存，AP核可执行私有动作。也可以在User_SoftReset_Hook中执行私有动作*/
__RAM_FUNC void User_Fota_Proc_Hook(uint32_t state)
{	       
	/*即将执行差分包下载与FLASH保存，会长时间挂起AP核，可执行保存E2等紧急事务，并开启看门狗*/
    if(state == 1)
    {
		if (xy_Flash_Write(FLASH_TEST_ADDR, BAK_MEM_BASE, FLASH_TEST_LEN) == false)
		{
			xy_printf("flash write fail\r\n");
		}
		else
		{
			xy_printf("flash write complete\r\n");
		}
    }
	/*FOTA流程异常中止结束，用户可恢复先前暂停的事务*/
    else
    {
        xy_printf("\r\nfota done\r\n");
    }
}

/*差分包校验通过后，即将重启进入差分升级流程。AP核用户可执行私有动作*/
void User_SoftReset_Hook(Soft_Reset_Type soft_reset_type)
{
	xy_printf("\r\nReady to reset:%d\r\n", soft_reset_type);

	if(soft_reset_type == SOFT_RB_BY_FOTA)
	{
		xy_printf("\r\nFOTA reset\r\n");
	}
}



void User_Hook_Regist(void)
{
	/*即将执行FOTA下载和FLASH保存，AP核可执行私有动作。失败后需要恢复私有动作*/
	Fota_Proc_Hook_Regist(User_Fota_Proc_Hook);

	/*差分包校验通过后，即将重启进入差分升级流程。AP核用户可执行私有动作，但不能执行擦写FLASH动作*/
	Pre_Reset_Hook_Regist(User_SoftReset_Hook);
}

