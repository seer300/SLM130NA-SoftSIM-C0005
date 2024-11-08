/**
 ******************************************************************************
 * @file ap_watchdog.h
 * @brief AP核硬件看门狗的定义与函数声明，每次系统上电(含深睡唤醒)，都必须重新初始化，并及时喂狗。
 * @warning 该看门狗仅能用于工作态，SoC深睡下无效。如果需要在深睡下有效，请使用UTC看门狗，参见utc_watchdog.h
 ******************************************************************************
 */
#pragma once

#include <stdint.h>
#include "prcm.h"
#include "watchdog.h"
#include "system.h"
#include "hw_types.h"
#include "xy_memmap.h"
#include "xy_system.h"

/**
 * @brief AP看门狗工作模式
 */
typedef enum
{
  AP_WDT_WORKMODE_INT = 0, //AP看门狗中断模式，超时触发看门狗中断
  AP_WDT_WORKMODE_RESET    //AP看门狗重启模式，超时触发芯片硬重启
} AP_WDT_WorkModeTypeDef;

/**
 * @brief AP看门狗初始化函数
 * @param WorkMode AP看门狗工作模式。详情参考 @ref AP_WDT_WorkModeTypeDef.
 * @param sec 触发AP看门狗工作的超时时间，单位秒
 * @note 如客户使用硬看门狗，main入口和p_User_Init_FastRecovery函数指针所指向函数中都必须调用该API
 * @note  OPEN形态用户设置的看门狗时长建议值由小到大分别为：AP硬看门狗时长(秒级) < UTC全局看门狗时长(分钟级) < 软看门狗时长(小时级)
 * @warning 产品调试阶段若发生不合理的硬看门狗重启，可以设置为AP_WDT_WORKMODE_INT模式，在中断函数中排查触发的根源。
 *          该看门狗仅能用于工作态，SoC深睡下无效。如果需要在深睡下有效，请使用UTC看门狗，参见utc_watchdog.h。
 *          考虑到FOTA期间频繁擦写FLASH，单次耗时百毫秒，进而看门狗时长建议设为秒级。
 */
void AP_WDT_Init(AP_WDT_WorkModeTypeDef WorkMode, uint32_t sec);

/**
 * @brief 用于秒级的周期性喂狗，通常在main主函数的while循环里执行喂狗操作。耗时几us
 * @param sec 触发AP看门狗工作的超时时间，单位秒
 * @note 如果用户按照工作容许的最大时长来设置，则无需调用该接口进行喂狗，一旦超时则表明未能正常进入芯片深睡，触发看门狗异常。
 * @warning 该看门狗仅能用于工作态，SoC深睡下无效。如果需要在深睡下有效，请使用UTC看门狗，参见utc_watchdog.h。
 *          考虑到FOTA期间频繁擦写FLASH，单次耗时百毫秒，进而看门狗时长建议设为秒级。
 */
void AP_WDT_Refresh(uint32_t sec);

/**
 * @brief 使能AP看门狗
 */
void AP_WDT_Enable(void);

/**
 * @brief 禁能AP看门狗
 */
void AP_WDT_Disable(void);

/**
 * @brief AP看门狗中断模式(AP_WDT_WORKMODE_INT)回调函数注册，用户可根据需求重新定义，例如进行产品级容错等。
 * @warning 硬看门狗异常，不保证系统一定能正常处理中断回调！进而建议客户使用UTC全局看门狗来二次监控
 */
void AP_WDT_Int_Reg(pFunType_void p_fun);
