
#pragma once
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



/**/
typedef enum
{
	EVENT_XY_BASE = 0,
	EVENT_AT_STR = EVENT_XY_BASE,   /* 暂未使用！指示CP核有AT命令从ICM_AT通道发送过来，仅用于不使用芯翼AT_CTL框架的场景，即虚拟AT通道场景 */
	EVENT_AT_URC,                   /* 仅适用于芯翼提供的AT框架，指示是否有待处理的URC上报，通常为云下行控制类数据或3GPP重要URC */
	EVENT_CLOUD_UPDATE,             /* 指示是否进行云的UPDATA，如果用户无需维持云始终保活，仅需搜索RTC_AP_TIMER_CLOUDUPDATE的配置点，注释掉即可 */
	EVENT_CLOUD_SEND,               /* 指示是否需要上报云端数据,如采集完后立即触发，或者RTC超时后周期性触发 */
    EVENT_BOOT_CP,                  /* 指示是否需要启动CP，通常为CP核的RTC超时触发，如TAU、update等*/
	EVENT_STOP_CP, 			        /* 指示是否需要强行停CP，通常用于压力测试*/
	EVENT_RC32K_CALI,               /* opencpu形态：由用户周期性触发RC校准，通过offset补偿解决老化问题*/
	EVENT_PING_PROCESS,             /* 指示是否进行ping处理*/
	
	EVENT_USER_BASE = 16,
	
	EVENT_MAX = 32,
} BASIC_EVENT_T;
	


void set_event(uint32_t event);

void clear_event(uint32_t event);

bool is_event_set(uint32_t event);

void clear_all_event();


