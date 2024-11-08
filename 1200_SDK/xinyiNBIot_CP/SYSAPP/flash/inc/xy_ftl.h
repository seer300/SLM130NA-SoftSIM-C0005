/*****************************************************************************************************************************   
 * @brief  磨损算法FTL机制，实质是拿多个flash扇区来保存不到一个扇区(XY_FTL_AVAILABLE_SIZE)的内容，以提高flash寿命。  \n
 *         同时，该机制也对异常断电进行保护,通过头尾信息识别出哪个扇区是完整的有效内容。
 * @note   芯翼使用的flash寿命是10万次擦写门限，用户自行根据flash擦写频率及产品寿命周期计算合理的磨损总flash扇区。
 ****************************************************************************************************************************/
 
/*****************************************************************************************************************************	 
 * @brief  FTL mechanism of the wear-Leveling algorithm is to take multiple flash sectors to save the contents of less than one sector (xy_ftl_available_size) to improve the flash life.
 *	       And the mechanism also protects against abnormal power off, and identifies which sector is the complete effective content through the head and tail information.
 * @note   The flash threshold is 100000 times. The user can calculate the reasonable total flash sector according to the flash opt frequency and product life cycle.
 ****************************************************************************************************************************/

#pragma once
#include <ctype.h>
#include <float.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmsis_os2.h"
#include "xy_memmap.h"
#include "xy_flash.h"

typedef struct
{
	uint32_t     dest_addr;
	uint32_t     src_addr;
    uint32_t     size;
}lpm_nv_msg_t;

/*****************************************************************************************************************************
 * @brief  用户可用的单个FTL块的有效flash内存空间(4084字节)，用户不得扩展该范围
 ****************************************************************************************************************************/
 /*****************************************************************************************************************************
 * @brief  The effective flash memory space (4084 bytes) of a single FTL block, user can not exceed this range
 ****************************************************************************************************************************/
#define  XY_FTL_AVAILABLE_SIZE      (FLASH_SECTOR_LENGTH-4-8) // 0x1000-4-8




/*****************************************************************************************************************************
 * @brief  写FTL块内的数据，内部会进行有效性校验
 * @param  addr: 用户层面的虚拟flash地址，即flash基地址+用户数据内偏移
 * @param  data: 待写入的RAM缓存地址
 * @param  size: 写入的数据长度，不可越界XY_FTL_AVAILABLE_SIZE
 * @warning  最大不得超过XY_FTL_AVAILABLE_SIZE字节
 ****************************************************************************************************************************/
/*****************************************************************************************************************************
 * @brief  Write the data into the FTL block, and perform validity verification internally
 * @param  addr: Virtual flash address at user level, i.e. flash base address + internal offset of user data
 * @param  data: RAM  address
 * @param  size: length
 * @warning  Maximum must not exceed XY_FTL_AVAILABLE_SIZE bytes
 ****************************************************************************************************************************/
bool xy_ftl_write(uint32_t addr,uint8_t *data, uint32_t size);



/*****************************************************************************************************************************
 * @brief  读FTL块内的数据，内部会进行有效性校验
 * @param  addr: 用户层面的虚拟flash地址，即flash基地址+用户数据内偏移
 * @param  data: 读取后的RAM存放地址
 * @param  size: 长度
 * @warning  最大不得超过XY_FTL_AVAILABLE_SIZE字节
 ****************************************************************************************************************************/
/*****************************************************************************************************************************
 * @brief  Read the data from the FTL block, and verify the validity internally
 * @param  addr: Virtual flash address at user level, i.e. flash base address + internal offset of user data
 * @param  data: RAM  address
 * @param  size: length
 * @warning  Maximum must not exceed XY_FTL_AVAILABLE_SIZE bytes
 ****************************************************************************************************************************/
bool xy_ftl_read(uint32_t addr, uint8_t *data, uint32_t size);


/*****************************************************************************************************************************
 * @brief  擦除某FTL块所有flash空间，其中addr为flash基地址
 ****************************************************************************************************************************/
/*****************************************************************************************************************************
 * @brief  Erase all flash spaces of an FTL block, and addr param is the flash base address
 ****************************************************************************************************************************/
void xy_ftl_erase(uint32_t addr);



/*****************************************************************************************************************************
 * @brief  注册一个FTL块。若len入参设置为一个扇区大小(FLASH_SECTOR_LENGTH),则意味着放弃磨损功能，仅起到有效性校验功能。
 * @param  addr: FTL块的flash起始地址,用户仅能从USER_FLASH_BASE中抠取一块出来使用，不得越界
 * @param  len:  flash总长度，必须是FLASH_PAGE_SIZE块整数倍，值越大，坏块风险越小
 ****************************************************************************************************************************/
/*****************************************************************************************************************************
 * @brief  Register a FTL block. If the len parameter sets to a sector size (FLASH_SECTOR_LENGTH), it means that the wear-Leveling function is abandoned and only the validity verification function is performed.
 * @param  addr: The flash start address of the FTL block, must in the range of USER_FLASH_BASE to (USER_FLASH_BASE+USER_FLASH_LEN_MAX).
 * @param  len:	 Total length of flash, must be an integer multiple of FLASH_PAGE_SIZE. The larger the value, the smaller the risk of bad blocks
 ****************************************************************************************************************************/
void xy_ftl_regist(uint32_t addr, uint32_t size);



int ftl_read_write_num(uint32_t addr);

/**
 * @brief 仅芯翼内部PS使用，同步写入flash，通常为3GPP相关AT命令触发的写NV
 * @param addr 用户层面的虚拟flash地址，即flash基地址+用户数据内偏移
 * @param data 待写入的RAM缓存地址
 * @param size 写入的数据长度，不可越界XY_FTL_AVAILABLE_SIZE
 * @warning  最大不得超过XY_FTL_AVAILABLE_SIZE字节
 */
bool ftl_write_with_mutex(uint32_t addr, uint8_t *data, uint32_t size, osMutexId_t mutex_id);

/**
 * @brief 仅芯翼内部使用，异步写入flash。该接口执行完后，不保证数据已写入flash.常见于PS内部后台保存部分非易变NV
 * @param addr 用户层面的虚拟flash地址，即flash基地址+用户数据内偏移
 * @param data 待写入的RAM缓存地址
 * @param size 写入的数据长度，不可越界XY_FTL_AVAILABLE_SIZE
 * @warning  最大不得超过XY_FTL_AVAILABLE_SIZE字节
 */
bool ftl_write_with_mutex_async(uint32_t addr, uint8_t *data, uint32_t size, osMutexId_t mutex_id);

