#ifndef __XY_FS_USER_CONFIG_H__
#define __XY_FS_USER_CONFIG_h__

#include "xy_memmap.h"
/*默认配置，不建议修改*/
#define USER_LFS_BLOCK_DEVICE_READ_SIZE      (16)       // 设置read_size
#define USER_LFS_BLOCK_DEVICE_PROG_SIZE      (16)       //设置prog_size
#define USER_LFS_BLOCK_DEVICE_CACHE_SIZE     (512)      //小于等于cache_size的文件直接存放到entry中，当小文件较多时可以适当增大这个值来提高flash的利用率
		                                                //文件系统挂载后会为读写各分配一个cache_size大小的buffer，所以这个值增大需要更多的ram                                                       
#define USER_LFS_BLOCK_DEVICE_ERASE_SIZE     (4096)     //一次性擦除的大小也是分配块的大小
#define USER_LFS_BLOCK_DEVICE_LOOK_AHEAD     (8)        //bitmap的大小lookahead_size*8，滑窗大小
#define USER_LFS_BLOCK_REVISION_NUM          (200)      //块擦写的阈值
 
/*若用户需要单独使用文件系统，请从xy_memmap.h头文件的USER_FLASH_BASE中挤出flash空间使用*/
#define USER_LFS_FLASH_BASE        USER_FLASH_BASE+USER_FLASH_LEN_MAX
#define USER_LFS_FLASH_LEN         (0)



#endif	/* end of __DUMP_H__ */
