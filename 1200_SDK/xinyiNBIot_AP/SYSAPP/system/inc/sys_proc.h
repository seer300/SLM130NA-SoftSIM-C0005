/**
 ******************************************************************************
 * @file      sys_proc.h
 * @ingroup   外设
 * @brief     提供睡眠流程相关API，包括进入睡眠状态，退出睡眠状态的接口。
 *
 ******************************************************************************
 * @attention
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 * http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "xy_system.h"
#include "xy_memmap.h"
#include "utc_watchdog.h"

typedef struct
{
	uint64_t    wall_time_ms;   
	uint64_t    rtc_ms;
	uint64_t    frame_ms;
	uint16_t    cellId;
	uint32_t    freq_num;
	int8_t      g_zone;
}snapshot_t;


typedef enum
{
    CP_DEFAULT_STATUS = 0,  // CP核尚未加载启动
    CP_IN_WORK,             // CP侧已加载且运行至操作系统调度
	CP_IN_STANDBY,          // CP侧已加载且进入Standby
    CP_IN_DEEPSLEEP,        // CP侧已加载且进入Dsleep
    CP_IN_DEEPSLEEP_FASTRECOVERY,        // CP侧已加载且进入Dsleep   
}CP_WORK_MODE;

typedef enum
{
    FORCE_STOP_CP_NONE = 0,  // 默认值
    FORCE_STOP_CP_REQ = 0x5A,             // AP发送的停CP请求
	FORCE_STOP_CP_ACK = 0xA5,          // CP返回AP应答

}STOP_CP_HANDSHAKE;

#define  CP_IS_DEEPSLEEP()   		((HWREGB(BAK_MEM_BOOT_CP_SYNC) == CP_IN_DEEPSLEEP) || (HWREGB(BAK_MEM_BOOT_CP_SYNC) == CP_IN_DEEPSLEEP_FASTRECOVERY))
#define  CP_IS_STANDBY()   			(HWREGB(BAK_MEM_BOOT_CP_SYNC) == CP_IN_STANDBY)
#define  CP_IS_FASTRECOVERY()       (HWREGB(BAK_MEM_BOOT_CP_SYNC) == CP_IN_DEEPSLEEP_FASTRECOVERY)

extern volatile uint32_t wakeup_info;




void Hold_CP_Core();

void Release_CP_Core();

uint32_t dump_into_to_flash(void);

/*****************************************************************************************************************************
* @brief  去使能RTC定时器，CP侧所有的RTC定时器都失效。
****************************************************************************************************************************/
void Disable_UTC_Alarm();


