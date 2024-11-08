/** 
* @file     user_hook.c
* @brief    用户重新实现的弱函数，根据自身需求实现需要的弱函数即可
*/

#include "xy_utils_hook.h"
#include "xy_at_api.h"
#include "xy_memmap.h"
#include "xy_system.h"
#include "xy_utils.h"
#include "xy_rtc_api.h"
#include "xy_at_api.h"
#include "at_uart.h"
#include "xy_flash.h"
#include "oss_nv.h"




void *g_user_nv = NULL;

void malloc_user_mem()
{
}



application_init(malloc_user_mem);



