/**
 * @file i2c_master_test.c
 * @brief 主设备测试代码
 * @version 0.2
 * @date 2022-10-08
 */

#include <stdio.h>
#include <string.h>
#include "hal_gpio.h"
#include "hal_i2c.h"
#include "master.h"

#if (DRIVER_TEST == 1)

/******************************************************************************************/
I2cArgStruct I2cArgStruct_Demo = {0};
uint32_t I2cSclPinArray[] = {GPIO_PAD_NUM_5,  MaxArg};
uint32_t I2cSdaPinArray[] = {GPIO_PAD_NUM_6,  MaxArg};
uint32_t I2cInstanceArray[] = {(uint32_t)HAL_I2C1, (uint32_t)HAL_I2C2, MaxArg};
uint32_t I2cAddressModeArray[] = {HAL_I2C_ADDRESS_7BITS, HAL_I2C_ADDRESS_10BITS, MaxArg};
uint32_t I2cClockSpeedArray[] = {HAL_I2C_SPEED_100K,
								/*HAL_I2C_SPEED_400K,*/	//I2C挂载在lsio时钟上，XY_SOR_VER==1时实际测试100K、400K可过；XY_SOR_VER==2时，1/2/4/8分频时，100K可过，400K不可过；I2C主模式（无从机）demo400K可过			
								MaxArg}; 
uint32_t I2cSlaveAddressArray[] = {0x24, 0x38, MaxArg};
uint32_t I2cTestSizeArray[] = {1, 2, 3, 31, 32, 33, 128, 512, MaxArg};

/******************************************************************************************/
HAL_I2C_HandleTypeDef i2c_master_handle = {0};
volatile uint8_t i2c_master_tx_flag = 0;
volatile uint8_t i2c_master_rx_flag = 0;

__RAM_FUNC void HAL_I2C1_ErrorCallback(HAL_I2C_HandleTypeDef *hi2c)
{
	hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
}

__RAM_FUNC void HAL_I2C1_RxCpltCallback(HAL_I2C_HandleTypeDef *hi2c)
{
	UNUSED_ARG(hi2c);
	i2c_master_rx_flag = 1;
}

__RAM_FUNC void HAL_I2C1_TxCpltCallback(HAL_I2C_HandleTypeDef *hi2c)
{
	UNUSED_ARG(hi2c);
	i2c_master_tx_flag = 1;
}

__RAM_FUNC void HAL_I2C2_ErrorCallback(HAL_I2C_HandleTypeDef *hi2c)
{
	hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
}

__RAM_FUNC void HAL_I2C2_RxCpltCallback(HAL_I2C_HandleTypeDef *hi2c)
{
	UNUSED_ARG(hi2c);
	i2c_master_rx_flag = 1;
}

__RAM_FUNC void HAL_I2C2_TxCpltCallback(HAL_I2C_HandleTypeDef *hi2c)
{
	UNUSED_ARG(hi2c);
	i2c_master_tx_flag = 1;
}

static void i2c_master_init(I2cArgStruct *pI2cArgStruct)
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

	i2c_master_handle.Instance            = (I2C_TypeDef *)pI2cArgStruct -> I2cInstance;
	i2c_master_handle.Init.Mode           = HAL_I2C_MODE_MASTER;
	i2c_master_handle.Init.ClockSpeed     = pI2cArgStruct -> I2cClockSpeed;
	i2c_master_handle.Init.AddressingMode = pI2cArgStruct -> I2cAddressMode;
	i2c_master_handle.Init.OwnAddress     = 0x00;
	HAL_I2C_Init(&i2c_master_handle);
}

/******************************************************************************************/
static void i2c_test(CuTest* tc, I2cArgStruct *pI2cArgStruct, uint32_t TestTimes)
{
    HAL_StatusTypeDef ret = HAL_OK;
	char CmdData[100] = {0};
	char RecvConfigBuffer[10] = {0};

    for(uint16_t i = 0; i < TestTimes; i++)
	{
		Debug_Print_Str(DEBUG_LEVEL_1, "\r\ntest times:%d/%d\r\n", i+1, TestTimes);

        /*************************主机初始化并准备收发缓存区****************************/
		//主机i2c初始化
		i2c_master_init(pI2cArgStruct);
        //根据size申请内存，并初始化为全0
		char *pSendBuffer = (char *)xy_malloc(pI2cArgStruct -> I2cTestSize + 1);
		char *pRecvBuffer = (char *)xy_malloc(pI2cArgStruct -> I2cTestSize + 1);
        memset(pSendBuffer, 0, pI2cArgStruct -> I2cTestSize + 1);
        memset(pRecvBuffer, 0, pI2cArgStruct -> I2cTestSize + 1);
        //初始化pSendBuffer
		for(uint16_t i = 0; i < pI2cArgStruct -> I2cTestSize; i++)
		{
			//i为64整数倍的时候，不要赋值'\0'
			((i % 64 == 0) ? *(pSendBuffer + i) = '\n' : (*(pSendBuffer + i) = i % 64));
		}

        /*****************************主机发送初始化参数给从机*********************************/
		GetCmdDataToSlave(CmdData, (uint32_t)test_I2C, sizeof(I2cArgStruct), InputDebugLevel, pI2cArgStruct);
		HAL_CSP_Transmit(&MasterSlave_UART_Handle, (uint8_t *)CmdData, sizeof(I2cArgStruct) + 12, TRANSMIT_DEFAULT_MAX_DELAY);

		/*************************主机等待从机初始化完成后回复"OK\n"************************/
		ret = WAIT_OK;
		Debug_Print_Str(DEBUG_LEVEL_2, "\r\n%s", RecvConfigBuffer);
		if(ret != HAL_OK)
		{
			CuFail(tc, "master send config failed");
		}
		if(strstr(RecvConfigBuffer, "OK") == NULL)
		{
			CuFail(tc, "slave config failed");
		}

    	/*************************主机i2c开始向从机发送数据并接收"************************/
		if(i % 2 == 0)
		{
			/*******主机阻塞发送*******/
			Debug_Print_Str(DEBUG_LEVEL_1, "\r\ni2c主机阻塞发送");
			Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pSendBuffer, pI2cArgStruct->I2cTestSize);

			HAL_Delay(5); //确保从机先等待OK
			SEND_OK;
			HAL_Delay(20); //确保从机先进入接收API

			if(pI2cArgStruct -> I2cAddressMode == HAL_I2C_ADDRESS_7BITS)
			{
				ret = HAL_I2C_Master_Transmit(&i2c_master_handle, (pI2cArgStruct->I2cSlaveAddress) << 1, (uint8_t *)pSendBuffer, pI2cArgStruct->I2cTestSize, TRANSMIT_DEFAULT_MAX_DELAY);
			}
			else if(pI2cArgStruct -> I2cAddressMode == HAL_I2C_ADDRESS_10BITS)
			{
				ret = HAL_I2C_Master_Transmit(&i2c_master_handle, pI2cArgStruct->I2cSlaveAddress, (uint8_t *)pSendBuffer, pI2cArgStruct->I2cTestSize, TRANSMIT_DEFAULT_MAX_DELAY);
			}
			if(ret != HAL_OK)
			{
				CuFail(tc, "tramsmit test failed");
			}

            /*******主机阻塞接收*******/
            Debug_Print_Str(DEBUG_LEVEL_1, "\r\ni2c主机阻塞接收");

			WAIT_OK;
			HAL_Delay(20); //确保从机先进入发送API

			if(pI2cArgStruct -> I2cAddressMode == HAL_I2C_ADDRESS_7BITS)
			{
				ret = HAL_I2C_Master_Receive(&i2c_master_handle, (pI2cArgStruct->I2cSlaveAddress) << 1, (uint8_t *)pRecvBuffer, pI2cArgStruct->I2cTestSize, RECEIVE_DEFAULT_MAX_DELAY);
			}
			else if(pI2cArgStruct -> I2cAddressMode == HAL_I2C_ADDRESS_10BITS)
			{
				ret = HAL_I2C_Master_Receive(&i2c_master_handle, pI2cArgStruct->I2cSlaveAddress, (uint8_t *)pRecvBuffer, pI2cArgStruct->I2cTestSize, RECEIVE_DEFAULT_MAX_DELAY);
			}
			if(ret != HAL_OK)
			{
                CuFail(tc, "receive test failed");
			}
			Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pI2cArgStruct->I2cTestSize);
		}
		else
		{
			/*******主机非阻塞发送*******/
            Debug_Print_Str(DEBUG_LEVEL_1, "\r\ni2c主机非阻塞发送");
			Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pSendBuffer, pI2cArgStruct->I2cTestSize);

			HAL_Delay(5); //确保从机先等待OK
			SEND_OK;
			HAL_Delay(20); //确保从机先进入接收API

			if(pI2cArgStruct -> I2cAddressMode == HAL_I2C_ADDRESS_7BITS)
			{
				ret = HAL_I2C_Master_Transmit_IT(&i2c_master_handle, (pI2cArgStruct->I2cSlaveAddress) << 1, (uint8_t *)pSendBuffer, pI2cArgStruct->I2cTestSize);
			}
			else if(pI2cArgStruct -> I2cAddressMode == HAL_I2C_ADDRESS_10BITS)
			{
				ret = HAL_I2C_Master_Transmit_IT(&i2c_master_handle, pI2cArgStruct->I2cSlaveAddress, (uint8_t *)pSendBuffer, pI2cArgStruct->I2cTestSize);
			}
			if(ret != HAL_OK)
			{
				CuFail(tc, "tramsmit test failed");
			}
            while(i2c_master_tx_flag != 1){;}
			i2c_master_tx_flag = 0;

			/*******主机非阻塞接收*******/
            Debug_Print_Str(DEBUG_LEVEL_1, "\r\ni2c主机非阻塞接收");

			WAIT_OK;
			HAL_Delay(20); //确保从机先进入发送API

			if(pI2cArgStruct -> I2cAddressMode == HAL_I2C_ADDRESS_7BITS)
			{
				ret = HAL_I2C_Master_Receive_IT(&i2c_master_handle, (pI2cArgStruct->I2cSlaveAddress) << 1, (uint8_t *)pRecvBuffer, pI2cArgStruct->I2cTestSize);
			}
			else if(pI2cArgStruct -> I2cAddressMode == HAL_I2C_ADDRESS_10BITS)
			{
				ret = HAL_I2C_Master_Receive_IT(&i2c_master_handle, pI2cArgStruct->I2cSlaveAddress, (uint8_t *)pRecvBuffer, pI2cArgStruct->I2cTestSize);
			}
            if(ret != HAL_OK)
			{
				CuFail(tc, "receive test failed");
			}
            while(i2c_master_rx_flag != 1){;}
			i2c_master_rx_flag = 0;
			Debug_Print_Hex(DEBUG_LEVEL_2, (uint8_t *)pRecvBuffer, pI2cArgStruct->I2cTestSize);
		}

		HAL_Delay(5); //确保从机先等待OK
		SEND_OK;
		HAL_Delay(5); //确保从机先进入从机接收状态

		HAL_I2C_Drivertest_DeInit(&i2c_master_handle);

		Debug_Print_Str(DEBUG_LEVEL_2, "\r\n%dth sendlen:%d recvlen:%d\r\n", i + 1, strlen(pSendBuffer), strlen(pRecvBuffer));

		CuAssertStrEquals(tc, pSendBuffer, pRecvBuffer);

        xy_free(pSendBuffer);
        xy_free(pRecvBuffer);
	}
}

/******************************************************************************************/
void i2c_master_test(CuTest* tc)
{
	Debug_Print_Str(DEBUG_LEVEL_0, "\r\n<i2c> test begin\r\n");

	uint32_t CaseNum = 1;

	InitArgStruct(&I2cArgStruct_Demo, I2cSclPinArray, I2cSdaPinArray, I2cInstanceArray, I2cAddressModeArray, \
									   I2cClockSpeedArray, I2cSlaveAddressArray, I2cTestSizeArray, MaxArgArray);
	while(GetArgStruct(&I2cArgStruct_Demo) != 0)
	{
		Debug_Print_Str(DEBUG_LEVEL_1, "\r\n->i2c case:%lu/%lu\r\n", CaseNum, GetTotalCaseNun());

		CaseNum++;

		Debug_Print_ArgStruct(DEBUG_LEVEL_1, &I2cArgStruct_Demo);

		i2c_test(tc, &I2cArgStruct_Demo, TestTimes);
	}

	DeInitArgStruct(&I2cArgStruct_Demo);

	Debug_Print_Str(DEBUG_LEVEL_0, "\r\n<i2c> test end\r\n");
}

#endif /* #if (DRIVER_TEST == 1) */

