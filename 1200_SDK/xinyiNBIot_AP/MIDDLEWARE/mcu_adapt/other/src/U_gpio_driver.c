/************************************************************************************
* @Copyright (c)	  :(C)2020, Qindao ieslab Co., Ltd
* @FileName   		  :hc32_gpio_driver.c
* @Author           :QDIES PLATFORM TEAM
* @Version          :V1.0
* @Date             :2020-07-01
* @Description      :
************************************************************************************/

#ifdef	__cplusplus
extern "C"
{
#endif

#include "U_gpio_driver.h"

/*****************************变量定义***************************************/


/*********************************************内部接口*************************************************/
__RAM_FUNC void GpioInit(void)
{
    McuGpioModeSet(EN_IR_PWR_GPIO_PIN, 0x00);    //推挽输出
    McuGpioDrvStrengthSet(EN_IR_PWR_GPIO_PIN,3); //高驱动能力
}

/*********************************************外部接口*************************************************/
/**
  * @brief  Read the specified input port pin.
  * @param  num：GPIO端口号，可选MCU_GPIO0-63，详情参考 @ref MCU_GPIO_PinTypeDef.
  *         This parameter can be GPIO_PIN_x where x can be (0..15).
  * @retval The input port pin value.
  */
__RAM_FUNC GPIO_PinState GPIO_ReadPin2(MCU_GPIO_PinTypeDef num)
{
    return McuGpioRead(num);
}

/*******************************************************************************************************
  * @brief  Set or clear the selected data port bit.
  *
  * @note   This function uses GPIOx_BSRR and GPIOx_BRR registers to allow atomic read/modify
  *         accesses. In this way, there is no risk of an IRQ occurring between
  *         the read and the modify access.
  *
  * @param  num：GPIO端口号，可选MCU_GPIO0-63，详情参考 @ref MCU_GPIO_PinTypeDef
  * @param  PinState: specifies the value to be written to the selected bit.
  *         This parameter can be one of the GPIO_PinState enum values:
  *            @arg GPIO_PIN_RESET: to clear the port pin
  *            @arg GPIO_PIN_SET: to set the port pin
  * @retval None
  *****************************************************************************************************/
__RAM_FUNC void GPIO_WritePin2(MCU_GPIO_PinTypeDef num, GPIO_PinState PinState)
{
    McuGpioWrite(num,PinState);
}

/*******************************************************************************************************
  * @brief  Toggle the specified GPIO pin.
  * @param  num：GPIO端口号，可选MCU_GPIO0-63，详情参考 @ref MCU_GPIO_PinTypeDef
  * @retval None
  *****************************************************************************************************/
__RAM_FUNC void GPIO_TogglePin2(MCU_GPIO_PinTypeDef num)
{
    McuGpioToggle(num);
}

#ifdef	__cplusplus
}
#endif
