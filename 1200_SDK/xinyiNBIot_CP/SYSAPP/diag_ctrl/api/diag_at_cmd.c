#include "diag_options.h"
#include "diag_at_cmd.h"
#include "diag_mem.h"
#include "diag_transmit_port.h"
#include "diag_item_struct.h"
#include "diag_item_types.h"
#include "diag_packet.h"
#include "diag_filter.h"

/*log通道进行AT命令应答发送*/
__FLASH_FUNC diag_print_state_t diag_at_response_output(char *at_str, int str_len)
{
    ATCmd_t * at_response;
    size_t buffer_size;

    // 当前不允许打log的话，就直接退出
    if(diag_filter_get_send_enable_state(-1) != DIAG_SEND_ENABLE)
    {
        return DIAG_PRINT_FAILED;
    }

    // 字符串占用长度
    buffer_size = str_len;

    // 使用动态申请内存的方式，替换原来的局部变量方式，方便通过宏增加或减小log支持的最大size，使用局部变量会影响调用线程的栈空间
    at_response = (ATCmd_t *) diag_mem_alloc(sizeof(ATCmd_t) + buffer_size + DIAG_STRUCT_TAIL_LEN, NONUSE_RESERED_AREA);

    // 内存申请失败，无法打印，退出函数
    // 退出前增加 log sequence，表示当前log丢失
    if(at_response == NULL)
    {
        // 调用该函数，自增 sequence number
        (void) diag_packet_get_sequence_num();
        return DIAG_PRINT_FAILED;
    }

    // 填充头部结构
    at_response->u16Len = (uint16_t) buffer_size;

    // 静态log，把格式化字符串也拷贝到buffer中，拷贝到参数之后
    // 这里放在这里拷贝，如果一开始申请的内存比较多，先拷贝这块可能会造成重复拷贝，浪费CPU资源
    DIAG_MEMCPY(at_response->u8Payload, at_str, buffer_size);

    // 填充公共的头部信息，即 ItemHeader_t 结构体
    diag_packet_fill_header((ItemHeader_t *)at_response, sizeof(ATCmd_t) + buffer_size, XY_AT_LOG, 0);
    // 填充CRC和数据尾部，该处会计算CRC，可能会比较耗时
    // 这里需要填充完头部信息后，才能计算CRC，CRC校验的长度会从头部中获取
    diag_packet_fill_crc_and_tail((ItemHeader_t *)at_response);

    // 发送log，这里指定要发送的数据的起始地址和长度，发送层不关心log的数据结构
    diag_port_send_log((void *)at_response, sizeof(ATCmd_t) + buffer_size + DIAG_STRUCT_TAIL_LEN);

    return DIAG_PRINT_SUCCESS;
}