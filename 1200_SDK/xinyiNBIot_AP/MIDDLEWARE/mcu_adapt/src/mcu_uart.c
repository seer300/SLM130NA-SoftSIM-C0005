#include "mcu_adapt.h"

#define MCU_UART_NUM (4)
mcu_uart_t MCU_UART[MCU_UART_NUM] = {{0,NULL,NULL}};
uint32_t g_mcu_uart_rate[MCU_UART_NUM] = {0}; // 获取波特率速率，关闭UART_TX时需要使用

static void Uart0IrqHandle(void);
static void Uart1IrqHandle(void);
static void Uart2IrqHandle(void);
static void Uart3IrqHandle(void);

//=====================================================================================
//================================uart=================================================
//=====================================================================================
/***********************************UART注意点*****************************************
    num等于0、1对应LPUART、UART2，发送时停止位固定为2bit，接收时停止位兼容1、1.5、2bit
    num等于2、3对应CSP2、CSP3发送接收时的停止位可配1或2bit
**************************************************************************************/
/************************************************************************************
* @brief  带引脚配置的串口设置. 接口耗时：num为0耗时1084.7us，num为1耗时751.8us，num为2耗时1158.9us，num为3耗时1159.2us
* @param  num 串口号，可选0-3，0:LPUART、1:UART、2:CSP2_UART、3:CSP3_UART
* @param  baud_rate 波特率
* @param  data_len  6：6位数据位  7：7位数据位  8：8位数据位    
* @param  stop_bit  1：1位停止位  2：2位停止位 （该参数在num为0、1时只能配2）
* @param  check_bit 0：无校验位   1：奇校验位 2：偶校验位
* @param  tx_pin 发送引脚的gpio号（0-63）（该参数在num为0时只能配3）
* @param  rx_pin 接收引脚的gpio号（0-63）（该参数在num为0时只能配4）
* @return  0：成功。 -1：失败，非法参数
************************************************************************************/
int8_t McuUartSet(uint8_t num, uint32_t baud_rate, uint8_t data_len, uint8_t stop_bit, uint8_t check_bit, uint8_t tx_pin, uint8_t rx_pin)
{
    uint32_t DataLen = 0, StopBit = 0, CheckBit = 0, base = 0;
    GPIO_InitTypeDef gpio_tx = {0}, gpio_rx = {0};

    //串口号检查、波特率检查
    if( (num > 3) || (baud_rate == 0) )
    {
    	debug_assert(0);
        return -1;
    }

    //获取波特率速率，关闭UART_TX时需要使用
    g_mcu_uart_rate[num] = baud_rate;

    if(num <= 1)
    {
        //数据位检查
        switch(data_len)
        {
            case 6: { DataLen = UART_CTL_CHAR_LEN_6; break; }
            case 7: { DataLen = UART_CTL_CHAR_LEN_7; break; }
            case 8: { DataLen = UART_CTL_CHAR_LEN_8; break; }
            default: 
            {
				debug_assert(0);
				return -1; //入参非法
            }
        }

        //停止位检查
        if(stop_bit != 2)
        {
            xy_assert(0);
        }

        //检验位检查
        switch(check_bit)
        {
            case 0: { CheckBit = UART_CTL_PARITY_NONE; break; }
            case 1: { CheckBit = UART_CTL_PARITY_ODD; break; }
            case 2: { CheckBit = UART_CTL_PARITY_EVEN; break; }
            default: 
            {
				debug_assert(0);
				return -1; //入参非法
            }
        }

        if(num == 0)
        {
        	xy_assert(tx_pin==MCU_GPIO3 && rx_pin==MCU_GPIO4);

            //GPIO3/4被分配为LPUART的TXD/RXD
            PRCM_LPUA1_PadSel(AON_LPUART_PAD_RXD_GPIO4);

            //配置LPUART时钟源与时钟分频系数
            PRCM_LPUA1_ClkSet_by_BaudRate(baud_rate);

            //开LPUART时钟
            PRCM_ClockEnable(CORE_CKG_CTL_LPUART_EN);

            //串口配置
            UARTDisable(UART1_BASE);
            UARTConfigSetExpClk(UART1_BASE, GetLpuartClockFreq(), baud_rate, DataLen | CheckBit | UART_CTL_ENDIAN_LITTLE);
            UARTTimeOutDisable(UART1_BASE);// 接收超时功能关闭
            UARTTxWaitSet(UART1_BASE, 0);  // 发送间隔设置为0
            UARTFIFODisable(UART1_BASE, UART_FIFO_ALL);

            //LPUART低功耗模式下的供电状态配置,A1的目前应用配置应为off in deepsleep。该电源控制与lptimer共用，需要与lptimer的需求
            //PRCM_LPUA_PWR_Ctl(LPUA_ANY_MODE_ON); //off in deepsleep

            //睡眠配置
            extern uint8_t g_lpuart_used;
            g_lpuart_used = 1;

#if (MODULE_VER == 0x0)	// opencpu,避免深睡前配置耗时
            PRCM_LPUA_PWR_Ctl(LPUA_ANY_MODE_ON); //force on in deesleep,lpuart和lptim中有一个维持工作，则需要维持外设寄存器不掉电。
#endif
            
            //注册中断向量
            NVIC_IntRegister(LPUART_IRQn, Uart0IrqHandle, 1);
        }
        else
        {
            //开GPIO时钟，开UART2时钟
            PRCM_ClockEnable(CORE_CKG_CTL_GPIO_EN);
            PRCM_ClockEnable(CORE_CKG_CTL_UART2_EN);

            //GPIO配置
            gpio_tx.Pin = tx_pin;
            gpio_tx.PinRemap = GPIO_UART2_TXD;
            gpio_tx.Mode = GPIO_MODE_HW_PER;
            GPIO_Init(&gpio_tx, NORMAL_SPEED);

            gpio_rx.Pin = rx_pin;
            gpio_rx.PinRemap = GPIO_UART2_RXD;
            gpio_rx.Mode = GPIO_MODE_HW_PER;
            GPIO_Init(&gpio_rx, NORMAL_SPEED);
            GPIO_InputPeriSelect(rx_pin, GPIO_UART2_RXD);
            GPIO_InputPeriSelectCmd(GPIO_UART2_RXD, ENABLE);

            //串口配置
            UARTDisable(UART2_BASE);
            UARTConfigSetExpClk(UART2_BASE, GetlsioFreq(), baud_rate, DataLen | CheckBit | UART_CTL_ENDIAN_LITTLE);
            UARTTimeOutDisable(UART2_BASE);// 接收超时功能关闭
            UARTTxWaitSet(UART2_BASE, 0);  // 发送间隔设置为0
            UARTFIFODisable(UART2_BASE, UART_FIFO_ALL);
            
            //注册中断向量
            NVIC_IntRegister(UART2_IRQn, Uart1IrqHandle, 1);
        }
    }
    else
    {
        //数据位检查
        switch(data_len)
        {
            case 6: { DataLen = 6; break; }
            case 7: { DataLen = 7; break; }
            case 8: { DataLen = 8; break; }
            default: 
            {
				debug_assert(0);
				return -1; //入参非法
            }
        }

        //停止位检查
        switch(stop_bit)
        {
            case 1: { StopBit = 1; break; }
            case 2: { StopBit = 2; break; }
            default: 
            {
				debug_assert(0);
				return -1; //入参非法
            }
        }

        //检验位检查
        switch(check_bit)
        {
            case 0: { CheckBit = CSP_UART_PARITYCHECK_None; break; }
            case 1: { CheckBit = CSP_UART_PARITYCHECK_Odd; break; }
            case 2: { CheckBit = CSP_UART_PARITYCHECK_Even; break; }
            default: 
            {
				debug_assert(0);
				return -1; //入参非法
            }
        }

        //配置共同点
        PRCM_ClockEnable(CORE_CKG_CTL_GPIO_EN);
        gpio_tx.Mode = GPIO_MODE_HW_PER;
        gpio_rx.Mode = GPIO_MODE_SW_PER_INPUT;
        gpio_rx.Pull = GPIO_PULL_UP;

        //配置差异点
        if(num == 2)
        {
            //GPIO配置
            gpio_tx.Pin = tx_pin;
            gpio_tx.PinRemap = GPIO_CSP2_TXD;
            GPIO_Init(&gpio_tx, NORMAL_SPEED);

            gpio_rx.Pin = rx_pin;
            gpio_rx.PinRemap = GPIO_CSP2_RXD;
            GPIO_Init(&gpio_rx, NORMAL_SPEED);
            GPIO_InputPeriSelect(rx_pin, GPIO_CSP2_RXD);
            GPIO_InputPeriSelectCmd(GPIO_CSP2_RXD, ENABLE);

            PRCM_ClockEnable(CORE_CKG_CTL_CSP2_EN);
            base = CSP2_BASE;

            //注册中断向量
            NVIC_IntRegister(CSP2_IRQn, Uart2IrqHandle, 1);
        }
        else
        {
            //GPIO配置
            gpio_tx.Pin = tx_pin;
            gpio_tx.PinRemap = GPIO_CSP3_TXD;
            GPIO_Init(&gpio_tx, NORMAL_SPEED);

            gpio_rx.Pin = rx_pin;
            gpio_rx.PinRemap = GPIO_CSP3_RXD;
            GPIO_Init(&gpio_rx, NORMAL_SPEED);
            GPIO_InputPeriSelect(rx_pin, GPIO_CSP3_RXD);
            GPIO_InputPeriSelectCmd(GPIO_CSP3_RXD, ENABLE);
            
            PRCM_ClockEnable(CORE_CKG_CTL_CSP3_EN);
            base = CSP3_BASE;

            //注册中断向量
            NVIC_IntRegister(CSP3_IRQn, Uart3IrqHandle, 1);
        }

        //串口配置
        CSP_Disable((CSP_TypeDef *)base);
        CSP_UARTModeSet2((CSP_TypeDef *)base, GetPCLK2Freq(), baud_rate, DataLen, CheckBit, StopBit);
        CSP_TxDisable((CSP_TypeDef *)base);
        CSP_RxDisable((CSP_TypeDef *)base);
    }

    return 0;
}

/************************************************************************************
* @brief  串口中断标志查询
* @param  num：串口号，可选0-3  
* @return 0x00：无中断产生  0x01：有发送中断产生  0x02：有接收中断产生  0x04：有接收超时中断产生。（可以为组合值）
************************************************************************************/
__RAM_FUNC static uint8_t McuUartIrqRead(uint8_t num)
{
    uint32_t base = 0, intsta = 0;
    uint8_t ret = 0;

    if(num <= 1)
    {
        switch (num)
        {
            case 0: { base = UART1_BASE; break; }
            case 1: { base = UART2_BASE; break; }
            default: break;
        }

        intsta = UARTIntRead(base);
        if(intsta & UART_INT_TXFIFO_EMPTY)
        {
            ret |= 0x01;
        }
        if(intsta & UART_INT_RXFIFO_TRIGGER)
        {
            ret |= 0x02;
        }
        if(intsta & UART_INT_TIMEOUT)
        {
            ret |= 0x04;
        }
    }
    else
    {
        switch (num)
        {
            case 2: { base = CSP2_BASE; break; }
            case 3: { base = CSP3_BASE; break; }
            default: break;
        }

        intsta = CSP_IntStatus((CSP_TypeDef *)base);
        if(intsta & CSP_INT_TX_DONE)
        {
            ret |= 0x01;
        }
        if(intsta & CSP_INT_RXFIFO_THD_REACH)
        {
            ret |= 0x02;
        }
        if(intsta & CSP_INT_RX_TIMEOUT)
        {
            ret |= 0x04;
        }  
    }

    return ret;
}

/************************************************************************************
* @brief  串口接收中断标志清零
* @param  num：串口号，可选0-3
* @return  NA
************************************************************************************/
__RAM_FUNC static void McuUartRxIrqClr(uint8_t num)
{
    uint32_t base = 0;

    if(num <= 1)
    {
        switch (num)
        {
            case 0: { base = UART1_BASE; break; }
            case 1: { base = UART2_BASE; break; }
            default: break;
        }
        UART_IntClear(base, UART_INT_RXFIFO_TRIGGER | UART_INT_TIMEOUT);
    }
    else
    {
        switch (num)
        {
            case 2: { base = CSP2_BASE; break; }
            case 3: { base = CSP3_BASE; break; }
            default: break;
        }
        CSP_IntClear((CSP_TypeDef *)base, CSP_INT_RXFIFO_THD_REACH | CSP_INT_RX_TIMEOUT);
    }
}

/************************************************************************************
* @brief  串口发送中断标志清零
* @param  num：串口号，可选0-3
* @return  NA
************************************************************************************/
__RAM_FUNC static void McuUartTxIrqClr(uint8_t num)
{
    uint32_t base = 0;

    if(num <= 1)
    {
        switch (num)
        {
            case 0: { base = UART1_BASE; break; }
            case 1: { base = UART2_BASE; break; }
            default: break;
        }
        UART_IntClear(base, UART_INT_TXFIFO_EMPTY);
    }
    else
    {
        switch (num)
        {
            case 2: { base = CSP2_BASE; break; }
            case 3: { base = CSP3_BASE; break; }
            default: break;
        }
        CSP_IntClear((CSP_TypeDef *)base, CSP_INT_TX_DONE);
    }
}

/************************************************************************************
* @brief  读取串口数据接收寄存器
* @param  num：串口号，可选0-3
* @param  out_byte：输出数据
* @return  0：无数据。1：有数据
************************************************************************************/
__RAM_FUNC static uint8_t McuUartReadAll(uint8_t num, uint8_t *out_byte)
{
    uint32_t base = 0;
    int32_t rxdata = 0;

    if(num <= 1)
    {
        switch (num)
        {
            case 0: { base = UART1_BASE; break; }
            case 1: { base = UART2_BASE; break; }
            default: break;
        }

        //获取数据
        rxdata = UARTCharGetNonBlocking(base);
        if(rxdata == -1)
        {
            return 0;
        }
        else
        {
            *out_byte = rxdata & 0xFF;
            return 1;
        }
    }
    else
    {
        switch (num)
        {
            case 2: { base = CSP2_BASE; break; }
            case 3: { base = CSP3_BASE; break; }
            default: break;
        }

        //获取数据
        rxdata = CSP_CharGetNonBlocking((CSP_TypeDef *)base);
        if(rxdata == -1)
        {
            return 0;
        }
        else
        {
            *out_byte = rxdata & 0xFF;
            return 1;
        }
    }

    return 0;
}

/************************************************************************************
* @brief  串口接收使能。接口耗时：num为0耗时7.5us，num为1耗时5.3us，num为2耗时5.6us，num为3耗时5.6us
* @param  num：串口号，可选0-3;0：LPUART;1:UART;2:CSP2_UART;3:CSP3_UART
* @return NA
************************************************************************************/
void McuUartRxEn(uint8_t num)
{
    uint32_t base = 0;

    if(num <= 1)
    {
        switch (num)
        {
            case 0: { base = UART1_BASE; break; }
            case 1: { base = UART2_BASE; break; }
            default: break;
        }
        UARTFIFOEnable(base, UART_FIFO_RX);
    }
    else
    {
        switch (num)
        {
            case 2: { base = CSP2_BASE; break; }
            case 3: { base = CSP3_BASE; break; }
            default: break;
        }
        CSP_RxEnable((CSP_TypeDef *)base);
    }  
}

/************************************************************************************
* @brief  串口接收禁能。接口耗时：num为0耗时7.5us，num为1耗时5.3us，num为2耗时5.6us，num为3耗时5.6us
* @param  num：串口号，可选0-3;0：LPUART;1:UART;2:CSP2_UART;3:CSP3_UART
* @return NA
************************************************************************************/
void McuUartRxDis(uint8_t num)
{
    uint32_t base = 0;

    if(num <= 1)
    {
        switch (num)
        {
            case 0: { base = UART1_BASE; break; }
            case 1: { base = UART2_BASE; break; }
            default: break;
        }
        UARTFIFODisable(base, UART_FIFO_RX);
    }
    else
    {
        switch (num)
        {
            case 2: { base = CSP2_BASE; break; }
            case 3: { base = CSP3_BASE; break; }
            default: break;
        }
        CSP_RxDisable((CSP_TypeDef *)base);
    } 
}

/************************************************************************************
* @brief  串口发送使能。接口耗时：num为0耗时8.3us，num为1耗时6.1us，num为2耗时5.6us，num为3耗时5.6us
* @param  num：串口号，可选0-3;0：LPUART;1:UART;2:CSP2_UART;3:CSP3_UART
* @return NA
************************************************************************************/
void McuUartTxEn(uint8_t num)
{
    uint32_t base = 0;

    if(num <= 1)
    {
        switch (num)
        {
            case 0: { base = UART1_BASE; break; }
            case 1: { base = UART2_BASE; break; }
            default: break;
        }
        UARTFIFOEnable(base, UART_FIFO_TX);
    }
    else
    {
        switch (num)
        {
            case 2: { base = CSP2_BASE; break; }
            case 3: { base = CSP3_BASE; break; }
            default: break;
        }
        CSP_TxEnable((CSP_TypeDef *)base);
    }
}

/**
 * @brief  获取UART_TXFIFO的使能状态
 * @param  num：串口号，可选0-3;0：LPUART;1:UART;2:CSP2_UART;3:CSP3_UART
 * @retval 0:禁能，1:使能
 */
static uint8_t McuUart_Get_TxEnable(uint8_t num)
{
    switch (num)
    {
        case 0: { return UART_TxEnable_Get(UART1_BASE); }
        case 1: { return UART_TxEnable_Get(UART2_BASE); }
        case 2: { return ((CSP2->TX_RX_ENABLE & CSP_TX_RX_ENABLE_TX_ENA) ? 1 : 0); }
        case 3: { return ((CSP3->TX_RX_ENABLE & CSP_TX_RX_ENABLE_TX_ENA) ? 1 : 0); }
        default:;
    }
}

/************************************************************************************
* @brief  串口发送禁能。接口耗时：num为0耗时238.8us，num为1耗时236.7us，num为2耗时6.6us，num为3耗时6.6us
* @param  num：串口号，可选0-3;0：LPUART;1:UART;2:CSP2_UART;3:CSP3_UART
* @return NA
************************************************************************************/
__RAM_FUNC void McuUartTxDis(uint8_t num)
{
    uint32_t base = 0;

    if(McuUart_Get_TxEnable(num))
    {
        //针对不同速率，增加1个字符的延时，以保证退出该接口时数据发送完成
        if(g_mcu_uart_rate[num] != 0)
        {
            uint32_t onechar_timeout = (uint32_t)(10 * 1000 * 1000 / g_mcu_uart_rate[num]);
            if(onechar_timeout < 50)
            {
                onechar_timeout = 50;
            }
            delay_func_us(onechar_timeout);
        }
    }

    if(num <= 1)
    {
        switch (num)
        {
            case 0: { base = UART1_BASE; break; }
            case 1: { base = UART2_BASE; break; }
            default: break;
        }
        UARTFIFODisable(base, UART_FIFO_TX);
    }
    else
    {
        switch (num)
        {
            case 2: { base = CSP2_BASE; break; }
            case 3: { base = CSP3_BASE; break; }
            default: break;
        }
        CSP_TxDisable((CSP_TypeDef *)base);
    }
}

/************************************************************************************
* @brief  写入串口一字节数据。接口耗时：num为0耗时9.8us，num为1耗时7.7us，num为2耗时8.8us，num为3耗时8.8us
* @param  num：串口号，可选0-3;0：LPUART;1:UART;2:CSP2_UART;3:CSP3_UART
* @param  in_byte：要发送的1字节数据
* @return 0：成功。-1：失败
************************************************************************************/
int8_t McuUartWrite(uint8_t num, uint8_t in_byte)
{
    uint32_t base = 0;

    if(num <= 1)
    {
        switch (num)
        {
            case 0: { base = UART1_BASE; break; }
            case 1: { base = UART2_BASE; break; }
            default: break;
        }

        //写入数据
        if(UARTCharPutNonBlocking(base, in_byte) == true)
        {
            return 0;
        }
        else
        {
        	debug_assert(0);
            return -1;
        }
    }
    else
    {
        switch (num)
        {
            case 2: { base = CSP2_BASE; break; }
            case 3: { base = CSP3_BASE; break; }
            default: break;
        }

        //写入数据
        if(CSP_CharPutNonBlocking((CSP_TypeDef *)base, in_byte) == true)
        {
            return 0;
        }
        else
        {
        	debug_assert(0);
            return -1;
        }
    }

    return -1;
}

/************************************************************************************
* @brief  串口发送总线忙查询。接口耗时：num为0耗时9.6us，num为1耗时7.5us，num为2耗时8.8us，num为3耗时8.8us
* @param  num：串口号，可选0-3;0：LPUART;1:UART;2:CSP2_UART;3:CSP3_UART
* @return 0：空闲  1：忙
************************************************************************************/
int8_t McuUartTxBusy(uint8_t num)
{
    uint32_t base = 0;

    if(num <= 1)
    {
        if(num == 1)
        {
            //检查TXFIFO是否满，没满则是空闲
            if(UARTTxFifoStatusGet(UART2_BASE, UART_FIFO_FULL) == 0)
            {
                return 0;
            }
        }
        else
        {
            //检查TXFIFO是否满，没满则是空闲
            if(LPUART_IS_TXFIFO_FULL() == 0)
            {
                return 0;
            }
        }

        return 1;
    }
    else
    {
        switch (num)
        {
            case 2: { base = CSP2_BASE; break; }
            case 3: { base = CSP3_BASE; break; }
            default: break;
        }

        //检查TXFIFO是否满，没满则是空闲
        if(CSP_TxFifoStatusGet((CSP_TypeDef *)base, CSP_FIFO_FULL) == 0)
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }

    return 0;
}

/************************************************************************************
* @brief  串口写指定长度数据。以115200bps为例，接口耗时如下，其中n为字节长度。
* num为0耗时45+95*(n-1)us，num为1耗时35+95*(n-1)us，
* num为2耗时115+85*(n-1)us， num为3耗时115+85*(n-1)us，
* @param  num：串口号，可选0-3;0：LPUART;1:UART;2:CSP2_UART;3:CSP3_UART     
* @param  buff：要发送数据的地址
* @param  length：要发送的数据长度
* @return 0：成功。-1：失败
************************************************************************************/
int8_t McuUartWriteFram(uint8_t num, uint8_t *buff, uint16_t length)
{
    uint32_t base = 0;

    if(length == 0)
    {
        return -1;
    }

    if(num > 3)
    {
    	debug_assert(0);
        return -1;
    }

    //设置基地址
    if(num >= 2)
    {
        switch (num)
        {
            case 2: { base = CSP2_BASE; break; }
            case 3: { base = CSP3_BASE; break; }
            default: break;
        }
        CSP_IntClear((CSP_TypeDef *)base, CSP_INT_TX_ALLOUT);
    }

    //发送数据
    while(length)
    {
        if(McuUartTxBusy(num) == 0)
        {
            McuUartWrite(num, *buff);
            length--;
            buff++;
        }
    }

    //等待TXFIFO为空
    if(num <= 1)
    {
        if(num == 1)
        {
            while(UARTTxFifoStatusGet(UART2_BASE, UART_FIFO_EMPTY) == 0);
        }
        else
        {
            while(LPUART_IS_TXFIFO_EMPTY() == 0){;}
        }
    }
    else
    {
        while(!(((CSP_TypeDef *)base)->INT_STATUS & CSP_INT_TX_ALLOUT));
        CSP_IntClear((CSP_TypeDef *)base, CSP_INT_TX_ALLOUT);
    }

    //加1字节延时
    uint32_t onechar_timeout = (uint32_t)(10 * 1000 * 1000 / g_mcu_uart_rate[num]);
    if(onechar_timeout < 50)
    {
        onechar_timeout = 50;
    }
    delay_func_us(onechar_timeout);

    return 0;
}

/************************************************************************************
* @brief  读取串口的一字节数据。接口耗时：num为0耗时5.4us，num为1耗时5.6us，num为2耗时5.6us，num为3耗时5.6us
* @param  num：串口号，可选0-3;0：LPUART;1:UART;2:CSP2_UART;3:CSP3_UART
* @param  out_byte：输出数据
* @return 0：无数据。1：有数据
************************************************************************************/
__RAM_FUNC uint8_t McuUartRead(uint8_t num, uint8_t *out_byte)
{
    //如果注册了中断回调函数，则通过句柄MCU_UART获取数据
    if (MCU_UART[num].rx_irq != NULL)
    {
        *out_byte = MCU_UART[num].rx_buff;
        return 1;
    }
    //否则为轮询方式，从寄存器获取数据
    else
    {
        return McuUartReadAll(num, out_byte);
    }
}

/************************************************************************************
 * @brief 判断串口接收fifo是否空
 * @param num 串口号，可选0-3；0:LPUART、1:UART、2:CSP2_UART、3:CSP3_UART
 * @return 0:接收fifo不空(即接收未完成)
 *         1:接收fifo空(即接收完成)
 *        -1:入参错误
************************************************************************************/
__RAM_FUNC int8_t McuIsUartRxfifoEmpty(uint8_t num)
{
    //设置基地址
    uint32_t base = 0;
    switch(num)
    {
        case 0: { base = UART1_BASE; break; }
        case 1: { base = UART2_BASE; break; }
        case 2: { base = CSP2_BASE; break; }
        case 3: { base = CSP3_BASE; break; }
        default: 
        {
			debug_assert(0);
			return -1; //入参非法
        }
    }

    //等待RXFIFO空
    if(num <= 1)
    {
        return UARTRxFifoStatusGet(base, UART_FIFO_EMPTY); //1:empty、0:not empty
    }
    else
    {
        return CSP_RxFifoStatusGet((CSP_TypeDef *)base, CSP_FIFO_EMPTY); //1:empty、0:not empty
    }
}

/************************************************************************************
 * @brief 判断串口发送fifo是否空
 * @param num 串口号，可选0-3；0:LPUART、1:UART、2:CSP2_UART、3:CSP3_UART
 * @return 0:发送fifo不空(即发送未完成)
 *         1:发送fifo空(即发送完成)
 *        -1:入参错误
************************************************************************************/
int8_t McuIsUartTxfifoEmpty(uint8_t num)
{
    //设置基地址
    uint32_t base = 0;
    switch(num)
    {
        case 2: { base = CSP2_BASE; break; }
        case 3: { base = CSP3_BASE; break; }
        default: 
        {
			debug_assert(0);
			return -1; //入参非法
        }
    }

    //等待TXFIFO空
    if(num == 0)
    {
        return LPUART_IS_TXFIFO_EMPTY();
    }
    else if(num == 1)
    {
        return UARTTxFifoStatusGet(UART2_BASE, UART_FIFO_EMPTY); //1:empty、0:not empty
    }
    else
    {
        return CSP_TxFifoStatusGet((CSP_TypeDef *)base, CSP_FIFO_EMPTY); //1:empty、0:not empty
    }
}

/************************************************************************************
 * @brief  LPUART中断服务函数
 * @return NA
************************************************************************************/
__RAM_FUNC static void Uart0IrqHandle(void)
{
    uint8_t irq_sta = McuUartIrqRead(0);

    // 接收阈值中断中断产生，且接收中断回调函数指针非空，则执行回调
    if((irq_sta & 0x02) && (MCU_UART[0].rx_irq != NULL))
    {
        McuUartRxIrqClr(0);
        while(McuUartReadAll(0, &MCU_UART[0].rx_buff))//读空本次RXFIFO
        {
            MCU_UART[0].rx_irq(0); //每字节回调一次，0表示有接收数据产生
        }

        //阈值中断且硬件超时类型是RXFIFO非空超时时，则切换硬件超时为RXIDLE超时
        if(UART_TimeoutCondition_Get(UART1_BASE))
        {
            UARTTimeOutDisable(UART1_BASE);
            UARTTimeOutCondition_Set(UART1_BASE, UART_RX_TIMEOUT_START_NO_MATTER);
            UARTTimeOutEnable(UART1_BASE);
        }
    }

    // 接收超时中断产生，且接收中断回调函数指针非空，则执行回调
    if((irq_sta & 0x04) && (MCU_UART[0].rx_irq != NULL))
    {
        McuUartRxIrqClr(0);
        while(McuUartReadAll(0, &MCU_UART[0].rx_buff))//读空本次RXFIFO
        {
            MCU_UART[0].rx_irq(0); //每字节回调一次，0表示有接收数据产生
        }

        McuUartRxIrqClr(0); //解决问题：当波特率很高(921600)且收到的串口数据较长时，进入接收完成回调前会带一个超时中断标志位
        MCU_UART[0].rx_irq(1); //1表示RX数据线空闲

        //配置超时触条件为RXD处于IDLE且RXFIFO非空
        UARTTimeOutDisable(UART1_BASE);
        UARTTimeOutCondition_Set(UART1_BASE, UART_RX_TIMEOUT_START_FIFO_NEMPTY);
        UARTTimeOutEnable(UART1_BASE);
    }

    // 发送中断产生，且发送中断回调函数指针非空，则执行回调
    if((irq_sta & 0x01) && (MCU_UART[0].tx_irq != NULL))
    {
        McuUartTxIrqClr(0);
        MCU_UART[0].tx_irq(); 
    }
}

/************************************************************************************
 * @brief  UART中断服务函数
 * @return NA
************************************************************************************/
__RAM_FUNC static void Uart1IrqHandle(void)
{
    uint8_t irq_sta = McuUartIrqRead(1);

    // 接收阈值中断中断产生，且接收中断回调函数指针非空，则执行回调
    if((irq_sta & 0x02) && (MCU_UART[1].rx_irq != NULL))
    {
        McuUartRxIrqClr(1);
        while(McuUartReadAll(1, &MCU_UART[1].rx_buff))//读空本次RXFIFO
        {
            MCU_UART[1].rx_irq(0); //每字节回调一次，0表示有接收数据产生
        }

        //阈值中断且硬件超时类型是RXFIFO非空超时时，则切换硬件超时为RXIDLE超时
        if(UART_TimeoutCondition_Get(UART2_BASE))
        {
            UARTTimeOutDisable(UART2_BASE);
            UARTTimeOutCondition_Set(UART2_BASE, UART_RX_TIMEOUT_START_NO_MATTER);
            UARTTimeOutEnable(UART2_BASE);
        }
    }

    // 接收超时中断产生，且接收中断回调函数指针非空，则执行回调
    if((irq_sta & 0x04) && (MCU_UART[1].rx_irq != NULL))
    {
        McuUartRxIrqClr(1);
        while(McuUartReadAll(1, &MCU_UART[1].rx_buff))//读空本次RXFIFO
        {
            MCU_UART[1].rx_irq(0); //每字节回调一次，0表示有接收数据产生
        }

        McuUartRxIrqClr(1); //解决问题：当波特率很高(921600)且收到的串口数据较长时，进入接收完成回调前会带一个超时中断标志位
        MCU_UART[1].rx_irq(1); //1表示RX数据线空闲

        //配置超时触条件为RXD处于IDLE且RXFIFO非空
        UARTTimeOutDisable(UART2_BASE);
        UARTTimeOutCondition_Set(UART2_BASE, UART_RX_TIMEOUT_START_FIFO_NEMPTY);
        UARTTimeOutEnable(UART2_BASE);
    }

    // 发送中断产生，且发送中断回调函数指针非空，则执行回调
    if((irq_sta & 0x01) && (MCU_UART[1].tx_irq != NULL))
    {
        McuUartTxIrqClr(1);
        MCU_UART[1].tx_irq(); 
    }
}

/************************************************************************************
 * @brief  CSP2_UART中断服务函数
 * @return NA
************************************************************************************/
__RAM_FUNC static void Uart2IrqHandle(void)
{
    uint8_t irq_sta = McuUartIrqRead(2);

    // 接收中断产生，且接收中断回调函数指针非空，则执行回调
    if((irq_sta & 0x02) && (MCU_UART[2].rx_irq != NULL))
    {
        McuUartRxIrqClr(2);
        while(McuUartReadAll(2, &MCU_UART[2].rx_buff))//读空本次RXFIFO
        {
            MCU_UART[2].rx_irq(0); //每字节回调一次，0表示有接收数据产生
        }

        //阈值中断且硬件超时类型是RXFIFO非空超时时，则切换硬件超时为RXIDLE超时
        if(CSP_UART_TimeoutCondition_Get(CSP2))
        {
            CSP_UART_TimeoutCondition_Set(CSP2, 0);
        }
    }

    // 接收超时中断产生，且接收中断回调函数指针非空，则执行回调
    if((irq_sta & 0x04) && (MCU_UART[2].rx_irq != NULL))
    {
        McuUartRxIrqClr(2);
        while(McuUartReadAll(2, &MCU_UART[2].rx_buff))//读空本次RXFIFO
        {
            MCU_UART[2].rx_irq(0); //每字节回调一次，0表示有接收数据产生
        }

        McuUartRxIrqClr(2); //解决问题：当波特率很高(921600)且收到的串口数据较长时，进入接收完成回调前会带一个超时中断标志位
        MCU_UART[2].rx_irq(1); //1表示RX数据线空闲

        //配置超时触条件为RXD处于IDLE且RXFIFO非空
        CSP_UART_TimeoutCondition_Set(CSP2, 1);
    }

    // 发送中断产生，且发送中断回调函数指针非空，则执行回调
    if((irq_sta & 0x01) && (MCU_UART[2].tx_irq != NULL))
    {
        McuUartTxIrqClr(2);
        MCU_UART[2].tx_irq();
    }
}

/************************************************************************************
 * @brief  CSP3_UART中断服务函数
 * @return NA
************************************************************************************/
__RAM_FUNC static void Uart3IrqHandle(void)
{
    uint8_t irq_sta = McuUartIrqRead(3);

    // 接收中断产生，且接收中断回调函数指针非空，则执行回调
    if((irq_sta & 0x02) && (MCU_UART[3].rx_irq != NULL))
    {
        McuUartRxIrqClr(3);
        while(McuUartReadAll(3, &MCU_UART[3].rx_buff))//读空本次RXFIFO
        {
            MCU_UART[3].rx_irq(0); //每字节回调一次，0表示有接收数据产生
        }

        //阈值中断且硬件超时类型是RXFIFO非空超时时，则切换硬件超时为RXIDLE超时
        if(CSP_UART_TimeoutCondition_Get(CSP3))
        {
            CSP_UART_TimeoutCondition_Set(CSP3, 0);
        }
    }

    // 接收超时中断产生，且接收中断回调函数指针非空，则执行回调
    if((irq_sta & 0x04) && (MCU_UART[3].rx_irq != NULL))
    {
        McuUartRxIrqClr(3);
        while(McuUartReadAll(3, &MCU_UART[3].rx_buff))//读空本次RXFIFO
        {
            MCU_UART[3].rx_irq(0); //每字节回调一次，0表示有接收数据产生
        }

        McuUartRxIrqClr(3); //解决问题：当波特率很高(921600)且收到的串口数据较长时，进入接收完成回调前会带一个超时中断标志位
        MCU_UART[3].rx_irq(1); //1表示RX数据线空闲

        //配置超时触条件为RXD处于IDLE且RXFIFO非空
        CSP_UART_TimeoutCondition_Set(CSP3, 1);
    }

    // 发送中断产生，且发送中断回调函数指针非空，则执行回调
    if((irq_sta & 0x01) && (MCU_UART[3].tx_irq != NULL))
    {
        McuUartTxIrqClr(3);
        MCU_UART[3].tx_irq(); 
    }
}

/************************************************************************************
* @brief  串口接收中断函数注册。接口耗时：num为0耗时43.6us，num为1耗时31us，num为2耗时28.4us，num为3耗时28.7us
* @param  num：串口号，可选0-3;0：LPUART;1:UART;2:CSP2_UART;3:CSP3_UART
* @param  p_fun：串口接收回调函数
* @return NA
************************************************************************************/
void McuUartRxIrqReg(uint8_t num, pFunType_u8 p_fun)
{
    uint32_t base = 0;

    if (num >= MCU_UART_NUM) xy_assert(0);

    //注册用户中断回调函数
    MCU_UART[num].rx_irq = p_fun;
    mark_dyn_addr(&MCU_UART[num].rx_irq);

    if(num <= 1)
    {
        switch (num)
        {
            case 0: { base = UART1_BASE; break; }
            case 1: { base = UART2_BASE; break; }
            default:;
        }

        //RXFIFO数据存量大于等于32字节时触发RXFIFO阈值中断
        UART_RXFIFO_LevelSet(base, UART_FIFO_LEVEL_RX3_4);

        //Timeout倒计时触发条件是RXD处于IDLE且RXFIFO非空，9600下超时时长约1ms
        UARTTimeOutDisable(base);
        UARTTimeOutConfig(base, UART_RX_TIMEOUT_START_FIFO_NEMPTY, 31);
        UARTTimeOutEnable(base);

        //开启接收阈值中断和接收超时中断
        UARTIntEnable(base, UART_INT_RXFIFO_TRIGGER | UART_INT_TIMEOUT);
    }
    else
    {
        switch (num)
        {
            case 2: { base = CSP2_BASE; break; }
            case 3: { base = CSP3_BASE; break; }
            default:;
        }

        //RXFIFO数据存量大于31字节时触发RXFIFO阈值中断
        CSP_RXFIFO_LevelSet((CSP_TypeDef *)base, 31);

        //Timeout倒计时触发条件是RXD处于IDLE且RXFIFO非空，9600下超时时长约1ms
        CSP_UARTRxTimeoutConfig((CSP_TypeDef *)base, 1, 31);

        //开启接收阈值中断和接收超时中断
        CSP_IntEnable((CSP_TypeDef *)base, CSP_INT_RXFIFO_THD_REACH | CSP_INT_RX_TIMEOUT);
    }
}

/************************************************************************************
* @brief  串口发送中断函数注册。接口耗时：num为0耗时19.4us，num为1耗时18us，num为2耗时18.4us，num为3耗时18.8us
* @param  num：串口号，可选0-3. 0：LPUART;1:UART;2:CSP2_UART;3:CSP3_UART
* @param  p_fun：串口发送回调函数
* @return NA
************************************************************************************/
void McuUartTxIrqReg(uint8_t num, pFunType_void p_fun)
{
    uint32_t base = 0;

    if (num >= MCU_UART_NUM) xy_assert(0);

    //注册用户中断回调函数
    MCU_UART[num].tx_irq = p_fun;
    mark_dyn_addr(&MCU_UART[num].tx_irq);

    if(num <= 1)
    {
        switch (num)
        {
            case 0: { base = UART1_BASE; break; }
            case 1: { base = UART2_BASE; break; }
            default:;
        }

        //开启TXFIFO空中断
        UARTIntEnable(base, UART_INT_TXFIFO_EMPTY);
    }
    else
    {
        switch (num)
        {
            case 2: { base = CSP2_BASE; break; }
            case 3: { base = CSP3_BASE; break; }
            default:;
        }

        //开启TXFIFO空中断
        CSP_IntEnable((CSP_TypeDef *)base, CSP_INT_TX_DONE);
    }
}
