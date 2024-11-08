#include "xy_flash.h"
#include "xy_utils.h"
#include "xy_at_api.h"
#include "oss_nv.h"
#include "xy_memmap.h"
#include "ipc_msg.h"
#include "mpu_protect.h"
#include "flash_adapt.h"
#include "xy_at_api.h"
#include "xy_fota.h"
#include "lpm_nv_write.h"

extern volatile int lpm_fs_flag;

/*CP核本地业务进行flash操作，通过后台代理线程执行操作，并可以被高优先级暂停*/
void cp_flash_operation(uint32_t dstAddr, uint32_t srcAddr, uint32_t size, uint32_t operation, uint8_t aysn_flag)
{
	volatile uint32_t val = 0;
	volatile uint32_t syncdelay;
	volatile flash_notice_t* flash_notice = (flash_notice_t*)BAK_MEM_FLASH_NOTICE;

	if(operation==flash_op_writeErase || operation==flash_op_erase || operation==flash_op_writeNoErase)
	{
		extern uint32_t assert_flash;
		extern uint8_t assert_val;
		/*仅用于调试，排查FLASH部分值被异常修改*/
		if(assert_flash>=dstAddr && assert_flash<dstAddr+size)
		{
			if(operation == flash_op_erase)
				xy_assert(assert_val == 0XFF);
			else
				xy_assert(*((uint8_t *)(srcAddr+(assert_flash-dstAddr))) == assert_val);
		}
			
		/*正常工作态，除了FOTA+调试命令容许写IMEI/FAC，文件系统初始化容许擦写flash，其它禁止写FLASH*/
		if(NOT_ALLOWED_SAVE_FLASH() && OTA_is_doing()!=true && dstAddr!=NV_MAIN_FACTORY_BASE && dstAddr!=NV_FLASH_RF_BASE && dstAddr!=NV_FLASH_RF_BAKUP_BASE && dstAddr!=FOTA_BREAKPOINT_INFO_ADDR && 
			(dstAddr<SECONDBOOT_PRIME_HEADER_BASE || dstAddr>SECONDBOOT_BACKUP_HEADER_BASE+0x1000) &&
			(dstAddr < FS_FLASH_BASE || dstAddr > (FS_FLASH_BASE + FS_FLASH_LENGTH)))
		{
			char str[50] = {0};
			
			sprintf(str, "\r\n+DBGINFO:ERR!!!CP FLASH BAN WRITE %lX\r\n",dstAddr);
			send_debug_by_at_uart(str);
			
			xy_printf(0,PLATFORM,WARN_LOG, "[ERR!!!]CP flash ban write %X",dstAddr);
			
			if (HWREGB(BAK_MEM_XY_DUMP) == 1)
				xy_assert(0);
		}
		else
		{
			xy_printf(0,PLATFORM,WARN_LOG, "cp_flash_operation CP flash write %X",dstAddr);
		}

		flash_write_protect(dstAddr,size);
	}
	
	/*操作系统正常运行时，写flash通过代理线程，以保证物理层等高优先级线程能及时suspend住flash*/
	if((osCoreGetState() == osCoreNormal))
	{
		//同步写，等待代理线程写完
		if(aysn_flag == 0)
		{
			osSemaphoreId_t flashwrite_msg_sem = xSemaphoreCreateBinary();
			notify_flash_do(dstAddr, srcAddr, size, operation, aysn_flag, flashwrite_msg_sem);
			xSemaphoreTake(flashwrite_msg_sem,osWaitForever);
			vSemaphoreDelete(flashwrite_msg_sem);
		}
		else  //异步写，通知代理线程写即可
		{
			notify_flash_do(dstAddr, srcAddr, size, operation, aysn_flag, NULL);
		}
	}
	//起调度前，nv_restore等接口可能执行写操作，如RF备份
	else if((osKernelGetState() == osKernelInactive))
	{
		xy_assert(cp_status_idle == flash_notice->cp_status);//cp_status在非idle态，直接断言，否则cp_status会错乱
		flash_notice->cp_status = cp_status_write;
		shm_msg_write("Inactive", 8, ICM_FLASHWRT_NOTICE); //通知ap挂起，写入线程名为了查问题方便
		while(flash_notice->ap_status == ap_status_run) //循环等待ap核进入suspend态，这里不能用osDelay切出去
		{
			for(syncdelay = 0; syncdelay < 12000; syncdelay ++); 
		}
		
        /*只要是通过xy_flash.h接口进行操作的，都临时容许进行前半段的擦写*/
		if(dstAddr < FLASH_HARDWARE_PROTECT_LIMIT)
			val = flash_hardware_protect_disable();

		if(operation == flash_op_writeErase)
		{
			flash_write_erase(dstAddr, (uint8_t *)srcAddr,size);
			if(aysn_flag == 1)
				xy_free((void *)srcAddr);
		}
		else if(operation == flash_op_erase)
			flash_erase(dstAddr,size);
		else if(operation == flash_op_writeNoErase)
			flash_write_no_erase(dstAddr, (uint8_t *)srcAddr,size);
		else
			xy_assert(0);

		/* 禁止flash前半段的擦写 */
		if(dstAddr < FLASH_HARDWARE_PROTECT_LIMIT)
			flash_hardware_protect_enable(val);

		flash_notice->cp_status = cp_status_writedone; //通知ap退出suspend态

		while(flash_notice->ap_status == ap_status_sus)  //循环等待ap核退出suspend态，这里不能用osDelay切出去
		{
			for(syncdelay = 0; syncdelay < 12000; syncdelay ++);
		}

		flash_notice->cp_status = cp_status_idle; //cp_status回到idle态
	}
	else if((osCoreGetState() == osCoreInCritical) && (osKernelIsRunningIdle() == osOK))	//idle线程且为锁中断,即深睡流程中保存flash
	{
		/*idle里写FLASH，通知AP核执行。目前没有在idle线程里直接擦写FLASH的操作*/
		if(lpm_fs_flag != 1)
		{
			uint8_t *ptr = NULL;
			uint32_t src_addr;

			if(operation != flash_op_erase)
			{
				xy_assert(srcAddr != NULL);
				ptr = xy_malloc(size);
				memcpy(ptr,(void *)srcAddr,size);
				src_addr = (unsigned int)ptr;
			}
			else
				src_addr = (uint32_t)NULL;

            /*深睡过程，idle线程中，一旦锁中断，则无法本地执行NV写，需要传递给AP核执行写flash动作*/
			lpm_nv_write_buff_add((uint32_t)Address_Translation_CP_To_AP((unsigned int)dstAddr), (uint32_t)Address_Translation_CP_To_AP((unsigned int)src_addr), size);
		}
		else /*FS写FLASH定制流程，通过挂起AP核后执行CP写FLASH操作*/
		{
			xy_assert(NOT_ALLOWED_SAVE_FLASH() == 0);
			
			if(operation == flash_op_writeErase)
			{
				flash_write_erase(dstAddr, (uint8_t *)srcAddr,size);
				if(aysn_flag == 1)
					xy_free((void *)srcAddr);
			}
			else if(operation == flash_op_erase)
				flash_erase(dstAddr,size);
			else if(operation == flash_op_writeNoErase)
				flash_write_no_erase(dstAddr, (uint8_t *)srcAddr,size);
			else
				xy_assert(0);
		}
	}
	else //其他的锁中断写flash直接断言
	{
		xy_assert(0);
	}
}


bool xy_Flash_Erase(uint32_t addr, uint32_t size)
{
	cp_flash_operation(addr, (uint32_t)NULL, size, flash_op_erase, 0);
	return 1;
}

bool xy_Flash_Read_UniqueID128(void *data)
{
	cp_flash_operation(FLASH_BASE, (uint32_t)data, 16, flash_op_read_UniqueID128, 0);
	return 1;
}

bool xy_Flash_Read_fhp_status(uint32_t *data)
{
	uint8_t* temp_data = xy_malloc(sizeof(uint32_t));
	cp_flash_operation(FLASH_BASE, (uint32_t)temp_data, 4, flash_op_read_fhpstatus, 0);
	*data = *(uint32_t *)temp_data;
	xy_free(temp_data);
	return 1;
}

bool xy_Flash_Read(uint32_t addr,void *data,uint32_t size)
{
	flash_read(addr, data, size);
	return 1;
}

void flash_write(uint32_t addr, void *data, uint32_t size, uint8_t asyn_flag)
{
	uint32_t offset;
	uint32_t addr_align;
	uint32_t write_size;
	uint8_t *p_page_buf = NULL;

	offset = addr&(FLASH_SECTOR_LENGTH-1);
	addr_align = addr - offset;
	if(offset == 0 && size == FLASH_SECTOR_LENGTH)		//目的地址为4k对齐，且写入数据长度为4k，则直接写入，所有ftl_write皆为此类
	{
		cp_flash_operation(addr_align, (uint32_t)data, FLASH_SECTOR_LENGTH, flash_op_writeErase, asyn_flag);
	}
	else	//目的地址非4k对齐或写入数据长度非4k时，需重申请4k地址，拷贝原data数据到相应位置，再以4k为单位写入，直接调用xy_flash_write时可能出现此类情况
	{
		while(size > 0)
		{
			if(size <= (FLASH_SECTOR_LENGTH - offset))
			{
				write_size = size;
			}
			else
			{
				write_size = FLASH_SECTOR_LENGTH - offset;
			}
			if(p_page_buf == NULL)
			{
				p_page_buf = xy_malloc(FLASH_SECTOR_LENGTH);
			}
			if(write_size != FLASH_SECTOR_LENGTH)
			{
				flash_read(addr_align, p_page_buf, FLASH_SECTOR_LENGTH);
			}
			memcpy((p_page_buf+offset),data,write_size);
			cp_flash_operation(addr_align, (uint32_t)p_page_buf, FLASH_SECTOR_LENGTH, flash_op_writeErase, asyn_flag);
			size -= write_size;
			addr += write_size;
			data += write_size;
			offset = addr&(FLASH_SECTOR_LENGTH-1);
			addr_align = addr - offset;
		}
		xy_free(p_page_buf);	//只有调用xy_flash_write接口才可能走到该分支，xy_flash_write为同步接口，需在flash写入后自行释放buf
	}
}

bool xy_Flash_Write(uint32_t addr, void *data, uint32_t size)
{
	flash_write(addr, data, size, 0);
	return 1;
}

void xy_flash_write_async(uint32_t addr, void *data, uint32_t size)
{
	flash_write(addr, data, size, 1);
}

bool xy_Flash_Write_No_Erase(uint32_t addr, void *data, uint32_t size)
{
	uint32_t offset;
	uint32_t write_size;

	while(size > 0)
	{
		offset = addr&(FLASH_SECTOR_LENGTH-1);

		if(size <= (FLASH_SECTOR_LENGTH - offset))
		{
			write_size = size;
		}
		else
		{
			write_size = FLASH_SECTOR_LENGTH - offset;
		}
		
		cp_flash_operation(addr, (uint32_t)data, write_size, flash_op_writeNoErase, 0);
		
		size -= write_size;
		addr += write_size;
		data += write_size;
	}
	
	return 1;
}


