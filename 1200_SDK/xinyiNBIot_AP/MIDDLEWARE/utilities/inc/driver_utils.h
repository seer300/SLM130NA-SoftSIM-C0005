#pragma once

#include "sys_clk.h"



/* clk timer的分频系数 */
#define CLK_TIMER_DIVN  (32)


/**
 * @brief 获取clock tick数。32K主频下32分频时，约为1ms。 
 * @note  clock tick深睡或软复位仍然维持计数，硬复位后重新计数。
 * @warning 该接口直接读取32位寄存器，40天左右就会存在翻转。如果要读取绝对的tick数，请使用GetAbsoluteTick。
 */
#define Get_Tick()  ((uint32_t)(TICK->COUNTER))



/**
 * @brief   通用的毫秒转某分频定时器count值的接口，入参可指定主频及分频数
 * @warning count = f × t，即计数值等于频率(单位:Hz)乘以时间(单位:s)，进而count = (freq / freq_div) × (ms / 1000)
 */
#define Common_Convert_Ms_to_Count(type, ms, freq, freq_div)    (type)((uint64_t)(ms) * (freq) / (freq_div) / 1000)



/**
 * @brief   通用的某定时器count值转毫秒接口
 * @warning t = count ÷ f，即时间(单位:s)等于计数值除以频率(单位:Hz)，所以(ms / 1000) = count ÷ (freq / freq_div)
 */
#define Common_Convert_Count_to_Ms(type, count, freq, freq_div) (type)((uint64_t)(count) * (freq_div) * 1000 / (freq))



/**
 * @brief   通用的某定时器count值转微秒接口
 * @warning t = count ÷ f，即时间(单位:s)等于计数值除以频率(单位:Hz)，所以(us / 1000 / 1000) = count ÷ (freq / freq_div)
 */
#define Common_Convert_Count_to_Us(type, count, freq, freq_div) (type)((uint64_t)(count) * (freq_div) * 1000 * 1000 / (freq))



/**
 * @brief   clktick定时器毫秒转tick数 
 * @warning 由于主频可能随温度而变化，进而只能进行时间差值的转换，不能进行绝对毫秒数的转换
 */
#define Convert_Ms_to_Tick(ms) Common_Convert_Ms_to_Count(uint32_t, ms, Get32kClockFreq(), CLK_TIMER_DIVN) // tick=ms*频率/分频/1000。注意：Tick外设的分频值为32分频，在 clk_tick_init 函数里设置的！！！

/**
 * @brief   clktick定时器tick数转为毫秒数
 * @warning 由于主频可能随温度而变化，进而只能进行tick差值的换算，不能进行寄存器tick数的直接转换
 */
#define Convert_Tick_to_Ms(tick) Common_Convert_Count_to_Ms(uint32_t, tick, Get32kClockFreq(), CLK_TIMER_DIVN) // ms=tick*1000/(频率/分频)。注意：Tick外设的分频值为32分频，在 clk_tick_init 函数里设置的！！！


/**
 * @brief   CM3核内systick定时器,不建议客户使用
 */
#define SYSTICK_CONVERT_US(x)		((uint32_t)(((uint64_t)(x)*1000*1000)/GetAPClockFreq()))
#define SYSTICK_CONVERT_10US(x)		((uint32_t)(((uint64_t)(x)*1000*10000)/GetAPClockFreq()))




/**
 * @brief 对Get_Tick接口获取tick数，进行超时判断。通常用于通信过程中的超时判断。    
 * @param start_tick 开始tick计数,调用Get_Tick()可得
 * @param timeout_ms 指定超时时长，单位ms，超时判断任务的执行时长大于等于该时长判定为超时
 * @return bool    1:超时  0:未超时
 */
bool Check_Ms_Timeout(uint32_t start_tick, uint32_t timeout_ms);


/**
 * @brief 用户调用的高精度延时接口，传入值单位为us，使用时必须保证不被中断打断
 * @brief 工作原理：AP通过执行cpu cycle固定的汇编循环语句达到延时效果.
 * @param   us   延时的us数，传入参数不得超过12s
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
void delay_func_us(float us);



/**
  * @brief	基于Get_Tick实现的死循环延迟毫秒数功能，精度为32K
  * @param	ms	延时的ms数.
*/
void delay_func_ms(uint32_t ms);
