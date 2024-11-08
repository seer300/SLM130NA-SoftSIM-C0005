#include "xy_ftl.h"
#include "xy_memmap.h"
#include "xy_flash.h"
#include "xy_system.h"
#include "string.h"
#include "xy_utils.h"
#include "xy_list.h"

#define EFTL_MAGIC_NUM          0X5A5A5A5A
#define EFTL_CSM_OFFSET        (FLASH_SECTOR_LENGTH - 8)
#define EFTL_WRITE_NUM_OFFSET  (FLASH_SECTOR_LENGTH - 4)
#define EFTL_PAGE_OFFSET_ADDR(ADDR, OFFSET) (*((uint32_t *)(ADDR + OFFSET)))

typedef struct {
	struct List_t *next;
	void* ftl_block_addr;
	uint32_t ftl_page_num;
}FTL_Node_T;


ListHeader_t g_ftl_block = {0};


 void xy_ftl_regist(void* addr, uint32_t len)
{
	FTL_Node_T *pxlist;
	uint32_t page_num = len/FLASH_SECTOR_LENGTH;

    if(len&(FLASH_SECTOR_LENGTH-1))
    {
        xy_assert(0);
    }

	pxlist = xy_malloc(sizeof(FTL_Node_T));

	pxlist->next = NULL;
	pxlist->ftl_block_addr = addr;
	pxlist->ftl_page_num = page_num;

	ListInsert((List_t *)pxlist,&g_ftl_block);
}


FTL_Node_T *find_ftl_node(uint32_t addr)
{
	FTL_Node_T *pxlist = (FTL_Node_T *)g_ftl_block.head;
	

	while(pxlist != NULL)
	{
		if(pxlist->ftl_block_addr == (void*)(addr&0XFFFFF000))
			return  pxlist;

		pxlist = (FTL_Node_T *)(pxlist->next);
	}
	
	xy_assert(0);
	
	return NULL;
}

void xy_choose_valid_ftl(uint32_t addr, void* data, int32_t *cur_ftl_idx, int32_t *next_ftl_idx)
{
    int32_t valid_page_idx = -1;
    uint32_t write_num;
    uint32_t max_write_num = 0;
	FTL_Node_T *ftl_node = find_ftl_node(addr);
    valid_page_idx = -1;

    for(uint32_t i = 0; i < ftl_node->ftl_page_num; i++)
    {
    	xy_Flash_Read((uint32_t)(ftl_node->ftl_block_addr) + FLASH_SECTOR_LENGTH * i, data, FLASH_SECTOR_LENGTH);
		
        write_num = EFTL_PAGE_OFFSET_ADDR(data, EFTL_WRITE_NUM_OFFSET);

		/*为了减少计算量，此处未执行checmsum的校验*/
        if(EFTL_PAGE_OFFSET_ADDR(data, 0) == EFTL_MAGIC_NUM && write_num != 0xFFFFFFFF && write_num != 0)
        {
            if(max_write_num <= write_num)
            {
                max_write_num = write_num;
                valid_page_idx = i;
            }
        }
    }

    if(valid_page_idx != -1)
    {
        xy_Flash_Read((uint32_t)(ftl_node->ftl_block_addr) + FLASH_SECTOR_LENGTH * valid_page_idx, data, FLASH_SECTOR_LENGTH);
        *cur_ftl_idx = valid_page_idx;
		*next_ftl_idx = (valid_page_idx + 1) % ftl_node->ftl_page_num;
    }
    else
	{
		*cur_ftl_idx = -1;
		*next_ftl_idx = 0;
	}
}


bool xy_ftl_read(uint32_t addr, void* data, uint32_t size)
{
    void* page_buf = NULL;
	uint32_t offset = addr&0XFFF;
	uint32_t page_addr_base = addr - offset;
    int32_t cur_ftl_idx = -1;
    int32_t next_ftl_idx = -1;


    xy_assert(offset + size <= XY_FTL_AVAILABLE_SIZE);

    page_buf = xy_malloc(FLASH_SECTOR_LENGTH);
    xy_choose_valid_ftl(page_addr_base,page_buf, &cur_ftl_idx, &next_ftl_idx);

    if(cur_ftl_idx == -1)
    {
    	if(page_addr_base == NV_FLASH_FACTORY_BASE)
    	{
    		xy_Flash_Read((NV_FLASH_MAIN_FACTORY_BASE+offset),data,size);
    	}
    	else
    	{
	        xy_free(page_buf);
	        return 0;
    	}
    }
    else
    {
        memcpy(data, ((char*)page_buf + 4 + offset), size);
    }
	xy_free(page_buf);
    return 1;
}

bool xy_ftl_write(uint32_t addr, void* data, uint32_t size)
{
    void* page_buf = NULL;
	uint32_t offset = addr&0XFFF;
	uint32_t page_addr_base = addr - offset;
    int32_t cur_ftl_idx = -1;
    int32_t next_ftl_idx = -1;
    uint32_t write_num = 0;
	int ret = 0;

    xy_assert(offset + size <= XY_FTL_AVAILABLE_SIZE);

    page_buf = xy_malloc(FLASH_SECTOR_LENGTH);

    xy_choose_valid_ftl(page_addr_base,page_buf, &cur_ftl_idx, &next_ftl_idx);

    /*flash中没有有效NV，一般是首次写入,尾部需设为全F，以加快erase擦写速度*/
    if (cur_ftl_idx == -1)
    {
		/* 出厂NV尚未保存，用main factory nv */
		if(page_addr_base == NV_FLASH_FACTORY_BASE)
		{
			xy_Flash_Read(NV_FLASH_MAIN_FACTORY_BASE, page_buf, FLASH_SECTOR_LENGTH);
			memset(page_buf+sizeof(factory_nv_t)+4,0XFF,FLASH_SECTOR_LENGTH-sizeof(factory_nv_t)-4);
		}
		else
		{
			/* buf清零防止首次写入时带入非零数据 */
			memset(page_buf,0XFF,FLASH_SECTOR_LENGTH);
		}
	}
	
    if(cur_ftl_idx != -1 && memcmp(((char*)page_buf + 4 + offset), data, size) == 0)
    {
        xy_free(page_buf);
        return 1;
    }

    *((uint32_t*)(page_buf)) = EFTL_MAGIC_NUM;

    memcpy(((char*)page_buf + 4 + offset), data, size);

	*((uint32_t*)((char*)page_buf + EFTL_CSM_OFFSET)) = xy_chksum((void*)page_buf, EFTL_CSM_OFFSET);

    write_num = *((uint32_t *)((char*)page_buf + EFTL_WRITE_NUM_OFFSET));

	write_num++;
	if(write_num == 0XFFFFFFFF || write_num == 0)
		write_num = 1;

    *((uint32_t *)((char*)page_buf+EFTL_WRITE_NUM_OFFSET)) = write_num;

    ret = xy_Flash_Write((uint32_t)(page_addr_base + FLASH_SECTOR_LENGTH * next_ftl_idx), page_buf, FLASH_SECTOR_LENGTH);

    if(ret != 1)
	{
		xy_free(page_buf);
		return 0;
	}

    xy_assert(*(uint32_t*)(page_addr_base + FLASH_SECTOR_LENGTH * next_ftl_idx) == EFTL_MAGIC_NUM);

    xy_free(page_buf);
    return 1;
}

void xy_ftl_erase(uint32_t addr)
{
	FTL_Node_T *temp = NULL;

	temp = find_ftl_node(addr);

    xy_Flash_Erase((uint32_t)(temp->ftl_block_addr),temp->ftl_page_num * FLASH_SECTOR_LENGTH);
}

void xy_ftl_init()
{
    xy_ftl_regist((void*)NV_FLASH_FACTORY_BASE,FLASH_SECTOR_LENGTH);
	xy_ftl_regist((void*)CALIB_FREQ_BASE,FLASH_SECTOR_LENGTH);
}