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
#include "xy_svd.h"
#include "dyn_load.h"
#include "xy_fota.h"
#include "xy_utils.h"
#include "driver_utils.h"
#include "cm3_systick.h"


/**
 * @brief FLASH测试AT命令，用于FLASH反复擦写性能测试,已打印方式输出擦写大小及对应的耗时。
 * @note AT+FLASHTEST =<擦写总次数>,<擦写大小>,<数据>
 * <擦写Flash大小>:数据类型为整型,擦写Flash大小,单位是4K,例如，若值为2,则代表擦写大小为8K,值范围为1~16
 * <数据>:数据类型为整型,测试时写入FLASH的数据,例如值为9,若此时FLASH擦写大小为4K,则会向4K大小的FLASH中写入9.
 */
__RAM_FUNC int at_FLASHTEST_req(char *param,char **rsp_cmd)
{
	UNUSED_ARG(rsp_cmd);

	if(g_cmd_type == AT_CMD_REQ)
	{
		int32_t test_total_count = 0;
		int32_t test_size = 0;
		int32_t test_write_data;
		uint8_t debug_info[50];
    	uint32_t recordCurTestCount = 1;
		uint8_t *data = NULL;
		uint8_t * check_data = NULL;//用于校验写入FLash值是否符合预期
		bool bootcp_flag = false;

		if (at_parse_param("%d,%d,%d", param, &test_total_count, &test_size, &test_write_data) != XY_OK)
		{
			return XY_ERR_PARAM_INVALID;
		}

		if(test_total_count <= 0 || test_size < 0 || test_size >16)
		{
			return XY_ERR_PARAM_INVALID;
		}
		
		while(test_total_count > 0)
		{
			if(CP_Is_Alive())
			{
				bootcp_flag = true;
				Stop_CP(0);//flash测试时，需使用AP单核
			}

			uint32_t startTime = Convert_Tick_to_Ms(Get_Tick());
			xy_Flash_Erase(USER_FLASH_BASE, test_size*0x1000);
			uint32_t endTime = Convert_Tick_to_Ms(Get_Tick());
			snprintf((char *)debug_info, 50, "\r\ntestNum %lu:erase %ldK,%lu ms\r\n",recordCurTestCount, test_size*4,(endTime - startTime));
			Send_AT_to_Ext((char *)debug_info);


			if(data == NULL)//数据以4K为单位写入
			{
				data = xy_malloc(0x1000);
				memset(data, test_write_data, 0x1000);
			}
			int32_t i = 0;
			startTime = Convert_Tick_to_Ms(Get_Tick());
			for(int32_t count = test_size; count > 0; count--)
			{
				//以4K为单位写入
				xy_Flash_Write(USER_FLASH_BASE + i*0x1000, data, 0x1000);
				i++;
			}
			endTime = Convert_Tick_to_Ms(Get_Tick());
			snprintf((char *)debug_info, 50, "testNum %lu:write %ldk,%lu ms\r\n", recordCurTestCount, test_size*4,(endTime - startTime));
			Send_AT_to_Ext((char *)debug_info);


			/*校验写入数据的正确性，并计算xy_flash_read的耗时*/
			if(check_data == NULL)
			{
				check_data = xy_malloc(0x1000);
			}
			memset(check_data, 0x00, 0x1000);
			startTime = Convert_Tick_to_Ms(Get_Tick());
			for(i = 0; i < test_size; i++)
			{
				xy_Flash_Read(USER_FLASH_BASE + i*0x1000, check_data, 0x1000);
				if(memcmp(data,check_data,0x1000)!=0)
				{
					xy_assert(0);//写入FLASH的值与读出的值不一致
				}
			}
			endTime = Convert_Tick_to_Ms(Get_Tick());
			snprintf((char *)debug_info, 50, "testNum %lu:read %ldk,%lu ms\r\n", recordCurTestCount, test_size*4,(endTime - startTime));
			Send_AT_to_Ext((char *)debug_info);

			

			startTime = Convert_Tick_to_Ms(Get_Tick());
			xy_Flash_Erase(USER_FLASH_BASE, test_size*0x1000);
			endTime = Convert_Tick_to_Ms(Get_Tick());
			snprintf((char *)debug_info, 50, "testNum %lu:erase %ldK,%lu ms\r\n",recordCurTestCount, test_size*4,(endTime - startTime));
			Send_AT_to_Ext((char *)debug_info);


			startTime = Convert_Tick_to_Ms(Get_Tick());
			for(int32_t count = test_size, i=0; count > 0; count--)
			{
				//以4K为单位写入
				xy_Flash_Write_No_Erase(USER_FLASH_BASE + i*0x1000, data, 0x1000);
				i++;
			}
			endTime = Convert_Tick_to_Ms(Get_Tick());
			snprintf((char *)debug_info, 50, "testNum %lu:writeNoErase %ldK,%lu ms\r\n",recordCurTestCount, test_size*4,(endTime - startTime));
			Send_AT_to_Ext((char *)debug_info);


			/*数据校验，读出FLASH与期望写入FLASH的值比较，是否一致*/
			memset(check_data, 0x00, 0x1000);
			for(i = 0; i < test_size; i++)
			{
				xy_Flash_Read(USER_FLASH_BASE + i*0x1000, check_data, 0x1000);
				if(memcmp(data,check_data,0x1000)!=0)
				{
					xy_assert(0);//写入FLASH的值与读出的值不一致
				}
			}

			
			test_total_count--;
			recordCurTestCount ++;

			if(test_total_count == 0)
			{
				snprintf((char *)debug_info, 50, "\r\nflash test end\r\n");
				Send_AT_to_Ext((char *)debug_info);

				if(data != NULL)
				{
					xy_free(data);
				}

				if(check_data != NULL)
				{
					xy_free(check_data);
				}
				
				if(!CP_Is_Alive() && bootcp_flag == true)//若FLASH测试前启动了CP,则恢复至起CP的状态
				{
					Boot_CP(WAIT_CP_BOOT_MS);
				}
			}

			AP_WDT_Refresh(120);
		}
	}
	else
	{
		return XY_ERR_PARAM_INVALID;
	}

	return XY_OK;
}



#define APTEST_FLASH_BASE (HWREG(BAK_MEM_FOTA_FLASH_BASE))
#define APTEST_FLASH_SIZE 0x20000 // 128K

uint32_t AP_Freq_test(uint32_t val1)
{
	char rsp[50] = {0};

	/*AT+APTEST=FREQ,0 AP主频切为PLL，以供AP外设测试*/
	if(val1 == 0)
	{
		if(PRCM_SysclkSrcGet() == SYSCLK_SRC_PLL)
		{
			return XY_OK;
		}

		PRCM_ForceCPOff_Disable();
        delay_func_us(100);
		Ap_HclkDivSet(10);
		SIDO_NORMAL_INIT();
		START_PLL();
        Sys_Clk_Src_Select(SYSCLK_SRC_PLL);
	}
	/*AT+APTEST=FREQ,1 AP主频切回HRC，以供AP外设测试*/
	else if(val1 == 1)
	{
		if(PRCM_SysclkSrcGet() == SYSCLK_SRC_HRC)
		{
			return XY_OK;
		}

		HRC_Clk_Src_Select(4);
		Sys_Clk_Src_Select(SYSCLK_SRC_HRC);
		Xtal_Pll_Ctl(SYSCLK_SRC_HRC,LSIO_CLK_SRC);
		PRCM_SlowfreqDivDisable();
	}
	/*AT+APTEST=FREQ,2 查询AP主频值*/
	else if(val1 == 2)
	{
		sprintf(rsp,"\r\nFreq:%ld\r\n",GetAPClockFreq());
		Send_AT_to_Ext(rsp);
	}
	else
	{
		return XY_ERR_PARAM_INVALID;
	}

	return XY_OK;
}
int system_test(uint32_t val1)
{
	char rsp[50] = {0};

	/*AT+APTEST=SYSTEM,0  构造AP核assert*/
	if(val1 == 0)
	{
		xy_assert(0);
	}
	/*AT+APTEST=SYSTEM,1  执行软复位*/
	else if(val1 == 1)
	{
		xy_Soft_Reset(SOFT_RB_BY_AP_USER);
	}
	/*AT+APTEST=SYSTEM,2  执行SOC复位*/
	else if(val1 == 2)
	{
		xy_Soc_Reset();
	}
	/*AT+APTEST=SYSTEM,3  查询rand随机值*/
	else if(val1 == 3)
	{
		srand(xy_seed());
		sprintf(rsp, "\r\nxy_seed:%ld;rand:%d\r\n",xy_seed(),rand());
		Send_AT_to_Ext(rsp);
	}
	/*AT+APTEST=SYSTEM,4  获取系统上电原因*/
	else if(val1 == 4)
	{
		sprintf(rsp, "\r\n+BOOT:%d,SUBBOOT:%ld\r\n", Get_Boot_Reason(), Get_Boot_Sub_Reason());
		Send_AT_to_Ext(rsp);
	}
	else
	{
		return XY_ERR_PARAM_INVALID;
	}
	return XY_OK;
}

int flash_test(uint32_t val1,uint32_t val2,uint32_t val3,uint32_t val4)
{
	/*AT+APTEST=FLASH,0  通过该命令构建150K的后台写flash事件，供其他人进行flash干扰测试*/
	if(val1 == 0)
	{	
		char *at_cmd = xy_malloc(50);

		sprintf(at_cmd, "AT+TEST=FLASH,%ld,%d\r\n", APTEST_FLASH_BASE, APTEST_FLASH_SIZE);

		if(AT_Send_And_Get_Rsp(at_cmd, 30, "\r\nOK\r\n",NULL))
		{
			Send_AT_to_Ext("\r\nflash fail!\r\n");
		}
		xy_free(at_cmd);
	}
	/*AT+APTEST=FLASH,1,<flash offset>,<data offset>,<length>  AP核写flash检查,其中offset取值0-4095；length取值小于1024*8即可*/
	else if(val1 == 1)
	{	
		void *data = xy_malloc(val4);
		
		xy_Flash_Write(USER_FLASH_BASE+val2,(void*)(BAK_MEM_BASE+val3),val4);
		xy_Flash_Read(USER_FLASH_BASE+val2,data,val4);

		if(!memcmp((void*)(BAK_MEM_BASE+val3),data,val4))
		{
			Send_AT_to_Ext("\r\nAP flash write succ!\r\n");
		}
		else
		{
			Send_AT_to_Ext("\r\nAP flash write fail!\r\n");
		}
		xy_free(data);
	}
	/*AT+APTEST=FLASH,2,<flash addr>,<size>  擦除flash操作*/
	else if(val1 == 2)
	{	
		xy_Flash_Erase(val2,val3);
	}
	else
	{
		return XY_ERR_PARAM_INVALID;
	}
	return XY_OK;
}

/*AT+NPING=<host>,<data_len>,<ping_num>,<time_out>,<interval_time>[,<rai>]*/
int ping_test(uint32_t val1)
{
	Boot_CP(WAIT_CP_BOOT_MS);

	if(xy_wait_tcpip_ok(60))
	{
		Send_AT_to_Ext("\r\nPDP fail!\r\n");
	}

	/*AT+APTEST=PING,0  阻塞式等待PING结果, 先报OK等一段时间再报URC不能使用同步接口，此处为错误示例*/
	if(val1 == 0)
	{
		if(AT_Send_And_Get_Rsp("AT+NPING=221.229.214.202,8,2,20,2\r\n", 30, "\r\n+NPING:",NULL))
		{
			Send_AT_to_Ext("\r\nNPING FAIL!\r\n");
		}
		else
		{
			Send_AT_to_Ext("\r\nNPING success,do none!\r\n");
		}
	}
	/*AT+APTEST=PING,1  构建超长时间的循环PING动作，非阻塞，用户收到OK后可以再触发其他AT命令，期间CP核在一直执行PING包*/
	else if(val1 == 1)
	{
		if(Send_AT_Req("AT+NPING=221.229.214.202,88,720,15,5\r\n",0))
		{
			Send_AT_to_Ext("\r\nNPING FAIL!\r\n");
		}
		else
		{
			Send_AT_to_Ext("\r\nNPING will work very long!\r\n");
		}
	}
	else
	{
		return XY_ERR_PARAM_INVALID;
	}
	return XY_OK;
}

__RAM_FUNC void AP_WDT_Int_Calback()
{
	xy_printf("AP WDT interrupt\r\n");
}

__RAM_FUNC int watchdog_test(uint32_t val1,uint32_t val2,uint32_t val3)
{
	/*AT+APTEST=WDT,0,<sec>  更改AP核看门狗的刷新时长，并进行一次刷新*/
	if(val1 == 0)
	{
		AP_WDT_Refresh(val2);

		Send_AT_to_Ext("AP WDT refresh\r\n");
	}

	else if(val1 == 1)
	{
		/*AT+APTEST=WDT,1,0,1  1秒后AP核看门狗将触发复位*/
		if(val2 == 0)	
		{
			AP_WDT_Init(AP_WDT_WORKMODE_RESET, val3);
		}
		/*AT+APTEST=WDT,1,1,1  1秒后AP核看门狗将触发看门狗中断*/
		else if(val2 == 1)
		{
			AP_WDT_Int_Reg(AP_WDT_Int_Calback);
			AP_WDT_Init(AP_WDT_WORKMODE_INT, val3);
		}
		else
		{
			return XY_ERR_PARAM_INVALID;
		}
		uint8_t apwdt = HWREGB(0xA0008000);  

		xy_printf("AP WDT EN:%d\n", (apwdt&0x20));//apwdten   bit5

		xy_printf("AP WDT RST EN:%d\n", (apwdt&0x1));//apwdtrsten   bit0

		xy_printf("AP WDT INT EN:%d\n", (apwdt&0x4));//apwdtinten   bit2

		Send_AT_to_Ext("\r\nAP WDT SET!\r\n");

		while(1);
	}
	else
	{
		return XY_ERR_PARAM_INVALID;
	}
	return XY_OK;
}


extern uint32_t CP_TEST(uint32_t val1,uint32_t val2,uint32_t val3,uint32_t val4);
extern int at_timer_test(uint32_t val1,uint32_t val2,uint32_t val3,uint32_t val4,uint32_t val5,uint32_t val6);
extern int gpio_test(uint32_t val1,uint32_t val2,uint32_t val3,uint32_t val4,uint32_t val5,uint32_t val6);
extern uint32_t at_adc_test(uint32_t val1,uint32_t val2,uint32_t val3,uint32_t val4,uint32_t val5,uint32_t val6);

/***若选用OPENCPU工程，需防止在进行CP测试过程中，DEMO自动起来运行起CP产生干扰。门磁设置AT+DEMOCFG=0XCDFE6000；追踪类设置AT+DEMOCFG=2H,OFF,OFF 
****可达到很长时间才会起CP上报云数据。另外，还需要考虑快速恢复的差异！***********************************************************************/
int at_APTEST_req(char *at_buf, char **prsp_cmd)
{
	(void) prsp_cmd;
	if (g_cmd_type == AT_CMD_REQ)
	{
		char cmd[10] = {0};
		char rsp[50] = {0};
		uint32_t  val1 = 0;
		uint32_t  val2 = 0;
		uint32_t  val3 = 0;
		uint32_t  val4 = 0;
		uint32_t  val5 = 0;
		uint32_t  val6 = 0;

		at_parse_param("%s,%d,%d,%d,%d,%d,%d",at_buf,cmd,&val1,&val2,&val3,&val4,&val5,&val6);

		/*AT+APTEST=CP,<val>  CP核的操控测试*/
		if (!strcmp(cmd, "CP"))
		{
			return CP_TEST(val1,val2,val3,val4);
		}
		/*AT+APTEST=UP  查看系统上电的一些debug信息*/
		else if(!strcmp(cmd, "UP"))
		{
			void print_up_dbg_info();
			print_up_dbg_info();
		}
		/*AT+APTEST=MPU  内存保护相关测试*/
		else if(!strcmp(cmd, "MPU"))
		{
			extern uint32_t mpu_test(uint32_t val1,uint32_t val2,uint32_t val3,uint32_t val4);
			return mpu_test(val1,val2,val3,val4);
		}
		/*AP主频测试命令*/
		else if (!strcmp(cmd, "FREQ"))
		{
			return AP_Freq_test(val1);
		}
		/*AT+APTEST=32K  查询当前32K模式、32K时钟源*/
		else if (!strcmp(cmd, "32K"))
		{
			int8_t mode = READ_FAC_NV(uint8_t, _32K_CLK_MODE);
			sprintf(rsp, "\r\n32KCLKMODE:%d,32KCLKSRC:%d\r\n", mode, HWREGB(BAK_MEM_32K_CLK_SRC));
			Send_AT_to_Ext(rsp);
		}
		/*AT+APTEST=APLOCK,<val>  AP锁操作*/
		else if (!strcmp(cmd, "APLOCK"))
		{
			/*AT+APTEST=APLOCK,0,<type>  释放AP核锁，type值为1-4为深睡锁，值26和27用于STANDBY锁*/
			if(val1 == 0)
			{
				LPM_UNLOCK(val2);
				Send_AT_to_Ext("\r\nLPM_UNLOCK\r\n");
			}
			/*AT+APTEST=APLOCK,1,<type>  申请AP核锁，type值为1-4为深睡锁，值26和27用于STANDBY锁*/
			else if(val1 == 1)
			{
				LPM_LOCK(val2);
				Send_AT_to_Ext("\r\nLPM_LOCK\r\n");
			}
			/*AT+APTEST=APLOCK,2,<type>  查询AP核锁是否持有，type值为1-4为深睡锁，值26和27用于STANDBY锁*/
			else if(val1 == 2)
			{
				sprintf(rsp, "\r\nLPM_LOCK_EXIST:%d\r\n", LPM_LOCK_EXIST(val2));
				Send_AT_to_Ext(rsp);
			}
			else
			{
				return XY_ERR_PARAM_INVALID;
			}
		}
		
		/*AT+APTEST=SYSTEM,<val>*/
		else if (!strcmp(cmd, "SYSTEM"))
		{
			return system_test(val1);
		}
		/*AT+APTEST=FLASH,*/
		else if (!strcmp(cmd, "FLASH"))
		{
			return flash_test(val1,val2,val3,val4);
		}
		/*xy_timer测试*/
		else if (!strcmp(cmd, "TIMER"))
		{			
			return at_timer_test(val1,val2,val3,val4,val5,val6);			
		}
		/*niurx*/
		else if (!strcmp(cmd, "ADC"))
		{
			return at_adc_test(val1,val2,val3,val4,val5,val6);
		}
		/*wuqingfei,AT+APTEST=SVD,1,<val_set>,<second period>.例如AT+APTEST=SVD,30,5表示5秒一次检查电压是否低于3伏*/
		else if (!strcmp(cmd, "SVD"))
		{
			if(val1 == 1)
				xy_SVD_Init(val2,val3);
			else
				xy_SVD_DeInit();
		}
		/*AT+APTEST=CLOUD,val*/
		else if (!strcmp(cmd, "CLOUD"))
		{
			/*设置产生At_status_type错误的随机阈值，数值越小，产生错误概率越高，0表示无效。建议设置为20左右，需要确保夜里待机时每种错误触发5次以上*/
			/*其中，错误 XY_ERR_WAIT_RSP_TIMEOUT 在CP核通过BAK_MEM_OPEN_TEST随机值产生，与AP核策略同理*/
			if(val1 == 0)
			{
				HWREGB(BAK_MEM_OPEN_TEST) = (uint8_t)val2;
				sprintf(rsp, "\r\nBAK_MEM_OPEN_TEST:%d\r\n",HWREGB(BAK_MEM_OPEN_TEST));
				Send_AT_to_Ext(rsp);
			}
			else
			{
				return XY_ERR_PARAM_INVALID;
			}
		}
		/*AT+APTEST=PING*/
		else if (!strcmp(cmd, "PING"))
		{
			return ping_test(val1);
		}
		/*AT+APTEST=AES*/
		else if (!strcmp(cmd, "AES"))
		{
			extern void AES_ECB_Case1_128(void);
			extern void AES_CBC_Case1_128(void);			
			for(int i = val1; i > 0; i--)
			{			
				AES_ECB_Case1_128();
				AES_CBC_Case1_128();
				HAL_Delay(val2);
			}
		}
		/*AT+APTEST=GPIO*/
		else if (!strcmp(cmd, "GPIO"))
		{
			return gpio_test(val1,val2,val3,val4,val5,val6);
		}

#if XY_DUMP == 0
		/*AT+APTEST=WDT,<val>*/
		else if (!strcmp(cmd, "WDT"))
		{
			return watchdog_test(val1,val2,val3);
		}
#endif
		/*AT+APTEST=UTCWDT,<val>*/
		else if (!strcmp(cmd, "UTCWDT"))
		{
			/*AT+APTEST=UTCWDT,0  去活UTC看门狗*/
			if(val1 == 0)
			{
				UTC_WDT_Deinit();

				Send_AT_to_Ext("UTC_WDT deinit\r\n");
			}
			/*AT+APTEST=UTCWDT,1,<time sec>  使能UTC看门狗，时长为sec*/
			else if(val1 == 1)
			{
				UTC_WDT_Init(val2);
				Send_AT_to_Ext("\r\nUTC_WDT SET!\r\n");		
			}
			/*AT+APTEST=UTCWDT,2,<time sec>  喂狗，时长为sec*/
			else if(val1 == 2)
			{
				UTC_WDT_Refresh(val2);
				Send_AT_to_Ext("\r\nUTC_WDT SET!\r\n");		
			}
		}
		/*AT+APTEST=SYSTICK,<val>*/
		else if (!strcmp(cmd, "SYSTICK"))
		{
			/*AT+APTEST=SYSTICK,0  关掉CM3内核的systick*/
			if(val1 == 0)
			{
				SysTick_Deinit();
				Send_AT_to_Ext("SysTick deinit\r\n");
			}
			/*AT+APTEST=SYSTICK,1  初始化CM3内核的SYSTICK定时器*/
			else if(val1 == 1)
			{
				SysTick_Init(0);
				Send_AT_to_Ext("\r\nSysTick SET!\r\n");
			}
		}
		else
		{
			return XY_ERR_PARAM_INVALID;
		}
		
    	return XY_OK;
	}

	else
	{
		return XY_ERR_PARAM_INVALID;
	}
}


