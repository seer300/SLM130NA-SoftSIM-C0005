/**
 * @file  csp_uart_poll_demo.c
 * @brief CSP_UART轮询收发Demo
 *        在这个Demo中,UART先进行阻塞接收92个字节的数据，阻塞的时间为500ms，如果在500ms内收到92个字节的数据则将接收到的数据进行回写，
 *        如果500ms未成功接收指定字节数则将实际收到的数据进行回写.
 * @note  除非是特别简单的应用，一般不会使用轮询模式。如果要使用轮询模式，一定要有个良好的程序架构或实现机制，避免程序无限挂起.
***********************************************************************************/
#include "hal_gpio.h"
#include "hal_csp.h"

//用户收发一次数据的长度，不得超过MAX_ONCE_DATA_LEN
#define USER_ONCE_DATA_LEN    92

//数据缓存大小，用户可自定义，最大65536字节
#define MAX_ONCE_DATA_LEN 128 // BYTES

uint8_t data[MAX_ONCE_DATA_LEN] = {0x00};

HAL_CSP_HandleTypeDef csp_uart_poll = {0};

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
	csp_uart_poll.Instance = HAL_CSP2;
	csp_uart_poll.CSP_UART_Init.BaudRate = 9600;
	csp_uart_poll.CSP_UART_Init.WordLength = HAL_CSP_UART_WORDLENGTH_8;
	csp_uart_poll.CSP_UART_Init.Parity = HAL_CSP_UART_PARITY_NONE;
	csp_uart_poll.CSP_UART_Init.StopBits = HAL_CSP_UART_STOPBITS_1;
	HAL_CSP_UART_Init(&csp_uart_poll);
}

/**
* @brief 在这个Demo中,CSP_UART先进行阻塞接收92个字节的数据，阻塞的时间为500ms，如果在500ms内收到92个字节的数据则将接收到的数据进行回写，
*        如果500ms未成功接收指定字节数则将实际收到的数据进行回写.
*/
__RAM_FUNC int main(void)
{
	SystemInit();

	CSP_UART_Init();

	while (1)
	{
        //阻塞接收92字节数据，超时时长为500ms.
		HAL_StatusTypeDef ret1 = HAL_CSP_Receive(&csp_uart_poll, data, USER_ONCE_DATA_LEN, 500);
		if (ret1 == HAL_OK)
		{
            //当成功接收到92字节数据后，调用下方API将数据进行回写，此处demo仅展示流程，用户可根据自身逻辑进行更改
			HAL_StatusTypeDef ret2 = HAL_CSP_Transmit(&csp_uart_poll, data, USER_ONCE_DATA_LEN, 500);
            if(ret2 == HAL_OK)
            {
                //如果有必要请用户添加业务代码
            }
            else
            {
                //异常类型请参考 hal_csp.h，常见的有外设被占用的HAL_BUSY, 传输超时的HAL_TIMEOUT, 和参数异常导致的HAL_ERROR,请根据情况处理
            }
		}
		else if (ret1 == HAL_TIMEOUT)
		{
			//如超时未成功接收指定字节数，可通过uart_poll.RxXferSize得到实际接收字节数，这里在实际接收字节不为0的情况下进行回写，用户可根据自身逻辑进行更改
			if (csp_uart_poll.RxXferCount > 0)
			{
				//用户进行数据处理，此处demo仅将接收到的数据回写
				HAL_CSP_Transmit(&csp_uart_poll, data, csp_uart_poll.RxXferCount, 500);
			}
		}
        else
        {
            //异常类型请参考 hal_csp.h，常见的有外设被占用的HAL_BUSY, 传输超时的HAL_TIMEOUT, 和参数异常导致的HAL_ERROR,请根据情况处理
        }
	}

	return 0;
}

