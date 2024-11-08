/*
 * dump_flash.h
 *
 *  Created on: 2022年5月18日
 *      Author: jiangzj
 */

#ifndef ARCH_ASSERT_INC_DUMP_FLASH_H_
#define ARCH_ASSERT_INC_DUMP_FLASH_H_

#include "dma.h"
#include "xy_memmap.h"


void dump_to_flash(void);
void dump_flash_to_file(void);


#endif /* ARCH_ASSERT_INC_DUMP_FLASH_H_ */
