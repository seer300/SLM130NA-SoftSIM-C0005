/************************************************************************************
* @Copyright (c)	:(C)2020, Qindao ieslab Co., Ltd
* @FileName   		:hc32_gpio_driver.h
* @Author           :QDIES PLATFORM TEAM
* @Version          :V1.0
* @Date             :2020-07-01
* @Description      :
************************************************************************************/

#ifndef __U_GPIO_DRIVER_H
#define __U_GPIO_DRIVER_H

#ifdef	__cplusplus
extern "C"
{
#endif
	
#include "xy_system.h"
#include "type.h"
#include "mcu_adapt.h"

/*****************************变量定义***************************************/

//GPIO时钟使能
#define MCU_GPIO_CLK_ENABLE()       PRCM_ClockEnable(CORE_CKG_CTL_GPIO_EN)
//GPIO时钟禁止
#define MCU_GPIO_CLK_DISABLE()      PRCM_ClockDisable(CORE_CKG_CTL_GPIO_EN)

#define EN_IR_PWR_GPIO_PIN               (MCU_GPIO10)

/*****************************外部接口***************************************/
//void MCU_GPIO_Init(GPIO_TypeDef GPIOx, GPIO_InitTypeDef *GPIO_Init);
//void MCU_GPIO_DeInit(GPIO_TypeDef GPIOx, GPIO_PIN_t GPIO_Pin);
void GpioInit(void);
GPIO_PinState GPIO_ReadPin2(MCU_GPIO_PinTypeDef num);
void GPIO_WritePin2(MCU_GPIO_PinTypeDef num, GPIO_PinState PinState);
void GPIO_TogglePin2(MCU_GPIO_PinTypeDef num);
void GPIO_LockPin2(MCU_GPIO_PinTypeDef num);

#ifdef	__cplusplus
}
#endif

#endif	/* __STM32L4xx_GPIO_DRIVER_H */

