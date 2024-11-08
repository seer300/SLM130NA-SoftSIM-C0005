/**
* @file		csp_uart_IT_demo.c
* @brief	CSP_UART中断收发Demo
* @note     在这个Demo中，MCU的CSP_UART不开启硬件流控功能，首先通过中断接收数据，接收完成后置位标志g_csp_uart_rcved_end，
*           然后调用中断发送API将收到的数据进行回写，发送完成后置位标志g_csp_uart_sent_end.
***********************************************************************************/
#include "hal_gpio.h"
#include "hal_csp.h"

//外部MCU传输过程中最大的干扰时长，用户可根据产品特性自行修改
//单位MS，该参数是设定并使用CSP模块自身的硬件超时特性，最大超时时长为 1/baudrate*1000*65535，例如当波特率为9600时，最大超时时长约6826ms。
//该参数在CSP_UART非阻塞接收API中的作用：当用户指定长度的数据没有全部收完时，若出现一段数据接收间隔大于设定的超时时长后就会触发接收完成回调函数的调用。
#define MAX_INTERRUPT_TIME    5

//用户收发一次数据的长度，不得超过MAX_ONCE_DATA_LEN
#define USER_ONCE_DATA_LEN    512

//数据缓存大小，用户可自定义，最大65536字节
#define MAX_ONCE_DATA_LEN     512     //BYTES

uint8_t data[MAX_ONCE_DATA_LEN] = {0};

//指示此次数据接收是否已完成，当指定长度接收完全或接收超时时值该位
volatile uint8_t g_csp_uart_rcved_end = 0;

//指示此次数据发送是否已完成，当指定长度接收完全时，该全局标志置位
volatile uint8_t g_csp_uart_sent_end = 0;

HAL_CSP_HandleTypeDef csp_uart_IT = {0};

/**
 * @brief 错误中断回调函数，用户在使用时可自行添加中断处理函数.
 */
__RAM_FUNC void HAL_CSP2_ErrorCallback(HAL_CSP_HandleTypeDef *hcsp)
{
	hcsp->ErrorCode = HAL_CSP_ERROR_NONE;
}

/**
 * @brief 接收完成回调函数，用户在使用时可自行添加中断处理函数.
 */
__RAM_FUNC void HAL_CSP2_RxCpltCallback(HAL_CSP_HandleTypeDef *hcsp)
{
    //用户可在回调函数内部进行数据的计算、拼包或者其他处理流程。
	UNUSED_ARG(hcsp);
	g_csp_uart_rcved_end = 1;
}

/**
 * @brief 发送完成回调函数，用户在使用时可自行添加中断处理函数.
 */
__RAM_FUNC void HAL_CSP2_TxCpltCallback(HAL_CSP_HandleTypeDef *hcsp)
{
    UNUSED_ARG(hcsp);
	g_csp_uart_sent_end = 1;
}

/**
 * @brief CSP_UART初始化函数，主要完成以下配置：
 * 		  1. CSP_UART的TXD、RXD引脚的GPIO初始化配置
 * 		  2. CSP_UART的波特率、位宽、奇偶检验模式、停止位
 */
void CSP_UART_Init(void)
{
    //gpio初始化
    HAL_GPIO_InitTypeDef gpio_init = {0};

	gpio_init.Pin = GPIO_PAD_NUM_9;
	gpio_init.Mode = GPIO_MODE_HW_PER;
	gpio_init.PinRemap = GPIO_CSP2_TXD;
	HAL_GPIO_Init(&gpio_init);

	gpio_init.Pin = GPIO_PAD_NUM_10;
	gpio_init.Mode = GPIO_MODE_HW_PER;
	gpio_init.PinRemap = GPIO_CSP2_RXD;
	HAL_GPIO_Init(&gpio_init);

	//初始化CSP为UART
	csp_uart_IT.Instance = HAL_CSP2;
	csp_uart_IT.CSP_UART_Init.BaudRate = 9600;
	csp_uart_IT.CSP_UART_Init.WordLength = HAL_CSP_UART_WORDLENGTH_8;
	csp_uart_IT.CSP_UART_Init.Parity = HAL_CSP_UART_PARITY_NONE;
	csp_uart_IT.CSP_UART_Init.StopBits = HAL_CSP_UART_STOPBITS_1;
	Debug_Runtime_Add("HAL_CSP_UART_Init  start");
	HAL_CSP_UART_Init(&csp_uart_IT);
	Debug_Runtime_Add("HAL_SPI_SetCS  stop");
}

/**
* @brief 在这个Demo中，MCU的CSP_UART不开启硬件流控功能，首先通过中断接收数据，接收完成后置位标志g_csp_uart_rcved_end，
*        然后调用中断发送API将收到的数据进行回写，发送完成后置位标志g_csp_uart_sent_end.
*/
__RAM_FUNC int main(void)
{
	SystemInit();

	CSP_UART_Init();

	//开启CSP中断接收
    HAL_CSP_Receive_IT(&csp_uart_IT, data, USER_ONCE_DATA_LEN, MAX_INTERRUPT_TIME);

    while(1)
	{
		if(g_csp_uart_rcved_end == 1)
		{
			g_csp_uart_rcved_end = 0;

			//对接收到的数据进行处理，用户自行开发处理代码。当前DEMO为再将收到的数据发送给外部
			if(csp_uart_IT.RxXferCount > 0)
			{
				//用户进行数据处理，此处demo仅将接收到的数据回写
				HAL_StatusTypeDef ret = HAL_CSP_Transmit_IT(&csp_uart_IT, data, csp_uart_IT.RxXferCount);
				if(ret != HAL_OK)
				{
					//用户可以在这里增加发送错误的处理流程
				}
				else
				{
                    while(g_csp_uart_sent_end!=1);
                    g_csp_uart_sent_end = 0;
                    memset(data, 0, USER_ONCE_DATA_LEN); //清空接收缓冲区，用户可根据需要处理
                    HAL_CSP_Receive_IT(&csp_uart_IT, data, USER_ONCE_DATA_LEN, MAX_INTERRUPT_TIME);//继续下一次接收
                }
			}
		}
	}

	return 0;
}

