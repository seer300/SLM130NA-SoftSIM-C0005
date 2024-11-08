/*
 * lpm_nv_write.h
 *
 *  Created on: 2024年6月13日
 *      Author:
 */

#ifndef SYSAPP_FLASH_INC_LPM_NV_WRITE_H_
#define SYSAPP_FLASH_INC_LPM_NV_WRITE_H_

#include "stdint.h"

#define LPM_NV_WRITE_NUM ( 5 )
typedef struct
{
	uint8_t lpm_nv_write_num;
	struct
	{
		uint32_t     dest_addr; /*flash*/
		uint32_t     src_addr; /*RAM*/
	    uint32_t     size;
	}lpm_nv_write_buff[LPM_NV_WRITE_NUM];
}lpm_nv_write_t;

void lpm_nv_write_buff_add(uint32_t dest_addr, uint32_t src_addr, uint32_t size);
void lpm_nv_write_buff_remove_all(void);
void lpm_nv_write_init(void);
#endif /* SYSAPP_FLASH_INC_LPM_NV_WRITE_H_ */
