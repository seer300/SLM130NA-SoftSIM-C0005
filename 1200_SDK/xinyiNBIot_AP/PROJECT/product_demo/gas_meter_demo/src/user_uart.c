/*****************************************************************************************************************************	 
 * user_uart.c
 ****************************************************************************************************************************/

#include "user_debug.h"
#include "mcu_adapt.h"
#include "user_eeprom.h"
#include "user_uart.h"
#include "vmcu.h"
#include "type_adapt.h"

/*******************************************************************************************************/
/*******************************************************************************************************/
/****************************************  lpuart ******************************************************/
/*******************************************************************************************************/
/*******************************************************************************************************/

uint8_t rxbuff[UART_LEN_MAX] = {0};
uint8_t txbuff[UART_LEN_MAX] = {0};

McuUartIt_Typedef uart0 = {0};

__TYPE_IRQ_FUNC void Uart0RxIrqHandle(uint8_t rx_status)
{
    if(rx_status)
    {
        uint8_t data = 0;
        if (VmcuUartRead(FAC_UART, &data))//读取数据
        {
            if (uart0.rxcnt<UART_LEN_MAX-1)
                rxbuff[uart0.rxcnt++] = data;
        }
    }
    else
    {
        uart0.rxdone = true;
    }
}
__TYPE_IRQ_FUNC void Uart0TxIrqHandle(void)
{
    if(uart0.txcnt == uart0.txlen) //发送完成判断
    {
        McuUartTxDis(FAC_UART);
        uart0.txcnt = 0;
        uart0.txlen = 0;
    }
    else
    {
        McuUartWrite(FAC_UART, txbuff[uart0.txcnt++]);
    }
}


void UserFactUartInit(void)
{
    //lpuart初始化
    VmcuUartSet(FAC_UART,9600,8,2,0);
    //VmcuUartTxIrqReg(FAC_UART,Uart0TxIrqHandle);
    VmcuUartRxIrqReg(FAC_UART,Uart0RxIrqHandle);
    VmcuUartRxEn(FAC_UART);
}

/*******************************************************************************************************/
/*******************************************************************************************************/
/****************************************  csp2 ********************************************************/
/*******************************************************************************************************/
/*******************************************************************************************************/

uint8_t rxbuff2[UART_LEN_MAX] = {0};
uint8_t txbuff2[UART_LEN_MAX] = {0};

McuUartIt_Typedef uart2 = {0};

__RAM_FUNC void Uart2RxIrqHandle(uint8_t rx_status)
{
    if(rx_status)
    {
        uint8_t data = 0;
        if (VmcuUartRead(TEST_UART, &data))//读取数据
        {
            if (uart2.rxcnt<UART_LEN_MAX-1)
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
        McuUartTxDis(TEST_UART);
        uart2.txcnt = 0;
        uart2.txlen = 0;
    }
    else
    {
        McuUartWrite(TEST_UART, txbuff2[uart2.txcnt++]);
    }
}

void UserTestUartInit(void)
{
    //csp2 9600
    memset(&uart2, 0x0, sizeof(McuUartIt_Typedef));
    VmcuUartSet(TEST_UART,9600,8,1,0);
    //VmcuUartTxIrqReg(TEST_UART,Uart2TxIrqHandle);
    VmcuUartRxIrqReg(TEST_UART,Uart2RxIrqHandle);
    VmcuUartRxEn(TEST_UART);

    //红外测试串口打开，不允许进深睡
    LPM_LOCK(TEST_UART_DEEPSLEEP_LOCK);
}

void UserTestUartDis(void)
{
    McuUartRxDis(TEST_UART);
    McuUartTxDis(TEST_UART);

    //红外测试串口关闭，允许进深睡
    LPM_UNLOCK(TEST_UART_DEEPSLEEP_LOCK);
}

/*******************************************************************************************************/
/*******************************************************************************************************/
/**********************************************  UserUartFunc ******************************************/
/*******************************************************************************************************/
/*******************************************************************************************************/

void UserUartFunc(void)
{
    //lpuart产线调试串口测试
    if (uart0.rxdone)
    {
        //准备待发送数据
        uart0.txcnt = 0;
        uart0.txlen = uart0.rxcnt;
        memcpy(txbuff, rxbuff, uart0.txlen);

        //打开uart tx，发送第一个数据，发送数组下标加1
        McuUartTxEn(FAC_UART);
        //McuUartWrite(FAC_UART, txbuff[uart0.txcnt++]);
        VmcuUartWriteFram(FAC_UART,txbuff,uart0.txlen);

        //为下次接收数据做准备
        uart0.rxcnt = 0;
        uart0.rxdone = false;
        memset(rxbuff, 0, UART_LEN_MAX);
    }

    //csp2红外串口测试
    if (uart2.rxdone)
    {
        //准备待发送数据
        uart2.txcnt = 0;
        uart2.txlen = uart2.rxcnt;
        memcpy(txbuff2, rxbuff2, uart2.txlen);

        //打开uart tx，发送第一个数据，发送数组下标加1
        McuUartTxEn(TEST_UART);
        //McuUartWrite(TEST_UART, txbuff2[uart2.txcnt++]);
        VmcuUartWriteFram(TEST_UART,txbuff2,uart2.txlen);

        //为下次接收数据做准备
        uart2.rxcnt = 0;
        uart2.rxdone = false;
        memset(rxbuff2, 0, UART_LEN_MAX);
    }
}