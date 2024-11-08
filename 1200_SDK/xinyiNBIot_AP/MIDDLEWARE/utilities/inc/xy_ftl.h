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

#include "hal_def.h"
#include "stdint.h"
#include "xy_flash.h"
#include "factory_nv.h"
#include "xy_memmap.h"
#include "sys_rc32k.h"

/*****************************************************************************************************************************
 * @brief  用户可用的单个FTL块的有效flash内存空间(4084字节)，用户不得扩展该范围
 ****************************************************************************************************************************/
#define XY_FTL_AVAILABLE_SIZE           (FLASH_SECTOR_LENGTH - 8 - 4)




/*****************************************************************************************************************************
 * @brief  注册一个FTL块。若len入参设置为一个扇区大小(FLASH_SECTOR_LENGTH),则意味着放弃磨损功能，仅起到有效性校验功能。
 * @param  addr: FTL块的flash起始地址,用户仅能从USER_FLASH_BASE中抠取一块出来使用，不得越界
 * @param  len:  flash总长度，必须是FLASH_PAGE_SIZE块整数倍，值越大，坏块风险越小
 * @warning  芯翼平台使用的FTL，在fac_nv_param_read中进行注册
 ****************************************************************************************************************************/
void  xy_ftl_regist(void* addr, uint32_t len);


/*****************************************************************************************************************************
 * @brief  读FTL块内的数据，内部会进行有效性校验
 * @param  addr: 用户层面的虚拟flash地址，即flash基地址+用户数据内偏移
 * @param  data: 读取后的RAM存放地址
 * @param  size: 长度
 * @warning  最大不得超过XY_FTL_AVAILABLE_SIZE字节
 ****************************************************************************************************************************/
bool xy_ftl_read(uint32_t addr, void* data, uint32_t size);


/*****************************************************************************************************************************
 * @brief  写FTL块内的数据，内部会进行有效性校验
 * @param  addr: 用户层面的虚拟flash地址，即flash基地址+用户数据内偏移
 * @param  data: 待写入的RAM缓存地址
 * @param  size: 写入的数据长度，不可越界XY_FTL_AVAILABLE_SIZE
 * @warning  最大不得超过XY_FTL_AVAILABLE_SIZE字节
 ****************************************************************************************************************************/
bool xy_ftl_write(uint32_t addr, void* data, uint32_t size);


/*****************************************************************************************************************************
 * @brief  擦除某FTL块所有flash空间，其中addr为flash基地址
 ****************************************************************************************************************************/
void xy_ftl_erase(uint32_t addr);
