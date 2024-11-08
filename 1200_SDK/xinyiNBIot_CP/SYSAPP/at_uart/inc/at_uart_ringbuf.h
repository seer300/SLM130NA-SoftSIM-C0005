#ifndef AT_UART_RINGBUF_H
#define AT_UART_RINGBUF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


/**
 * @brief 初始化环形队列
 * 
 * @param buf 环形队列的起始地址
 * @param size 环形队列的大小
 */
void at_uart_ringbuf_init(char * buf, uint32_t size);

/**
 * @brief 获取当前环形队列中有效数据的长度
 * 
 * @return 环形队列中有效数据的长度
 */
uint32_t at_uart_ringbuf_get_data_size(void);

/**
 * @brief 向环形队列中写入一个字节的数据
 * 
 * @param ch 要写入的字节的数据
 * @note 由于此环形队列仅为UART接收使用，所以写入接口仅提供单个字节的写入即可
 * @return 写入的数据的长度，由于只写入一个字节，只可能返回0和1，返回0代表写入失败
 */
uint32_t at_uart_ringbuf_write(char ch);

/**
 * @brief 从环形队列中读取指定长度的数据
 * 
 * @param buf 保存读取到的数据的缓冲区
 * @param len 缓冲区的长度
 * @return 读取到的数据的长度，根据当前环形队列有效数据的长度，读取到的数据可能小于缓冲区的长度
 */
uint32_t at_uart_ringbuf_read(char *buf, uint32_t len);


#ifdef __cplusplus
}
#endif

#endif  /* AT_UART_RINGBUF_H */
