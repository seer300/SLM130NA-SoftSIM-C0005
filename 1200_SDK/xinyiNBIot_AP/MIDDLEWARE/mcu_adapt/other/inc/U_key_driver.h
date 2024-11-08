/**
  ******************************************************************************
  * @file    hc32_key_driver.h
  * @author  (C)2020, Qindao ieslab Co., Ltd
  * @version V1.0
  * @date    2020-7-1
  * @brief   the function of the entity of system processor
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __U_KEY_DRIVER_H
#define __U_KEY_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "type.h"
#include "mcu_adapt.h"

/* MACRO Define---------------------------------------------------------------*/

#define HARDWARE_VERSION      (0x14)


#if (HARDWARE_VERSION==0x14)//方壳表
//磁敏按键IO配置
#define KEY_GPIO_PIN            MCU_GPIO0   //HD霍尔按键

#define KEY_IRQ					PORTC_E_IRQn 
#define KEY_PRIORITY     		(IrqLevel3)
//触摸按键IO配置
#define TCH_KEY_GPIO_PIN        MCU_GPIO1    //HD触摸按键中断

#define TCH_KEY_POWER_GPIO_PIN  MCU_GPIO2    //HD触摸按键电源

#define TCH_KEY_IRQ					PORTC_E_IRQn 
#define TCH_KEY_PRIORITY     		(IrqLevel3)

#elif (HARDWARE_VERSION==0x18)//圆壳表
//磁敏按键IO配置
#define KEY_GPIO_PIN            MCU_GPIO0   //HD霍尔按键

#define KEY_IRQ					PORTC_E_IRQn 
#define KEY_PRIORITY     		(IrqLevel3)

//触摸按键IO配置
#define TCH_KEY_GPIO_PIN        MCU_GPIO1    //HD触摸按键中断

#define TCH_KEY_POWER_GPIO_PIN  MCU_GPIO2    //HD触摸按键电源

#define TCH_KEY_IRQ					PORTC_E_IRQn 
#define TCH_KEY_PRIORITY     		(IrqLevel3)
#endif
//按键按下 (需要根据硬件设计修改低电平是按下还是高电平是按下)
#define  KEY_NO_PRESS           	(u32)1
#define  KEY_PRESS              	(u32)(!KEY_NO_PRESS)

//中断边沿
#define  KEY_RISING_EDGE          0
#define  KEY_FALLING_EDGE         (!KEY_RISING_EDGE)

//按键类型
#define  NONE_KEY       0
#define  MEGNET_KEY     1
#define  TOUCH_KEY      2

//按键事件
#define NO_EVENT          0x00	 //无操作
#define SHORT_PRESS       0x01	 //短按事件
#define LONG_PRESS        0x02	 //长按事件
#define ERR_PRESS         0x04	 //按键异常事件
#define LONGLONG_PRESS    0x08   //长按触发事件

#define SHORT_START       1        //短按起始时间
#define SHORT_END         200      //短按截止时间
#define LONG_INTERVAL     100      //长按MSG间隔时间
#define LONG_END          60       //长按截止时间 60s，测试完成修改为60s


//采用间隔
#define KEY_CHECK_TIME    2   	   //10ms检测一次
//触摸按键每日限制使用次数
#define TCH_PRESS_LIMIT_CNT  30
//状态机状态
#define KEY_M_S0 0              //按键驱动状态机状态号
#define KEY_M_S1 1              //按键驱动状态机状态号
#define KEY_M_S2 2              //按键驱动状态机状态号
#define KEY_M_S3 3              //按键驱动状态机状态号
#define KEY_M_S4 4              //按键驱动状态机状态号
#define KEY_M_S5 5              //按键驱动状态机状态号
#define KEY_M_S6 6              //按键驱动状态机状态号
#define KEY_M_S7 7              //按键驱动状态机状态号
#define KEY_M_S8 8              //按键驱动状态机状态号
#define KEY_M_S9 9              //按键驱动状态机状态号


/* Function Declare------------------------------------------------------------*/
void KeyInit(void);						//按键初始化
u8 KeyIfIdle(void);						//按键是否空闲
void KeyMachineDriver(void);		//按键主状态机
u8 KeyCheckMsg(void);					//检测按键消息
void KeyClearMsg(u8 msg);			//清除按键消息
u8 KeyIfSleep(void);						//按键是否允许休眠
void KeyPreSleep(void);				//按键休眠前接口
void KeyWakeSleep(void);				//按键唤醒接口
u8 TchKey_GetPressLimitState(void);

#ifdef __cplusplus
}
#endif

#endif /* __U_KEY_DRIVER_H */
