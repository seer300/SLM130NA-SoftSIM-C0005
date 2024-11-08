/*
 * @file       
 * @attention  上产线时，务必关注相关NV参数是否设置正确：open_log=0；off_debug=1；
 * @note		该头文件中已经包含了客户可能用到的所有平台头文件，用户不准私自引用非外部客户使用的API接口，否则会存在风险！
*/
 
#pragma once
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xy_lpm.h"
#include "xy_memmap.h"
#include "xy_system.h"
#include "xy_timer.h"
#include "xy_timer.h"
#include "at_CP_api.h"
#include "hal_def.h"
#include "xy_printf.h"
#include "hal_def.h"
#include "hal_csp.h"
#include "hal_gpio.h"
#include "hal_i2c.h"
#include "hal_lptimer.h"
#include "nvic.h"
#include "hal_spi.h"
#include "hal_timer.h"
#include "hal_uart.h"
#include "xy_cp.h"
#include "xy_flash.h"
#include "xy_ftl.h"
#include "ap_watchdog.h"
#include "xy_event.h"

/**************************************************************************************************************************************************************
 * @brief 对于OPENCPU产品形态，不建议CP核执行云的保存和自动UPDATE，以防止私自唤醒对AP核产生干扰，建议出厂NV参数save_cloud、CLOUDALIVE、set_CP_rtc三者皆设为0；close_drx_dsleep设为1
 *************************************************************************************************************************************************************/



/*用户自行定义的事件名序号为：16-32*/
typedef enum
{
	EVENT_USER_DEFINE1 = EVENT_USER_BASE,     /* 用户自行改宏名 */
	EVENT_USER_DEFINE2,                       /* 用户自行改宏名 */
	EVENT_USER_DEFINE3,                       /* 用户自行改宏名 */
	EVENT_USER_DEFINE4,                       /* 用户自行改宏名 */
    EVENT_USER_DEFINE5,                       /* 用户自行改宏名 */
} MAIN_EVENT_T;
	




