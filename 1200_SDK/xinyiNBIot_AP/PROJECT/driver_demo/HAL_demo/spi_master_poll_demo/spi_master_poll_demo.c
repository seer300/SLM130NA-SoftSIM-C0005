/**
* @file		spi_master_poll_demo.c
* @brief    SPI主机工作模式，轮询收发Demo
* @note     在这个Demo中，MCU作为SPI主机，通过GPIO6、GPIO7分别选中CS0和CS1从机。
*           首先MCU选中CS0从设备读取200字节数据，然后MCU选中CS1从设备发送200字节数据并接收200字节，最后MCU选中CS0从设备发送200字节数据。
***********************************************************************************/
#include "hal_def.h"
#include "hal_gpio.h"
#include "hal_spi.h"

//用户收发一次数据的长度，不得超过MAX_ONCE_DATA_LEN
#define USER_ONCE_DATA_LEN    200

//数据缓存大小，用户可自定义，最大65536字节
#define MAX_ONCE_DATA_LEN     256     //BYTES

uint8_t rxdata[MAX_ONCE_DATA_LEN] = {0};
uint8_t txdata[MAX_ONCE_DATA_LEN] = {0};

HAL_SPI_HandleTypeDef spi_master = {0};

HAL_StatusTypeDef ret = HAL_OK;

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
    Debug_Runtime_Add("HAL_SPI_Init  start");
    HAL_SPI_Init(&spi_master);
    Debug_Runtime_Add("HAL_SPI_Init  stop");
}

/**
* @brief 在这个Demo中，MCU作为SPI主机，通过GPIO6、GPIO14分别选中CS0和CS1从机。
*        首先MCU选中CS0从设备读取200字节数据，然后MCU选中CS1从设备发送200字节数据并接收200字节，最后MCU选中CS0从设备发送200字节数据。
*/
__RAM_FUNC int main(void)
{
    SystemInit();

    SPI_Master_Init();

    while(1)
    {
	    //选中CS0从机，阻塞接收200字节数据，设置超时时间为500ms
        Debug_Runtime_Add("HAL_SPI_ResetCS  start");
        HAL_SPI_ResetCS(HAL_SPI_CS0);
        Debug_Runtime_Add("HAL_SPI_ResetCS  stop");
        ret = HAL_SPI_Receive(&spi_master, rxdata, USER_ONCE_DATA_LEN, 500);
        Debug_Runtime_Add("HAL_SPI_SetCS  start");
        HAL_SPI_SetCS(HAL_SPI_CS0);
        Debug_Runtime_Add("HAL_SPI_SetCS  stop");
        if(ret == HAL_OK)
        {
            //成功接收指定字节数
			//用户进行数据处理，此处demo仅将接收到的数据发送CS1选中从机，并从CS1选中从机
            //选中CS1从机，阻塞发送并接收200字节，设置超时时间为500ms
            HAL_SPI_ResetCS(HAL_SPI_CS1);
            ret = HAL_SPI_Master_TransmitReceive(&spi_master, txdata, rxdata, USER_ONCE_DATA_LEN, 500);
            HAL_SPI_SetCS(HAL_SPI_CS1);
            if(ret == HAL_OK)
            {
                //选中CS0从机，阻塞发送200字节数据，设置超时时间为500ms
                HAL_SPI_ResetCS(HAL_SPI_CS0);
                HAL_SPI_Transmit(&spi_master, rxdata, USER_ONCE_DATA_LEN, 500);
                HAL_SPI_SetCS(HAL_SPI_CS0);
            }
        }
        else if(ret == HAL_TIMEOUT)
        {
            //超时未成功接收指定字节数，spi_master.RxXferCount为实际接收字节数
            if(spi_master.RxXferCount > 0)
            {
                //用户进行数据处理，此处demo仅将接收到的数据回写到CS0选中从机
                HAL_SPI_ResetCS(HAL_SPI_CS0);
                HAL_SPI_Transmit(&spi_master, rxdata, spi_master.RxXferCount, 500);
                HAL_SPI_SetCS(HAL_SPI_CS0);
            }
        }
    }

    return 0;
}
