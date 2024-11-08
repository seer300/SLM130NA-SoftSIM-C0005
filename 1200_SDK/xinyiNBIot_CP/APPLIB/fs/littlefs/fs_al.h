#ifndef LFS_PORT_H
#define LFS_PORT_H

#include "lfs.h"


//文件系统分区,默认为第一分区
#define FS_DEFAULT						1
#define FS_USER							2

typedef struct _fs_file{
	lfs_file_t file;
	char * filename;
    uint32_t diskType;
}fs_file_t;

typedef struct lfs_dir fs_dir_t;
typedef struct lfs_info fs_info_t;
typedef struct _fs_stat
{
    uint32_t total_block;
    uint32_t block_used;
    uint32_t block_size;
} fs_stat_t;




void fs_config_init(uint32_t disktype);

void fs_lock_init(void );
/**
 * @brief 挂载文件系统
 * @return 	   见lfs_error
 * @attention  第一次挂载时会先对挂载区域的flash进行格式化
  */

int fs_mount(lfs_t *lfs, struct lfs_config * lfs_cfg);

/**
 * @brief 卸载文件系统
 * @return 	   见lfs_error
 * @attention  该接口一般不需要使用
 */
int fs_unmount(lfs_t *lfs, struct lfs_config * lfs_cfg);

/**
 * @brief 打开文件
 * @param file 由用户申请和维护的文件结构体
 * @param path 全路径文件名，一般可以只写文件名不写路径（文件放在根目录下），目录用‘/’分割
 * @param flags 见lfs_open_flags
 * @return 	   见lfs_error
 * @attention  通过flags控制文件打开后的读写属性，可通过flags传LFS_O_CREAT新建文件
 */
int fs_open(fs_file_t *file, const char *path, int flags);

/**
 * @brief 关闭文件
 * @param file 由用户申请和维护的文件结构体
 * @return 	   见lfs_error
 * @attention  调用该接口或者fs_sync后才能保证写入的数据被真正写入flash
 */
int fs_close(fs_file_t *file);

/**
 * @brief 读文件
 * @param file 由用户申请和维护的文件结构体
 * @param buffer 读取数据存放的内存地址
 * @param size 需要读取的数据长度
 * @return 	   正数-实际读取数据的长度；负数-见 lfs_error
 * @attention  读取后文件位置指针自动后移，配合seek可以跳读任意偏移地址的数据
 */
int fs_read(fs_file_t *file, void *buffer, uint32_t size);

/**
 * @brief 写文件
 * @param file 由用户申请和维护的文件结构体
 * @param buffer 待写数据存放的内存地址
 * @param size 需要写入的数据长度
 * @return 	   正数-实际写入数据的长度；负数-见 lfs_error
 * @attention  如果修改旧文件，写入前需先用seek将文件位置指针指到目的偏移地址处
 */
int fs_write(fs_file_t *file, const void *buffer, uint32_t size);

/**
 * @brief 同步文件
 * @param file 由用户申请和维护的文件结构体
 * @return 	   见lfs_error
 * @attention  调用该接口或者fs_close后才能保证写入的数据被真正写入flash
 */
int fs_sync(fs_file_t *file);

/**
 * @brief 设置文件位置指针
 * @param file 由用户申请和维护的文件结构体
 * @param off 需要设置的偏移量，正数-向后偏移，负数-向前偏移
 * @param whence 0-从文件头偏移；1-从当前位置偏移；2-从文件尾偏移
 * @return 	   见lfs_error
 * @attention  
 */
int fs_seek(fs_file_t *file, int off, int whence);

/**
 * @brief 查询文件位置指针
 * @param file 由用户申请和维护的文件结构体
 * @return 	   文件位置指针（相对于文件头的偏移）
 * @attention  
 */
int fs_tell(fs_file_t *file);

/**
 * @brief 查询文件大小
 * @param file 由用户申请和维护的文件结构体
 * @return 	   文件大小（字节为单位）
 * @attention  
 */
int fs_size(fs_file_t *file);

/**
 * @brief 设置文件大小
 * @param file 由用户申请和维护的文件结构体
 * @param size 设置的文件大小，如果小于原文件大小文件尾部数据被截掉；如果大于原文件大小文件尾部追加相应数量的0
 * @return 	   见lfs_error
 * @attention  
 */
int fs_truncate(fs_file_t *file, uint32_t size);

/**
 * @brief 删除文件
 * @param path 全路径文件名，根目录下的文件可以只写文件名
 * @return 	   见lfs_error
 * @attention  
 */
int fs_remove(const char *path, uint32_t diskType);

/**
 * @brief 获取文件系统flash的使用情况
 * @param stat 用户传入的结构体指针
 * @return 	   见lfs_error
 * @attention  
 */
int fs_statfs(fs_stat_t *stat);

/**
 * @brief 文件重命名
 * @param oldpath 旧文件名
 * @param newpath 新文件名
 * @return 	   见lfs_error
 * @attention  
 */
int fs_rename(const char *oldpath, const char *newpath);

/**
 * @brief 创建目录
 * @param path 目录名，根目录是“/”
 * @return 	   见lfs_error
 * @attention  
 */
int fs_mkdir(const char *path);

/**
 * @brief 打开目录
 * @param dir 由用户申请和维护的目录结构体
 * @param path 目录名，根目录是“/”
 * @return 	   见lfs_error
 * @attention  一般需要查询目录下都有哪些文件时用
 */
int fs_dir_open(fs_dir_t *dir, const char *path);

/**
 * @brief 读目录数据
 * @param dir 由用户申请和维护的目录结构体
 * @param info 返回的文件信息
 * @return 	   小于0-错误码见lfs_error；0-读失败；1-读成功
 * @attention  一般需要查询目录下都有哪些文件时用
 */
int fs_dir_read(fs_dir_t *dir, fs_info_t *info);

/**
 * @brief 关闭目录
 * @param dir 由用户申请和维护的目录结构体
 * @return 	   见lfs_error
 * @attention 
 */
int fs_dir_close(fs_dir_t *dir);

#endif
