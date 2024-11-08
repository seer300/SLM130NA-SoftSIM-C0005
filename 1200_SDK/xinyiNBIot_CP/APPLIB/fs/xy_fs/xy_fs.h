/** 
* @brief   提供两个文件分区，芯翼使用的是FS_DEFAULT，flash地址范围为FS_FLASH_BASE+FS_FLASH_LENGTH，仅用于云业务及基础平台，不能用于用户数据保存。\n
*   提供了FS_USER分区，供客户进行私有大文件数据的保存，如OTA、采集数据等，flash地址范围为USER_LFS_FLASH_BASE+USER_LFS_FLASH_LEN值，目前默认关闭，用户自行打开。
*
* @attention  如果用户有更大的文件空间需求，可以考虑借用FOTA的flash区域(xy_OTA_flash_info)，但是不保证文件不丢失，因为FOTA升级/死机等都可能擦除该区域。
*
*/

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
#include "fs_al.h"

typedef fs_file_t * xy_file;

/**
 * @brief 打开文件
 * @param fileName 全路径文件名，一般可以只写文件名不写路径（文件放在根目录下），目录用‘/’分割
 * @param mode 
                r   以只读方式打开文件，该文件必须存在，不存在返回NULL，如果写，lfs内部将断言
                rb  同 r
                r+  以读/写方式打开文件，该文件必须存在，不存在返回NULL
                rb+ 同 r+
                w   以只写方式打开文件，若文件不存在则创建该文件，如果读，lfs内部将断言
                wb  同 w
                w+  以读/写方式打开文件，若文件不存在则创建该文件
                wb+ 同 w+
                a   以追加方式打开文件，文件是只写的，若文件不存在，则会创建该文件，如果文件存在，则写入的数据会被加到文件尾后，如果读，lfs内部将断言
                ab  同 a
                a+  以追加方式打开文件，文件是可读可写的，若文件不存在，则会创建该文件，如果文件存在，则写入的数据会被加到文件尾后
                ab+ 同 a+
 * @param diskType  FS_DEFAULT-内部默认使用; FS_USER-用户使用	         
 * @return 	   成功返回文件结构体指针，失败返回NULL
 */
xy_file xy_fopen(const char * fileName, const char * mode, uint32_t diskType);

/**
 * @brief 关闭文件
 * @param fp 文件结构体指针
 * @return 	   成功返回0，失败返回负数
 * @attention  调用该接口或者xy_fsync后才能保证写入的数据被真正写入flash
 */
int32_t xy_fclose(xy_file fp);

/**
 * @brief 读文件
 * @param fp 文件结构体指针
 * @param buffer 读取数据存放的内存地址
 * @param size buffer字节数
 * @return 	   正数-实际读取数据的长度；负数-见 lfs_error
 * @attention  读取后文件位置指针自动后移，配合seek可以跳读任意偏移地址的数据
 */
int32_t xy_fread(void *buf, uint32_t size, xy_file fp);

/**
 * @brief 写文件
 * @param fp 文件结构体指针
 * @param buffer 待写数据存放的内存地址
 * @param size buffer字节数
 * @return 	   正数-实际写入数据的长度；负数-见 lfs_error
 * @attention  如果修改旧文件，写入前需先用seek将文件位置指针指到目的偏移地址处
 */
int32_t xy_fwrite(void *buf, uint32_t size,xy_file fp);

/**
 * @brief 写文件
 * @param fp 文件结构体指针
 * @param buffer 待写数据存放的内存地址
 * @param size buffer字节数
 * @return 	   正数-实际写入数据的长度；负数-见 lfs_error
 * @attention  该函数主要用作NV保存数据，内部会读取数据对比，数据不同才写入
 */
int32_t xy_fwrite_safe(void *buf, uint32_t size, xy_file fp);

/**
 * @brief 查询文件位置指针
 * @param fp 文件结构体指针
 * @return 	   文件位置指针（相对于文件头的偏移）
 * @attention  
 */
int32_t xy_ftell(xy_file fp);

/**
 * @brief 设置文件位置指针
 * @param fp 由用户申请和维护的文件结构体
 * @param offset 需要设置的偏移量，正数-向后偏移，负数-向前偏移
 * @param seekType 0-从文件头偏移；1-从当前位置偏移；2-从文件尾偏移
 * @return 	   文件相对头的偏移，失败返回负数
 * @attention  
 */
int32_t xy_fseek(xy_file fp, int32_t offset, int32_t seekType);

/**
 * @brief 强制将数据写入flash
 * @param fp 文件结构体指针
 * @return 	   成功返回0，失败返回负数
 * @attention  调用该接口或者xy_fclose后才能保证写入的数据被真正写入flash
 */
int32_t xy_fsync(xy_file fp);

/**
 * @brief 查询文件大小
 * @param fp 文件结构体指针
 * @return 	   文件大小（字节为单位）
 * @attention  
 */
int32_t xy_fsize(xy_file fp);

/**
 * @brief 删除文件
 * @param path 全路径文件名，根目录下的文件可以只写文件名
 * @param diskType  FS_DEFAULT-内部默认使用; FS_USER-用户使用	
 * @return 	   成功返回0，失败返回负数
 * @attention  
 */
int32_t xy_fremove(const char *fileName, uint32_t diskType);



/**
 * @brief 专供Before_DeepSleep_Hook内部调用，用于深睡过程中保存文件系统，不得用于普通场景
 * @attention  
 */
xy_file lpm_fs_fopen(const char * fileName, const char * mode, uint32_t diskType);
int32_t lpm_fs_fwrite(void *buf, uint32_t len, xy_file fp);
int32_t lpm_fs_fread(void *buf, uint32_t len, xy_file fp);
int32_t lpm_fs_fclose(xy_file fp);

