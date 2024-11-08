/*
 * Copyright (c) 2022 LinJiajun.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "sys_debug.h"
#include "cmsis_device.h"
#include "dump.h"
#include "MPU.h"
#include <stdio.h>
#include "attribute.h"
#include "ipc_msg.h"
#include "hw_types.h"
#include "xy_memmap.h"
#include "watchdog.h"
#include "dfe.h"

volatile char     *assert_file = NULL;
volatile int       assert_line = 0;
volatile uint16_t  assert_primask = 0;
volatile uint32_t  assert_send_buff[2] = {0};

enum{
	cp_dumplogvi_status_idle,  //告知ap核准备导dump状态
	cp_dumplogvi_status_start, //告知ap核处于导dump状态
	cp_dumplogvi_status_reset, //告知ap核处于等待ap核进行复位状态
};

enum{
	ap_dumpflash_status_start,     //告知cp核暂时不允许导dump
	ap_dumpflash_status_idle,      //告知cp核可以准备导dump状态
	ap_dumpflash_status_reqreset,  //告知cp核准备复位
};

typedef struct{
	uint8_t ap_dumpflash_status:4;  //默认ap_dumpflash_status_start状态
	uint8_t cp_dumplogvi_status:4;  //默认cp_dumplogvi_status_idle状态
}dump_sync_t;

__RAM_FUNC void dump_sync(void)
{
	volatile dump_sync_t* dump_sync = (dump_sync_t*)BAK_MEM_DUMP_LOGVIEW_FLAG;

	xy_assert(dump_sync->cp_dumplogvi_status == cp_dumplogvi_status_start);

	if(dump_sync->ap_dumpflash_status == ap_dumpflash_status_start)
	{
		dump_sync->cp_dumplogvi_status = cp_dumplogvi_status_idle;
		while(dump_sync->ap_dumpflash_status != ap_dumpflash_status_idle)
		dump_sync->cp_dumplogvi_status = cp_dumplogvi_status_start;
	}
	else if(dump_sync->ap_dumpflash_status == ap_dumpflash_status_reqreset)
	{
		dump_sync->cp_dumplogvi_status = cp_dumplogvi_status_reset;
		while(1);
	}
}

__RAM_FUNC void send_assert_info_to_AP()
{
	volatile dump_sync_t* dump_sync = (dump_sync_t*)BAK_MEM_DUMP_LOGVIEW_FLAG;
    extern unsigned int Address_Translation_CP_To_AP(unsigned int addr);
    assert_send_buff[0] = (uint32_t)Address_Translation_CP_To_AP((unsigned int)assert_file);
    assert_send_buff[1] = assert_line;
    HWREG(BAK_MEM_CP_ASSERT_INFO) = (uint32_t)Address_Translation_CP_To_AP((unsigned int)assert_send_buff); 
    HWREGB(BAK_MEM_CP_DO_DUMP_FLAG) = 1;
	//通知AP核，cp死机，APwhile等待

    dump_sync->cp_dumplogvi_status = cp_dumplogvi_status_idle;
	Ipc_SetInt();
	while(dump_sync->ap_dumpflash_status != ap_dumpflash_status_idle);
	dump_sync->cp_dumplogvi_status = cp_dumplogvi_status_start;
//	while(HWREGB(BAK_MEM_DUMP_LOGVIEW_FLAG) != 1);
}

/******************************************************************************
 * @brief  : This function take the initiative to assert
 * @param  : file: assert file's name
 *           line: assert file's line
 * @retval : None
 *****************************************************************************/

__RAM_FUNC void sys_assert_proc(char *file, int line)
{
    /* sava assert message to global */
    assert_file = file;
    assert_line = line;

    assert_primask = __get_PRIMASK();

    __set_PRIMASK(1);

    DumpRegister_from_Normal();


    WatchdogDisable(CP_WDT_BASE);

    rf_trx_close(); //关闭射频，防止射频工作时间过长

    //ap核死机，此时ap核处于dump流程中，调用此函数也不会异常
    send_assert_info_to_AP();


#if (__MPU_PRESENT == 1U)
    MPU_Cmd(DISABLE);
#endif

#if (ASSERT_DUMP_MEMORY_ENABLE == 1)
    Dump_Memory_to_File();
#endif

    /* Do not use "while(1)" directly, otherwise the compiler will assume that
    the function will not return, and some callee registers will not be pushed
    when assembly code is generated and will be overwritten directly, resulting
    in scene damaged. */
    while(1)
    	dump_sync();
    while(line != 0);
}
