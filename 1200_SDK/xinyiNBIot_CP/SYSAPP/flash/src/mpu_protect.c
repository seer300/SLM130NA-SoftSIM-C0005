/*
 * flash_protect.c
 *
 *  Created on: 2022年8月15日
 *      Author: jiangzj
 */
#include "MPU.h"
#include "attribute.h"
#include "xy_memmap.h"
#include "flash_vendor.h"
#include "xy_flash.h"

extern uint32_t __vectors_start; // Defined by the linker.
extern uint32_t __data_start__;  // Defined by the linker.
volatile uint32_t flash_protect_enable = 1;
volatile uint32_t flash_protectNesting = 0;

//mpu保护物理内存区域， 一共可设置8块内存保护区域
void mpu_protect_init(void)
{
    /* calculate data for protect text section in ram. */
    uint32_t text_start = (uint32_t)&__vectors_start;
    uint32_t text_size = (uint32_t)&__data_start__ - text_start;
    uint32_t text_protect_addr, text_protect_size, sub_region_num = MPU_SUB_REGION_NUMBER_0;
    text_protect_addr = text_start;
    for (text_protect_size = 5; text_protect_size < 32; text_protect_size++)
    {
        if (((text_size - 1) >> text_protect_size) == 0)
        {
            break;
        }
    }
    uint32_t sub_region_size = 1 << (text_protect_size - 3);
    uint32_t region_size = 1 << text_protect_size;
    for (uint32_t i = 8; i > 0; i--)
    {
        if(text_size < region_size)
        {
            sub_region_num |= 1 << (i - 1 + 8);
            text_size += sub_region_size;
        }
        else
        {
            break;
        }
    }

    /* calculate data for protect data, bss, heap and stack section in ram. */
    uint32_t data_start = (uint32_t)&__data_start__;
	uint32_t data_protect_end = 0;
    if((HWREGB(AONPRCM_BASE + 0x01) & 0x01) ) //CP Remap
	{
    	data_protect_end = TCM_CP_REMAP_LIMITED;
	}
    else
    {
    	data_protect_end = TCM_CP_LIMITED;
    }
    uint32_t data_size = data_protect_end - data_start;
    uint32_t data_protect_addr, data_protect_size;
    for (data_protect_size = 5; data_protect_size < 32; data_protect_size++)
    {
        if ((data_size >> data_protect_size) == 1)
        {
            break;
        }
    }
    data_protect_addr = data_protect_end - (1 << data_protect_size);

    /* region configuration */
    uint32_t RegionConfig0, RegionConfig1, RegionConfig2, RegionConfig3, RegionConfig4, RegionConfig7, RegionConfig5;
    /* Region 0 configuration, full access and enable execute code,
     * use for global attribute. */
    RegionConfig0 = MPU_REGION_CONFIG_XN_INSTRUCTION_ACCESS_ENABLE | \
                    MPU_REGION_CONFIG_AP_FULL_ACCESS | \
                    MPU_REGION_CONFIG_TEX_S_C_B_OUTER_INNER_WRITEBACK_NO_WRITE_ALLOCATE;
    /* Region 1 configuration, no access and enable execute code,
     * use for protect text section. */
    RegionConfig1 = MPU_REGION_CONFIG_XN_INSTRUCTION_ACCESS_ENABLE | \
                    MPU_REGION_CONFIG_AP_READ_ONLY | \
                    MPU_REGION_CONFIG_TEX_S_C_B_OUTER_INNER_WRITEBACK_NO_WRITE_ALLOCATE;
    /* Region 2 configuration, full access and disable execute code,
     * use for protect data, bss, heap and stack section. */
    RegionConfig2 = MPU_REGION_CONFIG_XN_INSTRUCTION_ACCESS_DISABLE | \
                    MPU_REGION_CONFIG_AP_FULL_ACCESS | \
                    MPU_REGION_CONFIG_TEX_S_C_B_OUTER_INNER_WRITEBACK_NO_WRITE_ALLOCATE;
    /* Region 3 configuration, no access and disable execute code,
     * use for protect flash. */
    RegionConfig3 = MPU_REGION_CONFIG_XN_INSTRUCTION_ACCESS_ENABLE | \
                    MPU_REGION_CONFIG_AP_READ_ONLY | \
                    MPU_REGION_CONFIG_TEX_S_C_B_OUTER_INNER_WRITEBACK_NO_WRITE_ALLOCATE;
    /* Region 4 configuration, no access and disable execute code,
     * use for protect rom function. */
    RegionConfig4 = MPU_REGION_CONFIG_XN_INSTRUCTION_ACCESS_ENABLE | \
                    MPU_REGION_CONFIG_AP_READ_ONLY | \
                    MPU_REGION_CONFIG_TEX_S_C_B_OUTER_INNER_WRITEBACK_NO_WRITE_ALLOCATE;
    /* Region 7 configuration, read only and disable execute code,
     * use for protect isr_vector section. */
    RegionConfig7 = MPU_REGION_CONFIG_XN_INSTRUCTION_ACCESS_DISABLE | \
                    MPU_REGION_CONFIG_AP_READ_ONLY | \
                    MPU_REGION_CONFIG_TEX_S_C_B_OUTER_INNER_WRITEBACK_NO_WRITE_ALLOCATE;

    RegionConfig5 = MPU_REGION_CONFIG_XN_INSTRUCTION_ACCESS_DISABLE | \
        		MPU_REGION_CONFIG_AP_NO_ACCESS | \
                MPU_REGION_CONFIG_TEX_S_C_B_OUTER_INNER_WRITEBACK_NO_WRITE_ALLOCATE;

	MPU_Cmd(DISABLE);

	/* global memory protect. */
	MPU_RegionInit(MPU_REGION_NUMBER_0, 0x0, MPU_REGION_LENGTH_4G, RegionConfig0);  //所有内存默认访问属性，可执行，可读可写
	/* text section protect. */
	MPU_RegionInit(MPU_REGION_NUMBER_1, text_protect_addr, (text_protect_size - 1), RegionConfig1); //CP核代码段不可写保护；但未对AP核代码段保护
	MPU_SubregionCmd(MPU_REGION_NUMBER_1, sub_region_num, DISABLE);
	/* data, bss, heap and stack section protect. */
	MPU_RegionInit(MPU_REGION_NUMBER_2, data_protect_addr, (data_protect_size - 1), RegionConfig2); //CP核的RAM的数据段不可执行，可读可写
	/* isr_vector section protect. */
	MPU_RegionInit(MPU_REGION_NUMBER_7, (uint32_t)g_pfnVectors, MPU_REGION_LENGTH_64B, RegionConfig7); //中断向量表不可执行，只读
	/* flash protect, enable flash access in initialise, disable in special scenario. */
	if((HWREGB(AONPRCM_BASE + 0x01) & 0x01) ) //CP Remap
	{
		MPU_RegionInit(MPU_REGION_NUMBER_3, FLASH_CP_REMAP_BASE, MPU_REGION_LENGTH_4M, RegionConfig3);  //CP核flash不容许赋值写，只能通过flash寄存器配置写
		
		MPU_RegionInit(MPU_REGION_NUMBER_5, FLASH_CP_REMAP_BASE, MPU_REGION_LENGTH_32B, RegionConfig5);	//flash前32个字节为0地址，不可执行，防止0地址访问
		
		MPU_RegionInit(MPU_REGION_NUMBER_6, FLASH_CP_REMAP_BASE, MPU_REGION_LENGTH_4M, RegionConfig5); //不准执行XIP模式，该功能初始化时关闭；由擦flash过程时通过mpu_flash_access_disable2动态开启保护
		MPU_SubregionCmd(MPU_REGION_NUMBER_6, MPU_SUB_REGION_ALL, DISABLE); 
	}
	else
	{
		MPU_RegionInit(MPU_REGION_NUMBER_3, FLASH_CP_BASE, MPU_REGION_LENGTH_4M, RegionConfig3);
		
		MPU_RegionInit(MPU_REGION_NUMBER_5, FLASH_CP_BASE, MPU_REGION_LENGTH_32B, RegionConfig5);
	}
    /* rom code protect, 96K from 0x20100000. */
    MPU_RegionInit(MPU_REGION_NUMBER_4, 0x20100000, MPU_REGION_LENGTH_128K, RegionConfig4);   //rom代码段(一级boot)可执行，只读
    MPU_SubregionCmd(MPU_REGION_NUMBER_4, MPU_SUB_REGION_NUMBER_6 | MPU_SUB_REGION_NUMBER_7, DISABLE);

    MPU_Control(MPU_CONTROL_PRIVILEGED_DEFAULT_MEMORY_MAP, ENABLE);
    MPU_Cmd(ENABLE);
}

/*与mpu_flash_access_disable2配对使用*/
__RAM_FUNC void Flash_mpu_Unlock(void)
{
    /* enable flash execute code and access flash data. */
	MPU_SubregionCmd(MPU_REGION_NUMBER_6, MPU_SUB_REGION_ALL, DISABLE);
}

/*禁止任何的flash访问。通常用于中断函数的flash访问检查，例如flash擦除操作进行过程中*/
__RAM_FUNC void Flash_mpu_Lock(void)
{
    /* disable flash execute code and access flash data. */
	MPU_SubregionCmd(MPU_REGION_NUMBER_6, MPU_SUB_REGION_ALL, ENABLE);
}

/*****************************************FLASH HARDWARE PROTECT*********************************************/
/*用于野指针非法写flash或异常断电时写地址突变的保护。即因为低压造成写地址突变为前半部分区域，而该区域为代码段、原始NV等重要区域*/
__RAM_FUNC void flash_hardware_protect_enable(uint32_t val)
{
	 osCoreEnterCritical();

#if RUNTIME_DEBUG
		extern uint32_t xy_runtime_get_enter(void);
		uint32_t time_enter = xy_runtime_get_enter();
#endif

	 FLASH_SetProtectMode((uint8_t)(val&0xFF), (uint8_t)((val >> 8)&0xFF));

#if RUNTIME_DEBUG
	 	extern void xy_runtime_get_exit(uint32_t id, uint32_t time_enter);
		xy_runtime_get_exit(49, time_enter);
#endif

	 osCoreExitCritical();
}

/*临时关闭前半区域的擦写保护，通常为RF NV写/FOTA升级擦等特殊操作*/
__RAM_FUNC uint32_t flash_hardware_protect_disable(void)
{

	 flash_protect protect;

	 osCoreEnterCritical();

#if RUNTIME_DEBUG
		extern uint32_t xy_runtime_get_enter(void);
		uint32_t time_enter = xy_runtime_get_enter();
#endif

	 protect = FLASH_ProtectDisable();

#if RUNTIME_DEBUG
	 	extern void xy_runtime_get_exit(uint32_t id, uint32_t time_enter);
		xy_runtime_get_exit(50, time_enter);
#endif

	 osCoreExitCritical();

	 return (protect.cmp << 8) | protect.mode;
}

/*查询flash_hardware_protect功能是否开启*/
typedef enum
{
	hardware_protect_disable = 0,
	hardware_protect_enable
}fhp_status_t;

__RAM_FUNC void flash_hardware_protect_status(uint32_t* data)
{
	osCoreEnterCritical();

	unsigned int flash_rdid = 0;
	flash_protect protect ={0};
	fhp_status_t ret = hardware_protect_enable;

	flash_rdid = FLASH_GetDeviceID();

	protect.mode=FLASH_GetProtectMode();
	protect.cmp=FLASH_GetProtectCmp();

	//4m
	if(flash_rdid == 0xC86516)
	{
		if((protect.cmp == 0 && (protect.mode & 7) == 0) || (protect.cmp == 1 && (protect.mode & 7) == 7))
		{
			ret = hardware_protect_disable;
		}
	}
	else
	{
		if((protect.cmp == 0 && (protect.mode & 7) == 0) || (protect.cmp == 1 && (protect.mode & 6) == 6))
		{
			ret = hardware_protect_disable;
		}
	}

	*data = ret;

	osCoreExitCritical();
}

volatile int g_protect_enable_always = 1;
void flash_hardware_protect_set_always(int enable)
{
	/*FOTA下载期间设置为0，以防止每次写FLASH时频繁进行FLASH寄存器操作影响性能*/
	g_protect_enable_always = enable;
}


/*****************************************FLASH INTERFACE PROTECT*********************************************/

/*xy_flash.h中的擦写接口内部调用，检查入参地址的合法性，禁止擦写代码段/RFNV等特殊区域。*/
void flash_write_protect(uint32_t base_addr,uint32_t size)
{
	/*仅供芯翼指定业务调用，如RF NV写/FOTA升级擦等特殊写要求*/
	if(flash_protect_enable == 0)
	{
		return;
	}

	/*RF区域正常情况禁止写入*/
	if(base_addr+size >= NV_FLASH_RF_BASE && base_addr < (NV_FLASH_RF_BAKUP_BASE + NV_FLASH_RF_SIZE))
	{
		xy_assert(0);
	}

	/*禁止写代码段*/
	if((HWREGB(AONPRCM_BASE + 0x01) & 0x01) ) //CP Remap
	{
		/*禁止写代码段*/
		if(base_addr >= FLASH_CP_REMAP_BASE && base_addr < OTA_FLASH_BASE())
		{
			xy_assert(0);
		}
	}
	else
	{
		/*禁止写代码段*/
		if(base_addr >= FLASH_CP_BASE && base_addr < OTA_FLASH_BASE())
		{
			xy_assert(0);
		}
	}
}

//与flash_interface_protect_disable配套使用
void flash_interface_protect_enable(void)
{
	xy_assert(flash_protectNesting);
	flash_protectNesting--;

	if(flash_protectNesting == 0)
	{
		flash_protect_enable = 1;
	}
}

/*RF NV写/FOTA升级擦等特殊擦写动作，需要临时关闭flash_write_protect的写保护*/
void flash_interface_protect_disable(void)
{
	flash_protect_enable = 0;
	flash_protectNesting++;
}
