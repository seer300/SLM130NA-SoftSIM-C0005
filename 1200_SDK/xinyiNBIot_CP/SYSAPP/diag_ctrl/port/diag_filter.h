#ifndef DIAG_FILTER_H
#define DIAG_FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "diag_item_struct.h"
#include "diag_item_types.h"


// log过滤功能的初始化
void diag_filter_init(void);

/*用于log的一级过滤，src_id为-1表示特殊log，不参与规则匹配，一律输出。平台重点维护*/
diag_send_state_t diag_filter_get_send_enable_state(XY_SRC_E src_id);
// 设置普通log的位图
void diag_filter_set_log_bitmap(filterInfo * bitmap_info);
// 刷新心跳，收到心跳包时调用该函数
void diag_filter_refresh_heart_beat(void);
// 刷新心跳包标志，判断唤醒后是否需要刷新心跳，只在睡眠前调用
void diag_filter_refresh_heart_flag(void);
// 如果有必要，则刷新心跳包，只在睡眠唤醒后调用
void diag_filter_refresh_heart_if_needed(void);
// 普通log过滤
diag_send_state_t diag_filter_normal_log(XY_SRC_E src_id, XY_LOG_LEV lev);
// 信令log过滤
diag_send_state_t diag_filter_signalling_log(diag_ps_type_t signalling_type);
// 原语log过滤
diag_send_state_t diag_filter_primitive_log(uint32_t class_id, uint32_t msg_id);


#ifdef __cplusplus
}
#endif

#endif  /* DIAG_FILTER_H */
