#include "prcm.h"
#include "hw_ints.h"
#include "hal_def.h"
#include "hal_timer.h"
#include "nvic.h"
#include "driver_utils.h"

/**
 * @brief  定时器私有保护宏
 */
#define IS_TIM_INSTANCE(__INSTANCE__)                 (((__INSTANCE__) == HAL_TIM1) || \
                                                       ((__INSTANCE__) == HAL_TIM2) || \
                                                       ((__INSTANCE__) == HAL_TIM3))

#define IS_TIM_CLOCKDIVISION(__CLOCKDIVISION__)       (((__CLOCKDIVISION__) == HAL_TIM_CLK_DIV_1 )  || \
                                                       ((__CLOCKDIVISION__) == HAL_TIM_CLK_DIV_2 )  || \
                                                       ((__CLOCKDIVISION__) == HAL_TIM_CLK_DIV_4 )  || \
                                                       ((__CLOCKDIVISION__) == HAL_TIM_CLK_DIV_8 )  || \
                                                       ((__CLOCKDIVISION__) == HAL_TIM_CLK_DIV_16 ) || \
                                                       ((__CLOCKDIVISION__) == HAL_TIM_CLK_DIV_32 ) || \
                                                       ((__CLOCKDIVISION__) == HAL_TIM_CLK_DIV_64 ) || \
                                                       ((__CLOCKDIVISION__) == HAL_TIM_CLK_DIV_128 ))

#define IS_TIM_WORKMODE(__WORKMODE__)                 (((__WORKMODE__) == HAL_TIM_MODE_CONTINUOUS ) || \
                                                       ((__WORKMODE__) == HAL_TIM_MODE_COUNTER )    || \
                                                       ((__WORKMODE__) == HAL_TIM_MODE_PWM_SINGLE ) || \
                                                       ((__WORKMODE__) == HAL_TIM_MODE_CAPTURE )    || \
                                                       ((__WORKMODE__) == HAL_TIM_MODE_COMPARE )    || \
                                                       ((__WORKMODE__) == HAL_TIM_MODE_GATED )      || \
                                                       ((__WORKMODE__) == HAL_TIM_MODE_PWM_DUAL ))

#define IS_TIM_POLARTITY(__POLARTITY__)               (((__POLARTITY__) == HAL_TIM_Polarity_High ) || \
                                                       ((__POLARTITY__) ==  HAL_TIM_Polarity_Low ))

__RAM_FUNC void HAL_TIM1_IRQHandler(void)
{
	//timer1产生中断时，不需要手动清中断标志位，硬件会自动清除
	HAL_TIM1_Callback();
}

__RAM_FUNC void HAL_TIM2_IRQHandler(void)
{
	//timer2产生中断时，不需要手动清中断标志位，硬件会自动清除
	HAL_TIM2_Callback();
}

__RAM_FUNC void HAL_TIM3_IRQHandler(void)
{
	//timer3产生中断时，不需要手动清中断标志位，硬件会自动清除
	HAL_TIM3_Callback();
}

/**
 * @brief  获取Timer时钟源频率.
 * @param  htim 指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
 * @retval 返回当前定时器对应的时钟源频率
 */
static uint32_t HAL_GetTimer_ClkSrcFreq(HAL_TIM_HandleTypeDef *htim)
{
	assert_param(IS_TIM_INSTANCE(htim->Instance));

	switch((uint32_t)htim->Instance)
	{
		case (uint32_t)HAL_TIM1: { return GetPCLK1Freq(); }
		case (uint32_t)HAL_TIM2: { return GetlsioFreq();  }
		case (uint32_t)HAL_TIM3: { return GetPCLK2Freq(); }
		default : return 0;
	}
}

/**
 * @brief 用于设置timer在循环触发模式下的超时时间
 * 
 * @param htim 指向一个包含低功耗定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针. 详情参考 @ref HAL_TIM_HandleTypeDef.
 * @param timeout 超时时长，单位ms。
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示初始化外设成功
 *         @retval  HAL_ERROR    ：表示入参错误(为NULL)、模式错误或低功耗定时器正在使用中
 */
HAL_StatusTypeDef HAL_TIM_SetTimeout(HAL_TIM_HandleTypeDef *htim, uint64_t timeout)
{
	uint32_t timer_freq = 0, timer_freq_div = 0;
	uint64_t base_max_time = 0;

	if (htim == NULL)
	{
		return HAL_ERROR;
	}
	
	if(timeout == 0)
	{
		return HAL_ERROR;
	}

	timer_freq = HAL_GetTimer_ClkSrcFreq(htim);

	//1分频时reload寄存器的最大定时，单位ms，reload寄存器为32位
	base_max_time = ((uint64_t)(0xFFFFFFFFULL + 1) / timer_freq) * 1000;

	if(timeout <= base_max_time)
	{
		htim->Init.ClockDivision = HAL_TIM_CLK_DIV_1;
		timer_freq_div = 1;
	}
	else if(timeout <= (base_max_time * 2))
	{
		htim->Init.ClockDivision = HAL_TIM_CLK_DIV_2;
		timer_freq_div = 2;
	}
	else if(timeout <= (base_max_time * 4))
	{
		htim->Init.ClockDivision = HAL_TIM_CLK_DIV_4;
		timer_freq_div = 4;
	}
	else if(timeout <= (base_max_time * 8))
	{
		htim->Init.ClockDivision = HAL_TIM_CLK_DIV_8;
		timer_freq_div = 8;
	}
	else if(timeout <= (base_max_time * 16))
	{
		htim->Init.ClockDivision = HAL_TIM_CLK_DIV_16;
		timer_freq_div = 16;
	}
	else if(timeout <= (base_max_time * 32))
	{
		htim->Init.ClockDivision = HAL_TIM_CLK_DIV_32;
		timer_freq_div = 32;
	}
	else if(timeout <= (base_max_time * 64))
	{
		htim->Init.ClockDivision = HAL_TIM_CLK_DIV_64;
		timer_freq_div = 64;
	}
	else if(timeout <= (base_max_time * 128))
	{
		htim->Init.ClockDivision = HAL_TIM_CLK_DIV_128;
		timer_freq_div = 128;
	}
	else
	{
		return HAL_ERROR;
	}

    htim->Init.Reload = Common_Convert_Ms_to_Count(uint32_t, timeout, timer_freq, timer_freq_div);

	return HAL_OK;
}

/**
 * @brief 用于获取timer在输入捕获模式下两次有效边沿间的时长
 * 
 * @param hlptim 指向一个包含低功耗定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针. 详情参考 @ref HAL_TIM_HandleTypeDef.
 * @param tick1 第一次捕获到的计数值
 * @param tick2 第二次捕获到的计数值
 * @param time 计算出到的时长
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示初始化外设成功
 *         @retval  HAL_ERROR    ：表示入参错误(为NULL)、模式错误或低功耗定时器正在使用中
 */
HAL_StatusTypeDef HAL_TIM_GetTime(HAL_TIM_HandleTypeDef *htim, uint32_t tick1, uint32_t tick2, uint64_t *time)
{
	uint32_t timer_freq = 0, timer_freq_div = 0;

	if (htim == NULL)
	{
		return HAL_ERROR;
	}

	timer_freq = HAL_GetTimer_ClkSrcFreq(htim);

	switch(htim->Init.ClockDivision)
	{
		case HAL_TIM_CLK_DIV_1: { timer_freq_div = 1; break; }
		case HAL_TIM_CLK_DIV_2: { timer_freq_div = 2; break; }
		case HAL_TIM_CLK_DIV_4: { timer_freq_div = 4; break; }
		case HAL_TIM_CLK_DIV_8: { timer_freq_div = 8; break; }
		case HAL_TIM_CLK_DIV_16: { timer_freq_div = 16; break; }
		case HAL_TIM_CLK_DIV_32: { timer_freq_div = 32; break; }
		case HAL_TIM_CLK_DIV_64: { timer_freq_div = 64; break; }
		case HAL_TIM_CLK_DIV_128: { timer_freq_div = 128; break; }
		default : return HAL_ERROR;
	}

    *time = Common_Convert_Count_to_Ms(uint64_t, ((tick2 + 0xFFFFFFFFULL - tick1) % 0xFFFFFFFFUL), timer_freq, timer_freq_div);

	return HAL_OK;
}

/**
 * @brief  初始化定时器.
 * @param  htim 指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示初始化外设成功
 *         @retval  HAL_ERROR    ：表示入参错误(为NULL)、模式错误或定时器正在使用中
 */
HAL_StatusTypeDef HAL_TIM_Init(HAL_TIM_HandleTypeDef *htim)
{
	if (htim == NULL)
	{
		return HAL_ERROR;
	}

	assert_param(IS_TIM_INSTANCE(htim->Instance));
  	assert_param(IS_TIM_CLOCKDIVISION(htim->Init.ClockDivision));
	assert_param(IS_TIM_WORKMODE(htim->Init.WorkMode));
	assert_param(IS_TIM_POLARTITY(htim->Init.Polarity));

	if (htim->State == HAL_TIM_STATE_RESET)
	{
		htim->Lock = HAL_UNLOCKED;
	}
	else
	{
		return HAL_ERROR;
	}

	htim->State = HAL_TIM_STATE_BUSY;

	//使能timer时钟
	if (HAL_TIM1 == htim->Instance)
	{
		PRCM_ClockEnable(CORE_CKG_CTL_TMR1_EN);
	}
	else if (HAL_TIM2 == htim->Instance)
	{
		PRCM_ClockEnable(CORE_CKG_CTL_TMR2_EN);
	}
	else if (HAL_TIM3 == htim->Instance)
	{
		PRCM_ClockEnable(CORE_CKG_CTL_TMR3_EN);
	}

	//timer配置
	TimerDisable((uint32_t)htim->Instance);
	TimerConfigure((uint32_t)htim->Instance, htim->Init.Mode | htim->Init.Polarity | htim->Init.ClockDivision | htim->Init.DualPWMDeadtime);
	TimerInitCountValueSet((uint32_t)htim->Instance, 0x00000000);
	TimerReloadValueSet((uint32_t)htim->Instance, htim->Init.Reload);
	TimerPWMValueSet((uint32_t)htim->Instance, htim->Init.PWMCmp);

	//注册NVIC中断
	if (HAL_TIM1 == htim->Instance)
	{
		NVIC_IntRegister(TIM1_IRQn, HAL_TIM1_IRQHandler, 1);
	}
	else if (HAL_TIM2 == htim->Instance)
	{
		NVIC_IntRegister(TIM2_IRQn, HAL_TIM2_IRQHandler, 1);
	}
	else if (HAL_TIM3 == htim->Instance)
	{
		NVIC_IntRegister(TIM3_IRQn, HAL_TIM3_IRQHandler, 1);
	}
	else
	{
		return  HAL_ERROR;
	}

	//配置lptimer中断模式
	if(htim->Init.Mode == HAL_TIM_MODE_CAPTURE || htim->Init.Mode == HAL_TIM_MODE_GATED )
	{
		TimerReloadValueSet((uint32_t)htim->Instance, 0xFFFFFFFF);
		TimerIntEnable((uint32_t)htim->Instance, TIMER_CTL_TICONFIG_OUTER_EVENT);		
	}
	else if(htim->Init.Mode == HAL_TIM_MODE_COMPARE ||htim->Init.Mode == HAL_TIM_MODE_CONTINUOUS||htim->Init.Mode == HAL_TIM_MODE_COUNTER)
	{
		TimerIntEnable((uint32_t)htim->Instance,TIMER_CTL_TICONFIG_INNER_EVENT);
	}

	htim->State = HAL_TIM_STATE_READY;

	return HAL_OK;
}


/**
 * @brief  去初始化定时器.
 * @param  htim 指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示去初始化成功
 *         @retval  HAL_ERROR    ：表示入参错误(为NULL)或定时器未初始化
 */
HAL_StatusTypeDef HAL_TIM_DeInit(HAL_TIM_HandleTypeDef *htim)
{
	if (htim == NULL)
	{
		return HAL_ERROR;
	}

	assert_param(IS_TIM_INSTANCE(htim->Instance));

	if (htim->State == HAL_TIM_STATE_RESET)
	{
		return HAL_ERROR;
	}
    
	htim->State = HAL_TIM_STATE_RESET;

	__HAL_UNLOCK(htim);

	return HAL_OK;
}


/**
 * @brief  开启定时器.
 * @param  htim 指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示定时器启动成功
 *         @retval  HAL_ERROR    ：表示定时器未初始化
 *         @retval  HAL_BUSY     : 表示定时器正在使用
 */
HAL_StatusTypeDef HAL_TIM_Start(HAL_TIM_HandleTypeDef *htim)
{
	assert_param(IS_TIM_INSTANCE(htim->Instance));

	if (htim->State == HAL_TIM_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (htim->State != HAL_TIM_STATE_READY)
	{
		return HAL_BUSY;
	}

	htim->State = HAL_TIM_STATE_BUSY;

	TimerEnable((uint32_t)htim->Instance);

	htim->State = HAL_TIM_STATE_READY;

	return HAL_OK;
}

/**
 * @brief  停止定时器.
 * @param  htim 指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示定时器成功停止
 *         @retval  HAL_ERROR    ：表示定时器未初始化
 *         @retval  HAL_BUSY     : 表示定时器正在使用
 */
HAL_StatusTypeDef HAL_TIM_Stop(HAL_TIM_HandleTypeDef *htim)
{
	assert_param(IS_TIM_INSTANCE(htim->Instance));

	if (htim->State == HAL_TIM_STATE_RESET)
	{
		return HAL_ERROR;
	}
	
	if (htim->State != HAL_TIM_STATE_READY)
	{
		return HAL_BUSY;
	}

	htim->State = HAL_TIM_STATE_BUSY;

	TimerDisable((uint32_t)htim->Instance);
	TimerInitCountValueSet((uint32_t)htim->Instance, 0x00000000);

	htim->State = HAL_TIM_STATE_READY;

	return HAL_OK;
}

/**
 * @brief  获取定时器Count寄存器中的值.
 * @param  htim 指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
 * @retval Count寄存器中的值，定时器每经过一个时钟周期就做一次加1.
 */
uint32_t HAL_TIM_GetCount(HAL_TIM_HandleTypeDef *htim)
{
	return TimerCountValueGet((uint32_t)htim->Instance);
}

/**
 * @brief  获取定时器CaptureCount寄存器中的值.
 * @param  htim 指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
 * @retval CaptureCount寄存器中的值,仅用于capture模式读取捕获有效边沿时的定时器计数值.
 */
uint32_t HAL_TIM_GetCaptureCount(HAL_TIM_HandleTypeDef *htim)
{
	return TimerCaptureValueGet((uint32_t)htim->Instance);
}


/**
 * @brief  设置定时器Count寄存器中的值.
 * @param  htim	指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
 * @param	count	设置的定时器Count寄存器中的值.
 */
void HAL_TIM_SetCount(HAL_TIM_HandleTypeDef *htim, uint32_t count)
{
	TimerInitCountValueSet((uint32_t)htim->Instance, count);
}

/**
 * @brief  Timer1中断回调函数.
 */
__WEAK void HAL_TIM1_Callback(void)
{
}

/**
 * @brief  Timer2中断回调函数.
 */
__WEAK void HAL_TIM2_Callback(void)
{
}

/**
 * @brief  Timer3中断回调函数.
 */
__WEAK void HAL_TIM3_Callback(void)
{
}

