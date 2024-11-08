#include "flash_vendor.h"
#include "xy_flash.h"
#include "uart.h"
#include "sema.h"
#include "system.h"
#include "hal_def.h"
#include "hw_prcm.h"
#include "prcm.h"
#include "utc.h"
#include "sys_mem.h"
#include "xy_memmap.h"
#include "xy_cp.h"
#include "sys_ipc.h"
#include "qspi_flash.h"
#include "flash_vendor.h"
#include "mpu_protect.h"
#include <string.h>
#include <stdbool.h>
#include "common.h"
#include "mpu.h"
#include "xy_system.h"
#include "at_uart.h"
#include "xy_fota.h"
#include "sys_proc.h"

#define FLASH_OTP_INFO_VALID            0x55AA55AA
#define OTP_SYSTEM_FLAG_REG_SET         0x00000080


/*CP核擦写FLASH，需要挂起AP核，由于3GPP抢占，可能耗时很久，此处设为2分钟*/
#define WAIT_FLASH_DONE_MS (2*60*1000)


/**
 * @brief  flash操作锁结构体
 */
typedef enum
{
  FLASH_UNLOCKED = 0x00U,
  FLASH_LOCKED = 0x01U
} FLASH_LockTypeDef;

typedef struct __FLASH_HandleTypeDef
{
	FLASH_LockTypeDef Lock; /*!< flash操作锁   */
} FLASH_HandleTypeDef;

typedef enum
{
	flash_op_default = 0,
	flash_op_read,
	flash_op_erase,
	flash_op_writeNoErase,
	flash_op_writeErase
} flash_operation;

typedef struct
{
	uint32_t destAddr;
	uint8_t *srcAddr;
	uint32_t size;
	uint32_t operation : 3;
	uint32_t coretype : 1;
	uint32_t last_tran : 1;
	uint32_t reserved : 27;
} flash_PE_st;


//用于ap通知cp写flash
typedef struct
{
	uint32_t src_addr; // flash操作源地址
	uint8_t operation; // flash操作类型   1为读操作，2 3 4为写操作
	uint8_t is_done;   //操作是否执行完成 0为未完成，1为完成
} flash_op_sync;

typedef enum
{
	cp_status_idle,
	cp_status_write_ready,
	cp_status_write,
	cp_status_writedone,
} cp_status_t;

typedef enum
{
	ap_status_run,
	ap_status_sus,
	ap_status_sus_in_cri,
} ap_status_t;

//用于cp写flash时通知ap挂起
typedef struct
{
	uint8_t  reserve;  //
	uint8_t  ap_write_status;  //
	uint8_t  ap_status; // ap状态  0:idle 1:挂起
	uint8_t  cp_status; // cp状态  0:idle 1:write done
} flash_notice_t;

#define MAGIC_NUM         0x12345678
#define IMAGE_NUM         ( 12 )
#define MAX_FILE_NAME     ( 32 )
#define MAX_TIME_INFO     ( 64 )
#define BB_REG_LEN     	  ( 0x300 )
#define DMAC_REG_LEN      ( 0x300 )
#define COREPRCM_REG_LEN  ( 0x100 )
#define AONPRCM_REG_LEN   ( 0x100 )

// External Flash First 256 bytes
typedef struct {
	uint32_t flash_addr;
	uint32_t ram_addr;
	uint32_t len;
	char file_name[MAX_FILE_NAME];
}image_info_t;

// External Flash First 256 bytes
typedef struct {
	uint32_t is_time_right;
	char current_time[MAX_TIME_INFO];
}dump_time_info_t;

typedef struct {
	// Header Info
	uint32_t magic_num;
	uint32_t image_num;
	uint32_t assert_core;
	// Image Info
	image_info_t image_info[IMAGE_NUM];
	
	uint32_t head_addr;
	uint32_t addr;
	uint32_t remain_len;

	dump_time_info_t time_info;
} Flash_Header;

QSPI_FLASH_Def xinyi_flash = {.regbase = (unsigned char *)QSPI_BASE,
							  .ahbbase = (unsigned char *)QSPI_DATA_BASE,
							  .page_size = 256,
							  .block_size = 16,
							  .tshsl_ns = 60, // CS# High Time (read/write), >= 20ns
							  .tsd2d_ns = 12,
							  .tchsh_ns = 12,			   // CS# Active Hold Time, >= 5ns
							  .tslch_ns = 12,			   // CS# Active Setup Time, >= 5ns
							  .flash_type = FLASH_GD25Q16, // FLASH_UNKNOWN;  FLASH_GD25Q32
							  .addr_bytes = 3,
							  .otp_base = OTP_BASE_ADDR_GD25Q16};

#define IS_FLASH_ALIGNED(a) ((((unsigned int)a) & (FLASH_SECTOR_LENGTH - 1)) == 0)



#define __FLASH_LOCK(__HANDLE__)                                           \
								do										   \
								{                                          \
									DisablePrimask();                      \
									if((__HANDLE__)->Lock == FLASH_LOCKED) \
									{                                      \
										EnablePrimask();                   \
										return 1;                          \
									}                                      \
									else                                   \
									{                                      \
									    (__HANDLE__)->Lock = FLASH_LOCKED; \
									}                                      \
									EnablePrimask();                       \
								}while (0U)

#define __FLASH_UNLOCK(__HANDLE__)                                         \
							   do										   \
								{                                          \
								    (__HANDLE__)->Lock = FLASH_UNLOCKED;   \
								}while (0U)

FLASH_HandleTypeDef FLASHHandleStruct = {HAL_UNLOCKED};

extern int g_errno;

// 切换flashvcc(IOLDO2)为NORMAL模式
void flashvcc2normal(void)
{
    PRCM_IOLDO2_ModeCtl(IOLDO2_NORMAL_Enable);
}

void flash_read(uint32_t addr, void *data, uint32_t size)
{
	FLASH_ReadData(&xinyi_flash, addr, (uint32_t)data, size, FLASH_DMA_CHANNEL, MEMORY_TYPE_AP);
	FLASH_WaitIdle();
}

/*擦除地址所在的所有扇区*/
void flash_erase(uint32_t addr, uint32_t size)
{
	int page = 0;
	int sum_page = 0;

	sum_page = (size + (addr&(FLASH_SECTOR_LENGTH-1U)) + (FLASH_SECTOR_LENGTH-1U))/FLASH_SECTOR_LENGTH;
	addr &= ~(FLASH_SECTOR_LENGTH-1U);
	
	while (page < sum_page)
	{
		/*RF NV cannot erase*/
		if(addr+(uint32_t)(page*FLASH_SECTOR_LENGTH) >= NV_FLASH_RF_BASE && addr+(uint32_t)(page*FLASH_SECTOR_LENGTH) < NV_FLASH_RF_BASE+0x2000)
			xy_assert(0);
	
		FLASH_SectorErase(addr+(uint32_t)(page*FLASH_SECTOR_LENGTH));
		FLASH_WaitIdle();

		page++;
	}
}

void flash_write_no_erase(uint32_t addr, uint8_t *data,uint32_t size)
{
	extern uint32_t assert_flash;
	extern uint8_t assert_val;
	
	/*对FLASH中某单字节进行特殊值断言排查，用于FLASH值异常的问题定位*/
	if(assert_flash>=addr && assert_flash<addr+size)
		xy_assert(assert_val == *(data+(assert_flash-addr)));
	
	FLASH_WriteEnable();
	FLASH_WriteData(&xinyi_flash, (uint32_t)data, addr, size, FLASH_DMA_CHANNEL, MEMORY_TYPE_AP);
	FLASH_WaitIdle();
}

bool flash_write_erase(uint32_t addr, uint8_t *data, uint32_t size)
{
	uint32_t offset;
	uint32_t write_size;
	uint32_t actAddr;
	uint8_t *block_buf = NULL;

	while (size > 0) {
		offset = addr & 0xfff;
		actAddr = addr & 0xfffff000;

		if (size <= (FLASH_SECTOR_LENGTH - offset)) {
			write_size = size;
		} else {
			write_size = FLASH_SECTOR_LENGTH - offset;
		}

		if(write_size == FLASH_SECTOR_LENGTH)
		{
			flash_erase(actAddr, FLASH_SECTOR_LENGTH);
			flash_write_no_erase(actAddr,data, FLASH_SECTOR_LENGTH);
		}
		else
		{
			block_buf = xy_malloc2(FLASH_SECTOR_LENGTH);
			/*申请不到内存，放弃当前写flash动作*/
			if(block_buf == NULL)
			{
				xy_printf("[xy_malloc2]: malloc fail %s,%d\r\n",__FILE__,__LINE__);
				return false;
			}
			
			flash_read(actAddr, block_buf, FLASH_SECTOR_LENGTH);
			memcpy(block_buf + offset, data, write_size);
			flash_erase(actAddr, FLASH_SECTOR_LENGTH);
			flash_write_no_erase(actAddr, block_buf, FLASH_SECTOR_LENGTH);
			xy_free(block_buf);
		}
		size -= write_size;
		addr += write_size;
		data += write_size;
	}
	return true;
}


/*AP发送核间消息给CP，以通知CP执行擦写FLASH，并超时等待完成指示*/
bool wait_flash_is_done(IPC_Message pMsg)
{
	bool ret = true;
	volatile uint8_t done_flag = 0;
	uint32_t start_tick;

#if BAN_WRITE_FLASH
	DisablePrimask();
#endif		
	// must wait CP core have inited
	if(IPC_WriteMessage(&pMsg) < 0)
	{
		ret = false;
	}
	else
	{
		start_tick = Get_Tick();
		do
		{
			done_flag = ((volatile flash_op_sync *)(BAK_MEM_FLASH_DYN_SYNC))->is_done;
			if (Check_Ms_Timeout(start_tick, WAIT_FLASH_DONE_MS))
			{
#if MODULE_VER || XY_DUMP
				xy_assert(0);
#else
				ret = false;
				g_errno = XY_ERR_CP_DEAD;
				break;
#endif
			}
		} while (done_flag != 1); // wait for CP enterXIP
	}

#if BAN_WRITE_FLASH
	EnablePrimask();
#endif	

	return ret;
}

__FLASH_FUNC bool send_flash_msg(int operation, int last_tran, uint32_t ramAddr, uint32_t flashAddr, uint32_t size)
{
	volatile flash_notice_t* flash_notice = (flash_notice_t*)BAK_MEM_FLASH_NOTICE;
	bool ret = true;


	if(__get_BASEPRI() || __get_PRIMASK())
	{
		flash_notice->ap_write_status = ap_status_sus_in_cri;
	}
	else
	{
		flash_notice->ap_write_status = ap_status_sus;
	}

	((flash_op_sync *)(BAK_MEM_FLASH_DYN_SYNC))->is_done = 0;

	flash_PE_st flashPE = {0};
	flashPE.destAddr = (uint32_t)flashAddr;

	if (operation != flash_op_erase)
	{
		flashPE.srcAddr = (uint8_t *)ramAddr;
	}
	
	flashPE.size = size;
	flashPE.operation = (uint32_t)operation & ((1 << 3) - 1);
	flashPE.last_tran = last_tran;

	IPC_Message pMsg = {ICM_FLASHWRITE, &flashPE, sizeof(flash_PE_st)};

	ret = wait_flash_is_done(pMsg);
	
	flash_notice->ap_write_status = ap_status_run;

	return ret;
}

/*CP核执行写flash时，在此函数中等待CP核写flash完成*/
void wait_cp_flash_write_done(void)
{
	uint32_t start_tick = 0;
	volatile flash_notice_t* flash_notice = (flash_notice_t*)BAK_MEM_FLASH_NOTICE;
	
	if(flash_notice->cp_status != cp_status_write_ready && flash_notice->cp_status != cp_status_write)
		return;
/*由于无法保证OPENCPU二次开发时中断函数中不运行FLASH代码，此处关中断*/
#if (MODULE_VER==0)
	DisablePrimask();
#endif
	flash_notice->ap_status = ap_status_sus; //表示ap当前已经挂起

	start_tick = Get_Tick();
	//等待CPflash操作结束，若20s内无法结束，则认为CP异常，不再死等
	while(flash_notice->cp_status == cp_status_write_ready || flash_notice->cp_status == cp_status_write)
	{
		if (Check_Ms_Timeout(start_tick, WAIT_FLASH_DONE_MS)) 
		{
#if MODULE_VER || XY_DUMP
			xy_assert(0);
#else
			g_errno = XY_ERR_CP_DEAD;
			break;
#endif
		}
	}

	flash_notice->ap_status = ap_status_run; //表示ap当前已经可以运行flash代码
	
#if (MODULE_VER==0)
	EnablePrimask();
#endif
}

__FLASH_FUNC bool flash_operate_ext(int operation, uint32_t flash_addr, void *ram_data, uint32_t size)
{
	uint32_t offset = 0;
	uint32_t write_size = 0;
	uint32_t actAddr = 0;
	uint8_t *block_buf = NULL;
	bool ret = true;

	if(operation==flash_op_read)
	{
		ret = send_flash_msg(operation, 1, (uint32_t)(ram_data), flash_addr, size);
		return ret;
	}

	while (size > 0)
	{
		offset = flash_addr & 0xfff;
		actAddr = flash_addr & 0xfffff000;

		if (size <= (uint32_t)(FLASH_SECTOR_LENGTH - offset))
		{
			write_size = size;
		}
		else
		{
			write_size = FLASH_SECTOR_LENGTH - offset;
		}

		size -= write_size;

		if (operation == flash_op_writeErase)
		{
			if(write_size != FLASH_SECTOR_LENGTH)
			{
				block_buf = xy_malloc2(FLASH_SECTOR_LENGTH);

				if(block_buf == NULL)
				{
					xy_printf("[xy_malloc2]: malloc fail %s,%d\r\n",__FILE__,__LINE__);
					return false;
				}
				ret = send_flash_msg(flash_op_read, 0, (uint32_t)block_buf, actAddr, FLASH_SECTOR_LENGTH);
				memcpy(block_buf + offset, ram_data, write_size);

				ret = send_flash_msg(operation, (size > 0 ? 0 : 1), (uint32_t)(block_buf), actAddr, FLASH_SECTOR_LENGTH);
				xy_free(block_buf);
			}
			else
			{
				ret = send_flash_msg(operation, (size > 0 ? 0 : 1), (uint32_t)(ram_data), actAddr, FLASH_SECTOR_LENGTH);
			}

		}
		else if (operation == flash_op_writeNoErase)
		{
			ret = send_flash_msg(operation, (size > 0 ? 0 : 1), (uint32_t)(ram_data), actAddr + offset, write_size);
		}
		else if (operation == flash_op_erase)
		{
			ret = send_flash_msg(operation, (size > 0 ? 0 : 1), (uint32_t)(NULL), actAddr, FLASH_SECTOR_LENGTH);
		}

		flash_addr += write_size;
		ram_data = (char *)ram_data + write_size;
	}

	return ret;
}

/*指示是否为AP核容许进行寄存器操作的合法flash地址，目前仅FOTA区域向后的区域容许擦写。其他的内存保护参见Mpu_Protect_Init接口*/
bool is_allowed_opt_addr(uint32_t flash_addr, uint32_t data_addr)
{
	(void)data_addr;
	if(!(flash_addr >= HWREG(BAK_MEM_FOTA_FLASH_BASE) && flash_addr < (FLASH_BASE + FLASH_LENGTH)))
	{
		xy_assert(0);
		return false;
	}

	return true;
}

void dump_to_flash_erase(uint32_t addr, uint32_t size)
{
	if (is_allowed_opt_addr(addr, 0) == false)
	{
		return;
	}

	flash_erase(addr, size);
}

void dump_to_flash_write_no_erase(uint32_t addr, void *data, uint32_t size)
{
	if (is_allowed_opt_addr(addr, (uint32_t)data) == false)
	{
		return;
	}

	flash_write_no_erase(addr,data,size);
}

__FLASH_FUNC static void gen_image_info(Flash_Header* dump_flash, uint32_t ram_addr, uint32_t len, char* name)
{
	if(dump_flash->remain_len >= len)
	{
		image_info_t* image_info;

		image_info = &dump_flash->image_info[dump_flash->image_num++];
		image_info->len = len;
		image_info->flash_addr = (dump_flash->addr -= image_info->len);
		image_info->ram_addr = ram_addr; 
		snprintf(image_info->file_name, MAX_FILE_NAME, "%s", name);

		dump_flash->remain_len -= len;
	}
	else
	{
#if (AT_LPUART == 1)
		char dump_str[80] = {0};
		snprintf(dump_str,sizeof(dump_str)-1,"[dump flash]: %s fail %lx %lx\r\n", name, len, dump_flash->remain_len);
		at_uart_write_fifo(dump_str,strlen(dump_str));
#endif
	}	
}

extern void DumpRegister_from_Normal(void);
/*用于CP核工作异常时将死机现场保存到flash(FOTA备份区)中，以供死机定位*/
__FLASH_FUNC uint32_t dump_into_to_flash_if(void)
{
	/* 只dump一次，防止dump中assert */
	static uint8_t dump_done = 0;
	if(dump_done)
	{
		return 0;
	}
	dump_done = 1;

	/*xy_Flash_FastRecovery_Init快速恢复下未开DMA，不能写flash，此处再次初始化，开启DMA*/
	xy_Flash_Init();

	extern uint8_t g_dump_mem_into_flash;
	if(g_dump_mem_into_flash != 1)
		return 0;

	DisablePrimask();

	DumpRegister_from_Normal();
	/*保存flash是从FOTA备份区的尾部向前存放的，与导出工具配套*/
	/*计算FOTA备份区域的总长度，确保dump不越界*/
	Flash_Header dump_flash = {.magic_num = MAGIC_NUM, .image_num = 0, .assert_core = g_dump_core, .head_addr = DUMP_INFO_BASE_ADDR, .addr = DUMP_INFO_BASE_ADDR,
		.remain_len = DUMP_INFO_BASE_ADDR - HWREG(BAK_MEM_FOTA_FLASH_BASE), .time_info = {0 , {0}}};
	image_info_t* image_info;
	uint8_t dma_reg[DMAC_REG_LEN];
    RTC_TimeTypeDef rtctime = {0};

	if (Get_Current_UT_Time(&rtctime))
	{
		snprintf(dump_flash.time_info.current_time, MAX_TIME_INFO - 2, "\r\n+TIME:%02ld/%02lu/%02lu,%02lu:%02lu:%02lu\r\n", rtctime.tm_year, rtctime.tm_mon, rtctime.tm_mday, rtctime.tm_hour, rtctime.tm_min, rtctime.tm_sec);
		dump_flash.time_info.is_time_right = 1;
	}

	if(g_dump_core)
	{
		//cpram空间数据保存
		if(((AONPRCM->SMEM_SLPCTRL & CP_SRAM0_SLPCTL_Msk) >> CP_SRAM0_SLPCTL_Pos) != SRAM_POWER_MODE_FORCE_OFF
				&&((AONPRCM->SMEM_SLPCTRL & CP_SRAM1_SLPCTL_Msk) >> CP_SRAM1_SLPCTL_Pos) != SRAM_POWER_MODE_FORCE_OFF)
		{
			gen_image_info(&dump_flash, RAM1_DMA_BASE, TCM_CP_SIZE, "cp_assert_ram_10000000");

		}
		else
		{
#if (AT_LPUART == 1)
			snprintf((char*)dma_reg,sizeof(dma_reg)-1,"[dump flash]: cpram off\r\n");
			at_uart_write_fifo((char*)dma_reg, strlen((char*)dma_reg));
#endif
		}

		//retmem空间数据保存
		gen_image_info(&dump_flash, SHARE_RAM0_BASE, (BAK_MEM_LENGTH + (SHARE_RAM1_BASE - SHARE_RAM0_BASE)), "shared_ram_60000000");

		//apram空间数据保存
		gen_image_info(&dump_flash, SRAM_BASE, SRAM_LENGTH, "ap_ram_01000000");

	}
	else
	{
		//apram空间数据保存
		gen_image_info(&dump_flash, SRAM_BASE, SRAM_LENGTH, "ap_assert_ram_01000000");

		//cpram空间数据保存
		if(((AONPRCM->SMEM_SLPCTRL & CP_SRAM0_SLPCTL_Msk) >> CP_SRAM0_SLPCTL_Pos) != SRAM_POWER_MODE_FORCE_OFF
			&&((AONPRCM->SMEM_SLPCTRL & CP_SRAM1_SLPCTL_Msk) >> CP_SRAM1_SLPCTL_Pos) != SRAM_POWER_MODE_FORCE_OFF)
		{
			gen_image_info(&dump_flash, RAM1_DMA_BASE, TCM_CP_SIZE, "cp_ram_10000000");
		}
		else
		{
#if (AT_LPUART == 1)
			snprintf((char*)dma_reg,sizeof(dma_reg)-1,"[dump flash]: cpram off\r\n");
			at_uart_write_fifo((char*)dma_reg, strlen((char*)dma_reg));
#endif
		}

		//retmem空间数据保存
		gen_image_info(&dump_flash, SHARE_RAM0_BASE, (BAK_MEM_LENGTH + (SHARE_RAM1_BASE - SHARE_RAM0_BASE)), "shared_ram_60000000");
	}

	//coreprcm寄存器保存
	gen_image_info(&dump_flash, COREPRCM_BASE, COREPRCM_REG_LEN, "coreprcm_reg_0x40004000");
	//aon寄存器保存
	gen_image_info(&dump_flash, AONPRCM_BASE, AONPRCM_REG_LEN, "aonprcm_reg_0x40000000");
	//dma寄存器保存
	gen_image_info(&dump_flash, DMAC_BASE, DMAC_REG_LEN, "dmac_reg_0x40050000");
	//bb射频寄存器保存，射频分析dump会用到
	if((COREPRCM->PWRCTL_CFG & 0x20) == 0)
	{
		gen_image_info(&dump_flash, BB_REG_BASE, BB_REG_LEN, "bb_reg_0x4001b000");
	}
	else
	{
#if (AT_LPUART == 1)
		snprintf((char*)dma_reg,sizeof(dma_reg)-1,"[dump flash]: dma off\r\n");
		at_uart_write_fifo((char*)dma_reg, strlen((char*)dma_reg));
#endif
	}
	
#if (AT_LPUART == 1)
	snprintf((char*)dma_reg,sizeof(dma_reg)-1,"[dump flash]: write flash ...\r\n");
	at_uart_write_fifo((char*)dma_reg, strlen((char*)dma_reg));
#endif

	for(uint32_t i = 0; i < dump_flash.image_num; i++)
	{
		if(i == 0)
		{
			image_info = &dump_flash.image_info[dump_flash.image_num - 1];
			dump_to_flash_erase(image_info->flash_addr, dump_flash.head_addr - image_info->flash_addr);
		}

		image_info = &dump_flash.image_info[i];
		if(image_info->ram_addr == DMAC_BASE)
		{
			memcpy(dma_reg,(uint8_t *)image_info->ram_addr, image_info->len);
			dump_to_flash_write_no_erase(image_info->flash_addr, dma_reg, image_info->len);
		}
		else
		{
			dump_to_flash_write_no_erase(image_info->flash_addr, (uint8_t *)image_info->ram_addr, image_info->len);
		}
	}

	dump_to_flash_erase(dump_flash.head_addr, sizeof(dump_flash));
	dump_to_flash_write_no_erase(dump_flash.head_addr, (uint8_t *)&dump_flash, sizeof(dump_flash));

#if (AT_LPUART == 1)
	snprintf((char*)dma_reg,sizeof(dma_reg)-1,"[dump flash]: write flash done\r\n");
	at_uart_write_fifo((char*)dma_reg, strlen((char*)dma_reg));
#endif
    //EnablePrimask();

	return 1;
}

extern uint8_t is_FlashLPMode(void);
uint32_t dump_into_to_flash(void)
{
	uint32_t ret = 0;

	/*通过读取NV_FLASH_FACTORY_BASE头部4字节是否为5A来识别flash是否正常,若异常不执行写flash动作*/
	if(HWREG(NV_FLASH_FACTORY_BASE) == 0xffffffff || HWREG(NV_FLASH_FACTORY_BASE) == 0x5a5a5a5a)
	{
		ret = dump_into_to_flash_if();
	}
	else if(is_FlashLPMode())
	{
		FlashExitLPMode();

        //擦写flash前，防止flash供电不稳，将IOLDO2切换为normal模式
        flashvcc2normal();
		ret = dump_into_to_flash_if();
	}

	return ret;
}

/*指示是否执行过flash初始化，快速恢复唤醒后需设为0，因为此时只能XIP运行，但未开DMA，进而不能读写flash*/
volatile int g_Flash_Have_Init = 0;
void xy_Flash_Init(void)
{
	FlashExitLPMode();
	if(g_Flash_Have_Init == 1)
		return;
	
	PRCM_ClockEnable(CORE_CKG_CTL_DMAC_EN);
	PRCM_ClockEnable(CORE_CKG_CTL_QSPI_EN);

    qspi_wait_idle();
	// if(QSPI_WRITE_QUAD != HWREG(QSPI_REG_WR_INSTR_DEV0 + (QSPI_DEV0<<2))
    //     || QSPI_READ_QUAD != HWREG(QSPI_REG_RD_INSTR_DEV0 + (QSPI_DEV0<<2)))
	{
		FLASH_Init((QSPI_FLASH_Def *)&xinyi_flash, 38400000, 9600000);

        if(FLASH_isXIPMode())
        {
            FLASH_ExitXIPMode((QSPI_FLASH_Def *)&xinyi_flash);
            FLASH_WaitIdle();
        }

		FLASH_EnableQEFlag((QSPI_FLASH_Def *)&xinyi_flash);
		FLASH_WaitIdle();

		FLASH_SetReadMode(QSPI_DEV0, QSPI_READ_QUAD);
		FLASH_SetWriteMode(QSPI_DEV0, QSPI_WRITE_QUAD);
		FLASH_WaitIdle();
	}

	/*保护前1M不容许擦写，通常为NV和代码段等，如果误写，不报错，但擦写无效。常见于异常下电时防止写flash地址跳变*/
#if FLASH_2M
	FLASH_SetProtectMode(FLASH_2M_PROTECT_MODE_13, PROTECT_CMP_0);
#else
	FLASH_SetProtectMode(FLASH_4M_PROTECT_MODE_5, PROTECT_CMP_1);
#endif

	g_Flash_Have_Init = 1;
}

/*快速恢复后，开启XIP模式。但不能调用xy_flash.h中接口，因为DMA未开启，6.5M主频下耗时约170us*/
void xy_Flash_FastRecovery_Init(void)
{
	//PRCM_ClockEnable(CORE_CKG_CTL_QSPI_EN);	//在first_excute_in_reset_handler中打开时钟

	/* FLASH_Init */
    HWREG(QSPI_BASE + 0x00) = 0x80010040;
	HWREG(QSPI_BASE + 0x04) = QSPI_DELAY_DEFAULT;

    // loopback mode  
    HWREG(QSPI_BASE + 0x08) = 0x401;

	HWREG(QSPI_BASE + 0x14) = QSPI_READ_QUAD;
	HWREG(QSPI_BASE + 0x24) = QSPI_WRITE_QUAD;

	HWREG(QSPI_REG_CLK_CTRL) = 0;//disable force_clk_ena for power saving

	// qspi enable
	HWREG(QSPI_BASE + 0x00) |= 0x1;

    //先初始化qspi div，提高速度
    FlashExitLPMode_NoDelay();
}

extern void flash_off_and_on();
static uint32_t flash_reset(void)
{
	flash_off_and_on();
	return  1;
}

bool xy_Flash_Read(uint32_t addr, void *data, uint32_t size)
{
	bool ret = true;

	//中断中禁止操作flash 防止ap核读写flash嵌套,底层dma共抢冲突.
	xy_assert(!IS_IRQ_MODE());

	xy_Flash_Init();

	if(!((addr >= FLASH_BASE) && (addr <= FLASH_BASE+FLASH_LENGTH)))
	{
		return false;
	}

	if( CP_Is_Alive() == false || (g_errno == XY_ERR_CP_DEAD && flash_reset()) || (CP_IS_DEEPSLEEP()==true && IS_IRQ_MASKED()))
	{
#if BAN_WRITE_FLASH
		DisablePrimask();  
#endif
		flash_read(addr, data, size);
#if BAN_WRITE_FLASH
		EnablePrimask();
#endif
	}
	else
	{
		ret = flash_operate_ext(flash_op_read,addr,data,size);
	}

	return ret;
}


bool xy_Flash_Erase(uint32_t addr, uint32_t size)
{
	bool ret = true;

	/*耗时过久，不宜在中断函数中执行*/
	xy_assert(!IS_IRQ_MODE());
	
#if BAN_WRITE_FLASH
	/*CP核正常工作期间，AP写FLASH需要由CP核执行，与宏配置冲突，进而断言*/
	xy_assert(g_dump_core!=-1 || CP_Is_Alive() == false || (g_errno == XY_ERR_CP_DEAD && flash_reset()) || (CP_IS_DEEPSLEEP()==true && IS_IRQ_MASKED()));
#endif

	xy_Flash_Init();

	if (is_allowed_opt_addr(addr, 0) == false)
	{
		return false;
	}

	if( CP_Is_Alive() == false || (g_errno == XY_ERR_CP_DEAD && flash_reset()) || (CP_IS_DEEPSLEEP()==true && IS_IRQ_MASKED()))
	{
#if BAN_WRITE_FLASH
		DisablePrimask();  
#endif
        //擦写flash前，防止flash供电不稳，将IOLDO2切换为normal模式
        flashvcc2normal();
		flash_erase(addr,size);
#if BAN_WRITE_FLASH
		EnablePrimask();
#endif
	}
	else
	{
		ret = flash_operate_ext(flash_op_erase, addr, NULL, size);
	}

	return ret;
}

bool xy_Flash_Write(uint32_t addr, void *data, uint32_t size)
{
	bool ret = true;

	/*耗时过久，不宜在中断函数中执行*/
	xy_assert(!IS_IRQ_MODE());
	
#if BAN_WRITE_FLASH
	/*CP核正常工作期间，AP写FLASH需要由CP核执行，与宏配置冲突，进而断言*/
	xy_assert(g_dump_core!=-1 || CP_Is_Alive() == false || (g_errno == XY_ERR_CP_DEAD && flash_reset()) || (CP_IS_DEEPSLEEP()==true && IS_IRQ_MASKED()));
#endif


	xy_Flash_Init();

	if (is_allowed_opt_addr(addr, (uint32_t)data) == false)
	{
		return false;
	}

	if( CP_Is_Alive() == false || (g_errno == XY_ERR_CP_DEAD && flash_reset()) || (CP_IS_DEEPSLEEP()==true && IS_IRQ_MASKED()))
	{
#if BAN_WRITE_FLASH
		DisablePrimask();  
#endif
        //擦写flash前，防止flash供电不稳，将IOLDO2切换为normal模式
        flashvcc2normal();
		ret = flash_write_erase(addr,data,size);
#if BAN_WRITE_FLASH
		EnablePrimask();
#endif
	}
	else
	{
		ret = flash_operate_ext(flash_op_writeErase, addr, data, size);
	}

	return ret;
}

bool xy_Flash_Write_No_Erase(uint32_t addr, void *data, uint32_t size)
{
	bool ret = true;

	/*耗时过久，不宜在中断函数中执行*/
	xy_assert(!IS_IRQ_MODE());
	
#if BAN_WRITE_FLASH
	/*CP核正常工作期间，AP写FLASH需要由CP核执行，与宏配置冲突，进而断言*/
	xy_assert(g_dump_core!=-1 || CP_Is_Alive() == false || (g_errno == XY_ERR_CP_DEAD && flash_reset()) || (CP_IS_DEEPSLEEP()==true && IS_IRQ_MASKED()));
#endif

	xy_Flash_Init();

	if (is_allowed_opt_addr(addr, (uint32_t)data) == false)
	{
		return false;
	}

	if( CP_Is_Alive() == false || (g_errno == XY_ERR_CP_DEAD && flash_reset()) || (CP_IS_DEEPSLEEP()==true && IS_IRQ_MASKED()))
	{
#if BAN_WRITE_FLASH
		DisablePrimask();
#endif
        //擦写flash前，防止flash供电不稳，将IOLDO2切换为normal模式
        flashvcc2normal();
		flash_write_no_erase(addr,data,size);
#if BAN_WRITE_FLASH
		EnablePrimask();
#endif
	}
	else
	{
		ret = flash_operate_ext(flash_op_writeNoErase, addr, data, size);
	}

	return ret;
}

