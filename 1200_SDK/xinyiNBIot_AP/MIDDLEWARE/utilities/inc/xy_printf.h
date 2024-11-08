/*****************************************************************************************************************************	 
 * @brief  打印输出接口，使用时需要开启XY_LOG宏。AP和CP核共用一条log通道，目前为CSP3实现的uart口，使用GPIO18和19引脚。
 * 单AP核启动时，AP直接走log口吐给logview；当双核启动后，AP核通过核间消息通知CP核吐log到logview。
 * log输出波特率。单AP核最高只能为115200，双核启动最高可为921600。用户可以通过AT+NV=SET,LOGRATE,<val>和logview修改波特率
 ****************************************************************************************************************************/
#pragma once



#include <stdio.h>
#include "hal_gpio.h"
#include "hal_def.h"


/*
* @brief OPENCPU产品，目前AP核与CP核复用CSP3作为log打印输出，用户不得擅自修改LOG通道。若修改，必须两个核同时修改 
* @note  对于部分客户仅需求AP核明文打印，不使用logview工具场景，用户可自行修改对应的PIN脚，并将NV参数open_log设为7即可通过xy_printf进行明文LOG输出
*/
#define PRINT_UART_TX_PIN                         GPIO_PAD_NUM_18  
#define PRINT_UART_RX_PIN                         GPIO_PAD_NUM_19 

#define XY_LOGVIEW_CP_RATE 921600
#define XY_LOGVIEW_AP_RATE 230400                        

#if (XY_LOG == 1)
typedef enum{
    LOG_PRINT_WAY_BASE = 0,
    LOG_PRINT_NORMAL = LOG_PRINT_WAY_BASE, //默认使用方式，波特率会自动在单双核切换时切换，所有log打印均向log工具输出
    LOG_PRINT_ONLY_AP = 1, //AP单核向串口输出方式，同时会将CP核LOG关闭,也不能使用log工具
    LOG_PRINT_WAY_END
} Print_Output_TypeDef;

/* 
* @brief 此接口为LOG打印输出控制接口；入参为0为默认使用方式，波特率会自动在单双核切换时切换，所有log打印均向log工具输出；入参为1为AP单核向串口输出方式，同时会将CP核LOG关闭
* @warning 此接口仅能在AP核完成初始之后调用，不建议客户在业务运行后再调用
*/
void Log_Output_Control(Print_Output_TypeDef print_way);

/* 
* @brief 单AP核启动时，AP直接走log口吐给logview；当双核启动后，AP核通过核间消息通知CP核吐log到logview。
* @warning AP核严禁进行秒级高频率刷屏打印！原因是：调用boot_cp动态加载CP核后，CP核有十几秒时间MIPS被物理层全部占用，造成不能处理AP核发送来的log
* @warning 不得直接使用printf标准C库接口！
*/
void xy_printf(const char *fmt, ...);

/*1为开启，0表示关闭*/
void xy_open_log(uint8_t open);
void xy_print_uart_Init(uint32_t bound_rate);
void xy_logview_switch_ap();
void xy_logview_switch_cp();


#else
#define xy_print_uart_Init(bound_rate)
#define xy_open_log(open)
#define xy_logview_switch_ap()
#define xy_logview_switch_cp()
#define xy_printf_NMEA(...)
#define Log_Output_Control()
/* 
* @brief Release模式下，如果蓝牙的log功能开启，可以通过蓝牙输出AP核log
*/
void xy_printf(const char *fmt, ...);

#endif
