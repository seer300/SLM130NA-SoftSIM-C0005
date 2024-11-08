#include "diag_options.h"
#include "diag_packet.h"
#include "diag_mem.h"

static uint32_t diag_sequence_num = 0;

__RAM_FUNC uint32_t diag_packet_get_sequence_num(void)
{
    uint32_t sequnce_num;

    DIAG_CRITICAL_DEF(isr);

    DIAG_ENTER_CRITICAL(isr);
    sequnce_num = diag_sequence_num;
    diag_sequence_num++;
    DIAG_EXIT_CRITICAL(isr);

    return sequnce_num;
}

// 此函数均在锁临界区时调用
__RAM_FUNC void diag_packet_set_sequence_num(diag_list_t *diag_iter)
{
    ((ItemHeader_t *)(DIAG_LIST_GET_SEND_DATA(diag_iter)))->u16SeqNum = diag_sequence_num;
    ++diag_sequence_num;
}

__RAM_FUNC void diag_packet_add_length_err_num(void)
{
    DIAG_CRITICAL_DEF(isr);

    DIAG_ENTER_CRITICAL(isr);
    g_diag_debug.length_error_cnt++;
    DIAG_EXIT_CRITICAL(isr);
}

__RAM_FUNC void diag_packet_add_format_fail_num(void)
{
    DIAG_CRITICAL_DEF(isr);

    DIAG_ENTER_CRITICAL(isr);
    g_diag_debug.format_fail_cnt++;
    DIAG_EXIT_CRITICAL(isr);
}

__FLASH_FUNC void diag_packet_refresh_debug_info(diag_debug_info_t *record_info)
{
    DIAG_CRITICAL_DEF(isr);

    DIAG_ENTER_CRITICAL(isr);
    if(g_diag_debug.send_time_interval == record_info->send_time_interval)
    {
        g_diag_debug.send_time_interval = 0;
    }
    g_diag_debug.alloc_mem_succ_cnt -= record_info->alloc_mem_succ_cnt;
    g_diag_debug.occupy_node_full_cnt -= record_info->occupy_node_full_cnt;
    g_diag_debug.occupy_memory_full_cnt -= record_info->occupy_memory_full_cnt;
    g_diag_debug.length_error_cnt -= record_info->length_error_cnt;
    g_diag_debug.format_fail_cnt -= record_info->format_fail_cnt;
    DIAG_EXIT_CRITICAL(isr);
}

/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC void diag_packet_fill_header(ItemHeader_t *pItemHeader, uint32_t buf_sz, uint32_t type_id, uint32_t item_id)
{
    uint8_t *item_header = (uint8_t *)pItemHeader;

    item_header[0] = 0x5A;
    item_header[1] = 0xA5;
    item_header[2] = 0xFE;
    item_header[3] = Chip_1100;

    pItemHeader->u16Len = buf_sz - HEADERSIZE - LENSIZE;
    pItemHeader->u4TraceId = type_id;
    pItemHeader->u28ClassId = item_id;
    pItemHeader->u16SeqNum = 0;
    pItemHeader->u32Time = DIAG_GET_TICK_COUNT();
}
/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC void diag_packet_fill_crc_and_tail(ItemHeader_t *data)
{
#if (DIAG_CALCULATE_CRC_ENABLE == 1)
    
    uint8_t *cdata = (uint8_t *)data;

    int item_len;
    int i;
    char checksum = 0x00;

    item_len = data->u16Len + HEADERSIZE + LENSIZE;

    for(i = 0; i < item_len; i++)
    {
        checksum ^= cdata[i];
    }

    cdata[item_len] = checksum;
    cdata[item_len+1] = 0xFA;

#else

    uint8_t *cdata = (uint8_t *)data;

    int item_len = data->u16Len + HEADERSIZE + LENSIZE;
    cdata[item_len] = 0xFA;

#endif
}
/*----------------------------------------------------------------------------------------------------*/


__FLASH_FUNC void diag_packet_fill_dump_header(ItemHeader_t *pItemHeader, uint32_t buf_sz, uint32_t type_id, uint32_t item_id)
{
    char *item_header = (char *)pItemHeader;

    item_header[0] = 0x5A;
    item_header[1] = 0xA5;
    item_header[2] = 0xFE;
    item_header[3] = Chip_1100;

    pItemHeader->u16Len = buf_sz - HEADERSIZE - LENSIZE;
    pItemHeader->u4TraceId = type_id;
    pItemHeader->u28ClassId = item_id;
    pItemHeader->u16SeqNum = 0;
    pItemHeader->u32Time = 0;
}
/*----------------------------------------------------------------------------------------------------*/


__FLASH_FUNC void diag_packet_fill_dump_crc_and_tail(ItemHeader_t *data)
{
    uint8_t *cdata = (uint8_t *)data;

    int item_len = data->u16Len + HEADERSIZE + LENSIZE;
    cdata[item_len] = 0xFA;
}
/*----------------------------------------------------------------------------------------------------*/


__FLASH_FUNC int diag_packet_analysis_head(ItemHeader_t *pItemHeader)
{
    if((pItemHeader->u32Header & 0xFFFFFF) != 0xFEA55A)
        return -1;

    return 0;
}
/*----------------------------------------------------------------------------------------------------*/
