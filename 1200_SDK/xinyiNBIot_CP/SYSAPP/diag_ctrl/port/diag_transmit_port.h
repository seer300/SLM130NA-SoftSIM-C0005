#ifndef DIAG_TRANSMIT_PORT_H
#define DIAG_TRANSMIT_PORT_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "diag_item_types.h"
#include "diag_filter.h"
#include <stdint.h>


// 底层驱动专用！使用 diag_filter 中的全局log输出条件，来决定是否需要初始化 log
#define diag_port_get_send_enable_state()    diag_filter_get_send_enable_state(-1)


// 发送的初始化，包含硬件的初始化，如果使用线程发送log，还需要初始化线程
void diag_port_send_init(void);

// log的发送接口，上层调用该接口把log发出去
// 需要注意的是，当前的log发送接口的实现，传入的发送地址必须是通过log专用的内存申请接口返回的地址
void diag_port_send_log(void *log_str, uint32_t length);

// 直接发送log接口，数据通过这个接口会被直接发送到log串口，这个传入的地址不是log专用的内存申请接口返回的地址，
void diag_port_send_log_directly(void *log_str, uint32_t length);

// 清除所有未发送的log
void diag_port_clear_unsent_log(void);

// 判断当前是否有log处于发送中
// 1: 发送完成  0: 发送未完成
int diag_port_is_send_complete(void);

// 初始化dump的环境，设置了相关硬件的状态
void diag_port_dump_environment_init(void);

// 等待当前log数据发送完成
void diag_port_wait_send_done(void);

// 断言后的uart接收接口，传入buffer的起始地址和长度，返回实际写入buffer的长度
uint32_t diag_port_recv_after_dump(uint8_t *rcv_buf, uint32_t max_len);


#ifdef __cplusplus
 }
#endif

#endif  /* DIAG_TRANSMIT_PORT_H */
