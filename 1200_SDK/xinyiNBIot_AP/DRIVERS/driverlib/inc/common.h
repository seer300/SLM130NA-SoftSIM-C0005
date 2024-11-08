#ifndef _COMMON_H_
#define _COMMON_H_

#include "hw_types.h"

typedef enum {
    AccessModeUnknow = 0,
    AccessModeByte = 1,
    AccessModeWord = 2,
} RegAccessMode;


extern uint8_t REG_Bus_Field_Set(unsigned long ulRegBase, uint8_t ucBit_high, uint8_t ucBit_low, unsigned long ulValue);
extern uint8_t REG_Bus_Field_Get(unsigned long ulRegBase, uint8_t ucBit_high, uint8_t ucBit_low, unsigned long *ulValue);

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

/**
 * @brief delay功能初始化接口
 * @warning 使用delay_func_us接口前必须调用。若CPU主频有变更，则在每次变更后均需调用此接口后才能继续使用delay_func_us函数。
 * 
 */
void delay_func_init(void);

/**
 * @brief 用户调用的高精度延时接口，传入值单位为us，使用时必须保证不被中断打断
 * @brief 工作原理：AP通过执行cpu cycle固定的汇编循环语句达到延时效果.
 * @warning 传入参数不得超过12s
 * @warning 接口必须大于最小延时才能正常工作，低于最小延时按照最小延时处理。最小延迟时间计算公式为：3000000/AP的时钟频率*60；
 * 几个经典场景的延时如下：
 * AP时钟 HRC 4分频时，最小延迟不得低于40us；
 * AP时钟 PLL 10分频时，最小延迟不得低于5us
 * @note  utc_cnt_delay接口为低精度延迟接口，粒度为30us。
 */

/*  高精度接口的延时误差为：
            xtal 1分频	                                    pll 4分频
    理论（us）	实测（us）	误差（us）           理论（us）	 实测（us）	 误差（us）
	100         100.083333	0.083333            100	        100	        0
	200	        200.166667	0.166667            200	        200	        0
	300	        300.25	    0.25                300	        300.08	    0.08
	400	        400.166667	0.166667            400	        400	        0
	500	        500.166667	0.166667            500	        500	        0
	600	        600.25	    0.25                600	        600.08	    0.08
	700	        700.166667	0.166667            700	        700.08	    0.08
	800	        800.25	    0.25                800	        800.08	    0.08
	900	        900.25	    0.25                900	        900.08	    0.08
	1000	    1000.166667	0.166667            1000	    1000.08	    0.08
 */
void delay_func_us(float tick_us);

#endif /* _COMMON_H_ */
