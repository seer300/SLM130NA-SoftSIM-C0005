#include "diag_options.h"
#include "diag_mem.h"
#include "diag_format.h"
#include "diag_transmit_port.h"
#include "diag_item_struct.h"
#include "diag_item_types.h"
#include "diag_packet.h"
#include "diag_filter.h"
#include "diag_print.h"
#include <stdarg.h>



/*静态明文打印log输出，效率最低，仅限用户或特殊log打印输出*/
__FLASH_FUNC diag_print_state_t diag_platform_static_log(XY_TRANSPORT_e port,XY_SRC_E src_id, XY_LOG_LEV lev, const char *fmt, ...)
{
    StaticLog_t * static_log;
    va_list va;
    int arg_buf_len;
    size_t str_len;
    size_t buffer_size;

    // 当前不允许打log的话，就直接退出
    if(diag_filter_get_send_enable_state(src_id) != DIAG_SEND_ENABLE)
    {
        return DIAG_PRINT_FAILED;
    }

    if(diag_filter_normal_log(src_id, lev) != DIAG_SEND_ENABLE)
    {
        return DIAG_PRINT_FAILED;
    }

    // 第一次做格式化，用于获取需要的buffer长度
    va_start(va, fmt);
    arg_buf_len = diag_format_get_arguments_length(fmt, va);
    va_end(va);

    // 字符串实际占用长度包含子串尾的 '\0'
    str_len = DIAG_STRLEN(fmt) + 1;

    // 获取实际buffer占用的长度，包含格式化字符串部分和参数部分
    buffer_size = arg_buf_len + str_len;

    // 参数长度大于设置的值，丢弃该条log，退出函数
    // 退出前增加 log sequence，表示当前log丢失
    if(buffer_size > DIAG_ONE_LOG_MAX_SIZE_BYTE)
    {
        diag_packet_add_length_err_num();
        // 调用该函数，自增 sequence number
        (void) diag_packet_get_sequence_num();
        return DIAG_PRINT_FAILED;
    }

    // 使用动态申请内存的方式，替换原来的局部变量方式，方便通过宏增加或减小log支持的最大size，使用局部变量会影响调用线程的栈空间
    static_log = (StaticLog_t *) diag_mem_alloc(sizeof(StaticLog_t) + buffer_size + DIAG_STRUCT_TAIL_LEN, NONUSE_RESERED_AREA);

    // 内存申请失败，无法打印，退出函数
    // 退出前增加 log sequence，表示当前log丢失
    if(static_log == NULL)
    {
        // 调用该函数，自增 sequence number
        (void) diag_packet_get_sequence_num();
        return DIAG_PRINT_FAILED;
    }

    // 第二次格式化，用于填充buffer
    va_start(va, fmt);
    arg_buf_len = diag_format_arguments_to_buffer((char *)(static_log->u8Payload), arg_buf_len, fmt, va);
    va_end(va);

    // 如果返回负数，说明格式化失败，释放内存，退出函数
    // 退出前增加 log sequence，表示当前log丢失
    if(arg_buf_len < 0)
    {
        diag_packet_add_format_fail_num();
        // 调用该函数，自增 sequence number
        (void) diag_packet_get_sequence_num();
        diag_mem_free(static_log);
        return DIAG_PRINT_FAILED;
    }

    // 填充头部结构
    static_log->u8SrcId     = (uint8_t) src_id;
    static_log->u8MsgLev    = (uint8_t) lev;
    static_log->u8ParamSize = (uint8_t) arg_buf_len;
    static_log->u8CoreType  = (uint8_t) Core_CP;
    static_log->u16MsgSize  = (uint16_t) buffer_size;

    // 静态log，把格式化字符串也拷贝到buffer中，拷贝到参数之后
    // 这里放在这里拷贝，如果一开始申请的内存比较多，先拷贝这块可能会造成重复拷贝，浪费CPU资源
    DIAG_MEMCPY(static_log->u8Payload + arg_buf_len, fmt, str_len);

    // 填充公共的头部信息，即 ItemHeader_t 结构体
    diag_packet_fill_header((ItemHeader_t *)static_log, sizeof(StaticLog_t) + buffer_size, port, 0);
    // 填充CRC和数据尾部，该处会计算CRC，可能会比较耗时
    // 这里需要填充完头部信息后，才能计算CRC，CRC校验的长度会从头部中获取
    diag_packet_fill_crc_and_tail((ItemHeader_t *)static_log);

    // 发送log，这里指定要发送的数据的起始地址和长度，发送层不关心log的数据结构
    diag_port_send_log((void *)static_log, sizeof(StaticLog_t) + buffer_size + DIAG_STRUCT_TAIL_LEN);

    return DIAG_PRINT_SUCCESS;
}


/*----------------------------------------------------------------------------------------------------*/

/*明文字符串打印的动态ID输出方式，常用于PS和平台字符串输出*/
__FLASH_FUNC diag_print_state_t diag_platform_dynamic_log(int dyn_id, XY_SRC_E src_id, XY_LOG_LEV lev, const char *fmt, ...)
{
    DynamicLog_t * dyn_log;
    va_list va;
    int arg_buf_len;
    size_t buffer_size;

    // 当前不允许打log的话，就直接退出
    if(diag_filter_get_send_enable_state(src_id) != DIAG_SEND_ENABLE)
    {
        return DIAG_PRINT_FAILED;
    }
    
    if(diag_filter_normal_log(src_id, lev) != DIAG_SEND_ENABLE)
    {
        return DIAG_PRINT_FAILED;
    }

    // 第一次做格式化，用于获取需要的buffer长度
    va_start(va, fmt);
    arg_buf_len = diag_format_get_arguments_length(fmt, va);
    va_end(va);

    // 获取实际buffer占用的长度，这里只包含参数长度
    buffer_size = arg_buf_len;

    // 参数长度大于设置的值，丢弃该条log，退出函数
    // 退出前增加 log sequence，表示当前log丢失
    if(buffer_size > DIAG_ONE_LOG_MAX_SIZE_BYTE)
    {
        diag_packet_add_length_err_num();
        // 调用该函数，自增 sequence number
        (void) diag_packet_get_sequence_num();
        return DIAG_PRINT_FAILED;
    }

    // 使用动态申请内存的方式，替换原来的局部变量方式，方便通过宏增加或减小log支持的最大size，使用局部变量会影响调用线程的栈空间
    dyn_log = (DynamicLog_t *) diag_mem_alloc(sizeof(DynamicLog_t) + buffer_size + DIAG_STRUCT_TAIL_LEN, NONUSE_RESERED_AREA);

    // 内存申请失败，无法打印，退出函数
    // 退出前增加 log sequence，表示当前log丢失
    if(dyn_log == NULL)
    {
        // 调用该函数，自增 sequence number
        (void) diag_packet_get_sequence_num();
        return DIAG_PRINT_FAILED;
    }

    // 第二次格式化，用于填充buffer
    va_start(va, fmt);
    arg_buf_len = diag_format_arguments_to_buffer((char *)(dyn_log->u8Payload), arg_buf_len, fmt, va);
    va_end(va);
    
    // 如果返回负数，说明格式化失败，释放内存，退出函数
    // 退出前增加 log sequence，表示当前log丢失
    if(arg_buf_len < 0)
    {
        diag_packet_add_format_fail_num();
        // 调用该函数，自增 sequence number
        (void) diag_packet_get_sequence_num();
        diag_mem_free(dyn_log);
        return DIAG_PRINT_FAILED;
    }

    // 填充头部结构
    dyn_log->u8SrcId    = (uint8_t) src_id;
    dyn_log->u8MsgLev   = (uint8_t) lev;
    dyn_log->u16DynId   = (uint16_t) dyn_id;
    dyn_log->u16MsgSize = (uint16_t) buffer_size;

    // 填充公共的头部信息，即 ItemHeader_t 结构体
    diag_packet_fill_header((ItemHeader_t *)dyn_log, sizeof(DynamicLog_t) + buffer_size, XY_DYNAMIC_LOG, dyn_id);
    // 填充CRC和数据尾部，该处会计算CRC，可能会比较耗时
    // 这里需要填充完头部信息后，才能计算CRC，CRC校验的长度会从头部中获取
    diag_packet_fill_crc_and_tail((ItemHeader_t *)dyn_log);

    // 发送log，这里指定要发送的数据的起始地址和长度，发送层不关心log的数据结构
    diag_port_send_log((void *)dyn_log, sizeof(DynamicLog_t) + buffer_size + DIAG_STRUCT_TAIL_LEN);

    return DIAG_PRINT_SUCCESS;
}
/*----------------------------------------------------------------------------------------------------*/
