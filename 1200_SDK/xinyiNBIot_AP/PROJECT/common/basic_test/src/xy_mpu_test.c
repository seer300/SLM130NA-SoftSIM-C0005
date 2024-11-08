#include <stdint.h>
#include <string.h>
#include "xy_system.h"
#include "utc.h"
#include "xy_flash.h"
#include "xy_printf.h"
#include "at_uart.h"
#include "xy_timer.h"
#include "utc.h"
#include "xy_cp.h"
#include "at_cmd_regist.h"
#include "at_process.h"
#include "at_CP_api.h"
#include "ap_watchdog.h"
#include "hal_aes.h"
#include "at_cmd_regist.h"
#include "hal_gpio.h"
#include "xy_event.h"
#include "sys_proc.h"
#include "mpu_protect.h"

uint32_t assert_flash = 0;
uint32_t assert_val = 0;

/*AT+APTEST=MPU,<val>  主要排查Mpu_Protect_Init接口是否正常运行*/
uint32_t mpu_test(uint32_t val1,uint32_t val2,uint32_t val3,uint32_t val4)
{
	//char rsp[50] = {0};
	UNUSED_ARG(val2);
	UNUSED_ARG(val3);
	UNUSED_ARG(val4);

	/*AT+APTEST=MPU,0  禁止is_allowed_opt_addr接口外的flash写操作，如IMEI*/
	if(val1 == 0)
	{
        xy_Flash_Erase(NV_FLASH_RF_BASE,10);
	}
	/*AT+APTEST=MPU,1  禁止对flash地址直接操作*/
	else if(val1 == 1)
	{
		*((int *)NV_FLASH_RF_BASE) = 1;
	}
	/*AT+APTEST=MPU,2  AP核位于RAM区域的代码段不可写*/
	else if(val1 == 2)
	{
		void *temp = (void *)Mpu_Protect_Init;
		*(int *)temp = 8;
	}
	
	/*AT+APTEST=MPU,3  禁止ap核写CP核的cpram以及sharemem*/
	else if(val1 == 3)
	{
		int *temp = (int *)0X60000000;
		*temp = 2;
	}

	/*AT+APTEST=MPU,4  对一级boot的代码保护，禁止写。即零地址访问受限*/
	else if(val1 == 4)
	{
		int *temp = NULL;
		*temp = 2;
	}
	/*AT+APTEST=MPU,5  设置AP核RAM的数据段(data/bss)不可执行*/
	else if(val1 == 5)
	{
		((void(*)(void))((uint8_t*)0x0101f001))();
	}
	/*AT+APTEST=MPU,6,<flash addr>,<default byte val>  FLASH某地址单字节值不符合预期直接断言*/
	else if(val1 == 6)
	{
		assert_flash = val2;
		assert_val = val3;
	}
	else
	{
		return XY_ERR_PARAM_INVALID;
	}

	return XY_OK;
}

