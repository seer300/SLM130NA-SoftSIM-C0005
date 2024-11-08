#include "xy_flash.h"
#include "xy_utils.h"
#include "xy_at_api.h"
#include "flash_adapt.h"
#include "oss_nv.h"
#include "xy_memmap.h"
#include "ipc_msg.h"
#include "mpu_protect.h"
#include "lpm_nv_write.h"

#define EFTL_MAGIC_NUM    0X5A5A5A5A
#define EFTL_PAGE_OFFSET_ADDR(ADDR,INDEX,OFFSET) (*((uint32_t *)(ADDR+FLASH_SECTOR_LENGTH*INDEX+OFFSET)))
#define EFTL_CSM_OFFSET             (FLASH_SECTOR_LENGTH-sizeof(struct eftl_tail_tag))  //0XFF0
#define EFTL_WRITE_NUM_OFFSET       (FLASH_SECTOR_LENGTH-sizeof(struct eftl_tail_tag)+4)  //0XFF4

#define RF_NV_VALID     0xE88


struct eftl_tail_tag {
	uint32_t checksum;
	uint32_t write_num;   
};

struct eftl_cfg {
	uint32_t addr;      //FTL块的flash基地址
	uint32_t tpage_total;//FTL块flash总字节长度，必须是FLASH_SECTOR_LENGTH整数倍
};


struct xy_eftl_cfg {
	struct xy_eftl_cfg *next;
	uint32_t addr;//flash start addr
	uint32_t tpage_total;//true pages,such as 5
};

struct xy_eftl_cfg *user_nv_cfgs = NULL;
osMutexId_t ftl_mutex;


uint8_t chk_xrl(const void*data,uint32_t length,const uint32_t step)
{
	uint8_t ret = 0;
	int   val = 0;
	int *tmp_ptr = (int *)data;
	unsigned char *tail;

	UNUSED_ARG(step);

	xy_assert(((int)data%4) == 0);
	while((length/4)>0)
	{
		val ^= *tmp_ptr;
		tmp_ptr+=1;
		length-=4;
	}
	ret ^= *((unsigned char *)&val);
	ret ^= *((unsigned char *)&val+1);
	ret ^= *((unsigned char *)&val+2);
	ret ^= *((unsigned char *)&val+3);

	tail = (unsigned char *)tmp_ptr;
	while((int)tail < (int)data+(int)length)
	{
		ret ^= *tail;
		tail++;
	}
	
	//if(ret==0 || ret==0XFF)
	//	ret = 1;
	
	return ret+10;
}


struct xy_eftl_cfg *find_valid_ftl_cfg(uint32_t addr)
{
	struct xy_eftl_cfg *user_nv_cfgs_temp = user_nv_cfgs;
	while(user_nv_cfgs_temp != NULL)
	{
		if(user_nv_cfgs_temp->addr == addr)
		{
			return user_nv_cfgs_temp;
		}
		user_nv_cfgs_temp = user_nv_cfgs_temp->next;
	}
	return NULL;
}


int ftl_read_write_num(uint32_t addr)
{
	struct xy_eftl_cfg *nv_cfgs_temp = NULL;
	uint32_t addr_aligned = addr - (addr&(FLASH_SECTOR_LENGTH-1));

	nv_cfgs_temp = find_valid_ftl_cfg(addr);
	if(nv_cfgs_temp == NULL)
		return -1;

	return (int)EFTL_PAGE_OFFSET_ADDR(addr_aligned,0,EFTL_WRITE_NUM_OFFSET);
}


bool xy_choose_valid_ftl(uint32_t addr, uint8_t *data, int *p_cur_ftl_idx, int *p_next_ftl_idx)
{
	int valid_page;
	uint32_t max_writenum = 0;
	uint32_t i = 0;
	struct xy_eftl_cfg *nv_cfgs_temp = NULL;
	char *str = NULL;
	uint32_t write_num;
	uint32_t errPage_bitmap = 0;		//此处限制最大32个磨损page

	nv_cfgs_temp = find_valid_ftl_cfg(addr);

	xy_assert(nv_cfgs_temp != NULL);

CHOOSE_AGAIN:

	valid_page = -1;
	max_writenum = 0;

	for(i = 0; i<nv_cfgs_temp->tpage_total; i++)
	{
		write_num = EFTL_PAGE_OFFSET_ADDR(addr,i,EFTL_WRITE_NUM_OFFSET);
		if(EFTL_PAGE_OFFSET_ADDR(addr,i,0) == EFTL_MAGIC_NUM 
			&& write_num != 0XFFFFFFFF 
			&& write_num != 0
			&& (errPage_bitmap&(1<<i)) == 0)
		{
			if(max_writenum <= write_num)
			{
				max_writenum = write_num;
				valid_page = i;
			}
		}
	}

	if(valid_page != -1)
	{
		xy_Flash_Read(nv_cfgs_temp->addr+FLASH_SECTOR_LENGTH*valid_page, data, FLASH_SECTOR_LENGTH);
		
		/*由于checksum计算耗时，深睡唤醒场景不校验checksum。甚至完全不校验也问题不大*/
		if(addr != CALIB_FREQ_BASE && !Is_WakeUp_From_Dsleep() && *((uint32_t *)(data+EFTL_CSM_OFFSET)) != xy_chksum((void*)data,EFTL_CSM_OFFSET))
		{
			str = xy_malloc(64);
			sprintf(str,"+DBGINFO:checksum err %lX \r\n", nv_cfgs_temp->addr+FLASH_SECTOR_LENGTH*valid_page);
			send_debug_by_at_uart(str);
			xy_free(str);

			/*RF NV checksum失败，大概率是FLASH驱动偶现异常。不进行擦除动作，直接返回或断言*/
			if(addr >= NV_FLASH_RF_BASE && addr < NV_FLASH_RF_BAKUP_BASE+NV_FLASH_RF_SIZE)
			{
				*p_cur_ftl_idx = 0;
				*p_next_ftl_idx = 0;
				//xy_assert(0);
				return 1;
			}	
			
			//位图记录校验失败的page,继续遍历
			errPage_bitmap |= (1<<valid_page);
			goto CHOOSE_AGAIN;
		}

		*p_cur_ftl_idx = valid_page;
		if(p_next_ftl_idx != NULL)
			*p_next_ftl_idx = (valid_page+1) % nv_cfgs_temp->tpage_total;
	}
	else
	{
NO_FIND:
		*p_cur_ftl_idx = -1;
		if(p_next_ftl_idx != NULL)
			*p_next_ftl_idx = 0;
	}

	return 1;
}
void nv_read_fail_dbg(uint32_t addr)
{
	char  *dbg_info = xy_malloc(50);
	
	if(addr>=NV_FLASH_FACTORY_BASE && addr<NV_FLASH_FACTORY_BASE+NV_FLASH_FACTORY_LEN)
		sprintf(dbg_info,"+DBGINFO:factory nv read fail!\r\n");
	
	else if(addr>=NV_NON_VOLATILE_BASE && addr<NV_NON_VOLATILE_BASE+NV_NON_VOLATILE_LEN)
		sprintf(dbg_info,"+DBGINFO:PS invar nv read fail!\r\n");
	
	else if(addr>=BAK_MEM_FLASH_BASE && addr<BAK_MEM_FLASH_BASE+BAK_MEM_FLASH_TOTAL_LEN)
			sprintf(dbg_info,"+DBGINFO:bakmem var nv read fail!\r\n");
	
	else if(addr>=NV_FLASH_RF_BASE && addr<NV_FLASH_RF_BASE+NV_FLASH_RF_SIZE*2)
			sprintf(dbg_info,"+DBGINFO:RF nv read fail!\r\n");

	else
		sprintf(dbg_info,"+DBGINFO:user nv %lX read fail!\r\n",addr);

	send_debug_by_at_uart(dbg_info);

	xy_free(dbg_info);
}

bool xy_ftl_read(uint32_t addr, uint8_t *data, uint32_t size)
{
	uint32_t offset = addr&(FLASH_SECTOR_LENGTH-1);
	uint32_t addr_align = addr - offset;
	uint8_t *temp = NULL;
    uint32_t rf_base_nv_vld= 0;
    uint32_t rf_bake_base_nv_vld = 0;
	int next_ftl_idx = -1;
	int cur_ftl_idx = -1;
	
	/*OPENCPU形态，出厂NV和非易变全局，位于深睡保持供电的RAM区，唤醒/stop_cp/非FOTA的软复位 时无需读FLASH*/
	if((NOT_ALLOWED_SAVE_FLASH()) && ((addr >= NV_FLASH_FACTORY_BASE && addr < NV_FLASH_FACTORY_BASE+NV_FLASH_FACTORY_LEN) || (addr >= NV_NON_VOLATILE_BASE && addr < NV_NON_VOLATILE_BASE+NV_NON_VOLATILE_LEN)))
	{
		if(Get_Boot_Reason()==WAKEUP_DSLEEP || (Get_Boot_Reason()==SOFT_RESET && Get_Boot_Sub_Reason()!=SOFT_RB_BY_FOTA) || ((Get_Boot_Reason()==POWER_ON)&&(Get_Boot_Sub_Reason()==1)))
			return 1;
	}
	
	xy_assert(size <= XY_FTL_AVAILABLE_SIZE);
	xy_assert((addr+size) <= (addr_align+EFTL_CSM_OFFSET)); 

	flash_acquire_mutex(ftl_mutex, osWaitForever);

	temp = xy_malloc(FLASH_SECTOR_LENGTH);

	xy_choose_valid_ftl(addr_align,temp,&cur_ftl_idx, &next_ftl_idx);

	if(cur_ftl_idx == -1)
	{
		if(osKernelGetState() >= osKernelRunning && osKernelIsRunningIdle()!= osOK && HWREGB(BAK_MEM_XY_DUMP) == 1)
		{
			PrintLog(0,PLATFORM,WARN_LOG,"[FLASH_LOG]xy_ftl_read find ftl fail: %x %d %s %d", addr, size, osThreadGetName(osThreadGetId()), Get_Boot_Sub_Reason());
			nv_read_fail_dbg(addr);
		}
		
		/* first poweron, use main factory nv */
		if(addr_align==NV_FLASH_FACTORY_BASE)
		{		
			/*模组形态深睡唤醒下工作态出厂NV一定有效*/
			if(Get_Boot_Reason() == WAKEUP_DSLEEP && !Is_OpenCpu_Ver())
				xy_assert(0);
			
			//发现工作态出厂NV无效，立即更新一份
			xy_Flash_Read(NV_MAIN_FACTORY_BASE,temp,sizeof(factory_nv_t));
			memcpy(data,temp+offset, size);

			xy_ftl_write(NV_FLASH_FACTORY_BASE,temp, sizeof(factory_nv_t));

			xy_free(temp);
			flash_release_mutex(ftl_mutex);
			return 1;
		}
		/*黄雪兵定制，无需平台维护*/
		else if (addr_align==NV_FLASH_RF_BASE || addr_align==NV_FLASH_RF_BAKUP_BASE)
		{ 
		    xy_Flash_Read(addr_align,temp,NV_FLASH_RF_SIZE-12);
    		memcpy(data,temp+offset, size);
            
            xy_Flash_Read(NV_FLASH_RF_BASE+RF_NV_VALID + 4,&rf_base_nv_vld,4);
            xy_Flash_Read(NV_FLASH_RF_BAKUP_BASE+RF_NV_VALID + 4,&rf_bake_base_nv_vld,4);
            
            if((0xFFFFFFFF == rf_base_nv_vld)&&(0xFFFFFFFF == rf_bake_base_nv_vld))
            {
    			xy_ftl_write(NV_FLASH_RF_BASE,temp, NV_FLASH_RF_SIZE - 12);
    			xy_ftl_write(NV_FLASH_RF_BAKUP_BASE,temp, NV_FLASH_RF_SIZE - 12);
                rf_base_nv_vld = 0x0;
                rf_bake_base_nv_vld = 0x0;
                xy_ftl_write(NV_FLASH_RF_BASE+RF_NV_VALID,&rf_base_nv_vld,4);
                xy_ftl_write(NV_FLASH_RF_BAKUP_BASE+RF_NV_VALID,&rf_bake_base_nv_vld,4);
            }
			xy_free(temp);
			flash_release_mutex(ftl_mutex);
			return 1;
		}
		else
		{			
			xy_free(temp);
			flash_release_mutex(ftl_mutex);
			
			/*模组形态深睡唤醒下，读NV一定成功*/
			if(HWREGB(BAK_MEM_XY_DUMP) == 1 && Get_Boot_Reason() == WAKEUP_DSLEEP && !Is_OpenCpu_Ver())
				xy_assert(0);
			
			return 0;
		}
	}

	memcpy(data,temp+offset+4,size);
	xy_free(temp);
	flash_release_mutex(ftl_mutex);
	return 1;
}

void nv_write_dbg_info(uint32_t addr,uint32_t write_num)
{
	char  *dbg_info = xy_malloc(50);
	
	if(addr>=NV_FLASH_FACTORY_BASE && addr<NV_FLASH_FACTORY_BASE+NV_FLASH_FACTORY_LEN)
		sprintf(dbg_info,"+DBGINFO:factory nv write %ld nums!\r\n",write_num);
	
	else if(addr>=NV_NON_VOLATILE_BASE && addr<NV_NON_VOLATILE_BASE+NV_NON_VOLATILE_LEN)
		sprintf(dbg_info,"+DBGINFO:PS invar nv write %ld nums!\r\n",write_num);
	
	else if(addr>=BAK_MEM_FLASH_BASE && addr<BAK_MEM_FLASH_BASE+BAK_MEM_FLASH_TOTAL_LEN)
			sprintf(dbg_info,"+DBGINFO:bakmem var nv write %ld nums!\r\n",write_num);
	
	else if(addr>=NV_FLASH_RF_BASE && addr<NV_FLASH_RF_BASE+NV_FLASH_RF_SIZE*2)
			sprintf(dbg_info,"+DBGINFO:RF nv write %ld nums!\r\n",write_num);

	else
		sprintf(dbg_info,"+DBGINFO:user nv %lX write!\r\n",addr);

	send_debug_by_at_uart(dbg_info);

	xy_free(dbg_info);
}

/*深睡被打断，及时释放跨核写flash的内存*/
void free_ext_flash_write_node(void)
{
	lpm_nv_write_buff_remove_all();
}

#define CP_IS_IN_IDLE()  ((osKernelGetState() != osKernelInactive) && (osCoreGetState() == osCoreInCritical) &&(osKernelIsRunningIdle() == osOK))

extern factory_nv_t *g_factory_nv;
extern void xy_flash_write_async(uint32_t addr, void *data, uint32_t size);
bool ftl_write(uint32_t addr, uint8_t *data, uint32_t size, uint8_t asyn_flag, osMutexId_t mutex_id)
{
	uint32_t write_num = 0;
	uint32_t offset = addr&(FLASH_SECTOR_LENGTH-1);
	uint32_t page_addr_base = addr-offset;
	int next_ftl_idx = -1;
	int cur_ftl_idx = -1;
	uint8_t *p_page_buf = NULL;
	static int dslp_save_fac = 0;
		
	xy_assert((offset+size) <= XY_FTL_AVAILABLE_SIZE);
	

	/*OPENCPU形态，仅深睡时才可以通知AP核保存出厂NV和部分非易变NV*/
	if(NOT_ALLOWED_SAVE_FLASH())
	{
		if(addr >= BAK_MEM_FLASH_BASE && addr < BAK_MEM_FLASH_BASE+BAK_MEM_FLASH_TOTAL_LEN)
			return 1;
		
		else if(addr >= NV_NON_VOLATILE_BASE && addr < NV_NON_VOLATILE_BASE+NV_NON_VOLATILE_LEN)
		{
			if(!CP_IS_IN_IDLE())
				return 1;
		}	
		else if(page_addr_base == NV_FLASH_FACTORY_BASE)
		{
			lpm_nv_write_buff_add((uint32_t)Address_Translation_CP_To_AP(page_addr_base),(uint32_t)g_factory_nv,sizeof(factory_nv_t));

			return 1;		
		}
	}

	p_page_buf = xy_malloc2(FLASH_SECTOR_LENGTH);
	if(p_page_buf == NULL)
	{
		send_debug_by_at_uart("+DBGINFO:ftl_write malloc fail!\r\n");
		return 0;
	}

	xy_choose_valid_ftl(page_addr_base,p_page_buf,&cur_ftl_idx, &next_ftl_idx);

	if(osKernelGetState() >= osKernelRunning && osKernelIsRunningIdle()!= osOK && HWREGB(BAK_MEM_XY_DUMP) == 1)
		PrintLog(0,PLATFORM,WARN_LOG,"[running]xy_ftl_write: %x %d %d %d %s %d ", addr, size, asyn_flag, mutex_id, osThreadGetName(osThreadGetId()), Get_Boot_Sub_Reason());

	//flash中没有有效NV，一般是首次写入
	if(cur_ftl_idx == -1)
	{
		if(osKernelGetState() >= osKernelRunning && osKernelIsRunningIdle()!= osOK && HWREGB(BAK_MEM_XY_DUMP) == 1)
			PrintLog(0,PLATFORM,WARN_LOG,"[running]xy_ftl_write find ftl fail: %x %d %d %d %s %d ", addr, size, asyn_flag, mutex_id, osThreadGetName(osThreadGetId()), Get_Boot_Sub_Reason());

		/* 出厂NV尚未保存，用main factory nv */
		if(page_addr_base == NV_FLASH_FACTORY_BASE)
		{
			xy_Flash_Read(NV_MAIN_FACTORY_BASE, p_page_buf+4, FLASH_SECTOR_LENGTH-4);
			memset(p_page_buf + sizeof(factory_nv_t) + 4, 0xff, FLASH_SECTOR_LENGTH - sizeof(factory_nv_t) - 4);
		}
		else
		{
			/* buf清零防止首次写入时带入非零数据 */
			memset(p_page_buf, 0xff, FLASH_SECTOR_LENGTH);
		}
	}
	/*仅用于PS工作态的NV保存*/
	if(mutex_id != NULL)
		flash_acquire_mutex(mutex_id, osWaitForever);

	//if new data is equal old data, Don't write flash
	if(cur_ftl_idx != -1 && memcmp((p_page_buf+4+offset),data,size) == 0)
	{
		xy_free(p_page_buf);

		if(mutex_id != NULL)
			flash_release_mutex(mutex_id);
		
		if(osKernelGetState() >= osKernelRunning && osKernelIsRunningIdle()!= osOK && HWREGB(BAK_MEM_XY_DUMP) == 1)
			PrintLog(0,PLATFORM,WARN_LOG,"[running]xy_ftl_write same: %x %d %d %d %s %d ", addr, size, asyn_flag, mutex_id, osThreadGetName(osThreadGetId()), Get_Boot_Sub_Reason());
	
		return 1;
	}
	memcpy((p_page_buf+4+offset),data,size);

	if(mutex_id != NULL)
		flash_release_mutex(mutex_id);

	*((uint32_t *)(p_page_buf)) = 0X5A5A5A5A;
	*((uint32_t*)(p_page_buf+EFTL_CSM_OFFSET)) = xy_chksum((void*)p_page_buf,EFTL_CSM_OFFSET);

	write_num = *((uint32_t *)(p_page_buf+EFTL_WRITE_NUM_OFFSET));
	if(write_num == 0XFFFFFFFF)
		write_num = 0;
	write_num++;
	*((uint32_t *)(p_page_buf+EFTL_WRITE_NUM_OFFSET)) = write_num;
	
	if(asyn_flag == 0)
	{
		/*ldle线程，通知AP核执行写动作*/
		if(CP_IS_IN_IDLE())
		{
			lpm_nv_write_buff_add((uint32_t)Address_Translation_CP_To_AP((unsigned int)(page_addr_base+FLASH_SECTOR_LENGTH*next_ftl_idx)), \
										(uint32_t)Address_Translation_CP_To_AP((unsigned int)p_page_buf),FLASH_SECTOR_LENGTH);

			return 1;		
		}
		else/*CP核执行写FLASH动作。OPEN形态，RF有低概率写需求*/
		{
			nv_write_dbg_info(page_addr_base+FLASH_SECTOR_LENGTH*next_ftl_idx,write_num);

			xy_Flash_Write(page_addr_base+FLASH_SECTOR_LENGTH*next_ftl_idx,p_page_buf,FLASH_SECTOR_LENGTH);

			/*增加flash写校验，只有同步写接口能保证此时flash已写入*/
			if (HWREGB(BAK_MEM_XY_DUMP) == 1 && !((osKernelGetState() != osKernelInactive) && (osCoreGetState() == osCoreInCritical) && (osKernelIsRunningIdle() == osOK)))
			{
				xy_assert(memcmp((uint8_t *)(page_addr_base+FLASH_SECTOR_LENGTH*next_ftl_idx),p_page_buf,FLASH_SECTOR_LENGTH) == 0);
			}

			xy_free(p_page_buf);
		}
	}
	else if(!(NOT_ALLOWED_SAVE_FLASH()))/*仅用于PS非易变NV的保存*/
	{
		nv_write_dbg_info(page_addr_base+FLASH_SECTOR_LENGTH*next_ftl_idx,write_num);

		xy_flash_write_async(page_addr_base+FLASH_SECTOR_LENGTH*next_ftl_idx,p_page_buf,FLASH_SECTOR_LENGTH);
	}
		
	
	return 1;
}

bool xy_ftl_write(uint32_t addr, uint8_t *data, uint32_t size)
{
	bool  ret;
	
	flash_acquire_mutex(ftl_mutex, osWaitForever);

	/*RF驱动执行写NV时，临时放开写保护;并记录debug信息*/
	if(addr >= NV_FLASH_RF_BASE && addr < NV_FLASH_RF_BAKUP_BASE+NV_FLASH_RF_SIZE)
	{
		flash_interface_protect_disable();
		extern void save_rf_dbg_into(void *param,int len);	
		save_rf_dbg_into(NULL,0);
	}
	
	ret = ftl_write(addr, data, size, 0, NULL);

	if(addr >= NV_FLASH_RF_BASE && addr < NV_FLASH_RF_BAKUP_BASE+NV_FLASH_RF_SIZE)
		flash_interface_protect_enable();

	flash_release_mutex(ftl_mutex);

	return ret;
}


bool ftl_write_with_mutex(uint32_t addr, uint8_t *data, uint32_t size, osMutexId_t mutex_id)
{
	bool ret;
	flash_acquire_mutex(ftl_mutex, osWaitForever);
	ret = ftl_write(addr, data, size, 0, mutex_id);
	flash_release_mutex(ftl_mutex);
	return ret;
}

bool ftl_write_with_mutex_async(uint32_t addr, uint8_t *data, uint32_t size, osMutexId_t mutex_id)
{
	bool ret;
	flash_acquire_mutex(ftl_mutex, osWaitForever);
	ret = ftl_write(addr, data, size, 1, mutex_id);
	flash_release_mutex(ftl_mutex);
	return ret;
}

void xy_ftl_erase(uint32_t addr)
{
	struct xy_eftl_cfg *nv_cfgs_temp = NULL;

	flash_acquire_mutex(ftl_mutex, osWaitForever);

	nv_cfgs_temp = find_valid_ftl_cfg(addr);

	xy_assert(nv_cfgs_temp != NULL);

	/*RF驱动执行擦NV时，临时放开写保护;并记录debug信息*/
	if(addr >= NV_FLASH_RF_BASE && addr < NV_FLASH_RF_BAKUP_BASE+NV_FLASH_RF_SIZE)
	{
		flash_interface_protect_disable();
		extern void save_rf_dbg_into(void *param,int len);	
		save_rf_dbg_into(NULL,0);
	}
		
	
	xy_Flash_Erase(nv_cfgs_temp->addr,nv_cfgs_temp->tpage_total*FLASH_SECTOR_LENGTH);

	if(addr >= NV_FLASH_RF_BASE && addr < NV_FLASH_RF_BAKUP_BASE+NV_FLASH_RF_SIZE)
		flash_interface_protect_enable();

	flash_release_mutex(ftl_mutex);
}

void xy_ftl_regist(uint32_t addr,uint32_t size)
{
    struct xy_eftl_cfg *new_user_nv_cfgs = (struct xy_eftl_cfg*)xy_malloc(sizeof(struct xy_eftl_cfg));

	xy_assert(addr%FLASH_SECTOR_LENGTH==0 && size%FLASH_SECTOR_LENGTH==0);
    new_user_nv_cfgs->next = NULL;
    new_user_nv_cfgs->addr = addr;
	new_user_nv_cfgs->tpage_total = size/FLASH_SECTOR_LENGTH;
	
	flash_acquire_mutex(ftl_mutex, osWaitForever);
	
    if (user_nv_cfgs == NULL)
    {
        user_nv_cfgs = new_user_nv_cfgs;
    }
	else
	{
		new_user_nv_cfgs->next = user_nv_cfgs;
		user_nv_cfgs = new_user_nv_cfgs;
	}
	
	flash_release_mutex(ftl_mutex);
}

