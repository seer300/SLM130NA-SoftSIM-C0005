#pragma once

#include "flash_vendor.h"
#include "cmsis_os2.h"

typedef struct{
	uint32_t pe_status:3;
	uint32_t sus_status:2;
	uint32_t isDone:2;
	uint32_t isNeedtoExitIdle:1;
	uint32_t reserved:24;
}flash_status;

typedef enum{
	flash_op_default=0,
	flash_op_read,
	flash_op_erase,
	flash_op_writeNoErase,
	flash_op_writeErase,
	flash_op_read_UniqueID128,
	flash_op_read_fhpstatus,
}flash_operation;

enum async_flag{
	sync = 0,
	async,
};

typedef struct{
	uint32_t destAddr;  //flash地址
	uint8_t *srcAddr;   //RAM地址
	uint32_t size;
	uint32_t operation:3;
	uint32_t coretype:1;
	uint32_t aysn_flag:1;  //0:同步写；1：异步写
	uint32_t reserved:27;
	osSemaphoreId_t msg_sem;
}flash_PE_st;

typedef enum{
	flash_PE_default = 0,
	flash_Erasing,
	flash_EraseDone,
	flash_Programming,
	flash_ProgramDone
}flash_PEstatus;

typedef enum{
	flash_PES_default = 0,
	flash_PES_done,
	flash_PES_busy,
	flash_PES_max
}flash_PES;

typedef struct{
	uint32_t src_addr; //flash操作源地址
	uint8_t operation; //flash操作类型
	uint8_t is_done;   //是否执行完成
}flash_op_sync;


enum core_type {
	diff_core = 0,
	common_core,
};

typedef enum{
	cp_status_idle,
	cp_status_write_ready,
	cp_status_write,
	cp_status_writedone,
}cp_status_t;

typedef enum{
	ap_status_run,
	ap_status_sus,
}ap_status_t;

typedef struct{
	uint8_t  reserve;
	uint8_t  ap_write_status;
	uint8_t  ap_status;
	uint8_t  cp_status;
}flash_notice_t;

typedef struct fList_tt
{
	struct fList_tt* next;
}fList_t;

typedef struct
{
	fList_t* next;
	flash_PE_st* flashPE;
}flash_List;

typedef struct
{
	fList_t* head;
	fList_t* tail;
}ListHeader_t;

extern osMutexId_t g_flashwrite_msg_m;

#define BURSTSIZE (64)
#define EFTL_PAGE_SIZE         	(0x1000)
#define IS_FLASH_ALIGNED(a)     ((((uint32_t)a)&(EFTL_PAGE_SIZE-1))==0)

/*FOTA的flash起始头部地址，与AP大版本复用一块FLASH区域*/
#define OTA_FLASH_BASE()   (HWREG(BAK_MEM_FOTA_FLASH_BASE) - FLASH_CP_BASE)


extern volatile flash_status g_flash_status;
extern QSPI_FLASH_Def xinyi_flash;

void List_Insert(fList_t* temp, ListHeader_t* flash_list);
fList_t* List_Remove(ListHeader_t* flash_list);

int flash_read(uint32_t addr, uint8_t *data,uint32_t size);
int flash_read_UniqueID128( uint8_t *data);
int flash_write_erase(uint32_t addr, uint8_t *data,uint32_t size);
int flash_write_no_erase(uint32_t addr, uint8_t *data,uint32_t size);
int flash_erase(uint32_t addr, uint32_t size);
void flash_task_init(void);
void ipc_flash_process(char * msg);

int flashTsk_write_no_erase(uint32_t addr, uint8_t *data, uint32_t size);
int flashTsk_erase(uint32_t addr, uint32_t size);
void notify_flash_do(uint32_t destAddr, uint32_t srcAddr, uint32_t size, uint32_t operation, uint8_t asyn_flag, osSemaphoreId_t flashwrite_msg_sem);
void flashTsk_check(void);

void flash_acquire_mutex(osMutexId_t mutex_id, uint32_t timeout);
void flash_release_mutex(osMutexId_t mutex_id);
