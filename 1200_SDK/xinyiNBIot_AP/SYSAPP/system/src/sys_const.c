#include <stdint.h>

#if DYN_LOAD

/*开启了so功能，但so目录中没有代码，运行时会死机，进而通过该全局表示是否有代码运行。下面这些值在编译时会被替换为真正取值。*/
const int32_t SO_AVAILABLE = 0;

const uint32_t HEAP_END = 1;
const uint32_t SO_TEXT_LOAD_ADDR = 2;
const uint32_t SO_TEXT_EXEC_ADDR1 = 3;
const uint32_t SO_TEXT_EXEC_ADDR2 = 30;
const uint32_t SO_TEXT_SIZE = 4;
const uint32_t ELF_GOT_PLT1_LOAD_ADDR = 5;
const uint32_t ELF_GOT_PLT2_LOAD_ADDR = 6;
const uint32_t SO_GOT_PLT1_LOAD_ADDR = 7;
const uint32_t SO_GOT_PLT_SIZE = 8;

const uint32_t SO_GOT_MODIFIED = 0;
const uint32_t SO_GOT1_LOAD_ADDR = 1;
const uint32_t SO_GOT_EXEC_ADDR = 10;
const uint32_t SO_GOT2_LOAD_ADDR = 2;
const uint32_t SO_GOT_SIZE = 3;
const uint32_t DYN_FOTA_FLASH_BASE = 4;

const uint32_t SO_DATA_CHANGE_INFO_ADDR = 0;
const uint32_t SO_DATA_CHANGE_INFO_COUNT = 0;
#endif