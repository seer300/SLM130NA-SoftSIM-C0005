/*
 * mpu_protect.h
 *
 *  Created on: 2022年8月16日
 *      Author: jiangzj
 */

#ifndef SYSAPP_FLASH_INC_FLASH_PROTECT_H_
#define SYSAPP_FLASH_INC_FLASH_PROTECT_H_

#include "xy_memmap.h"

#ifdef __cplusplus
 extern "C" {
#endif

#if FLASH_2M
	#define FLASH_HARDWARE_PROTECT_LIMIT  (FLASH_BASE + 0x100000) //值是根据flash状态寄存器里面的值算出来的
#else
	#define FLASH_HARDWARE_PROTECT_LIMIT  (FLASH_BASE + 0x300000)
#endif

void mpu_protect_init(void);
void mpu_flash_access_enable(void);
void mpu_flash_access_disable(void);






/*禁止任何的flash访问。通常用于中断函数的flash访问检查，例如flash擦除操作进行过程中*/
void Flash_mpu_Lock(void);
void Flash_mpu_Unlock(void);




/*用于野指针非法写flash或异常断电时写地址突变的保护*/
void flash_hardware_protect_enable(uint32_t val);

/*临时关闭前半区域的擦写保护，通常为RF NV写/FOTA升级擦等特殊操作*/
uint32_t flash_hardware_protect_disable(void);

/*查询flash_hardware_protect功能是否开启*/
void flash_hardware_protect_status(uint32_t* data);
/*通常用于某个业务流程中长时间关闭FLASH硬件保护能力，如FOTA下载期间，以防止频繁进行FLASH寄存器操作影响性能*/
void flash_hardware_protect_set_always(int enable);


/*xy_flash.h中的擦写接口内部调用，检查入参地址的合法性，禁止擦写代码段/RFNV等特殊区域。*/
void flash_write_protect(uint32_t base_addr,uint32_t size);





/*RF NV写/FOTA升级擦等特殊擦写动作，需要临时关闭flash_write_protect的写保护*/
void flash_interface_protect_disable(void);
void flash_interface_protect_enable(void);


#ifdef __cplusplus
 }
#endif

#endif /* SYSAPP_FLASH_INC_FLASH_PROTECT_H_ */
