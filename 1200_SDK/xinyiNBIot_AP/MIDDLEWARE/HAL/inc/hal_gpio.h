/**
 ******************************************************************************
 * @file	hal_gpio.h
 * @brief	此文件包含GPIO外设的变量，枚举，结构体定义，函数声明等.
 * @note    具体GPIO特性请参考《XY1200引脚和外设使用说明》
 *
 ******************************************************************************
 * @attention
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 * http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

#pragma once
#include "hal_def.h"
#include "gpio.h"
#include "prcm.h"
#include "xy_system.h"

/**
 * @brief  GPIO状态枚举
 */
#define HAL_GPIO_StateTypeDef GPIO_PinState

/**
 * @brief  GPIO引脚枚举.
 */
#define HAL_GPIO_PinTypeDef GPIO_PadTypeDef

/**
 * @brief  GPIO重映射枚举
 */
#define HAL_GPIO_RemapTypeDef GPIO_RemapTypeDef

/**
 *  @brief GPIO工作模式枚举
 *
 *  GPIO_MODE_HW_PER        GPIO映射为外设模式
 *  GPIO_MODE_INPUT         普通GPIO输入模式
 *  GPIO_MODE_OUTPUT_PP     普通GPIO推挽输出模式
 *  GPIO_MODE_OUTPUT_OD     普通GPIO开漏输出模式
 *  GPIO_MODE_INOUT         普通GPIO输入输出模式
 */
#define HAL_GPIO_ModeTypeDef GPIO_ModeTypeDef

/**
 *  @brief GPIO中断触发类型枚举
 *
 *  GPIO_INT_FALL_EDGE   上升沿外部中断模式
 *  GPIO_INT_FALL_EDGE   下降沿外部中断模式
 *  GPIO_INT_BOTH_EDGE   双边沿外部中断模式
 *  GPIO_INT_HIGH_LEVEL  高电平外部中断模式
 */
#define HAL_GPIO_IntTypeDef GPIO_IntTypeDef

/**
 *  @brief GPIO上拉下拉枚举
 *
 *  GPIO_FLOAT       浮空
 *  GPIO_PULL_UP     上拉
 *  GPIO_PULL_DOWN   下拉
 */
#define HAL_GPIO_PullTypeDef GPIO_PullTypeDef

/**
 *  @brief    GPIO初始化结构体，有以下配置项
 *
 *  Pin       指定的GPIO.       详情参考 @ref HAL_GPIO_PinTypeDef
 *  Mode      GPIO工作模式.     详情参考 @ref HAL_GPIO_ModeTypeDef
 *  PinRemap  GPIO引脚功能映射. 详情参考 @ref HAL_GPIO_RemapTypeDef
 *  Pull      GPIO上下拉状态.   详情参考 @ref HAL_GPIO_PullTypeDef
 *  Int       GPIO中断触发类型. 详情参考 @ref HAL_GPIO_IntTypeDef
 */
#define HAL_GPIO_InitTypeDef GPIO_InitTypeDef

/**
 * @brief  初始化GPIO.
 *
 * @param  耗时55us
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK    ：表示GPIO初始化成功
 *         @retval  HAL_ERROR ：表示入参错误
 */
HAL_StatusTypeDef HAL_GPIO_Init(HAL_GPIO_InitTypeDef *hgpio);

/**
 * @brief 设置指定GPIO的上下拉状态
 * 
 * @param GPIO_Pin GPIO_Pin 指定的GPIO引脚.详情参考 @ref HAL_GPIO_PinTypeDef.
 * @param pull 上下拉状态.详情参考 @ref HAL_GPIO_PullTypeDef.
 */
void HAL_GPIO_SetPull(HAL_GPIO_PinTypeDef GPIO_Pin, HAL_GPIO_PullTypeDef pull);

/**
 * @brief  获取指定GPIO的电平状态.
 *
 * @param  GPIO_Pin 指定的GPIO引脚.详情参考 @ref HAL_GPIO_PinTypeDef.
 * @return 引脚状态.详情参考 @ref HAL_GPIO_StateTypeDef.
 *         参数可以是以下枚举值:
 *		   @retval GPIO_PIN_RESET : 指定GPIO为低电平
 *		   @retval GPIO_PIN_SET   : 指定GPIO为高电平
 */
HAL_GPIO_StateTypeDef HAL_GPIO_ReadPin(HAL_GPIO_PinTypeDef GPIO_Pin);

/**
 * @brief  设置指定GPIO输出状态.
 *
 * @param  GPIO_Pin 指定的GPIO引脚.详情参考 @ref HAL_GPIO_PinTypeDef.
 * @param  PinState 设置的输出状态.详情参考 @ref HAL_GPIO_StateTypeDef.
 *         参数可以是以下枚举值:
 *		   @arg GPIO_PIN_RESET : 指定GPIO输出低电平
 *		   @arg GPIO_PIN_SET   : 指定GPIO输出高电平
 */
void HAL_GPIO_WritePin(HAL_GPIO_PinTypeDef GPIO_Pin, HAL_GPIO_StateTypeDef PinState);

/**
 * @brief  同时设置多个GPIO输出状态.
 *
 * @param  GPIO_PinArray 指向的一组GPIO引脚.详情参考 @ref HAL_GPIO_PinTypeDef.
 * @param  PinNum GPIO个数
 * @param  PinState 设置的输出状态.详情参考 @ref HAL_GPIO_StateTypeDef.
 *         参数可以是以下枚举值:
 *		   @arg GPIO_PIN_RESET : 指定GPIO输出低电平
 *		   @arg GPIO_PIN_SET   : 指定GPIO输出高电平
 */
void HAL_GPIO_WritePinArray(HAL_GPIO_PinTypeDef *GPIO_PinArray, uint8_t PinNum, HAL_GPIO_StateTypeDef PinState);

/**
 * @brief  翻转指定GPIO输出状态.
 *
 * @param  GPIO_Pin 指定的GPIO引脚.详情参考 @ref HAL_GPIO_PinTypeDef.
 * @note   此API只能用于GPIO为输出模式
 */
void HAL_GPIO_TogglePin(HAL_GPIO_PinTypeDef GPIO_Pin);

/**
 * @brief  翻转指定AGPIO输出状态.
 *
 * @param  AGPIO_Pin 指定的AGPIO引脚.
 * @note   此API只能用于AGPIO为输出模式
 */
void HAL_AGPIO_TogglePin(uint8_t AGPIO_Pin);

/**
 * @brief  读取并清除所有GPIO中断值.
 *
 * @return GPIO中断状态寄存器的值
 */
uint64_t HAL_GPIO_ReadAndClearIntFlag(void);

/**
 * @brief  读取GPIO中断状态.
 *
 * @return GPIO中断状态标志，
 */
uint8_t HAL_GPIO_ReadIntFlag(HAL_GPIO_PinTypeDef GPIO_Pin);

/**
 * @brief  清除所有GPIO中断状态标志.
 */
void HAL_GPIO_ClearIntFlag(HAL_GPIO_PinTypeDef GPIO_Pin);

/**
 * @brief GPIO0-GPIO7的控制寄存器从AON区域切出，以便能重新控制GPIO0-GPIO7.
 *
 * @note 深睡唤醒时需要调用，否则唤醒后无法控制GPIO0-GPIO7.
 */
void HAL_AGPIO0_7_UPDATE(void);

/**
 * @brief  GPIO中断回调函数
 *
 */
void HAL_GPIO_InterruptCallback(void);
