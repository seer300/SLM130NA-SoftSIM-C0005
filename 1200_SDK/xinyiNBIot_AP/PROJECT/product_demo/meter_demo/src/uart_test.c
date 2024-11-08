#include "mcu_adapt.h"
#include "xy_system.h"
#include "system.h"
#include "user_config.h"
#include "sema.h"
#include "at_uart.h"

#define UART_NUM_USED (4)   //使用的uart个数
#define UART_LEN_MAX (256)  //uart最大收发长度，单位字节

uint8_t uart_rx_buff[UART_LEN_MAX] = {0};
uint8_t uart_tx_buff[UART_LEN_MAX] = {0};

uint16_t uart_rx_cnt = 0;
uint16_t uart_tx_cnt = 0;

volatile uint8_t uart_rx_done = 0;
volatile uint8_t uart_tx_done = 0;

uint16_t uart_tx_len = 0;

uint8_t g_uart_num = 0xFF; //初始化为一个无效值

#if AT_LPUART
extern HAL_LPUART_HandleTypeDef g_at_lpuart;
#endif

__RAM_FUNC void UartRxIrqHandle(uint8_t rx_status)
{
    (void)rx_status;
    //读取数据
    McuUartRead(g_uart_num, &uart_rx_buff[uart_rx_cnt]);

    //结束字符判断
    if(uart_rx_buff[uart_rx_cnt] == '\n')
    {
        uart_rx_done = 1;
    }
    uart_rx_cnt++;
  
#if AT_LPUART
    if(g_uart_num == 0)//避免数据接收后立马进入深睡
        g_at_lpuart.RxXferCount = uart_rx_cnt;
#endif

}

__RAM_FUNC void UartTxIrqHandle(void)
{
    //结束判断
    if(uart_tx_cnt >= uart_tx_len)
    {
        uart_tx_done = 1;
    }
    else
    {
        McuUartWrite(g_uart_num, uart_tx_buff[uart_tx_cnt]);//发送数据
    }
    uart_tx_cnt++;
}

