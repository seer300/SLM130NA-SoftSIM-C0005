#include "diag_options.h"
#include "diag_cmd_send.h"
#include "diag_mem.h"
#include "diag_packet.h"
#include "diag_format.h"
#include "diag_transmit_port.h"
#include "diag_item_struct.h"
#include "diag_item_types.h"
#include "at_uart.h"
#include "factory_nv.h"
#include <stdarg.h>


// 这里是为了4字节对齐，防止编译器分配到不对齐的地址
static uint32_t  diag_dump_buffer[ (sizeof(ItemHeader_t) + sizeof(MemInfo_t) + DIAG_DUMP_ONE_PACKET_SIZE + DIAG_STRUCT_TAIL_LEN + sizeof(uint32_t) - 1) / sizeof(uint32_t) ];



__FLASH_FUNC void diag_dump_mem_info(char *mem_file, uint32_t mem_addr, size_t mem_size)
{
    ItemHeader_t * item_head;
    MemInfo_t * mem_info;
    uint32_t filename_len;
    uint32_t buffer_size;
    uint32_t has_tras_size = 0;
    uint16_t packet_seq = 0;

    item_head = (ItemHeader_t *) diag_dump_buffer;
    mem_info = (MemInfo_t *) (item_head->u8Payload);

    // 获取文件名的长度
    filename_len = DIAG_STRLEN(mem_file) + 1;

    // 文件名拷贝需要条件，防止文件名过长，数组越界
    if(filename_len > DIAG_DUMP_ONE_PACKET_SIZE)
    {
        buffer_size = DIAG_DUMP_ONE_PACKET_SIZE;

        DIAG_MEMCPY(mem_info->u8Payload, mem_file, buffer_size - 1);
        mem_info->u8Payload[buffer_size - 1] = '\0';
    }
    else
    {
        buffer_size = filename_len;

        DIAG_MEMCPY(mem_info->u8Payload, mem_file, buffer_size);
    }

    // 发送文件名的包
    mem_info->u8Len = buffer_size;
    mem_info->u8Seq = packet_seq++;
    mem_info->u8PEnd = 0;

    // 填充公共的头部信息，即 ItemHeader_t 结构体
    diag_packet_fill_dump_header(item_head, sizeof(ItemHeader_t) + sizeof(MemInfo_t) + buffer_size, XY_MEMORY_LOG, 0);
    // 填充CRC和数据尾部，该处会计算CRC，可能会比较耗时
    // 这里需要填充完头部信息后，才能计算CRC，CRC校验的长度会从头部中获取
    diag_packet_fill_dump_crc_and_tail(item_head);
    // 发送log，这里指定要发送的数据的起始地址和长度，发送层不关心log的数据结构
    diag_port_send_log_directly((void *)item_head, sizeof(ItemHeader_t) + sizeof(MemInfo_t) + buffer_size + DIAG_STRUCT_TAIL_LEN);

    // 一直发送数据包，直到数据包发送完成
    while(has_tras_size < mem_size)
    {
        // 当前包不是最后一包，则发送整包大小，否则只发送剩余的包大小
        if((has_tras_size + DIAG_DUMP_ONE_PACKET_SIZE) < mem_size)
        {
            buffer_size = DIAG_DUMP_ONE_PACKET_SIZE;
            mem_info->u8Len = buffer_size;
            mem_info->u8Seq = packet_seq;
            mem_info->u8PEnd = 0;
        }
        else
        {
            buffer_size = mem_size - has_tras_size;
            mem_info->u8Len = buffer_size;
            mem_info->u8Seq = packet_seq;
            mem_info->u8PEnd = 1;
        }

        DIAG_MEMCPY(mem_info->u8Payload, (void *) (mem_addr + has_tras_size), buffer_size);

        // 每发送一包数据，需要更新下面的数值
        packet_seq++;
        has_tras_size += buffer_size;

        // 填充公共的头部信息，即 ItemHeader_t 结构体
        diag_packet_fill_dump_header(item_head, sizeof(ItemHeader_t) + sizeof(MemInfo_t) + buffer_size, XY_MEMORY_LOG, 0);
        // 填充CRC和数据尾部，该处会计算CRC，可能会比较耗时
        // 这里需要填充完头部信息后，才能计算CRC，CRC校验的长度会从头部中获取
        diag_packet_fill_dump_crc_and_tail(item_head);
        // 发送log，这里指定要发送的数据的起始地址和长度，发送层不关心log的数据结构
        diag_port_send_log_directly((void *)item_head, sizeof(ItemHeader_t) + sizeof(MemInfo_t) + buffer_size + DIAG_STRUCT_TAIL_LEN);
    }
	


}
/*----------------------------------------------------------------------------------------------------*/

/*用于logview上行命令的应答回复*/
__FLASH_FUNC diag_print_state_t diag_cmd_send_normal(uint32_t type_id, uint32_t cmd_id, uint8_t src_id, char* data, int msg_len)
{
    CommonCnf_t * cmd_log;
    size_t buffer_size;

    // 当前不允许打log的话，就直接退出
    if(diag_filter_get_send_enable_state(-1) != DIAG_SEND_ENABLE)
    {
        return DIAG_PRINT_FAILED;
    }

    // 获取buffer占用的内存大小，在填充公共头 ItemHeader_t 时会用到
    buffer_size = msg_len;

    // 使用动态申请内存的方式，替换原来的局部变量方式，方便通过宏增加或减小log支持的最大size，使用局部变量会影响调用线程的栈空间
    cmd_log = (CommonCnf_t *) diag_mem_alloc(sizeof(CommonCnf_t) + buffer_size + DIAG_STRUCT_TAIL_LEN, NONUSE_RESERED_AREA);

    // 内存申请失败，无法打印，退出函数
    // 退出前增加 log sequence，表示当前log丢失
    if(cmd_log == NULL)
    {
        // 调用该函数，自增 sequence number
        (void) diag_packet_get_sequence_num();
        return DIAG_PRINT_FAILED;
    }

    // 填充 CommonCnf_t 的描述信息
    cmd_log->u8SrcId = src_id;
    cmd_log->u16Len = msg_len;
    
    // 拷贝数据到buffer区域
    if((data != NULL) && (msg_len > 0))
    {
        DIAG_MEMCPY(cmd_log->u8Payload, data, msg_len);
    }

    // 填充公共的头部信息，即 ItemHeader_t 结构体
    diag_packet_fill_header((ItemHeader_t *)cmd_log, sizeof(CommonCnf_t) + buffer_size, type_id, cmd_id);
    // 填充CRC和数据尾部，该处会计算CRC，可能会比较耗时
    // 这里需要填充完头部信息后，才能计算CRC，CRC校验的长度会从头部中获取
    diag_packet_fill_crc_and_tail((ItemHeader_t *)cmd_log);

    // 发送log，这里指定要发送的数据的起始地址和长度，发送层不关心log的数据结构
    diag_port_send_log((void *)cmd_log, sizeof(CommonCnf_t) + buffer_size + DIAG_STRUCT_TAIL_LEN);

    return DIAG_PRINT_SUCCESS;
}
/*----------------------------------------------------------------------------------------------------*/


__FLASH_FUNC diag_print_state_t diag_cmd_send_directly(uint32_t type_id, uint32_t cmd_id, uint8_t src_id, char* data, int msg_len)
{
    CommonCnf_t * cmd_log;
    size_t buffer_size;

    // 获取buffer占用的内存大小，在填充公共头 ItemHeader_t 时会用到
    buffer_size = msg_len;

    // 直接发送时，复用dump的buffer，数据不能超过buffer的长度
    if(buffer_size > DIAG_DUMP_ONE_PACKET_SIZE)
    {
        return DIAG_PRINT_FAILED;
    }

    // 使用动态申请内存的方式，替换原来的局部变量方式，方便通过宏增加或减小log支持的最大size，使用局部变量会影响调用线程的栈空间
    cmd_log = (CommonCnf_t *) diag_dump_buffer;

    // 填充 CommonCnf_t 的描述信息
    cmd_log->u8SrcId = src_id;
    cmd_log->u16Len = msg_len;
    
    // 拷贝数据到buffer区域
    if((data != NULL) && (msg_len > 0))
    {
        DIAG_MEMCPY(cmd_log->u8Payload, data, msg_len);
    }

    // 填充公共的头部信息，即 ItemHeader_t 结构体
    diag_packet_fill_header((ItemHeader_t *)cmd_log, sizeof(CommonCnf_t) + buffer_size, type_id, cmd_id);
    // 填充CRC和数据尾部，该处会计算CRC，可能会比较耗时
    // 这里需要填充完头部信息后，才能计算CRC，CRC校验的长度会从头部中获取
    diag_packet_fill_crc_and_tail((ItemHeader_t *)cmd_log);

    // 发送log，这里指定要发送的数据的起始地址和长度，发送层不关心log的数据结构
    diag_port_send_log_directly((void *)cmd_log, sizeof(CommonCnf_t) + buffer_size + DIAG_STRUCT_TAIL_LEN);

    return DIAG_PRINT_SUCCESS;
}
/*----------------------------------------------------------------------------------------------------*/


__FLASH_FUNC diag_print_state_t diag_static_log_directly(XY_SRC_E src_id, XY_LOG_LEV lev, const char *fmt, ...)
{
    StaticLog_t * static_log;
    va_list va;
    int arg_buf_len;
    size_t str_len;
    size_t buffer_size;

    // 第一次做格式化，用于获取需要的buffer长度
    va_start(va, fmt);
    arg_buf_len = diag_format_get_arguments_length(fmt, va);
    va_end(va);

    // 字符串实际占用长度包含子串尾的 '\0'
    str_len = DIAG_STRLEN(fmt) + 1;

    // 获取实际buffer占用的长度，包含格式化字符串部分和参数部分
    buffer_size = arg_buf_len + str_len;

    // 此包数据长度大于buffer长度，丢弃该条log，退出函数
    // 退出前增加 log sequence，表示当前log丢失
    if((sizeof(StaticLog_t) + buffer_size + DIAG_STRUCT_TAIL_LEN) > sizeof(diag_dump_buffer))
    {
        diag_packet_add_length_err_num();
        // 调用该函数，自增 sequence number
        (void) diag_packet_get_sequence_num();
        return DIAG_PRINT_FAILED;
    }

    // 把直接输出使用的buffer作为静态log的结构
    static_log = (StaticLog_t *) diag_dump_buffer;

    // 第二次格式化，用于填充buffer
    va_start(va, fmt);
    arg_buf_len = diag_format_arguments_to_buffer((char *)(static_log->u8Payload), arg_buf_len, fmt, va);
    va_end(va);

    // 如果返回负数，说明格式化失败，退出函数
    // 退出前增加 log sequence，表示当前log丢失
    if(arg_buf_len < 0)
    {
        diag_packet_add_format_fail_num();
        // 调用该函数，自增 sequence number
        (void) diag_packet_get_sequence_num();
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
    diag_packet_fill_header((ItemHeader_t *)static_log, sizeof(StaticLog_t) + buffer_size, XY_STATIC_LOG, 0);
    // 填充CRC和数据尾部，该处会计算CRC，可能会比较耗时
    // 这里需要填充完头部信息后，才能计算CRC，CRC校验的长度会从头部中获取
    diag_packet_fill_crc_and_tail((ItemHeader_t *)static_log);

    // 发送log，这里指定要发送的数据的起始地址和长度，发送层不关心log的数据结构
    diag_port_send_log_directly((void *)static_log, sizeof(StaticLog_t) + buffer_size + DIAG_STRUCT_TAIL_LEN);

    return DIAG_PRINT_SUCCESS;
}
/*----------------------------------------------------------------------------------------------------*/


__FLASH_FUNC diag_print_state_t diag_cmd_production_response(char *str) 
{
    StaticLog_t * static_log;
    size_t str_len;
    size_t buffer_size;

    // 字符串实际占用长度包含子串尾的 '\0'
    str_len = DIAG_STRLEN(str) + 1;

    // 获取实际buffer占用的长度，包含格式化字符串部分和参数部分
    buffer_size = str_len;

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

    // 填充头部结构
    static_log->u8SrcId     = (uint8_t) PLATFORM;
    static_log->u8MsgLev    = (uint8_t) WARN_LOG;
    static_log->u8ParamSize = (uint8_t) 0;
    static_log->u8CoreType  = (uint8_t) Core_CP;
    static_log->u16MsgSize  = (uint16_t) buffer_size;

    // 静态log，把格式化字符串也拷贝到buffer中，拷贝到参数之后
    // 这里放在这里拷贝，如果一开始申请的内存比较多，先拷贝这块可能会造成重复拷贝，浪费CPU资源
    DIAG_MEMCPY(static_log->u8Payload, str, str_len);

    // 填充公共的头部信息，即 ItemHeader_t 结构体
    diag_packet_fill_header((ItemHeader_t *)static_log, sizeof(StaticLog_t) + buffer_size, XY_STATIC_LOG, 0);
    // 填充CRC和数据尾部，该处会计算CRC，可能会比较耗时
    // 这里需要填充完头部信息后，才能计算CRC，CRC校验的长度会从头部中获取
    diag_packet_fill_crc_and_tail((ItemHeader_t *)static_log);

    // 发送log，这里指定要发送的数据的起始地址和长度，发送层不关心log的数据结构
    diag_port_send_log((void *)static_log, sizeof(StaticLog_t) + buffer_size + DIAG_STRUCT_TAIL_LEN);

    return DIAG_PRINT_SUCCESS;
}
/*----------------------------------------------------------------------------------------------------*/
