#include "diag_options.h"
#include "diag_recv_msg.h"
#include "diag_cmd_send.h"
#include "diag_filter.h"
#include "diag_item_struct.h"
#include "diag_item_types.h"
#include "diag_msg_type.h"
#include "diag_transmit_port.h"
#include "diag_mem.h"
#include "diag_packet.h"
#include "dump_flash.h"
#include "factory_nv.h"
#include "xy_system.h"
#include "watchdog.h"
#include "prcm.h"
#include "tick.h"
#include "at_ctl.h"
#include "dfe.h"
#include "ipc_msg.h"
#include "mpu_protect.h"
#include "dump.h"

// 由该变量判断，当前是否处于死机断言后的dump状态，根据这个状态，也需要调用不同的接口
static int diag_dump_nesting = 0;
static int diag_dump_wait_respond = 0;
static int diag_dump_req = 1;
/*----------------------------------------------------------------------------------------------------*/
static int diag_strcmp(const char* p1, const char* p2)
{
	const unsigned char* s1 = (const unsigned char*)p1;
	const unsigned char* s2 = (const unsigned char*)p2;
	unsigned char c1, c2;

	do
	{
		c1 = (unsigned char) *s1++;
		c2 = (unsigned char) *s2++;
		if(c1 == '\0')
			return c1 - c2;
	}while(c1 == c2);

	return c1 - c2;
}

// 导出所有的内存，会在第一次断言时以及发送dump命令时调用
static void diag_dump_memory_to_file(char* filename)
{
	if(filename == NULL)
	{
        char ThreadName[configMAX_TASK_NAME_LEN+1] ={0};
	    char *pxName =NULL;
	    UBaseType_t x;

		if(xTaskGetCurrentTaskHandle() != NULL)
		{
			pxName = (char *)osThreadGetName(osThreadGetId());
			for( x = ( UBaseType_t ) 0; x < ( UBaseType_t ) configMAX_TASK_NAME_LEN; x++ )
			{
				ThreadName[ x ] = pxName[ x ];
			}
		}
		// 在内存导出之前，输出死机的文件名，行号和当前线程名
		diag_static_log_directly(PLATFORM, CRITICAL_LOG, "+ASSERT:CP,%s,%d,%s", assert_file, assert_line, ThreadName);

		diag_dump_mem_info("ap_ram_01000000", AP_SRAM_BASE, AP_SRAM_LENGTH);
		diag_dump_mem_info("cp_ram_10000000", SRAM_BASE, SRAM_LENGTH);
		diag_dump_mem_info("shared_ram_60000000", SHARE_MEM_BASE, SHARE_MEM_LENGTH + BAK_MEM_LENGTH);
		diag_dump_mem_info("coreprcm_reg_0x40004000", COREPRCM_BASE, 0x100);
		diag_dump_mem_info("aonprcm_reg_0x40000000", AONPRCM_BASE, 0x100);
		diag_dump_mem_info("dmac_reg_0x40050000", DMAC_BASE, 0x300);
		diag_dump_mem_info("bb_reg_0x4001b000", BB_REG_BASE, 0x300);

	}
	else
	{
		if(!diag_strcmp(filename, "ap_ram_01000000"))
		{
			diag_dump_mem_info("ap_ram_01000000", AP_SRAM_BASE, AP_SRAM_LENGTH);
		}
		else if(!diag_strcmp(filename, "cp_ram_10000000"))
		{
			diag_dump_mem_info("cp_ram_10000000", SRAM_BASE, SRAM_LENGTH);
		}
		else if(!diag_strcmp(filename, "shared_ram_60000000"))
		{
			diag_dump_mem_info("shared_ram_60000000", SHARE_MEM_BASE, SHARE_MEM_LENGTH + BAK_MEM_LENGTH);
		}
		else if(!diag_strcmp(filename, "coreprcm_reg_0x40004000"))
		{
			diag_dump_mem_info("coreprcm_reg_0x40004000", COREPRCM_BASE, 0x100);
		}
		else if(!diag_strcmp(filename, "aonprcm_reg_0x40000000"))
		{
			diag_dump_mem_info("aonprcm_reg_0x40000000", AONPRCM_BASE, 0x100);
		}
		else if(!diag_strcmp(filename, "dmac_reg_0x40050000"))
		{
			diag_dump_mem_info("dmac_reg_0x40050000", DMAC_BASE, 0x300);
		}
		else if(!diag_strcmp(filename, "bb_reg_0x4001b000"))
		{
			diag_dump_mem_info("bb_reg_0x4001b000", BB_REG_BASE, 0x300);
		}

	}
	
	diag_port_wait_send_done();
}
/*----------------------------------------------------------------------------------------------------*/


 void proc_FORCEDL_req()
{
    // 设置boot模式并软复位
    HWREGB(0x4000003C) = 0x81;//force uart boot
    HWREGB(0x40000002) |= 0x08;//softreset
    while(1);
}

 void diag_recv_command_process(ItemHeader_t *cmd_buffer)
{
    uint32_t trace_id = cmd_buffer->u4TraceId;
    uint32_t cmd_id = cmd_buffer->u28ClassId;

    if(trace_id == XY_SYSAPPREQ_LOG)
    {
        switch(cmd_id)
        {
            case XY_SYSAPP_ASSERT_REQ:
                {
                    // 如果不在断言流程，才能够通过该命令主动断言，否则断言会嵌套
                    if (diag_dump_nesting == 0)
                    {
                        DIAG_ASSERT(0);
                    }
                }
                break;

            case XY_SYSAPP_MEMREADY_REQ:
                {
                    // 内存导出时，必须在dump流程，否则可能和正常的log数据穿插在一起，因为这里是直接发送数据的
                    if (diag_dump_nesting != 0)
                    {
                    	diag_dump_req = 0;
#if (DUMP_TO_FILE == 1)
                        diag_dump_memory_to_file(NULL);
                        diag_dump_wait_respond = 1;
#endif
                    }
                }
                break;

            case XY_SYSAPP_MEMDUMP_REQ:
                {
                    // 内存导出时，必须在dump流程，否则可能和正常的log数据穿插在一起，因为这里是直接发送数据的
                    if (diag_dump_nesting != 0)
                    {
                    	MemInfo_t * mem_info = (MemInfo_t *)cmd_buffer->u8Payload;
                    	if(mem_info->u8Len)
                    	{
#if (DUMP_TO_FILE == 1)
							diag_dump_memory_to_file((char*)mem_info->u8Payload);
							diag_dump_wait_respond = 1;
#endif
                        }
                        else
                        {
                        	diag_dump_wait_respond = 0;
                        }
                    }
                }
                break;

            case XY_SYSAPP_FILTER_REQ:
                {
                    // 只在非断言流程才响应该功能
                    if (diag_dump_nesting == 0)
                    {
                        // 获取数据区域的起始地址
                        filterInfo * bitmap_info = (filterInfo *) cmd_buffer->u8Payload;
                        // 取前两个4字节，分别为 src_id_bimap 和 log_lev_bitmap
                        diag_filter_set_log_bitmap(bitmap_info);
                        diag_cmd_send_normal(XY_SYSAPPCNF_LOG, XY_SYSAPP_FILTER_CNF, PLATFORM, NULL, 0);
                    }
                }
                break;
            
            case XY_SYSAPP_MAXLEN_REQ:
                {
                    // 只在非断言流程才响应该功能
                    if (diag_dump_nesting == 0)
                    {
                        // TODO: 此功能暂未实现
                    }
                }
                break;
            
            case XY_SYSAPP_HEART_REQ:
                {
                    // 只在非断言流程才响应该功能
                    if (diag_dump_nesting == 0)
                    {
                        diag_filter_refresh_heart_beat();

                        //first revceive heart cnf
                        if(cmd_buffer->u16SeqNum == 0)
                        {
                            diag_cmd_send_normal(XY_SYSAPPCNF_LOG, XY_SYSAPP_HEART_CNF,PLATFORM, XY_SOFTWARE_VERSION, strlen(XY_SOFTWARE_VERSION));
                        }
                        else
                        {
                            if(g_diag_debug.occupy_node_full_cnt != 0 || g_diag_debug.occupy_memory_full_cnt != 0 || g_diag_debug.length_error_cnt != 0 || g_diag_debug.format_fail_cnt != 0)
                            {
                                DIAG_CRITICAL_DEF(isr);
                                DIAG_ENTER_CRITICAL(isr);
                                diag_debug_info_t record_info = g_diag_debug;  //由于log接收线程优先级低，规避被高优先级打断后debug info被更新
                                DIAG_EXIT_CRITICAL(isr);
                                
                                if (DIAG_PRINT_SUCCESS == PrintLogSt(PLATFORM, WARN_LOG, "SoC logInfo:succ=%d;mem fail=%d;node full=%d;len err=%d;format fail=%d;delay max=%d ms", record_info.alloc_mem_succ_cnt, record_info.occupy_memory_full_cnt, record_info.occupy_node_full_cnt, record_info.length_error_cnt, record_info.format_fail_cnt, record_info.send_time_interval))
                                {
                                    diag_packet_refresh_debug_info(&record_info);
                                }
                            }
                        }
                    }
                }
                break;

            case XY_SYSAPP_FORCEDL_REQ:
                {
                    __set_PRIMASK(1);
                    if (diag_dump_nesting == 0)
                    {
                        diag_port_dump_environment_init();//死机流程已完dump环境配置
                    }
                    // 回复给工具侧的命令，直接发送，需要等待发送完成
                    diag_cmd_send_directly(XY_SYSAPPCNF_LOG, XY_SYSAPP_FORCEDL_CNF, 0, NULL, 0);
                    diag_port_wait_send_done();
                    proc_FORCEDL_req();
                }
                break;
            
            default:
                // 未识别命令不作处理
                break;
        }
    }
    else if (trace_id == XY_AT_LOG)
    {
        ATCmd_t * at_cmd = (ATCmd_t *) cmd_buffer;
        
        // 只在非断言流程才响应该功能
        if (diag_dump_nesting == 0 && at_cmd->itemHeader.u16Len == (10 + 4 + at_cmd->u16Len))
        {
        	//log发送过来的at数据受log接收buffer影响，一次最大传输不超过128字节
            at_recv_from_log(FARPS_LOG_FD,(char *)at_cmd->u8Payload, at_cmd->u16Len);
        }
    }
    else if (trace_id == XY_MAX_LOG)
    {
        // 此处是为了向产线工具回复数据，只要不是 0x6B/0x6C 即可
        if (diag_dump_nesting == 0)
        {
            diag_cmd_production_response("Production Line");
        }
        else
        {
            diag_static_log_directly(PLATFORM, CRITICAL_LOG, "Production Line");
        }
    }
}
/*----------------------------------------------------------------------------------------------------*/

/*该函数及用到的子函数必须全部运行在RAM上，否则AP核执行DUMP到flash时退XIP会跑飞*/
 void Dump_Memory_to_File(void)
{
    uint8_t  uart_rx_fifo[32];
    uint32_t buf_len;

    //logView导dump功能，release模式下仅尝试导一次，然后等AP核触发容错机制；debug模式下死等logview导出
    if(diag_dump_nesting == 0)
    {
        diag_dump_nesting = 1;

        // 如果当前不允许打印log，无法dump内存，直接退出
        // if(diag_filter_get_send_enable_state() == DIAG_SEND_ENABLE)
        {
            // 初始化dump环境，才能正常执行后续dump操作
            diag_port_dump_environment_init();
            
            // 清空接收buffer，从正常流程进入到dump流程，以前的数据需要清空，防止重复执行
            diag_recv_reset_buffer();

            // 根据当前clk_tick的时钟和分频，设置超时的时刻点为2s后，超时不再与工具进行交互
			uint64_t timeout_tick = (uint64_t) TickCounterGet() + 32768 * 2 / 32;
			uint64_t wait_respond_tick = 0;
			uint64_t wait_req_tick = 0;

			while((uint64_t) TickCounterGet() < timeout_tick)
			{
				if(diag_dump_req)
				{
					uint64_t current_tick = (uint64_t) TickCounterGet();
					// 发送dump命令给工具，工具会回复dump请求包，收到dump请求包后进行dump
					while((current_tick - wait_req_tick) > (32768 / 5 / 32) || wait_req_tick == 0) //每隔200ms
					{
						diag_cmd_send_directly(XY_SYSAPPCNF_LOG, XY_SYSAPP_MEMHAVE_IND, PLATFORM, NULL, 0);
						wait_req_tick = current_tick;
					}
				}

				buf_len = diag_port_recv_after_dump(uart_rx_fifo, sizeof(uart_rx_fifo));
				diag_recv_write_data_to_buffer(uart_rx_fifo, buf_len);
				diag_recv_check_buffer_and_process_command();

				if(diag_dump_wait_respond)
				{
					uint64_t current_tick = (uint64_t) TickCounterGet();
					timeout_tick = current_tick + 32768 * 2 / 32;
					while((current_tick - wait_respond_tick) > (32768 / 5 / 32) || wait_respond_tick == 0) //每隔200ms
					{
						diag_cmd_send_directly(XY_SYSAPPCNF_LOG, XY_SYSAPP_MEMDONE_IND, PLATFORM, NULL, 0);
						wait_respond_tick = current_tick;
					}
				}
			}

			/*尝试一段时间dump到logview后，通知AP核可以进行芯片复位，是否复位根据debug，release模式*/
//			HWREGB(BAK_MEM_DUMP_LOGVIEW_FLAG) = 1;
			wait_respond_tick = 0;

			/*debug模式下，死等logview的接入导出，release模式下后续代码没有意义*/
            if(HWREGB(BAK_MEM_XY_DUMP) == 1)
            {
//#if OPEN_LOGVIEW_DUMP
            	// 一直与log工具进行交互，处理命令
				while(1)
				{
					buf_len = diag_port_recv_after_dump(uart_rx_fifo, sizeof(uart_rx_fifo));
					diag_recv_write_data_to_buffer(uart_rx_fifo, buf_len);
					diag_recv_check_buffer_and_process_command();
					if(diag_dump_wait_respond)
					{
						uint64_t current_tick = (uint64_t) TickCounterGet();
						while((current_tick - wait_respond_tick) > (32768 / 5 / 32) || wait_respond_tick == 0) //每隔200ms
						{
							diag_cmd_send_directly(XY_SYSAPPCNF_LOG, XY_SYSAPP_MEMDONE_IND, PLATFORM, NULL, 0);
							wait_respond_tick = current_tick;
						}
					}

					dump_sync();
				}
//#endif
            }

        }
    }
}
/*----------------------------------------------------------------------------------------------------*/
