#include "cm3_systick.h"
#include "at_uart.h"

/**
 * @brief 初始化CM3内核的SYSTICK定时器产生1ms一次中断并计数
 * @param TickPriority SysTick的内核异常中断优先级设置，可选0~7，数字越小优先级越高
 * @return 0:初始化失败，1:初始化成功
 */
bool SysTick_Init(uint32_t TickPriority)
{
    //配置SysTick的重装载值为"GetAPClockFreq() / 1000U"，并清零计数值、使能中断、开启计数
    //SysTick_Config会默认配置SysTick为内核异常中断优先级为7（最低级）
    if (SysTick_Config(GetAPClockFreq() / 1000U) > 0U)
    {
        return 0;
    }

    //当优先级需要更高时，则配置SysTick的内核异常中断优先级
    if (TickPriority < (1UL << __NVIC_PRIO_BITS))
    {
        NVIC_SetPriority(SysTick_IRQn, TickPriority);
    }
    else
    {
        return 0;
    }

    return 1;
}

/**
 * @brief 关闭CM3内核的SYSTICK定时器
 */
void SysTick_Deinit(void)
{
	SysTick->CTRL = (0ul << SysTick_CTRL_ENABLE_Pos); /* Disable SysTick Timer */
}

/**
 * @brief SYSTICK中断服务函数
 * @note  注意：SYSTICK中断服务函数重新定义时，需要和startup_cm3.s中定义的弱函数同名
 * @warning  中断函数中仅能处理小于1ms的动作，否则会造成丢中断影响计时准确性。
 */
void SysTick_Handler(void)
{
	static uint64_t g_sysTick = 0;
	g_sysTick++;
	if((g_sysTick % 100) == 0)
		Send_AT_to_Ext("SysTick 100ms\r\n");
}
