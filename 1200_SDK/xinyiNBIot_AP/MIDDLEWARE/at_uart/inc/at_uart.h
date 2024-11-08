#pragma once

#include <stdint.h>
#include <string.h>
#include "hal_lpuart.h"


#if (AT_LPUART == 1)

extern uint8_t g_at_standby_timeout;
extern HAL_LPUART_HandleTypeDef g_at_lpuart;
extern uint8_t g_wakup_at;

#define  GET_BAUDRATE()    (g_at_lpuart.Init.BaudRate)

/**
 * @brief 初始化AT通道
 * @note  1. AT通道初始化调用HAL_LPUART_Init接口实现，LPUART的电源配置也在这个里面实现。
 *        2. AT通道的接收调用HAL_LPUART_Receive_IT接口实现，超时入参非HAL_MAX_DELAY，则表明即使用接收阈值中断又使用接收超时中断。
 *        3. AT通道的发送调用底层的driverlib接口实现。
 */
void at_uart_init(void);

/**
 * @brief 通过AT通道发送ASCII码的AT命令，需要填写数据长度
 * @param buf  待发送数据的起始地址
 * @param size 待发送数据长度
 * @note  (1) 中断服务函数可用、非中断函数也可用，但不保证AT通道输出数据的实时性
 *        (2) 当GNSS_EN开启且为码流模式时，该调用不会使AT通道输出数据
 */
void at_uart_write_data(char *buf, int size);

/**
 * @brief AT通道直接写数据至LPUART的TXFIFO，优点是简单高效、带核间互斥锁，通常用于导DUMP场景，也可被封装为其他AT发送接口
 * @param buf  待发送数据的起始地址
 * @param size 待发送数据长度
 * @note  用于AT通道必须立即输出数据的场景，如导DUMP
 */
void at_uart_write_fifo(char *buf, int size);

/**
 * @brief 向AT_UART串口写入ASCII码的AT命令，与at_uart_write_data差异仅为没有长度入参
 */
#define Send_AT_to_Ext(str)         at_uart_write_data((char *)(str), (int)strlen(str))

/**
 * @brief 等待LPUART数据发送完成，若数据一直发送不完则2秒后自动退出，
 *        若2s内发送完成了，则根据波特率增加1个字符的延时以保证数据全部发送至串口总线上。
 */
void at_uart_wait_send_done(void);

/**
 * @brief 配置LPUART的波特率和奇偶校验位
 * @param baudrate 1200~921600
 * @param parity   0：无校验位，1：偶校验，2：奇校验
 */
void at_uart_config(uint32_t baudrate, uint32_t parity);

/**
 * @brief 解析接收到的AT命令解析和AT命令回复、上报
 * @note  OPENCPU产品不使用，进而放在flash上不影响功耗
 */
void at_uart_recv_and_process(void);

/**
 * @brief 若波特率高于9600，芯翼后台会自动关闭STANDBY等级的低功耗，以防止数据脏乱。
 *        对于AT唤醒DEEPSLEEP场景，高于9600波特率会存在丢数据，进而建议客户通过发送“AT\r\n”来唤醒，然后过100ms后再发送“AT\r\n”，直到收到“OK\r\n”。
 * @attention LPUART只能在小于等于9600波特率情况下，才能保证唤醒后不丢AT命令；建议客户不要选用高于9600波特率。
 */
void set_standby_by_rate(uint32_t rate);


/* 波特率超过9600，AT触发唤醒或者运行过程中收到AT命令，延迟若干秒开启STANDBY */
void at_uart_standby_ctl(void);


/*波特率大于9600时，AT唤醒深睡，首条命令丢失不报错，第二条命令正常执行*/
int drop_dirty_at_from_sleep(void);

/**
 * @brief  检测当前是否有待收发处理的AT命令，若未处理则退出睡眠
 * @return false ：AT命令收发处理未结束，AT通道处于非IDLE态
 *         true  ：AT命令收发处理已结束，AT通道处于IDEL态
 */
bool at_uart_idle();

#else

#define  Send_AT_to_Ext(str)
#define  at_uart_recv_and_process()
#define  at_uart_config(baudrate, parity)
#define  set_standby_by_rate(rate)
#endif


