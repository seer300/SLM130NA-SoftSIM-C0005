/***********************************************************************************
 * @Copyright (c)   :(C)2020, Qindao ieslab Co., Ltd
 * @FileName        :hc32_uart_driver.c
 * @Author          :
 * @Version         :V1.0
 * @Date            :2020-7-1
 * @Description :the function of the entity of system processor
 ************************************************************************************/

/* Includes ------------------------------------------------------------------*/
//#include "sys_config.h"
#include "U_uart_driver.h"
#include "U_timer1uS_driver.h"
//#include "U_ir_app.h"
//#include "reset.h"
#include "mcu_adapt.h"
/* Private variables ---------------------------------------------------------*/
static UART_STRUCT uart_type[COM_MAX_NUM];
u8 ir_tx_end_rdy_flag;				//ir tx发送预结束标志
#if COM0_OPEN
    static u8 s_uart0_rcv_buf[UART0_BUF_SIZE];  //串口0接收缓冲区
#endif

#if COM1_OPEN
    static u8 s_uart1_rcv_buf[UART1_BUF_SIZE];  //串口1接收缓冲区
#endif

#if COM2_OPEN
    static u8 s_uart2_rcv_buf[UART2_BUF_SIZE];  //串口2接收缓冲区
#endif

#if COM3_OPEN
    static u8 s_uart3_rcv_buf[UART3_BUF_SIZE];  //串口3接收缓冲区
#endif

#if LPCOM0_OPEN
    static u8 s_lpuart0_rcv_buf[LPUART0_BUF_SIZE];  //低功耗串口0接收缓冲区
#endif

#if LPCOM1_OPEN
    static u8 s_lpuart1_rcv_buf[LPUART1_BUF_SIZE];  //低功耗串口1接收缓冲区
#endif

/* Private function prototypes -----------------------------------------------*/
static void UartRcvMachine(void);        //串口接收前台状态机
void UartSendMachine(void);
static void UartPortCfg(u8 com);         //串口IO及中断控制器初始化
static void UartStartReceiveIT(u8 com);  //启动中断接收
static void UartStopReceiveIT(u8 com);   //停止中断接收
static void UartIRQHandler(u8 com, u8 recv_or_tran);      //串口中断处理函数
static void UartReceiveIT(u8 com);       //串口接收中断处理函数
static void UartTransmitIT(u8 com);      //串口发送中断处理函数
static void HAL_UartCfg(u8 com);

void Uart0_IRQTxHandler(void);
void Uart0_IRQHandler(uint8_t rx_status);
void Uart1_IRQHandler(uint8_t rx_status);
void Uart1_IRQTxHandler(void);
void Uart2_IRQHandler(uint8_t rx_status);
void Uart2_IRQTxHandler(void);
void LpUart0_IRQHandler(uint8_t rx_status);
void LpUart0_IRQTxHandler(void);

#define TASK_IDLE           (1)
#define TASK_BUSY           (0)


//void IR_Uart_38K_Config(void);
//void IR_Uart_38K_Enable(boolean_t enCmd);
/* Exported functions -------------------------------------------------------*/
/*************************************************
Function:u8 UartInit(u8 com,u8 baud,u8 databit,u8 stop,u8 check,u8 lpmode)
Description: 串口初始化
Input:  com 串口；baud：波特率；databit：数据位；stop：停止位；check：校验位;
lpmode：仅对5和lp有效选择是否作为低功耗串口使用（UART_LPUART_Disable，UART_LPUART_Enable）
return: SUCCESS：初始化成功；ERROR ：初始化失败
Others: 处于MainSpace，第一类接口：上电初始化接口。可供选择串口1、2、3、4、5、lp
*************************************************/
u8 UartInit(u8 com, u8 baud, u8 databit, u8 stop, u8 check, u8 lpmode)
{
//    switch (com)  //选择要配置的串口
//    {
//        #if COM0_OPEN
//        case (COM0):
//        {
//            uart_type[com].Instance = M0P_UART0;
//        }
//        break;
//        #endif
//
//        #if COM1_OPEN
//        case (COM1):
//        {
//            uart_type[com].Instance = M0P_UART1;
//        }
//        break;
//        #endif
//
//        #if COM2_OPEN
//        case (COM2):
//        {
//            uart_type[com].Instance = M0P_UART2;
//        }
//        break;
//        #endif
//
//        #if COM3_OPEN
//        case (COM3):
//        {
//            uart_type[com].Instance = M0P_UART3;
//        }
//        break;
//        #endif
//
//        #if LPCOM0_OPEN
//        case (LPCOM0):
//        {
//            uart_type[com].LP_Instance = M0P_LPUART0;
//        }
//        break;
//        #endif
//
//        #if LPCOM1_OPEN
//        case (LPCOM1):
//        {
//            uart_type[com].LP_Instance = M0P_LPUART1;
//        }
//        break;
//        #endif
//
//        default:
//        {
//            return ERROR;
//        }
//    }

    uart_type[com].uart_status = TASK_IDLE;

    switch (baud)  //设置波特率
    {
        case (BAUD1200):
        {
            uart_type[com].para.BaudRate = 1200;
        }
        break;

        case (BAUD2400):
        {
            uart_type[com].para.BaudRate = 2400;
        }
        break;

        case (BAUD4800):
        {
            uart_type[com].para.BaudRate = 4800;
        }
        break;

        case (BAUD9600):
        {
            uart_type[com].para.BaudRate = 9600;
        }
        break;

        case (BAUD19200):
        {
            uart_type[com].para.BaudRate = 19200;
        }
        break;

        case (BAUD38400):
        {
            uart_type[com].para.BaudRate = 38400;
        }
        break;

        case (BAUD57600):
        {
            uart_type[com].para.BaudRate = 57600;
        }
        break;

        case (BAUD115200):
        {
            uart_type[com].para.BaudRate = 115200;
        }
        break;

        default:
        {
            return ERROR;
        }
    }

    switch (databit)  //设置数据长度
    {
        case (DATA_BIT_8):
        {
            uart_type[com].para.WordLength = UART_WORDLEN_8B;
        }
        break;

        default:
        {
            return ERROR;
        }
    }

    switch (stop)  //设置停止位
    {
        case (STOPBIT_1):
        {
            uart_type[com].para.StopBits = UART_STOPBIT_1;
        }
        break;

        case (STOPBIT_2):
        {
            uart_type[com].para.StopBits = UART_STOPBIT_2;
        }
        break;

        case (STOPBIT_1_5):
        {
            uart_type[com].para.StopBits = UART_STOPBIT_1_5;
        }
        break;

        default:
        {
            return ERROR;
        }
    }

    switch (check)  //设置校验位
    {
        case (UART_NONE_PARITY):
        {
            uart_type[com].para.Parity = UART_NONE_PARITY;
//            uart_type[com].para.UARTMode = UartMskMode1;
        }
        break;

        case (UART_ODD_PARITY):
        {
            uart_type[com].para.Parity = UART_ODD_PARITY;
//            uart_type[com].para.UARTMode = UartMskMode3;
        }
        break;

        case (UART_EVEN_PARITY):
        {
            uart_type[com].para.Parity = UART_EVEN_PARITY;
//            uart_type[com].para.UARTMode = UartMskMode3;
        }
        break;

        default:
        {
            return ERROR;
        }
    }

//    uart_type[com].para.OverSampling = UartMsk8Or16Div;//UartMsk16Or32Div;  // 16位采样

//    if((LPCOM0 == com) || (LPCOM1 == com))
//    {
//        uart_type[com].para.LPModeFunction = lpmode;  //选择串口是否使用低功耗功能
//    }


    switch(com)
    {
    //LPUART，深睡保持串口，支持深睡唤醒，参考时钟不受启停CP影响。波特率范围：1200~921600
#if LPCOM0_OPEN
    case LPCOM0:
	{
		McuUartSet2(com, uart_type[com].para.BaudRate, uart_type[com].para.WordLength, uart_type[com].para.StopBits, uart_type[com].para.Parity, LPUART0_TX_PIN, LPUART0_RX_PIN);
		McuUartRxIrqReg(com, LpUart0_IRQHandler);
	    McuUartRxEn(com);
		break;
	}
#endif
        //UART，普通串口，深睡不保持，参考时钟不受启停CP影响。波特率范围：2400~921600
#if COM0_OPEN
        case COM0:
        {
        	McuUartSet2(com, uart_type[com].para.BaudRate, uart_type[com].para.WordLength, uart_type[com].para.StopBits, uart_type[com].para.Parity, UART0_TX_PIN, UART0_RX_PIN);
        	McuUartRxIrqReg(com, Uart0_IRQHandler);
            McuUartRxEn(com);
        	break;
        }
#endif
        //CSP2_UART，普通串口，深睡不保持，参考时钟受启停CP影响，需要重新设置。波特率范围：1200~921600
#if COM1_OPEN
        case COM1:
        {
        	McuUartSet2(com, uart_type[com].para.BaudRate, uart_type[com].para.WordLength, uart_type[com].para.StopBits, uart_type[com].para.Parity, UART1_TX_PIN, UART1_RX_PIN);
        	McuUartRxIrqReg(com, Uart1_IRQHandler);
            McuUartRxEn(com);
        	break;
        }
#endif
        //CSP3_UART，普通串口，深睡不保持，参考时钟受启停CP影响，需要重新设置。波特率范围：1200~921600
#if COM2_OPEN
        case COM2:
        {
        	McuUartSet2(com, uart_type[com].para.BaudRate, uart_type[com].para.WordLength, uart_type[com].para.StopBits, uart_type[com].para.Parity, UART2_TX_PIN, UART2_RX_PIN);
        	McuUartRxIrqReg(com, Uart2_IRQHandler);
            McuUartRxEn(com);
        	break;
        }
#endif

#if COM3_OPEN
        case COM3:

#endif

#if LPCOM1_OPEN
        case LPCOM1:

#endif

        default: return ERROR;
    }

    return SUCCESS;
}


void IrTxDealMachine(void)
{
    if(TRUE == ir_tx_end_rdy_flag)
    {
        if (0 == Check100msTimer(TIMER_100MS_IR_TX_38K_END)) //TX BUF 空
        {
            ir_tx_end_rdy_flag = FALSE;
//            IR_38KCarrier_Close();
        }
    }
}

/*************************************************
Function: void UartMachineDriver (void)
Description:串口前台处理机
Input:  None
Return: None
Others: 处于MainSpace，第二类接口：工作接口。
*************************************************/
void UartMachineDriver(void)
{
    UartRcvMachine();
    UartSendMachine();
    IrTxDealMachine();
}
/*************************************************
Function: u8 UartIfIdle (u8 com)
Description: 串口是否空闲
Input:  com：端口号
return: TASK_IDLE:空闲；TASK_BUSY:忙碌
Others: 处于MainSpace，第二类接口：工作接口。
*************************************************/
u8 UartIfIdle(u8 com)
{
    if(com < COM_MAX_NUM)
    {
        return uart_type[com].uart_status;
    }
    else
    {
        return TASK_IDLE;
    }
}

/*************************************************
Function:void UartSend (u8 com, u16 len, u8 *data,u8 *msg)
Description: 串口发生数据
Input:  com：端口号；len:数据长度；*data：要发送的内容；*msg：发送输出信息
return: TASK_IDLE:空闲；TASK_BUSY:忙碌
Others: 处于MainSpace，第二类接口：工作接口。
*************************************************/
void UartSend(u8 com, u16 len, u8 *data, u8 *out_msg)
{
    if(com < COM_MAX_NUM)
    {
        if((len > 0) && (len < UP_BUF_SIZE * 2)) //对发送的数据长度进行限制
        {
            uart_type[com].send_point = data;
            uart_type[com].send_len = len;
            uart_type[com].send_status_point = out_msg;
            *uart_type[com].send_status_point = TASK_BUSY;
            uart_type[com].uart_status = TASK_BUSY;
            uart_type[com].start_send_flag = TRUE;
            uart_type[com].send_over_time = uart_type[com].send_len / 10; //100ms至少发送10个字节

            if(uart_type[com].send_over_time < 30)
            {
                uart_type[com].send_over_time = 30;  //至少3秒
            }

			switch(com)
				{
					//LPUART，深睡保持串口，支持深睡唤醒，参考时钟不受启停CP影响。波特率范围：1200~921600
				#if LPCOM0_OPEN
					case LPCOM0:
					{
//						IR_38KCarrier_Open();
						ir_tx_end_rdy_flag = FALSE;
						McuUartRxIrqUnReg(com);  //发送时关闭接收，红外会互相干扰
						McuUartTxIrqClr(com);
						McuUartTxIrqReg(com, LpUart0_IRQTxHandler);///<开启发送中断
						McuUartTxEn(com);
		                McuUartWrite(com, *uart_type[com].send_point);
		                uart_type[com].send_point++;
		                uart_type[com].send_len--;
						break;
					}
				#endif
						//UART，普通串口，深睡不保持，参考时钟不受启停CP影响。波特率范围：2400~921600
				#if COM0_OPEN
					case COM0:
					{
						McuUartTxIrqReg(com, Uart0_IRQTxHandler);///<开启发送中断
						McuUartTxEn(com);
		                McuUartWrite(com, *uart_type[com].send_point);
		                uart_type[com].send_point++;
		                uart_type[com].send_len--;
						break;
					}
				#endif
						//CSP2_UART，普通串口，深睡不保持，参考时钟受启停CP影响，需要重新设置。波特率范围：1200~921600
				#if COM1_OPEN
					case COM1:
					{
						McuUartTxIrqReg(com, Uart1_IRQTxHandler);///<开启发送中断
						McuUartTxEn(com);
		                McuUartWrite(com, *uart_type[com].send_point);
		                uart_type[com].send_point++;
		                uart_type[com].send_len--;
						break;
					}
				#endif
						//CSP3_UART，普通串口，深睡不保持，参考时钟受启停CP影响，需要重新设置。波特率范围：1200~921600
				#if COM2_OPEN
					case COM2:
					{
						McuUartTxIrqReg(com, Uart2_IRQTxHandler);///<开启发送中断
						McuUartTxEn(com);
		                McuUartWrite(com, *uart_type[com].send_point);
		                uart_type[com].send_point++;
		                uart_type[com].send_len--;
						break;
					}
				#endif

				#if COM3_OPEN
					case COM3:

				#endif

				#if LPCOM1_OPEN
					case LPCOM1:

				#endif

						default: break;
			  }
         }
	}
}
/*************************************************
Function: u8 UartRcvFinish (u8 com)
Description: 串口接收完成
Input:  com:端口号
Return: None
Others: 处于MainSpace，第二类接口：工作接口。
*************************************************/
u8 UartRcvFinish(u8 com)
{
    if(com < COM_MAX_NUM)
    {
        if (TRUE == uart_type[com].rcv_msg)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}

/*************************************************
Function: void UartClearMsg (u8 com)
Description: 清除串口接收完成事件Msg,并使能接收
Input:  com 端口号
Return: None
Others: 处于MainSpace，第二类接口：工作接口。
*************************************************/
void UartClearMsg(u8 com)
{
    if(com < COM_MAX_NUM)
    {
        uart_type[com].rcv_msg = FALSE;
        uart_type[com].rcv_len = 0;
        UartStartReceiveIT(com);  //开启接收中断
    }
}
/*************************************************
Function: void UartRcv (u8 com ,u16 *out_len , u8 **out_data)
Description: 串口接收数据
Input:  com：端口号；out_len：数据长度；out_data:数据指针
Return: None
Others: 处于MainSpace，第二类接口：工作接口。
*************************************************/
void UartRcv(u8 com, u16 *out_len, u8 **out_data)
{
    if(com < COM_MAX_NUM)
    {
        *out_len = uart_type[com].rcv_len;

        switch (com)
        {
                #if COM0_OPEN

            case (COM0):
            {
                *out_data = s_uart0_rcv_buf;
            }
            break;
            #endif

            #if COM1_OPEN

            case (COM1):
            {
                *out_data = s_uart1_rcv_buf;
            }
            break;
            #endif

            #if COM2_OPEN

            case (COM2):
            {
                *out_data = s_uart2_rcv_buf;
            }
            break;
            #endif

            #if COM3_OPEN

            case (COM3):
            {
                *out_data = s_uart3_rcv_buf;
            }
            break;
            #endif

            #if LPCOM0_OPEN

            case (LPCOM0):
            {
                *out_data = s_lpuart0_rcv_buf;
            }
            break;
            #endif

            #if LPCOM1_OPEN

            case (LPCOM1):
            {
                *out_data = s_lpuart1_rcv_buf;
            }
            break;
            #endif

            default:
                break;
        }
    }
}
/*************************************************
Function: u8 UartIfSleep(u8 com)
Description: 串口是否允许休眠
Input:  None
Return: TASK_IDLE:TASK_BUSY
Others: 处于MainSpace，第三类接口：休眠前接口。
*************************************************/
u8 UartIfSleep(u8 com)
{
    if(com < COM_MAX_NUM)
    {
        return uart_type[com].uart_status;
    }
    else
    {
        return TASK_IDLE;
    }
}
/*************************************************
Function: void UartPreSleep(u8 com)
Description: 休眠前处理
Input:  com:端口号
Return: None
Others: 处于MainSpace，第三类接口：休眠前接口。
*************************************************/
void UartPreSleep(u8 com)
{
    switch (com)
    {
        #if COM0_OPEN
        case COM0:
        {
        	//配置UART时钟
        	PRCM_ClockEnable(CORE_CKG_CTL_UART2_EN);

            //关闭UART
            UARTDisable(UART2_BASE);

            //失能外设时钟
            PRCM_ClockDisable(CORE_CKG_CTL_UART2_EN);
        }
        break;
        #endif

        #if COM1_OPEN
        case COM1:
        {
        	//使能CSP时钟
        	PRCM_ClockEnable(CORE_CKG_CTL_CSP2_EN);

            //关闭CSP
            CSP_Disable(CSP2_BASE);

            //失能外设时钟
            PRCM_ClockDisable(CORE_CKG_CTL_CSP2_EN);
        }
        break;
        #endif

        #if COM2_OPEN
        case COM2:
        {
        	//使能CSP时钟
        	PRCM_ClockEnable(CORE_CKG_CTL_CSP3_EN);

            //关闭CSP
            CSP_Disable(CSP3_BASE);

            //失能外设时钟
            PRCM_ClockDisable(CORE_CKG_CTL_CSP3_EN);
        }
        break;
        #endif

        #if COM3_OPEN
        case COM3:
        {
        }
        break;
        #endif

        #if LPCOM0_OPEN
        case LPCOM0:
        {
//            #if LPCOM0_ENABLESLEEPMODE
//            uart_type[LPCOM1].LP_Instance->SCON_f.RCIE = 1;
//            uart_type[LPCOM1].LP_Instance->SCON_f.REN = 1;
//            #else
//            uart_type[LPCOM0].LP_Instance->SCON = 0;  //关闭所有功能
//            M0P_SYSCTRL->PERI_CLKEN0_f.LPUART0 = 0;   //关闭串口时钟
//            #endif

        	//配置LPUART时钟
        	PRCM_ClockEnable(CORE_CKG_CTL_LPUART_EN);
            //关闭LPUART
            UARTDisable(UART1_BASE);

            //失能外设时钟
        	PRCM_ClockDisable(CORE_CKG_CTL_LPUART_EN);
        }
        break;
        #endif

        #if LPCOM1_OPEN
        case LPCOM1:
        {
        }
        break;
        #endif

        default:
            break;
    }
}
/*************************************************
Function:void UartWakeSleep(u8 com,u8 baud,u8 databit,u8 stop,u8 check,lpmode)
Description: 串口唤醒
Input:  Input:com
串口；baud：波特率；databit：数据位；stop：停止位；check：校验位; return: None
Others: 处于MainSpace，第四类接口：唤醒后接口。
*************************************************/
void UartWakeSleep(u8 com, u8 baud, u8 databit, u8 stop, u8 check, u8 lpmode)
{
    UartInit(com, baud, databit, stop, check, lpmode);
}

/*************************************************
Function:void USART0_IRQHandler(void)
Description: 串口0中断入口函数
Input:  None
Return: None
Others: None
*************************************************/
#if COM0_OPEN
void Uart0_IRQHandler(uint8_t rx_status)
{
    (void)rx_status;
    UartIRQHandler(COM0, 0);
}

void Uart0_IRQTxHandler(void)
{
    UartIRQHandler(COM0, 1);
}
#endif

/*************************************************
Function:void USART1_IRQHandler(void)
Description: 串口1中断入口函数
Input:  None
Return: None
Others: None
*************************************************/
#if COM1_OPEN
void Uart1_IRQHandler(uint8_t rx_status)
{
    (void)rx_status;
    UartIRQHandler(COM1, 0);
}
void Uart1_IRQTxHandler(void)
{
    UartIRQHandler(COM1, 1);
}
#endif

/*************************************************
Function:void USART2_IRQHandler(void)
Description: 串口2中断入口函数
Input:  None
Return: None
Others: None
*************************************************/
#if COM2_OPEN
void Uart2_IRQHandler(uint8_t rx_status)
{
    (void)rx_status;
    UartIRQHandler(COM2, 0);
}
void Uart2_IRQTxHandler(void)
{
    UartIRQHandler(COM2, 1);
}
#endif

/*************************************************
Function:void UART3_IRQHandler(void)
Description: 串口3中断入口函数
Input:  None
Return: None
Others: None
*************************************************/
#if COM3_OPEN
void Uart3_IRQHandler(void)
{
    UartIRQHandler(COM3, 0);
}
void Uart3_IRQTxHandler(void)
{
    UartIRQHandler(COM3, 1);
}
#endif

/*************************************************
Function:void UART5_IRQHandler(void)
Description: 串口5中断入口函数
Input:  None
Return: None
Others: None
*************************************************/
#if LPCOM0_OPEN
void LpUart0_IRQHandler(uint8_t rx_status)
{
    (void)rx_status;
    UartIRQHandler(LPCOM0, 0);
}
void LpUart0_IRQTxHandler(void)
{
    UartIRQHandler(LPCOM0, 1);
}
#endif

/*************************************************
Function:void LPUART1_IRQHandler(void)
Description: 低功耗串口中断入口函数
Input:  None
Return: None
Others: None
*************************************************/
#if LPCOM1_OPEN
void LpUart1_IRQHandler(void)
{
    UartIRQHandler(LPCOM1, 0);
}
void LpUart1_IRQTxHandler(void)
{
    UartIRQHandler(LPCOM1, 1);
}
#endif

/*************************************************
Function:void UartRcvMachine(void)
Description: 串口接收前台状态机
Input:  None
return: None
Others: 内部接口
*************************************************/
static void UartRcvMachine(void)
{
    #if COM0_OPEN

    switch (uart_type[COM0].rcv_state)
    {
        case (UART_RCV_OVERTIME_S0):
        {
            if (1 == uart_type[COM0].rcv_flag)
            {
                uart_type[COM0].rcv_flag = 0;
                Set5msTimer(TIMER_5MS_UART0, FRAMEOVERTIME);
                uart_type[COM0].rcv_state = UART_RCV_OVERTIME_S1;
            }
        }
        break;

        case (UART_RCV_OVERTIME_S1):
        {
            if (1 == uart_type[COM0].rcv_flag)
            {
                uart_type[COM0].rcv_flag = 0;
                Set5msTimer(TIMER_5MS_UART0, FRAMEOVERTIME);
                uart_type[COM0].rcv_state = UART_RCV_OVERTIME_S1;
            }
            else if ((0 == uart_type[COM0].rcv_flag) && (0 == Check5msTimer(TIMER_5MS_UART0)))
            {
                uart_type[COM0].rcv_msg = TRUE;
                uart_type[COM0].rcv_state = UART_RCV_OVERTIME_S0;
                UartStopReceiveIT(COM0);
            }
        }
        break;

        default:
        {
            uart_type[COM0].rcv_state = UART_RCV_OVERTIME_S0;
        }
        break;
    }
    #endif

    #if COM1_OPEN
    switch (uart_type[COM1].rcv_state)
    {
        case (UART_RCV_OVERTIME_S0):
        {
            if (1 == uart_type[COM1].rcv_flag)
            {
                uart_type[COM1].rcv_flag = 0;
                Set5msTimer(TIMER_5MS_UART1, FRAMEOVERTIME);
                uart_type[COM1].rcv_state = UART_RCV_OVERTIME_S1;
            }
        }
        break;

        case (UART_RCV_OVERTIME_S1):
        {
            if (1 == uart_type[COM1].rcv_flag)
            {
                uart_type[COM1].rcv_flag = 0;
                Set5msTimer(TIMER_5MS_UART1, FRAMEOVERTIME);
                uart_type[COM1].rcv_state = UART_RCV_OVERTIME_S1;
            }
            else if ((0 == uart_type[COM1].rcv_flag) && (0 == Check5msTimer(TIMER_5MS_UART1)))
            {
                uart_type[COM1].rcv_msg = TRUE;
                uart_type[COM1].rcv_state = UART_RCV_OVERTIME_S0;
                UartStopReceiveIT(COM1);
            }
        }
        break;

        default:
        {
            uart_type[COM1].rcv_state = UART_RCV_OVERTIME_S0;
        }
        break;
    }
    #endif

    #if COM2_OPEN
    switch (uart_type[COM2].rcv_state)
    {
        case (UART_RCV_OVERTIME_S0):
        {
            if (1 == uart_type[COM2].rcv_flag)
            {
                uart_type[COM2].rcv_flag = 0;
                Set5msTimer(TIMER_5MS_UART2, FRAMEOVERTIME);
                uart_type[COM2].rcv_state = UART_RCV_OVERTIME_S1;
            }
        }
        break;

        case (UART_RCV_OVERTIME_S1):
        {
            if (1 == uart_type[COM2].rcv_flag)
            {
                uart_type[COM2].rcv_flag = 0;
                Set5msTimer(TIMER_5MS_UART2, FRAMEOVERTIME);
                uart_type[COM2].rcv_state = UART_RCV_OVERTIME_S1;
            }
            else if ((0 == uart_type[COM2].rcv_flag) && (0 == Check5msTimer(TIMER_5MS_UART2)))
            {
                uart_type[COM2].rcv_msg = TRUE;
                uart_type[COM2].rcv_state = UART_RCV_OVERTIME_S0;
                UartStopReceiveIT(COM2);
            }
        }
        break;

        default:
        {
            uart_type[COM2].rcv_state = UART_RCV_OVERTIME_S0;
        }
        break;
    }
    #endif

    #if COM3_OPEN
    switch (uart_type[COM3].rcv_state)
    {
        case (UART_RCV_OVERTIME_S0):
        {
            if (1 == uart_type[COM3].rcv_flag)
            {
                uart_type[COM3].rcv_flag = 0;
                Set5msTimer(TIMER_5MS_UART3, FRAMEOVERTIME);
                uart_type[COM3].rcv_state = UART_RCV_OVERTIME_S1;
            }
        }
        break;

        case (UART_RCV_OVERTIME_S1):
        {
            if (1 == uart_type[COM3].rcv_flag)
            {
                uart_type[COM3].rcv_flag = 0;
                Set5msTimer(TIMER_5MS_UART3, FRAMEOVERTIME);
                uart_type[COM3].rcv_state = UART_RCV_OVERTIME_S1;
            }
            else if ((0 == uart_type[COM3].rcv_flag) && (0 == Check5msTimer(TIMER_5MS_UART3)))
            {
                uart_type[COM3].rcv_msg = TRUE;
                uart_type[COM3].rcv_state = UART_RCV_OVERTIME_S0;
                UartStopReceiveIT(COM3);
            }
        }
        break;

        default:
        {
            uart_type[COM3].rcv_state = UART_RCV_OVERTIME_S0;
        }
        break;
    }
    #endif

    #if LPCOM0_OPEN
    switch (uart_type[LPCOM0].rcv_state)
    {
        case (UART_RCV_OVERTIME_S0):
        {
            if (1 == uart_type[LPCOM0].rcv_flag)
            {
                uart_type[LPCOM0].rcv_flag = 0;
                Set5msTimer(TIMER_5MS_LPUART0, FRAMEOVERTIME);
                uart_type[LPCOM0].rcv_state = UART_RCV_OVERTIME_S1;
            }
        }
        break;

        case (UART_RCV_OVERTIME_S1):
        {
            if (1 == uart_type[LPCOM0].rcv_flag)
            {
                uart_type[LPCOM0].rcv_flag = 0;
                Set5msTimer(TIMER_5MS_LPUART0, FRAMEOVERTIME);
                uart_type[LPCOM0].rcv_state = UART_RCV_OVERTIME_S1;
            }
            else if ((0 == uart_type[LPCOM0].rcv_flag) && (0 == Check5msTimer(TIMER_5MS_LPUART0)))
            {
                uart_type[LPCOM0].rcv_msg = TRUE;
                uart_type[LPCOM0].rcv_state = UART_RCV_OVERTIME_S0;
                UartStopReceiveIT(LPCOM0);
            }
        }
        break;

        default:
        {
            uart_type[LPCOM0].rcv_state = UART_RCV_OVERTIME_S0;
        }
        break;
    }
    #endif

    #if LPCOM1_OPEN
    #if MODULE_BLE_USED
    switch (uart_type[LPCOM1].rcv_state)
    {
        case (UART_RCV_OVERTIME_S0):
        {
            if (1 == BleRcvFrameDataFlag())//if (1 == uart_type[LPCOM1].rcv_flag)
            {
                BleClrRcvFrameDataFlag();//uart_type[LPCOM1].rcv_flag = 0;
                Set5msTimer(TIMER_5MS_LPUART1, FRAMEOVERTIME);
                uart_type[LPCOM1].rcv_state = UART_RCV_OVERTIME_S1;
            }
        }
        break;

        case (UART_RCV_OVERTIME_S1):
        {
            if (1 == BleRcvFrameDataFlag())//if (1 == uart_type[LPCOM1].rcv_flag)
            {
                BleClrRcvFrameDataFlag();//uart_type[LPCOM1].rcv_flag = 0;
                Set5msTimer(TIMER_5MS_LPUART1, FRAMEOVERTIME);
                uart_type[LPCOM1].rcv_state = UART_RCV_OVERTIME_S1;
            }
            else if ((0 == BleRcvFrameDataFlag()) && (0 == Check5msTimer(TIMER_5MS_LPUART1))) //else if ((0 == uart_type[LPCOM1].rcv_flag) && (0 == Check5msTimer(TIMER_5MS_LPUART1)))
            {
                uart_type[LPCOM1].rcv_msg = TRUE;
                uart_type[LPCOM1].rcv_state = UART_RCV_OVERTIME_S0;
                UartStopReceiveIT(LPCOM1);
            }
        }
        break;

        default:
        {
            uart_type[LPCOM1].rcv_state = UART_RCV_OVERTIME_S0;
        }
        break;
    }

    #else

    switch (uart_type[LPCOM1].rcv_state)
    {
        case (UART_RCV_OVERTIME_S0):
        {
            if (1 == uart_type[LPCOM1].rcv_flag)
            {
                uart_type[LPCOM1].rcv_flag = 0;
                Set5msTimer(TIMER_5MS_LPUART1, FRAMEOVERTIME);
                uart_type[LPCOM1].rcv_state = UART_RCV_OVERTIME_S1;
            }
        }
        break;

        case (UART_RCV_OVERTIME_S1):
        {
            if (1 == uart_type[LPCOM1].rcv_flag)
            {
                uart_type[LPCOM1].rcv_flag = 0;
                Set5msTimer(TIMER_5MS_LPUART1, FRAMEOVERTIME);
                uart_type[LPCOM1].rcv_state = UART_RCV_OVERTIME_S1;
            }
            else if ((0 == uart_type[LPCOM1].rcv_flag) && (0 == Check5msTimer(TIMER_5MS_LPUART1)))
            {
                uart_type[LPCOM1].rcv_msg = TRUE;
                uart_type[LPCOM1].rcv_state = UART_RCV_OVERTIME_S0;
                UartStopReceiveIT(LPCOM1);
            }
        }
        break;

        default:
        {
            uart_type[LPCOM1].rcv_state = UART_RCV_OVERTIME_S0;
        }
        break;
    }

    #endif
    #endif
}
//判串口接收空闲
u8 UartRcvMachineIfIdle(u8 com)
{
    switch(com)
    {
        #if COM0_OPEN
        case(COM0):
        {
            if(UART_RCV_OVERTIME_S0 == uart_type[COM0].rcv_state)
            {
                return TASK_IDLE;
            }
            else
            {
                return TASK_BUSY;
            }
        }
//		break;
        #endif

        #if COM1_OPEN
        case(COM1):
        {
            if(UART_RCV_OVERTIME_S0 == uart_type[COM1].rcv_state)
            {
                return TASK_IDLE;
            }
            else
            {
                return TASK_BUSY;
            }
        }
//		break;
        #endif

        #if COM2_OPEN
        case(COM2):
        {
            if(UART_RCV_OVERTIME_S0 == uart_type[COM2].rcv_state)
            {
                return TASK_IDLE;
            }
            else
            {
                return TASK_BUSY;
            }
        }
//		break;
        #endif

        #if COM3_OPEN
        case(COM3):
        {
            if(UART_RCV_OVERTIME_S0 == uart_type[COM3].rcv_state)
            {
                return TASK_IDLE;
            }
            else
            {
                return TASK_BUSY;
            }
        }
//		break;
        #endif

        #if LPCOM0_OPEN
        case(LPCOM0):
        {
            if(UART_RCV_OVERTIME_S0 == uart_type[LPCOM0].rcv_state)
            {
                return TASK_IDLE;
            }
            else
            {
                return TASK_BUSY;
            }
        }
//		break;
        #endif

        #if LPCOM1_OPEN

        case(LPCOM1):
        {
            if(UART_RCV_OVERTIME_S0 == uart_type[LPCOM1].rcv_state)
            {
                return TASK_IDLE;
            }
            else
            {
                return TASK_BUSY;
            }
        }
//		break;
        #endif

        default:
            break;
    }

    return TASK_IDLE;
}
/*************************************************
Function:void UartSendMachine(void)
Description: 串口发送前台状态机
Input:  None
return: None
Others: 内部接口
*************************************************/
void UartSendMachine(void)
{
    #if COM0_OPEN

    if(TASK_IDLE == uart_type[COM0].uart_status)
    {
        Set100msTimer(TIMER_100MS_UART0_BUSY, UART_BUSY_OVERTIME);
    }
    else if(0 == Check100msTimer(TIMER_100MS_UART0_BUSY))
    {
        uart_type[COM0].uart_status = TASK_IDLE;//超时，强制复位
        UartPreSleep(COM0);
        uart_type[COM0].finish_send_flag = FALSE;
        UartInit(COM0, BAUD2400, DATA_BIT_8, STOPBIT_1, UART_NONE_PARITY, UART_LPUART_Disable);
        uart_type[COM0].send_state = UART_SEND_OVERTIME_S0;
    }

    switch (uart_type[COM0].send_state)
    {
        case (UART_SEND_OVERTIME_S0):
        {
            if (TRUE == uart_type[COM0].start_send_flag)
            {
                uart_type[COM0].start_send_flag = FALSE;
                Set100msTimer(TIMER_100MS_UART0_SEND, uart_type[COM0].send_over_time);
                uart_type[COM0].send_state = UART_SEND_OVERTIME_S1;
            }
        }
        break;

        case (UART_SEND_OVERTIME_S1):
        {
            if (TRUE == uart_type[COM0].finish_send_flag)
            {
                uart_type[COM0].finish_send_flag = FALSE;
                uart_type[COM0].send_state = UART_SEND_OVERTIME_S0;
            }
            else if (0 == Check100msTimer(TIMER_100MS_UART0_SEND))
            {
                uart_type[COM0].uart_status = TASK_IDLE;//发送超时，强制复位
                UartPreSleep(COM0);
                uart_type[COM0].finish_send_flag = FALSE;
                UartInit(COM0, BAUD2400, DATA_BIT_8, STOPBIT_1, UART_NONE_PARITY, UART_LPUART_Disable);
                uart_type[COM0].send_state = UART_SEND_OVERTIME_S0;
            }
        }
        break;

        default:
        {
            uart_type[COM0].send_state = UART_SEND_OVERTIME_S0;
        }
        break;
    }

    #endif
    #if COM1_OPEN

    if(TASK_IDLE == uart_type[COM1].uart_status)
    {
        Set100msTimer(TIMER_100MS_UART1_BUSY, UART_BUSY_OVERTIME);
    }
    else if(0 == Check100msTimer(TIMER_100MS_UART1_BUSY))
    {
        uart_type[COM1].uart_status = TASK_IDLE;//超时，强制复位
        UartPreSleep(COM1);
        uart_type[COM1].finish_send_flag = FALSE;
        UartInit(NB_UART_COM, BAUD9600, DATA_BIT_8, STOPBIT_1, UART_NONE_PARITY, UART_LPUART_Disable);
        uart_type[COM1].send_state = UART_SEND_OVERTIME_S0;
    }

    switch (uart_type[COM1].send_state)
    {
        case (UART_SEND_OVERTIME_S0):
        {
            if (TRUE == uart_type[COM1].start_send_flag)
            {
                uart_type[COM1].start_send_flag = FALSE;
                Set100msTimer(TIMER_100MS_UART1_SEND, uart_type[COM1].send_over_time);
                uart_type[COM1].send_state = UART_SEND_OVERTIME_S1;
            }
        }
        break;

        case (UART_SEND_OVERTIME_S1):
        {
            if (TRUE == uart_type[COM1].finish_send_flag)
            {
                uart_type[COM1].finish_send_flag = FALSE;
                uart_type[COM1].send_state = UART_SEND_OVERTIME_S0;
            }
            else if (0 == Check100msTimer(TIMER_100MS_UART1_SEND))
            {
                uart_type[COM1].uart_status = TASK_IDLE;//发送超时，强制复位
                UartPreSleep(COM1);
                uart_type[COM1].finish_send_flag = FALSE;
                UartInit(NB_UART_COM, BAUD9600, DATA_BIT_8, STOPBIT_1, UART_NONE_PARITY, UART_LPUART_Disable);
                uart_type[COM1].send_state = UART_SEND_OVERTIME_S0;
            }
        }
        break;

        default:
        {
            uart_type[COM1].send_state = UART_SEND_OVERTIME_S0;
        }
        break;
    }

    #endif
    #if COM2_OPEN

    if(TASK_IDLE == uart_type[COM2].uart_status)
    {
        Set100msTimer(TIMER_100MS_UART2_BUSY, UART_BUSY_OVERTIME);
    }
    else if(0 == Check100msTimer(TIMER_100MS_UART2_BUSY))
    {
        uart_type[COM2].uart_status = TASK_IDLE;//超时，强制复位
        UartPreSleep(COM2);
        uart_type[COM2].finish_send_flag = FALSE;
        UartInit(COM2, BAUD2400, DATA_BIT_8, STOPBIT_1, UART_NONE_PARITY, UART_LPUART_Disable);
        uart_type[COM2].send_state = UART_SEND_OVERTIME_S0;
    }

    switch (uart_type[COM2].send_state)
    {
        case (UART_SEND_OVERTIME_S0):
        {
            if (TRUE == uart_type[COM2].start_send_flag)
            {
                uart_type[COM2].start_send_flag = FALSE;
                Set100msTimer(TIMER_100MS_UART2_SEND, uart_type[COM2].send_over_time);
                uart_type[COM2].send_state = UART_SEND_OVERTIME_S1;
            }
        }
        break;

        case (UART_SEND_OVERTIME_S1):
        {
            if (TRUE == uart_type[COM2].finish_send_flag)
            {
                uart_type[COM2].finish_send_flag = FALSE;
                uart_type[COM2].send_state = UART_SEND_OVERTIME_S0;
            }
            else if (0 == Check100msTimer(TIMER_100MS_UART2_SEND))
            {
                uart_type[COM2].uart_status = TASK_IDLE;//发送超时，强制复位
                UartPreSleep(COM2);
                uart_type[COM2].finish_send_flag = FALSE;
                UartInit(COM2, BAUD2400, DATA_BIT_8, STOPBIT_1, UART_NONE_PARITY, UART_LPUART_Disable);
                uart_type[COM2].send_state = UART_SEND_OVERTIME_S0;
            }
        }
        break;

        default:
        {
            uart_type[COM2].send_state = UART_SEND_OVERTIME_S0;
        }
        break;
    }

    #endif
    #if COM3_OPEN

    if(TASK_IDLE == uart_type[COM3].uart_status)
    {
        Set100msTimer(TIMER_100MS_UART3_BUSY, UART_BUSY_OVERTIME);
    }
    else if(0 == Check100msTimer(TIMER_100MS_UART3_BUSY))
    {
        uart_type[COM3].uart_status = TASK_IDLE;//超时，强制复位
        Reset_RstPeripheral1(ResetMskUart3);
        uart_type[COM3].finish_send_flag = FALSE;
        UartInit(COM3, BAUD2400, DATA_BIT_8, STOPBIT_1, UART_NONE_PARITY, UART_LPUART_Disable);
        uart_type[COM3].send_state = UART_SEND_OVERTIME_S0;
    }

    switch (uart_type[COM3].send_state)
    {
        case (UART_SEND_OVERTIME_S0):
        {
            if (TRUE == uart_type[COM3].start_send_flag)
            {
                uart_type[COM3].start_send_flag = FALSE;
                Set100msTimer(TIMER_100MS_UART3_SEND, uart_type[COM3].send_over_time);
                uart_type[COM3].send_state = UART_SEND_OVERTIME_S1;
            }
        }
        break;

        case (UART_SEND_OVERTIME_S1):
        {
            if (TRUE == uart_type[COM3].finish_send_flag)
            {
                uart_type[COM3].finish_send_flag = FALSE;
                uart_type[COM3].send_state = UART_SEND_OVERTIME_S0;
            }
            else if (0 == Check100msTimer(TIMER_100MS_UART3_SEND))
            {
                uart_type[COM3].uart_status = TASK_IDLE;//发送超时，强制复位
                Reset_RstPeripheral1(ResetMskUart3);
                uart_type[COM3].finish_send_flag = FALSE;
                UartInit(COM3, BAUD2400, DATA_BIT_8, STOPBIT_1, UART_NONE_PARITY, UART_LPUART_Disable);
                uart_type[COM3].send_state = UART_SEND_OVERTIME_S0;
            }
        }
        break;

        default:
        {
            uart_type[COM3].send_state = UART_SEND_OVERTIME_S0;
        }
        break;
    }

    #endif
    #if LPCOM0_OPEN

    if(TASK_IDLE == uart_type[LPCOM0].uart_status)
    {
        Set100msTimer(TIMER_100MS_LPUART0_BUSY, UART_BUSY_OVERTIME);
    }
    else if(0 == Check100msTimer(TIMER_100MS_LPUART0_BUSY))
    {
        uart_type[LPCOM0].uart_status = TASK_IDLE;//超时，强制复位
        UartPreSleep(LPCOM0);
        uart_type[LPCOM0].finish_send_flag = FALSE;
        UartInit(IR_UART_COM, BAUD2400, DATA_BIT_8, STOPBIT_1, UART_EVEN_PARITY, UART_LPUART_Disable);
        uart_type[LPCOM0].send_state = UART_SEND_OVERTIME_S0;
    }

    switch (uart_type[LPCOM0].send_state)
    {
        case (UART_SEND_OVERTIME_S0):
        {
            if (TRUE == uart_type[LPCOM0].start_send_flag)
            {
                uart_type[LPCOM0].start_send_flag = FALSE;
                Set100msTimer(TIMER_100MS_LPUART0_SEND, uart_type[LPCOM0].send_over_time);
                uart_type[LPCOM0].send_state = UART_SEND_OVERTIME_S1;
            }
        }
        break;

        case (UART_SEND_OVERTIME_S1):
        {
            if (TRUE == uart_type[LPCOM0].finish_send_flag)
            {
                uart_type[LPCOM0].finish_send_flag = FALSE;
                uart_type[LPCOM0].send_state = UART_SEND_OVERTIME_S0;
            }
            else if (0 == Check100msTimer(TIMER_100MS_LPUART0_SEND))
            {
                uart_type[LPCOM0].uart_status = TASK_IDLE;//发送超时，强制复位
                UartPreSleep(LPCOM0);
                uart_type[LPCOM0].finish_send_flag = FALSE;
                UartInit(IR_UART_COM, BAUD2400, DATA_BIT_8, STOPBIT_1, UART_EVEN_PARITY, UART_LPUART_Disable);
                uart_type[LPCOM0].send_state = UART_SEND_OVERTIME_S0;
            }
        }
        break;

        default:
        {
            uart_type[LPCOM0].send_state = UART_SEND_OVERTIME_S0;
        }
        break;
    }

    #endif
    #if LPCOM1_OPEN

    if(TASK_IDLE == uart_type[LPCOM1].uart_status)
    {
        Set100msTimer(TIMER_100MS_LPUART1_BUSY, UART_BUSY_OVERTIME);
    }
    else if(0 == Check100msTimer(TIMER_100MS_LPUART1_BUSY))
    {
        uart_type[LPCOM1].uart_status = TASK_IDLE;//超时，强制复位
        Reset_RstPeripheral0(ResetMskLpUart1);
        uart_type[LPCOM1].finish_send_flag = FALSE;
        #if MODULE_BLE_USED
        UartInit(BLE_UART_COM, BAUD38400, DATA_BIT_8, STOPBIT_1, UART_EVEN_PARITY, UART_LPUART_Disable);
        #else //NB
        UartInit(NB_UART_COM, BAUD9600, DATA_BIT_8, STOPBIT_1, UART_NONE_PARITY, UART_LPUART_Disable);
        #endif
        uart_type[LPCOM1].send_state = UART_SEND_OVERTIME_S0;
    }

    switch (uart_type[LPCOM1].send_state)
    {
        case (UART_SEND_OVERTIME_S0):
        {
            if (TRUE == uart_type[LPCOM1].start_send_flag)
            {
                uart_type[LPCOM1].start_send_flag = FALSE;
                Set100msTimer(TIMER_100MS_LPUART1_SEND, uart_type[LPCOM1].send_over_time);
                uart_type[LPCOM1].send_state = UART_SEND_OVERTIME_S1;
            }
        }
        break;

        case (UART_SEND_OVERTIME_S1):
        {
            if (TRUE == uart_type[LPCOM1].finish_send_flag)
            {
                uart_type[LPCOM1].finish_send_flag = FALSE;
                uart_type[LPCOM1].send_state = UART_SEND_OVERTIME_S0;
            }
            else if (0 == Check100msTimer(TIMER_100MS_LPUART1_SEND))
            {
                uart_type[LPCOM1].uart_status = TASK_IDLE;//发送超时，强制复位
                Reset_RstPeripheral0(ResetMskLpUart1);
                uart_type[LPCOM1].finish_send_flag = FALSE;
                #if MODULE_BLE_USED
                UartInit(BLE_UART_COM, BAUD38400, DATA_BIT_8, STOPBIT_1, UART_EVEN_PARITY, UART_LPUART_Disable);
                #else //NB
                UartInit(NB_UART_COM, BAUD9600, DATA_BIT_8, STOPBIT_1, UART_NONE_PARITY, UART_LPUART_Disable);
                #endif
                uart_type[LPCOM1].send_state = UART_SEND_OVERTIME_S0;
            }
        }
        break;

        default:
        {
            uart_type[LPCOM1].send_state = UART_SEND_OVERTIME_S0;
        }
        break;
    }

    #endif
}
//判串口发送空闲
u8 UartSendMachineIfIdle(u8 com)
{
    switch(com)
    {
        #if COM0_OPEN
        case(COM0):
        {
            if(UART_RCV_OVERTIME_S0 == uart_type[COM0].send_state)
            {
                return TASK_IDLE;
            }
            else
            {
                return TASK_BUSY;
            }
        }
        #endif

        #if COM1_OPEN
        case(COM1):
        {
            if(UART_RCV_OVERTIME_S0 == uart_type[COM1].send_state)
            {
                return TASK_IDLE;
            }
            else
            {
                return TASK_BUSY;
            }
        }
        #endif

        #if COM2_OPEN
        case(COM2):
        {
            if(UART_RCV_OVERTIME_S0 == uart_type[COM2].send_state)
            {
                return TASK_IDLE;
            }
            else
            {
                return TASK_BUSY;
            }
        }
        #endif

        #if COM3_OPEN
        case(COM3):
        {
            if(UART_RCV_OVERTIME_S0 == uart_type[COM3].send_state)
            {
                return TASK_IDLE;
            }
            else
            {
                return TASK_BUSY;
            }
        }

        #endif

        #if LPCOM0_OPEN
        case(LPCOM0):
        {
            if(UART_RCV_OVERTIME_S0 == uart_type[LPCOM0].send_state)
            {
                return TASK_IDLE;
            }
            else
            {
                return TASK_BUSY;
            }
        }
        #endif

        #if LPCOM1_OPEN
        case(LPCOM1):
        {
            if(UART_RCV_OVERTIME_S0 == uart_type[LPCOM1].send_state)
            {
                return TASK_IDLE;
            }
            else
            {
                return TASK_BUSY;
            }
        }
        #endif

        default:
            break;
    }

    return TASK_IDLE;
}
/*************************************************
Function:void UartStartReceiveIT(u8 com)
Description: 开启串口接收中断
Input:  com:端口号
Return: void
Others: 内部接口
*************************************************/
static void UartStartReceiveIT(u8 com)
{
    switch(com)
    {
    //LPUART，深睡保持串口，支持深睡唤醒，参考时钟不受启停CP影响。波特率范围：1200~921600
#if LPCOM0_OPEN
    case LPCOM0:
	{
		McuUartRxIrqReg(com, LpUart0_IRQHandler);
		break;
	}
#endif
        //UART，普通串口，深睡不保持，参考时钟不受启停CP影响。波特率范围：2400~921600
#if COM0_OPEN
        case COM0:
        {
        	McuUartRxIrqReg(com, Uart0_IRQHandler);
        	break;
        }
#endif
        //CSP2_UART，普通串口，深睡不保持，参考时钟受启停CP影响，需要重新设置。波特率范围：1200~921600
#if COM1_OPEN
        case COM1:
        {
        	McuUartRxIrqReg(com, Uart1_IRQHandler);
        	break;
        }
#endif
        //CSP3_UART，普通串口，深睡不保持，参考时钟受启停CP影响，需要重新设置。波特率范围：1200~921600
#if COM2_OPEN
        case COM2:
        {
        	McuUartRxIrqReg(com, Uart2_IRQHandler);
        	break;
        }
#endif

#if COM3_OPEN
        case COM3:

#endif

#if LPCOM1_OPEN
        case LPCOM1:

#endif

        default:  ;
    }
}
/*************************************************
Function:void UartStopReceiveIT(u8 com)
Description: 停止串口接收中断
Input:  com:端口号
Return: void
Others: 内部接口
*************************************************/
static void UartStopReceiveIT(u8 com)
{
	McuUartRxIrqUnReg(com);//关闭接收中断
}

/*************************************************
Function:void UartPortCfg(u8 com)
Description: 串口IO及中断控制器初始化
Input:  com,端口号
Return: void
Others: 内部接口
*************************************************/
static void UartPortCfg(u8 com)
{

}

/*************************************************
Function: u8 HAL_GetUart_Hander(u8 com,M0P_UART_TypeDef* UARTx)
Description: 通过串口号获取串口的句柄
Input:  com：串口号
Return: void
Others: 内部接口
*************************************************/
M0P_UART_TypeDef *HAL_GetUart_Hander(u8 com)
{
    M0P_UART_TypeDef *Addr;

    return ((M0P_UART_TypeDef *)Addr);
}

M0P_LPUART_TypeDef *HAL_GetLPUart_Hander(u8 com)
{
    M0P_LPUART_TypeDef *Addr;


    return ((M0P_LPUART_TypeDef *)Addr);
}
/*************************************************
Function: void HAL_UartCfg(u8 com)
Description: 串口初始化
Input:  com：串口号
Return: void
Others: 内部接口
*************************************************/
static void HAL_UartCfg(u8 com)
{

}
/*************************************************
Function:void UartTransmitIT(u8 com)
Description: 串口发送数据
Input:  com：端口号；
return: void
Others: 内部接口
*************************************************/
static void UartTransmitIT(u8 com)
{

	if (uart_type[com].send_len > 0)
	{
		McuUartWrite(com, *uart_type[com].send_point);
		uart_type[com].send_point++;
		uart_type[com].send_len--;
	}

	else if (0 == uart_type[com].send_len)
	{

		switch(com)
		{
			//LPUART，深睡保持串口，支持深睡唤醒，参考时钟不受启停CP影响。波特率范围：1200~921600
		#if LPCOM0_OPEN
			case LPCOM0:
			{
				McuUartTxIrqUnReg(com);///<禁止发送中断
				McuUartRxIrqReg(com, LpUart0_IRQHandler);///<红外发送完成，开启接收中断

				ir_tx_end_rdy_flag = TRUE;
				Set100msTimer(TIMER_100MS_IR_TX_38K_END, 10);
				break;
			}
		#endif
				//UART，普通串口，深睡不保持，参考时钟不受启停CP影响。波特率范围：2400~921600
		#if COM0_OPEN
			case COM0:
			{
				McuUartTxIrqUnReg(com);///<禁止发送中断
				break;
			}
		#endif
				//CSP2_UART，普通串口，深睡不保持，参考时钟受启停CP影响，需要重新设置。波特率范围：1200~921600
		#if COM1_OPEN
			case COM1:
			{
				McuUartTxIrqUnReg(com);///<禁止发送中断
				break;
			}
		#endif
				//CSP3_UART，普通串口，深睡不保持，参考时钟受启停CP影响，需要重新设置。波特率范围：1200~921600
		#if COM2_OPEN
			case COM2:
			{
				McuUartTxIrqUnReg(com);///<禁止发送中断
				break;
			}
		#endif

		#if COM3_OPEN
			case COM3:

		#endif

		#if LPCOM1_OPEN
			case LPCOM1:

		#endif

				default: break;
		}

		*uart_type[com].send_status_point = TASK_IDLE;
		uart_type[com].uart_status = TASK_IDLE;
		uart_type[com].finish_send_flag = TRUE;
	}
}

/*************************************************
Function:void UartReceiveIT(u8 com)
Description: 串口接收中断
Input:  com:端口号
Return: void
Others: 内部接口
*************************************************/
static void UartReceiveIT(u8 com)
{
    volatile u8 temp = 0;

    if (uart_type[com].rcv_len < UP_BUF_SIZE)
    {
        uart_type[com].rcv_flag = 1;  //接收到数据

        switch(com)
        {
            #if COM0_OPEN
            case(COM0):
            {
                McuUartRead(com, &s_uart0_rcv_buf[uart_type[com].rcv_len]);///读取数据
            }
            break;
            #endif

            #if COM1_OPEN
            case(COM1):
            {
//                NBUartReceiveIT();					//NB uart接口，单独处理
//                uart_type[com].rcv_len = 0;
//                //s_uart1_rcv_buf[uart_type[com].rcv_len] = Uart_ReceiveData(M0P_UART1);///读取数据

                McuUartRead(com, &s_uart1_rcv_buf[uart_type[com].rcv_len]);///读取数据
            }
            break;
            #endif

            #if COM2_OPEN
            case(COM2):
            {
                McuUartRead(com, &s_uart2_rcv_buf[uart_type[com].rcv_len]);///读取数据
            }
            break;
            #endif

            #if COM3_OPEN
            case(COM3):
            {
            }
            break;
            #endif

            #if LPCOM0_OPEN
            case (LPCOM0):
            {
                McuUartRead(com, &s_lpuart0_rcv_buf[uart_type[com].rcv_len]);///读取数据
            }
            break;
            #endif

            #if LPCOM1_OPEN
            case (LPCOM1):
            {
            }
            break;
            #endif
        }

        uart_type[com].rcv_len++;
    }
    else
    {
        McuUartRead(com, &temp);///读取数据

        uart_type[com].rcv_len = 0;
    }
}
/*************************************************
Function:void UartIRQHandler(u8 com)
Description: 中断处理函数
Input:  com:端口号
Return: None
Others: 处于中断程序空间；内部接口
*************************************************/
static void UartIRQHandler(u8 com, u8 recv_or_tran)
{
    if(recv_or_tran == 0)
    {
    	UartReceiveIT(com);
    }
	else if(recv_or_tran == 1)
	{
		UartTransmitIT(com);
	}
}





