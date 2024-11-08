#include "hal_def.h"
#include "sys_clk.h"
#include <string.h>
#include "hal_csp.h"
#include "hal_gpio.h"
#include "hal_i2c.h"
#include "hal_uart.h"
#include "hal_spi.h"
#include "hal_lptimer.h"
#include "hal_lpuart.h"

/**
 * @brief lptimer1中断回调函数.
 */
__WEAK void HAL_LPTIM1_Callback(void)
{	

}

/**
 * @brief lptimer2中断回调函数.
 */
__WEAK void HAL_LPTIM2_Callback(void)
{	

}

/**
 * @brief   GPIO中断回调函数
 * @warning 用户自己在实现该函数时，不能直接覆盖，应该拷贝该函数体内部的全部内容，然后在后面添加用户自己的处理代码.
 */
__WEAK void HAL_GPIO_InterruptCallback(void)
{
}


/**
 * @brief  CSP1错误中断回调函数.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
__WEAK void HAL_CSP1_ErrorCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	UNUSED_ARG(hcsp);
}

/**
 * @brief  CSP1接收完成回调函数.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
__WEAK void HAL_CSP1_RxCpltCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	UNUSED_ARG(hcsp);
}

/**
 * @brief  CSP1发送完成回调函数.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
__WEAK void HAL_CSP1_TxCpltCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	UNUSED_ARG(hcsp);
}

/**
 * @brief  CSP2错误中断回调函数.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
__WEAK void HAL_CSP2_ErrorCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	UNUSED_ARG(hcsp);
}

/**
 * @brief  CSP2接收完成回调函数.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
__WEAK void HAL_CSP2_RxCpltCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	UNUSED_ARG(hcsp);
}

/**
 * @brief  CSP2发送完成回调函数.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
__WEAK void HAL_CSP2_TxCpltCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	UNUSED_ARG(hcsp);
}

/**
 * @brief  CSP3错误中断回调函数.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
__WEAK void HAL_CSP3_ErrorCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	UNUSED_ARG(hcsp);
}

/**
 * @brief  CSP3接收完成回调函数.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
__WEAK void HAL_CSP3_RxCpltCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	UNUSED_ARG(hcsp);
}

/**
 * @brief  CSP3发送完成回调函数.
 *
 * @param  hcsp. 详情参考 @ref HAL_CSP_HandleTypeDef.
 */
__WEAK void HAL_CSP3_TxCpltCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	UNUSED_ARG(hcsp);
}


/**
 * @brief  I2C1错误回调函数
 *
 * @param  hi2c. 详见结构体定义 HAL_I2C_HandleTypeDef.Def.
 */
__WEAK void HAL_I2C1_ErrorCallback(HAL_I2C_HandleTypeDef *hi2c)
{
	UNUSED_ARG(hi2c);
}

/**
 * @brief  I2C1发送完成回调函数
 *
 * @param  hi2c. 详见结构体定义 HAL_I2C_HandleTypeDef.
 */
__WEAK void HAL_I2C1_TxCpltCallback(HAL_I2C_HandleTypeDef *hi2c)
{
	UNUSED_ARG(hi2c);
}

/**
 * @brief  I2C1接收完成回调函数
 *
 * @param  hi2c. 详见结构体定义 HAL_I2C_HandleTypeDef.
 */
__WEAK void HAL_I2C1_RxCpltCallback(HAL_I2C_HandleTypeDef *hi2c)
{
	UNUSED_ARG(hi2c);
}

/**
 * @brief  I2C2错误回调函数
 *
 * @param  hi2c. 详见结构体定义 HAL_I2C_HandleTypeDef.Def.
 */
__WEAK void HAL_I2C2_ErrorCallback(HAL_I2C_HandleTypeDef *hi2c)
{
	UNUSED_ARG(hi2c);
}

/**
 * @brief  I2C2发送完成回调函数
 *
 * @param  hi2c. 详见结构体定义 HAL_I2C_HandleTypeDef.
 */
__WEAK void HAL_I2C2_TxCpltCallback(HAL_I2C_HandleTypeDef *hi2c)
{
	UNUSED_ARG(hi2c);
}

/**
 * @brief  I2C2接收完成回调函数
 *
 * @param  hi2c. 详见结构体定义 HAL_I2C_HandleTypeDef.
 */
__WEAK void HAL_I2C2_RxCpltCallback(HAL_I2C_HandleTypeDef *hi2c)
{
	UNUSED_ARG(hi2c);
}



/**
 * @brief  SPI错误中断回调函数
 *
 * @param  hspi. 详见结构体定义  @ref HAL_SPI_HandleTypeDef.
 */
__WEAK void HAL_SPI_ErrorCallback(HAL_SPI_HandleTypeDef *hspi)
{
	UNUSED_ARG(hspi);
}

/**
 * @brief  SPI发送完成回调函数
 *
 * @param  hspi. 详见结构体定义  @ref HAL_SPI_HandleTypeDef.
 */
__WEAK void HAL_SPI_RxCpltCallback(HAL_SPI_HandleTypeDef *hspi)
{
	UNUSED_ARG(hspi);
}

/**
 * @brief  SPI接收完成回调函数
 *
 * @param  hspi. 详见结构体定义  @ref HAL_SPI_HandleTypeDef.
 */
__WEAK void HAL_SPI_TxCpltCallback(HAL_SPI_HandleTypeDef *hspi)
{
	UNUSED_ARG(hspi);
}

/**
 * @brief  UART错误回调函数
 * @param  huart. 详见结构体定义 HAL_UART_HandleTypeDef.
 */
__WEAK void HAL_UART_ErrorCallback(HAL_UART_HandleTypeDef *huart)
{
	UNUSED_ARG(huart);
}

/**
 * @brief  UART发送完成回调函数
 * @param  huart. 详见结构体定义 HAL_UART_HandleTypeDef.
 */
__WEAK void HAL_UART_TxCpltCallback(HAL_UART_HandleTypeDef *huart)
{
	UNUSED_ARG(huart);
}

/**
 * @brief  UART接收完成回调函数
 * @param  huart. 详见结构体定义 HAL_UART_HandleTypeDef.
 * @note   该回调函数会在指定长度(RxXferSize)数据接收完成后被调用，或者LPUART模块接收超时后被调用.
 */
__WEAK void HAL_UART_RxCpltCallback(HAL_UART_HandleTypeDef *huart)
{
	UNUSED_ARG(huart);
}

/**
 * @brief  LPUART错误回调函数
 * @param  hlpuart. 详见结构体定义 HAL_LPUART_HandleTypeDef.
 */
__WEAK void HAL_LPUART_ErrorCallback(HAL_LPUART_HandleTypeDef *hlpuart)
{
	UNUSED_ARG(hlpuart);
}

/**
 * @brief  LPUART发送完成回调函数.
 * @param  hlpuart. 详见结构体定义 HAL_LPUART_HandleTypeDef.
 */
__WEAK void HAL_LPUART_TxCpltCallback(HAL_LPUART_HandleTypeDef *hlpuart)
{
	UNUSED_ARG(hlpuart);
}

/**
 * @brief  LPUART接收完成回调函数.
 * @param  hlpuart. 详见结构体定义 HAL_LPUART_HandleTypeDef.
 * @note   该回调函数会在指定长度(RxXferSize)数据接收完成后被调用，或者LPUART模块接收超时后被调用.
 */
__WEAK void HAL_LPUART_RxCpltCallback(HAL_LPUART_HandleTypeDef *hlpuart)
{
	UNUSED_ARG(hlpuart);
}
