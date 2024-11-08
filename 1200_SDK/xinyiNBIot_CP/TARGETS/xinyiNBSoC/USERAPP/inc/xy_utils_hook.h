/**
* @file    xy_utils_hook.h
* @brief   芯翼提供的供用户重新实现的模块工具级弱函数声明，根据自身需求实现需要的弱函数即可
*/

#pragma once

#include "xy_system.h"

/********************************** 版本相关 **********************************/
/* 设备型号，用户可自行更改 */
#define PRODUCT_NAME         "SLM130-NA"

#define VENDOR_NAME          "MeiG"
#define MODULE_CHIP_NAME     "XY1200S"
#define PRODUCT_VER PRODUCT_NAME
#define MODULE_VER_STR  PRODUCT_NAME


#define BOARD_PATCH      	 "1.2.0.5"
#define VERSION_DEV_TYPE	 "M019"

#define BUILD_DATE           "1028"
#define VERSION_INFO_NEW	 "05"


/********************************** 云业务相关 **********************************/
/**
 * @brief 云平台获取设备厂商信息函数
 * @param manufacturer[OUT] 厂商信息
 * @param size[IN]  信息长度
 */
void user_get_Manufacturer(char *manufacturer, int size);

/**
 * @brief 云平台获取设备SN的函数
 * @param serial_number[OUT] SN字符串数据
 * @param size[IN] SN长度
 */
void user_get_SNumber(char *serial_number, int size);

/**
 * @brief 云平台获取模组芯片型号
 * @return 字符串数据 
 * @note
 */
void user_get_chiptype(char *chiptype, int size);

/**
 * @brief   厂商信息
 * @param   modul_ver  存储modul_ver的内存buf，由调用者分配，buf长度不小于20；  
 * @param   len     modul_ver的内存长度，不小于20
**/
bool user_get_MODULVER(char *modul_ver, int len);

/**
 * @brief   软件版本号
 * @param   versionExt  存储versionExt的内存buf，由调用者分配，buf长度不小于28
 * @param   len   versionExt的内存长度，不小于28
**/
bool user_get_VERSIONEXT(char *versionExt, int len);

/**
 * @brief   硬件版本号
 * @param   hardver  存储hardver的内存buf，由调用者分配，buf长度不小于20；
 * @param   len    hardver的内存长度，不小于20
**/
bool user_get_HARDVER(char *hardver, int len);


/********************************** 驱动相关 **********************************/

/**
 * @brief   获取电池电量的回调接口
 * @return int    mAh
 * @warning 用户必须实现对电池电量的监控，并提供指示灯等方式提示更换电池 \n
 *     对于FOTA等耗时长的关键动作，必须检测电量是否足够完成该事务 
 * @note  该接口由用户二次开发实现，目前芯翼平台在@ref xy_is_enough_Capacity接口中会调用，以裁决当前电量是否足够FOTA升级。\n
 *  用户根据自身的业务开发，也可以在适当点调用获取当前电量。
 */
unsigned int xy_getVbatCapacity();

/**
 * @brief  检测电池电量是否足够的回调接口，常见于FOTA等特殊流程
 * @param state   unused
 * @warning  该接口必须配置出厂NV(min_mah)一起使用。目前该接口仅在FOTA升级之前调用，若发现电量不足，则平台会放弃此次升级。\n
 *  用户根据自身的业务开发，也可以适当调用该接口，以决定是否能执行某高功耗的动作。
 */
bool xy_is_enough_Capacity(int state);




