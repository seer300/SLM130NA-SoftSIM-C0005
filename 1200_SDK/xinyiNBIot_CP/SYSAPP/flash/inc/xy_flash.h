
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
#include "at_utils.h"
#include "cmsis_os2.h"
#include "flash_adapt.h"
#include "xy_ftl.h"
#include "xy_fota.h"


/*opencpu形态，禁止实时写FLASH，仅AP单核或SoC深睡时容许写，FOTA除外*/
#define NOT_ALLOWED_SAVE_FLASH()   (HWREGB(BAK_MEM_BAN_WRITE_FLASH) == 1)



/*****************************************************************************************************************************
 * @brief 从Flash读取数据，一个扇区4K字节，耗时183us
 * @param addr flash地址
 * @param data RAM地址
 * @param size 读取长度
 ****************************************************************************************************************************/
/*****************************************************************************************************************************
 * @brief Read data from flash, 4K bytes per sector, takes 183us
 * @param addr: flash address
 * @param data: RAM address
 * @param size: length
 ****************************************************************************************************************************/
bool xy_Flash_Read(uint32_t addr, void *data, uint32_t size);

 

/*****************************************************************************************************************************
 * @brief 向Flash写入数据.内部先读后写，保证不改变同一扇区内其他位置的数据内容。一个扇区4K字节，耗时54ms
 * @param addr  flash地址
 * @param data  RAM地址
 * @param size  长度
 * @note 由于异常断电会造成Flash内容损坏，所以客户必须自行添加头尾校验位来确保内容的有效性. 
 ****************************************************************************************************************************/
/*****************************************************************************************************************************
 * @brief Write data to flash. And ensure that the data content of other positions in the same sector is not changed. 4K bytes in one sector, takes 54ms
 * @param addr: flash address
 * @param data: RAM address
 * @param size: length
 * @note Due to the damage of flash content caused by abnormal power down, user must add check info to ensure the validity of the content
 ****************************************************************************************************************************/
bool xy_Flash_Write(uint32_t addr,void *data, uint32_t size);


/*****************************************************************************************************************************
 * @brief 直接向Flash指定位置写入数据.一个扇区4K字节，耗时9ms.
 * @param addr  flash地址
 * @param data  RAM地址
 * @param size  长度
 * @note 由用户先通过xy_Flash_Erase接口执行大面积擦除动作后，再通过该接口顺序分步写入内容。通常用于FOTA差分包保存/dump信息保存等特殊流程。
 ****************************************************************************************************************************/
/*****************************************************************************************************************************
 * @brief Write data directly to the designated location of flash. A sector of 4K bytes, takes 9ms
 * @param addr: flash address
 * @param data: RAM address
 * @param size: length
 * @note  User must call xy_Flash_Erase to erase,then call this API to write content step by step. It is usually used for special processes such as FOTA  packet storage,dump info storage...
 ****************************************************************************************************************************/
bool xy_Flash_Write_No_Erase(uint32_t addr,void *data, uint32_t size);



/*****************************************************************************************************************************
 * @brief 擦除flash，一个扇区4K字节，耗时45ms
 * @param addr flash地址，需4K字节对齐
 * @param size 擦除flash的长度，传入长度如果不是4K整数倍则强制向上对齐到4K整数倍
 ****************************************************************************************************************************/
/*****************************************************************************************************************************
 * @brief Erasing flash, 4K bytes per sector, takes 45ms
 * @param addr: flash address, 4K byte alignment required
 * @param size: length,If it is not a 4K integer multiple, it is forced to align up to a 4K integer multiple
 ****************************************************************************************************************************/
bool xy_Flash_Erase(uint32_t addr, uint32_t size);



