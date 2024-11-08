/**
* @file	  csp_spi_master_poll_demo.c
* @brief  CSP_SPI主机工作模式，轮询收发Demo
*         在这个Demo中，MCU作为SPI主机，通过GPIO6选中从机。
*         首先MCU选中从设备读取200字节数据，然后MCU选中从设备发送200字节数据并接收200字节，最后MCU选中从设备发送200字节数据。
***********************************************************************************/
#include "hal_gpio.h"
#include "hal_csp.h"

//用户收发一次数据的长度，不得超过MAX_ONCE_DATA_LEN
#define USER_ONCE_DATA_LEN    200

//数据缓存大小，用户可自定义，最大65536字节
#define MAX_ONCE_DATA_LEN     256     //BYTES

uint8_t rxdata[MAX_ONCE_DATA_LEN] = {0};

HAL_CSP_HandleTypeDef csp_spi_poll = {0};

HAL_StatusTypeDef ret = HAL_OK;

/**
 * @brief CSP_SPI初始化函数，主要完成以下配置：
 * 		  1. CSP_SPI的MISO、MOSI、SCLK、NSS引脚的GPIO初始化配置
 * 		  2. CSP_SPI的主从模式、工作模式、速率初始化配置
 */
void CSP_SPI_Init(void)
{
    //gpio初始化
    HAL_GPIO_InitTypeDef gpio_init = {0};

	gpio_init.Pin = GPIO_PAD_NUM_9;
	gpio_init.Mode = GPIO_MODE_HW_PER;
	gpio_init.PinRemap = GPIO_CSP2_TXD; //MOSI
	HAL_GPIO_Init(&gpio_init);

	gpio_init.Pin = GPIO_PAD_NUM_10;
	gpio_init.Mode = GPIO_MODE_HW_PER;
	gpio_init.PinRemap = GPIO_CSP2_RXD; //MISO
	HAL_GPIO_Init(&gpio_init);

	gpio_init.Pin = GPIO_PAD_NUM_5;
	gpio_init.Mode = GPIO_MODE_HW_PER;
	gpio_init.PinRemap = GPIO_CSP2_SCLK;//SCLK
	HAL_GPIO_Init(&gpio_init);

	gpio_init.Pin = GPIO_PAD_NUM_6;
	gpio_init.Mode = GPIO_MODE_HW_PER;
	gpio_init.PinRemap = GPIO_CSP2_TFS; //NSS
	HAL_GPIO_Init(&gpio_init);

	//初始化CSP为SPI
	csp_spi_poll.Instance = HAL_CSP2;
	csp_spi_poll.CSP_SPI_Init.MasterSlave = HAL_CSP_SPI_MODE_MASTER;
	csp_spi_poll.CSP_SPI_Init.WorkMode = HAL_CSP_SPI_WORKMODE_0;
    csp_spi_poll.CSP_SPI_Init.Speed = HAL_CSP_SPI_SPEED_1M;
	Debug_Runtime_Add("HAL_CSP_SPI_Init  start");
	HAL_CSP_SPI_Init(&csp_spi_poll);
	Debug_Runtime_Add("HAL_CSP_SPI_Init  stop");
}

/**
* @brief 在这个Demo中，MCU作为SPI主机，通过GPIO6选中从机.
*        首先MCU选中从设备读取200字节数据，然后MCU选中从设备发送200字节数据并接收200字节，最后MCU选中从设备发送200字节数据.
*/
__RAM_FUNC int main(void)
{
	SystemInit();

	CSP_SPI_Init();

	while (1)
	{
        //选中从机，阻塞接收200字节数据，设置超时时间为500ms
		Debug_Runtime_Add("HAL_CSP_SPI_ResetCS  start");
        HAL_CSP_SPI_ResetCS(&csp_spi_poll);
		Debug_Runtime_Add("HAL_CSP_SPI_ResetCS  stop");
        ret = HAL_CSP_Receive(&csp_spi_poll, rxdata, USER_ONCE_DATA_LEN, 500);
        if(ret == HAL_OK)
        {
            //成功接收指定字节数
			//用户进行数据处理，此处demo仅将接收到的数据发送从机
            //选中从机，阻塞发送并接收200字节，设置超时时间为500ms
            ret = HAL_CSP_SPI_TransmitReceive(&csp_spi_poll, rxdata, rxdata, USER_ONCE_DATA_LEN, 500);
            if(ret == HAL_OK)
            {
                //选中从机，阻塞发送200字节数据，设置超时时间为500ms
                HAL_CSP_Transmit(&csp_spi_poll, rxdata, USER_ONCE_DATA_LEN, 500);
            }
        }
        else if(ret == HAL_TIMEOUT)
        {
            //超时未成功接收指定字节数，csp_spi_poll.RxXferCount为实际接收字节数
            if(csp_spi_poll.RxXferCount > 0)
            {
                //用户进行数据处理，此处demo仅将接收到的数据回写到从机
                HAL_CSP_Transmit(&csp_spi_poll, rxdata, csp_spi_poll.RxXferCount, 500);
            }
        }
		Debug_Runtime_Add("HAL_CSP_SPI_SetCS  start");
        HAL_CSP_SPI_SetCS(&csp_spi_poll);
		Debug_Runtime_Add("HAL_CSP_SPI_SetCS  stop");
	}

	return 0;
}

