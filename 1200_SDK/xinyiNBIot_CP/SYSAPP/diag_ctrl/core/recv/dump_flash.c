/*
 * dump_flash.c
 *
 *  Created on: 2022年5月18日
 *      Author: jiangzj
 */

#include "xy_memmap.h"
#include "dump_flash.h"
#include "flash_adapt.h"
#include "qspi_flash.h"
#include "xy_flash.h"
#include "mpu_protect.h"
#include "ipc_msg.h"
#include "hw_memmap.h"

extern void flash_vendor_delay(unsigned long uldelay);
#define UPROUND(raw, base)    ((((raw)+(base)-1)/(base))*(base))
#define MAGIC_NUM         0x12345678
#define IMAGE_NUM         ( 12 )
#define MAX_FILE_NAME     ( 32 )
#define BB_REG_LEN     	  ( 0x300 )     //BB_REG_BASE:0x4001b000

// External Flash First 256 bytes
typedef struct {
	uint32_t flash_addr;
	uint32_t ram_addr;
	uint32_t len;
	char file_name[MAX_FILE_NAME];
}image_info_t;

typedef struct {
	// Header Info
	uint32_t magic_num;
	uint32_t image_num;
	uint32_t assert_core;
	// Image Info
	image_info_t image_info[IMAGE_NUM];
} Flash_Header;

typedef struct {
	unsigned int image_id[3];
	unsigned int load_addr;
	unsigned int exec_addr;
	unsigned int len_byte;
	unsigned int image_content_crc;
	unsigned int image_info_crc;
} Flash_Image_Def;
// External Flash First 256 bytes
typedef struct {
	// Header Info
	unsigned int image_num;
	unsigned int image_info_addr_base;
	unsigned int user_code;
	unsigned int nv_addr_base;
	unsigned int nv_len_byte;
	unsigned int exchange_addr_base;
	unsigned int exchange_len_byte;
	unsigned int header_info_crc;

	// Image Info
    Flash_Image_Def image_info[7];
} Flash_Header_Def;


__FLASH_FUNC void dump_flash_to_file(void)
{
	Flash_Header dump_flash;
	image_info_t* image_info;

	// 等待正常流程的log数据发送完成
	osCoreEnterCritical();
	if(DMAChannelTransferRemainCNT(DMA_CHANNEL_1) != 0)
	{
		DMAChannelWaitIdle(DMA_CHANNEL_1);
	}
	osCoreExitCritical();

	flash_read(DUMP_INFO_BASE_ADDR, (uint8_t *)&dump_flash, sizeof(dump_flash));

	extern void diag_dma_to_io(void);
	diag_dma_to_io();
	
	if(dump_flash.magic_num == MAGIC_NUM)
	{
		for(unsigned int i = 0; i < dump_flash.image_num; i++)
		{
			image_info = &dump_flash.image_info[i];
			diag_dump_mem_info(image_info->file_name, image_info->flash_addr, image_info->len);
		}
	}
	
	diag_port_wait_send_done();
		
	extern void diag_io_to_dma(void);
	diag_io_to_dma();
}

