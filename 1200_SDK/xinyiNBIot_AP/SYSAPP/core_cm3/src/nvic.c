#include "nvic.h"

/**
 * @brief 注册中断服务函数
 * @param IRQn 中断向量号
 * @param IRQnHandle 中断服务函数指针
 * @param priority   中断向量号的优先级，可选0~7，数字越小优先级越高
 */
void NVIC_IntRegister(IRQn_Type IRQn, IRQnHandle_Type IRQnHandle, uint32_t priority)
{
	//中断向量号检查
    if((IRQn < CLKTIM_IRQn) || (IRQn > SVD_IRQn))
    {
        return;
    }

    //将IRQnHandle中断服务函数注册到中断向量表中
	g_pfnVectors[IRQn + 16] = (uint32_t)IRQnHandle;

    //设置中断优先级为priority
	NVIC_SetPriority(IRQn, priority);

    //使能中断
	if (IRQn >= 0)
	{
		NVIC_EnableIRQ(IRQn);
	}
}

/**
 * @brief 去注册中断服务函数
 * @param IRQn 中断向量号
 */
void NVIC_IntUnregister(IRQn_Type IRQn)
{
	//中断向量号检查
    if((IRQn < CLKTIM_IRQn) || (IRQn > SVD_IRQn))
    {
        return;
    }

	//将Default_Handler中断服务函数注册到中断向量表中
	g_pfnVectors[IRQn + 16] = (uint32_t)Default_Handler;

    //设置中断优先级为0（最高级）
	NVIC_SetPriority(IRQn, 0);

    //禁能中断
	if (IRQn >= 0)
	{
		NVIC_DisableIRQ(IRQn);
	}
}
