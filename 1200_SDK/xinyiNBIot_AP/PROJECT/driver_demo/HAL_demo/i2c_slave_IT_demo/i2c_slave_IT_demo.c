// /**
//  * @file    i2c_slave_IT_demo.c
//  * @brief   I2C从机机工作模式，中断收发Demo
//  * @note    在这个Demo中，MCU做为I2C从机，中断接收92字节数据，当全部数据接收完成后会置位标志g_i2c1_slave_rcved_end，
//  *          然后I2C从机将收到的数据进行中断发送。
// ***********************************************************************************/
// #include "hal_gpio.h"
// #include "hal_i2c.h"

// //用户收发一次数据的长度，不得超过MAX_ONCE_DATA_LEN
// #define USER_ONCE_DATA_LEN    92

// //数据缓存大小，用户可自定义，最大65536字节
// #define MAX_ONCE_DATA_LEN     128     //BYTES

// uint8_t rxdata[MAX_ONCE_DATA_LEN] = {0x00};

// HAL_I2C_HandleTypeDef i2c1_slave = {0};

// HAL_StatusTypeDef ret = HAL_OK;

// //指示此次数据接收是否已完成，当指定长度接收完全时置位
// volatile uint8_t g_i2c1_slave_rcved_end = 0;

// /**
//  * @brief 错误中断回调函数，用户在使用时可自行添加中断处理函数.
//  */
// __RAM_FUNC void HAL_I2C1_ErrorCallback(HAL_I2C_HandleTypeDef *hi2c)
// {
// 	//用户根据实际需求添加错误处理代码，此处默认为软重启I2C
// 	HAL_I2C_SoftReset(hi2c);
// 	//错误处理完成后恢复ErrorCode
// 	hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
// }

// /**
//  * @brief 中断回调函数，用户在使用时可自行添加中断处理函数.
//  */
// __RAM_FUNC void HAL_I2C1_RxCpltCallback(HAL_I2C_HandleTypeDef *hi2c)
// {
// 	//用户可在回调函数内部进行数据的计算、拼包或者其他处理流程.
// 	UNUSED_ARG(hi2c);
// 	g_i2c1_slave_rcved_end = 1;
// }

// /**
//  * @brief I2C从机初始化函数，主要完成以下配置：
//  * 		  1. I2C的SCL、SDA引脚的GPIO初始化配置
//  * 		  2. I2C的主从模式、速率、自身地址、寻址模式初始化配置
//  */
// void I2C_Slave_Init(void)
// {
// 	// i2c gpio初始化
// 	HAL_GPIO_InitTypeDef GPIO_InitStruct = {0};

// 	GPIO_InitStruct.Mode = GPIO_MODE_HW_PER;
// 	GPIO_InitStruct.PinRemap = GPIO_I2C1_SCL;
// 	GPIO_InitStruct.Pin = GPIO_PAD_NUM_6;
// 	HAL_GPIO_Init(&GPIO_InitStruct);

//     GPIO_InitStruct.Mode = GPIO_MODE_HW_PER;
// 	GPIO_InitStruct.PinRemap = GPIO_I2C1_SDA;
// 	GPIO_InitStruct.Pin = GPIO_PAD_NUM_5;
// 	HAL_GPIO_Init(&GPIO_InitStruct);

// 	// i2c初始化
// 	i2c1_slave.Instance = HAL_I2C1;
// 	i2c1_slave.Init.Mode = HAL_I2C_MODE_SLAVE;
// 	i2c1_slave.Init.ClockSpeed = HAL_I2C_SPEED_400K;
// 	i2c1_slave.Init.OwnAddress = 0x38; //作为从机时的自身地址
// 	i2c1_slave.Init.AddressingMode = HAL_I2C_ADDRESS_7BITS;
// 	HAL_I2C_Init(&i2c1_slave);
// }

// /**
//  * @brief   在这个Demo中，MCU做为I2C从机，中断接收92字节数据，当全部数据接收完成后会置位标志g_i2c1_slave_rcved_end，
//  *          然后I2C从机将收到的数据进行中断发送。
//  */
// __RAM_FUNC int main(void)
// {
// 	SystemInit();

// 	I2C_Slave_Init();

// 	//开启I2C接收中断
// 	HAL_I2C_Slave_Receive_IT(&i2c1_slave, rxdata, USER_ONCE_DATA_LEN);

// 	while (1)
// 	{
// 		if (g_i2c1_slave_rcved_end == 1)
// 		{
//             g_i2c1_slave_rcved_end = 0;

//             //对接收到的数据进行处理，用户自行开发处理代码。当前DEMO为再将收到的数据发送给外部
//             if(i2c1_slave.XferCount > 0)
//             {
//                 //用户进行数据处理，此处demo仅将接收到的数据回写
// #if !USER_PROCESS
// 			    ret = HAL_I2C_Slave_Transmit(&i2c1_slave, rxdata, i2c1_slave.XferCount, HAL_MAX_DELAY);
//                 if (ret != HAL_OK)
//                 {
//                     //用户可以在这里增加发送错误的处理流程
//                 }
//                 else
//                 {
//                     memset(rxdata, 0, USER_ONCE_DATA_LEN); //清空接收缓冲区
//                     //继续下一次接收
//                     HAL_I2C_Slave_Receive_IT(&i2c1_slave, rxdata, USER_ONCE_DATA_LEN);
//                 }
//             }
// #endif
// 		}
// 	}

// 	return 0;
// }
