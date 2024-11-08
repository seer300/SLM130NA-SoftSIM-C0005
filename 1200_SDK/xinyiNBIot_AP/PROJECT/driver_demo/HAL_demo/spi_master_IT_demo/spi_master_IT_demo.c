/**
* @file     spi_master_IT_demo.c
* @brief    SPI主机工作模式，中断收发Demo
* @note     在这个Demo中，MCU作为SPI主机，通过GPIO6、GPIO7分别选中CS0和CS1从机。
*           首先MCU选中CS0从机读取200字节数据，然后MCU选中CS1从机发送200字节数据。
***********************************************************************************/
#include "hal_def.h"
#include "hal_gpio.h"
#include "hal_spi.h"

//用户收发一次数据的长度，不得超过MAX_ONCE_DATA_LEN
#define USER_ONCE_DATA_LEN    200

//数据缓存大小，用户可自定义，最大65536字节
#define MAX_ONCE_DATA_LEN     256     //BYTES

uint8_t rxdata[MAX_ONCE_DATA_LEN] = {0};

HAL_SPI_HandleTypeDef spi_master = {0};

HAL_StatusTypeDef ret = HAL_OK;

//指示此次数据接收是否已完成，当指定长度接收完全时置位
volatile uint8_t g_spi_rcved_end = 0;

//指示此次数据发送是否已完成，当指定长度发送完全时置位
volatile uint8_t g_spi_sent_end = 0;

/**
 * @brief  SPI错误中断回调函数.在对应的CSP中断处理函数中调用.
 * @param  hspi. 详见结构体定义  @ref HAL_SPI_HandleTypeDef.
 */
__RAM_FUNC void HAL_SPI_ErrorCallback(HAL_SPI_HandleTypeDef *hspi)
{
	//用户可在回调函数内部进行数据的计算、拼包或者其他处理流程。
	UNUSED_ARG(hspi);
}

/**
 * @brief  SPI发送完成回调函数.在对应的SPI中断处理函数中调用.
 * @param  hspi. 详见结构体定义  @ref HAL_SPI_HandleTypeDef.
 */
__RAM_FUNC void HAL_SPI_TxCpltCallback(HAL_SPI_HandleTypeDef *hspi)
{
	//用户可在回调函数内部进行数据的计算、拼包或者其他处理流程.
	UNUSED_ARG(hspi);
	g_spi_sent_end = 1;
}

/**
 * @brief  SPI接收完成回调函数.在对应的SPI中断处理函数中调用.
 * @param  hspi. 详见结构体定义  @ref HAL_SPI_HandleTypeDef.
 */
__RAM_FUNC void HAL_SPI_RxCpltCallback(HAL_SPI_HandleTypeDef *hspi)
{
    //用户可在回调函数内部进行数据的计算、拼包或者其他处理流程.
	UNUSED_ARG(hspi);
	g_spi_rcved_end = 1;
}

/**
 * @brief SPI初始化函数，主要完成以下配置：
 * 		  1. SPI的MISO、MOSI、SCLK、NSS引脚的GPIO初始化配置
 * 		  2. SPI的主从模式、工作模式、时钟分频初始化配置
 */
void SPI_Master_Init(void)
{
    //spi gpio初始化
    HAL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PAD_NUM_9;
    GPIO_InitStruct.Mode = GPIO_MODE_HW_PER;
    GPIO_InitStruct.PinRemap = GPIO_SPI_MOSI;
    HAL_GPIO_Init(&GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PAD_NUM_10;
    GPIO_InitStruct.Mode = GPIO_MODE_HW_PER;
    GPIO_InitStruct.PinRemap = GPIO_SPI_MISO;
    HAL_GPIO_Init(&GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PAD_NUM_5;
    GPIO_InitStruct.Mode = GPIO_MODE_HW_PER;
    GPIO_InitStruct.PinRemap = GPIO_SPI_SCLK;
    HAL_GPIO_Init(&GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PAD_NUM_6;
    GPIO_InitStruct.Mode = GPIO_MODE_HW_PER;
    GPIO_InitStruct.PinRemap = GPIO_SPI_SS_N;
    HAL_GPIO_Init(&GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PAD_NUM_14;
    GPIO_InitStruct.Mode = GPIO_MODE_HW_PER;
    GPIO_InitStruct.PinRemap = GPIO_SPI_SS1_N;
    HAL_GPIO_Init(&GPIO_InitStruct);

    //spi 初始化
    spi_master.Instance = HAL_SPI;
    spi_master.Init.MasterSlave = HAL_SPI_MODE_MASTER;
    spi_master.Init.WorkMode = HAL_SPI_WORKMODE_0;
    spi_master.Init.Clock_Prescaler = HAL_SPI_CLKDIV_64;
    HAL_SPI_Init(&spi_master);
}

/**
 * @brief 在这个Demo中，MCU作为SPI主机，通过GPIO6、GPIO14分别选中CS0和CS1从机。
 *        首先MCU选中CS0从机读取200字节数据，然后MCU选中CS1从机发送200字节数据。
 */
__RAM_FUNC int main(void)
{
    SystemInit();

    SPI_Master_Init();

	//选中CS0从机，非阻塞接收200字节数据
    HAL_SPI_ResetCS(HAL_SPI_CS0);
    HAL_SPI_Receive_IT(&spi_master, rxdata, USER_ONCE_DATA_LEN);

    while(1)
    {
        if(g_spi_rcved_end == 1)
        {
            g_spi_rcved_end = 0;
            HAL_SPI_SetCS(HAL_SPI_CS0);

            //对接收到的数据进行处理，用户自行开发处理代码。
            if(spi_master.RxXferCount > 0)
            {
                //此处demo仅将接收到的数据回写到CS1选中从机
#if !USER_PROCESS
                //选中CS1从机，非阻塞发送200字节数据
                HAL_SPI_ResetCS(HAL_SPI_CS1);
                ret = HAL_SPI_Transmit_IT(&spi_master, rxdata, USER_ONCE_DATA_LEN);
				if(ret != HAL_OK)
				{
					//用户可以在这里增加发送错误的处理流程
				}
				else
				{
                    while(g_spi_sent_end != 1);
                    g_spi_sent_end = 0;
                    HAL_SPI_SetCS(HAL_SPI_CS1);
                    memset(rxdata, 0, USER_ONCE_DATA_LEN); //清空接收缓冲区

                    //再次选中CS0从机，非阻塞接收200字节数据
                    HAL_SPI_ResetCS(HAL_SPI_CS0);
                    HAL_SPI_Receive_IT(&spi_master, rxdata, USER_ONCE_DATA_LEN);
                }
#endif
            }
        }
    }

    return 0;
}
