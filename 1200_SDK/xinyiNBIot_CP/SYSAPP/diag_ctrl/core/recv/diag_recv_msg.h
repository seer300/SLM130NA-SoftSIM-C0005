#ifndef DIAG_RECV_MSG_H
#define DIAG_RECV_MSG_H

#include <stdint.h>


// 清空接收buffer，一般在流程开始时调用，在从正行流程就如dump流程时，防止数据干扰
void diag_recv_reset_buffer(void) ;

// 把数据写入buffer，调用该接口后，一般需要调用下面的函数进行处理
void diag_recv_write_data_to_buffer(uint8_t *data, uint32_t length) ;

// 处理buffer中的数据
void diag_recv_check_buffer_and_process_command(void) ;


#endif  /* DIAG_RECV_MSG_H */
