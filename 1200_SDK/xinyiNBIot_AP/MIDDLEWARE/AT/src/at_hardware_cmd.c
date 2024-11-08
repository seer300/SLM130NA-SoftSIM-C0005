#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "xinyi2100.h"
#include "xy_printf.h"
#include "hal_gpio.h"
#include "uart.h"
#include "hal_lpuart.h"
#include "at_uart.h"
#include "at_process.h"
#include "at_hardware_cmd.h"
#include "at_cmd_regist.h"
#include "xy_flash.h"
#include "mcu_adapt.h"
#include "at_uart.h"


extern HAL_LPUART_HandleTypeDef g_at_lpuart;

uint8_t g_baudrate_flag = 0;	//波特率切换标识

extern char g_at_RecvBuffer[];

#if VER_BC95

static int32_t g_old_parity = 0,g_new_parity = 0;
static int32_t g_old_rate = 0;
static int32_t g_rate_store = -1;
static int32_t g_natspeed_timeout = 0;

//切换超时，回退到原波特率。中断的回调
__RAM_FUNC void natspeed_timeout_hook()
{
	if (g_baudrate_flag)
	{
		g_baudrate_flag = 0;
		at_uart_config(g_old_rate,g_old_parity);
		g_at_lpuart.Init.BaudRate = g_old_rate;
		HWREGB(SOFT_VAR_NV_ATPARITY) = (uint8_t)g_old_parity;
		set_standby_by_rate(g_at_lpuart.Init.BaudRate);
		LPM_UNLOCK(XY_NATSPEED_LOCK);
	}
}

//在切换波特率后收到外部MCU一条正确的AT命令后,表示波特率切换成功
void natspeed_succ_hook()
{
	if (g_baudrate_flag  && (g_at_RecvBuffer[0] == 'A'||g_at_RecvBuffer[0] == 'a' ) && (g_at_RecvBuffer[1] == 'T'||g_at_RecvBuffer[1] == 't') )
	{
		g_baudrate_flag = 0;
		if (g_rate_store == 1)
		{
			uint16_t temp_baudrate = (g_at_lpuart.Init.BaudRate == 1200)? 0x03 : (g_at_lpuart.Init.BaudRate / 2400);
			
			WRITE_FAC_PARAM(at_uart_rate,&temp_baudrate);
			WRITE_FAC_PARAM(at_parity, &g_new_parity);
		}
		Timer_DeleteEvent(TIMER_WAIT_AT_RSP);
		LPM_UNLOCK(XY_NATSPEED_LOCK);
	}
}

void natspeed_change_hook()
{
	// while(UARTTxFifoStatusGet(UART1_BASE,UART_FIFO_EMPTY)==0); 
    while(LPUART_IS_TXFIFO_EMPTY() == 0){;}
    for(volatile uint32_t i=0;i<10000;i++){;}

	at_uart_config(g_at_lpuart.Init.BaudRate,g_new_parity);
	HWREGB(SOFT_VAR_NV_ATPARITY) = (uint8_t)g_new_parity;
	set_standby_by_rate(g_at_lpuart.Init.BaudRate);
	Timer_AddEvent(TIMER_WAIT_AT_RSP,(g_natspeed_timeout*1000),natspeed_timeout_hook, 0);	
}
/**
  * @brief  AT串口配置AT命令
  * @param  param:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 请求类AT命令：AT+NATSPEED=<baud_rate>,<timeout>,<store>,[<sync_mode>[,<stopbits>[,<parity>[,<xonxoff>]]]]
  *      @arg <baud_rate>,可配置波特率（必选）：1200,2400,4800,9600,57600,115200,230400,460800，921600
  *      @arg <timeout>,超时时间（必选）,可设(0~30)s，默认3s,0表示使用默认值。
  *      @arg <store>,是否存储在NV中（必选），1为存储，将设置的波特率保存到at_uart_rate参数中，0为不保存到NV中， 设置后立即生效，重启后失效。
  *      @arg <sync_mode>,同步模式,只支持[0]，0表示正常采样，不支持提前或稍后采样。
  *      @arg <stopbits>,停止位，只支持[2],2表示两个停止位。
  *      @arg <parity>,校验位，支持[0，1，2],0表示无校验，1表示偶（Even）校验，2表示奇（odd）校验。
  *      @arg <xonoff>,软件流控，只支持[0]，0表示禁用软件流控。
  * @arg 查询类AT命令：AT+NATSPEED?
  * @arg 测试类AT命令：AT+NATSPEED=?
  * 
  * @attention 波特率大于9600则关闭standby,小于等于9600则打开standby。
  * @attention LPUART只能在小于等于9600波特率情况下，才能保证唤醒后不丢AT命令；建议客户不要选用高于9600波特率。
  */
int at_NATSPEED_req(char  *param, char **rsp_cmd)
{
	uint32_t baud_rate=9600,parity_bit = 0;
	int default_timeout = 3;
	int sync_mode = 0;
	int stopbits = 2;
	int xonxoff = 0;
	

    if (g_cmd_type == AT_CMD_TEST) //测试类
	{
		*rsp_cmd = xy_malloc(200);
		sprintf(*rsp_cmd, "+NATSPEED:(1200,2400,4800,9600,57600,115200,230400,460800,921600),(0-30),(0,1),(0),(2),(0-2),(0)");
	}
    else if (g_cmd_type == AT_CMD_QUERY) //查询类
	{
		if((g_at_lpuart.Instance->CTRL & UART_CTL_PARITY_Msk) == UART_CTL_PARITY_ODD) parity_bit = 1;
		else if ((g_at_lpuart.Instance->CTRL & UART_CTL_PARITY_Msk) == UART_CTL_PARITY_EVEN) parity_bit = 2;
		else parity_bit = 0;

        *rsp_cmd = xy_malloc(40);
		sprintf(*rsp_cmd, "+NATSPEED:%d,%d,%d,%d,%d", (int)g_at_lpuart.Init.BaudRate, 0, stopbits, (int)parity_bit, 0);
	}
    else if (g_cmd_type == AT_CMD_REQ) //设置类
	{
		g_old_rate = g_at_lpuart.Init.BaudRate;
		g_old_parity = (g_at_lpuart.Instance->CTRL & UART_CTL_PARITY_Msk) >> UART_CTL_PARITY_Pos;

		at_parse_param("%d,%d,%d,%d,%d,%d,%d", param, &baud_rate, &g_natspeed_timeout, &g_rate_store, &sync_mode, &stopbits, &parity_bit, &xonxoff);

		uint32_t valid_baud_flag = 0;
		uint32_t valid_baud[] = {1200, 2400, 4800, 9600, 57600, 115200, 230400, 460800, 921600};
		for (uint32_t i = 0; i < sizeof(valid_baud) / sizeof(valid_baud[0]); i++)
		{
			if (baud_rate == valid_baud[i])
			{
				valid_baud_flag = 1;
				break;
			}
		}
		//所有参数的合法性检测
		if ((valid_baud_flag == 0) || \
			((g_natspeed_timeout > 30)||(g_natspeed_timeout < 0)) || \
			(g_rate_store != 0 && g_rate_store != 1) || \
			(sync_mode != 0) || \
			(stopbits != 2) || \
			(parity_bit > 2) || \
			(xonxoff != 0))
		{
			return XY_ERR_PARAM_INVALID;
		}

		//xy1200的lpuart奇偶校验与移远该项参数取值相反
		if(parity_bit == 2 ) g_new_parity = 1;  
		else if(parity_bit == 1 ) g_new_parity = 2;
		else g_new_parity = 0;

		if (g_natspeed_timeout == 0)
			g_natspeed_timeout = default_timeout;

		g_baudrate_flag = 1;
		g_at_lpuart.Init.BaudRate = baud_rate;
		LPM_LOCK(XY_NATSPEED_LOCK);			
	}
	// else
	// {
	// 	return XY_ERR_PARAM_INVALID;
	// }
	return XY_OK;
}
#else
/**
  * @brief  AT串口配置AT命令
  * @param  param:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 请求类AT命令：AT+NATSPEED=<baud_rate>[,<timeout>,[<store>,[<sync_mode>[,<stopbits>]]]]
  *      @arg <baud_rate>,可配置波特率（必选参数）：2400,4800,9600,57600,115200,230400,460800,921600
  *      @arg <timeout>,超时时间,不支持,默认填为0。
  *      @arg <store>,是否存储在NV中，1存储，0不存储。默认为1：存储。
  *      @arg <sync_mode>,同步模式,只支持[0]，0表示正常采样，不支持提前或稍后采样。
  *      @arg <stopbits>,停止位，只支持[2],2表示两个停止位。
  * @arg 查询类AT命令：AT+NATSPEED?
  * @arg 测试类AT命令：AT+NATSPEED=?
  * 
  * @attention 波特率大于9600则关闭standby,小于等于9600则打开standby。
  * @attention LPUART只能在小于等于9600波特率情况下，才能保证唤醒后不丢AT命令；建议客户不要选用高于9600波特率。
  */
int at_NATSPEED_req(char  *param, char **rsp_cmd)
{
	uint32_t baud_rate;
	int store=1;
	int stopbits = 2;
	int sync_mode = 0;
	int natspeed_timeout = 0;

	if (g_cmd_type == AT_CMD_QUERY) //查询类 
	{
		*rsp_cmd = xy_malloc(100);
		sprintf(*rsp_cmd, "+NATSPEED:%d", g_at_lpuart.Init.BaudRate);		
	}
	else if(g_cmd_type == AT_CMD_TEST) //测试类
	{
		*rsp_cmd = xy_malloc(200);
		sprintf(*rsp_cmd,  "+NATSPEED:(2400,4800,9600,57600,115200,230400,460800,921600)");		
	}
	else if(g_cmd_type == AT_CMD_REQ) //设置类
	{
		at_parse_param("%d,%d,%d,%d,%d", param, &baud_rate, &natspeed_timeout, &store, &sync_mode,&stopbits);

		uint32_t valid_baud_flag = 0;
		uint32_t valid_baud[] = {2400, 4800, 9600, 57600, 115200, 230400, 460800, 921600};
		for (uint32_t i = 0; i < sizeof(valid_baud) / sizeof(valid_baud[0]); i++) //波特率合法性检测
		{
			if (baud_rate == valid_baud[i])
			{
				valid_baud_flag = 1;
				break;
			}
		}

		//所有参数的合法性检测
		if ((valid_baud_flag == 0) || \
			(natspeed_timeout != 0) || \
			(store != 0 && store != 1) || \
			(sync_mode != 0) || \
			(stopbits != 2))
		{
			return XY_ERR_PARAM_INVALID;
		}

		if(store == 1)
		{
			uint16_t temp_baudrate = baud_rate / 2400;
			WRITE_FAC_PARAM(at_uart_rate, &temp_baudrate);

			Send_AT_to_Ext("\r\nREBOOTING\r\n");

			// while(UARTTxFifoStatusGet(UART1_BASE,UART_FIFO_EMPTY)==0); 
			while(LPUART_IS_TXFIFO_EMPTY() == 0){;}
            for(volatile uint32_t i=0;i<10000;i++){;}
			
			xy_Soft_Reset(SOFT_RB_BY_NRB);	//重启	
		}
		else 
		{	
			at_uart_config(baud_rate,0); //切换波特率
			g_at_lpuart.Init.BaudRate = baud_rate;		
			set_standby_by_rate(g_at_lpuart.Init.BaudRate);
		}	
	}
	// else
	// {
	// 	return XY_ERR_PARAM_INVALID
	// }
	return XY_OK;
}
#endif
/**
  * @brief  AT串口配置(波特率（必选），是否存储在NV中（可选），是否打开standby（可选）)
  * @param  param:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 请求类AT命令：AT+UARTSET=<baud_rate>[,<store>[,<open standby>]]
  *      @arg <baud_rate>,可配置波特率（必选）：2400,4800,9600,19200,38400,57600,115200,230400,460800,921600
  *      @arg <store>,是否动态波特率，默认值为0，即动态切换波特率。
  * 				1：表示固定波特率保存到NV中；
  * 				0：表示动态波特率生效，需要对方同步切换波特率。
  *      @arg <open standby>,是否打开standby睡眠，由于大于9600波特率时，standby会导致脏数据，故不建议客户使用此参数，维持默认设置即可。
  * 				1：表示启动standby睡眠机制；
  * 				0：表示波特率大于9600则关闭standby,小于等于9600则打开standby。
  * @arg 查询类AT命令：AT+UARTSET?
  * @arg 测试类AT命令：AT+UARTSET=?  
  * @attention LPUART只能在小于等于9600波特率情况下，才能保证唤醒后不丢AT命令；建议客户不要选用高于9600波特率。
  *  
  */
int at_UARTSET_req(char *param, char **prsp_cmd)
{
	if (g_cmd_type == AT_CMD_REQ)
	{
		int store = 0;
		int open_standby = -1;
		uint32_t baud_rate;

		at_parse_param("%d,%d,%d", param, &baud_rate, &store, &open_standby);	
		int valid_baud_flag = 0;
		uint32_t valid_baud[] = {2400, 4800, 9600, 19200,38400,57600, 115200, 230400, 460800, 921600};
		for(uint32_t i = 0; i < sizeof(valid_baud) / sizeof(valid_baud[0]); i++)
		{
			if(baud_rate == valid_baud[i])
			{
				valid_baud_flag = 1;
				break;
			}
		}

		if ((valid_baud_flag == 0) || \
			(store != 0 && store != 1) || \
			(open_standby != 0 && open_standby != 1 && open_standby != -1))
		{
			return XY_ERR_PARAM_INVALID;
		}

		if (store == 1)
		{
			uint16_t temp_baudrate = baud_rate / 2400;
			WRITE_FAC_PARAM(at_uart_rate, &temp_baudrate);

			Send_AT_to_Ext("\r\nREBOOTING\r\n");

            // while(UARTTxFifoStatusGet(UART1_BASE,UART_FIFO_EMPTY)==0);
            while(LPUART_IS_TXFIFO_EMPTY() == 0){;}
            for(volatile uint32_t i=0;i<10000;i++){;}

			xy_Soft_Reset(SOFT_RB_BY_NRB);	//重启
		}
		else
		{
			at_uart_config(baud_rate,0); //切换波特率	 
			g_at_lpuart.Init.BaudRate = baud_rate;	
			set_standby_by_rate(g_at_lpuart.Init.BaudRate);
		}
	}
	else if (g_cmd_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(30);
		sprintf(*prsp_cmd, "+UARTSET:%d", (READ_FAC_NV(uint16_t,at_uart_rate) & 0x1ff) * 2400);
	}
	else if (g_cmd_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(100);
		sprintf(*prsp_cmd, "+UARTSET:(2400,4800,9600,19200,38400,57600,115200,230400,460800,921600)");
	}
	// else
	// 	*prsp_cmd = AT_ERR_BUILD(ATERR_PARAM_INVALID);

	return XY_OK;
}

void set_and_save_baudrate(uint32_t baud_rate)
{
	if (baud_rate == 0)
		return;
	at_uart_config(baud_rate, 0); // 切换波特率
	set_standby_by_rate(g_at_lpuart.Init.BaudRate);
	baud_rate = baud_rate / 2400;
	WRITE_FAC_PARAM(at_uart_rate, &baud_rate);
}

/**
 * @brief  AT串口配置(波特率)
 * @param  param:
 * @param  prsp_cmd:
 * @retval AT_END
 * @arg 请求类AT命令：AT+IPR=<baud_rate>
 *      @arg <baud_rate>,可配置波特率：0,4800,9600,19200,38400,57600,115200,230400,460800,921600;0表示启用波特率自适应
 *		 若设置的波特率高于9600，则同时修改NV关闭standby
 * @arg 查询类AT命令：AT+IPR?
 * @arg 测试类AT命令：AT+IPR=?
 * 
 * @attention 波特率大于9600则关闭standby,小于等于9600则打开standby。
 * @attention LPUART只能在小于等于9600波特率情况下，才能保证唤醒后不丢AT命令；建议客户不要选用高于9600波特率。
 */
int at_IPR_req(char  *param, char **rsp_cmd)
{
#if VER_BC95
	(void)rsp_cmd;
	int32_t baud_rate = -1;
	if (g_cmd_type == AT_CMD_REQ) //设置类
	{
		at_parse_param("%d", param, &baud_rate);
		if (baud_rate != 0)	//仅支持AT+IPR=0,功能为开启波特率自适应	
		{
			return XY_ERR_PARAM_INVALID;
		}	
		WRITE_FAC_PARAM(at_uart_rate, &baud_rate);
	}
#else
	if (g_cmd_type == AT_CMD_TEST)  //测试类
	{
		*rsp_cmd = xy_malloc(128);
#if VER_BC25
		sprintf(*rsp_cmd, "(4800,9600,19200,38400,57600,115200,230400,460800,921600)");
#else
		sprintf(*rsp_cmd, "+IPR:(4800,9600,19200,38400,57600,115200,230400,460800,921600)");
#endif
	}
	else if (g_cmd_type == AT_CMD_QUERY) //查询类
	{
		*rsp_cmd = xy_malloc(40);
#if VER_BC25
		sprintf(*rsp_cmd, "%d", g_at_lpuart.Init.BaudRate);
#else
		sprintf(*rsp_cmd, "+IPR:%d", g_at_lpuart.Init.BaudRate);
#endif
	}
	else if (g_cmd_type == AT_CMD_REQ) //设置类
	{
		uint32_t baud_rate = 9600;
		at_parse_param("%d", param, &baud_rate);

		uint8_t valid_baud_flag = 0;

		uint32_t valid_baud[] = {4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};

		for (uint32_t i = 0; i < sizeof(valid_baud) / sizeof(valid_baud[0]); i++)
		{
			if (baud_rate == valid_baud[i])
			{
				valid_baud_flag = 1;
				break;
			}
		}

		if ((valid_baud_flag == 0))
		{
			return XY_ERR_PARAM_INVALID;
		}

			g_at_lpuart.Init.BaudRate = baud_rate;
#if VER_BC25
		g_baudrate_flag = 1;
		return XY_OK;
#endif
		set_and_save_baudrate(baud_rate);
	
	}
#endif
	return XY_OK;
}

/**
 * @brief 按键生效时长命令
 * 
 * 请求命令：AT+RESETCTL=<mode>
 * 当LPUART_RXD_RESET_MUX为0时，mode可取0~11中任意值
 *          @arg <mode>:0，按键高电平大于160ms唤醒，大于5.12s复位，NV默认值
 *          @arg <mode>:1，按键高电平大于10ms唤醒，大于320ms复位
 *          @arg <mode>:2，按键高电平大于10ms唤醒，大于1.28s复位
 *          @arg <mode>:3，按键高电平大于10ms唤醒，大于2.56s复位
 * 			@arg <mode>:4，按键高电平大于10ms唤醒，大于5.12s复位
 *          @arg <mode>:5，按键高电平大于20ms唤醒，大于320ms复位
 *          @arg <mode>:6，按键高电平大于20ms唤醒，大于1.28s复位
 *          @arg <mode>:7，按键高电平大于20ms唤醒，大于2.56s复位
 * 			@arg <mode>:8，按键高电平大于20ms唤醒，大于5.12s复位
 *          @arg <mode>:9，按键高电平大于160ms唤醒，大于320ms复位
 *          @arg <mode>:10，按键高电平大于160ms唤醒，大于1.28s复位
 *          @arg <mode>:11，按键高电平大于160ms唤醒，大于2.56s复位
 *          @arg <mode>:12 按键低电平大于160ms唤醒，大于5.12s复位
 *          @arg <mode>:13 按键低电平大于10ms唤醒，大于320ms复位
 *          @arg <mode>:14 按键低电平大于10ms唤醒，大于1.28s复位
 *          @arg <mode>:15 按键低电平大于10ms唤醒，大于2.56s复位
 *          @arg <mode>:16 按键低电平大于10ms唤醒，大于5.12s复位
 *          @arg <mode>:17 按键低电平大于20ms唤醒，大于320ms复位
 *          @arg <mode>:18 按键低电平大于20ms唤醒，大于1.28s复位
 *          @arg <mode>:19 按键低电平大于20ms唤醒，大于2.56s复位
 *          @arg <mode>:20 按键低电平大于20ms唤醒，大于5.12s复位
 *          @arg <mode>:21 按键低电平大于160ms唤醒，大于320ms复位
 *          @arg <mode>:22 按键低电平大于160ms唤醒，大于1.28s复位
 *          @arg <mode>:23 按键低电平大于160ms唤醒，大于2.56s复位
 * 当LPUART_RXD_RESET_MUX为1时，mode可取0~3中任意值
 *          @arg <mode>:0，按键低电平大于2.56s复位，NV默认值
 *          @arg <mode>:1，按键低电平大于5.12s复位
 *          @arg <mode>:2，按键低电平大于320ms复位
 *          @arg <mode>:3，按键低电平大于1.28s复位
 *          @arg <mode>:4 按键高电平大于2.56s复位
 *          @arg <mode>:5 按键高电平大于5.12s复位
 *          @arg <mode>:6 按键高电平大于320ms复位
 *          @arg <mode>:7 按键高电平大于1.28s复位
 * 执行命令：AT+RESETCTL
 * 查询命令：AT+RESETCTL?，查询当前设定档位
 * 测试命令：AT+RESETCTL?，查询所有可设定档位
 * 
 */
int at_RESETCTL_req(char *param, char **rsp_cmd)
{
	if (g_cmd_type == AT_CMD_REQ) //请求类
	{
		WakeupEn_InitTypeDef wkup_en = {0};
		uint8_t rstwkp_pw = 0;
		int32_t val1 = -1;

		//参数解析并检查，若非负数则赋值给rstwkp_pw
		at_parse_param("%d", param, &val1);
		if (val1 < 0)
		{
			return XY_ERR_PARAM_INVALID;
		}
		rstwkp_pw = (uint8_t)val1;

		// 配置唤醒电平
		if ( rstwkp_pw <=11 ) //wkup_en引脚既有复位功能又有唤醒功能，设置其默认低电平状态，唤醒电平为高电平，下降沿触发
		{
			wkup_en.mode = WAKEUP_AND_RESET;
			wkup_en.pull = PIN_PULLDOWN;
			wkup_en.wkup_polarity = POLARITY_HIGH;
			wkup_en.wkup_edge = FALLING;
		}
		else if ( rstwkp_pw >=12 && rstwkp_pw <=23 ) //wkup_en引脚既有复位功能又有唤醒功能，设置其默认高电平状态，唤醒电平为低电平，上升沿触发
		{
			wkup_en.mode = WAKEUP_AND_RESET;
			wkup_en.pull = PIN_PULLUP;
			wkup_en.wkup_polarity = POLARITY_LOW;
			wkup_en.wkup_edge = RISING;

			rstwkp_pw -= 12;
		}
		else
		{
			return XY_ERR_PARAM_INVALID;
		}
		
		// 配置唤醒脉宽
		switch(rstwkp_pw)
		{
			case 1:  { wkup_en.wakeup_time = PULSE_10ms; wkup_en.reset_time = PLUSE_320ms; break; }
			case 2:  { wkup_en.wakeup_time = PULSE_10ms; wkup_en.reset_time = PLUSE_1p28s; break; }
			case 3:  { wkup_en.wakeup_time = PULSE_10ms; wkup_en.reset_time = PLUSE_2p56s; break; }
			case 4:  { wkup_en.wakeup_time = PULSE_10ms; wkup_en.reset_time = PLUSE_5p12s; break; }
			case 5:  { wkup_en.wakeup_time = PULSE_20ms; wkup_en.reset_time = PLUSE_320ms; break; }
			case 6:  { wkup_en.wakeup_time = PULSE_20ms; wkup_en.reset_time = PLUSE_1p28s; break; }
			case 7:  { wkup_en.wakeup_time = PULSE_20ms; wkup_en.reset_time = PLUSE_2p56s; break; }
			case 8:  { wkup_en.wakeup_time = PULSE_20ms; wkup_en.reset_time = PLUSE_5p12s; break; }
			case 9:  { wkup_en.wakeup_time = PULSE_160ms; wkup_en.reset_time = PLUSE_320ms; break; }
			case 10: { wkup_en.wakeup_time = PULSE_160ms; wkup_en.reset_time = PLUSE_1p28s; break; }
			case 11: { wkup_en.wakeup_time = PULSE_160ms; wkup_en.reset_time = PLUSE_2p56s; break; } 
			case 0: { wkup_en.wakeup_time = PULSE_160ms; wkup_en.reset_time = PLUSE_5p12s; break; } //NV默认值
			default:;
		}

		WakeupEn_Init(&wkup_en);
		
		//保存val1的值至NV参数resetctl中
		WRITE_FAC_PARAM(resetctl, &val1);
	}
	else if( g_cmd_type == AT_CMD_ACTIVE ) //执行类
	{

	}
	else if( g_cmd_type == AT_CMD_QUERY ) //查询类
	{
		*rsp_cmd = xy_malloc(30);
		sprintf(*rsp_cmd, "+RESETCTL:%d", READ_FAC_NV(uint8_t, resetctl));
	}
	else if( g_cmd_type == AT_CMD_TEST ) //测试类
	{
		*rsp_cmd = xy_malloc(120);

		sprintf(*rsp_cmd, "+RESETCTL:(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23)");
	}
	else
	{
		return XY_ERR_PARAM_INVALID;
	}
	return XY_OK;
}


/*******************************************************************************
 *						 user set GPIO PIN here by self module				   *
 ******************************************************************************/
#define XYCNNT_TEST_SUM (10 * 2)
char g_PIN_ID[XYCNNT_TEST_SUM] = {MCU_GPIO52, 	MCU_GPIO54,
								  MCU_GPIO5, 	MCU_GPIO6,
								  MCU_GPIO7, 	MCU_GPIO9,
								  MCU_GPIO8, 	MCU_GPIO13,
								  MCU_WKP2, MCU_WKP1,
								  MCU_GPIO20, 	MCU_GPIO21,
								  MCU_GPIO22, 	MCU_GPIO23,
								  MCU_GPIO25, 	MCU_GPIO26,
								  MCU_GPIO14, 	MCU_GPIO24,
								  MCU_GPIO10, 	MCU_WKP3,};

int g_XYCNNT_bitmap = 0xFFFFF; //此值为XYCNNT_TEST_SUM 个bit的1

/**
  * @brief  测试所选引脚输入输出是否正常（数字信号），两两配对测试：一个引脚输出电平，另一个引脚读取。
  * @param  gpio_in:输入所选引脚，将会用来读取电平
  * @param  gpio_out:输出所选引脚
  * @param  is_High:@param  gpio_out输出的高电平或低电平
  *                 @arg 1:高电平
  *                 @arg 0:低电平
  * @retval 所选输入引脚读取所选输出引脚电平状态是否正确
  *         @arg 1:正确
  *         @arg 0:错误
  */
int check_connectivity(char gpio_in, char gpio_out, char is_High)
{
	unsigned char read_rlt;
	volatile unsigned int soft_delay;

	McuGpioModeSet(gpio_in,0x11);//浮空输入
	McuGpioModeSet(gpio_out,0x00);//推挽输出

	McuGpioWrite(gpio_out, is_High); 

	for (soft_delay = 0; soft_delay < 1000; soft_delay++);

	read_rlt = McuGpioRead(gpio_in);

	if ((is_High == 1 && read_rlt == 1) || (is_High == 0 && read_rlt == 0))
		return 1;
	else
		return 0;
}

/**
  * @brief  循环测试所选的所有引脚，并且读取SEN_4处的电压（需有分压电阻）
  * @param  prsp_cmd:,test_cmd:是否是测试类命令，1为测试类命令，0为查询类命令
  * @retval 最终结果
  *         @arg 1:正确
  *         @arg 0:错误
  */
int do_XYCNNT_process(char **prsp_cmd, int test_cmd)
{
	int ret = 1;
	*prsp_cmd = xy_malloc(4 * XYCNNT_TEST_SUM + 100);

	if (test_cmd == 0)
		snprintf(*prsp_cmd, 30, "\r\n+XYCNNT:bitmap=0x%x\r\n", g_XYCNNT_bitmap);
	else if (test_cmd == 1)
		snprintf(*prsp_cmd, 31, "\r\n+XYCNNT:bitmap=(0-0x%x)\r\n", g_XYCNNT_bitmap);

	if (g_XYCNNT_bitmap == 0)
	{
		snprintf(*prsp_cmd + strlen(*prsp_cmd), 25, "\r\n+XYCNNT:FAIL");
		return 0;
	}

	snprintf(*prsp_cmd + strlen(*prsp_cmd), 20, "\r\n+XYCNNT:");

	for (int i = 0; i < XYCNNT_TEST_SUM; i++)
	{
		if (g_XYCNNT_bitmap & (1 << i))
		{
			if (i % 2 == 0)
			{
				if (check_connectivity(g_PIN_ID[i + 1], g_PIN_ID[i], 1))
					snprintf(*prsp_cmd + strlen(*prsp_cmd), 10, "%x%c", i, 'Y');
				else
				{
					snprintf(*prsp_cmd + strlen(*prsp_cmd), 10, "%x%c", i, 'N');
					ret = 0;
				}

				if (check_connectivity(g_PIN_ID[i + 1], g_PIN_ID[i], 0))
					snprintf(*prsp_cmd + strlen(*prsp_cmd), 5, "%c,", 'Y');
				else
				{
					snprintf(*prsp_cmd + strlen(*prsp_cmd), 5, "%c,", 'N');
					ret = 0;
				}
			}
			else
			{
				if (check_connectivity(g_PIN_ID[i - 1], g_PIN_ID[i], 1))
					snprintf(*prsp_cmd + strlen(*prsp_cmd), 10, "%x%c", i, 'Y');
				else
				{
					snprintf(*prsp_cmd + strlen(*prsp_cmd), 10, "%x%c", i, 'N');
					ret = 0;
				}

				if (check_connectivity(g_PIN_ID[i - 1], g_PIN_ID[i], 0))
					snprintf(*prsp_cmd + strlen(*prsp_cmd), 5, "%c,", 'Y');
				else
				{
					snprintf(*prsp_cmd + strlen(*prsp_cmd), 5, "%c,", 'N');
					ret = 0;
				}
			}
		}
	}

	if (ret == 1)
	{
		snprintf(*prsp_cmd + strlen(*prsp_cmd), 20, "SUCCESS");
		return 1;
	}
	else
	{
		snprintf(*prsp_cmd + strlen(*prsp_cmd), 20, "FAIL");
		return 0;
	}
}

/**
  * @brief  引脚测试相关的AT命令处理函数
  * @param  at_buf:
  * @param  prsp_cmd:
  * @retval AT_END
  * @arg 请求类AT命令：AT+XYCNNT=<bit map>
  * @arg 查询类AT命令：AT+XYCNNT?
  * @arg 测试类AT命令：AT+XYCNNT=?
  * @attention 此指令仅限于示例用，用户根据具体使用的GPIO自行调整维护。
  */
int at_XYCNNT_req(char *at_buf, char **prsp_cmd)
{
	int temp_bitmap;
	if (g_cmd_type == AT_CMD_REQ)
	{
		at_parse_param("%d", at_buf, &temp_bitmap);

		if(temp_bitmap > g_XYCNNT_bitmap )
			return XY_ERR_PARAM_INVALID;

		g_XYCNNT_bitmap = temp_bitmap;
	}
	else if (g_cmd_type == AT_CMD_QUERY)
	{
		do_XYCNNT_process(prsp_cmd, 0);
	}
	else if (g_cmd_type == AT_CMD_TEST)
	{
		do_XYCNNT_process(prsp_cmd, 1);
	}
	// else
	// 	return  (ATERR_PARAM_INVALID);

	return XY_OK;
}


