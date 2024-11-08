#include "diag_options.h"
#include "diag_mem.h"
#include "diag_format.h"
#include "diag_transmit_port.h"
#include "diag_item_struct.h"
#include "diag_msg_type.h"
#include "diag_packet.h"
#include "diag_filter.h"
#include "diag_print.h"
#include <stdarg.h>
#include "diag_config.h"

/*明文字符串动态log输出，指定参数的格式，以加快速度*/
__RAM_FUNC diag_print_state_t diag_phy_fixed_arguments_log(int dyn_id, XY_SRC_E src_id, XY_LOG_LEV lev, uint32_t arg_num, const char *fmt, ...)
{
    DynamicLog_t * dyn_log;
    va_list va;
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

    buffer_size = sizeof(uint32_t) * arg_num;
    #if(DIAG_PHY_MEMPOOL_USE)
    if(lev == PREALLOC_LOG)
    {
        dyn_log = (DynamicLog_t *) diag_phy_mempool_malloc();
    }

    // 使用动态申请内存的方式，替换原来的局部变量方式，方便通过宏增加或减小log支持的最大size，使用局部变量会影响调用线程的栈空间
    else
    {
        dyn_log = (DynamicLog_t *) diag_mem_alloc(sizeof(DynamicLog_t) + buffer_size + DIAG_STRUCT_TAIL_LEN, NONUSE_RESERED_AREA);
    }
    #else
    dyn_log = (DynamicLog_t *) diag_mem_alloc(sizeof(DynamicLog_t) + buffer_size + DIAG_STRUCT_TAIL_LEN, NONUSE_RESERED_AREA);
    #endif

    // 内存申请失败，无法打印，退出函数
    // 退出前增加 log sequence，表示当前log丢失
    if(dyn_log == NULL)
    {
        // 调用该函数，自增 sequence number
        (void) diag_packet_get_sequence_num();
        return DIAG_PRINT_FAILED;
    }

    // 填充 DynamicLog_t 的描述信息
    dyn_log->u8SrcId    = (uint8_t) src_id;
    dyn_log->u8MsgLev   = (uint8_t) lev;
    dyn_log->u16DynId   = (uint16_t) dyn_id;
    dyn_log->u16MsgSize = (uint16_t) buffer_size;

    // 填充指定的参数个数，从变参中提取
    va_start(va, fmt);
    diag_format_fixed_args(dyn_log->u8Payload, arg_num, va);
    va_end(va);

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

typedef struct
{
    uint16_t  usHSN;
    uint16_t  usSFN;
    uint8_t   ucSubf;
    uint8_t   ucSN;
    uint16_t  usLen;
    uint8_t   ucDataType;
    uint8_t   ucPadding;
    uint8_t   aucRawData[0];
} PHY_RAW_DATA_t;

/*物理层结构体内容的打印输出*/
__FLASH_FUNC diag_print_state_t diag_phy_primitive_log(void *pMsg, uint16_t usLen, uint16_t usHSN, uint16_t usSFN, uint8_t ucSubf, uint8_t ucSN, uint8_t ucDataType)
{
    CommonCnf_t * prim_log;
    PHY_RAW_DATA_t * pstPhyRawData;
    size_t buffer_size;

    DIAG_ASSERT(pMsg != NULL);

    // 当前不允许打log的话，就直接退出
    if(diag_filter_get_send_enable_state(LPHY) != DIAG_SEND_ENABLE)
    {
        return DIAG_PRINT_FAILED;
    }

    buffer_size = sizeof(PHY_RAW_DATA_t) + usLen;

    prim_log = (CommonCnf_t *) diag_mem_alloc(sizeof(CommonCnf_t) + buffer_size + DIAG_STRUCT_TAIL_LEN, CAN_USE_RESERED_AREA);

    // 内存申请失败，无法打印，退出函数
    // 退出前增加 log sequence，表示当前log丢失
    if(prim_log == NULL)
    {
        // 调用该函数，自增 sequence number
        (void) diag_packet_get_sequence_num();
        return DIAG_PRINT_FAILED;
    }

    // 填充 CommonCnf_t 的描述信息
    prim_log->u8SrcId = (uint8_t) LPHY;
    prim_log->pad     = 0;
    prim_log->u16Len  = (uint16_t) buffer_size;

    pstPhyRawData = (PHY_RAW_DATA_t *) prim_log->u8Payload;
    pstPhyRawData->usHSN      = usHSN;
    pstPhyRawData->usSFN      = usSFN;
    pstPhyRawData->ucSubf     = ucSubf;
    pstPhyRawData->ucSN       = ucSN;
    pstPhyRawData->usLen      = usLen;
    pstPhyRawData->ucDataType = ucDataType;
    pstPhyRawData->ucPadding  = 0;

    DIAG_MEMCPY(pstPhyRawData->aucRawData, pMsg, usLen);

    // 填充公共的头部信息，即 ItemHeader_t 结构体
    diag_packet_fill_header((ItemHeader_t *)prim_log, sizeof(CommonCnf_t) + buffer_size, XY_SUBSYSCNF_LOG, XY_SUBSYS_PHY_RAW_DATA_CNF);
    // 填充CRC和数据尾部，该处会计算CRC，可能会比较耗时
    // 这里需要填充完头部信息后，才能计算CRC，CRC校验的长度会从头部中获取
    diag_packet_fill_crc_and_tail((ItemHeader_t *)prim_log);

    // 发送log，这里指定要发送的数据的起始地址和长度，发送层不关心log的数据结构
    diag_port_send_log((void *)prim_log, sizeof(CommonCnf_t) + buffer_size + DIAG_STRUCT_TAIL_LEN);

    return DIAG_PRINT_SUCCESS;
}
/*----------------------------------------------------------------------------------------------------*/
