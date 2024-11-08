/**
  ******************************************************************************
  * @file    hc32_timer_driver.h
  * @author  (C)2015, Qindao ieslab Co., Ltd
  * @version V1.0
  * @date    2015-12-25
  * @brief   the function of the entity of system processor
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/


#ifndef __UWATER_TIMER1US_DRIVER_H__
#define __UWATER_TIMER1US_DRIVER_H__


#ifdef __cplusplus
extern "C" {
#endif

	
#include "xy_system.h"
#include "type.h"
	 

#define TIME_1uS_Timer1			1
#define TIME_1uS_Timer2			0

#define TRUE					(1)
#define FALSE					(0)
 
 
 typedef enum{
	TIMER_5MS_KEY, 
	TIMER_5MS_ADC, 
	TIMER_5MS_ADC_OVERTIME, 
	TIMER_5MS_POW_DOWN, 
	TIMER_5MS_POW_ON, 
    TIMER_5MS_PULSE,
	TIMER_5MS_NBIOT,
	TIMER_5MS_MOVEALERT,
	TIMER_5MS_LEAKAGE,
	TIMER_5MS_TAMPER,
	TIMER_5MS_LCD,
	TIMER_5MS_UART0, 
	TIMER_5MS_UART1, 
	TIMER_5MS_UART2, 
	TIMER_5MS_UART3, 
	TIMER_5MS_LPUART0, 
	TIMER_5MS_LPUART1, 
    TIMER_5MS_VALVE,
	TIMER_5MS_VALVE_MAIN,
	TIMER_5MS_VALVE_PAUSE,
	TIMER_5MS_VALVE_ADC_CHECK,
	TIMER_5MS_VALVE_INIT,
	TIMER_5MS_LOWPOWER,
	TIMER_5MS_BLE_MONITOR,
    
    TIMER_5MS_MAX_SUM
} TIMER_5MS_ENUM;
 
typedef enum{   
    TIMER_100MS_TEST,
	TIMER_100MS_UPLOAD_MONITOR,
	TIMER_100MS_EEPROM,
	TIMER_100MS_SYS_UPLOAD,
	TIMER_100MS_BUZZER,
	TIMER_100MS_UP_DEAL,
	TIMER_100MS_FRAME,
	TIMER_100MS_NB_ONLINE,
	TIMER_100MS_NB_REV,
	TIMER_100MS_NB_ECLSNR,
	TIMER_100MS_WAIT_FRAME,
	TIMER_100MS_NBIOT,
	TIMER_100MS_POWERON_LCD,
	TIMER_100MS_SYS,
	TIMER_100MS_VALVE,
	TIMER_100MS_POWERDOWN,
    TIMER_100MS_IR_TX_38K_END,
	TIMER_100MS_IR_OPEN,
	TIMER_100MS_IR_KEY_OPEN_ONECE,
	TIMER_100MS_IR_KEY_OPEN_ALL,
	TIMER_100MS_UART0_SEND,
	TIMER_100MS_UART1_SEND,
	TIMER_100MS_UART2_SEND,
	TIMER_100MS_UART3_SEND,
	TIMER_100MS_LPUART0_SEND,
	TIMER_100MS_LPUART1_SEND,
	TIMER_100MS_UART0_BUSY,
	TIMER_100MS_UART1_BUSY,
	TIMER_100MS_UART2_BUSY,
	TIMER_100MS_UART3_BUSY,
	TIMER_100MS_LPUART0_BUSY,
	TIMER_100MS_LPUART1_BUSY,
    TIMER_100MS_TOUCH_RESET,
	TIMER_100MS_METER_RTCCLK_MONITOR,
    TIMER_100MS_LONGLONGKEYSTART,
    TIMER_100MS_LONGLONGKEYEND,
	//NB上报相关
	TIMER_100MS_WAIT_SLEEP_GPRS,
	TIMER_100MS_ERROR_UPLOAD,
	TIMER_100MS_NBIOT_20,
	TIMER_100MS_BLE,
	TIMER_100MS_BLE_CON_REV30S,
	TIMER_100MS_BLE_CON_ALL90S,
	TIMER_100MS_BLE_SCAN,
	TIMER_100MS_BLE_CON,
    
    TIMER_100MS_MAX_SUM
}TIMER_100MS_ENUM;

//#define TIMER_100MS_MAX_SUM 10
 
/* Function Declare------------------------------------------------------------*/	 

/*************************************************
Function:void Timer1usInit(void)
Description: 1us定时器初始化函数
Input:  None
Return: None
Others:	上电初始化接口
*************************************************/
void Timer1usInit(void);

/*************************************************
Function:void Timer1usWakeSleep(void)
Description: 1us定时器MCU休眠唤醒后处理函数
Input:  None
Return: None
Others:	MCU休眠唤醒后处理函数
*************************************************/
void Timer1usWakeSleep(void);

/*************************************************
Function:void Timer1usPreSleep(void)
Description: 1us定时器MCU进入休眠前处理函数
Input:  None
Return: None
Others:	MCU进入休眠前处理函数
*************************************************/
void Timer1usPreSleep(void);

/*************************************************
Function:u8 TimerIfSleep(void)
Description: 1us定时器是否允许MCU进入休眠的判断函数
Input:  TRUE表示当前模块已空闲；FALSE表示当前模块非空闲，不能进行休眠
Return: None
Others:	是否允许MCU进入休眠的判断函数
*************************************************/
u8 TimerIfSleep(void);

/*************************************************
Function:u32 Timer1usGetTick(void)
Description: 获取1us定时器当前计数值的函数
Input:  None
Return: 当前1us定时器的计数值
Others:	None
*************************************************/
u32 Timer1usGetTick(void);

/*************************************************
Function:void Set5msTimer(u8 timer_5ms_num,u16 timespan_5ms)
Description: 对timer_5ms_num指定的定时资源进行5ms当量的定时
Input:  timer_5ms_num表示定时资源的编码序号；timespan_5ms表示需要定时的时间
Return: None
Others:	定时时间=timespan_5ms*5
*************************************************/
void Set5msTimer(u8 timer_5ms_num,u16 timespan_5ms);

/*************************************************
Function:u8 Check5msTimer(u8 timer_5ms_num)
Description: 查询5ms当量的定时时间是否完成
Input:  None
Return: 0表示已定时完成，1表示定时未完成
Others:	None
*************************************************/
u8 Check5msTimer(u8 timer_5ms_num);

/*************************************************
Function:void Set100msTimer(u8 timer_100ms_num,u16 timespan_100ms)
Description: 对timer_100ms_num指定的定时资源进行100ms当量的定时
Input:  timer_100ms_num表示定时资源的编码序号；timespan_100ms表示需要定时的时间
Return: None
Others:	定时时间=timespan_100ms*100
*************************************************/
void Set100msTimer(u8 timer_100ms_num,u16 timespan_100ms);

/*************************************************
Function:u8 Check100msTimer(u8 timer_5ms_num)
Description: 查询100ms当量的定时时间是否完成
Input:  None
Return: 0表示已定时完成，1表示定时未完成
Others:	None
*************************************************/
u8 Check100msTimer(u8 timer_100ms_num);

/*************************************************
Function:u32 Get100msTimer(u8 timer_100ms_num)
Description: 查询100ms当量的定时剩余时间
Input:  timer_100ms_num表示待插叙的定时资源序号
Return: 100ms当量的定时剩余时间
Others:	None
*************************************************/
u32 Get100msTimer(u8 timer_100ms_num);

#ifdef __cplusplus
}
#endif


#endif
//#endif /* __U_TIMER_DRIVER_H */

