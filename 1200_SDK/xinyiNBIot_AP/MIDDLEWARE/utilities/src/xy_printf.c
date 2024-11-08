#include "system.h"
#include <stdarg.h>
#include <string.h>
#include "xy_printf.h"
#include "xy_system.h"
#include "hal_csp.h"
#include "sys_ipc.h"
#include "sys_proc.h"
#include "xy_cp.h"
#include "zero_copy.h"
#include "at_uart.h"


#if (XY_LOG == 1)

/*CP核最高支持921600，AP核最高支持230400*/
HAL_CSP_HandleTypeDef csp_log_handle = {0};

void Log_Output_Control(Print_Output_TypeDef print_way)
{
	if(print_way == LOG_PRINT_ONLY_AP)
	{
		HWREGB(BAK_MEM_AP_LOG) = 7;
	}
	else
	{
		HWREGB(BAK_MEM_AP_LOG) = READ_FAC_NV(uint8_t,open_log);
	}
}

/*OPENC单AP核工作，在AP核初始化串口；启动CP核后在CP核初始化串口。若设置NV参数open_log为7，则无CP核LOG输出，通过xy_printf进行AP核的明文LOG输出*/
void xy_print_uart_Init(uint32_t bound_rate)
{
	HAL_GPIO_InitTypeDef uart_debug_gpio_init;	
	uart_debug_gpio_init.Pin      = PRINT_UART_TX_PIN;
	uart_debug_gpio_init.PinRemap = GPIO_CSP3_TXD;
	uart_debug_gpio_init.Mode     = GPIO_MODE_HW_PER;
	HAL_GPIO_Init(&uart_debug_gpio_init);

	uart_debug_gpio_init.Pin      = PRINT_UART_RX_PIN;
	uart_debug_gpio_init.PinRemap = GPIO_CSP3_RXD;
    uart_debug_gpio_init.Mode     = GPIO_MODE_HW_PER;
	HAL_GPIO_Init(&uart_debug_gpio_init);

	HAL_CSP_DeInit(&csp_log_handle);

	//初始化CSP3为UART
	csp_log_handle.Instance = HAL_CSP3;
	csp_log_handle.CSP_UART_Init.BaudRate = bound_rate;  
	csp_log_handle.CSP_UART_Init.WordLength = HAL_CSP_UART_WORDLENGTH_8;
	csp_log_handle.CSP_UART_Init.Parity = HAL_CSP_UART_PARITY_NONE;
	csp_log_handle.CSP_UART_Init.StopBits = HAL_CSP_UART_STOPBITS_1;
	HAL_CSP_UART_Init(&csp_log_handle);
}

/*连发三次给logview，通知切换波特率*/
void xy_logview_rate_set(uint32_t bound_rate)
{

	/*4字节魔术数字+2字节长度+2字节seq+4字节ID+4字节时间戳+静态头8字节(src+level+parasize+pad+msgsize)。其中0x0D为AP_CORE_LOG*/
	uint8_t log_head[21] = {0x5a,0xa5,0xfe,0xef, 0x0E,0x00, 0xfd,0x00, 0xe7,0x00,0x60,0x00, 0x00,0x00,0x00,0x00, 0x80,0x25,0x00,0x00, 0xFA};

	*((uint32_t *)(log_head+16)) = bound_rate;
	HAL_CSP_Transmit(&csp_log_handle, (uint8_t *)log_head,21,1000);
	HAL_Delay(2);
	HAL_CSP_Transmit(&csp_log_handle, (uint8_t *)log_head,21,1000);
	HAL_Delay(2);
	HAL_CSP_Transmit(&csp_log_handle, (uint8_t *)log_head,21,1000);
	HAL_Delay(2);
}

/*CP核未启动或已进深睡，走AP核输出log*/
uint8_t g_logview_switch_flag = 0;//该标志位保障xy_logview_switch_cp和xy_logview_switch_ap配对生效
void xy_logview_switch_ap()
{
	/*仅AP输出明文log,CP核不输出任何LOG*/
	if(HWREGB(BAK_MEM_AP_LOG) == 7)
	{
		return;
	}

	if(g_logview_switch_flag == 0)
		return;

	/*此时LOG驱动在CP核，切换握手先在高波特率上进行*/
	xy_print_uart_Init(XY_LOGVIEW_CP_RATE);
	xy_logview_rate_set(XY_LOGVIEW_AP_RATE);
	xy_print_uart_Init(XY_LOGVIEW_AP_RATE);
	g_logview_switch_flag = 0;
}

/*boot_CP时，切换到高波特率上，CP核会重新初始化LOG驱动*/
void xy_logview_switch_cp()
{
	/*仅AP输出明文log,CP核不输出任何LOG*/
	if(HWREGB(BAK_MEM_AP_LOG) == 7)
	{
		return;
	}

	xy_logview_rate_set(XY_LOGVIEW_CP_RATE);
	g_logview_switch_flag = 1;
}

void xy_printf_ap(char *pBuffer, int size)
{
		/*4字节魔术数字+2字节长度+2字节seq+4字节ID+4字节时间戳+静态头8字节(src+level+parasize+pad+msgsize)。其中0x0D为AP_CORE_LOG*/
		uint8_t log_head[24] = {0X5A,0XA5,0XFE,0XEF,0,0,0,1,0X01,0,0,0,0,0,0,0,0x0D,0x02,0,0x01,0,0,0,0};
		uint8_t log_tail = 0XFA;
		//(void)fd;

		*((uint16_t *)(log_head+4)) = size+18;
		*((uint16_t *)(log_head+20)) = size;
		HAL_CSP_Transmit(&csp_log_handle, (uint8_t *)log_head,24,1000);
		HAL_CSP_Transmit(&csp_log_handle, (uint8_t *)pBuffer, (uint16_t)size,1000);
		HAL_CSP_Transmit(&csp_log_handle, (uint8_t *)&log_tail,1,100);
}

#if defined(__CC_ARM)

// 使用MDK编译器时，GCC库的printf实现
// 加入以下代码,支持printf函数,而不需要选择use MicroLIB
#pragma import(__use_no_semihosting)
//标准库需要的支持函数
struct __FILE
{
	int handle;
	/* Whatever you require here. If the only file you are using is */
	/* standard output using printf() for debugging, no file handling */
	/* is required. */
};
/* FILE is typedef in stdio.h. */
FILE __stdout;
// 定义_sys_exit()以避免使用半主机模式
void _sys_exit(int x)
{
	(void)x;
}
// printf implementation of the MDK library
int fputc(int ch, FILE *f)
{
	(void)f;

	HAL_CSP_Transmit(&csp_log_handle, (uint8_t *)&ch, 1);
	return ch;
}

#elif defined(__GNUC__)


/**
 * @brief	重写C库函数，用于printf重定向到CSP3打印，用户也可以自行更改
 */
__RAM_FUNC int _write(int fd, char *pBuffer, int size)
{
	/*仅AP输出明文log,CP核不输出任何LOG*/
	if(HWREGB(BAK_MEM_AP_LOG) == 7)
	{
		HAL_CSP_Transmit(&csp_log_handle, (uint8_t *)pBuffer, (uint16_t)size,1000);
		return size;
	}
		
	/*通过核间消息由CP核输出log*/
	if(CP_Is_Alive() == true)
	{
		if(CP_IS_DEEPSLEEP() == true)
		{
			xy_logview_switch_ap();
			xy_printf_ap(pBuffer, size);
		}

		uint8_t *log;
		IPC_Message pMsg = {0};
		

		log = xy_malloc(size+1);
		memcpy(log,pBuffer,size);
		*(log+size) = '\0';

		pMsg.len = 4;
		pMsg.buf = &log;

			pMsg.id = ICM_AP_LOG;

		if (IPC_WriteMessage(&pMsg) < 0)
		{
			xy_free(log);
			return XY_ERR_IPC_FAIL;
		}
		else
		{
			Insert_ZeroCopy_Buf(log);
		}
	}
	/*单AP核启动时，复用CP核的log通道，直接输出到logview*/
	else
	{
		xy_printf_ap(pBuffer, size);
	}
	return size;
}

#endif

uint8_t gLogDynamicOpen = 1;

/*1为开启AP核log输出，0表示关闭*/
__RAM_FUNC void xy_open_log(uint8_t open)
{
	gLogDynamicOpen = open;
}

__RAM_FUNC void xy_printf(const char *fmt, ...)
{
	if(gLogDynamicOpen != 1)
		return;
	
	DisableInterrupt();
	va_list args;
	va_start(args, fmt);  //以传入的形参为内存基准
	vprintf(fmt, args);
	fflush(stdout);
	va_end(args);   //args指针置0
	EnableInterrupt();
}
#else
__RAM_FUNC void xy_printf(const char *fmt, ...)
{
	(void) fmt;
}
#endif