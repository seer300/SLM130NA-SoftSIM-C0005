#pragma once
#include "stdint.h"

//核心延时函数，因流水线影响，耗时3个cpu cycle
static inline void delay_count(uint32_t count)
{
    __asm__ __volatile__(
        "loop:       \n\t"
        "subs %0, #1 \n\t"
        "bne loop    \n\t"
        :
        : "r"(count)
    );
}

extern volatile float g_unit_timer;

//@brief delay_us功能函数初始化接口，用来计算delay_us内部所需的单cpu cycle执行时间。、
//@warning 使用delay_us接口前必须调用。若CPU主频有动态调频功能，则在每次调频后均需调用此接口后才能继续使用delay_us函数。
void delay_func_init(void);

//@brief 用户调用的接口，传入值单位为us。传入最小值不得小于3us，最大值不得大于120,000,000 us
//@warning 该接口基础平台团队及业务团队不得调用。仅驱动团队和BSP团队可用。
//@warning 在线程中想获得精准延时应锁中断
/*
理论（us）     实测（us）		备注
    3           3.00
    4           4.00
    5           5.00
    6           6.00
    7           7.00
    8           8.00		测试精度0.04us
    9           9.00
    10          10.04
    20          20.04
    30          30.04
    100         100.04
*/
void delay_func_us(float user_delay_timer);