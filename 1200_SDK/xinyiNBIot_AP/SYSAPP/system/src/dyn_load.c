#include "dyn_load.h"
#include "xy_system.h"
#include "flash_vendor.h"
#include "xy_flash.h"
#include "cache.h"

#if DYN_LOAD

void set_got_r9()
{
    if (SO_AVAILABLE)
    {
        __asm volatile(
            "   dsb    \n"
            "   ldr r9, r9_addr_const    \n"
            "   ldr r9, [r9]    \n"
            "   bx lr    \n"
            "   .align 4    \n"
            "r9_addr_const: .word SO_GOT_EXEC_ADDR");
    }
}

typedef struct dyn_addr_info_s
{
    struct dyn_addr_info_s *next;
    void *addr;
} dyn_addr_info_t;

dyn_addr_info_t *dyn_addr_info_head = NULL;

void mark_dyn_addr(void *addr)
{
    DisablePrimask();

	dyn_addr_info_t *cur = dyn_addr_info_head;
	while (cur)
    {
        if (cur->addr == addr)
        {
            EnablePrimask();
            return;
        }
        cur = cur->next;
    }

    dyn_addr_info_t *new_dyn_addr_info = (dyn_addr_info_t *)xy_malloc(sizeof(dyn_addr_info_t));
    new_dyn_addr_info->addr = addr;
    new_dyn_addr_info->next = dyn_addr_info_head;
    dyn_addr_info_head = new_dyn_addr_info;

    EnablePrimask();
}


/*该接口耗时约15us*/
static void switch_dyn_addr(int index)
{
    dyn_addr_info_t *cur = dyn_addr_info_head;
    uint32_t dyn_addr;
    if (index == 1)
    {
        while (cur)
        {
            dyn_addr = *(uint32_t *)(cur->addr);
            if (dyn_addr >= SO_TEXT_EXEC_ADDR2 && dyn_addr < SO_TEXT_EXEC_ADDR2 + SO_TEXT_SIZE)
            {
                *(uint32_t *)(cur->addr) = dyn_addr - SO_TEXT_EXEC_ADDR2 + SO_TEXT_EXEC_ADDR1;
            }
            cur = cur->next;
        }
    }
    else if (index == 2)
    {
        while (cur)
        {
            dyn_addr = *(uint32_t *)(cur->addr);
            if (dyn_addr >= SO_TEXT_EXEC_ADDR1 && dyn_addr < SO_TEXT_EXEC_ADDR1 + SO_TEXT_SIZE)
            {
                *(uint32_t *)(cur->addr) = dyn_addr - SO_TEXT_EXEC_ADDR1 + SO_TEXT_EXEC_ADDR2;
            }
            cur = cur->next;
        }
    }
}

/*入参为1，中断函数运行在CP RAM上；2运行在FLASH上*/
static void switch_nvic_vectors(int index)
{
    uint32_t i;
    if (index == 1)
    {
        for (i = 0; i < sizeof(g_pfnVectors) / sizeof(g_pfnVectors[0]); i++)
        {
            if (g_pfnVectors[i] >= SO_TEXT_EXEC_ADDR2 && g_pfnVectors[i] < SO_TEXT_EXEC_ADDR2 + SO_TEXT_SIZE)
            {
                g_pfnVectors[i] = g_pfnVectors[i] - SO_TEXT_EXEC_ADDR2 + SO_TEXT_EXEC_ADDR1;
            }
        }
    }
    else if (index == 2)
    {
        for (i = 0; i < sizeof(g_pfnVectors) / sizeof(g_pfnVectors[0]); i++)
        {
            if (g_pfnVectors[i] >= SO_TEXT_EXEC_ADDR1 && g_pfnVectors[i] < SO_TEXT_EXEC_ADDR1 + SO_TEXT_SIZE)
            {
                g_pfnVectors[i] = g_pfnVectors[i] - SO_TEXT_EXEC_ADDR1 + SO_TEXT_EXEC_ADDR2;
            }
        }
    }
}


static void switch_changed_data(int index)
{
    if (SO_DATA_CHANGE_INFO_COUNT > 0)
    {
        uint32_t data_addr;
        uint32_t addr;
        uint32_t i;
        if (index == 1)
        {
            for (i = 0; i < SO_DATA_CHANGE_INFO_COUNT; i++)
            {
                data_addr = *(uint32_t *)(SO_DATA_CHANGE_INFO_ADDR + i * sizeof(uint32_t));
                addr = *(uint32_t *)(data_addr);
                if (addr >= SO_TEXT_EXEC_ADDR2 && addr < SO_TEXT_EXEC_ADDR2 + SO_TEXT_SIZE)
                {
                    *(uint32_t *)(data_addr) = addr - SO_TEXT_EXEC_ADDR2 + SO_TEXT_EXEC_ADDR1;
                }
            }
        }
        else if (index == 2)
        {
            for (i = 0; i < SO_DATA_CHANGE_INFO_COUNT; i++)
            {
                data_addr = *(uint32_t *)(SO_DATA_CHANGE_INFO_ADDR + i * sizeof(uint32_t));
                addr = *(uint32_t *)(data_addr);
                if (addr >= SO_TEXT_EXEC_ADDR1 && addr < SO_TEXT_EXEC_ADDR1 + SO_TEXT_SIZE)
                {
                    *(uint32_t *)(data_addr) = addr - SO_TEXT_EXEC_ADDR1 + SO_TEXT_EXEC_ADDR2;
                }
            }
        }
    }
}

/*当前SO使用的空间，1表示运行在CP RAM上，2表示运行在FLASH上。*/
int g_cur_dyn_index = 1;

/*非深睡唤醒上电，会在二级boot进行搬到CP RAM的动作；后续一旦boot_CP，会切换到FLASH上运行；进入深睡时，再执行搬到CP RAM的动作*/
/*入参为1时，会执行代码的搬移，其中64K耗时24ms，100K耗时37.5ms*/
void Dyn_Switch(int index)
{
    if (SO_AVAILABLE)
    {
        DisablePrimask();
        extern uint8_t *__got_plt_start__;
        extern uint8_t *__got_plt_end__;
        uint32_t got_plt_start = (uint32_t)&__got_plt_start__;
        uint32_t got_plt_end = (uint32_t)&__got_plt_end__;
        if (g_cur_dyn_index == index)
        {
            EnablePrimask();
            return;
        }
        // boot_CP完后，深睡时执行，将SO文件夹内容放在CP的SRAM上运行
        if (index == 1) // SO文件夹内容放在CP的SRAM上运行
        {
        	/*使能CP RAM进cache*/
        	CacheDis(AP_CACHE_BASE);
        	CacheInit(AP_CACHE_BASE, CACHE_CCR0_WRITE_THROUGH, SHARE_RAM0_BASE, SHARE_RAM1_BASE-1);
			CacheCleanInvalidAll(AP_CACHE_BASE);
			
            xy_Flash_Read(ELF_GOT_PLT1_LOAD_ADDR, (void *)got_plt_start, got_plt_end - got_plt_start);  //elf访问SO函数所产生的表
            xy_Flash_Read(SO_TEXT_LOAD_ADDR, (void *)SO_TEXT_EXEC_ADDR1, SO_TEXT_SIZE);  //搬代码到CP RAM，64K耗时24ms，100K耗时37.5ms
            xy_Flash_Read(SO_GOT_PLT1_LOAD_ADDR, (void *)SO_TEXT_EXEC_ADDR1 + SO_TEXT_SIZE, SO_GOT_PLT_SIZE);  //SO访问elf函数所产生的表
            if (SO_GOT_MODIFIED)
                xy_Flash_Read(SO_GOT1_LOAD_ADDR, (void *)SO_GOT_EXEC_ADDR, SO_GOT_SIZE);  //SO全局变量
#if BAN_WRITE_FLASH
            switch_nvic_vectors(1);
#endif
            switch_changed_data(1);
            switch_dyn_addr(1);
            g_cur_dyn_index = 1;
        }
        //boot_CP时执行，将SO文件夹内容放在FLASH上运行
        else if (index == 2)
        {
        	/*使能FLASH进cache*/
        	CacheDis(AP_CACHE_BASE);
        	CacheInit(AP_CACHE_BASE, CACHE_CCR0_WRITE_THROUGH, ARM_FLASH_BASE_ADDR, HWREG(BAK_MEM_FOTA_FLASH_BASE) - 1);
			CacheCleanInvalidAll(AP_CACHE_BASE);
			
            xy_Flash_Read(ELF_GOT_PLT2_LOAD_ADDR, (void *)got_plt_start, got_plt_end - got_plt_start);  //elf访问SO函数所产生的表
            if (SO_GOT_MODIFIED)
                xy_Flash_Read(SO_GOT2_LOAD_ADDR, (void *)SO_GOT_EXEC_ADDR, SO_GOT_SIZE);   //SO全局变量
#if BAN_WRITE_FLASH
            switch_nvic_vectors(2);
#endif
            switch_changed_data(2);
            switch_dyn_addr(2);
            g_cur_dyn_index = 2;
        }
        else
        {
        }
        EnablePrimask();
    }
    else
    {
    }
}
#endif
