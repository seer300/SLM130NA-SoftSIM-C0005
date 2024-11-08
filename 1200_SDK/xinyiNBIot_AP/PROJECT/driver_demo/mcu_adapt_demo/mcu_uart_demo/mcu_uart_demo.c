#include "mcu_adapt.h"
#include "xy_system.h"
#include "system.h"


#define UART_LEN_MAX (512)  //uart最大收发长度，单位字节

uint8_t rxbuff0[UART_LEN_MAX] = {0};
uint8_t txbuff0[UART_LEN_MAX] = {0};

uint8_t rxbuff1[UART_LEN_MAX] = {0};
uint8_t txbuff1[UART_LEN_MAX] = {0};

uint8_t rxbuff2[UART_LEN_MAX] = {0};
uint8_t txbuff2[UART_LEN_MAX] = {0};

uint8_t rxbuff3[UART_LEN_MAX] = {0};
uint8_t txbuff3[UART_LEN_MAX] = {0};

struct 
{
    volatile uint16_t rxcnt;
    volatile bool rxdone;
    volatile uint16_t txlen;
    volatile uint16_t txcnt;
    volatile bool txdone;
} uart0 = {0}, uart1 = {0}, uart2 = {0}, uart3 = {0};

/********************************LPUART中断回调函数*************************************/
/**
 * @brief 串口接收回调函数
 * @param rx_status 0：有接收数据产生，1：RX数据线空闲
 */
__RAM_FUNC void Uart0RxIrqHandle(uint8_t rx_status)
{
    if(rx_status == 0)
    {
        uint8_t data = 0;
        if(McuUartRead(0, &data)) //读取数据
        {
            rxbuff0[uart0.rxcnt++] = data;
        }
    }
    else
    {
        uart0.rxdone = true;
    }
}

/**********************************UART中断回调函数*************************************/
/**
 * @brief 串口接收回调函数
 * @param rx_status 0：有接收数据产生，1：RX数据线空闲
 */
__RAM_FUNC void Uart1RxIrqHandle(uint8_t rx_status)
{
    if(rx_status == 0)
    {
        uint8_t data = 0;
        if(McuUartRead(1, &data)) //读取数据
        {
            rxbuff1[uart1.rxcnt++] = data;
        }
    }
    else
    {
        uart1.rxdone = true;
    }
}

/********************************CSP2_UART中断回调函数**********************************/
/**
 * @brief 串口接收回调函数
 * @param rx_status 0：有接收数据产生，1：RX数据线空闲
 */
__RAM_FUNC void Uart2RxIrqHandle(uint8_t rx_status)
{
    if(rx_status == 0)
    {
        uint8_t data = 0;
        if(McuUartRead(2, &data)) //读取数据
        {
            rxbuff2[uart2.rxcnt++] = data;
        }
    }
    else
    {
        uart2.rxdone = true;
    }
}

__RAM_FUNC void Uart2TxIrqHandle(void)
{
    if(uart2.txcnt == uart2.txlen) //发送完成判断
    {
        McuUartTxDis(2);
        uart2.txcnt = 0;
        uart2.txlen = 0;
    }
    else
    {
        McuUartWrite(2, txbuff2[uart2.txcnt++]);
    }
}

/********************************CSP2_UART中断回调函数***********************************/
/**
 * @brief 串口接收回调函数
 * @param rx_status 0：有接收数据产生，1：RX数据线空闲
 */
__RAM_FUNC void Uart3RxIrqHandle(uint8_t rx_status)
{
    if(rx_status == 0)
    {
        uint8_t data = 0;
        if(McuUartRead(3, &data)) //读取数据
        {
            rxbuff3[uart3.rxcnt++] = data;
        }
    }
    else
    {
        uart3.rxdone = true;
    }
}

__RAM_FUNC void Uart3TxIrqHandle(void)
{
    if(uart3.txcnt == uart3.txlen) //发送完成判断
    {
        McuUartTxDis(3);
        uart3.txcnt = 0;
        uart3.txlen = 0;
    }
    else
    {
        McuUartWrite(3, txbuff3[uart3.txcnt++]);
    }
}

/***************************************主函数******************************************/
volatile bool dsleep_fastwakeup_status = false;
__RAM_FUNC int main(void)
{
    SystemInit();

    McuUartSet(0, 9600, 8, 2, 0, 3, 4);
    McuUartRxIrqReg(0, Uart0RxIrqHandle);
    McuUartRxEn(0);
    McuUartTxEn(0);

    if(Get_Boot_Reason() != WAKEUP_DSLEEP) 
    {
        McuUartWriteFram(0, (uint8_t *)"\r\nPOWERON or RESET\r\n", strlen("\r\nPOWERON or RESET\r\n"));
    }

    McuUartSet(1, 115200, 8, 2, 0, 18, 19);
    McuUartRxIrqReg(1, Uart1RxIrqHandle);
    McuUartRxEn(1);
    McuUartTxEn(1);

    McuUartSet(2, 115200, 8, 1, 0, 14, 20);
    McuUartRxIrqReg(2, Uart2RxIrqHandle);
    McuUartTxIrqReg(2, Uart2TxIrqHandle);
    McuUartRxEn(2);

    McuUartSet(3, 115200, 8, 1, 0, 22, 23);
    McuUartRxIrqReg(3, Uart3RxIrqHandle);
    McuUartTxIrqReg(3, Uart3TxIrqHandle);
    McuUartRxEn(3);

    while (1)
    {
        //接收完成回显数据
        if (uart0.rxdone)
        {
            //准备待发送数据
            uart0.txcnt = 0;
            uart0.txlen = uart0.rxcnt;
            memcpy(txbuff0, rxbuff0, uart0.txlen);

            //为下次接收数据做准备
            uart0.rxcnt = 0;
            uart0.rxdone = false;

            //打开uart tx，发送数据
            McuUartWriteFram(0, txbuff0, uart0.txlen);
            uart0.txdone = true;
        }

        //回显完成进入深睡
        if(uart0.txdone)
        {
            uart0.txdone = false;
            //dsleep_fastwakeup_status = Enter_LowPower_Mode(LPM_DSLEEP);

            //XY_SOC_VER为2、4、5时深睡唤醒才会执行快速恢复，使用mcu_uart接口且num入参为0时，请务必保证AT_LPUART宏为0。
            // if(dsleep_fastwakeup_status && (Get_Boot_Reason() == WAKEUP_DSLEEP))
            // {
            // McuUartWriteFram(0, "\r\nFastRecovery Dsleep Wkup\r\n", strlen("\r\nFastRecovery Dsleep Wkup\r\n"));
            // }
        }

        if (uart1.rxdone)
        {
            //准备待发送数据
            uart1.txcnt = 0;
            uart1.txlen = uart1.rxcnt;
            memcpy(txbuff1, rxbuff1, uart1.txlen);

            //为下次接收数据做准备
            uart1.rxcnt = 0;
            uart1.rxdone = false;

            //打开uart tx，发送数据
            McuUartWriteFram(1, txbuff1, uart1.txlen);
        }

        if (uart2.rxdone)
        {
            //准备待发送数据
            uart2.txcnt = 0;
            uart2.txlen = uart2.rxcnt;
            memcpy(txbuff2, rxbuff2, uart2.txlen);

            //为下次接收数据做准备
            uart2.rxcnt = 0;
            uart2.rxdone = false;

            //打开uart tx，发送第一个数据，发送数组下标加1
            McuUartTxEn(2);
            McuUartWrite(2, txbuff2[uart2.txcnt++]);
        }

        if (uart3.rxdone)
        {
            //准备待发送数据
            uart3.txcnt = 0;
            uart3.txlen = uart3.rxcnt;
            memcpy(txbuff3, rxbuff3, uart3.txlen);

            //为下次接收数据做准备
            uart3.rxcnt = 0;
            uart3.rxdone = false;

            //打开uart tx，发送第一个数据，发送数组下标加1
            McuUartTxEn(3);
            McuUartWrite(3, txbuff3[uart3.txcnt++]);
        }
    }
}

