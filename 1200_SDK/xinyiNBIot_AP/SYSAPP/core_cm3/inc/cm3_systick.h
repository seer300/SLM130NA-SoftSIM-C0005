#pragma once

#include "core_cm3.h"
#include "xy_system.h"

/**
 * @brief 初始化CM3内核的SYSTICK定时器产生1ms一次中断并计数
 * @param TickPriority SysTick的内核异常中断优先级设置，可选0~7，数字越小优先级越高
 * @return 0:初始化失败，1:初始化成功 
 */
bool SysTick_Init(uint32_t TickPriority);

/**
 * @brief 关闭CM3内核的SYSTICK定时器
 */
void SysTick_Deinit(void);