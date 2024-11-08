/**
 * @file at_uart_ringbuf.c
 * @author LinJiajun
 * @brief 为AT_UART接收数据实现的环形队列
 * @version 0.1
 * @date 2022-07-28
 * 
 * @attention 由于此环形队列仅为AT UART接收服务，写接口仅在UART中断服务程序中调用，
 *          读接口仅在UART接收线程中调用，所以这里没有做互斥保护
 * 
 * @copyright Copyright (c) 2022
 * 
 */


#include "at_uart_ringbuf.h"
#include "sys_debug.h"
#include <string.h>


typedef struct
{
    char *    buffer;
    uint16_t  write;
    uint16_t  read;
    uint16_t  size;
    uint16_t  overflow_cnt;
} at_ringbuf_t;


static at_ringbuf_t  g_ringbuf = {0};


void at_uart_ringbuf_init(char * buf, uint32_t size)
{
    if ((buf != NULL) && (size != 0))
    {
        g_ringbuf.buffer = (char *) buf;
        g_ringbuf.size   = (uint16_t) size;
    }
    else
    {
        Sys_Assert(0);
    }
}


uint32_t at_uart_ringbuf_get_data_size(void)
{
    uint16_t  write_cnt = g_ringbuf.write;
    uint16_t  read_cnt  = g_ringbuf.read;
    uint32_t  data_size;

    if (write_cnt >= read_cnt)
    {
        data_size = write_cnt - read_cnt;
    }
    else
    {
        data_size = g_ringbuf.size + write_cnt - read_cnt;
    }

    return data_size;
}


uint32_t at_uart_ringbuf_write(char ch)
{
    uint16_t  write_cnt = g_ringbuf.write;
    uint16_t  read_cnt  = g_ringbuf.read;
    uint16_t  buf_size  = g_ringbuf.size;
    uint16_t  surplus_size;
    char *    buf;

    // 计算环形队列中的剩余空间
    if (write_cnt >= read_cnt)
    {
        surplus_size = buf_size + read_cnt - write_cnt;
    }
    else
    {
        surplus_size = read_cnt - write_cnt;
    }

    // 必须预留一个字节的空间，否则环形队列逻辑会出错
    if (surplus_size > 1)
    {
        buf = g_ringbuf.buffer;
        buf[write_cnt] = ch;

        // 更新写指针，这里只做判断，不做取余操作，增加代码执行效率
        write_cnt += 1;
        if (write_cnt == buf_size) write_cnt = 0;

        g_ringbuf.write = write_cnt;

        return 1;
    }
    else
    {
        g_ringbuf.overflow_cnt ++;

        return 0;
    }
}


uint32_t at_uart_ringbuf_read(char *buf, uint32_t len)
{
    uint16_t  write_cnt = g_ringbuf.write;
    uint16_t  read_cnt  = g_ringbuf.read;
    uint16_t  buf_size  = g_ringbuf.size;
    uint32_t  data_size;
    uint32_t  tmp_size;

    // 获取当前环形队列中有效数据的长度
    if (write_cnt >= read_cnt)
    {
        data_size = write_cnt - read_cnt;
    }
    else
    {
        data_size = buf_size + write_cnt - read_cnt;
    }

    // 取有效长度和传入buffer长度的较小值，读取此长度的数据
    data_size = (data_size > len) ? len : data_size;

    // 如果要读取的数据没有超过环形队列的末尾，则无需进行两次memcpy
    if ((read_cnt + data_size) <= buf_size)
    {
        memcpy(buf, (g_ringbuf.buffer + read_cnt), data_size);
        // 更新读指针，这里只做判断，不做取余操作，增加代码执行效率
        read_cnt += data_size;
        if (read_cnt == buf_size) read_cnt = 0;
    }
    else
    {
        tmp_size = buf_size - read_cnt;
        memcpy(buf, (g_ringbuf.buffer + read_cnt), tmp_size);
        // 如果从头开始读取数据，可以直接更新读指针，复用变量
        read_cnt = data_size - tmp_size;
        memcpy((buf + tmp_size), g_ringbuf.buffer, read_cnt);
    }

    g_ringbuf.read = read_cnt;

    return data_size;
}
