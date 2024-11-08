#include "diag_options.h"
#include "diag_mem.h"
#include "diag_transmit_port.h"
#include "diag_item_struct.h"
#include "diag_item_types.h"
#include "diag_msg_type.h"
#include "diag_packet.h"
#include "diag_filter.h"
#include "diag_print.h"
#include <stdarg.h>

/*PS模块内输出结构体内容使用，不能用于模块间消息*/
__FLASH_FUNC diag_print_state_t diag_ps_signalling_log(uint8_t msg_id, char *msg, int msg_len, diag_ps_type_t signalling_type)
{
    uint32_t type_size = 0;
    Protocol_t * signal_log;
    size_t buffer_size;

    // 当前不允许打log的话，就直接退出
    if(diag_filter_get_send_enable_state(LRRC) != DIAG_SEND_ENABLE)
    {
        return DIAG_PRINT_FAILED;
    }
    
    if(diag_filter_signalling_log(signalling_type) != DIAG_SEND_ENABLE)
    {
        return DIAG_PRINT_FAILED;
    }

    // 根据不同的信令类型，获取需要申请的内存空间
    if(signalling_type == NAS_TYPE)
    {
        type_size = sizeof(NasHeader_t);
    }
    else if((signalling_type == RRC_TYPE) || (signalling_type == ASN1_BER_TYPE))
    {
        type_size = sizeof(RrcHeader_t);
    }
    else
    {
        DIAG_ASSERT(0);
    }

    // 获取buffer占用的内存大小，在填充公共头 ItemHeader_t 时会用到
    buffer_size = type_size + msg_len;

    // 申请指定大小的内存
    signal_log = (Protocol_t *) diag_mem_alloc(sizeof(Protocol_t) + buffer_size + DIAG_STRUCT_TAIL_LEN, CAN_USE_RESERED_AREA);

    // 内存申请失败，无法打印，退出函数
    // 退出前增加 log sequence，表示当前log丢失
    if(signal_log == NULL)
    {
        // 调用该函数，自增 sequence number
        (void) diag_packet_get_sequence_num();
        return DIAG_PRINT_FAILED;
    }

    // 填充 Protocol_t 结构体
    signal_log->u8Type = (uint8_t) signalling_type;
    signal_log->u8Ver = 0;
    signal_log->u16Len = type_size + msg_len;

    // 根据信令的类型，把信令头部和信令内容填充入指定地址
    if(signalling_type == NAS_TYPE)
    {
        NasHeader_t *nas_header = (NasHeader_t *)signal_log->u8Payload;
        nas_header->u8Direction = msg_id;
        nas_header->u16Len = msg_len;

        // 把信令内容拷贝到指定位置
        DIAG_MEMCPY(nas_header->u8Payload, msg, msg_len);
    }
    else if((signalling_type == RRC_TYPE) || (signalling_type == ASN1_BER_TYPE))
    {
        RrcHeader_t *rrc_header = (RrcHeader_t *)signal_log->u8Payload;
        rrc_header->u8MsgId = msg_id;
        rrc_header->u16Len = msg_len;
        
        // 把信令内容拷贝到指定位置
        DIAG_MEMCPY(rrc_header->u8Payload, msg, msg_len);
    }
    else
    {
        DIAG_ASSERT(0);
    }

    // 填充公共的头部信息，即 ItemHeader_t 结构体
    diag_packet_fill_header((ItemHeader_t *)signal_log, sizeof(Protocol_t) + buffer_size, XY_PROTOCOL_LOG, 0);
    // 填充CRC和数据尾部，该处会计算CRC，可能会比较耗时
    // 这里需要填充完头部信息后，才能计算CRC，CRC校验的长度会从头部中获取
    diag_packet_fill_crc_and_tail((ItemHeader_t *)signal_log);

    // 发送log，这里指定要发送的数据的起始地址和长度，发送层不关心log的数据结构
    diag_port_send_log((void *)signal_log, sizeof(Protocol_t) + buffer_size + DIAG_STRUCT_TAIL_LEN);

    return DIAG_PRINT_SUCCESS;
}
/*----------------------------------------------------------------------------------------------------*/



/*PS模块间消息传递头信息*/
typedef struct msg_header_stru
{
    uint16_t  ulMemSize;
    uint8_t   ulSrcTskId;  //消息发送方模块ID
    uint8_t   ulDestTskId; //消息接收方模块ID
    uint32_t  ulMsgClass;  //PS内部细分的子模块
    uint32_t  ulMsgName;   //模块间消息ID
} MSG_HEADER_STRU;

/*PS与phy等模块间标准消息传递的输出log*/
__FLASH_FUNC diag_print_state_t diag_ps_primitive_log(char *msg)
{
    Message_t * prim_log = (Message_t*)msg;
    MSG_HEADER_STRU * PsMsgHeader;
    size_t buffer_size;

    DIAG_ASSERT(prim_log != NULL);

    PsMsgHeader = (MSG_HEADER_STRU *) (prim_log->u8Payload);

    // 当前不允许打log的话，就直接退出
    if(diag_filter_get_send_enable_state(LRRC) != DIAG_SEND_ENABLE)
    {
        diag_mem_free(msg);
        return DIAG_PRINT_FAILED;
    }
    
    if(diag_filter_primitive_log(PsMsgHeader->ulMsgClass, PsMsgHeader->ulMsgName) != DIAG_SEND_ENABLE)
    {
        diag_mem_free(msg);
        return DIAG_PRINT_FAILED;
    }
	
    // 获取buffer占用的内存大小，在填充公共头 ItemHeader_t 时会用到
    buffer_size = PsMsgHeader->ulMemSize;

    // 填充公共的头部信息，即 ItemHeader_t 结构体
    diag_packet_fill_header((ItemHeader_t *)prim_log, sizeof(Message_t) + buffer_size, XY_MESSAGE_LOG, PsMsgHeader->ulMsgName);

	PsMsgHeader->ulMemSize -= sizeof(MSG_HEADER_STRU);
	
    // 填充CRC和数据尾部，该处会计算CRC，可能会比较耗时
    // 这里需要填充完头部信息后，才能计算CRC，CRC校验的长度会从头部中获取
    diag_packet_fill_crc_and_tail((ItemHeader_t *)prim_log);

    // 发送log，这里指定要发送的数据的起始地址和长度，发送层不关心log的数据结构
    diag_port_send_log((void *)prim_log, sizeof(Message_t) + buffer_size + DIAG_STRUCT_TAIL_LEN);

    return DIAG_PRINT_SUCCESS;
}
/*----------------------------------------------------------------------------------------------------*/



__FLASH_FUNC diag_print_state_t diag_ps_wireshark_log(char *data, int data_len, uint8_t type)
{
    CommonCnf_t * wire_log;
    WireShark_t * wireShark;
    size_t buffer_size;

    // 当前不允许打log的话，就直接退出
    if(diag_filter_get_send_enable_state(-1) != DIAG_SEND_ENABLE)
    {
        return DIAG_PRINT_FAILED;
    }

    // 获取buffer占用的内存大小，在填充公共头 ItemHeader_t 时会用到
    buffer_size = sizeof(WireShark_t) + data_len;

    // 申请指定大小的内存
    wire_log = (CommonCnf_t *) diag_mem_alloc(sizeof(CommonCnf_t) + buffer_size + DIAG_STRUCT_TAIL_LEN, NONUSE_RESERED_AREA);

    // 内存申请失败，无法打印，退出函数
    // 退出前增加 log sequence，表示当前log丢失
    if(wire_log == NULL)
    {
        // 调用该函数，自增 sequence number
        (void) diag_packet_get_sequence_num();
        return DIAG_PRINT_FAILED;
    }

    // 填充头结构信息
    wire_log->u8SrcId = WIRESHARCK_AP;
    wire_log->u16Len = buffer_size;

    // 填充wireshark头结构
    wireShark = (WireShark_t *) (wire_log->u8Payload);
    wireShark->len = data_len;
    wireShark->type = type;

    // 拷贝wireshark的数据
    if(data_len > 0)
    {
        DIAG_MEMCPY(wireShark->u8Payload, data, data_len);
    }

    // 填充公共的头部信息，即 ItemHeader_t 结构体
    diag_packet_fill_header((ItemHeader_t *)wire_log, sizeof(CommonCnf_t) + buffer_size, XY_SYSAPPCNF_LOG, XY_SYSAPP_WIRESHARK_IND);
    // 填充CRC和数据尾部，该处会计算CRC，可能会比较耗时
    // 这里需要填充完头部信息后，才能计算CRC，CRC校验的长度会从头部中获取
    diag_packet_fill_crc_and_tail((ItemHeader_t *)wire_log);

    // 发送log，这里指定要发送的数据的起始地址和长度，发送层不关心log的数据结构
    diag_port_send_log((void *)wire_log, sizeof(CommonCnf_t) + buffer_size + DIAG_STRUCT_TAIL_LEN);

    return DIAG_PRINT_SUCCESS;
}
/*----------------------------------------------------------------------------------------------------*/
