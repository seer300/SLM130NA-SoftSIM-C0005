/**
 * @file i2c_slave_test.c
 * @brief 从测试设备代码
 * @version 0.2
 * @attention  由于hal库I2C从机代码已引掉，测试I2C从机时需要放开，否则编译报错，无法测试
 * @date 2022-06-20
 */
#include <string.h>
#include "slave.h"
#include "hal_gpio.h"
#include "hal_i2c.h"

#if (DRIVER_TEST == 2)

/******************************************************************************************/
HAL_I2C_HandleTypeDef i2c_slave_handle = {0};
volatile uint8_t i2c_slave_rx_flag = 0;

__RAM_FUNC void HAL_I2C1_ErrorCallback(HAL_I2C_HandleTypeDef *hi2c)
{
	hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
}

__RAM_FUNC void HAL_I2C1_RxCpltCallback(HAL_I2C_HandleTypeDef *hi2c)
{
	UNUSED_ARG(hi2c);
	i2c_slave_rx_flag = 1;
}

__RAM_FUNC void HAL_I2C2_ErrorCallback(HAL_I2C_HandleTypeDef *hi2c)
{
	hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
}

__RAM_FUNC void HAL_I2C2_RxCpltCallback(HAL_I2C_HandleTypeDef *hi2c)
{
	UNUSED_ARG(hi2c);
	i2c_slave_rx_flag = 1;
}

static void i2c_slave_init(I2cArgStruct *pI2cArgStruct)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};

    if(pI2cArgStruct -> I2cInstance == (uint32_t)HAL_I2C1)
    {
        gpio_init.Pin		= (HAL_GPIO_PinTypeDef)pI2cArgStruct -> I2cSclPin;
        gpio_init.Mode		= GPIO_MODE_HW_PER;
        gpio_init.PinRemap	= GPIO_I2C1_SCL;
        HAL_GPIO_Init(&gpio_init);
        gpio_init.Pin		= (HAL_GPIO_PinTypeDef)pI2cArgStruct -> I2cSdaPin;
        gpio_init.Mode		= GPIO_MODE_HW_PER;
        gpio_init.PinRemap	= GPIO_I2C1_SDA;
        HAL_GPIO_Init(&gpio_init);
    }
	else if(pI2cArgStruct -> I2cInstance == (uint32_t)HAL_I2C2)
    {
        gpio_init.Pin		= (HAL_GPIO_PinTypeDef)pI2cArgStruct -> I2cSclPin;
	    gpio_init.Mode		= GPIO_MODE_HW_PER;
		gpio_init.PinRemap	= GPIO_I2C2_SCL;
	    HAL_GPIO_Init(&gpio_init);
	    gpio_init.Pin		= (HAL_GPIO_PinTypeDef)pI2cArgStruct -> I2cSdaPin;
	    gpio_init.Mode		= GPIO_MODE_HW_PER;
		gpio_init.PinRemap	= GPIO_I2C2_SDA;
	    HAL_GPIO_Init(&gpio_init);
    }

    i2c_slave_handle.Instance			 = (I2C_TypeDef *)pI2cArgStruct -> I2cInstance;
    i2c_slave_handle.Init.Mode			 = HAL_I2C_MODE_SLAVE;
    i2c_slave_handle.Init.ClockSpeed	 = pI2cArgStruct -> I2cClockSpeed;
    i2c_slave_handle.Init.AddressingMode = pI2cArgStruct -> I2cAddressMode;
    i2c_slave_handle.Init.OwnAddress 	 = pI2cArgStruct -> I2cSlaveAddress;
    HAL_I2C_Init(&i2c_slave_handle);
}

/******************************************************************************************/
void i2c_slave_test(I2cArgStruct *pI2cArgStruct)
{
	HAL_StatusTypeDef ret = HAL_OK;
	char RecvConfigBuffer[10] = {0};
    static uint32_t TestTimesFlag = 0;
	TestTimesFlag++;

	Debug_Print_Str(DEBUG_LEVEL_2, "\r\n<i2c> test begin\r\n");
	Debug_Print_ArgStruct(DEBUG_LEVEL_1, pI2cArgStruct);

    /*************************从机初始化并准备接收缓存区****************************/
    //i2c从机初始化
    i2c_slave_init(pI2cArgStruct);
    //根据size申请内存，并初始化为全0
	char *pRecvBuffer = (char *)xy_malloc(pI2cArgStruct -> I2cTestSize);
    memset(pRecvBuffer, 0, pI2cArgStruct -> I2cTestSize);

    /***********************回应"OK\n"************************/
	SEND_OK;

    /***********************i2c从机接收"************************/
	if(TestTimesFlag % 2 ==0)
	{
        /**********从机阻塞接收**********/
        Debug_Print_Str(DEBUG_LEVEL_1, "\r\ni2c从机阻塞接收");

	    WAIT_OK;

        ret = HAL_I2C_Slave_Receive(&i2c_slave_handle, (uint8_t *)pRecvBuffer, pI2cArgStruct->I2cTestSize, RECEIVE_DEFAULT_MAX_DELAY);
        if(ret != HAL_OK)
		{
            return;
            
        }
	}
	else
	{
        /**********从机非阻塞接收********/
        Debug_Print_Str(DEBUG_LEVEL_1, "\r\ni2c从机非阻塞接收");

        WAIT_OK;

        ret = HAL_I2C_Slave_Receive_IT(&i2c_slave_handle, (uint8_t *)pRecvBuffer, pI2cArgStruct->I2cTestSize);
        if(ret != HAL_OK)
		{
			return;
		}
		while(i2c_slave_rx_flag != 1);
		i2c_slave_rx_flag = 0;
	}
    Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pI2cArgStruct->I2cTestSize);

    /***********************uart从机发送"************************/
    Debug_Print_Str(DEBUG_LEVEL_1, "\r\ni2c从机阻塞发送");
    Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pI2cArgStruct->I2cTestSize);

    HAL_Delay(5); //确保主机先等待OK
    SEND_OK;

    ret = HAL_I2C_Slave_Transmit(&i2c_slave_handle, (uint8_t *)pRecvBuffer, pI2cArgStruct->I2cTestSize, TRANSMIT_DEFAULT_MAX_DELAY);
    if(ret != HAL_OK)
	{
        return;
	}

    WAIT_OK;

    HAL_I2C_Drivertest_DeInit(&i2c_slave_handle);

    xy_free(pRecvBuffer);

	Debug_Print_Str(DEBUG_LEVEL_2, "\r\n<i2c> test end\r\n");
}

#endif /* #if (DRIVER_TEST == 2) */
