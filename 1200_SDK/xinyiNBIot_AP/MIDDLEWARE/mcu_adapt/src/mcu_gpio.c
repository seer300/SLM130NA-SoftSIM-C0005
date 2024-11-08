#include "mcu_adapt.h"

pFunType_void mcu_gpio_irq_list[MCU_GPIO_NUM] = {NULL};
GPI_Irq_t mcu_gpi_irq_list[MCU_GPI_NUM] = {{0,NULL}};
GPIOWkp_Irq_t mcu_wkup_irq_list[MCU_GPIOWKP_NUM] = {{0,NULL}};
pFunType_void p_Wkupen_WakeupCallback = NULL;

/*****************************************************************************************************
* @brief  设置IO驱动能力。接口耗时：16.7us。
* @param  num：GPIO端口号可选MCU_GPIO0-63，详情参考 @ref MCU_GPIO_PinTypeDef
* @param  drvstrength：drvstrength对于不同的VDDIO参考电压代表不同的GPIO驱动能力，具体如下。
  一、当VDDIO参考电压为3V时，drvstrength有如下含义：
    0：GPIO驱动能力约为7mA    1：GPIO驱动能力约为14mA
    2：GPIO驱动能力约为21mA   3：GPIO驱动能力约为28mA
  二、当VDDIO参考电压为1.8V时，drvstrength有如下含义：
    0：GPIO驱动能力约为2mA    1：GPIO驱动能力约为5mA
    2：GPIO驱动能力约为8mA   3：GPIO驱动能力约为10mA 
* @return  0：成功。 -1：失败，非法参数
*****************************************************************************************************/
int8_t McuGpioDrvStrengthSet(MCU_GPIO_PinTypeDef num,uint8_t drvstrength)
{
    if( num<MCU_GPIO0 || num>MCU_GPIO63 )
    {
    	debug_assert(0);
        return -1;
    }

    //开GPIO时钟
    PRCM_ClockEnable(CORE_CKG_CTL_GPIO_EN);

    //计算gpio所在的DRVCTL寄存器，并获取该寄存器值
    uint32_t DrvRegValue = GPIO->DRVCTL[num / 10];
    //计算gpio在该DRVCTL寄存器中的偏移量
    uint32_t BitOffset = (num % 10) * 3;
    //更新gpio的驱动能力
    GPIO->DRVCTL[num / 10] = (DrvRegValue & (~((uint32_t)0x07 << BitOffset))) | (drvstrength << BitOffset);

    return 0;
}

/*****************************************************************************************************
* @brief  设置IO模式。接口耗时：入参num为MCU_GPIO_WKP耗时35.6us，入参num为MCU_GPIO耗时42.2us
* @param  num：GPIO端口号可选MCU_WKP1-3，MCU_GPIO0-63，详情参考 @ref MCU_GPIO_PinTypeDef
* @param  mode：
  0x00：推挽输出；    0x01：开漏输出（默认输入也打开）
  0x11：数字浮空输入；0x12：内部上拉输入；0x13：内部下拉输入
  0x21: 浮空；       0x22：内部上拉；    0x23：内部下拉    0x24：高阻态
* @return  0：成功。 -1：失败，非法参数
* @note：WKPx不支持mode为0x01，WKPx上拉在1.4v左右为正确现象，WKPx输出高为3.0V左右
*****************************************************************************************************/
__RAM_FUNC int8_t McuGpioModeSet(MCU_GPIO_PinTypeDef num,uint8_t mode)
{
    if( num<MCU_WKP3 || num>MCU_GPIO63 )
    {
    	debug_assert(0);
        return -1;
    }

    if( num >= MCU_GPIO0 )
    {
        //开GPIO时钟
        PRCM_ClockEnable(CORE_CKG_CTL_GPIO_EN);

        //寄存器偏移计算
        uint32_t PinRegPos = (num >= 32) ? 0x01 : 0x00;
        uint32_t PinBitPos = (num >= 32) ? ((uint32_t)0x01 << (num & 31)) : ((uint32_t)0x01 << num);

        //如果是gpio3、4，则关闭lpuart，释放gpio3、4
        if(num == MCU_GPIO3 || num == MCU_GPIO4)
        {
            AONPRCM->LPUA1_CTRL &= ~0x03;
        }

        //切为GPIO模式
        GPIO->MODE[PinRegPos] &= ~PinBitPos;
        GPIO->CTL[PinRegPos]  &= ~PinBitPos;
        GPIO->PERI[num] =  0xFF;

        //GPIO配置
        switch (mode)
        {
            case 0x00: //推挽输出
            {
                GPIO->INPUT_EN[PinRegPos]  &= ~PinBitPos;
                GPIO->OUTPUT_EN[PinRegPos] &= ~PinBitPos;
                GPIO->OPEND_DRAIN[PinRegPos] |= PinBitPos;
                GPIO_SetPull(num, GPIO_FLOAT);
                break;
            }
            case 0x01: //开漏输出，需要将输入输出都打开，内部上下拉都关闭
            {
                GPIO->INPUT_EN[PinRegPos]  |= PinBitPos;
                GPIO->OUTPUT_EN[PinRegPos] &= ~PinBitPos;
                GPIO->OPEND_DRAIN[PinRegPos] &= ~PinBitPos;
                GPIO_SetPull(num, GPIO_FLOAT);
                break;
            }
            case 0x11: //数字浮空输入
            {
                GPIO->INPUT_EN[PinRegPos]  |= PinBitPos;
                GPIO->OUTPUT_EN[PinRegPos] |= PinBitPos;
                GPIO_SetPull(num, GPIO_FLOAT);
                break;
            }
            case 0x12: //内部上拉输入
            {
                GPIO->INPUT_EN[PinRegPos]  |= PinBitPos;
                GPIO->OUTPUT_EN[PinRegPos] |= PinBitPos;
                GPIO_SetPull(num, GPIO_PULL_UP);
                break;
            }
            case 0x13: //内部下拉输入
            {
                GPIO->INPUT_EN[PinRegPos]  |= PinBitPos;
                GPIO->OUTPUT_EN[PinRegPos] |= PinBitPos;
                GPIO_SetPull(num, GPIO_PULL_DOWN);
                break;
            }
            case 0x21: //浮空
            {
                GPIO_SetPull(num, GPIO_FLOAT);
                break;
            }
            case 0x22: //内部上拉
            {
                GPIO_SetPull(num, GPIO_PULL_UP);
                break;
            }
            case 0x23: //内部下拉
            {
                GPIO_SetPull(num, GPIO_PULL_DOWN);
                break;
            }
            case 0x24: //高阻态
            {
                //禁能输入、输出
                GPIO->INPUT_EN[PinRegPos] &= ~PinBitPos;
                GPIO->OUTPUT_EN[PinRegPos] |= PinBitPos;

                //禁能上下拉（即浮空）
                GPIO_SetPull(num, GPIO_FLOAT);
                break;
            }
            default: 
            {
				debug_assert(0);
				return -1; //入参非法
            }
        }
    }
    else
    {
        //转换为WKP接口入参
        uint8_t wkp_num = 0;
        switch(num)
        {
            case MCU_WKP1: { wkp_num = AGPIO0; break; }
            case MCU_WKP2: { wkp_num = AGPIO1; break; }
            case MCU_WKP3: { wkp_num = AGPIO2; break; }
            default: 
            {
				debug_assert(0);
				return -1; //入参非法
            }
        }

        //WKP配置
        switch (mode)
        {
            case 0x00: //推挽输出
            {
                AGPIO_InputDisable(wkp_num);
                AGPIO_OutputEnable(wkp_num);
                AGPIO_PullupDisable(wkp_num);
                AGPIO_PulldownDisable(wkp_num);
                break;
            }
            case 0x11: //数字浮空输入
            {
                AGPIO_OutputDisable(wkp_num);
                AGPIO_InputEnable(wkp_num);
                AGPIO_PullupDisable(wkp_num);
                AGPIO_PulldownDisable(wkp_num);
                break;
            }
            case 0x12: //内部上拉输入
            {
                AGPIO_OutputDisable(wkp_num);
                AGPIO_InputEnable(wkp_num);
                AGPIO_PulldownDisable(wkp_num);
                AGPIO_PullupEnable(wkp_num);
                break;
            }
            case 0x13: //内部下拉输入
            {
                AGPIO_OutputDisable(wkp_num);
                AGPIO_InputEnable(wkp_num);
                AGPIO_PullupDisable(wkp_num);
                AGPIO_PulldownEnable(wkp_num);
                break;
            }
            case 0x21: //浮空
            {
                AGPIO_PullupDisable(wkp_num);
                AGPIO_PulldownDisable(wkp_num);
                break;
            }
            case 0x22: //内部上拉
            {
                AGPIO_PulldownDisable(wkp_num);
                AGPIO_PullupEnable(wkp_num);
                break;
            }
            case 0x23: //内部下拉
            {
                AGPIO_PullupDisable(wkp_num);
                AGPIO_PulldownEnable(wkp_num);
                break;
            }
            case 0x24: //高阻态
            {
                //禁能输入、输出
                AGPIO_InputDisable(wkp_num);
                AGPIO_OutputDisable(wkp_num);

                //禁能上下拉（即浮空）
                AGPIO_PullupDisable(wkp_num);
                AGPIO_PulldownDisable(wkp_num);
                break;
            }
            default: 
            {
				debug_assert(0);
				return -1; //入参非法
            }
        }
    }

    return 0;
}

/*****************************************************************************************************
* @brief  读GPIO电平。接口耗时：入参num为MCU_GPIO_WKP耗时22.9us，入参num为MCU_GPIO耗时16us
* @param  num：GPIO端口号可选MCU_WKP1-3，MCU_GPIO0-63，详情参考 @ref MCU_GPIO_PinTypeDef
* @return  1：高电平  0：低电平  -1：无效
*****************************************************************************************************/
__RAM_FUNC int8_t McuGpioRead(MCU_GPIO_PinTypeDef num)
{
    if( num<MCU_WKP3 || num>MCU_GPIO63 )
    {
        return -1;
    }

    if( num >= MCU_GPIO0 )
    {
        return GPIO_Read_InOutPin(num);
    }
    else
    {
        //转换为WKP接口入参
        uint8_t wkp_num = 0;
        switch(num)
        {
            case MCU_WKP1: { wkp_num = AGPIO0; break; }
            case MCU_WKP2: { wkp_num = AGPIO1; break; }
            case MCU_WKP3: { wkp_num = AGPIO2; break; }
            default: 
            {
				debug_assert(0);
				return -1; //入参非法
            }
        }
        
        return AGPIO_ReadPin(wkp_num);
    }

    return -1;
}

/*****************************************************************************************************
* @brief  写GPIO电平。接口耗时：入参num为MCU_GPIO_WKP耗时11.7us，MCU_GPIO耗时10.1us
* @param  num：GPIO端口号可选MCU_WKP1-3，MCU_GPIO0-63，详情参考 @ref MCU_GPIO_PinTypeDef
* @param  value：1：高电平  0：低电平
* @return  0：有效 -1：无效
*****************************************************************************************************/
__RAM_FUNC int8_t McuGpioWrite(MCU_GPIO_PinTypeDef num,uint8_t value)
{
    if( num<MCU_WKP3 || num>MCU_GPIO63 )
    {
        return -1;
    }

    if( num >= MCU_GPIO0 )
    {
        (value == 0x00) ? GPIO_WritePin(num, GPIO_PIN_RESET) : GPIO_WritePin(num, GPIO_PIN_SET);
    }
    else
    {
        //转换为WKP接口入参
        uint8_t wkp_num = 0;
        switch(num)
        {
            case MCU_WKP1: { wkp_num = AGPIO0; break; }
            case MCU_WKP2: { wkp_num = AGPIO1; break; }
            case MCU_WKP3: { wkp_num = AGPIO2; break; }
            default: 
            {
				debug_assert(0);
				return -1; //入参非法
            }
        }

        (value == 0x00) ? AGPIO_Clear(wkp_num) : AGPIO_Set(wkp_num);
    }
    
    return 0;
}

/*****************************************************************************************************
* @brief  翻转GPIO电平。接口耗时：入参num为MCU_GPIO_WKP耗时11.7us，MCU_GPIO耗时10.1us
* @param  num：GPIO端口号可选MCU_WKP1-3，MCU_GPIO0-63，详情参考 @ref MCU_GPIO_PinTypeDef
* @note   被翻转的GPIO必须配置为输出模式
* @return  0：有效 -1：无效
*****************************************************************************************************/
__RAM_FUNC int8_t McuGpioToggle(MCU_GPIO_PinTypeDef num)
{
    if( num<MCU_WKP3 || num>MCU_GPIO63 )
    {
        return -1;
    }

    if( num >= MCU_GPIO0 )
    {
        GPIO_TogglePin(num);
    }
    else
    {
        //转换为WKP接口入参
        uint8_t wkp_num = 0;
        switch(num)
        {
            case MCU_WKP1: { wkp_num = AGPIO0; break; }
            case MCU_WKP2: { wkp_num = AGPIO1; break; }
            case MCU_WKP3: { wkp_num = AGPIO2; break; }
            default: 
            {
				debug_assert(0);
				return -1; //入参非法
            }
        }
        AGPIO_TogglePin(wkp_num);
    }
    
    return 0;
}

/*****************************************************************************************************
* @brief  GPIO中断使能。接口耗时：入参num为MCU_GPIO_WKP耗时10.9us，入参MCU_GPIO耗时9.6us
* @param  num：GPIO端口号可选MCU_WKP1-3，MCU_GPIO0-63，详情参考 @ref MCU_GPIO_PinTypeDef
* @return  0：有效 -1：无效
* @note：
* MCU_WKP1-3 对应中断回调函数为 AGPI_Wakeup_Callback
* MCU_GPIO0-7 对应中断回调函数为 GPI_Wakeup_Callback
* MCU_GPIO-63 对应中断服务函数为 McuGpio_IrqHandler
*****************************************************************************************************/
int8_t McuGpioIrqEn(MCU_GPIO_PinTypeDef num)
{
    if( num<MCU_WKP3 || num>MCU_GPIO63 )
    {
        return -1;
    }

    if( num >= MCU_GPIO0 )
    {
        //GPIO0-7唤醒中断使能，对应中断回调函数为 GPI_Wakeup_Callback
        //gpi引脚对应的是WAKEUP中断，该中断是默认打开的，因此只需要将gpi引脚的唤醒功能打开即可产生中断
        //GPIO8-63外部中断使能，对应中断服务函数为 McuGpio_IrqHandler
        (num <= MCU_GPIO7) ? GPI_WakeupEnable((uint8_t)num) : GPIO_IntCmd((GPIO_PadTypeDef)num, ENABLE);
    }
    else
    {
        //转换为WKP接口入参
        uint8_t wkp_num = 0;
        switch(num)
        {
            case MCU_WKP1: { wkp_num = AGPIO0; break; }
            case MCU_WKP2: { wkp_num = AGPIO1; break; }
            case MCU_WKP3: { wkp_num = AGPIO2; break; }
            default: 
            {
				debug_assert(0);
				return -1; //入参非法
            }
        }

        //wkp1-3唤醒中断使能，对应中断回调函数为 AGPI_Wakeup_Callback
        //wkp引脚对应的是WAKEUP中断，该中断是默认打开的，因此只需要将wkp引脚的唤醒功能打开即可产生中断，
        AGPI_WakeupEnable(wkp_num);
    }
    
    return 0;
}

/*****************************************************************************************************
* @brief  GPIO中断禁能。接口耗时：入参num为MCU_GPIO_WKP耗时63.4us，入参MCU_GPIO耗时10.3s
* @param  num：GPIO端口号可选MCU_WKP1-3，MCU_GPIO0-63，详情参考 @ref MCU_GPIO_PinTypeDef
* @return  0：有效 -1：无效
*****************************************************************************************************/
int8_t McuGpioIrqDis(MCU_GPIO_PinTypeDef num)
{
    if( num<MCU_WKP3 || num>MCU_GPIO63 )
    {
        return -1;
    }

    if( num >= MCU_GPIO0 )
    {
        //GPIO0-7唤醒中断禁能
        (num <= MCU_GPIO7) ? GPI_WakeupDisable((uint8_t)num) : GPIO_IntCmd((GPIO_PadTypeDef)num, DISABLE);
    }
    else
    {
        //转换为WKP接口入参
        uint8_t wkp_num = 0;
        switch(num)
        {
            case MCU_WKP1: { wkp_num = AGPIO0; break; }
            case MCU_WKP2: { wkp_num = AGPIO1; break; }
            case MCU_WKP3: { wkp_num = AGPIO2; break; }
            default: 
            {
				debug_assert(0);
				return -1; //入参非法
            }
        }

        //wkp引脚对应的是WAKEUP中断，该中断是默认打开的，因此只需要将wkp引脚的唤醒功能打开即可产生中断，
        //若不想产生中断则关闭wkp引脚的唤醒功能即可。
        AGPI_WakeupDisable(wkp_num);
    }

    return 0;
}

/*****************************************************************************************************
* @brief   GPIO中断回调函数
* @return  NA
* @note：MCU_GPIO-63 对应中断服务函数为 McuGpio_IrqHandler
*****************************************************************************************************/
__RAM_FUNC void McuGpio_IrqHandler(void)
{
    for (uint8_t num = 8; num < MCU_GPIO_NUM + 8; num++)
    {
        // 如果中断产生，且中断回调函数指针非空，则执行回调
        if(GPIO_GetIntStatus(num)) 
        {
            if(mcu_gpio_irq_list[num - 8] != NULL)
            {
                mcu_gpio_irq_list[num - 8]();                
            }
            GPIO_IntStatusClear(num);
        }
    }
}

/*****************************************************************************************************
 * @brief  GPIO_0-7 中断回调函数
 * @param  num: GPI中断号，可选0~7
 * @return  NA
*****************************************************************************************************/
__RAM_FUNC void McuGpi_Wakeup_Callback(uint8_t num)
{
    // 如果中断产生，且中断回调函数指针非空，则执行回调
    if(mcu_gpi_irq_list[num].irq != NULL)
    {
        mcu_gpi_irq_list[num].irq();
    }

    //双边沿模式需要特殊处理
    if(mcu_gpi_irq_list[num].trg_mode == 1)
    {
        uint32_t trg_level = 0;
        GPI_WakeupDisable(num); //GPI唤醒配置必须在唤醒功能关闭时才能配置！！！
        if(GPIO_Read_InOutPin(num) == GPIO_PIN_SET)
        {
            trg_level = GPI_POL_CFG_LOW_LEVEL;
            GPIO_SetPull(num, GPIO_PULL_UP); //确保上下拉状态与外部电平一致，以降低功耗
        }
        else
        {
            trg_level = GPI_POL_CFG_HIGH_LEVEL;
            GPIO_SetPull(num, GPIO_PULL_DOWN); //确保上下拉状态与外部电平一致，以降低功耗
        }
        GPI_WakeupConfig(num, DISABLE, trg_level, DISABLE);
        GPI_WakeupEnable(num);
    }
}

/*****************************************************************************************************
 * @brief  WKP_1-3 中断回调函数
 * @param  num: AGPI中断号，可选0、1、2
 * @return  NA
 * @attention 上升沿与下降沿模式，只会调用一次回调；双边沿模式，由于两个边沿都会触发中断，故会调用两次回调
*****************************************************************************************************/
__RAM_FUNC void McuAgpi_Wakeup_Callback(uint8_t num)
{
    // 如果中断产生，且中断回调函数指针非空，则执行回调
    if (mcu_wkup_irq_list[num].irq != NULL)
    {
        mcu_wkup_irq_list[num].irq();
    }

    //双边沿模式需要特殊处理
    if(mcu_wkup_irq_list[num].trg_mode == 1)
    {
        uint32_t trg_level = 0;
        AGPI_WakeupDisable(num); //AGPI唤醒配置必须在唤醒功能关闭时才能配置！！！
        if(AGPIO_ReadPin(num) == 0)
        {
            trg_level = AGPI_POL_CFG_LOW_LEVEL;
            AGPIO_PullupDisable(num); //确保上下拉状态与外部电平一致，以降低功耗
            AGPIO_PulldownEnable(num);
        }
        else
        {
            trg_level = AGPI_POL_CFG_HIGH_LEVEL;
            AGPIO_PulldownDisable(num); //确保上下拉状态与外部电平一致，以降低功耗
            AGPIO_PullupEnable(num);
        }
        AGPI_WakeupConfig(num, trg_level, AGPI_WKUP_CFG_FALLING, AGPI_PLS_CFG_35_UTC_CLK); //实际无消抖
        AGPI_WakeupEnable(num);
    }
}

/*****************************************************************************************************
* @brief  GPIO中断处理函数注册。接口耗时：入参num为MCU_GPIO_WKP耗时40.4us，入参MCU_GPIO耗时39.8us
* @param  num：GPIO端口号可选MCU_WKP1-3，MCU_GPIO0-63，详情参考 @ref MCU_GPIO_PinTypeDef
* @param  p_fun：中断回调函数
* @param  mode: 中断触发模式，0：上升沿或下降沿触发 1：上升沿触发 2：下降沿触发 
* @return  0：有效 -1：无效
* @note   1、对于MCU_GPIO_WKPx，MCU_GPIO0-7 相关中断在工作态与睡眠时有效，可唤醒深睡。
          2、对于MCU_GPIO8-63，相关中断只能在工作态时使用。
*****************************************************************************************************/
int8_t McuGpioIrqReg(MCU_GPIO_PinTypeDef num,pFunType_void p_fun,MCU_GPIO_Int_ModeTypedef mode)
{
    if( num<MCU_WKP3 || num>MCU_GPIO63 )
    {
        return -1;
    }

    uint32_t trg_level = 0;

    if( num >= MCU_GPIO0 )
    {
        //GPIO0-7配置GPI唤醒中断触发模式
        if(num <= MCU_GPIO7)
        {
            if(mode == MCU_GPIO_INT_RISING) //上升沿触发
            {
                mcu_gpi_irq_list[num].trg_mode = 0;
                trg_level = GPI_POL_CFG_HIGH_LEVEL;
                GPIO_SetPull(num, GPIO_PULL_DOWN);
            }
            else if(mode == MCU_GPIO_INT_FALLING)//下降沿触发 
            {
                mcu_gpi_irq_list[num].trg_mode = 0;
                trg_level = GPI_POL_CFG_LOW_LEVEL;
                GPIO_SetPull(num, GPIO_PULL_UP);
            }
            else //上升沿或下降沿触发
            {
                mcu_gpi_irq_list[num].trg_mode = 1;
                //若为低电平则首次为上升沿触发
                if(GPIO_Read_InOutPin(num) == GPIO_PIN_RESET)
                {
                    trg_level = GPI_POL_CFG_HIGH_LEVEL;
                    GPIO_SetPull(num, GPIO_PULL_DOWN);
                }
                //若为高电平则首次为下降沿触发
                else
                {
                    trg_level = GPI_POL_CFG_LOW_LEVEL;
                    GPIO_SetPull(num, GPIO_PULL_UP);
                }
            }
            GPI_WakeupConfig(num, DISABLE, trg_level, DISABLE);

            //GPI引脚对应的是WAKEUP中断，该中断是默认打开的，因此只需要将GPI引脚的唤醒功能打开即可产生中断，
            //若不想产生中断则关闭GPI引脚的唤醒功能即可。
            //因此这里只需要注册用户中断回调函数
            mcu_gpi_irq_list[num].irq = p_fun;
            mark_dyn_addr(&(mcu_gpi_irq_list[num].irq));
        }
        //GPIO8-63配置GPIO中断触发模式
        else
        {
            uint32_t PinRegPos = (num >= 32) ? 0x01 : 0x00;
            uint32_t PinBitPos = (num >= 32) ? ((uint32_t)0x01 << (num & 31)) : ((uint32_t)0x01 << num);
            
            if(mode == MCU_GPIO_INT_RISING) //上升沿触发
            {
                GPIO->INTS[PinRegPos]  &= ~PinBitPos;
                GPIO->INTBE[PinRegPos] &= ~PinBitPos;
                GPIO->INTEV[PinRegPos] &= ~PinBitPos;
                GPIO_SetPull(num, GPIO_PULL_DOWN);
            }
            else if(mode == MCU_GPIO_INT_FALLING) //下降沿触发
            {
                GPIO->INTS[PinRegPos]  &= ~PinBitPos;
                GPIO->INTBE[PinRegPos] &= ~PinBitPos;
                GPIO->INTEV[PinRegPos] |= PinBitPos;
                GPIO_SetPull(num, GPIO_PULL_UP);
            }
            else //上升沿或下降沿触发
            {
                GPIO->INTS[PinRegPos]  &= ~PinBitPos;
                GPIO->INTBE[PinRegPos] |= PinBitPos;
                if(GPIO_Read_InOutPin(num) == GPIO_PIN_RESET)
                {
                    GPIO_SetPull(num, GPIO_PULL_DOWN);
                }
                else
                {
                    GPIO_SetPull(num, GPIO_PULL_UP);
                }
            }

            //注册中断服务函数，设置优先级，使能总中断
            NVIC_IntRegister(GPIO_IRQn, McuGpio_IrqHandler, 1);
            
            //注册用户中断回调函数
            mcu_gpio_irq_list[num - 8] = p_fun;
            mark_dyn_addr(&(mcu_gpio_irq_list[num - 8]));
        }
    }
    else
    {
        //转换为WKP接口入参
        uint8_t wkp_num = 0;
        switch(num)
        {
            case MCU_WKP1: { wkp_num = AGPIO0; break; }
            case MCU_WKP2: { wkp_num = AGPIO1; break; }
            case MCU_WKP3: { wkp_num = AGPIO2; break; }
            default: 
            {
				debug_assert(0);
				return -1; //入参非法
            }
        }

        //设置WKP触发模式
        if(mode == MCU_GPIO_INT_RISING) //上升沿触发
        {
            mcu_wkup_irq_list[wkp_num].trg_mode = 0;
            trg_level = AGPI_POL_CFG_LOW_LEVEL;
            AGPIO_PullupDisable(wkp_num);
            AGPIO_PulldownEnable(wkp_num);
        }
        else if(mode == MCU_GPIO_INT_FALLING)//下降沿触发
        {
            mcu_wkup_irq_list[wkp_num].trg_mode = 0;
            trg_level = AGPI_POL_CFG_HIGH_LEVEL;
            AGPIO_PulldownDisable(wkp_num);
            AGPIO_PullupEnable(wkp_num);
        }
        else //双边沿触发
        {
            mcu_wkup_irq_list[wkp_num].trg_mode = 1;
            //若为低电平则首次为上升沿触发
            if(AGPIO_ReadPin(wkp_num) == 0)
            {
                trg_level = AGPI_POL_CFG_LOW_LEVEL;
                AGPIO_PullupDisable(wkp_num);
                AGPIO_PulldownEnable(wkp_num);
            }
            //若为高电平则首次为下降沿触发
            else
            {
                trg_level = AGPI_POL_CFG_HIGH_LEVEL;  
                AGPIO_PulldownDisable(wkp_num);
                AGPIO_PullupEnable(wkp_num);
            }
        }
        AGPI_WakeupConfig(wkp_num, trg_level, AGPI_WKUP_CFG_FALLING, AGPI_PLS_CFG_35_UTC_CLK); //实际无消抖

        //wkp引脚对应的是WAKEUP中断，该中断是默认打开的，因此只需要将wkp引脚的唤醒功能打开即可产生中断，
        //若不想产生中断则关闭wkp引脚的唤醒功能即可。
        //因此这里只需要注册用户中断回调函数
        mcu_wkup_irq_list[wkp_num].irq  = p_fun;
        mark_dyn_addr(&(mcu_wkup_irq_list[wkp_num].irq));
    }

    return 0;
}
/**
 * @brief 用户初始化WKUP_EN为单唤醒引脚、单复位引脚、复位唤醒引脚中的一个功能
 * @param wakeup_en_init 详情参考 @ref WakeupEn_InitTypeDef
 * @note 唤醒时序如下：
 * 	     (1)若 wkup_polarity 配为 POLARITY_HIGH ：
 *          当 wkup_edge 配为 FALLING ，则引脚触发唤醒的时序为：收到上升沿，外部持续高电平时长不少于 wakeup_time ，收到下降沿触发唤醒
 * 			当 wkup_edge 配为 RISING ， 则引脚触发唤醒的时序为：收到上升沿触发唤醒
 * 			当 wkup_edge 配为 BOTH ，   则引脚触发唤醒的时序为：收到上升沿第一次唤醒，外部持续高电平时长不少于 wakeup_time后收到下降沿第二次唤醒
 * 		 (2)若 wkup_polarity 配为 POLARITY_LOW ：
 *          当 wkup_edge 配为 FALLING ，则引脚触发唤醒的时序为：收到下降沿触发唤醒
 * 			当 wkup_edge 配为 RISING ， 则引脚触发唤醒的时序为：收到下降沿，外部持续低电平时长不少于 wakeup_time ，收到上升沿触发唤醒
 * 			当 wkup_edge 配为 BOTH ，   则引脚触发唤醒的时序为：收到下降沿第一次唤醒，外部持续低电平时长不少于 wakeup_time后收到上升沿第二次唤醒
 * @note 以上配置以WKUP_EN引脚的单唤醒功能为例，对于WKUP_EN引脚的单复位功能、复位唤醒功能，推荐用户使用如下配置：
 *       配置一： pull 配为 PIN_PULLDOWN ， wkup_polarity 配为 POLARITY_HIGH ， wkup_edge 配为 FALLING，时序为引脚收到上升沿时开始计时，当收到下降沿时，高电平持续时间大于wakeup_time小于reset_time时触发唤醒，大于reset_time时则触发复位
 *       配置二： pull 配为 PIN_PULLUP ， wkup_polarity 配为 POLARITY_LOW ， wkup_edge 配为 RISING，时序为引脚收到下降沿时开始计时，当收到上升沿时，高电平持续时间大于wakeup_time小于reset_time时触发唤醒，大于reset_time时则触发复位
 */
void WakeupEn_Init(WakeupEn_InitTypeDef *wakeup_en_init)
{
    uint8_t polarity = 0;
    uint8_t edge = 0;

	//配置wkp_en引脚前关闭复位功能和唤醒功能，确保wkp_en引脚的复位功能关闭，防止配置时产生复位
    AONPRCM->RST_CTRL1 &= ~0x01;
    AONPRCM->RSTWKP_CFG &= ~0x01;

    EXPIN_PullSet(wakeup_en_init->pull);

    if(wakeup_en_init->wkup_polarity == POLARITY_LOW)
    {
        polarity = EXPIN_POL_CFG_LOW_LEVEL;

        if(wakeup_en_init->wkup_edge == FALLING)
            edge = 2;
        else if(wakeup_en_init->wkup_edge == RISING)
            edge = 0;
        else if(wakeup_en_init->wkup_edge == BOTH)
            edge = 3;
    }
    else if(wakeup_en_init->wkup_polarity == POLARITY_HIGH)
    {
        polarity = EXPIN_POL_CFG_HIGH_LEVEL;

        if(wakeup_en_init->wkup_edge == FALLING)
            edge = 0;
        else if(wakeup_en_init->wkup_edge == RISING)
            edge = 2;
        else if(wakeup_en_init->wkup_edge == BOTH)
            edge = 3;
    }

    EXPIN_WkupResetConfig(polarity, edge, wakeup_en_init->wakeup_time, wakeup_en_init->reset_time);

    if(wakeup_en_init->mode == WAKEUP)
    {
        EXPIN_WkupEnable();
    }    
    else if(wakeup_en_init->mode == RESET_ONLY)
    {
        EXPIN_ResetEnable();
    }
    else if(wakeup_en_init->mode == WAKEUP_AND_RESET)
    {
        EXPIN_WkupResetEnable();
    }

    //WakeupEn引脚对应的是WAKEUP中断，该中断是默认打开的，因此只需要将WakeupEn引脚的唤醒功能打开即可产生中断，
    //WakeupEn引脚的唤醒功能是始终开启的，因此这里只需要注册用户中断回调函数
    p_Wkupen_WakeupCallback = wakeup_en_init->irq;
    mark_dyn_addr(&p_Wkupen_WakeupCallback);
}
