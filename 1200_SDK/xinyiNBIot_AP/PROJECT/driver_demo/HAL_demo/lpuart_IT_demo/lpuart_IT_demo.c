/**
* @file		lpuart_IT_demo.c
* @brief    LPUART中断收发Demo
* @note     在这个Demo中，MCU的LPUART不开启硬件流控功能，首先通过中断接收数据，接收完成后置位标志g_lpuart_rcved_end，
*           然后调用中断发送API将收到的数据进行回写，发送完成后置位标志g_lpuart_sent_end.
***********************************************************************************/
#include "hal_lpuart.h"

//外部MCU传输过程中最大的干扰时长，用户可根据产品特性自行修改
//单位MS，该参数是设定并使用LPUART模块自身的硬件超时特性，最大超时时长为 1/baudrate*1000*31，例如当波特率为9600时，最大超时时长约3ms。
//该参数在UART非阻塞接收API中的作用：当用户指定长度的数据没有全部收完时，若出现一段数据接收间隔大于设定的超时时长后就会触发接收完成回调函数的调用。
#define MAX_INTERRUPT_TIME 3

//用户收发一次数据的长度，不得超过MAX_ONCE_DATA_LEN
#define USER_ONCE_DATA_LEN    256

//数据缓存大小，用户可自定义，最大65536字节
#define MAX_ONCE_DATA_LEN 256 // BYTES

uint8_t data[MAX_ONCE_DATA_LEN] = {0x00};

//指示此次数据接收是否已完成，当指定长度接收完全或接收超时，该全局标志置位
volatile uint8_t g_lpuart_rcved_end = 0;

//指示此次数据发送是否已完成，当指定长度接收完全时，该全局标志置位
volatile uint8_t g_lpuart_sent_end = 0;

HAL_LPUART_HandleTypeDef lpuart_IT = {0};

/**
 * @brief 错误中断回调函数，用户在使用时可自行添加中断处理函数.
 */

__RAM_FUNC void HAL_LPUART_ErrorCallback(HAL_LPUART_HandleTypeDef* hlpuart)
{
	hlpuart->ErrorCode = LPUART_ERROR_NONE;
}

/**
 * @brief 接收完成回调函数，用户在使用时可自行添加中断处理函数.
 */
__RAM_FUNC void HAL_LPUART_RxCpltCallback(HAL_LPUART_HandleTypeDef *hlpuart)
{
    //用户可在回调函数内部进行数据的计算、拼包或者其他处理流程。
    UNUSED_ARG(hlpuart);
	g_lpuart_rcved_end = 1;
}

/**
 * @brief 发送完成回调函数，用户在使用时可自行添加中断处理函数.
 */
__RAM_FUNC void HAL_LPUART_TxCpltCallback(HAL_LPUART_HandleTypeDef *hlpuart)
{
	UNUSED_ARG(hlpuart);
	g_lpuart_sent_end = 1;
}

/**
 * @brief LPUART初始化函数，主要完成以下配置：
 * 		  1. LPUART的TXD、RXD、CTS、RTS引脚选择 @ref HAL_LPUART_GPIOStateTypeDef
 * 		  2. LPUART的波特率、位宽、奇偶检验模式、硬件流控的配置
 */
void LPUART_Init(void)
{
	//LPUART初始化
	lpuart_IT.Instance        = LPUART;
	lpuart_IT.Init.BaudRate   = 9600;
	lpuart_IT.Init.WordLength = LPUART_WORDLENGTH_8;
	lpuart_IT.Init.Parity     = LPUART_PARITY_NONE;
    lpuart_IT.Init.PadSel     = LPUART_PADSEL_RXD_GPIO4; //GPIO3/4分别为LPUART的TXD/RXD，硬件固定配置，不可用户重配
    Debug_Runtime_Add("HAL_LPUART_Init  start");
    HAL_LPUART_Init(&lpuart_IT);
    Debug_Runtime_Add("HAL_LPUART_Init  stop");
}

/**
 * @note 在这个Demo中，MCU的LPUART不开启硬件流控功能，首先通过中断接收数据，接收完成后置位标志g_lpuart_rcved_end，
 *       然后调用中断发送API将收到的数据进行回写，发送完成后置位标志g_lpuart_sent_end.
 */
__RAM_FUNC int main(void)
{
	SystemInit();

	LPUART_Init();

	//开启LPUART的中断接收
	HAL_LPUART_Receive_IT(&lpuart_IT, data, USER_ONCE_DATA_LEN, MAX_INTERRUPT_TIME);

	while(1)
	{
        if(g_lpuart_rcved_end == 1)
        {
            g_lpuart_rcved_end = 0;

            //对接收到的数据进行处理，用户自行开发处理代码。当前DEMO为再将收到的数据发送给外部
            if(lpuart_IT.RxXferCount > 0)
            {
                //用户进行数据处理，此处demo仅将接收到的数据回写
#if !USER_PROCESS
                HAL_StatusTypeDef ret = HAL_LPUART_Transmit_IT(&lpuart_IT, data, lpuart_IT.RxXferCount);
                if(ret != HAL_OK)
                {
                    //用户可以在这里增加发送错误的处理流程
                }
                else
                {
                    while(g_lpuart_sent_end!=1);
                    g_lpuart_sent_end = 0;
                    memset(data, 0, USER_ONCE_DATA_LEN); //清空接收缓冲区，用户可根据需要处理
                    HAL_LPUART_Receive_IT(&lpuart_IT, data, USER_ONCE_DATA_LEN, MAX_INTERRUPT_TIME); //继续下一次接收
                }
#endif
            }
        }
    }

    return 0;
}
