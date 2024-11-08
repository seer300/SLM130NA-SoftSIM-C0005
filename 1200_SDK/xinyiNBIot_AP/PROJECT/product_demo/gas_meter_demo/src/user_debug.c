/*****************************************************************************************************************************	 
 * user_debug.c
 ****************************************************************************************************************************/

#include "user_debug.h"
#include "type_adapt.h"

//=====================================================================================
//=====================================JK_PRINTF========================================
//=====================================================================================
#ifdef DEBUG_PRINTF_EN

//CSP1为jk_printf
//CSP3为xy_printf,SDK使用
#include "system.h"
#include <stdarg.h>
#include <string.h>
#include "xy_system.h"
#include "hal_csp.h"
#include "hal_gpio.h"
#include "hal_def.h"
#include "sys_ipc.h"
#include "xy_cp.h"


HAL_CSP_HandleTypeDef jk_csp_log_handle = {0};

/* OPENCPU产品，目前AP核与CP核复用CSP3作为log打印输出。单AP核直接输出，6.5M下最高能到380400。当启动CP核后，经核间消息通知CP核进行log输出 */
__TYPE_IRQ_FUNC void jk_printf_uart_Init()
{
	HAL_GPIO_InitTypeDef uart_debug_gpio_init;

	uart_debug_gpio_init.Pin      = GPIO_PAD_NUM_6;
	uart_debug_gpio_init.PinRemap = GPIO_CSP1_TXD;
	uart_debug_gpio_init.Mode     = GPIO_MODE_HW_PER;
	HAL_GPIO_Init(&uart_debug_gpio_init);

	HAL_CSP_DeInit(&jk_csp_log_handle);
 
	//初始化CSP1为UART
	jk_csp_log_handle.Instance = HAL_CSP1;
	jk_csp_log_handle.CSP_UART_Init.BaudRate = HAL_CSP_UART_BAUD_115200;  //HAL_CSP_UART_BAUDRATE_115200
	jk_csp_log_handle.CSP_UART_Init.WordLength = HAL_CSP_UART_WORDLENGTH_8;
	jk_csp_log_handle.CSP_UART_Init.Parity = HAL_CSP_UART_PARITY_NONE;
	jk_csp_log_handle.CSP_UART_Init.StopBits = HAL_CSP_UART_STOPBITS_1;
	HAL_CSP_UART_Init(&jk_csp_log_handle);
}

__TYPE_IRQ_FUNC void jk_printf(const char *fmt, ...)
{
    char str[PRINT_BUFF_SIZE] = {0};
    uint16_t size = 0;
    va_list args;

    // 将数据格式化至str中
    va_start(args, fmt);
    vsprintf(str, fmt, args);
    va_end(args);

    // 算出字符串最大长度
    while(str[size] != '\0')
    {
        if (size < (PRINT_BUFF_SIZE-1))
            size++;
        else
            break;
    }

    // 调用接口打印数据
    HAL_CSP_Transmit(&jk_csp_log_handle, (uint8_t *)str, (uint16_t)size, 1000 * 60);
}

#endif
//=====================================JK_PRINTF_END===================================

