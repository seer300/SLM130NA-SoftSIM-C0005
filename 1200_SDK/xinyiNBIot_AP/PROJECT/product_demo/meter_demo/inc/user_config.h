#pragma once

#include "basic_config.h"

//事件宏重定义
#define EVENT_XY_TIMER						EVENT_USER_DEFINE1
#define EVENT_LPTIMER						EVENT_USER_DEFINE3
#define EVENT_TIMER2						EVENT_USER_DEFINE4


//定时器宏重定义
#define XY_TIMER_TEST						    TIMER_LP_USER1
#define BOOT_CP_TIMER TIMER_LP_USER2
#define STOP_CP_TIMER TIMER_LP_USER3
#define WRITE_FLASH_TIMER TIMER_LP_USER4

#define FLASH_TIMER_BASE 15
#define FLASH_TIMER_RANGE 5
/*************************************************************************************/
//boot_stop_cp_test.c
#define BSCP_BOOT_WAIT_MS           (30)
#define BSCP_PERIOD                 (1)
#define BSCP_STOP_TIMEOUT_MAX       (30)
#define BSCP_STOP_TIMEOUT_MIN       (1)
/*************************************************************************************/

/*bit0:boot_CP; bit1:boot_CP+stop_CP;bit2:lcd; bit3:e2; bit4:xy_timer; bit5:lptimer; bit6:adc; bit7:timer2, bit8:spi, bit9: boot_CP+stop_CP(中断中执行)...*/
extern uint32_t g_mcu_bitmap;

/*************************************************************************************/
//cp_test.c
extern uint32_t g_boot_min_sec;   /*执行Boot_CP超时定时器随机时长的最小值*/
extern uint32_t g_boot_max_sec;   /*执行Boot_CP超时定时器随机时长的最大值*/
extern uint32_t g_stop_min_sec;   /*执行Stop_CP超时定时器随机时长的最小值*/
extern uint32_t g_stop_max_sec;   /*执行Stop_CP超时定时器随机时长的最大值*/
extern uint32_t g_stop_timeout_ms; /*Stop_CP(uint32_t wait_ms)入参默认值*/
extern uint32_t g_PING_flag;
/*************************************************************************************/

/*************************************************************************************/
// boot_stop_cp_test.c
extern uint32_t g_boot_period_ms;      /*执行Boot_CP的周期，单位毫秒*/
extern uint32_t g_stop_max_ms;         /*执行Stop_CP超时定时器随机时长的最大值，单位毫秒*/
extern uint32_t g_stop_min_ms;         /*执行Stop_CP超时定时器随机时长的最小值，单位毫秒*/
/*************************************************************************************/

extern uint8_t g_timer_mode;//timer_test测试模式
void Lcd_Work(void);
void E2_Work(void);
void Lptimer_Work();
void XY_TIMER_Work();
void TIMER2_Work();
void Uart_Work();
void ADC_test_set(uint32_t val1,uint32_t val2,uint32_t val3);
int timer_test_set(char **prsp_cmd,int val1,int val2,int val3,int val4,int val5);
void SPI_Work(void);
int rc32_cal_test_set(char **prsp_cmd,int val1,int val2,int val3,int val4,int val5);
