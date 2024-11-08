#include "rom.h"
#include "hw_ints.h"
#include "hal_def.h"
#include "hal_lptimer.h"
#include "hal_def.h"
#include "nvic.h"
#include "driver_utils.h"

/**
 * @brief  低功耗定时器私有保护宏
 */
#define IS_LPTIM_INSTANCE(__INSTANCE__)               (((__INSTANCE__) == HAL_LPTIM1) || \
                                                       ((__INSTANCE__) == HAL_LPTIM2))

#define IS_LPTIM_CLOCKDIVISION(__CLOCKDIVISION__)     (((__CLOCKDIVISION__) == HAL_LPTIM_CLK_DIV_1 )  || \
                                                       ((__CLOCKDIVISION__) == HAL_LPTIM_CLK_DIV_2 )  || \
                                                       ((__CLOCKDIVISION__) == HAL_LPTIM_CLK_DIV_4 )  || \
                                                       ((__CLOCKDIVISION__) == HAL_LPTIM_CLK_DIV_8 )  || \
                                                       ((__CLOCKDIVISION__) == HAL_LPTIM_CLK_DIV_16 ) || \
                                                       ((__CLOCKDIVISION__) == HAL_LPTIM_CLK_DIV_32 ) || \
                                                       ((__CLOCKDIVISION__) == HAL_LPTIM_CLK_DIV_64 ) || \
                                                       ((__CLOCKDIVISION__) == HAL_LPTIM_CLK_DIV_128 ))

#define IS_LPTIM_WORKMODE(__WORKMODE__)               (((__WORKMODE__) == HAL_LPTIM_MODE_CONTINUOUS ) || \
                                                       ((__WORKMODE__) == HAL_LPTIM_MODE_COUNTER ) || \
                                                       ((__WORKMODE__) == HAL_LPTIM_MODE_PWM_SINGLE ) || \
                                                       ((__WORKMODE__) == HAL_LPTIM_MODE_CAPTURE ) || \
                                                       ((__WORKMODE__) == HAL_LPTIM_MODE_COMPARE ) || \
                                                       ((__WORKMODE__) == HAL_LPTIM_MODE_GATED ) || \
													   ((__WORKMODE__) == HAL_LPTIM_MODE_PWM_DUAL ))

#define IS_LPTIM_POLARTITY(__POLARTITY__)             (((__POLARTITY__) == HAL_LPTIM_Polarity_Low ) || \
                                                       ((__POLARTITY__) ==  HAL_LPTIM_Polarity_High ))                                       

extern uint8_t g_lptim_used;


static __RAM_FUNC void HAL_LPTIM_IRQHandler(void)
{
	uint8_t lptimer_intsta = LPTimerIntStatus(); //读清lptimer中断标志位
    if(lptimer_intsta & LPTIMER_INT_LPTIMER1_Msk)
    {
	    HAL_LPTIM1_Callback();
    }
    if(lptimer_intsta & LPTIMER_INT_LPTIMER2_Msk)
    {
		extern __RAM_FUNC void HAL_LPTIM2_Callback(void);
        HAL_LPTIM2_Callback();
    }
}

/**
 * @brief 用于设置lptimer在循环触发模式下的超时时间
 * 
 * @param hlptim 指向一个包含低功耗定时器具体配置信息的HAL_LPTIM_HandleTypeDef结构体的指针. 详情参考 @ref HAL_LPTIM_HandleTypeDef.
 * @param timeout 超时时长，单位ms，由于LPTIMER特性（16位计数，参考时钟32K，最大分频128），最长超时时长约为256000ms
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示初始化外设成功
 *         @retval  HAL_ERROR    ：表示入参错误(为NULL)、模式错误或低功耗定时器正在使用中
 */
HAL_StatusTypeDef HAL_LPTIM_SetTimeout(HAL_LPTIM_HandleTypeDef *hlptim, uint32_t timeout)
{
	uint32_t lptimer_freq = 0, lptimer_freq_div = 0;
	uint32_t base_max_time = 0;

	if (hlptim == NULL)
	{
		return HAL_ERROR;
	}
	
	if(timeout == 0)
	{
		return HAL_ERROR;
	}

	lptimer_freq = Get32kClockFreq();

	//1分频时reload寄存器的最大定时，单位ms，reload寄存器为16位
	//32k是时钟频率为32KHz
	//此时base_max_time 的典型值为 2048 ms
	base_max_time = ((uint32_t)(0xFFFFUL + 1) / lptimer_freq) * 1000;

	if(timeout <= base_max_time)
	{
		hlptim->Init.Division = HAL_LPTIM_CLK_DIV_1;
		lptimer_freq_div = 1;
	}
	else if(timeout <= (base_max_time * 2))
	{
		hlptim->Init.Division = HAL_LPTIM_CLK_DIV_2;
		lptimer_freq_div = 2;
	}
	else if(timeout <= (base_max_time * 4))
	{
		hlptim->Init.Division = HAL_LPTIM_CLK_DIV_4;
		lptimer_freq_div = 4;
	}
	else if(timeout <= (base_max_time * 8))
	{
		hlptim->Init.Division = HAL_LPTIM_CLK_DIV_8;
		lptimer_freq_div = 8;
	}
	else if(timeout <= (base_max_time * 16))
	{
		hlptim->Init.Division = HAL_LPTIM_CLK_DIV_16;
		lptimer_freq_div = 16;
	}
	else if(timeout <= (base_max_time * 32))
	{
		hlptim->Init.Division = HAL_LPTIM_CLK_DIV_32;
		lptimer_freq_div = 32;
	}
	else if(timeout <= (base_max_time * 64))
	{
		hlptim->Init.Division = HAL_LPTIM_CLK_DIV_64;
		lptimer_freq_div = 64;
	}
	else if(timeout <= (base_max_time * 128))
	{
		hlptim->Init.Division = HAL_LPTIM_CLK_DIV_128;
		lptimer_freq_div = 128;
	}
	else
	{
		return HAL_ERROR;
	}

    hlptim->Init.Reload = Common_Convert_Ms_to_Count(uint16_t, timeout, lptimer_freq, lptimer_freq_div);

	return HAL_OK;
}

/**
 * @brief 用于获取lptimer在输入捕获模式下两次有效边沿间的时长
 * 
 * @param hlptim 指向一个包含低功耗定时器具体配置信息的HAL_LPTIM_HandleTypeDef结构体的指针. 详情参考 @ref HAL_LPTIM_HandleTypeDef.
 * @param tick1 第一次捕获到的计数值
 * @param tick2 第二次捕获到的计数值
 * @param time 计算出到的时长
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示初始化外设成功
 *         @retval  HAL_ERROR    ：表示入参错误(为NULL)、模式错误或低功耗定时器正在使用中
 */
HAL_StatusTypeDef HAL_LPTIM_GetTime(HAL_LPTIM_HandleTypeDef *hlptim, uint16_t tick1, uint16_t tick2, uint32_t *time)
{
	uint32_t lptimer_freq = 0, lptimer_freq_div = 0;

	if (hlptim == NULL)
	{
		return HAL_ERROR;
	}

	lptimer_freq = Get32kClockFreq();

	switch(hlptim->Init.Division)
	{
		case HAL_LPTIM_CLK_DIV_1: { lptimer_freq_div = 1; break; }
		case HAL_LPTIM_CLK_DIV_2: { lptimer_freq_div = 2; break; }
		case HAL_LPTIM_CLK_DIV_4: { lptimer_freq_div = 4; break; }
		case HAL_LPTIM_CLK_DIV_8: { lptimer_freq_div = 8; break; }
		case HAL_LPTIM_CLK_DIV_16: { lptimer_freq_div = 16; break; }
		case HAL_LPTIM_CLK_DIV_32: { lptimer_freq_div = 32; break; }
		case HAL_LPTIM_CLK_DIV_64: { lptimer_freq_div = 64; break; }
		case HAL_LPTIM_CLK_DIV_128: { lptimer_freq_div = 128; break; }
		default : return HAL_ERROR;
	}

    *time = Common_Convert_Count_to_Ms(uint32_t, ((tick2 + 0xFFFFUL - tick1) % 0xFFFFU), lptimer_freq, lptimer_freq_div);

	return HAL_OK;
}

/**
 * @brief  初始化低功耗定时器.
 * @param  hlptim 指向一个包含低功耗定时器具体配置信息的HAL_LPTIM_HandleTypeDef结构体的指针. 详情参考 @ref HAL_LPTIM_HandleTypeDef.
  * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示初始化外设成功
 *         @retval  HAL_ERROR    ：表示入参错误(为NULL)、模式错误或低功耗定时器正在使用中
 */
HAL_StatusTypeDef HAL_LPTIM_Init(HAL_LPTIM_HandleTypeDef *hlptim)
{
	uint32_t tmpcfgr;

	if (hlptim == NULL)
	{
		return HAL_ERROR;
	}

	assert_param(IS_LPTIM_INSTANCE(hlptim->Instance));
  	assert_param(IS_LPTIM_CLOCKDIVISION(hlptim->Init.Division));
	assert_param(IS_LPTIM_WORKMODE(hlptim->Init.Mode));
	assert_param(IS_LPTIM_POLARTITY(hlptim->Init.Polarity));

	if (hlptim->State == HAL_LPTIM_STATE_RESET)
	{
		hlptim->Lock = HAL_UNLOCKED;
	}
	else
	{
		return HAL_ERROR;
	}

	hlptim->State = HAL_LPTIM_STATE_BUSY;

	//使能timer时钟
	PRCM_ClockEnable(CORE_CKG_CTL_LPTMR_EN);

	LPTimerClockSrcMux((uint32_t)hlptim->Instance, LPTIMER_CONFIG_CLK_MUX_APB_ONLY);
	while (!LPTimerClockStateGet((uint32_t)hlptim->Instance, LPTIMER_CLK_APB_FLAG_Msk)){;}			

	//lptimer配置
	LPTimerDisable((uint32_t)hlptim->Instance);
	LPTimerInitCountValueSet((uint32_t)hlptim->Instance, 0x0000);
	LPTimerPWMValueSet((uint32_t)hlptim->Instance, hlptim->Init.PWMCmp);
	LPTimerReloadValueSet((uint32_t)hlptim->Instance, hlptim->Init.Reload);
	tmpcfgr = hlptim->Init.Mode | hlptim->Init.Division | hlptim->Init.Polarity | hlptim->Init.DualPWMDeadtime;
	LPTimerConfigure((uint32_t)hlptim->Instance, tmpcfgr);

	//切换时钟源为32k时钟
	LPTimerClockSrcMux((uint32_t)hlptim->Instance, LPTIMER_CONFIG_CLK_MUX_INTER_ONLY);
	while(!LPTimerClockStateGet((uint32_t)hlptim->Instance, LPTIMER_CLK_INTER_FLAG_Msk));

	//注册NVIC中断
	NVIC_IntRegister(LPTIM_IRQn, HAL_LPTIM_IRQHandler, 1);

	//配置lptimer中断模式
	if(hlptim->Init.Mode == HAL_LPTIM_MODE_CAPTURE || hlptim->Init.Mode == HAL_LPTIM_MODE_GATED)
	{
		LPTimerReloadValueSet((uint32_t)hlptim->Instance, 0xFFFF);
		LPTimerIntEnable((uint32_t)hlptim->Instance, LPTIMER_CTL_TICONFIG_OUTER_EVENT);
	}
	else if(hlptim->Init.Mode == HAL_LPTIM_MODE_COMPARE || hlptim->Init.Mode == HAL_LPTIM_MODE_CONTINUOUS|| hlptim->Init.Mode == HAL_LPTIM_MODE_COUNTER)
	{
		LPTimerIntEnable((uint32_t)hlptim->Instance,LPTIMER_CTL_TICONFIG_INNER_EVENT);
	}
	else if (hlptim->Init.Mode == HAL_LPTIM_MODE_PWM_DUAL)
	{
		AONPRCM-> LPTMR_IOCTRL |= (0x01<<3); //使能深睡下PWM_OUTN输出功能
		AONPRCM-> LPTMR_IOCTRL |= (0x01<<2); //使能深睡下PWM_OUTP输出功能
	}
	else if (hlptim->Init.Mode == LPTIMER_CTL_TMODE_PWM_SINGLE)
	{
		if (hlptim->Init.SingPWMOUTPolinDSleep == GPIO_LPTMR1PWM_OUTP)
			AONPRCM-> LPTMR_IOCTRL |= (0x01<<2); //使能深睡下PWM_OUTP输出功能
		else if (hlptim->Init.SingPWMOUTPolinDSleep == GPIO_LPTMR1PWM_OUTN)
			AONPRCM-> LPTMR_IOCTRL |= (0x01<<3); //使能深睡下PWM_OUTN输出功能
	}

	if(hlptim->Init.Mode == HAL_LPTIM_MODE_CONTINUOUS)
	{
		g_lptim_used = 1;
#if (MODULE_VER == 0x0)	// opencpu,避免深睡前配置耗时
        PRCM_LPUA_PWR_Ctl(LPUA_ANY_MODE_ON); //force on in deesleep,lpuart和lptim中有一个维持工作，则需要维持外设寄存器不掉电。
#endif
	}
	
	hlptim->State = HAL_LPTIM_STATE_READY;

	return HAL_OK;
}

/**
 * @brief  去初始化低功耗定时器.
 * @param  hlptim 指向一个包含低功耗定时器具体配置信息的HAL_LPTIM_HandleTypeDef结构体的指针. 详情参考 @ref HAL_LPTIM_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示去初始化成功
 *         @retval  HAL_ERROR    ：表示入参错误(为NULL)或低功耗定时器未初始化
 */
HAL_StatusTypeDef HAL_LPTIM_DeInit(HAL_LPTIM_HandleTypeDef *hlptim)
{
	if (hlptim == NULL)
	{
		return HAL_ERROR;
	}

	assert_param(IS_LPTIM_INSTANCE(hlptim->Instance));

	if (hlptim->State == HAL_LPTIM_STATE_RESET)
	{
		return HAL_ERROR;
	}

	hlptim->State = HAL_LPTIM_STATE_BUSY;

	PRCM_ClockEnable(CORE_CKG_CTL_LPTMR_EN);

	LPTimerDisable((uint32_t)hlptim->Instance);
	LPTimerConfigure((uint32_t)hlptim->Instance, 0x0000);
	LPTimerInitCountValueSet((uint32_t)hlptim->Instance,  0x0000);
	LPTimerReloadValueSet((uint32_t)hlptim->Instance, 0xFFFF);
	LPTimerPWMValueSet((uint32_t)hlptim->Instance,  0x0000);

	PRCM_ClockDisable(CORE_CKG_CTL_LPTMR_EN);

	hlptim->State = HAL_LPTIM_STATE_RESET;
	g_lptim_used = 0;

	__HAL_UNLOCK(hlptim);

	return HAL_OK;
}

/**
 * @brief  开启低功耗定时器.
 * @param  hlptim 指向一个包含低功耗定时器具体配置信息的HAL_LPTIM_HandleTypeDef结构体的指针. 详情参考 @ref HAL_LPTIM_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示低功耗定时器启动成功
 *         @retval  HAL_ERROR    ：表示低功耗定时器未初始化
 *         @retval  HAL_BUSY     : 表示低功耗定时器正在使用
 */
HAL_StatusTypeDef HAL_LPTIM_Start(HAL_LPTIM_HandleTypeDef *hlptim)
{
	assert_param(IS_LPTIM_INSTANCE(hlptim->Instance));

	if (hlptim->State == HAL_LPTIM_STATE_RESET)
	{
		return HAL_ERROR;
	}
	
	if (hlptim->State != HAL_LPTIM_STATE_READY)
	{
		return HAL_BUSY;
	}

	hlptim->State = HAL_LPTIM_STATE_BUSY;

	LPTimerEnable((uint32_t)hlptim->Instance);

	hlptim->State = HAL_LPTIM_STATE_READY;

	return HAL_OK;
}

/**
 * @brief  停止低功耗定时器.
 * @param  hlptim 指向一个包含低功耗定时器具体配置信息的HAL_LPTIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_LPTIM_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示成功停掉低功耗定时器
 *         @retval  HAL_ERROR    ：表示低功耗定时器未初始化
 *         @retval  HAL_BUSY     : 表示低功耗定时器正在使用
 */
HAL_StatusTypeDef HAL_LPTIM_Stop(HAL_LPTIM_HandleTypeDef *hlptim)
{
	assert_param(IS_LPTIM_INSTANCE(hlptim->Instance));

	if (hlptim->State == HAL_LPTIM_STATE_RESET)
	{
		return HAL_ERROR;
	}

	if (hlptim->State != HAL_LPTIM_STATE_READY)
	{
		return HAL_BUSY;
	}

	hlptim->State = HAL_LPTIM_STATE_BUSY;

	LPTimerDisable((uint32_t)hlptim->Instance);
	LPTimerInitCountValueSet((uint32_t)hlptim->Instance, 0x0000);

	hlptim->State = HAL_LPTIM_STATE_READY;

	return HAL_OK;
}

/**
 * @brief  获取低功耗定时器Count寄存器中的值.
 * @param  hlptim 指向一个包含低功耗定时器具体配置信息的HAL_LPTIM_HandleTypeDef结构体的指针. 详情参考 @ref HAL_LPTIM_HandleTypeDef.
 * @retval Count寄存器中的值，低功耗定时器每经过一个时钟周期就做一次加1.
 */
uint16_t HAL_LPTIM_GetCount(HAL_LPTIM_HandleTypeDef *hlptim)
{
	return LPTimerCountValueGet((uint32_t)hlptim->Instance);
}

/**
 * @brief  获取低功耗定时器CaptureCount寄存器中的值.
 * @param  hlptim 指向一个包含低功耗定时器具体配置信息的HAL_LPTIM_HandleTypeDef结构体的指针. 详情参考 @ref HAL_LPTIM_HandleTypeDef.
 * @retval CaptureCount寄存器中的值，仅用于capture模式读取捕获有效边沿时的低功耗定时器计数值.
 */
uint16_t HAL_LPTIM_GetCaptureCount(HAL_LPTIM_HandleTypeDef *hlptim)
{
	return LPTimerCaptureValueGet((uint32_t)hlptim->Instance);
}

/**
 * @brief  设置低功耗定时器Count寄存器中的值.
 * @param  hlptim	指向一个包含低功耗定时器具体配置信息的HAL_LPTIM_HandleTypeDef结构体的指针. 详情参考 @ref HAL_LPTIM_HandleTypeDef.
 * @retval count	待设置低功耗定时器Count寄存器中的值.
 */
void HAL_LPTIM_SetCount(HAL_LPTIM_HandleTypeDef *hlptim, uint16_t count)
{
	LPTimerInitCountValueSet((uint32_t)hlptim->Instance, count);
}










