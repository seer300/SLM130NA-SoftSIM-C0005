#ifndef DIAG_PRINT_H
#define DIAG_PRINT_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "diag_options.h"
#include "diag_item_types.h"


/*静态明文打印log输出，效率最低，仅限用户或特殊log打印输出*/
diag_print_state_t diag_platform_static_log(XY_TRANSPORT_e port,XY_SRC_E src_id, XY_LOG_LEV lev, const char*fmt, ...) DIGA_ATTR_PRINTF(4, 5);

/*明文打印的精简ID输出方式，常用于PS和平台字符串输出*/
diag_print_state_t diag_platform_dynamic_log(int dyn_id, XY_SRC_E src_id, XY_LOG_LEV lev, const char *fmt, ...) DIGA_ATTR_PRINTF(4, 5);

// 物理层指定参数个数的动态log打印，注意变参只能传递32位的实际数据，不能传输地址和超过32位
diag_print_state_t diag_phy_fixed_arguments_log(int dyn_id, XY_SRC_E src_id, XY_LOG_LEV lev, uint32_t arg_num, const char *fmt, ...) DIGA_ATTR_PRINTF(5, 6);

/*物理层结构体内容的打印输出*/
diag_print_state_t diag_phy_primitive_log(void *pMsg, uint16_t usLen, uint16_t usHSN, uint16_t usSFN, uint8_t ucSubf, uint8_t ucSN, uint8_t ucDataType);

/*PS模块内输出结构体内容使用，不能用于模块间消息*/
diag_print_state_t diag_ps_signalling_log(uint8_t msg_id, char *msg, int msg_len, diag_ps_type_t signalling_type);

/*PS与phy等模块间标准消息传递的输出log*/
diag_print_state_t diag_ps_primitive_log(char *msg);

// 协议栈的 wireshark log 打印
diag_print_state_t diag_ps_wireshark_log(char *data, int data_len, uint8_t type);


#ifdef __cplusplus
 }
#endif

#endif  /* DIAG_PRINT_H */
