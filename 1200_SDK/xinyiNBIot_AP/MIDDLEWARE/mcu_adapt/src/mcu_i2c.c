#include "mcu_adapt.h"

typedef struct
{
	volatile uint16_t tx_Size;
	volatile uint16_t rx_Size;
    uint8_t rx_buff;
    pFunType_void rx_irq;
    pFunType_void tx_irq;
}mcu_i2c_t;

#define MCU_I2C_NUM (2)

mcu_i2c_t MCU_I2C[MCU_I2C_NUM] = {{0, 0, 0, NULL, NULL}, {0, 0, 0, NULL, NULL}};
//=====================================================================================
//================================I2C==================================================
//=====================================================================================
/************************************************************************************
* @brief  带引脚配置的I2C主机设置。代码段位于RAM的接口耗时：num为0耗时683.8us，num为1耗时666.5us
*                                代码段位于FLASH的接口耗时：num为0耗时1496.9us，num为1耗时944.5us
* @param  num：i2c硬件端口号，可选0-1，分别对应I2C1/I2C2
* @param  freq：频率，单位KHZ，可选100、400
* @param  scl_pin  ：  I2C主机时钟线
* @param  sda_pin  ：  I2C主机数据线
* @return  0：成功。 -1：失败，非法参数
************************************************************************************/
int8_t McuI2cMasterSet2(uint8_t num, uint32_t freq, uint8_t scl_pin, uint8_t sda_pin)
{
    GPIO_InitTypeDef i2c_gpio = {0};
    uint32_t base = 0, pclk = 0, clk_mode = 0;

    if( num>1 )
    {
    	debug_assert(0);
        return -1;
    }

    switch(freq)
    {
        case 100: { clk_mode = I2C_standard_mode; break; }
        case 400: { clk_mode = I2C_fast_mode; break; }
        default: 
        {
			debug_assert(0);
			return -1; //入参非法
        }
    }

    //配置共同点
    PRCM_ClockEnable(CORE_CKG_CTL_GPIO_EN);
    i2c_gpio.Mode = GPIO_MODE_HW_PER;

    if(num == 0)
    {
        i2c_gpio.PinRemap = GPIO_I2C1_SCL;
        i2c_gpio.Pin = scl_pin;
        GPIO_Init(&i2c_gpio, NORMAL_SPEED);
        GPIO_InputPeriSelect(scl_pin, GPIO_I2C1_SCL);
        GPIO_InputPeriSelectCmd(GPIO_I2C1_SCL, ENABLE);

        i2c_gpio.PinRemap = GPIO_I2C1_SDA;
        i2c_gpio.Pin = sda_pin;
        GPIO_Init(&i2c_gpio, NORMAL_SPEED);
        GPIO_InputPeriSelect(sda_pin, GPIO_I2C1_SDA);
        GPIO_InputPeriSelectCmd(GPIO_I2C1_SDA, ENABLE);

        PRCM_I2C1refclkSet(I2C1_REFCLK_SEL_LSioclk);
        PRCM_ClockEnable(CORE_CKG_CTL_I2C1_EN);
        pclk = GetI2c1ClockFreq();
        base = I2C1_BASE;
    }
    else
    {
        i2c_gpio.PinRemap = GPIO_I2C2_SCL;
        i2c_gpio.Pin = scl_pin;
        GPIO_Init(&i2c_gpio, NORMAL_SPEED);
        GPIO_InputPeriSelect(scl_pin, GPIO_I2C2_SCL);
        GPIO_InputPeriSelectCmd(GPIO_I2C2_SCL, ENABLE);

        i2c_gpio.PinRemap = GPIO_I2C2_SDA;
        i2c_gpio.Pin = sda_pin;
        GPIO_Init(&i2c_gpio, NORMAL_SPEED);
        GPIO_InputPeriSelect(sda_pin, GPIO_I2C2_SDA);
        GPIO_InputPeriSelectCmd(GPIO_I2C2_SDA, ENABLE);

        PRCM_I2C1refclkSet(I2C2_REFCLK_SEL_LSioclk);
        PRCM_ClockEnable(CORE_CKG_CTL_I2C2_EN);
        pclk = GetI2c2ClockFreq();
        base = I2C2_BASE;
    }

	//配置I2C
	I2C_Init((I2C_TypeDef *)base, I2C_master_normal_noStartByte, clk_mode, I2C_addr_7b, pclk);
	I2C_SetDelayBetweenByte((I2C_TypeDef *)base, 0, 0);//关闭字节间延时

    return 0;
}

/************************************************************************************
* @brief  I2C选用的端口配置。接口耗时：num为0耗时683.8us，num为1耗时666.5us
* @param  num：i2c硬件端口号，可选0-1，分别对应I2C1/I2C2
* @param  freq：频率，单位KHZ，可选100、400
* @return  0：成功。 -1：失败，非法参数
************************************************************************************/
int8_t McuI2cMasterSet(uint8_t num, uint32_t freq)
{
    switch(num)
    {
        //I2C1，普通I2C，深睡不保持，参考时钟不受启停CP影响
        case 0:{ return McuI2cMasterSet2(num,freq,MCU_GPIO36,MCU_GPIO35); }
        //I2C2，普通I2C，深睡不保持，参考时钟不受启停CP影响
        case 1:{ return McuI2cMasterSet2(num,freq,MCU_GPIO21,MCU_GPIO24); }
        default: return -1;
    }
}

/************************************************************************************
* @brief  I2C去初始化。
* @param  num：i2c硬件端口号，可选0-1，分别对应I2C1/I2C2
* @return  0：成功。-1：失败，非法参数
************************************************************************************/
int8_t McuI2cDis(uint8_t num)
{
    if (num == 0)
    {
        PRCM_I2C1refclkSet(I2C1_REFCLK_SEL_LSioclk);
		PRCM_ClockEnable(CORE_CKG_CTL_I2C1_EN);
        I2C_DeInit((I2C_TypeDef *)I2C1_BASE);
        PRCM_ClockDisable(CORE_CKG_CTL_I2C1_EN);
    }
    else if (num == 1)
    {
        PRCM_I2C2refclkSet(I2C2_REFCLK_SEL_LSioclk);
		PRCM_ClockEnable(CORE_CKG_CTL_I2C2_EN);
        I2C_DeInit((I2C_TypeDef *)I2C2_BASE);
        PRCM_ClockDisable(CORE_CKG_CTL_I2C2_EN);
    }
    else
    {
        return -1;
    }
    return 0;
}

/************************************************************************************
* @brief  I2C主机写。接口内直接操作寄存器，耗时与外接i2c设备类型有关，时长不确定。
* @param  num：i2c硬件端口号，可选0-1，分别对应I2C1/I2C2
* @param  device_addr: 器件地址
* @param  addr_cnt   : 数据地址个数，如：24C16为1个，24C512为2个
* @param  addr       : 数据地址
* @param  pdata      : 待写数据缓冲首地址
* @param  len        : 长度
* @return  0：成功。-1：失败
************************************************************************************/
__RAM_FUNC int8_t McuI2cMasterWrite(uint8_t num, uint8_t device_addr, uint8_t addr_cnt, uint32_t addr, const uint8_t *pdata, uint16_t len, uint32_t Timeout)
{
    int8_t retval = 0;
    uint32_t base = 0;
    uint32_t tickstart = 0;

    if( num>1 )
    {
        return -1;
    }

    if(addr_cnt!=1 && addr_cnt!=2)
    {
    	debug_assert(0);
        return -1;
    }

    if(len == 0)
    {
        return -1;
    }

    if(num == 0)
    {
        base = I2C1_BASE;
    }
    else
    {
        base = I2C2_BASE;
    }

    //获取通信开始时间
    tickstart = Get_Tick();

    //如果tx fifo还有数据就复位i2c
    if (0 == (I2C_Status((I2C_TypeDef *)base) & I2C_STAT_TFE_Msk))
    {
        I2C_Reset((I2C_TypeDef *)base);
    }
    
    //设置目标从机地址，设置完成后I2C开始寻址直到寻到从机
    I2C_SetAddr((I2C_TypeDef *)base, device_addr>>1);

    //发送I2C存储器地址
    while(addr_cnt)
    {
        if (I2C_Status((I2C_TypeDef *)base) & I2C_STAT_TFNF_Msk)
        {
            if(addr_cnt == 2)
            {
                ((I2C_TypeDef *)base)->CMD_DATA = (uint8_t)((uint16_t)(((uint16_t)(addr & (uint16_t)0xFF00)) >> 8));
            }
            else
            {
                ((I2C_TypeDef *)base)->CMD_DATA = (uint8_t)((uint16_t)(addr & (uint16_t)0x00FF));
            }
            addr_cnt--;
        }
        else if(Check_Ms_Timeout(tickstart,Timeout))
        {
            retval = -1;
            goto error;
        }
    }

    //发送数据
    while (len)
	{
		if (I2C_Status((I2C_TypeDef *)base) & I2C_STAT_TFNF_Msk)
        {
            if (len > 1)
            {
                ((I2C_TypeDef *)base)->CMD_DATA = *pdata;
            }
            else
            {
                ((I2C_TypeDef *)base)->CMD_DATA = (*pdata | 0x0200);//最后1字节数据要产生停止条件
            }
            len--;
            pdata++;
        }
        else if(Check_Ms_Timeout(tickstart,Timeout))
        {
            retval = -1;
            goto error;
        }
	}

    //等待产生停止条件，等待TXFIFO变为空
	while (!(((I2C_TypeDef *)base)->INT_STAT & I2C_INT_COMP) || (!(I2C_Status((I2C_TypeDef *)base) & I2C_STAT_TFE_Msk)))
    {
        if(Check_Ms_Timeout(tickstart,Timeout))
        {
            retval = -1;
            goto error;
        }
    }
    I2C_IntClear((I2C_TypeDef *)base, I2C_INT_COMP);

error:
    if (retval == -1)
    {
        // 如果发生总线仲裁中断，复位i2c
        if (((I2C_TypeDef *)base)->INT_STAT & I2C_INT_ARB_LOST)
        {
            I2C_Reset((I2C_TypeDef *)base);
            I2C_IntClear((I2C_TypeDef *)base, I2C_INT_ARB_LOST);
        }

        // 如果发生时钟超时中断（目前配置，通信过程中，SCK拉低超过5ms会触发），复位i2c
        if (((I2C_TypeDef *)base)->INT_STAT & I2C_INT_TIMEOUT)
        {
            I2C_Reset((I2C_TypeDef *)base);
            I2C_IntClear((I2C_TypeDef *)base, I2C_INT_TIMEOUT);
        }
    }

    return retval;
}

/************************************************************************************
* @brief  I2C主机直写。接口内直接操作寄存器，耗时与外接i2c设备类型有关，时长不确定。
* @param  num：i2c硬件端口号，可选0-1，分别对应I2C1/I2C2
* @param  device_addr: 器件地址
* @param  pdata      : 待写数据缓冲首地址
* @param  len        : 长度
* @return  0：成功。-1：失败
************************************************************************************/
__RAM_FUNC int8_t McuI2cMasterDirectWrite(uint8_t num, uint8_t device_addr, const uint8_t *pdata, uint16_t len, uint32_t Timeout)
{
    int8_t retval = 0;
    uint32_t base = 0;
    uint32_t tickstart = 0;

    if( num>1 )
    {
        return -1;
    }

    if(len == 0)
    {
        return -1;
    }

    if(num == 0)
    {
        base = I2C1_BASE;
    }
    else
    {
        base = I2C2_BASE;
    }

    //获取通信开始时间
    tickstart = Get_Tick();

    //如果tx fifo还有数据就复位i2c
    if (0 == (I2C_Status((I2C_TypeDef *)base) & I2C_STAT_TFE_Msk))
    {
        I2C_Reset((I2C_TypeDef *)base);
    }
    
    //设置目标从机地址，设置完成后I2C开始寻址直到寻到从机
    I2C_SetAddr((I2C_TypeDef *)base, device_addr>>1);

    //发送数据
    while (len)
	{
		if (I2C_Status((I2C_TypeDef *)base) & I2C_STAT_TFNF_Msk)
        {
            if (len > 1)
            {
                ((I2C_TypeDef *)base)->CMD_DATA = *pdata;
            }
            else
            {
                ((I2C_TypeDef *)base)->CMD_DATA = (*pdata | 0x0200);//最后1字节数据要产生停止条件
            }
            len--;
            pdata++;
        }
        else if(Check_Ms_Timeout(tickstart,Timeout))
        {
            retval = -1;
            goto error;
        }
	}

    //等待产生停止条件，等待TXFIFO变为空
	while (!(((I2C_TypeDef *)base)->INT_STAT & I2C_INT_COMP) || (!(I2C_Status((I2C_TypeDef *)base) & I2C_STAT_TFE_Msk)))
    {
        if(Check_Ms_Timeout(tickstart,Timeout))
        {
            retval = -1;
            goto error;
        }
    }
    I2C_IntClear((I2C_TypeDef *)base, I2C_INT_COMP);

error:
    if (retval == -1)
    {
        // 如果发生总线仲裁中断，复位i2c
        if (((I2C_TypeDef *)base)->INT_STAT & I2C_INT_ARB_LOST)
        {
            I2C_Reset((I2C_TypeDef *)base);
            I2C_IntClear((I2C_TypeDef *)base, I2C_INT_ARB_LOST);
        }

        // 如果发生时钟超时中断（目前配置，通信过程中，SCK拉低超过5ms会触发），复位i2c
        if (((I2C_TypeDef *)base)->INT_STAT & I2C_INT_TIMEOUT)
        {
            I2C_Reset((I2C_TypeDef *)base);
            I2C_IntClear((I2C_TypeDef *)base, I2C_INT_TIMEOUT);
        }
    }

    return retval;
}

/************************************************************************************
* @brief  I2C主机读。接口内直接操作寄存器，耗时与外接i2c设备类型有关，时长不确定。
* @param  num：i2c硬件端口号，可选0-1，分别对应I2C1/I2C2
* @param  device_addr: 器件地址
* @param  addr_cnt   : 数据地址个数，如：24C16为1个，24C512为2个
* @param  addr       : 数据地址
* @param  pdata      : 待写数据缓冲首地址
* @param  len        : 长度
* @return  0：成功。-1：失败
************************************************************************************/
__RAM_FUNC int8_t McuI2cMasterRead(uint8_t num, uint8_t device_addr, uint8_t addr_cnt, uint32_t addr, uint8_t *pdata, uint16_t len, uint32_t Timeout)
{
    int8_t retval = 0;
    uint32_t base = 0;
    uint32_t tickstart = 0;
    
    if( num>1 )
    {
        return -1;
    }

    if(addr_cnt!=1 && addr_cnt!=2)
    {
        return -1;
    }

    if(len == 0)
    {
        return -1;
    }

    if(num == 0)
    {
        base = I2C1_BASE;
    }
    else
    {
        base = I2C2_BASE;
    }

    //获取通信开始时间
    tickstart = Get_Tick();

    //如果rx fifo还有数据就复位i2c
    if (I2C_Status((I2C_TypeDef *)base) & I2C_STAT_RFNE_Msk)
    {
        I2C_Reset((I2C_TypeDef *)base);
    }
    
    //设置目标从机地址，设置完成后I2C开始寻址直到寻到从机
    I2C_SetAddr((I2C_TypeDef *)base, device_addr>>1);

    //发送I2C存储器地址
    while(addr_cnt)
    {
        if (I2C_Status((I2C_TypeDef *)base) & I2C_STAT_TFNF_Msk)
        {
            if(addr_cnt == 2)
            {
                ((I2C_TypeDef *)base)->CMD_DATA = (uint8_t)((uint16_t)(((uint16_t)(addr & (uint16_t)0xFF00)) >> 8));
            }
            else
            {
                ((I2C_TypeDef *)base)->CMD_DATA = (uint8_t)((uint16_t)(addr & (uint16_t)0x00FF));
            }
            addr_cnt--;
        }
        else if(Check_Ms_Timeout(tickstart,Timeout))
        {
            retval = -1;
            goto error;
        }
    }

    //发送dummy以接收数据
    while (len)
	{
		if ((I2C_Status((I2C_TypeDef *)base) & I2C_STAT_TFNF_Msk))
        {
            if (len > 1)
            {
                ((I2C_TypeDef *)base)->CMD_DATA = 0x0100;
            }
            else
            {
                ((I2C_TypeDef *)base)->CMD_DATA = 0x0300;//最后1字节数据要产生停止条件
            }

            while ((!(I2C_Status((I2C_TypeDef *)base) & I2C_STAT_RFNE_Msk)))
            {
                if(Check_Ms_Timeout(tickstart,Timeout))
                {
                    retval = -1;
                    goto error;
                }
            }
            *pdata = (uint8_t)(((I2C_TypeDef *)base)->CMD_DATA);
            len--;
            pdata++;
        }
        else if(Check_Ms_Timeout(tickstart,Timeout))
        {
            retval = -1;
            goto error;
        }
	}

    //等待产生停止条件，等待TXFIFO变为空
	while (!(((I2C_TypeDef *)base)->INT_STAT & I2C_INT_COMP) || (!(I2C_Status((I2C_TypeDef *)base) & I2C_STAT_TFE_Msk)))
    {
        if(Check_Ms_Timeout(tickstart,Timeout))
        {
            retval = -1;
            goto error;
        }
    }
    I2C_IntClear((I2C_TypeDef *)base, I2C_INT_COMP);

error:
    if (retval == -1)
    {
        // 如果发生总线仲裁中断，复位i2c
        if (((I2C_TypeDef *)base)->INT_STAT & I2C_INT_ARB_LOST)
        {
            I2C_Reset((I2C_TypeDef *)base);
            I2C_IntClear((I2C_TypeDef *)base, I2C_INT_ARB_LOST);
        }

        // 如果发生时钟超时中断（目前配置，通信过程中，SCK拉低超过5ms会触发），复位i2c
        if (((I2C_TypeDef *)base)->INT_STAT & I2C_INT_TIMEOUT)
        {
            I2C_Reset((I2C_TypeDef *)base);
            I2C_IntClear((I2C_TypeDef *)base, I2C_INT_TIMEOUT);
        }
    }

    return retval;
}

/************************************************************************************
* @brief  I2C主机直读。接口内直接操作寄存器，耗时与外接i2c设备类型有关，时长不确定。
* @param  num：i2c硬件端口号，可选0-1，分别对应I2C1/I2C2
* @param  device_addr: 器件地址
* @param  pdata      : 待写数据缓冲首地址
* @param  len        : 长度
* @return  0：成功。-1：失败
************************************************************************************/
__RAM_FUNC int8_t McuI2cMasterDirectRead(uint8_t num, uint8_t device_addr, uint8_t *pdata, uint16_t len, uint32_t Timeout)
{
    int8_t retval = 0;
    uint32_t base = 0;
    uint32_t tickstart = 0;
    
    if( num>1 )
    {
        return -1;
    }

    if(len == 0)
    {
        return -1;
    }

    if(num == 0)
    {
        base = I2C1_BASE;
    }
    else
    {
        base = I2C2_BASE;
    }

    //获取通信开始时间
    tickstart = Get_Tick();

    //如果rx fifo还有数据就复位i2c
    if (I2C_Status((I2C_TypeDef *)base) & I2C_STAT_RFNE_Msk)
    {
        I2C_Reset((I2C_TypeDef *)base);
    }
    
    //设置目标从机地址，设置完成后I2C开始寻址直到寻到从机
    I2C_SetAddr((I2C_TypeDef *)base, device_addr>>1);

    //发送dummy以接收数据
    while (len)
	{
		if ((I2C_Status((I2C_TypeDef *)base) & I2C_STAT_TFNF_Msk))
        {
            if (len > 1)
            {
                ((I2C_TypeDef *)base)->CMD_DATA = 0x0100;
            }
            else
            {
                ((I2C_TypeDef *)base)->CMD_DATA = 0x0300;//最后1字节数据要产生停止条件
            }

            while ((!(I2C_Status((I2C_TypeDef *)base) & I2C_STAT_RFNE_Msk)))
            {
                if(Check_Ms_Timeout(tickstart,Timeout))
                {
                    retval = -1;
                    goto error;
                }
            }
            *pdata = (uint8_t)(((I2C_TypeDef *)base)->CMD_DATA);
            len--;
            pdata++;
        }
        else if(Check_Ms_Timeout(tickstart,Timeout))
        {
            retval = -1;
            goto error;
        }
	}

    //等待产生停止条件，等待TXFIFO变为空
	while (!(((I2C_TypeDef *)base)->INT_STAT & I2C_INT_COMP) || (!(I2C_Status((I2C_TypeDef *)base) & I2C_STAT_TFE_Msk)))
    {
        if(Check_Ms_Timeout(tickstart,Timeout))
        {
            retval = -1;
            goto error;
        }
    }
    I2C_IntClear((I2C_TypeDef *)base, I2C_INT_COMP);

error:
    if (retval == -1)
    {
        // 如果发生总线仲裁中断，复位i2c
        if (((I2C_TypeDef *)base)->INT_STAT & I2C_INT_ARB_LOST)
        {
            I2C_Reset((I2C_TypeDef *)base);
            I2C_IntClear((I2C_TypeDef *)base, I2C_INT_ARB_LOST);
        }

        // 如果发生时钟超时中断（目前配置，通信过程中，SCK拉低超过5ms会触发），复位i2c
        if (((I2C_TypeDef *)base)->INT_STAT & I2C_INT_TIMEOUT)
        {
            I2C_Reset((I2C_TypeDef *)base);
            I2C_IntClear((I2C_TypeDef *)base, I2C_INT_TIMEOUT);
        }
    }

    return retval;
}

/**
 * @brief 根据XferSize设置rxfifo trigger level，并发送dummy数据以触发RXFIFO阈值中断，同时判断是否设置停止条件
 * @param hi2c 详情参考 @ref HAL_I2C_HandleTypeDef.
 */
__RAM_FUNC static void I2C_SetRxFiFoThreshold_by_XferSize(uint8_t num)
{
    uint32_t base = 0;

#define RXFIFO_1_2_DEPTH   (16) //RXFIFO 1/2深度（字节数）

	switch (num)
	{
		case 0: { base = I2C1_BASE; break; }
		case 1: { base = I2C2_BASE; break; }
		default: break;
	}

    if(MCU_I2C[num].rx_Size)
    {
        uint8_t dummy_write_size = 0;
        uint8_t set_stop = 0;
        if(MCU_I2C[num].rx_Size > RXFIFO_1_2_DEPTH)
        {
            set_stop = 0;
            dummy_write_size = RXFIFO_1_2_DEPTH;
        }
        else
        {
            set_stop = 1;
            dummy_write_size = MCU_I2C[num].rx_Size;
        }

        //RXFIFO存量数据大于等于dummy_write_size个字节时触发阈值中断
        I2C_SetRxFiFoThreshold((I2C_TypeDef *)base, dummy_write_size);

        //I2C主机发送dummy_write_size个dummy数据使RXFIFO收到数据，以触发RXFIFO阈值中断，同时判断是否设置停止条件
        while(dummy_write_size > 0)
        {
            if ((I2C_Status((I2C_TypeDef *)base) & I2C_STAT_TFNF_Msk) == I2C_STAT_TFNF_Msk)
            {
                if(set_stop == 0)
                {
                	I2C_PutData((I2C_TypeDef *)base, 0x0100);
                }
                else
                {
                    (dummy_write_size > 1) ? \
					I2C_PutData((I2C_TypeDef *)base, 0x0100) : \
					I2C_PutData((I2C_TypeDef *)base, 0x0300); // 最后1字节数据产生停止条件
                }
                dummy_write_size--;
            }
        }
    }
}

__RAM_FUNC void McuI2cMasterRead_It(uint8_t num, uint8_t device_addr, uint8_t addr_cnt, uint32_t addr, const uint8_t *pdata, uint16_t len, uint32_t Timeout)
{
	uint32_t base = 0;
    (void)pdata;
    (void)Timeout;

    if( num>1 )
	{
		return;
	}

	if(len == 0)
	{
		return;
	}

	switch (num)
	{
		case 0: { base = I2C1_BASE; break; }
		case 1: { base = I2C2_BASE; break; }
		default: break;
	}

	MCU_I2C[num].rx_Size = len;

    //设置目标从机地址，设置完成后I2C开始寻址直到寻到从机
    I2C_SetAddr((I2C_TypeDef *)base, device_addr>>1);

    //发送I2C存储器地址
	while(addr_cnt)
	{
		if (I2C_Status((I2C_TypeDef *)base) & I2C_STAT_TFNF_Msk)
		{
			if(addr_cnt == 2)
				I2C_PutData((I2C_TypeDef *)base, (uint8_t)((uint16_t)(((uint16_t)(addr & (uint16_t)0xFF00)) >> 8)));
			else
				I2C_PutData((I2C_TypeDef *)base, (uint8_t)((uint16_t)(addr & (uint16_t)0x00FF)));
			addr_cnt--;
		}
	}

    //根据RxXferSize设置rxfifo trigger level，并发送dummy数据以触发RXFIFO阈值中断，同时判断是否设置停止条件
    I2C_SetRxFiFoThreshold_by_XferSize(num);
}

__RAM_FUNC void McuI2cMasterWrite_It(uint8_t num, uint8_t device_addr, uint8_t addr_cnt, uint32_t addr, const uint8_t *pdata, uint16_t len, uint32_t Timeout)
{
    uint32_t base = 0;
    (void)Timeout;

    if( num>1 )
    {
        return;
    }

    if(len == 0)
    {
        return;
    }

	switch (num)
	{
		case 0: { base = I2C1_BASE; break; }
		case 1: { base = I2C2_BASE; break; }
		default: break;
	}

	MCU_I2C[num].tx_Size = len;

    //设置目标从机地址，设置完成后I2C开始寻址直到寻到从机
    I2C_SetAddr((I2C_TypeDef *)base, device_addr>>1);

    //传输地址会触发tx阈值中断
    I2C_IntDisable((I2C_TypeDef *)base, I2C_INT_TX_AE);
    //发送I2C存储器地址
	while(addr_cnt)
	{
		if (I2C_Status((I2C_TypeDef *)base) & I2C_STAT_TFNF_Msk)
		{
			if(addr_cnt == 2)
				I2C_PutData((I2C_TypeDef *)base, (uint8_t)((uint16_t)(((uint16_t)(addr & (uint16_t)0xFF00)) >> 8)));
			else
				I2C_PutData((I2C_TypeDef *)base, (uint8_t)((uint16_t)(addr & (uint16_t)0x00FF)));
			addr_cnt--;
		}
	}

    //设置TXFIFO阈值为0字节，当TXFIFO现存量小于等于阈值时触发TXFIFO阈值中断
	while(!(I2C_Status((I2C_TypeDef *)base) & I2C_STAT_TFE_Msk));
	I2C_IntClear((I2C_TypeDef *)base, I2C_INT_TX_AE);
    I2C_IntEnable((I2C_TypeDef *)base, I2C_INT_TX_AE);

    //发送数据以触发TXFIFO阈值中断
    (MCU_I2C[num].tx_Size == 1) ? \
    I2C_PutData((I2C_TypeDef *)base, (*(pdata) | 0x0200)): \
	I2C_PutData((I2C_TypeDef *)base, *(pdata)); // 最后1字节数据产生停止条件
}

/************************************************************************************
* @brief  串口接收中断标志清零
* @param  num：串口号，可选0-3
* @return  NA
************************************************************************************/
static void McuI2cRxIrqClr(uint8_t num)
{
    uint32_t base = 0;

	switch (num)
	{
		case 0: { base = I2C1_BASE; break; }
		case 1: { base = I2C2_BASE; break; }
		default: break;
	}

	I2C_IntClear((I2C_TypeDef *)base, I2C_INT_RX_AF);
}
/************************************************************************************
* @brief  串口发送中断标志清零
* @param  num：串口号，可选0-3
* @return  NA
************************************************************************************/
static void McuI2cTxIrqClr(uint8_t num)
{
    uint32_t base = 0;

	switch (num)
	{
		case 0: { base = I2C1_BASE; break; }
		case 1: { base = I2C2_BASE; break; }
		default: break;
	}

	I2C_IntClear((I2C_TypeDef *)base, I2C_INT_TX_AE);
}
/************************************************************************************
* @brief  串口中断标志查询
* @param  num：串口号，可选0-3
* @return 0x00：无中断产生  0x01：有发送中断产生  0x02：有接收中断产生。（可以为组合值）
************************************************************************************/
static uint8_t McuI2cIrqRead(uint8_t num)
{
    uint32_t base = 0, intsta = 0;
    uint8_t ret = 0;

	switch (num)
	{
		case 0: { base = I2C1_BASE; break; }
		case 1: { base = I2C2_BASE; break; }
		default: break;
	}

	intsta = I2C_IntStatus((I2C_TypeDef *)base);
	if(intsta & I2C_INT_TX_AE)
	{
		ret |= 0x01;
	}
	if(intsta & (I2C_INT_RX_AF))
	{
		ret |= 0x02;
	}

    return ret;
}
/************************************************************************************
* @brief  读取串口数据接收寄存器
* @param  num：串口号，可选0-3
* @param  out_byte：输出数据
* @return  0：无数据。1：有数据
************************************************************************************/
__RAM_FUNC static uint8_t McuI2cReadAll(uint8_t num, uint8_t *out_byte)
{
    uint32_t base = 0;
    int32_t rxdata = 0;

	switch (num)
	{
		case 0: { base = I2C1_BASE; break; }
		case 1: { base = I2C2_BASE; break; }
		default: break;
	}

	//获取数据
	rxdata = I2C_GetDataNonBlocking((I2C_TypeDef *)base);
	if(rxdata == -1)
	{
		return 0;
	}
	else
	{
		*out_byte = rxdata & 0xFF;
		return 1;
	}

    return 0;
}
/************************************************************************************
* @brief  读取串口的一字节数据。接口耗时：num为0耗时5.4us，num为1耗时5.6us，num为2耗时5.6us，num为3耗时5.6us
* @param  num：串口号，可选0-3;0：LPUART;1:UART;2:CSP2_UART;3:CSP3_UART
* @param  out_byte：输出数据
* @return 0：无数据。1：有数据
************************************************************************************/
__RAM_FUNC uint8_t McuI2cRead(uint8_t num, uint8_t *out_byte)
{
    //如果注册了中断回调函数，则通过句柄MCU_I2C获取数据
    if (MCU_I2C[num].rx_irq != NULL)
    {
        *out_byte = MCU_I2C[num].rx_buff;
        return 1;
    }
    //否则为轮询方式，从寄存器获取数据
    else
    {
        return McuI2cReadAll(num, out_byte);
    }
}

__RAM_FUNC void McuI2cWrite(uint8_t num, uint16_t data)
{
	uint32_t base = 0;

	if( num>1 )
	{
		debug_assert(0);
		return;
	}

	switch (num)
	{
		case 0: { base = I2C1_BASE; break; }
		case 1: { base = I2C2_BASE; break; }
		default: break;
	}

	I2C_PutData((I2C_TypeDef *)base, data);
}

static void I2c0IrqHandle(void)
{
    volatile uint8_t irq_sta;
    uint8_t num = 0;

	irq_sta = McuI2cIrqRead(num);

    // 接收中断产生，且接收中断回调函数指针非空，则执行回调
    if((irq_sta & 0x02) && (MCU_I2C[num].rx_irq != NULL))
    {
    	McuI2cRxIrqClr(num);
        while(McuI2cReadAll(num,&MCU_I2C[num].rx_buff))//读空本次RXFIFO
        {
        	MCU_I2C[num].rx_irq(); //每字节回调一次
        	if(MCU_I2C[num].rx_Size > 0)
        		MCU_I2C[num].rx_Size--;
        }
        I2C_SetRxFiFoThreshold_by_XferSize(num);
    }

    // 发送中断产生，且发送中断回调函数指针非空，则执行回调
    if((irq_sta & 0x01) && (MCU_I2C[num].tx_irq != NULL))
    {
    	McuI2cTxIrqClr(num);

		if(MCU_I2C[num].tx_Size > 0) //防止rx产生的txfifo空中断
		{
			while((I2C_Status((I2C_TypeDef *)I2C1_BASE) & I2C_STAT_TFNF_Msk))
			{
				if(MCU_I2C[num].tx_Size == 2) //倒数第二个数据已经写到fifo，等待fifo空，再进行最后一个数据传输
				{
					if(!(I2C_Status((I2C_TypeDef *)I2C1_BASE) & I2C_STAT_TFE_Msk)) //等待前面数据全部传输完成，才传输最后一个数据
						break;
				}
				if(MCU_I2C[num].tx_Size > 0)
					MCU_I2C[num].tx_Size--;
				MCU_I2C[num].tx_irq();
				if(MCU_I2C[num].tx_Size == 1) //倒数第一个数据已经写到fifo
				{
					while(!(I2C_Status((I2C_TypeDef *)I2C1_BASE) & I2C_STAT_TFE_Msk));//等待最后一个数据全部传输完成
					break;
				}
				else if(MCU_I2C[num].tx_Size == 0) //最后一个数据已经从fifo传出
				{
					break;
				}
			}
		}
    }
}
/************************************************************************************
 * @brief  LPUART中断服务函数
 * @return NA
************************************************************************************/
static void I2c1IrqHandle(void)
{
    uint8_t irq_sta;
    uint8_t num = 1;

	irq_sta = McuI2cIrqRead(num);

    // 接收中断产生，且接收中断回调函数指针非空，则执行回调
    if((irq_sta & 0x02) && (MCU_I2C[num].rx_irq != NULL))
    {
    	McuI2cRxIrqClr(num);
        while(McuI2cReadAll(num,&MCU_I2C[num].rx_buff))//读空本次RXFIFO
        {
        	MCU_I2C[num].rx_irq(); //每字节回调一次
        	if(MCU_I2C[num].rx_Size > 0)
        		MCU_I2C[num].rx_Size--;
        }
        I2C_SetRxFiFoThreshold_by_XferSize(num);
    }

    // 发送中断产生，且发送中断回调函数指针非空，则执行回调
    if((irq_sta & 0x01) && (MCU_I2C[num].tx_irq != NULL))
    {
    	McuI2cTxIrqClr(num);

		if(MCU_I2C[num].tx_Size > 0) //防止rx产生的txfifo空中断
		{
			while((I2C_Status((I2C_TypeDef *)I2C2_BASE) & I2C_STAT_TFNF_Msk))
			{
				if(MCU_I2C[num].tx_Size == 2) //倒数第二个数据已经写到fifo，等待fifo空，再进行最后一个数据传输
				{
					if(!(I2C_Status((I2C_TypeDef *)I2C2_BASE) & I2C_STAT_TFE_Msk))
						break;
				}
				if(MCU_I2C[num].tx_Size > 0)
					MCU_I2C[num].tx_Size--;
				MCU_I2C[num].tx_irq();
				if(MCU_I2C[num].tx_Size == 1) //倒数第一个数据已经写到fifo
				{
					while(!(I2C_Status((I2C_TypeDef *)I2C2_BASE) & I2C_STAT_TFE_Msk));//等待最后一个数据全部传输完成
					break;
				}
				else if(MCU_I2C[num].tx_Size == 0) //最后一个数据已经从fifo传出
				{
					break;
				}
			}
		}
    }
}
/************************************************************************************
* @brief  串口接收中断函数注册。接口耗时：num为0耗时43.6us，num为1耗时31us，num为2耗时28.4us，num为3耗时28.7us
* @param  num：串口号，可选0-3;0：LPUART;1:UART;2:CSP2_UART;3:CSP3_UART
* @param  p_fun：串口接收回调函数
* @return NA
************************************************************************************/
__FLASH_FUNC void McuI2cRxIrqReg(uint8_t num,pFunType_void p_fun)
{
    uint32_t base = 0;

    if (num >= MCU_I2C_NUM) xy_assert(0);

    //注册用户中断回调函数
    MCU_I2C[num].rx_irq = p_fun;
    mark_dyn_addr(&MCU_I2C[num].rx_irq);

	switch (num)
	{
		case 0: { base = I2C1_BASE; break; }
		case 1: { base = I2C2_BASE; break; }
		default:;
	}

    //RXFIFO存量数据大于等于dummy_write_size个字节时触发阈值中断
    I2C_SetRxFiFoThreshold((I2C_TypeDef *)base, 32);

    //注册中断向量
	if(num == 0)
	{
		 NVIC_IntRegister(I2C1_IRQn, I2c0IrqHandle, 1);
	}
	else
	{
		NVIC_IntRegister(I2C2_IRQn, I2c1IrqHandle, 1);
	}

	//清除所有中断标志位
	I2C_IntClear((I2C_TypeDef *)base, I2C_INT_RX_AF);
    //使能中断源
	I2C_IntEnable((I2C_TypeDef *)base, I2C_INT_RX_AF);
}

/************************************************************************************
* @brief  串口接收中断函数注册。接口耗时：num为0耗时43.6us，num为1耗时31us，num为2耗时28.4us，num为3耗时28.7us
* @param  num：串口号，可选0-3;0：LPUART;1:UART;2:CSP2_UART;3:CSP3_UART
* @param  p_fun：串口接收回调函数
* @return NA
************************************************************************************/
__FLASH_FUNC void McuI2cRxIrqUnReg(uint8_t num)
{
    uint32_t base = 0;

    if (num >= MCU_I2C_NUM) xy_assert(0);

    //注册用户中断回调函数
    MCU_I2C[num].rx_irq = NULL;
    mark_dyn_addr(&MCU_I2C[num].rx_irq);

	switch (num)
	{
		case 0: { base = I2C1_BASE; break; }
		case 1: { base = I2C2_BASE; break; }
		default:;
	}

	//清除所有中断标志位
	I2C_IntClear((I2C_TypeDef *)base, I2C_INT_RX_AF);
    //使能中断源
	I2C_IntDisable((I2C_TypeDef *)base, I2C_INT_RX_AF);
}

/************************************************************************************
* @brief  串口发送中断函数注册。接口耗时：num为0耗时19.4us，num为1耗时18us，num为2耗时18.4us，num为3耗时18.8us
* @param  num：串口号，可选0-3. 0：LPUART;1:UART;2:CSP2_UART;3:CSP3_UART
* @param  p_fun：串口发送回调函数
* @return NA
************************************************************************************/
__FLASH_FUNC void McuI2cTxIrqReg(uint8_t num,pFunType_void p_fun)
{
    uint32_t base = 0;

    if (num >= MCU_I2C_NUM) xy_assert(0);

    //注册用户中断回调函数
    MCU_I2C[num].tx_irq = p_fun;
    mark_dyn_addr(&MCU_I2C[num].tx_irq);

	switch (num)
	{
		case 0: { base = I2C1_BASE; break; }
		case 1: { base = I2C2_BASE; break; }
		default:;
	}

    //设置TXFIFO阈值为0字节，当TXFIFO现存量小于等于阈值时触发TXFIFO阈值中断
	I2C_SetTxFiFoThreshold((I2C_TypeDef *)base, 0);

    //注册中断向量
	if(num == 0)
	{
		 NVIC_IntRegister(I2C1_IRQn, I2c0IrqHandle, 1);
	}
	else
	{
		NVIC_IntRegister(I2C2_IRQn, I2c1IrqHandle, 1);
	}

	//清除所有中断标志位
	I2C_IntClear((I2C_TypeDef *)base, I2C_INT_TX_AE);

	//开启TXFIFO空中断
	I2C_IntEnable((I2C_TypeDef *)base, I2C_INT_TX_AE);
}

__FLASH_FUNC void McuI2cTxIrqUnReg(uint8_t num)
{
    uint32_t base = 0;

    if (num >= MCU_I2C_NUM) xy_assert(0);

    //注册用户中断回调函数
    MCU_I2C[num].tx_irq = NULL;
    mark_dyn_addr(&MCU_I2C[num].tx_irq);

	switch (num)
	{
		case 0: { base = I2C1_BASE; break; }
		case 1: { base = I2C2_BASE; break; }
		default:;
	}

	//清除所有中断标志位
	I2C_IntClear((I2C_TypeDef *)base, I2C_INT_TX_AE);

	//开启TXFIFO空中断
	I2C_IntDisable((I2C_TypeDef *)base, I2C_INT_TX_AE);
}
