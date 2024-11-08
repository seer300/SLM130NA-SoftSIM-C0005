#pragma once

#include <stdint.h>
#include "system.h"

#if DYN_LOAD


void set_got_r9();

extern const int32_t SO_AVAILABLE;

extern const uint32_t HEAP_END;
extern const uint32_t SO_TEXT_LOAD_ADDR;
extern const uint32_t SO_TEXT_EXEC_ADDR1;
extern const uint32_t SO_TEXT_EXEC_ADDR2;
extern const uint32_t SO_TEXT_SIZE;
extern const uint32_t ELF_GOT_PLT1_LOAD_ADDR;
extern const uint32_t ELF_GOT_PLT2_LOAD_ADDR;
extern const uint32_t SO_GOT_PLT1_LOAD_ADDR;
extern const uint32_t SO_GOT_PLT_SIZE;

extern const uint32_t SO_GOT_MODIFIED;
extern const uint32_t SO_GOT1_LOAD_ADDR;
extern const uint32_t SO_GOT_EXEC_ADDR;
extern const uint32_t SO_GOT2_LOAD_ADDR;
extern const uint32_t SO_GOT_SIZE;
extern const uint32_t DYN_FOTA_FLASH_BASE;

extern const uint32_t SO_DATA_CHANGE_INFO_ADDR;
extern const uint32_t SO_DATA_CHANGE_INFO_COUNT;

/*0,无意义；1，SO文件夹内容放CPRAM运行；2：放flash运行*/
void Dyn_Switch(int index);

/*入参为函数指针赋值的地址空间，以便后台动态内存切换时更新对应的函数地址*/
void mark_dyn_addr(void *addr);
#else
#define mark_dyn_addr(addr)
#endif
