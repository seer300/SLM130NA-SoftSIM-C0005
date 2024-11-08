#include "lpm_nv_write.h"
#include "xy_memmap.h"

extern void *g_cloud_mem_p;
extern factory_nv_t *g_factory_nv;
/*CP深睡过程中需要保存的NV/FLASH数据，最终由AP核在深睡流程中写FLASH*/
void lpm_nv_write_buff_add(uint32_t dest_addr, uint32_t src_addr, uint32_t size)
{
	volatile lpm_nv_write_t *lpm_nv_write = (lpm_nv_write_t *)BAK_MEM_LPM_NV_WRITE_BUFF;
	uint8_t write_num = lpm_nv_write->lpm_nv_write_num;

	for(uint8_t i = 0; i < write_num; i++)
	{
		if((find_valid_ftl_cfg(dest_addr) != NULL && find_valid_ftl_cfg(lpm_nv_write->lpm_nv_write_buff[i].dest_addr) == find_valid_ftl_cfg(dest_addr)) ||
			(find_valid_ftl_cfg(dest_addr) == NULL && lpm_nv_write->lpm_nv_write_buff[i].dest_addr == dest_addr))
		{
			/*出厂NV和云配置，始终使用全局RAM*/
			if((lpm_nv_write->lpm_nv_write_buff[i].src_addr != (uint32_t)g_cloud_mem_p) && (dest_addr != Address_Translation_CP_To_AP(NV_FLASH_FACTORY_BASE)))
			{
				lpm_nv_write->lpm_nv_write_buff[i].src_addr = src_addr;
			}
			return;
		}
	}
	
	xy_assert(write_num < LPM_NV_WRITE_NUM);
	
	lpm_nv_write->lpm_nv_write_buff[write_num].dest_addr = dest_addr;
	lpm_nv_write->lpm_nv_write_buff[write_num].src_addr = src_addr;
	lpm_nv_write->lpm_nv_write_buff[write_num].size = size;

	lpm_nv_write->lpm_nv_write_num++;
}


/*当CP核深睡被打断恢复，需要清空待写的NV缓存，待下次深睡时重新写入*/
void lpm_nv_write_buff_remove_all(void)
{
	volatile lpm_nv_write_t *lpm_nv_write = (lpm_nv_write_t *)BAK_MEM_LPM_NV_WRITE_BUFF;
	uint8_t write_num = lpm_nv_write->lpm_nv_write_num;

	lpm_nv_write->lpm_nv_write_num = 0;

	for(uint8_t i = 0; i < write_num; i++)
	{
		uint32_t src_addr = (uint32_t)Address_Translation_AP_To_CP(lpm_nv_write->lpm_nv_write_buff[i].src_addr);

		/*出厂NV和云配置，始终使用全局RAM*/
		if(((void*)src_addr != NULL) && ((void*)src_addr != g_cloud_mem_p) && ((void*)src_addr != (uint32_t)g_factory_nv))
		{
			xy_free((void*)src_addr);
		}
	}	
}


void lpm_nv_write_init(void)
{
	volatile lpm_nv_write_t *lpm_nv_write = (lpm_nv_write_t *)BAK_MEM_LPM_NV_WRITE_BUFF;

	lpm_nv_write->lpm_nv_write_num = 0;
}
