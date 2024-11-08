/*
 * flash_protect.c
 *
 *  Created on: 2022年8月15日
 *      Author: jiangzj
 */
#include "mpu.h"
#include "flash_vendor.h"
#include "system.h"
#include "core_cm3.h"
#include "dyn_load.h"

#if defined(__CC_ARM)
extern uint32_t Image$$ER_RAM$$Base;
extern uint32_t Image$$ER_RAM1$$RW$$Base;
#elif defined(__GNUC__)
extern uint32_t __vectors_start; // Defined by the linker.
extern uint32_t __data_start__;  // Defined by the linker.
#endif


/*对版本的分区和段属性添加MPU保护，以防止误操作flash某特殊内存，如往某代码段写数据。通过xy_flash.h进行擦写的保护，参见is_allowed_opt_addr接口*/
/*通过FLASH_SetProtectMode接口实现了flash寄存器的内存保护模式，即保护flash的前半段禁止写入*/
void Mpu_Protect_Init(void)
{
#if defined(__CC_ARM)
    /* calculate data for protect text section in ram. */
    uint32_t text_start = (uint32_t)&Image$$ER_RAM$$Base;
    uint32_t text_size = (uint32_t)&Image$$ER_RAM1$$RW$$Base - text_start;
#elif defined(__GNUC__)
    /* calculate data for protect text section in ram. */
    uint32_t text_start = (uint32_t)&__vectors_start;
    uint32_t text_size = (uint32_t)&__data_start__ - text_start;
#endif
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

#if defined(__CC_ARM)
    /* calculate data for protect data, bss, heap and stack section in ram. */
    uint32_t data_start = (uint32_t)&Image$$ER_RAM1$$RW$$Base;
#elif defined(__GNUC__)
    /* calculate data for protect data, bss, heap and stack section in ram. */
    uint32_t data_start = (uint32_t)&__data_start__;
#endif
    uint32_t data_protect_end = TCM_AP_LIMITED;

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
    uint32_t RegionConfig0, RegionConfig1, RegionConfig2, RegionConfig3, RegionConfig4, RegionConfig7, RegionConfig5, RegionConfig6;
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
    

    RegionConfig6 = MPU_REGION_CONFIG_XN_INSTRUCTION_ACCESS_DISABLE | \
					MPU_REGION_CONFIG_AP_READ_ONLY | \
					MPU_REGION_CONFIG_TEX_S_C_B_OUTER_INNER_WRITEBACK_NO_WRITE_ALLOCATE;

	(void)RegionConfig5;
	MPU_Cmd(DISABLE);

	/* global memory protect. */
	MPU_RegionInit(MPU_REGION_NUMBER_0, 0x0, MPU_REGION_LENGTH_4G, RegionConfig0);  //配置所有地址可执行，可读写
	/* text section protect. */
	MPU_RegionInit(MPU_REGION_NUMBER_1, text_protect_addr, (text_protect_size - 1), RegionConfig1);  //保护AP核位于RAM区域的代码段不可写，flash区域的不可写参见is_allowed_opt_addr接口
	MPU_SubregionCmd(MPU_REGION_NUMBER_1, sub_region_num, DISABLE);
	/* data, bss, heap and stack section protect. */
	MPU_RegionInit(MPU_REGION_NUMBER_2, data_protect_addr, (data_protect_size - 1), RegionConfig2);  //设置AP核RAM的数据段(data/bss)不可执行

	/* isr_vector section protect. */
	//MPU_RegionInit(MPU_REGION_NUMBER_7, (uint32_t)g_pfnVectors, MPU_REGION_LENGTH_64B, RegionConfig7);
	/* flash protect, enable flash access in initialise, disable in special scenario. */
	MPU_RegionInit(MPU_REGION_NUMBER_3, FLASH_CP_BASE, MPU_REGION_LENGTH_4M, RegionConfig3);   //AP核不容许直接写flash，如memset；但可读，如if(*(addr) == 1)

	MPU_RegionInit(MPU_REGION_NUMBER_5, FLASH_CP_BASE, MPU_REGION_LENGTH_4M, RegionConfig5);   //设置flash的读写/运行保护区域，默认关闭。通常中断函数被执行时通过Flash_mpu_Lock来开启保护

	MPU_SubregionCmd(MPU_REGION_NUMBER_5, MPU_SUB_REGION_ALL, DISABLE);

    /* rom code protect, 96K from 0x20100000. */
    MPU_RegionInit(MPU_REGION_NUMBER_4, 0x00000000, MPU_REGION_LENGTH_128K, RegionConfig4);   //对一级boot的代码保护，禁止写。即零地址访问受限
    MPU_SubregionCmd(MPU_REGION_NUMBER_4, MPU_SUB_REGION_NUMBER_6 | MPU_SUB_REGION_NUMBER_7, DISABLE);

	/* 禁止ap核写CP核的cpram以及sharemem */
#if (DYN_LOAD == 0)
    MPU_RegionInit(MPU_REGION_NUMBER_6, RAM1_DMA_BASE, MPU_REGION_LENGTH_128K, RegionConfig6);  //禁止ap核写CP核的cpram以及sharemem
    MPU_SubregionCmd(MPU_REGION_NUMBER_6, MPU_SUB_REGION_NUMBER_7, DISABLE);
	MPU_RegionInit(MPU_REGION_NUMBER_7, SHARE_RAM0_BASE, MPU_REGION_LENGTH_64K, RegionConfig6);
#else
    MPU_RegionInit(MPU_REGION_NUMBER_6, RAM1_DMA_BASE, MPU_REGION_LENGTH_128K, RegionConfig6);  //禁止ap核写CP核的cpram，但容许写64K的sharemem
    MPU_SubregionCmd(MPU_REGION_NUMBER_6, MPU_SUB_REGION_NUMBER_7, DISABLE);
#endif

    MPU_Control(MPU_CONTROL_PRIVILEGED_DEFAULT_MEMORY_MAP, ENABLE);
    MPU_Cmd(ENABLE);

	(void)RegionConfig7;




}

#if (MPU_EN == 1)

extern void DisablePrimask(void);
extern void EnablePrimask(void);

/*禁止执行FLASH上的代码，常用于中断服务程序或秒级周期性事件处理*/
void Flash_mpu_Lock(void)
{
    DisablePrimask();
            
    // mpu_isr_access_flash_check_enable();
    /* disable flash execute code and access flash data. */
    MPU_SubregionCmd(MPU_REGION_NUMBER_5, MPU_SUB_REGION_ALL, ENABLE);

    __DSB();
    __ISB();

    EnablePrimask();
}

void Flash_mpu_Unlock(void)
{
    DisablePrimask();

    // mpu_isr_access_flash_check_disable();
    /* enable flash execute code and access flash data. */
    MPU_SubregionCmd(MPU_REGION_NUMBER_5, MPU_SUB_REGION_ALL, DISABLE);

    __DSB();
    __ISB();

    EnablePrimask();
}
#else
/*由于该函数用于检查代码位置区异常的，且没有考虑原子操作，进而发货版本默认关闭*/
void Flash_mpu_Lock(void)
{
}

void Flash_mpu_Unlock(void)
{
}
#endif
