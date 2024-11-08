/**
  ******************************************************************************
  * @file	  hal_lptimer.h
  * 
  * @note   LPTIMER最长定时时长不得大于256s，否则会主动断言！
  * 
  * @brief	此文件包含低功耗定时器外设的变量，枚举，结构体定义，函数声明等. 
  * @note   低功耗定时器在芯片进入睡眠状态后，依然可以工作。
 ******************************************************************************
 */
#pragma once

#include "prcm.h"
#include "lptimer.h"

/**
 * @brief  低功耗定时器工作模式
 */
typedef enum
{
  HAL_LPTIM_MODE_CONTINUOUS = LPTIMER_CTL_TMODE_CONTINUOUS,   /*!< 循环触发模式，连续多次定时  */
  HAL_LPTIM_MODE_COUNTER = LPTIMER_CTL_TMODE_COUNTER,         /*!< 计数模式，对外部输入信号的有效边沿进行计数 */
  HAL_LPTIM_MODE_PWM_SINGLE = LPTIMER_CTL_TMODE_PWM_SINGLE,   /*!< 单PWM模式，输出一路脉冲调制波形 */
  HAL_LPTIM_MODE_CAPTURE = LPTIMER_CTL_TMODE_CAPTURE,         /*!< 捕获模式，捕捉外部输入信号的有效边沿 */
  HAL_LPTIM_MODE_COMPARE = LPTIMER_CTL_TMODE_COMPARE,         /*!< 比较模式，主要用于单次定时 */
  HAL_LPTIM_MODE_GATED = LPTIMER_CTL_TMODE_GATED,             /*!< 门模式，主要用于脉宽测量 */
  HAL_LPTIM_MODE_PWM_DUAL = LPTIMER_CTL_TMODE_PWM_DUAL        /*!< 双PWM模式，输出两路互补带死区的PWM波 */
} HAL_LPTIM_WorkModeTypeDef;

/**
 * @brief  低功耗定时器分频系数
 */
typedef enum
{
  HAL_LPTIM_CLK_DIV_1 = LPTIMER_CTL_PRES_DIVIDE_1,    /*!< 1分频 */
  HAL_LPTIM_CLK_DIV_2 = LPTIMER_CTL_PRES_DIVIDE_2,    /*!< 2分频 */
  HAL_LPTIM_CLK_DIV_4 = LPTIMER_CTL_PRES_DIVIDE_4,    /*!< 4分频 */
  HAL_LPTIM_CLK_DIV_8 = LPTIMER_CTL_PRES_DIVIDE_8,    /*!< 8分频 */
  HAL_LPTIM_CLK_DIV_16 = LPTIMER_CTL_PRES_DIVIDE_16,  /*!< 16分频 */
  HAL_LPTIM_CLK_DIV_32 = LPTIMER_CTL_PRES_DIVIDE_32,  /*!< 32分频 */
  HAL_LPTIM_CLK_DIV_64 = LPTIMER_CTL_PRES_DIVIDE_64,  /*!< 64分频 */
  HAL_LPTIM_CLK_DIV_128 = LPTIMER_CTL_PRES_DIVIDE_128 /*!< 128分频 */
} HAL_LPTIM_ClkDivTypeDef;

/**
 * @brief  低功耗定时器双PWM模式死区时间，注意此参数单位为低功耗定时器时钟源周期，不受低功耗定时器的分频系数影响
 */
typedef enum
{
  HAL_LPTIM_DELAY_0_CYCLES = LPTIMER_CTL_PWMD_NO_DELAY,         /*!< 0个周期 */
  HAL_LPTIM_DELAY_2_CYCLES = LPTIMER_CTL_PWMD_DELAY_2_CYCLE,    /*!< 2个周期 */
  HAL_LPTIM_DELAY_4_CYCLES = LPTIMER_CTL_PWMD_DELAY_4_CYCLE,    /*!< 4个周期 */
  HAL_LPTIM_DELAY_8_CYCLES = LPTIMER_CTL_PWMD_DELAY_8_CYCLE,    /*!< 8个周期 */
  HAL_LPTIM_DELAY_16_CYCLES = LPTIMER_CTL_PWMD_DELAY_16_CYCLE,  /*!< 16个周期 */
  HAL_LPTIM_DELAY_32_CYCLES = LPTIMER_CTL_PWMD_DELAY_32_CYCLE,  /*!< 32个周期 */
  HAL_LPTIM_DELAY_64_CYCLES = LPTIMER_CTL_PWMD_DELAY_64_CYCLE,  /*!< 64个周期 */
  HAL_LPTIM_DELAY_128_CYCLES = LPTIMER_CTL_PWMD_DELAY_128_CYCLE /*!< 128个周期 */
} HAL_LPTIM_DeadtimeTypeDef;

/**
 * @brief  低功耗定时器各工作模式输出电平及输入触发源设置.
 * @note	不同模式下极性对应的含义不同，具体含义如下:
 *  循环触发模式：此模式下无需配置.
 *	计数模式: Polarity_Low表示上升沿计数，Polarity_High表示下降沿计数.
 *	捕获模式: Polarity_Low表示捕获上升沿，Polarity_High表示捕获下降沿.
 *	比较模式: 此模式下无需配置.
 *	门模式: Polarity_Low表示在输入为高电平时Count寄存器计数，下降沿触发中断，Polarity_High表示在输入为低电平时Count寄存器计数，上升沿触发中断.
 *	单PWM模式: Polarity_Low表示低功耗定时器失能时输出为低电平，开启低功耗定时器Count寄存器计数值达到PWM寄存器的值后翻转为高电平，Count寄存器计数值达到Reload寄存器重载值后重新输出低电平。
               Polarity_High表示低功耗定时器失能时输出为高电平，开启低功耗定时器Count寄存器计数值达到PWM寄存器的值后翻转为低电平，Count寄存器计数值达到Reload寄存器重载值后重新输出高电平。 
 *	双PWM模式：含义与单PWM模式相同，区别在于双PWM模式会输出另一类极性相反的互补PWM波，且可以设置死区长度，详情参考 @ref HAL_LPTIM_DeadtimeTypeDef.
 */
typedef enum
{
  HAL_LPTIM_Polarity_Low =  LPTIMER_CTL_TPOL_FALSE,   /*!< 输出类模式下：低功耗定时器失能时输出低电平；输入类模式下：上升沿为有效边沿 */
  HAL_LPTIM_Polarity_High = LPTIMER_CTL_TPOL_TRUE     /*!< 输出类模式下：低功耗定时器失能时输出高电平；输入类模式下：下降沿为有效边沿 */
} HAL_LPTIM_PolarityTypeDef;


/**
 * @brief  低功耗定时器工作状态枚举
 */
typedef enum
{
  HAL_LPTIM_STATE_RESET = 0x00U,   /*!< 外设未初始化 */
  HAL_LPTIM_STATE_READY = 0x01U,   /*!< 外设初始化完成可以使用 */
  HAL_LPTIM_STATE_BUSY = 0x02U,    /*!< 外设正在工作状态下 */
} HAL_LPTIM_StateTypeDef;

/**
 * @brief 低功耗定时器初始化结构体
 */
typedef struct
{
  HAL_LPTIM_ClkDivTypeDef Division;             /*!< 低功耗定时器时钟分频系数 */
  uint16_t Mode;                                /*!< 低功耗定时器工作模式 */
  uint16_t Reload;                              /*!< 低功耗定时器重载值,capture/gated模式时不需要配置，计数模式时待计数的有效边沿数，其它模式下为重载值 */
  uint16_t PWMCmp;                              /*!< 低功耗定时器PWM寄存器值,仅用于低功耗定时器PWM模式，PMW电平翻转时的比较值，决定PWM波的占空比 */
  HAL_LPTIM_PolarityTypeDef Polarity;           /*!< 低功耗定时器极性，不同模式下含义不同，详情参考 @ref HAL_LPTIM_PolarityTypeDef */
  HAL_LPTIM_DeadtimeTypeDef DualPWMDeadtime;    /*!< 仅用于定时器双PWM模式下，此参数用于设置PWM死区的长度 */
  uint32_t SingPWMOUTPolinDSleep;               /*!< 仅用于定时器单PWM模式下，此参数用于选择深睡时输出PWM的极性 */
} HAL_LPTIM_InitTypeDef;

/**
 * @brief  低功耗定时器控制结构体
 */
typedef struct
{
  LPTMR_TypeDef *Instance;                  /*!< 低功耗定时器基地址 */
  HAL_LPTIM_InitTypeDef Init;               /*!< 低功耗定时器初始化结构体 */
  HAL_LockTypeDef Lock;                     /*!< 低功耗定时器设备锁 */
  volatile HAL_LPTIM_StateTypeDef State;    /*!< 低功耗定时器状态 */
} HAL_LPTIM_HandleTypeDef;

/**
 * @brief  低功耗定时器外设基地址
 */
#define HAL_LPTIM1 LPTMR1
#define HAL_LPTIM2 LPTMR2

/**
 * @brief 用于低功耗定时器在循环触发模式下设置超时时间。耗时39us
 * 
 * @param hlptim 指向一个包含低功耗定时器具体配置信息的HAL_LPTIM_HandleTypeDef结构体的指针. 详情参考 @ref HAL_LPTIM_HandleTypeDef.
 * @param timeout 超时时长，单位ms，由于LPTIMER特性（16位计数，参考时钟32K，最大分频128），最长超时时长约为256000ms
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示初始化外设成功
 *         @retval  HAL_ERROR    ：表示入参错误(为NULL)、模式错误或低功耗定时器正在使用中
 */
HAL_StatusTypeDef HAL_LPTIM_SetTimeout(HAL_LPTIM_HandleTypeDef *hlptim, uint32_t timeout);

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
HAL_StatusTypeDef HAL_LPTIM_GetTime(HAL_LPTIM_HandleTypeDef *hlptim, uint16_t tick1, uint16_t tick2, uint32_t *time);

/**
 * @brief  初始化低功耗定时器.耗时139us
 * @param  hlptim 指向一个包含低功耗定时器具体配置信息的HAL_LPTIM_HandleTypeDef结构体的指针. 详情参考 @ref HAL_LPTIM_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示初始化外设成功
 *         @retval  HAL_ERROR    ：表示入参错误(为NULL)、模式错误或低功耗定时器正在使用中
 */
HAL_StatusTypeDef HAL_LPTIM_Init(HAL_LPTIM_HandleTypeDef *hlptim);

/**
 * @brief  去初始化低功耗定时器.耗时52us
 * @param  hlptim 指向一个包含低功耗定时器具体配置信息的HAL_LPTIM_HandleTypeDef结构体的指针. 详情参考 @ref HAL_LPTIM_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示去初始化成功
 *         @retval  HAL_ERROR    ：表示入参错误(为NULL)或低功耗定时器未初始化
 */
HAL_StatusTypeDef HAL_LPTIM_DeInit(HAL_LPTIM_HandleTypeDef *hlptim);

/**
 * @brief  开启低功耗定时器.耗时17us
 * @param  hlptim 指向一个包含低功耗定时器具体配置信息的HAL_LPTIM_HandleTypeDef结构体的指针. 详情参考 @ref HAL_LPTIM_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示低功耗定时器启动成功
 *         @retval  HAL_ERROR    ：表示低功耗定时器未初始化
 *         @retval  HAL_BUSY     : 表示低功耗定时器正在使用
 */
HAL_StatusTypeDef HAL_LPTIM_Start(HAL_LPTIM_HandleTypeDef *hlptim);

/**
 * @brief  停止低功耗定时器.
 * @param  hlptim 指向一个包含低功耗定时器具体配置信息的HAL_LPTIM_HandleTypeDef结构体的指针. 详情参考 @ref HAL_LPTIM_HandleTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK       ：表示成功停掉低功耗定时器
 *         @retval  HAL_ERROR    ：表示低功耗定时器未初始化
 *         @retval  HAL_BUSY     : 表示低功耗定时器正在使用
 */
HAL_StatusTypeDef HAL_LPTIM_Stop(HAL_LPTIM_HandleTypeDef *hlptim);

/**
 * @brief  获取低功耗定时器Count寄存器中的值.
 * @param  hlptim 指向一个包含低功耗定时器具体配置信息的HAL_LPTIM_HandleTypeDef结构体的指针. 详情参考 @ref HAL_LPTIM_HandleTypeDef.
 * @retval Count寄存器中的值，低功耗定时器每经过一个时钟周期就做一次加1.
 */
uint16_t HAL_LPTIM_GetCount(HAL_LPTIM_HandleTypeDef *hlptim);

/**
 * @brief  获取低功耗定时器CaptureCount寄存器中的值.
 * @param  hlptim 指向一个包含低功耗定时器具体配置信息的HAL_LPTIM_HandleTypeDef结构体的指针. 详情参考 @ref HAL_LPTIM_HandleTypeDef.
 * @retval CaptureCount寄存器中的值，仅用于capture模式读取捕获有效边沿时的低功耗定时器计数值.
 */
uint16_t HAL_LPTIM_GetCaptureCount(HAL_LPTIM_HandleTypeDef *hlptim);

/**
 * @brief  设置低功耗定时器Count寄存器中的值.
 * @param  hlptim	指向一个包含低功耗定时器具体配置信息的HAL_LPTIM_HandleTypeDef结构体的指针. 详情参考 @ref HAL_LPTIM_HandleTypeDef.
 * @param	 count	设置低功耗定时器Count寄存器中的值.
 */
void HAL_LPTIM_SetCount(HAL_LPTIM_HandleTypeDef *hlptim, uint16_t count);

/**
 * @brief  lptimer中断回调函数.
 */
void HAL_LPTIM1_Callback(void);