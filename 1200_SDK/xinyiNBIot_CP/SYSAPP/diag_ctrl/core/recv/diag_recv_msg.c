#include "diag_options.h"
#include "diag_recv_msg.h"
#include "diag_packet.h"
#include "diag_transmit_port.h"
#include "diag_item_struct.h"
#include "diag_item_types.h"
#include "diag_msg_type.h"
#include "diag_mem.h"
#include <string.h>


// 数据包是否有效，作为 diag_recv_check_command_packet 函数的返回值
#define DIAG_RECEIVE_PACKET_VALID           0
#define DIAG_RECEIVE_PACKET_INVALID         -1
#define DIAG_RECEIVE_PACKET_UNFINISHED      1

// 接收buffer的大小，建议不要小于64字节
#define DIAG_RECEIVE_BUFFER_SZIE            128

// 接收数据的buffer以及buffer中未处理数据的长度
static uint32_t diag_recv_buffer[DIAG_RECEIVE_BUFFER_SZIE / sizeof(uint32_t)];
static uint32_t diag_recv_len = 0;

/*
 * 移除buffer起始位置的无效数据，保证buffer的起始位置是数据包的有效头部
 */
static void diag_recv_remove_invalid_data(void);

/*
 * 检查buffer的起始位置是否存在有效数据包，除了数据包头部外，还需要校验整个数据包
 */
static int diag_recv_check_command_packet(void);

/*
 * 删除buffer中已经处理的数据包长度，移动未处理数据到buffer起始位置
 */
static void diag_recv_clear_processed_data(uint16_t length);

/*
 * 处理接收到的有效的数据包，根据数据包的内容执行相应的处理，该函数实现在单独的一个文件中
 */
void diag_recv_command_process(ItemHeader_t *cmd_buffer);

/*
 * 向接收buffer中写入特殊的数据包，在接受处理中进行处理，此函数是为产线工具发送0x00时做的特殊处理
 * 这里没有直接在此函数中回复数据，需要在接收处理中判断当前是否是断言状态
 */
static void diag_recv_special_response(void);

/*----------------------------------------------------------------------------------------------------*/


__FLASH_FUNC void diag_recv_reset_buffer(void)
{
    // 复位接收buffer，清空所有数据
    diag_recv_len = 0;
}
/*----------------------------------------------------------------------------------------------------*/


__FLASH_FUNC void diag_recv_write_data_to_buffer(uint8_t *data, uint32_t length)
{
    uint8_t *recv_buf = (uint8_t *)diag_recv_buffer;

    // 如果数据长度还没有超过buffer剩余长度，则把数据写入buffer中
    if ((length > 0) && ((diag_recv_len + length) <= DIAG_RECEIVE_BUFFER_SZIE))
    {
        memcpy(&recv_buf[diag_recv_len], data, length);
        diag_recv_len += length;
    }
}
/*----------------------------------------------------------------------------------------------------*/


__FLASH_FUNC void diag_recv_check_buffer_and_process_command(void)
{
    ItemHeader_t *cmd_buffer = (ItemHeader_t *)diag_recv_buffer;
    uint16_t item_len;
    int packet_check;

    // buffer中还存在数据，就一直处理
    while (diag_recv_len > 0)
    {
        // 移除buffer起始位置的无效数据，保证起始位置是有效的数据包头部
        diag_recv_remove_invalid_data();

        // 校验数据包，判断是否有效
        packet_check = diag_recv_check_command_packet();

        // 校验数据有效，处理数据，并清除已处理的数据
        if(packet_check == DIAG_RECEIVE_PACKET_VALID)
        {
            g_log_status.recv_valid_packet_tick = (uint32_t)DIAG_GET_TICK_COUNT();
            // 收到有效数据包，进行处理
            diag_recv_command_process(cmd_buffer);

            // 数据包已经处理完成，清除当前已经处理过的数据
            item_len = cmd_buffer->u16Len + HEADERSIZE + LENSIZE;
            diag_recv_clear_processed_data(item_len);
        }
        // 数据还未收到完整的一包，则退出循环
        else if(packet_check == DIAG_RECEIVE_PACKET_UNFINISHED)
        {
            uint8_t * first_byte = (uint8_t *) diag_recv_buffer;
            
            // 此处是为产线工具做的定制，收到0x00时，返回非 0x6B/0x6C 的数据
            if ((diag_recv_len == 1) && (*first_byte == 0x00))
            {
                diag_recv_special_response();
            }
            else
            {
                break;
            }
        }
        // 数据校验失败，移除数据包头部的4字节
        else
        {
            diag_recv_clear_processed_data(4);
        }
    }
}
/*----------------------------------------------------------------------------------------------------*/


__FLASH_FUNC static void diag_recv_remove_invalid_data(void)
{
    uint8_t *recv_buf = (uint8_t *)diag_recv_buffer;
    uint32_t invalid_len;
    uint32_t head_val;
    uint8_t *tmp_head = (uint8_t *)&head_val;

    // 接收长度大于等于4，才能判断，因为必须这里判断的是数据包的头部，头部占4字节
    if(diag_recv_len >= 4)
    {
        // 一直寻找到最后的4字节
        for(invalid_len = 0; invalid_len < (diag_recv_len - 3); invalid_len++)
        {
            tmp_head[0] = recv_buf[invalid_len + 0];
            tmp_head[1] = recv_buf[invalid_len + 1];
            tmp_head[2] = recv_buf[invalid_len + 2];
            tmp_head[3] = 0;
            
            // 一直找到有效的头部数据，再退出循环
            if (head_val == 0xFEA55A)
            {
                break;
            }
        }

        // 即使全部数据都是无效的，这里最终也会留下3个字节，防止后面的数据和这剩余的3个字节可以组成一个有效头部
        if(invalid_len != 0)
        {
            // 清除buffer中无效的数据，保证有效数据包头部处于buffer的起始位置
            diag_recv_clear_processed_data(invalid_len);
        }
    }
}
/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC static int diag_recv_check_command_packet(void)
{
    ItemHeader_t *cmd_buffer = (ItemHeader_t *)diag_recv_buffer;
    uint16_t item_len = cmd_buffer->u16Len + HEADERSIZE + LENSIZE;

    // 接收的数据长度小于6，则认为没有接收完成，6字节是4字节头，加上2字节长度
    if(diag_recv_len < 6)
    {
        return DIAG_RECEIVE_PACKET_UNFINISHED;
    }
    // 数据长度错误，不必继续校验，返回数据包无效
    else if((item_len > DIAG_RECEIVE_BUFFER_SZIE) || (item_len < sizeof(ItemHeader_t)))
    {
        // 不做处理
    }
    // 已经接收的数据包长度大于等于数据包长度的时候，再进行校验
    else if(item_len <= diag_recv_len)
    {
        // 数据包校验成功，返回成功的状态，让后续进行处理
        if(0 == diag_packet_analysis_head(cmd_buffer))
        {
            return DIAG_RECEIVE_PACKET_VALID;
        }
    }
    else
    {
        return DIAG_RECEIVE_PACKET_UNFINISHED;
    }

    return DIAG_RECEIVE_PACKET_INVALID;
}
/*----------------------------------------------------------------------------------------------------*/


__FLASH_FUNC static void diag_recv_clear_processed_data(uint16_t length)
{
    uint8_t *recv_buf = (uint8_t *)diag_recv_buffer;

    // 减去已经处理过的数据的长度
    diag_recv_len -= length;

    // 未处理数据长度大于0，就需要把未处理数据移动到buffer的起始位置
    if((diag_recv_len > 0) && (length > 0))
    {
        memmove(recv_buf, &recv_buf[length], diag_recv_len);
    }
}
/*----------------------------------------------------------------------------------------------------*/

__FLASH_FUNC static void diag_recv_special_response(void)
{
    ItemHeader_t *cmd_buffer = (ItemHeader_t *)diag_recv_buffer;

    DIAG_CRITICAL_DEF(isr);

    // 进入临界区，防止此时产生中断，打乱buffer中的内容
    DIAG_ENTER_CRITICAL(isr);

    // 手动填充buffer的内容，处理此命令
    cmd_buffer->u32Header = 0xFFEA55A;
    cmd_buffer->uChipType = 0xE;
    cmd_buffer->u16Len = 10;
    cmd_buffer->u16SeqNum = 0;
    cmd_buffer->u4TraceId = XY_MAX_LOG;
    cmd_buffer->u28ClassId = 0;
    cmd_buffer->u32Time = 0;

    // 更新接收到的数据包长度
    diag_recv_len = sizeof(ItemHeader_t);

    DIAG_EXIT_CRITICAL(isr);
}
/*----------------------------------------------------------------------------------------------------*/
