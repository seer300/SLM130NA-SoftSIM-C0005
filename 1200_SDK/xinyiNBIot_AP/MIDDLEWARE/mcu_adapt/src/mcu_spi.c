#include "mcu_adapt.h"

static uint8_t g_cs_pin[2] = {0};//cs引脚，0：csp1_spi的片选，1：spi的片选

//=====================================================================================
//================================SPI==================================================
//=====================================================================================
/*******************************warning！SPI使用注意事项***********************************
	num值为1表示SPI硬件实现SPI功能，其优点为耗时相比CSP_SPI要小，其缺点为受起CP核影响工作主频。

    num值为0表示使用CSP1实现SPI功能，其优点为不受起CP核影响工作主频，其缺点为耗时比SPI硬件大。
    CSP1外设pclk典型值为26MHz，作为SPI使用时，具有以下特性：
    (1)模拟为SPI主机时，SPI速率入参不得超过pclk的1/2
    (2)模拟为SPI从机时，SPI速率入参不得超过pclk的1/16

    此外当CSP模拟为SPI从机时，SPI从机对主机片选信号（CS#）有如下要求：
    当SPI主机发送完一字节数据后，主机必须拉高拉低一次片选信号（CS#），才可以继续给从机发送数据。
    如果SPI主机在发送过程中片选信号一直保持拉低，则SPI从机的数据发送和接收均会失败。
**************************************************************************************/
/************************************************************************************
* @brief  带引脚配置的SPI主机设置。接口耗时：num为0耗时：1.2623ms,num为1耗时：1.1958ms
* @param  num：spi底层硬件类型号，0:CSP1_SPI,1:SPI
* @param  freq：频率，单位KHZ
                num为0时可选：100,500,1000,2000,4000,6000
                num为1时可选：360,720,1440,2880,5760
* @param  mode      ：SPI工作模式，可选0,1,2,3
* @param  clk_pin   ：SPI主机时钟线
* @param  cs_pin    ：SPI主机片选线，MasterSet后为默认为高电平，数据收发过程中需拉低，收发完毕需拉高。
*                     该引脚除了可选普通MCU_GPIOx外，还可以选择深睡保持引脚，如MCU_WKP1~3，MCU_GPIO0~7。
* @param  mosi_pin  ：SPI主机数据输出线
* @param  miso_pin  ：SPI主机数据输入线
* @return  0：成功。 -1：失败，非法参数
************************************************************************************/
int8_t McuSpiMasterSet2(uint8_t num, uint32_t freq, uint32_t mode, uint8_t clk_pin, uint8_t cs_pin, uint8_t mosi_pin, uint8_t miso_pin)
{
    GPIO_InitTypeDef spi_gpio = {0};
    uint32_t spi_freq = 0, pclk = 0;

    //转换SPI协议的工作模式为具体硬件接口的入参
    uint8_t CPOL = 0, CPHA = 0; //CSP_SPI的工作模式参数
    uint32_t work_mode = 0;     //SPI的工作模式参数
    switch(mode)
    {
        case 0: { CPOL = 0, CPHA = 0; work_mode = SPI_FRF_MOTO_MODE_0; break; }
        case 1: { CPOL = 0, CPHA = 1; work_mode = SPI_FRF_MOTO_MODE_1; break; }
        case 2: { CPOL = 1, CPHA = 0; work_mode = SPI_FRF_MOTO_MODE_2; break; }
        case 3: { CPOL = 1, CPHA = 1; work_mode = SPI_FRF_MOTO_MODE_3; break; }
        default: return -1;
    }

    //SPI配置
    if(num == 0)
    {
        //spi速率选择
        switch(freq)
        {
            case 100: { spi_freq = 100000; break; }
            case 500: { spi_freq = 500000; break; }
            case 1000: { spi_freq = 1000000; break; }
            case 2000: { spi_freq = 2000000; break; }
            case 4000: { spi_freq = 4000000; break; }
            case 6000: { spi_freq = 6000000; break; }
            default: return -1;
        }
        //CSP外设时钟源选择
        pclk = GetlsioFreq();

        //校验spi速率与外设时钟源频率是否满足
        if(spi_freq > pclk/2)
        {
            return -1;
        }

        //开时钟
        PRCM_ClockEnable(CORE_CKG_CTL_GPIO_EN);//GPIO时钟
        PRCM_ClockEnable(CORE_CKG_CTL_CSP1_EN);//CSP1时钟

        //CSP_SPI gpio配置
        spi_gpio.Mode = GPIO_MODE_HW_PER;
        spi_gpio.DrvStrength = GPIO_DRV_STRENGTH_7;
        spi_gpio.PinRemap = GPIO_CSP1_SCLK;//CLK
        spi_gpio.Pin = clk_pin;
        GPIO_Init(&spi_gpio, NORMAL_SPEED);

        McuGpioModeSet(cs_pin, 0); //CS
        g_cs_pin[num] = cs_pin;

        spi_gpio.PinRemap = GPIO_CSP1_TXD;//MOSI
        spi_gpio.Pin = mosi_pin;
        GPIO_Init(&spi_gpio, NORMAL_SPEED);

        spi_gpio.PinRemap = GPIO_CSP1_RXD;//MISO
        spi_gpio.Pin = miso_pin;
        GPIO_Init(&spi_gpio, NORMAL_SPEED);
        GPIO_InputPeriSelect(miso_pin, GPIO_CSP1_RXD);
        GPIO_InputPeriSelectCmd(GPIO_CSP1_RXD, ENABLE);

        //配置CSP_SPI：工作模式x、SPI主机、8位传输
        CSP_Disable(CSP1);
        CSP_RxDisable(CSP1);
        CSP_TxDisable(CSP1);
        CSP_FifoReset(CSP1);
        CSP_FifoStart(CSP1);
        CSP_SPIConfigSetExpClk(CSP1, pclk, spi_freq, CSP_MODE1_CLOCK_MODE_Master, CPOL, CPHA, 8);
        McuSpiCsSet(num);//cs引脚初始化为高电平，不选中从机
    }
    else if(num == 1)
    {
        //注意：BootCP后pclk2为92.16MHz，以下所有频率值都以此频率除以div所得
        switch(freq)
        {
            case 5760: { spi_freq = SPI_CONFIG_CLK_DIV_16; break; }
            case 2880: { spi_freq = SPI_CONFIG_CLK_DIV_32; break; }
            case 1440: { spi_freq = SPI_CONFIG_CLK_DIV_64; break; }
            case 720:  { spi_freq = SPI_CONFIG_CLK_DIV_128; break; }
            case 360:  { spi_freq = SPI_CONFIG_CLK_DIV_256; break; }
            default: return -1;
        }
        //开时钟
        PRCM_ClockEnable(CORE_CKG_CTL_GPIO_EN);//GPIO时钟
        PRCM_ClockEnable(CORE_CKG_CTL_SPI_EN);//SPI时钟

        //同一个引脚即作为CS输出又作为CS输入时有效
        //对于有输入、输出功能的PAD需要失能输入选择
        GPIO_InputPeriSelectCmd(GPIO_SPI_CS_N, DISABLE);

        //spi_gpio配置
        spi_gpio.Mode = GPIO_MODE_HW_PER;
        spi_gpio.DrvStrength = GPIO_DRV_STRENGTH_7;
        spi_gpio.PinRemap = GPIO_SPI_SCLK;
        spi_gpio.Pin = clk_pin;
        GPIO_Init(&spi_gpio, NORMAL_SPEED);

        //cs引脚配置
        McuGpioModeSet(cs_pin, 0);
        g_cs_pin[num] = cs_pin;

        spi_gpio.PinRemap = GPIO_SPI_MOSI;
        spi_gpio.Pin = mosi_pin;
        GPIO_Init(&spi_gpio, NORMAL_SPEED);

        spi_gpio.PinRemap = GPIO_SPI_MISO;
        spi_gpio.Pin = miso_pin;
        GPIO_Init(&spi_gpio, NORMAL_SPEED);
        GPIO_InputPeriSelect(miso_pin, GPIO_SPI_MISO);
        GPIO_InputPeriSelectCmd(GPIO_SPI_MISO, ENABLE);

        //SPI配置：工作模式2、SPI主机、8位传输
        SPI_Disable();
        SPI_TxFifoDisable();
        SPI_RxFifoDisable();
        SPI_ConfigSetExpClk(spi_freq, work_mode, SPI_CONFIG_MODE_MASTER, SPI_CONFIG_WORD_SIZE_BITS_8);
        SPI_SetDelay(0x02, 0x04, 0x04, 0x04); // MASTER时的输出时序延迟
        SPI_DisManualTransmit(); //关闭手动TXFIFO发送
        McuSpiCsSet(num); //cs引脚初始化为高电平，不选中从机
    } 
    else
    {
        return -1;
    }
    return 0;
}

/************************************************************************************
* @brief  SPI主机选用的端口配置。接口耗时：num为0耗时：1.2623ms,num为1耗时：1.1958ms
* @param  num：spi底层硬件类型号，0:CSP1_SPI,1:SPI
* @param  freq：频率，单位KHZ
                num为0时可选：100,500,1000,2000,4000,6000
                num为1时可选：360,720,1440,2880,5760
* @param  mode ：SPI工作模式，可选0,1,2,3
* @return  0：成功。 -1：失败，非法参数
************************************************************************************/
int8_t McuSpiMasterSet(uint8_t num, uint32_t freq, uint32_t mode)
{
    switch (num)
    {
        //CSP1_SPI，普通SPI，深睡不保持，参考时钟不受启停CP影响
        case 0: { return McuSpiMasterSet2(num, freq, mode, MCU_GPIO25, MCU_GPIO26, MCU_GPIO52, MCU_GPIO53); }
        //SPI，普通SPI，深睡不保持，参考时钟受启停CP影响
        case 1: { return McuSpiMasterSet2(num, freq, mode, MCU_GPIO49, MCU_GPIO27, MCU_GPIO31, MCU_GPIO30); } 
        default: return -1;
    }
}

/************************************************************************************
* @brief   SPI使能。接口耗时：num为0耗时：22.7us,num为1耗时：29.9us
* @param   num：spi底层硬件类型号，0:CSP1_SPI,1:SPI
* @return   NA
************************************************************************************/
void McuSpiEn(uint8_t num)
{
    if(num == 0)
    {
        PRCM_ClockEnable(CORE_CKG_CTL_CSP1_EN);//开CSP1时钟，避免没有初始化时，调用该接口卡住

        CSP_TxEnable(CSP1);
        CSP_RxEnable(CSP1);
        CSP_Enable(CSP1);
    }
    else if(num == 1)
    {
        //开时钟，避免没有初始化时，调用该接口卡住
        PRCM_ClockEnable(CORE_CKG_CTL_GPIO_EN);//GPIO时钟
        PRCM_ClockEnable(CORE_CKG_CTL_SPI_EN);//SPI时钟

        GPIO_InputPeriInvertCmd(GPIO_SPI_CS_N, ENABLE); //SPI从机有效
        SPI_TxFifoEnable();
        SPI_RxFifoEnable();
	    SPI_Enable();
        GPIO_InputPeriInvertCmd(GPIO_SPI_CS_N, DISABLE); //SPI从机有效
    }
}

/************************************************************************************
* @brief   SPI禁能。接口耗时：num为0耗时：22.5us,num为1耗时：17.2us
* @param   num：spi底层硬件类型号，0:CSP1_SPI,1:SPI
* @return   NA
************************************************************************************/
void McuSpiDis(uint8_t num)
{
    if(num == 0)
    {
        PRCM_ClockEnable(CORE_CKG_CTL_CSP1_EN);//开CSP1时钟，避免没有初始化时，调用改接口卡住

        CSP_TxDisable(CSP1);
        CSP_RxDisable(CSP1);
        CSP_Disable(CSP1);
    }
    else if(num == 1)
    {
        //开时钟，避免没有初始化时，调用该接口卡住
        PRCM_ClockEnable(CORE_CKG_CTL_SPI_EN);//SPI时钟

        SPI_TxFifoDisable();
        SPI_RxFifoDisable();
        SPI_Disable();
    }
}

/************************************************************************************
* @brief   CS片选拉低 接口耗时：18us
* @param   num：spi底层硬件类型号，0:CSP1_SPI,1:SPI
* @return   NA
************************************************************************************/
void McuSpiCsReset(uint8_t num)
{
    if ((num == 0) || (num == 1))
    {
        McuGpioWrite(g_cs_pin[num], 0);
    }
}

/************************************************************************************
* @brief   CS片选拉高 接口耗时：18us
* @param   num：spi底层硬件类型号，0:CSP1_SPI,1:SPI
* @return   NA
************************************************************************************/
void McuSpiCsSet(uint8_t num)
{
    if ((num == 0) || (num == 1))
    {
        McuGpioWrite(g_cs_pin[num], 1);
    }
}

/************************************************************************************
* @brief  SPI主机写一帧数据。接口耗时，以写10字节测试，num为0耗时：136.4us,num为1耗时：849.8us
* @param  num：spi底层硬件类型号，0:CSP1_SPI,1:SPI
* @param  cmd：要发送数据的地址
* @param  len：要发送的数据长度
* @return  0：成功。-1：失败
************************************************************************************/
int8_t McuSpiMasterWrite(uint8_t num, uint8_t *cmd, uint16_t len)
{
    if(len == 0)
    {
        return -1;
    }

    if(num == 0)
    {
        //发送数据
        CSP_IntClear(CSP1, CSP_INT_TX_ALLOUT);
        while(len)
        {
            if(!(CSP1->TX_FIFO_STATUS & CSP_TX_FIFO_STATUS_FULL_Msk))
            {
                CSP1->TX_FIFO_DATA = *cmd;
                len--;
                cmd++;
            }
        }

        //等待全部发送完毕
        while(!(CSP1->INT_STATUS & CSP_INT_TX_ALLOUT));
        CSP_IntClear(CSP1, CSP_INT_TX_ALLOUT);

        //只发送数据时发送结束要清空接收FIFO
        CSP_RXFifoClear(CSP1);

        // 根据SPI速率增加1个字符的延时，以保证数据发送完成
        uint32_t rate = 0, onechar_timeout = 0;
        rate = GetlsioFreq()/2/(CSP1->CLOCK_DIVISOR+1);
        onechar_timeout = (uint32_t)((2 * 11 * 1000 * 1000 / rate) + 1); // 按11bit、2字节计算，单位us
        if(onechar_timeout < 50)
        {
            onechar_timeout = 50;
        }
        delay_func_us(onechar_timeout);
    }
    else if(num == 1)
    {
        //发送数据
        while(len)
        {
            if(SPI_TxFifoStatusGet(SPI_FIFO_FULL) == 0)
            {
                uint8_t txfifo_len = SPI_TxFifoStatusGet(SPI_FIFO_DATA_LEN);
                if ((len >= 4) && (txfifo_len <= 124))
                {
                    SPI->TXD = *(uint32_t *)cmd;
                    len -= 4;
                    cmd += 4;
                }
                else if ((len >= 2 && len < 4) && (txfifo_len <= 126))
                {
                    SPI->TXD_16 = *(uint16_t *)cmd;
                    len -= 2;
                    cmd += 2;

                    //SPI主机CLK较低时需要等待发送完成，否则倒数第二个数据会被最后一个数据吃掉，导致数据丢失
                    while(SPI_TxFifoStatusGet(SPI_FIFO_EMPTY) == 0);
                }
                else if ((len == 1) && (txfifo_len <= 127))
                {
                    SPI->TXD_8 = *(uint8_t *)cmd;
                    len--;
                    cmd++;
                }
            }
        }

        //等待TXFIFO为空
        while(SPI_TxFifoStatusGet(SPI_FIFO_EMPTY) == 0);
        
        //只发送数据时发送结束要清空接收FIFO
        SPI_RxFifoReset();
    }
	else
	{
		debug_assert(0);
		return -1;
	}
    return 0;
}

/************************************************************************************
* @brief  SPI主机接收一帧数据
* @param  num：spi底层硬件类型号，0:CSP1_SPI,1:SPI
* @param  rec：接收数据
* @param  len：接收长度
* @return  0：成功。-1：失败
************************************************************************************/
int8_t McuSpiMasterRead(uint8_t num, uint8_t *rec, uint16_t len)
{   
    if(len == 0)
    {
        return -1;
    }

    return McuSpiMasterWriteRead(num, rec, rec, len);
}

/************************************************************************************
* @brief  SPI主机各发送接收一帧数据.接口耗时，以写10字节数据测试，num为0时耗时252.9us,当num为1时耗时：939.2us
* @param  num：spi底层硬件类型号，0:CSP1_SPI,1:SPI
* @param  cmd：发送数据
* @param  rec：接收数据
* @param  len：接收和发送长度
* @return  0：成功。-1：失败
************************************************************************************/
int8_t McuSpiMasterWriteRead(uint8_t num, uint8_t *cmd, uint8_t *rec, uint16_t len)
{
    uint16_t tx_len = len, rx_len = len;
    uint32_t tx_allow = 1; //主机需要使用该标志控制读写顺序，否则容易丢接收数据

    if(len == 0)
    {
        return -1;
    }

    if(num == 0)
    {
        //先发送再接收数据
        while (tx_len || rx_len)
        {
            //发送数据
            if((!(CSP1->TX_FIFO_STATUS & CSP_TX_FIFO_STATUS_FULL_Msk)) && (tx_len > 0) && (tx_allow == 1))
            {
                CSP1->TX_FIFO_DATA = *cmd;
                tx_len--;
                cmd++;
                tx_allow = 0;
            }
            
            //接收数据
            if((!(CSP1->RX_FIFO_STATUS & CSP_RX_FIFO_STATUS_EMPTY_Msk)) && (rx_len > 0) && (tx_allow == 0))
            {
                *rec = (uint8_t)(CSP1->RX_FIFO_DATA);
                rx_len--;
                rec++;
                tx_allow = 1;
            }
        }
    }
    else if(num == 1)
    {
        //先发送再接收数据
        while (tx_len || rx_len)
        {
            //发送数据
            if ((SPI_TxFifoStatusGet(SPI_FIFO_FULL) == 0) && (tx_len > 0) && (tx_allow == 1))
            {
                uint8_t txfifo_len = SPI_TxFifoStatusGet(SPI_FIFO_DATA_LEN);
                if ((tx_len >= 4) && (txfifo_len <= 124))
                {
                    SPI->TXD = *(uint32_t *)cmd;
                    tx_len -= 4;
                    cmd += 4;
                    tx_allow = 0;
                }
                else if ((tx_len >= 2 && tx_len < 4) && (txfifo_len <= 126))
                {
                    SPI->TXD_16 = *(uint16_t *)cmd;
                    tx_len -= 2;
                    cmd += 2;
                    tx_allow = 0;

                    //SPI主机CLK较低时需要等待发送完成，否则倒数第二个数据会被最后一个数据吃掉，导致数据丢失
                    while(SPI_TxFifoStatusGet(SPI_FIFO_EMPTY) == 0);
                    
                }
                else if ((tx_len == 1) && (txfifo_len <= 127))
                {
                    SPI->TXD_8 = *(uint8_t *)cmd;
                    tx_len--;
                    cmd++;
                    tx_allow = 0;
                }
            }

            //接收数据
            if ((SPI_RxFifoStatusGet(SPI_FIFO_EMPTY) == 0) && (rx_len > 0) && (tx_allow == 0))
            {
                uint8_t rxfifo_len = SPI_RxFifoStatusGet(SPI_FIFO_DATA_LEN);
                uint8_t full = SPI_RxFifoStatusGet(SPI_FIFO_FULL);
                if ((rx_len >= 4) && (rxfifo_len >= 4 || full == 1))
                {
                    *(uint32_t *)rec = SPI->RXD;
                    rx_len -= 4;
                    rec += 4;
                    tx_allow = 1;
                }
                else if ((rx_len >= 2 && rx_len < 4) && (rxfifo_len >= 2 || full == 1))
                {
                    *(uint16_t *)rec = SPI->RXD_16;
                    rx_len -= 2;
                    rec += 2;
                    tx_allow = 1;
                }
                else if ((rx_len == 1) && (rxfifo_len >= 1 || full == 1))
                {
                    *(uint8_t *)rec = SPI->RXD_8;
                    rx_len--;
                    rec++;
                    tx_allow = 1;
                }
            }
        }
    }
    else
    {
    	debug_assert(0);
        return -1;
    }

    return 0;
}
