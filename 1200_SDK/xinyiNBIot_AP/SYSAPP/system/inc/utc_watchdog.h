/**
 ******************************************************************************
 * @file      utc_watchdog.h
 * @brief     UTC全局看门狗，深睡下仍能工作。喂狗超时后自动触发芯片复位，复位后软件识别的父状态为GLOBAL_RESET，子状态为WDT_RESET。。
 *
 ******************************************************************************
 */

/* 防止递归包含 -----------------------------------------------------------------*/
#pragma once

/* 引用头文件 ------------------------------------------------------------------*/
#include <stdint.h>





/**
 * @brief  初始化UTC看门狗。该接口耗时较久，单AP核近1ms。
 * @param  sec  喂狗时长，最长不能超过1小时
 * @note   该看门狗深睡下仍能工作，进而仅需初始化一次。
 * @note  OPEN形态用户设置的看门狗时长建议值由小到大分别为：AP硬看门狗时长(秒级) < UTC全局看门狗时长(分钟级) < 软看门狗时长(小时级)
 * @warning 该看门狗不支持中断模式，默认配置为RESET模式
 * @warning 考虑到FOTA期间频繁擦写FLASH，单次耗时百毫秒，进而看门狗时长建议设为秒级
 */
void UTC_WDT_Init(uint32_t sec);

/**
 * @brief  喂狗接口。耗时单核约130us左右，双核约60us左右。
 * @param  sec  喂狗时长，最长不能超过1小时
 * @note   如果客户需要高频率周期性喂狗，可以使用ap_watchdog.h来降低功耗开销，缺点是该看门狗SoC深睡无效。
 * @warning 考虑到FOTA期间频繁擦写FLASH，单次耗时百毫秒，进而看门狗时长建议设为秒级
 */
void UTC_WDT_Refresh(uint32_t sec);


/**
 * @brief  初始化UTC看门狗。该接口耗时相比于UTC_WDT_Init更长，达到10ms左右。
 * @param  sec  喂狗时长，可设置小时级别
 * @note   该看门狗深睡下仍能工作，进而仅需初始化一次。
 * @note  OPEN形态用户设置的看门狗时长建议值由小到大分别为：AP硬看门狗时长(秒级) < UTC全局看门狗时长(分钟级) < 软看门狗时长(小时级)
 * @warning 该看门狗不支持中断模式，默认配置为RESET模式
 * @warning 耗时久，建议小时以内需求，优先使用UTC_WDT_Init接口
 */
void UTC_WDT_Init2(uint32_t sec);

/**
 * @brief  喂狗接口。该接口耗时相比于UTC_WDT_Refresh更长，达到10ms左右。
 * @param  sec  喂狗时长，可设置小时级别
 * @note   该看门狗深睡下仍能工作，进而仅需初始化一次。
 * @warning 该看门狗不支持中断模式，默认配置为RESET模式
 * @warning 耗时久，建议小时以内需求，优先使用UTC_WDT_Refresh接口
 */
void UTC_WDT_Refresh2(uint32_t sec);


/**
 * @brief  去初始化UTC看门狗，若再次使用，必须调用UTC_WDT_Init
 */
void UTC_WDT_Deinit(void);




