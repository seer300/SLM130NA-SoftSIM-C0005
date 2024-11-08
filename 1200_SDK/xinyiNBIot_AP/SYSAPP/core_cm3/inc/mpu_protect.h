/*
 * flash_protect.h
 *
 *  Created on: 2022年8月16日
 *      Author: jiangzj
 */

#pragma once



#ifdef __cplusplus
 extern "C" {
#endif

void Mpu_Protect_Init(void);




/**
 * @brief  仅用于调试！禁止访问flash(XIP/读写等)。
 * @warning  必须与Flash_mpu_Unlock配对使用。以排查中断中运行FLASH代码，或者毫秒级周期性事件中运行FLASH代码
 */
void Flash_mpu_Lock(void);

/**
 * @brief  仅用于调试！容许访问flash(XIP/读写等)。
 * @warning  必须与Flash_mpu_Lock配对使用。以排查中断中运行FLASH代码，或者毫秒级周期性事件中运行FLASH代码
 */
void Flash_mpu_Unlock(void);

#ifdef __cplusplus
 }
#endif


