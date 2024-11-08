/*
 * @file  xy_svd.h
 * @brief SVD模块，作为芯片的vbat低电压检测，并触发中断，以便客户进行低压的紧急容错处理。详情参阅《芯翼XY1200S&XY2100S产品SVD功能使用指南》
 * @warning
 */

#pragma once
#include <stdint.h>
#include "prcm.h"
#include "system.h"
/**
 * @brief SVD的唤醒中断回调函数注册接口
 */
void Svd_Wakeup_Cb_Reg(pFunType_void pfun);

/**
 * @brief  初始化SVD，SVD检测源默认为GPIO_WKP2引脚，不支持用户配置。
 * @param  val_set  阈值设置，范围为2.2V-3.5V，对应的取值为22-35。当vbat电压低于该值，会触发中断，
 *                  用户可以调用Svd_WakeupCallback_Reg来注册中断回调函数，并在回调函数里实现产品级低压容错处理。
 * @param  period_sec  SVD检测工作周期设置，单位为秒。取值只能为0/1/5/10四个档位。当值为0时，功耗开销最大，增加约110uA，但时延最小。
 *
 * @warning 产品客户建议在main主函数入口初始化时调用一次即可，深睡可保持.
 *
 * @attention 若对功耗极度敏感，建议在main主函数入口设置周期为0，在深睡HOOK函数Before_Deepsleep_Hook中设置长周期或
 * 直接执行xy_SVD_DeInit关闭检测功能，理由是深睡期间耗电很小，电池电压会维持不变。
 * 
 */
void xy_SVD_Init(uint8_t val_set, uint32_t period_sec);


/**
 * @brief  SVD去初始化，关闭SVD低压检测.
 * @warning 
 */
void xy_SVD_DeInit(void);