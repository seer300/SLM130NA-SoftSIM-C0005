/*****************************************************************************************************************************	 
 * user_debug.h
 ****************************************************************************************************************************/

#ifndef USER_DEBUG_H__
#define USER_DEBUG_H__

#include "xinyi2100.h"
#include "user_config.h"



#define PRINT_BUFF_SIZE         (256)
#define DEBUG_PERP_INTTERRRUPT //中断外设

#ifdef DEBUG_PRINTF_EN
void jk_printf_uart_Init();
void jk_printf(const char *fmt, ...);
#else
#define jk_printf_uart_Init()
#define jk_printf(...)
#endif


#endif

