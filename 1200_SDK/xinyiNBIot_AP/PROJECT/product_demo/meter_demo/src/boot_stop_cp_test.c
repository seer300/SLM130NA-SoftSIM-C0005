#include "at_uart.h"
#include "xy_event.h"
#include "xy_timer.h"
#include "xy_cp.h"
#include "user_config.h"

/*AT+DEMOCFG=BSCP,<g_boot_period_ms>,<g_stop_max_ms>,<g_stop_min_ms>*/
uint32_t g_boot_period_ms = BSCP_PERIOD;     /*执行Boot_CP的周期，单位毫秒*/
uint32_t g_stop_max_ms = BSCP_STOP_TIMEOUT_MAX;       /*执行Stop_CP超时定时器随机时长的最大值，单位毫秒*/
uint32_t g_stop_min_ms = BSCP_STOP_TIMEOUT_MIN;        /*执行Stop_CP超时定时器随机时长的最小值，单位毫秒*/

int g_stop_cp_timer_id = TIMER_LP_USER6;

__RAM_FUNC void Test_Stop_CP_Timeout(void)
{
    Stop_CP2(0, 1);
    Send_AT_to_Ext("stop cp2\r\n"); 
}

__RAM_FUNC void Test_Boot_Stop_CP()
{
    char str[128];
    sprintf(str, "%d,%d,%d\r\n", g_boot_period_ms, g_stop_max_ms, g_stop_min_ms);
    Send_AT_to_Ext(str);
    
    //添加定时器超时执行stop_cp
    Timer_AddEvent(g_stop_cp_timer_id, g_stop_min_ms + (rand() % (g_stop_max_ms-g_stop_min_ms)), Test_Stop_CP_Timeout, 0);

    // bootcp失败，会再一次stopcp
    if (0 == Boot_CP(BSCP_BOOT_WAIT_MS))
    {
        Send_AT_to_Ext("stop cp\r\n"); 
        Stop_CP2(0, 1);
    }

    HAL_Delay(g_boot_period_ms);
}



