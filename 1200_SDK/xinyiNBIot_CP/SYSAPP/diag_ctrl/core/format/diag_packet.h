#ifndef DIAG_PACKET_H
#define DIAG_PACKET_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "diag_item_struct.h"
#include "diag_list.h"
#include "diag_mem.h"

// 获取 sequence number，函数内部会 sequence number 自增
uint32_t diag_packet_get_sequence_num(void);

// 设置 sequence number
void diag_packet_set_sequence_num(diag_list_t *diag_iter);

//发生数据长度错误导致丢log后调用，函数内部计数
void diag_packet_add_length_err_num(void);

//发生格式化失败导致丢log后调用，函数内部计数
void diag_packet_add_format_fail_num(void);

//丢log的debug log打印发送成功后调用该接口，更新各统计数值
void diag_packet_refresh_debug_info(diag_debug_info_t *record_info);

// 填充 ItemHeader_t 的头部
void diag_packet_fill_header(ItemHeader_t *pItemHeader, uint32_t buf_sz, uint32_t type_id, uint32_t item_id);

// 填充 ItemHeader_t 的 crc 校验和尾部
void diag_packet_fill_crc_and_tail(ItemHeader_t *data);

// 执行dump流程时，填充 ItemHeader_t 的头部
void diag_packet_fill_dump_header(ItemHeader_t *pItemHeader, uint32_t buf_sz, uint32_t type_id, uint32_t item_id);

// 执行dump流程时，填充 ItemHeader_t 的 crc 校验和尾部
void diag_packet_fill_dump_crc_and_tail(ItemHeader_t *data);

// 检测接收的数据包是否正确，返回0代表正确，返回非0代表错误
int diag_packet_analysis_head(ItemHeader_t *pItemHeader);


#ifdef __cplusplus
 }
#endif

#endif  /* DIAG_PACKET_H */
