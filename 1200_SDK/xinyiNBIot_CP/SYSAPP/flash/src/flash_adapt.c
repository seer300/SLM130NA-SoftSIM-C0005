#include "sema.h"
#include "mpu_protect.h"
#include "flash_adapt.h"
#include "flash_vendor.h"
#include "factory_nv.h"
#include "xy_log.h"
#include "hw_memmap.h"
#include "xy_memmap.h"
#include "hw_utc.h"
#include "cmsis_os2.h"
#include "ipc_msg.h"
#include "dma.h"
#include "common.h"
#include "xy_flash.h"
#include "xy_fota.h"

QSPI_FLASH_Def xinyi_flash = {.regbase = (unsigned char *)QSPI_BASE,
								.ahbbase = (unsigned char *)QSPI_DATA_BASE,
								.page_size = 256,
								.block_size = 16,
								.tshsl_ns = 60, // CS# High Time (read/write), >= 20ns
								.tsd2d_ns = 12,
								.tchsh_ns = 12,  // CS# Active Hold Time, >= 5ns
								.tslch_ns = 12,  // CS# Active Setup Time, >= 5ns
								.flash_type = FLASH_GD25Q16, //FLASH_UNKNOWN;  FLASH_GD25Q32
								.addr_bytes = 3,
								.otp_base = OTP_BASE_ADDR_GD25Q16};


osThreadId_t g_flash_TskHandle = NULL;
osMutexId_t g_flashdma_mutex = NULL; //flash底层读写接口使用的dma通道互斥锁，读写互斥
volatile flash_status g_flash_status = {0};
osMessageQueueId_t g_flash_MsgQueue;
ListHeader_t flash_list = {0};
extern uint8_t GetForbidFlash(void);

void flash_acquire_mutex(osMutexId_t mutex_id, uint32_t timeout)
{
	if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING && osCoreGetState() == osCoreNormal)
	{
		osMutexAcquire(mutex_id, timeout);
	}
}

void flash_release_mutex(osMutexId_t mutex_id)
{
	if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING && osCoreGetState() == osCoreNormal)
	{
		osMutexRelease(mutex_id);
	}
}

/*CP核内尚未开启OS调度时调用，严禁业务人员调用，while其实多余了，外部已经做了while动作*/
__FLASH_FUNC int flash_write_erase(uint32_t addr, uint8_t *data,uint32_t size)
{
	uint32_t offset;	
	uint32_t write_size;
	uint32_t actAddr;
	uint8_t *block_buf = NULL;

	block_buf = xy_malloc(EFTL_PAGE_SIZE);

	while (size > 0) 
	{
		offset = addr & 0xfff;
		actAddr = addr & 0xfffff000;

		if (size <= (EFTL_PAGE_SIZE - offset)) 
		{
			write_size = size;
		} 
		else 
		{
			write_size = EFTL_PAGE_SIZE - offset;
		}

		if(write_size == EFTL_PAGE_SIZE)
		{
			flash_erase(actAddr, EFTL_PAGE_SIZE);
			flash_write_no_erase(actAddr, data, EFTL_PAGE_SIZE);
		}
		else
		{

			flash_read(actAddr, block_buf, EFTL_PAGE_SIZE);

			if(write_size<100 && !memcmp(block_buf + offset, data, write_size))
				goto CONTINUE;
			
			memcpy(block_buf + offset, data, write_size);
			flash_erase(actAddr, EFTL_PAGE_SIZE);
			flash_write_no_erase(actAddr, block_buf, EFTL_PAGE_SIZE);
		}
	CONTINUE:

		size -= write_size;
		addr += write_size;
		data += write_size;
	}

	xy_free(block_buf);

	return 0;
}

/*CP核内尚未开启OS调度时调用，严禁业务人员调用*/
int flash_write_no_erase(uint32_t addr, uint8_t *data,uint32_t size)
{	
	FLASH_WriteEnable();

	FLASH_WriteData(&xinyi_flash, (unsigned long)data, addr, size, FLASH_DMA_CHANNEL, MEMORY_TYPE_CP);
	FLASH_WaitIdle();
	
	if (HWREGB(BAK_MEM_XY_DUMP) == 1)
	{
		xy_assert(memcmp((uint8_t*)addr, data, size) == 0);
	}

	return 0;
}

/*擦除addr所覆盖的所有扇区*/
int flash_erase(uint32_t addr, uint32_t size)
{
	uint32_t page=0;
	uint32_t sum_page;

	sum_page = (uint32_t)((size + (addr&0xfff) + 0xfff)/EFTL_PAGE_SIZE);
	addr &= 0xFFFFF000;
	
	while (page < sum_page) {	
	
		FLASH_SectorErase(addr+page*EFTL_PAGE_SIZE);
		FLASH_WaitIdle();

		if (HWREGB(BAK_MEM_XY_DUMP) == 1)
		{
			for(uint32_t i = 0; i < EFTL_PAGE_SIZE/4; i++)
			{
				xy_assert(*(uint32_t*)(addr+page*EFTL_PAGE_SIZE + 4*i) == 0xffffffff);
			}
		}

		page++;
	}

	return 0;
}

void QSPI_WaitIdle(void)
{
	volatile uint8_t flash_reg1 = 0;

	while(1) 
	{
		delay_func_us(100);
		
		osCoreEnterCritical();
#if RUNTIME_DEBUG
		extern uint32_t xy_runtime_get_enter(void);
		uint32_t time_enter = xy_runtime_get_enter();
#endif

		qspi_rbuf_wait_idle();
		qspi_wbuf_wait_idle();
		qspi_wait_idle();

		flash_reg1 = FLASH_GetStatusReg1();

#if RUNTIME_DEBUG
	 	extern void xy_runtime_get_exit(uint32_t id, uint32_t time_enter);
		xy_runtime_get_exit(48, time_enter);
#endif

		osCoreExitCritical();

		// wip == 0
		if((flash_reg1 & 0x01) == 0x00)
		{
			break;
		}
	}
}

/*flash代理线程执行动作，严禁业务人员调用*/
__FLASH_FUNC int flashTsk_write_erase(uint32_t addr, uint8_t *data,uint32_t size)
{
	xy_assert(IS_FLASH_ALIGNED(addr) && size==FLASH_SECTOR_LENGTH);
	
	flashTsk_erase(addr, size);
	flashTsk_write_no_erase(addr, data, size);

	return 0;
}

/*flash代理线程执行动作，严禁业务人员调用*/
__FLASH_FUNC int flashTsk_write_no_erase(uint32_t addr, uint8_t *data, uint32_t size)
{
	uint32_t i=0;
	uint32_t burstcount=0;
	uint32_t lastburstsize=0;
	uint8_t *srcDataBuf = NULL;
    //uint8_t flash_cmd[4] = {0};
    //uint8_t flash_cmd_rw = 0;
    uint8_t addr4Align = 1;
    
	if (addr%4 != 0) 
	{
		uint32_t shiftsize = addr & 3;
		srcDataBuf = xy_malloc(size + 4);
	
		uint8_t *tempBUF = srcDataBuf;
		uint8_t tempData[4] = {0};
		
		addr &= 0xFFFFFFFC;

		memcpy((void *)tempData, (void *)addr, 4);
		
		for (i=0; i<shiftsize; i++) {
			*tempBUF++ = tempData[i];
		}

		flash_acquire_mutex(g_flashdma_mutex, osWaitForever);

		DMAChannelTransfer(FLASH_DMA_CHANNEL, (uint32_t)data, (uint32_t)tempBUF, size, MEMORY_TYPE_CP);

		flash_release_mutex(g_flashdma_mutex);

		data = srcDataBuf;

		size += shiftsize;
        addr4Align = 0;
	}

	while(DMAChannelTransferRemainCNT(FLASH_DMA_CHANNEL) !=  0x00);

    burstcount=(size+63) >> 6;
    lastburstsize=(size & 0x3f);

	for(i=0; i<burstcount; i++) {

		// 优化代码，防止关中断之后，GetForbidFlash()为1，然后还执行flash写动作
		while (1)
		{
			while(GetForbidFlash());/*1200Bx系列芯片，由于CS 模块的dma功能和Flash 的dma功能存在同时发生的场景，会导致Cs的dma完成中断被严重退后，最终导致Cs模块软硬件控制时序错乱，此接口将禁止在 cs coarse场景下触发写Flash动作*/

			flash_acquire_mutex(g_flashdma_mutex, osWaitForever);
			osCoreEnterCritical();

			if (GetForbidFlash()) 
			{
				osCoreExitCritical();
				flash_release_mutex(g_flashdma_mutex);
			}
			else
			{
				break;
			}
		}
		
#if RUNTIME_DEBUG
		extern uint32_t xy_runtime_get_enter(void);
		uint32_t time_enter = xy_runtime_get_enter();
#endif

		FLASH_WriteEnable();

		g_flash_status.pe_status = flash_Programming;

		if(lastburstsize == 0 || (i != (burstcount-1))) 
		{
			FLASH_WriteData(&xinyi_flash, (uint32_t)(data+i*BURSTSIZE), addr+i*BURSTSIZE, BURSTSIZE, FLASH_DMA_CHANNEL, MEMORY_TYPE_CP);
		} 
		else 
		{
			FLASH_WriteData(&xinyi_flash, (uint32_t)(data+i*BURSTSIZE), addr+i*BURSTSIZE, lastburstsize, FLASH_DMA_CHANNEL, MEMORY_TYPE_CP);
		}
		
		g_flash_status.pe_status = flash_ProgramDone;


#if RUNTIME_DEBUG
	 	extern void xy_runtime_get_exit(uint32_t id, uint32_t time_enter);
		xy_runtime_get_exit(46, time_enter);
#endif

		osCoreExitCritical();
		
		flash_release_mutex(g_flashdma_mutex);



	}
	
	if (srcDataBuf != NULL) xy_free(srcDataBuf);
	

	if (HWREGB(BAK_MEM_XY_DUMP) == 1)
	{
		xy_assert(memcmp((uint8_t*)addr, data, size) == 0);
	}

	return 0;
}


/*flash代理线程执行动作，严禁业务人员调用，while其实多余了，外部已经做了while动作*/
int flashTsk_erase(uint32_t addr, uint32_t size)
{
	uint32_t page=0;
	addr &= 0xFFFFF000;
	size = (size + 0xFFF) & 0xFFFFF000;
	unsigned char flash_cmd;
	unsigned char flash_cmd_rw;


	while (page < size/EFTL_PAGE_SIZE) {

		osCoreEnterCritical();

#if RUNTIME_DEBUG
		extern uint32_t xy_runtime_get_enter(void);
		uint32_t time_enter = xy_runtime_get_enter();
#endif

		g_flash_status.pe_status = flash_Erasing;

		FLASH_WaitIdle();

		/*flash操作时，退出XIP模式，此时容许相应中断函数，但中断函数内部不得运行flash上的代码*/
		Flash_mpu_Lock();

		FLASH_SectorErase(addr+page*EFTL_PAGE_SIZE);

		flash_cmd = FLASH_CMD_READ_STATUS_REG1;
		flash_cmd_rw = 0x00;
		qspi_apb_command_read(1, &flash_cmd, 1, &flash_cmd_rw);

		while((flash_cmd_rw & STATUS_REG1_WIP) == 0x00) {
				qspi_apb_command_read(1, &flash_cmd, 1, &flash_cmd_rw);
		    }

#if RUNTIME_DEBUG
	 	extern void xy_runtime_get_exit(uint32_t id, uint32_t time_enter);
		xy_runtime_get_exit(47, time_enter);
#endif

		osCoreExitCritical();

		//FLASH_WaitIdle();
		QSPI_WaitIdle();

		osCoreEnterCritical();
		//此函数非原子，可能会与pendsv中函数产生race condition
		Flash_mpu_Unlock();

		g_flash_status.pe_status = flash_EraseDone;

		osCoreExitCritical();

		if (HWREGB(BAK_MEM_XY_DUMP) == 1)
		{
			for(uint32_t i = 0; i < EFTL_PAGE_SIZE/4; i++)
			{
				xy_assert(*(uint32_t*)(addr+page*EFTL_PAGE_SIZE + 4*i) == 0xffffffff);
			}
		}

		page++;
	}

	return 0;
}

__FLASH_FUNC void notify_flash_do(uint32_t destAddr, uint32_t srcAddr, uint32_t size, uint32_t operation, uint8_t asyn_flag, osSemaphoreId_t flashwrite_msg_sem)
{
	flash_PE_st *pflashPE = xy_malloc(sizeof(flash_PE_st));
	pflashPE->destAddr = destAddr;
	pflashPE->srcAddr = (uint8_t *)srcAddr;
	pflashPE->size = size;
	pflashPE->operation = operation;
	pflashPE->coretype = common_core;
	if(asyn_flag == 0)
	{
		pflashPE->aysn_flag = sync;
		pflashPE->msg_sem = flashwrite_msg_sem;
	}
	else
		pflashPE->aysn_flag = async;

	osMessageQueuePut(g_flash_MsgQueue, &pflashPE, 0, osWaitForever);
}

extern void flash_write_protect(uint32_t base_addr,uint32_t size);
/*AP核操作flash，通过核间消息通知CP核代理线程执行*/
__FLASH_FUNC void ipc_flash_process(char * msg)
{
	flash_PE_st *pflashPE = xy_malloc(sizeof(flash_PE_st));

	flash_PE_st *tmp = (flash_PE_st *)msg;
	
	if (tmp == NULL || pflashPE == NULL)
		xy_assert(0);
	
	((flash_op_sync*)(BAK_MEM_FLASH_DYN_SYNC)) -> src_addr = (uint32_t)(tmp->srcAddr);
	((flash_op_sync*)(BAK_MEM_FLASH_DYN_SYNC)) -> operation = tmp->operation;
	((flash_op_sync*)(BAK_MEM_FLASH_DYN_SYNC)) -> is_done = 0;

	pflashPE->coretype = diff_core;
	pflashPE->destAddr = (uint32_t)Address_Translation_AP_To_CP(tmp->destAddr);
	pflashPE->srcAddr = (uint8_t *)Address_Translation_AP_To_CP((uint32_t)(tmp->srcAddr));
	pflashPE->operation = tmp->operation;
	pflashPE->size = tmp->size;

	osMessageQueuePut(g_flash_MsgQueue, &pflashPE, 0, osWaitForever); 
}

extern int g_protect_enable_always;
__FLASH_FUNC void flash_task_operation(flash_PE_st *pflashPE)
{
	volatile uint32_t val = 0;
	static int en_now = 1;

	if(pflashPE->operation == flash_op_read)
	{
		flash_read((uint32_t)(pflashPE->destAddr), (uint8_t *)(pflashPE->srcAddr),pflashPE->size);
	}
	else if(pflashPE->operation == flash_op_read_UniqueID128)
	{
		flash_read_UniqueID128((uint8_t *)(pflashPE->srcAddr));
	}
	else if(pflashPE->operation == flash_op_read_fhpstatus)
	{
		flash_hardware_protect_status((uint32_t *)(pflashPE->srcAddr));
	}
	else
	{
#if 0		/*正常工作态，除了FOTA+调试命令容许写IMEI/FAC，其它禁止写FLASH*/
		if(BAN_WRITE_FLASH() && pflashPE->destAddr!=NV_MAIN_FACTORY_BASE && pflashPE->destAddr!=NV_FLASH_RF_BASE && pflashPE->destAddr!=NV_FLASH_RF_BAKUP_BASE)
			xy_assert(0);
#endif			
		g_flash_status.isDone = flash_PES_busy;
		
		/*只要是通过xy_flash.h接口进行操作的，都临时容许进行前半段的擦写*/
		if(pflashPE->destAddr < FLASH_HARDWARE_PROTECT_LIMIT)
		{
			if(!(pflashPE->destAddr == NV_FLASH_RF_BASE 
				|| pflashPE->destAddr == NV_FLASH_RF_BAKUP_BASE 
				|| pflashPE->destAddr == FOTA_BREAKPOINT_INFO_ADDR 
				||(pflashPE->destAddr >= SECONDBOOT_PRIME_HEADER_BASE && pflashPE->destAddr < (SECONDBOOT_BACKUP_HEADER_BASE + 0x1000))
				|| pflashPE->destAddr == NV_MAIN_FACTORY_BASE 
				|| pflashPE->destAddr >= OTA_FLASH_BASE()))
			{
				xy_assert(0);
			}
			
			/*擦写前半部分FLASH，临时关闭FLASH禁止写硬保护*/
			if(en_now == 1)
			{
				val = flash_hardware_protect_disable();
				en_now = 0;
			}
		}


		if(pflashPE->operation == flash_op_erase) {
			flashTsk_erase(pflashPE->destAddr, pflashPE->size);
		} else if (pflashPE->operation == flash_op_writeNoErase) {
			flashTsk_write_no_erase(pflashPE->destAddr, pflashPE->srcAddr, pflashPE->size);
		} else if (pflashPE->operation == flash_op_writeErase) {
			flashTsk_write_erase(pflashPE->destAddr, pflashPE->srcAddr, pflashPE->size);
		} else {
			xy_assert(0);
		}

		extern uint32_t assert_flash;
		extern uint8_t assert_val;
		/*仅用于调试，排查FLASH部分值被异常修改*/
		if(assert_flash>=pflashPE->destAddr && assert_flash<pflashPE->destAddr+pflashPE->size)
		{
			if(pflashPE->operation == flash_op_erase)
				xy_assert(assert_val == 0XFF);
			else
				xy_assert(*(pflashPE->srcAddr+(assert_flash-pflashPE->destAddr)) == assert_val);
		}
		
		/*恢复前半部分的FLASH写硬保护，但FOTA升级不恢复，因为要进行代码段升级*/
		if(pflashPE->destAddr < FLASH_HARDWARE_PROTECT_LIMIT)
		{
			if(!(pflashPE->destAddr == NV_FLASH_RF_BASE 
				|| pflashPE->destAddr == NV_FLASH_RF_BAKUP_BASE 
				|| pflashPE->destAddr == FOTA_BREAKPOINT_INFO_ADDR 
				||(pflashPE->destAddr >= SECONDBOOT_PRIME_HEADER_BASE && pflashPE->destAddr < (SECONDBOOT_BACKUP_HEADER_BASE + 0x1000))
				|| pflashPE->destAddr == NV_MAIN_FACTORY_BASE 
				|| pflashPE->destAddr >= OTA_FLASH_BASE()))
			{
				xy_assert(0);
			}
				
			/*仅FOTA升级时设为0，不得恢复FLASH写的硬保护*/
			if(g_protect_enable_always == 1)
			{
				flash_hardware_protect_enable(val);
				en_now = 1;
			}
		}

		g_flash_status.isDone = flash_PES_done;
	}
}

void List_Insert(fList_t* temp, ListHeader_t* flash_list)
{
	fList_t* temp1 = (fList_t*)flash_list->tail;

	if(temp1 != NULL)
	{
		temp1->next = temp;
	}
	else
	{
		flash_list->head = temp;
	}

	flash_list->tail = temp;
}

void List_Insert_Head(fList_t* temp, ListHeader_t* flash_list)
{
	fList_t* temp1 = (fList_t*)flash_list->head;

	if(temp1 != NULL)
	{
		temp->next = flash_list->head;
		flash_list->head = temp;
	}
	else
	{
		flash_list->head = temp;
		flash_list->tail = temp;
	}
}

fList_t* List_Remove(ListHeader_t* flash_list)
{
	fList_t* temp = flash_list->head;

	if(temp != NULL)
	{
		flash_list->head = temp->next;
		if(flash_list->head == NULL)
		{
			flash_list->tail = NULL;
		}
	}

	return temp;
}

fList_t* List_Get_Item(ListHeader_t* flash_list)
{
	return flash_list->head;
}

void flash_insert(flash_PE_st *pflashPE)
{
	flash_List* item = xy_malloc(sizeof(flash_List));
	item->flashPE = pflashPE;
	item->next = NULL;
	List_Insert((fList_t*)item, &flash_list); //插入链表,后续处理消息全部从这个链表中读取
}

void flash_insert_head(flash_PE_st *pflashPE)
{
	flash_List* item = xy_malloc(sizeof(flash_List));
	item->flashPE = pflashPE;
	item->next = NULL;
	List_Insert_Head((fList_t*)item, &flash_list); //ap核发起读写请求，将消息插入链表头
}

void flash_remove(void)
{
	flash_List* item = (flash_List*)List_Remove(&flash_list); //将消息从链表上移除
	if(item)
	{
		xy_free(item->flashPE);
		xy_free(item);
	}
}

flash_PE_st* flash_get_item(void)
{
	flash_PE_st* temp = NULL;
	flash_List* item = (flash_List*)List_Get_Item(&flash_list);
	if(item)
	{
		temp = item->flashPE;
	}

	return temp;
}

__FLASH_FUNC void flash_task_entry(void)
{
	volatile flash_PE_st *pflashPE = NULL;
	volatile flash_notice_t* flash_status = (flash_notice_t*)BAK_MEM_FLASH_NOTICE;
	volatile uint8_t flag = 1;
	//flash_List* item;

	while(1) 
	{
		osMessageQueueGet(g_flash_MsgQueue, &pflashPE, 0, osWaitForever); 

	    PrintLog(0,PLATFORM,WARN_LOG,"[FLASH_LOG]flash task start: coretype %d operation %d aysn_flag %d destAddr %08x srcAddr %08x size %x", pflashPE->coretype, pflashPE->operation,
						pflashPE->aysn_flag,pflashPE->destAddr, pflashPE->srcAddr, pflashPE->size);

		//cp写flash时若此时ap在锁中断写flash,会造成双核卡死，当前的解决方法是优先处理ap的读写flash请求
		flash_insert((flash_PE_st *)pflashPE); //插入链表,后续处理消息全部从这个链表中读取

		while((pflashPE = flash_get_item()) != NULL)
		{
			if(common_core == pflashPE->coretype) //CP核写flash
			{
				while(1)
				{
					switch(flash_status->cp_status)
					{
						case cp_status_idle:
							flash_status->cp_status = cp_status_write_ready;  //cp_status准备写flash
							shm_msg_write((void *)osThreadGetName(osThreadGetId()), 8, ICM_FLASHWRT_NOTICE); //通知ap挂起，写入线程名为了查问题方便
						case cp_status_write_ready:
							while(flash_status->ap_status == ap_status_run) //循环等待ap核进入suspend态
							{
								/*等待ap核进入suspend态的过程中，非阻塞从队列上读取消息，如果发现此时有ap的擦写请求时，
								将消息插入链表头，并退出此次cp写，如果是cp核擦写请求，插入链表即可*/
								volatile flash_PE_st *temp = NULL;
								osMessageQueueGet(g_flash_MsgQueue, &temp, 0, osNoWait);
								if(temp != NULL)
								{
									if(diff_core == temp->coretype)
									{
										flash_insert_head((flash_PE_st *)temp); //ap核发起读写请求，将消息插入链表头
										flag = 1;
										break;
									}
									else
									{
										flash_insert((flash_PE_st *)temp); //cp核擦写请求，插入链表
									}
								}
							}

							if(flag) //进入此分支，表明cp擦写被打断，此状态机能保持被打断cp擦写时cp_status的状态
							{
								flag = 0;
								break;
							}

							flash_status->cp_status = cp_status_write;
						case cp_status_write:
							flash_task_operation((flash_PE_st *)pflashPE);
							flash_status->cp_status = cp_status_writedone;
						case cp_status_writedone:
							while(flash_status->ap_status == ap_status_sus) //循环等待ap核退出suspend态
							{
								/*等待ap核进入suspend态的过程中，非阻塞从队列上读取消息，如果发现此时有ap的擦写请求时，
								将消息插入链表头，并退出此次cp写，如果是cp核擦写请求，插入链表即可*/
								volatile flash_PE_st *temp = NULL;
								osMessageQueueGet(g_flash_MsgQueue, &temp, 0, osNoWait);
								if(temp != NULL)
								{
									if(diff_core == temp->coretype)
									{
										flash_insert_head((flash_PE_st *)temp); //ap核发起读写请求，将消息插入链表头
										flag = 1;
										break;
									}
									else
									{
										flash_insert((flash_PE_st *)temp); //cp核擦写请求，插入链表
									}
								}
							}

							if(flag) //进入此分支，表明cp擦写被打断，此状态机能保持被打断cp擦写时cp_status的状态
							{
								flag = 0;
								break;
							}

							flash_status->cp_status = cp_status_idle; //一次擦写结束

						    PrintLog(0,PLATFORM,WARN_LOG,"[FLASH_LOG]flash task   end: coretype %d operation %d aysn_flag %d destAddr %08x srcAddr %08x size %x", pflashPE->coretype, pflashPE->operation,
											pflashPE->aysn_flag,pflashPE->destAddr, pflashPE->srcAddr, pflashPE->size);

							osCoreEnterCritical();

							if(pflashPE->operation == flash_op_writeErase && pflashPE->aysn_flag == async)
								xy_free(pflashPE->srcAddr);
							if(pflashPE->aysn_flag == sync)
							{
								xy_assert(pflashPE->msg_sem != NULL);
								osSemaphoreRelease(pflashPE->msg_sem);
							}

											
							flash_remove();

							osCoreExitCritical();
							break;
						default:
							xy_assert(0);
							break;
					}

					//cp_status_idle表示一次擦写结束，cp_status_write_ready和cp_status_writedone表示处理过程中ap发起读写请求，需要退出此次cp擦写，
					if(cp_status_write_ready == flash_status->cp_status
						|| cp_status_writedone == flash_status->cp_status
						|| cp_status_idle == flash_status->cp_status)
					{
						break;
					}
				}
			}
			else if(diff_core == pflashPE->coretype) //AP核写flash
			{
				flash_task_operation((flash_PE_st *)pflashPE);

				PrintLog(0,PLATFORM,WARN_LOG,"[FLASH_LOG]flash task   end: coretype %d operation %d aysn_flag %d destAddr %08x srcAddr %08x size %x", pflashPE->coretype, pflashPE->operation,
								pflashPE->aysn_flag,pflashPE->destAddr, pflashPE->srcAddr, pflashPE->size);


				osCoreEnterCritical();

				((flash_op_sync*)(BAK_MEM_FLASH_DYN_SYNC)) -> is_done = 1; //通知ap退出suspend状态

								
				flash_remove();

				osCoreExitCritical();
			}
			else
			{
				xy_assert(0);
			}
		}
	}
}


__FLASH_FUNC void flash_task_init(void)
{
	osThreadAttr_t thread_attr = {0};
	//osThreadId_t temp_TskHandle = NULL;

	g_flashdma_mutex = osMutexNew(NULL);
	/* create queue */
	g_flash_MsgQueue = osMessageQueueNew(4, sizeof(flash_PE_st*), NULL);
	
	thread_attr.name = "flash_dyn";
	thread_attr.priority = osPriorityBelowNormal;
	thread_attr.stack_size = osStackShared;
	g_flash_TskHandle = osThreadNew((osThreadFunc_t)(flash_task_entry), NULL, &thread_attr);
	//孙泉删除，待确认
	//osThreadSetLowPowerFlag(g_flash_TskHandle, osLpmNoRealtime);

}

int flash_read(uint32_t addr, uint8_t *data, uint32_t size)
{
	flash_acquire_mutex(g_flashdma_mutex, osWaitForever);
	FLASH_ReadData(&xinyi_flash, addr, (unsigned long)data, size, FLASH_DMA_CHANNEL, MEMORY_TYPE_CP);

	FLASH_WaitIdle();
	flash_release_mutex(g_flashdma_mutex);

	return 0;
}


int flash_read_UniqueID128( uint8_t *data)
{
	flash_acquire_mutex(g_flashdma_mutex, osWaitForever);

	//FLASH_GetUniqueID128会切换读指令，这时候不能跑flash代码
	osCoreEnterCritical();
	FLASH_GetUniqueID128(&xinyi_flash, data, MEMORY_TYPE_CP);
	osCoreExitCritical();

	flash_release_mutex(g_flashdma_mutex);

	return 0;
}

extern char *xTaskGetCurrentTaskName( void );



void flashTsk_check(void)
{
    char * ptaskname = NULL;

    ptaskname = xTaskGetCurrentTaskName();
    
	if (strcmp(ptaskname, "flash_dyn") == 0)
	{

		qspi_rbuf_wait_idle();
		qspi_wbuf_wait_idle();
		qspi_wait_idle();

		// sus == 1, need to resume
		if(FLASH_need_Resume(&xinyi_flash))
		{
			FLASH_PE_Resume();
			delay_func_us(50);

            while(FLASH_need_Resume(&xinyi_flash)) {  
                FLASH_PE_Resume();
                delay_func_us(50);
            }
			/*等待flash擦除操作完成过程中调用，此时容许响应中断函数，但中断函数内部不得运行flash上的代码和读写flash*/
            Flash_mpu_Lock();
		}
	}
	else
	{
		// wip == 1, need to suspend
		if((FLASH_GetStatusReg1() & 0x01) == 0x01)
		{

			FLASH_PE_Suspend();

			delay_func_us(100);
			
			while((FLASH_GetStatusReg1() & 0x01) == 0x01);  //wait WIP==0

			if((FLASH_GetStatusReg2() & 0x84) == 0) //check SUS1 SUS2
        	{  
				FLASH_PE_Suspend();
				delay_func_us(100);
            }

			qspi_rbuf_wait_idle();
			qspi_wbuf_wait_idle();
			qspi_wait_idle();
		}

		/*等待flash擦除操作完成过程中调用，此时容许响应中断函数，但中断函数内部不得运行flash上的代码和读写flash*/
		Flash_mpu_Unlock();

	}
}


