#ifndef DIAG_CMD_SEND_H
#define DIAG_CMD_SEND_H

#include "diag_options.h"
#include "diag_item_types.h"


// 内存导出接口，接口内部会直接发送数据，没有内存申请和普通log的链表操作
void diag_dump_mem_info(char *mem_file, uint32_t mem_addr, size_t mem_size) ;

// 命令发送接口，系统正常运行时调用，和普通log类似，需要申请内存和挂链表
diag_print_state_t diag_cmd_send_normal(uint32_t type_id, uint32_t cmd_id, uint8_t src_id, char* data, int msg_len);

// 命令发送接口，一般于系统异常后调用，接口内部会直接发送数据，没有内存申请和普通log的链表操作
diag_print_state_t diag_cmd_send_directly(uint32_t type_id, uint32_t cmd_id, uint8_t src_id, char* data, int msg_len) ;

// 静态log直接打印接口，一般于系统异常后调用，接口内部会直接发送数据，没有内存申请和普通log的链表操作
diag_print_state_t diag_static_log_directly(XY_SRC_E src_id, XY_LOG_LEV lev, const char *fmt, ...) ;

// 产线模式回复内容，实际是静态log
diag_print_state_t diag_cmd_production_response(char *str);


#endif  /* DIAG_CMD_SEND_H */
