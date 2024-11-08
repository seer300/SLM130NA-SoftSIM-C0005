/**
  ******************************************************************************
  * @file	hal_timer.h
  * @brief	此文件包含定时器外设的变量，枚举，结构体定义，函数声明等.
  * @note   当前硬件timer用户可使用TIM1、TIM2。
  *         TIM2不受启停CP影响，TIM2的适用场景为：高精度的短时定时，pwm生成功能，计数器功能、比较器功能等。
  *         TIM1受到启停CP影响，因此启停CP后需要重新初始化TIM1。
  * @warning  用户业务开发请使用xy_timer.h中的定时器，不得使用该硬件timer进行普通的定时.
  ******************************************************************************
 */

#pragma once

#include "hal_def.h"
#include "timer.h"

/**
 * @brief  定时器工作模式枚举
 */
typedef enum
{
  HAL_TIM_MODE_CONTINUOUS = TIMER_CTL_TMODE_CONTINUOUS,   /*!< 循环触发模式，连续多次定时 */
  HAL_TIM_MODE_COUNTER = TIMER_CTL_TMODE_COUNTER,         /*!< 计数模式,对外部输入信号的有效边沿进行计数 */
  HAL_TIM_MODE_PWM_SINGLE = TIMER_CTL_TMODE_PWM_SINGLE,   /*!< 单PWM模式，输出一路PWM(脉冲调制)波形*/
  HAL_TIM_MODE_CAPTURE = TIMER_CTL_TMODE_CAPTURE,         /*!< 捕获模式，捕捉外部输入信号的有效边沿*/
  HAL_TIM_MODE_COMPARE = TIMER_CTL_TMODE_COMPARE,         /*!< 比较模式，主要用于单次定时 */
  HAL_TIM_MODE_GATED = TIMER_CTL_TMODE_GATED,             /*!< 门模式，主要用于脉宽测量*/
  HAL_TIM_MODE_PWM_DUAL = TIMER_CTL_TMODE_PWM_DUAL        /*!< 双PWM模式，输出两路互补带死区的PWM波*/
} HAL_TIM_ModeTypeDef;

/**
 * @brief  定时器分频系数枚举
 */
typedef enum
{
  HAL_TIM_CLK_DIV_1 = TIMER_CTL_PRES_DIVIDE_1,    /*!< 1分频 */
  HAL_TIM_CLK_DIV_2 = TIMER_CTL_PRES_DIVIDE_2,    /*!< 2分频 */
  HAL_TIM_CLK_DIV_4 = TIMER_CTL_PRES_DIVIDE_4,    /*!< 4分频 */
  HAL_TIM_CLK_DIV_8 = TIMER_CTL_PRES_DIVIDE_8,    /*!< 8分频 */
  HAL_TIM_CLK_DIV_16 = TIMER_CTL_PRES_DIVIDE_16,  /*!< 16分频 */
  HAL_TIM_CLK_DIV_32 = TIMER_CTL_PRES_DIVIDE_32,  /*!< 32分频 */
  HAL_TIM_CLK_DIV_64 = TIMER_CTL_PRES_DIVIDE_64,  /*!< 64分频 */
  HAL_TIM_CLK_DIV_128 = TIMER_CTL_PRES_DIVIDE_128 /*!< 128分频 */
} HAL_TIM_ClkDivTypeDef;


/**
 * @brief  定时器双PWM模式死区时间，注意此参数单位为定时器时钟源周期，不受定时器的分频系数影响
 */
typedef enum
{
  HAL_TIM_DEADTIME_0_CYCLES = TIMER_CTL_PWMD_NO_DELAY,         /*!<0个周期 */
  HAL_TIM_DEADTIME_2_CYCLES = TIMER_CTL_PWMD_DELAY_2_CYCLE,    /*!<2个周期 */
  HAL_TIM_DEADTIME_4_CYCLES = TIMER_CTL_PWMD_DELAY_4_CYCLE,    /*!<4个周期 */
  HAL_TIM_DEADTIME_8_CYCLES = TIMER_CTL_PWMD_DELAY_8_CYCLE,    /*!<8个周期 */
  HAL_TIM_DEADTIME_16_CYCLES = TIMER_CTL_PWMD_DELAY_16_CYCLE,  /*!<16个周期 */
  HAL_TIM_DEADTIME_32_CYCLES = TIMER_CTL_PWMD_DELAY_32_CYCLE,  /*!<32个周期 */
  HAL_TIM_DEADTIME_64_CYCLES = TIMER_CTL_PWMD_DELAY_64_CYCLE,  /*!<64个周期 */
  HAL_TIM_DEADTIME_128_CYCLES = TIMER_CTL_PWMD_DELAY_128_CYCLE /*!<128个周期 */
} HAL_TIM_DeadtimeTypeDef;

/**
 * @brief  定时器各工作模式输出电平及输入触发源设置.
 * @note	不同模式下极性对应的含义不同，具体含义如下: 
 	计数模式: Polarity_Low表示上升沿计数，Polarity_High表示下降沿计数.
 *	捕获模式: Polarity_Low表示捕获上升沿，Polarity_High表示捕获下降沿.
 *	比较模式: 此模式下无需配置.
 *	门模式: Polarity_Low表示在输入为高电平时Count寄存器计数，下降沿触发中断，Polarity_High表示在输入为低电平时Count寄存器计数，上升沿触发中断.
 *	单PWM模式: Polarity_Low表示低功耗定时器失能时输出为低电平，开启低功耗定时器Count寄存器计数值达到PWM寄存器的值后翻转为高电平，Count寄存器计数值达到Reload寄存器重载值后重新输出低电平。
              Polarity_High表示低功耗定时器失能时输出为高电平，开启低功耗定时器Count寄存器计数值达到PWM寄存器的值后翻转为低电平，Count寄存器计数值达到Reload寄存器重载值后重新输出高电平。 
 *	双PWM模式：含义与单PWM模式相同，区别在于双PWM模式会输出另一类极性相反的互补PWM波，且可以设置死区长度，详情参考 @ref HAL_TIM_DeadtimeTypeDef.
 */
typedef enum
{
  HAL_TIM_Polarity_Low =  TIMER_CTL_TPOL_FALSE, /*!< 输出类模式下：定时器失能时输出低电平；输入类模式下：上升沿为有效边沿 */
  HAL_TIM_Polarity_High = TIMER_CTL_TPOL_TRUE   /*!< 输出类模式下：定时器失能时输出高电平；输入类模式下：下降沿为有效边沿 */
} HAL_TIM_PolarityTypeDef;

/**
 * @brief  定时器工作状态枚举
 */
typedef enum
{
  HAL_TIM_STATE_RESET = 0x00U,   /*!< 外设未初始化 */
  HAL_TIM_STATE_READY = 0x01U,   /*!< 外设初始化完成可以使用 */
  HAL_TIM_STATE_BUSY = 0x02U,    /*!< 外设正在工作状态下 */
} HAL_TIM_StateTypeDef;

/**
 * @brief 定时器初始化结构体
 */
typedef struct
{
  HAL_TIM_ModeTypeDef Mode;                  /*!< 定时器工作模式 */
  uint32_t Reload;                           /*!< 定时器重载值,capture/gated模式时不需要配置，计数模式时待计数的有效边沿数，其它模式下为重载值*/
  uint32_t PWMCmp;                           /*!< 定时器PWM比较值，仅用于定时器PWM模式，PMW电平翻转时的比较值，决定PWM波的占空比 */
  HAL_TIM_ClkDivTypeDef ClockDivision;       /*!< 定时器时钟分频 */
  HAL_TIM_PolarityTypeDef Polarity;          /*!< 定时器极性，不同模式下含义不同，详情参考 @ref HAL_TIM_PolarityTypeDef*/
  HAL_TIM_DeadtimeTypeDef DualPWMDeadtime;   /*!< 仅用于定时器双PWM模式下，此参数用于设置PWM死区的长度 */
} HAL_TIM_InitTypeDef;

/**
 * @brief  定时器控制结构体
 */
typedef struct
{
  TMR_TypeDef *Instance;               /*!< 定时器基地址 */  
  HAL_TIM_InitTypeDef Init;            /*!< 定时器初始化结构体 */
  HAL_LockTypeDef Lock;                /*!< 定时器设备锁 */
  volatile HAL_TIM_StateTypeDef State; /*!< 定时器状态 */
} HAL_TIM_HandleTypeDef;

/**
 * @brief  定时器外设基地址
 */
#define HAL_TIM1 (TMR_TypeDef *)TIMER1_BASE 
#define HAL_TIM2 (TMR_TypeDef *)TIMER2_BASE
#define HAL_TIM3 (TMR_TypeDef *)TIMER3_BASE //AP侧不到万不得已不使用，使用时必须确认CP侧不使用

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
HAL_StatusTypeDef HAL_TIM_SetTimeout(HAL_TIM_HandleTypeDef *htim, uint64_t timeout);

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
HAL_StatusTypeDef HAL_TIM_GetTime(HAL_TIM_HandleTypeDef *htim, uint32_t tick1, uint32_t tick2, uint64_t *time);

/**
 * @brief  初始化定时器.耗时61us
 * @param  htim 指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示初始化外设成功
 *         @retval  HAL_ERROR    ：表示入参错误(为NULL)、模式错误或定时器正在使用中
 */
HAL_StatusTypeDef HAL_TIM_Init(HAL_TIM_HandleTypeDef *htim);

/**
 * @brief  去初始化定时器.耗时33us
 * @param  htim 指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示去初始化成功
 *         @retval  HAL_ERROR    ：表示入参错误(为NULL)或定时器未初始化
 */
HAL_StatusTypeDef HAL_TIM_DeInit(HAL_TIM_HandleTypeDef *htim);

/**
 * @brief  开启定时器.耗时16us
 * @param  htim 指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示定时器启动成功
 *         @retval  HAL_ERROR    ：表示定时器未初始化
 *         @retval  HAL_BUSY     : 表示定时器正在使用
 */
HAL_StatusTypeDef HAL_TIM_Start(HAL_TIM_HandleTypeDef *htim);

/**
 * @brief  停止定时器.
 * @param  htim 指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示成功停掉定时器
 *         @retval  HAL_ERROR    ：表示定时器未初始化
 *         @retval  HAL_BUSY     : 表示定时器正在使用
 */
HAL_StatusTypeDef HAL_TIM_Stop(HAL_TIM_HandleTypeDef *htim);

/**
 * @brief  获取定时器Count寄存器中的值.
 * @param  htim 指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
 * @retval Count寄存器中的值，定时器每经过一个时钟周期就做一次加1.
 */
uint32_t HAL_TIM_GetCount(HAL_TIM_HandleTypeDef *htim);

/**
 * @brief  获取定时器CaptureCount寄存器中的值.
 * @param  htim 指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
 * @retval CaptureCount寄存器中的值，仅用于capture模式读取捕获有效边沿时的定时器计数值.
 */
uint32_t HAL_TIM_GetCaptureCount(HAL_TIM_HandleTypeDef *htim);

/**
 * @brief  设置定时器Count寄存器中的值.
 * @param  htim	指向一个包含定时器具体配置信息的HAL_TIM_HandleTypeDef结构体的指针.详情参考 @ref HAL_TIM_HandleTypeDef.
 * @param	 count	设置的定时器Count寄存器中的值.
 */
void HAL_TIM_SetCount(HAL_TIM_HandleTypeDef *htim, uint32_t count);

/**
 * @brief  Timer1中断回调函数.
 */
void HAL_TIM1_Callback(void);

/**
 * @brief  Timer2中断回调函数.
 */
void HAL_TIM2_Callback(void);

/**
 * @brief  Timer3中断回调函数.
 */
void HAL_TIM3_Callback(void);
