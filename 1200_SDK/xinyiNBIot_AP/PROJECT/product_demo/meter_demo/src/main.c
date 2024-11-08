#include "xy_system.h"
#include "xy_timer.h"
#include "ap_watchdog.h"
#include "hal_adc.h"
#include "xy_printf.h"
#include "basic_config.h"
#include "at_uart.h"
#include "user_config.h"
#include "xy_event.h"
#include "at_cmd_regist.h"
#include "urc_process.h"
#include "sys_rc32k.h"

/**
 * @brief 非AON区外设由于深睡后无法保持配置，因此如果唤醒后要使用，必须重新初始化，即：初始化-->使用-->去初始化。
 * 该函数每次深睡唤醒都会调用，因此不得轻易在此函数中初始化，否则功耗会抬高
 * 非AON区外设：CSP1,CSP2,CSP3,TIMER2,ADC,UART2,I2C1,I2C2,SPI
 */
__RAM_FUNC void User_Init_FastRecovery(void)
{
	Flash_mpu_Lock();

	/*快速恢复后看门狗失效，需用户自行重启看门狗*/
	//AP_WDT_Init(AP_WDT_WORKMODE_RESET, 30);
	//Soft_Watchdog_Init(30*60);
}

//=====================================================================================
//===================================METER_TEST========================================
//=====================================================================================
/*控制main主循环中测试的功能点，bit0:boot_CP; bit1:boot_CP+stop_CP;bit2:lcd; bit3:e2; bit4:xy_timer; bit5:lptimer; bit6:adc; bit7:timer2, bit8:spi, bit9: boot_CP+stop_CP(中断中执行)...*/
uint32_t g_mcu_bitmap = 0x1FF;

extern uint8_t g_uart_num;
extern uint32_t g_i2c_num;
extern uint32_t g_i2c_freq;
extern uint32_t  g_spi_num;
extern uint32_t g_spi_freq;
extern uint32_t g_spi_workmode;


/*进行单个模块的参数重配置，也可以触发周期性定时动作事件，甚至可以自行进行组合事件设计*/
int at_DEMOCFG_req(char *at_buf, char **prsp_cmd)
{
    if (g_cmd_type == AT_CMD_REQ)
	{
		char cmd[10] = {0};
		int  val1 = 0;
		int  val2 = 0;
		int  val3 = 0;
		int  val4 = 0;
        int  val5 = 0;

		at_parse_param("%s,%d,%d,%d,%d,%d", at_buf, cmd, &val1, &val2, &val3, &val4, &val5);

		/*功能点的测试，以AT+DEMOCFG=BITMAP,0X十六进制方式 输入位图即可*/
		if(!strcmp(cmd, "BITMAP"))
        {
            /*bit0:boot_CP; bit1:boot_CP+stop_CP;bit2:lcd; bit3:e2; bit4:xy_timer; bit5:lptimer; bit6:adc; bit7:timer2, bit8:spi, bit9: boot_CP+stop_CP(中断中执行)...*/
        	g_mcu_bitmap = val1;
            return XY_OK;
		}
		
		/*设置Test_CP_Event中启停CP的周期时长边界，构建随机性测试.AT+DEMOCFG=CP,<g_boot_min_sec>,<g_boot_max_sec>,<g_stop_min_sec>,<g_stop_max_sec>,<g_stop_timeout_ms>*/
		else if(!strcmp(cmd, "CP"))
	 	{
	 		at_parse_param(",%d,%d,%d,%d,%d", at_buf, &g_boot_min_sec, &g_boot_max_sec, &g_stop_min_sec, &g_stop_max_sec, &g_stop_timeout_ms);
			return XY_OK;
	 	}

		/*boot_CP后构造长时间的PING动作。AT+DEMOCFG=PING,<g_PING_flag>*/
		else if(!strcmp(cmd, "PING"))
	 	{
	 		at_parse_param(",%d,%d,%d,%d,%d", at_buf, &g_PING_flag);
			return XY_OK;
	 	}
		
		/*周期性进行RC32校准.AT+DEMOCFG=RC32,<timer_type>,<RC32 cal period sec>*/
		else if(!strcmp(cmd, "RC32"))
	 	{
	 		if(rc32_cal_test_set(prsp_cmd,val1,val2,val3,val4,val5)== XY_OK)
            {
                return XY_OK;
            }
            else
                return XY_ERR_PARAM_INVALID;

	 	}
		
        //测试LCD显示;命令格式：AT+DEMOCFG=LCD,...
        //val1=0 全显示；val1=1 全熄灭；val1=其他值 显示val2的值；
        else if(!strcmp(cmd, "LCD"))
        {
            if(val1 == 0)
            {
                ;
                HAL_Delay(1000);//延迟一秒钟，方便观察LCD屏幕显示
            }
            else if(val1 == 1)
            {
                
            }
            else
            {
                ;
                HAL_Delay(1000);//延迟一秒钟，方便观察LCD屏幕显示
            }
            return XY_OK;                        
        }
        
        //测试I2C读写EEPROM;命令格式：AT+DEMOCFG=I2C,...
        //AT+DEMOCFG=I2C,<val1><val2> I2C_EEPROM读写测试，val1是串口号，可选0-1，分别对应I2C1、I2C2;val2对应I2C操作E2的时间间隔，单位S。
        //读写测试说明：该测试每次进行512字节读写测试，每完成一次读写后，改变读写地址继续读写，直到完成对全片存储空间的读写为止。
		else if(!strcmp(cmd, "I2C"))
        {
            g_i2c_num = val1;
            g_i2c_freq = val2;
            return XY_OK;
        }
        
        //测试ADC;命令格式：AT+DEMOCFG=ADC,...
        //val = 1表示single模式获取温度；val = 2表示single模式获取VBAT电压值；val = 3,表示scan模式获取温度与VBAT的值 
        else if(!strcmp(cmd, "ADC"))
        {
            ADC_test_set(val1,val2,val3);
            return XY_OK;  

        }
        
        //AT+DEMOCFG=TIMER,<timer_type>,<timer_mode>,<xy_tmier period sec>,<Lptmier period sec>,<TIMER2 period sec>
		else if(!strcmp(cmd, "TIMER"))
        {
            timer_test_set(prsp_cmd,val1,val2,val3,val4,val5);
            return XY_OK;
        }

        //AT+DEMOCFG=UART,<val1> uart测试，val1是串口号，可选1-3;1:UART;2:CSP2_UART;3:CSP3_UART
		else if(!strcmp(cmd, "UART"))
        {
            if(val1 > 4 )
            {
                return XY_ERR_PARAM_INVALID;
            }
            else 
            {
                //uart_test_set(val1);   
                g_uart_num = val1;         
                return XY_OK;
            }
        }
        //spi测试 AT+DEMOCFG=SPI,<val1>,<val2>,<val3> 
        //val1:是spi编号，可选0-1,0：csp1_spi 1:spi;
        //val2:是频率(单位KHZ),csp1_spi时可选：100,500,1000,2000,4000,6000;spi时可选：360,720,1440,2880,5760
        //val3:工作模式，可选0,1,2,3
        else if(!strcmp(cmd, "SPI"))
        {
            g_spi_num = val1;
            g_spi_freq = val2;
            g_spi_workmode = val3;
            return XY_OK;        
        }
        //周期boot cp，中断里随机stop cp测试 AT+DEMOCFG=BSCP,<g_boot_period_ms>,<g_stop_max_ms>,<g_stop_min_ms>
        else if(!strcmp(cmd, "BSCP"))
        {
            at_parse_param(",%d,%d,%d", at_buf, &g_boot_period_ms, &g_stop_max_ms, &g_stop_min_ms);

            // 如果传入的参数有误，则按默认值处理
            if ((g_stop_max_ms<=g_stop_min_ms) || (g_stop_min_ms==0)) 
            {
                g_stop_max_ms = BSCP_STOP_TIMEOUT_MAX;
                g_stop_min_ms = BSCP_STOP_TIMEOUT_MIN;
                return XY_ERR_PARAM_INVALID;
            }
            
            return XY_OK;
        }
        else
        {
            return XY_ERR_PARAM_INVALID;
        }
    }
	else if (g_cmd_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(100);
		sprintf(*prsp_cmd, "\r\n+DEMOCFG:BITMAP,%lX ;TEST for ",g_mcu_bitmap);
		
		//bit0:boot_CP; bit1:boot_CP+stop_CP;bit2:lcd; bit3:e2; bit4:xy_timer; bit5:lptimer; bit6:adc; bit7:timer2, bit8:spi
		if(g_mcu_bitmap & (1<<0))
			sprintf(*prsp_cmd+strlen(*prsp_cmd), "boot_CP+");
		else if(g_mcu_bitmap & (1<<0))
			sprintf(*prsp_cmd+strlen(*prsp_cmd), "stop_CP+");
		else if(g_mcu_bitmap & (1<<0))
			sprintf(*prsp_cmd+strlen(*prsp_cmd), "lcd+");
		else if(g_mcu_bitmap & (1<<0))
			sprintf(*prsp_cmd+strlen(*prsp_cmd), "e2+");
		else if(g_mcu_bitmap & (1<<0))
			sprintf(*prsp_cmd+strlen(*prsp_cmd), "xy_timer+");
		else if(g_mcu_bitmap & (1<<0))
			sprintf(*prsp_cmd+strlen(*prsp_cmd), "lptimer+");
		else if(g_mcu_bitmap & (1<<0))
			sprintf(*prsp_cmd+strlen(*prsp_cmd), "adc+");
		else if(g_mcu_bitmap & (1<<0))
			sprintf(*prsp_cmd+strlen(*prsp_cmd), "timer2+");
		else if(g_mcu_bitmap & (1<<0))
			sprintf(*prsp_cmd+strlen(*prsp_cmd), "spi+");

		sprintf(*prsp_cmd+strlen(*prsp_cmd), "\r\n\r\nOK\r\n");
        return XY_OK;
	}
    else
    {
        return XY_ERR_PARAM_INVALID;
    }

}
volatile int g_flash_timeout = 0;
__RAM_FUNC void Flash_Write_Timeout(void)
{
	g_flash_timeout = 1;
}

uint8_t flash_data[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
uint32_t flash_offset = 0;
uint32_t writen_page = 0;
uint8_t flash_reset_num = 0;
void Flash_Test(void)
{
	static uint8_t s_flash_flag = 0;
	/*非深睡唤醒的场景上电*/
	if (0 == s_flash_flag)
	{
		s_flash_flag = 1;
		Timer_AddEvent(WRITE_FLASH_TIMER, ((FLASH_TIMER_BASE + rand() % FLASH_TIMER_RANGE) * 1000), Flash_Write_Timeout, 0);
	}
	else if (g_flash_timeout)
	{
		g_flash_timeout = 0;

		xy_assert(xy_Flash_Write(USER_FLASH_BASE + flash_offset, flash_data, 16));
		flash_offset += 16;
		if(flash_offset % FLASH_SECTOR_LENGTH == 0)
		{
			writen_page++;
			xy_printf("\r\nflash write page:%d\r\n", writen_page);
			if(flash_offset == USER_FLASH_LEN_MAX)
			{
				flash_offset = 0;
				flash_data[0 + flash_reset_num] = 0x5A;
				flash_data[4 + flash_reset_num] = 0x5A;
				flash_data[8 + flash_reset_num] = 0x5A;
				flash_data[12 + flash_reset_num] = 0x5A;
				flash_reset_num++;
				xy_printf("\r\nall user flash is writen:%d\r\n", flash_reset_num);
				xy_assert(flash_reset_num < 4);
			}
		}
		Timer_AddEvent(WRITE_FLASH_TIMER, ((FLASH_TIMER_BASE + rand() % FLASH_TIMER_RANGE) * 1000), Flash_Write_Timeout, 0);
	}
}

extern void Start_CP_Test();
extern void Stop_CP_Test();
extern void User_Hook_Regist(void);

__RAM_FUNC int main(void)
{
	SystemInit();

    FastRecov_Init_Hook_Regist(User_Init_FastRecovery);
	User_Hook_Regist();
	// //默认加上standby锁，使芯片不进standby
    // LPM_LOCK(STANDBY_DEFAULT);
	
	xy_printf("meter demo start\n");

	Debug_Runtime_Init();
	
	while(1)
	{
		at_uart_recv_and_process();
		CP_URC_Process();

		/*bit0:boot_CP; bit1:boot_CP+stop_CP;bit2:lcd; bit3:e2; bit4:xy_timer; bit5:lptimer; bit6:adc; bit7:timer2, bit8:spi, bit9: boot_CP+stop_CP(中断中执行)...*/
		if (g_mcu_bitmap & 1 << 0)
		{
			Start_CP_Test();
		}
		if (g_mcu_bitmap & 1 << 1)
		{
			Stop_CP_Test();
		}
		// if (g_mcu_bitmap & (1 << 2))
        // {
		//     Lcd_Work();
        // }
        if (g_mcu_bitmap & (1 << 3))
        {
		    E2_Work();
        }
        if(g_mcu_bitmap & (1 << 4))
        {
		    XY_TIMER_Work();  //添加了看门狗功能
        }
        if(g_mcu_bitmap & (1 << 5))
        {
		    Lptimer_Work();   //添加了看门狗功能
        }
        // if(g_mcu_bitmap & (1 << 6))
        // {
		// 	Uart_Work();
        // }
        if(g_mcu_bitmap & (1 << 7))
        {
		    TIMER2_Work();
        }
        if(g_mcu_bitmap & (1 << 8))
        {
		    SPI_Work();
        }
        if(g_mcu_bitmap & (1 << 9))
        {
		    Flash_Test();
        }

        RC32k_Cali_Process();

		Enter_LowPower_Mode(LPM_DSLEEP);
	}
}
