#pragma once

#include <stdint.h>
#include "core_cm3.h"

//在startup_cm3.s中定义
extern uint32_t g_pfnVectors[16 + 48]; 

//在startup_cm3.s中定义
extern void Default_Handler(void);

/**
 * @brief NVIC优先级分组
 *        使用三位来表示优先级分组，最高位表示抢占优先级，剩余表示亚优先级
 */
#define NVIC_PriorityUseBits __NVIC_PRIO_BITS
#define NVIC_PriorityGroup_0 ((uint32_t)0x700) //优先级分组0：0位表示抢占优先级，3位表示亚优先级
#define NVIC_PriorityGroup_1 ((uint32_t)0x600) //优先级分组1：1位表示抢占优先级，2位表示亚优先级
#define NVIC_PriorityGroup_2 ((uint32_t)0x500) //优先级分组2：2位表示抢占优先级，1位表示亚优先级
#define NVIC_PriorityGroup_3 ((uint32_t)0x400) //优先级分组3：3位表示抢占优先级，0位表示亚优先级

//中断服务函数类型定义
typedef void (*IRQnHandle_Type)(void);

/**
 * @brief 中断服务函数注册函数
 * @param IRQn 中断向量号
 * @param IRQnHandle 中断服务函数指针
 * @param priority   中断向量号的优先级，可选0~7，数字越小优先级越高
 */
void NVIC_IntRegister(IRQn_Type IRQn, IRQnHandle_Type IRQnHandle, uint32_t priority);

/**
 * @brief 去注册中断服务函数
 * @param IRQn 中断向量号
 */
void NVIC_IntUnregister(IRQn_Type IRQn);
