#ifndef AT_UART_H
#define AT_UART_H

#ifdef __cplusplus
extern "C" {
#endif


#include "cmsis_os2.h"
#include "uart.h"



#define AT_PRINT_CHAR(ch) ((void)ch)
#define at_uart_change_config(...)
#define at_uart_check_buffer_and_process() (0)
#define set_standby_by_rate()

/*AT口初始化函数，由于AT口初始化是在AP侧，通用版本此函数为空，BC95版本会重配校验位*/
void at_uart_init();

extern osMutexId_t g_at_uart_mux;

/**
 * @brief 等待AT_UART串口的FIFO数据发送完成
 *
 * @note 由于UART只能等待FIFO为空，不能等待移位寄存器中的数据也发送完成，所以需要再函数的最后加一段软件延时
 *
 * @attention 只能在关键性流程里面调用，如重启和睡眠
 *
 */
void at_uart_wait_send_done(void);


/*对于长URC，为了避免AP核缓存内存不足，直接在CP核输出到uart口*/
int write_to_at_uart(char* buf, int size);


/*对于长URC，为了避免AP核缓存内存不足，直接在CP核输出到uart口*/
bool at_send_to_uart(void* at_ctx, void *buf, int size);

/*仅供睡眠接口内部URC上报使用*/
int write_to_uart_for_Dslp(char *buf, int size);


#ifdef __cplusplus
}
#endif

#endif  /* AT_UART_H */
