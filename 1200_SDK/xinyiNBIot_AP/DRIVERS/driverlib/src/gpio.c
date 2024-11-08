#include "gpio.h"
#include "interrupt.h"

/**
 * @brief 遍历64个引脚的外设功能寄存器值，
 *        若该引脚的外设功能寄存器值与目标外设功能寄存器值相等，且该引脚号与目标引脚号不相等
 *        那么，将该引脚的外设功能寄存器值设置成无效值。
 *        此时，目标引脚号就可以正常的配成目标外设功能了，保证了一个外设功能只被一个引脚占有。
 * 
 * @param CurrentPin  : 目标引脚号
 * @param CurrentPeri : 目标外设功能
 */
void GPIO_RemoveAllocate(uint8_t CurrentPin, uint8_t CurrentPeri)
{
    for(uint8_t PadNum = 0; PadNum < 64; PadNum++)
    {
        if((GPIO->PERI[PadNum] == CurrentPeri) && (PadNum != CurrentPin))
        {
            GPIO->PERI[PadNum] = 0xFF;
        }
    }
}

/**
  * @brief Initializes the GPIO.
  * @param GPIO_Init: pointer to a GPIO_InitTypeDef structure that
  *        contains the configuration information for the specified GPIO peripheral.
  * @param speed，当为FAST_SPEED时为快速GPIO初始化，当某GPIO复用为外设引脚时，在外设使用完成后，
 *         必须调用 GPIO_AllocateRemove 接口去映射，才可以保证该引脚在下次复用为其他外设引脚时可以正常使用。
  * @retval None.
  */
void GPIO_Init(GPIO_InitTypeDef *GPIO_InitStu, GPIO_InitSpeedTypedef speed)
{
    uint8_t CurrentPin = GPIO_InitStu->Pin;
	uint8_t CurrentPeri = GPIO_InitStu->PinRemap;
	uint32_t PinRegPos  = (CurrentPin >= 32) ? 0x01 : 0x00;
    uint32_t PinBitPos =  (CurrentPin >= 32) ? ((uint32_t)0x01 << (CurrentPin & 31)) : ((uint32_t)0x01 << CurrentPin);

    /* In case of GPIO is set to hardware mode(peripheral control) */
    if(GPIO_InitStu->Mode == GPIO_MODE_HW_PER)
    {
        if (speed == NORMAL_SPEED)
        {
            GPIO_RemoveAllocate(CurrentPin, CurrentPeri);
        }

		/*pad and peripheral remap*/
        GPIO->PERI[CurrentPin] =  CurrentPeri;
        GPIO->MODE[PinRegPos] |= PinBitPos;
		GPIO->PULL_UP_SEL[PinRegPos] &= ~PinBitPos;
        GPIO->CTL[PinRegPos]  |= PinBitPos;
    }
    else if(GPIO_InitStu->Mode == GPIO_MODE_HIS) //高阻态（无论引脚是否悬空都不会对芯片内部产生影响）
    {
        //切为GPIO模式
        GPIO->MODE[PinRegPos] &= ~PinBitPos;
        GPIO->CTL[PinRegPos] &= ~PinBitPos;
        GPIO->PERI[CurrentPin] = 0xFF;

        //禁能输入、输出
        GPIO->INPUT_EN[PinRegPos] &= ~PinBitPos;
        GPIO->OUTPUT_EN[PinRegPos] |= PinBitPos;

        //禁能上下拉（即浮空）
        GPIO->PULL_UP[PinRegPos] |= PinBitPos;
        GPIO->PULL_DOWN[PinRegPos] |= PinBitPos;
    }
    else  /* In case of GPIO is set to hardware mode(register control) or GPIO mode*/
    {
        if(GPIO_InitStu->Mode & GPIO_PER_MODE) //hardware mode(register control)
        {
            if (speed == NORMAL_SPEED)
            {
                GPIO_RemoveAllocate(CurrentPin, CurrentPeri);
            }

            GPIO->MODE[PinRegPos] |= PinBitPos;
            GPIO->PERI[CurrentPin] =  CurrentPeri;
			GPIO->PULL_UP_SEL[PinRegPos] |= PinBitPos;
            GPIO->CTL[PinRegPos]  &= ~PinBitPos;
        }
        else if(GPIO_InitStu->Mode & GPIO_IO_MODE) //GPIO mode
        {
            GPIO->MODE[PinRegPos] &= ~PinBitPos;
            GPIO->CTL[PinRegPos]  &= ~PinBitPos;
            GPIO->PERI[CurrentPin] =  0xFF;
        }

        /* In case of Output or Input function mode selection */
        if(GPIO_InitStu->Mode & GPIO_DIR_INOUT) // inoutput
        {
            GPIO->INPUT_EN[PinRegPos]  |= PinBitPos;
            GPIO->OUTPUT_EN[PinRegPos] &= ~PinBitPos;
        }
        else if(GPIO_InitStu->Mode & GPIO_DIR_INPUT) // input
        {
            GPIO->INPUT_EN[PinRegPos]  |= PinBitPos;
            GPIO->OUTPUT_EN[PinRegPos] |= PinBitPos;
        }
        else // output
        {
            GPIO->OUTPUT_EN[PinRegPos] &= ~PinBitPos;

            if(GPIO_InitStu->Mode & GPIO_DIR_OD) //output opendrain
            {
                // 开漏输出模式需要打开输入，才可以读到输入寄存器值
                GPIO->INPUT_EN[PinRegPos]  |= PinBitPos;
                GPIO->OPEND_DRAIN[PinRegPos] &= ~PinBitPos;
            }
            else if(GPIO_InitStu->Mode & GPIO_DIR_PP) //output pushpull
            {
                GPIO->INPUT_EN[PinRegPos]  &= ~PinBitPos;
                GPIO->OPEND_DRAIN[PinRegPos] |= PinBitPos;
            }
        }

        /* Configure the Pull-up or Pull-down resistor for the current pin */        
        if(GPIO_InitStu->Pull == GPIO_PULL_UP)
        {
            GPIO->PULL_UP[PinRegPos]   &= ~PinBitPos;
            GPIO->PULL_DOWN[PinRegPos] |= PinBitPos;
        }
        else if(GPIO_InitStu->Pull == GPIO_PULL_DOWN)
        {
            GPIO->PULL_UP[PinRegPos]   |= PinBitPos;
            GPIO->PULL_DOWN[PinRegPos] &= ~PinBitPos;
        }
        else // float
        {
            GPIO->PULL_UP[PinRegPos]   |= PinBitPos;
            GPIO->PULL_DOWN[PinRegPos] |= PinBitPos;
        }

        /* Configure the interrupt source for the current pin */
        if((GPIO_InitStu->Mode & GPIO_IO_MODE) && GPIO_InitStu->Int)
        {
            if(GPIO_InitStu->Int == GPIO_INT_HIGH_LEVEL)
            {
                GPIO->INTS[PinRegPos] |= PinBitPos;
            }
            else 
            {
                GPIO->INTS[PinRegPos]  &= ~PinBitPos;
                if(GPIO_InitStu->Int == GPIO_INT_BOTH_EDGE)
                {
                    GPIO->INTBE[PinRegPos] |= PinBitPos;
                }
                else
                {
                    GPIO->INTBE[PinRegPos] &= ~PinBitPos;
                    if(GPIO_InitStu->Int == GPIO_INT_RISE_EDGE)
                    {
                        GPIO->INTEV[PinRegPos] &= ~PinBitPos;
                    }
                    else if(GPIO_InitStu->Int == GPIO_INT_FALL_EDGE)
                    {
                        GPIO->INTEV[PinRegPos] |= PinBitPos;
                    }
                }
            }
            GPIO->INTEN[PinRegPos] |= PinBitPos;
        }
    }
    /*--------------------- GPIO Drive Strength Configuration ------------------------*/
    /* Get the register value corresponding to pins */
    uint32_t DrvRegValue = GPIO->DRVCTL[CurrentPin / 10];

    /* Calculate the bits offset corresponding to pins */
    uint32_t BitOffset = (CurrentPin % 10) * 3;

    GPIO->DRVCTL[CurrentPin / 10] = (DrvRegValue & (~((uint32_t)0x07 << BitOffset))) |
	                                (GPIO_InitStu->DrvStrength << BitOffset);
}

/**
 * @brief 设置GPIO上下拉状态
 * @param GPIO_Pin 
 * @param pull 
 */
void GPIO_SetPull(GPIO_PadTypeDef GPIO_Pin, GPIO_PullTypeDef pull)
{
    uint32_t PinRegPos = (GPIO_Pin >= 32) ? 0x01 : 0x00;
    uint32_t PinBitPos = (GPIO_Pin >= 32) ? ((uint32_t)0x01 << (GPIO_Pin & 31)) : ((uint32_t)0x01 << GPIO_Pin);

    switch (pull)
    {
        case GPIO_PULL_UP: 
        {   
            GPIO->PULL_UP[PinRegPos]   &= ~PinBitPos;
            GPIO->PULL_DOWN[PinRegPos] |= PinBitPos;
            break;
        }
        case GPIO_PULL_DOWN: 
        { 
            GPIO->PULL_UP[PinRegPos]   |= PinBitPos;
            GPIO->PULL_DOWN[PinRegPos] &= ~PinBitPos;
            break;
        }
        case GPIO_FLOAT: 
        {
            GPIO->PULL_UP[PinRegPos]   |= PinBitPos;
            GPIO->PULL_DOWN[PinRegPos] |= PinBitPos;
            break;
        }
        default: break;
    }
}

/**
  * @brief Release the allocation of peripheral.
  * @param PeriNum: the signal of Peripheral
  * @retval None
  */
void GPIO_AllocateRemove(GPIO_RemapTypeDef PeriNum)
{
	uint8_t PadNum = 0x00;
	uint32_t PeriRegValue = 0xFF;

	for(PadNum = 0; PadNum < 64; PadNum++)
	{
		PeriRegValue = GPIO->PERI[PadNum];
		if(PeriRegValue == PeriNum)
		{
			GPIO->PERI[PadNum] =  0xFF;
		}
	}

	/* Clear pad allocated to input peripherals*/
	if(GPIO->PERILIN[PeriNum] != 0)
	{
		GPIO->PERILIN[PeriNum] = 0;
		GPIO->PERILIN_EN[PeriNum >> 5] &= ~(1 << (PeriNum & 0x1F));
		
		GPIO->PERILIN_INV[PeriNum >> 5] &= ~(1 << (PeriNum & 0x1F));
	}
}

void GPIO_AP_Jlink_AllocateRemove(void)
{
    //remove GPIO_AP_SWCLKTCK 
    if(GPIO->PERI[8] == GPIO_AP_SWCLKTCK)
    {
        GPIO->PERI[8] = 0xFF;
    }
	if(GPIO->PERILIN[GPIO_AP_SWCLKTCK] != 0)
	{
		GPIO->PERILIN[GPIO_AP_SWCLKTCK] = 0;
		GPIO->PERILIN_EN[GPIO_AP_SWCLKTCK >> 5] &= ~(1 << (GPIO_AP_SWCLKTCK & 0x1F));
		GPIO->PERILIN_INV[GPIO_AP_SWCLKTCK >> 5] &= ~(1 << (GPIO_AP_SWCLKTCK & 0x1F));
	}

    //remove GPIO_AP_SWDIOTMS 
    if(GPIO->PERI[13] == GPIO_AP_SWDIOTMS)
    {
        GPIO->PERI[13] = 0xFF;
    }
	if(GPIO->PERILIN[GPIO_AP_SWDIOTMS] != 0)
	{
		GPIO->PERILIN[GPIO_AP_SWDIOTMS] = 0;
		GPIO->PERILIN_EN[GPIO_AP_SWDIOTMS >> 5] &= ~(1 << (GPIO_AP_SWDIOTMS & 0x1F));
		GPIO->PERILIN_INV[GPIO_AP_SWDIOTMS >> 5] &= ~(1 << (GPIO_AP_SWDIOTMS & 0x1F));
	}
}

/**
  * @brief Set GPIO output status.
  * @param PadNum: The GPIO number.
  * @param PinState: the output status of pin.
  * @retval None
  */
void GPIO_WritePin(GPIO_PadTypeDef PadNum, GPIO_PinState PinState)
{
	uint8_t  PinRegPos  = (PadNum >= 32) ? 0x01 : 0x00;
    uint32_t PinBitPos =  (PadNum >= 32) ?
	                      ((uint32_t)0x01 << (PadNum & 31)):
	                      ((uint32_t)0x01 << PadNum);

    if(PinState != GPIO_PIN_RESET)
    {
        GPIO->DOUT[PinRegPos] |= PinBitPos;
    }
    else
    {
        GPIO->DOUT[PinRegPos] &= ~PinBitPos;
    }
}

/**
  * @brief Get GPIO output value.
  * @param PadNum: The GPIO number.
  * @retval the output value of pin.
  */
static GPIO_PinState GPIO_GetWritePin(GPIO_PadTypeDef PadNum)
{
	GPIO_PinState BitValue;
	uint8_t  PinRegPos  = (PadNum >= 32) ? 0x01 : 0x00;
    uint32_t PinBitPos =  (PadNum >= 32) ?
	                      ((uint32_t)0x01 << (PadNum & 31)):
	                      ((uint32_t)0x01 << PadNum);

	BitValue = (GPIO_PinState)(GPIO->DOUT[PinRegPos] & PinBitPos ? 1 : 0);
	return BitValue;
}

void GPIO_TogglePin(GPIO_PadTypeDef GPIO_Pin)
{
    if (GPIO_GetWritePin(GPIO_Pin) == GPIO_PIN_SET)
    {
        GPIO_WritePin(GPIO_Pin, GPIO_PIN_RESET);
    }
    else
    {
        GPIO_WritePin(GPIO_Pin, GPIO_PIN_SET);
    }
}
/**
  * @brief Get GPIO input status.
  * @param PadNum: The GPIO number.
  * @retval The input status of pin.
  */
static GPIO_PinState GPIO_ReadPin(GPIO_PadTypeDef PadNum)
{
    GPIO_PinState BitStatus;
	uint8_t  PinRegPos  = (PadNum >= 32) ? 0x01 : 0x00;
    uint32_t PinBitPos =  (PadNum >= 32) ?
	                      ((uint32_t)0x01 << (PadNum & 31)):
	                      ((uint32_t)0x01 << PadNum);

    BitStatus = (GPIO_PinState)(GPIO->DIN[PinRegPos] & PinBitPos ? 1 : 0);

    return BitStatus;
}

GPIO_PinState GPIO_Read_InOutPin(GPIO_PadTypeDef GPIO_Pin)
{
    uint32_t PinRegPos = (GPIO_Pin >= 32) ? 0x01 : 0x00;
    uint32_t PinBitPos = (GPIO_Pin >= 32) ? ((uint32_t)0x01 << (GPIO_Pin & 31)) : ((uint32_t)0x01 << GPIO_Pin);
    //开漏输出模式需要同时打开输入输出，考虑用户实际需求，默认读取输入寄存器的值
    if (GPIO->INPUT_EN[PinRegPos] & PinBitPos)
    {
        return GPIO_ReadPin(GPIO_Pin);
    }
    else
    {
        return GPIO_GetWritePin(GPIO_Pin);
    }
}

/**
  * @brief Remaps the GPIO to the input signal of peripheral.
  * @param PadNum: The GPIO number
  * @param PeriNum: the signal of Peripheral
  * @retval None
  */
void GPIO_InputPeriSelect(GPIO_PadTypeDef PadNum, GPIO_RemapTypeDef PeriNum)
{
    GPIO->PERILIN[PeriNum]  = PadNum;
}

/**
  * @brief Enable or disable the input signal of peripheral.
  * @param PeriNum: the signal of Peripheral
  * @param NewState: new state of the input signal of peripheral.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void GPIO_InputPeriSelectCmd(GPIO_RemapTypeDef PeriNum, FunctionalState NewState)
{
    volatile uint32_t* RegAddress;
    volatile uint32_t BitOffset;

	/* Calculate the register address corresponding to PeriNum */
	RegAddress = &(GPIO->PERILIN_EN[(uint32_t)PeriNum >> 5]);

	/* Calculate the bits offset corresponding to PeriNum */
	BitOffset = PeriNum & 31;

	if (NewState != DISABLE)
	{
		*RegAddress |= ((uint32_t)0x01 << BitOffset);
	}
	else
	{
		*RegAddress &= ~((uint32_t)0x01 << BitOffset);
	}
}

/**
  * @brief Inverts the input signal of peripheral.
  * @param PeriNum: the signal of Peripheral
  * @param NewState: new state of the input signal.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void GPIO_InputPeriInvertCmd(uint8_t PeriNum, FunctionalState NewState)
{
    volatile uint32_t* RegAddress;
    uint32_t BitOffset;

	/* Calculate the offset of register address corresponding to PeriNum */
	RegAddress = &(GPIO->PERILIN_INV[(uint32_t)PeriNum >> 5]);

	/* Calculate the bits of register address corresponding to PeriNum */
	BitOffset = PeriNum & 31;

	if (NewState != DISABLE)
	{
		*RegAddress |= ((uint32_t)0x01 << BitOffset);
	}
	else
	{
		*RegAddress &= ~((uint32_t)0x01 << BitOffset);
	}
}

/**
  * @brief Gets the conflict status of allocation for the specified GPIO.
  * @param PadNum: The GPIO number
  * @retval The conflict status of GPIO.
  */
uint8_t GPIO_ConflictStatusGet(GPIO_PadTypeDef PadNum)
{
	uint8_t  PinRegPos  = (PadNum >= 32) ? 0x01 : 0x00;
    uint32_t PinBitPos =  (PadNum >= 32) ?
	                      ((uint32_t)0x01 << (PadNum & 31)):
	                      ((uint32_t)0x01 << PadNum);

    return ((GPIO->PCFT[PinRegPos] & PinBitPos) ? 1 : 0);
}

/**
  * @brief Gets the allocation status of GPIO.
  * @param PadNum: The GPIO number
  * @retval The allocation status of the specified GPIO
  */
uint8_t GPIO_AllocationStatusGet(GPIO_PadTypeDef PadNum)
{
	uint8_t  PinRegPos  = (PadNum >= 32) ? 0x01 : 0x00;
    uint32_t PinBitPos =  (PadNum >= 32) ?
	                      ((uint32_t)0x01 << (PadNum & 31)):
	                      ((uint32_t)0x01 << PadNum);

    return ((GPIO->PBLKS[PinRegPos] & PinBitPos) ? 1 : 0);
}

/**
  * @brief Enable or disable the Pull-up function of GPIO
  * @param PadNum: The GPIO number
  * @param NewState: new state of the Pull-up function.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void GPIO_PullUpCmd(GPIO_PadTypeDef PadNum, FunctionalState NewState)
{
	uint8_t  PinRegPos  = (PadNum >= 32) ? 0x01 : 0x00;
    uint32_t PinBitPos =  (PadNum >= 32) ?
	                      ((uint32_t)0x01 << (PadNum & 31)):
	                      ((uint32_t)0x01 << PadNum);

	if(NewState != DISABLE)
	{
		GPIO->PULL_UP[PinRegPos] &= ~PinBitPos;
	}
	else
	{
		GPIO->PULL_UP[PinRegPos] |= PinBitPos;
	}
}

/**
  * @brief Enable or disable the Pull-down function of GPIO
  * @param PadNum: the GPIO number
  * @param NewState: new state of the Pull-down function.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void GPIO_PullDownCmd(GPIO_PadTypeDef PadNum, FunctionalState NewState)
{
	uint8_t  PinRegPos  = (PadNum >= 32) ? 0x01 : 0x00;
    uint32_t PinBitPos =  (PadNum >= 32) ?
	                      ((uint32_t)0x01 << (PadNum & 31)):
	                      ((uint32_t)0x01 << PadNum);

	if(NewState != DISABLE)
	{
		GPIO->PULL_DOWN[PinRegPos] &= ~PinBitPos;
	}
	else
	{
		GPIO->PULL_DOWN[PinRegPos] |= PinBitPos;
	}
}

/**
  * @brief Enable or disable the Input function of GPIO
  * @param PadNum: The GPIO number
  * @param NewState: new state of the Input function.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void GPIO_InputCmd(GPIO_PadTypeDef PadNum, FunctionalState NewState)
{
    uint8_t  PinRegPos  = (PadNum >= 32) ? 0x01 : 0x00;
    uint32_t PinBitPos =  (PadNum >= 32) ?
	                      ((uint32_t)0x01 << (PadNum & 31)):
	                      ((uint32_t)0x01 << PadNum);

    if(NewState != DISABLE)
    {
        GPIO->INPUT_EN[PinRegPos]  |= PinBitPos;
    }
    else
    {
        GPIO->INPUT_EN[PinRegPos]  &= ~PinBitPos;
    }
}

/**
  * @brief Enable or disable the Output function of GPIO
  * @param PadNum: The GPIO number
  * @param NewState: new state of the Output function.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void GPIO_OutputCmd(GPIO_PadTypeDef PadNum, FunctionalState NewState)
{
    uint8_t  PinRegPos  = (PadNum >= 32) ? 0x01 : 0x00;
    uint32_t PinBitPos =  (PadNum >= 32) ?
	                      ((uint32_t)0x01 << (PadNum & 31)):
	                      ((uint32_t)0x01 << PadNum);

    if(NewState != DISABLE)
    {
        GPIO->OUTPUT_EN[PinRegPos] &= ~PinBitPos;
    }
    else
    {
        GPIO->OUTPUT_EN[PinRegPos] |= PinBitPos;
    }
}

/**
 * @brief 选择一组GPIO使能上拉或禁能上拉
 * 
 * @param PadArray GPIO_PAD数组
 * @param PadNum   数组元素个数
 * @param NewState DISABLE: 禁能，ENABLE: 使能
 */
void Gpio_PullupConfig(GPIO_PadTypeDef *PadArray, uint8_t PadNum, FunctionalState NewState)
{
    for (uint8_t i = 0; i < PadNum; i++)
    {
        GPIO_PullUpCmd(PadArray[i], NewState);
    }
}

/**
 * @brief 选择一组GPIO使能下拉或禁能下拉
 * 
 * @param PadArray GPIO_PAD数组
 * @param PadNum   数组元素个数
 * @param NewState DISABLE: 禁能，ENABLE: 使能
 */
void Gpio_PulldownConfig(GPIO_PadTypeDef *PadArray, uint8_t PadNum, FunctionalState NewState)
{
    for (uint8_t i = 0; i < PadNum; i++)
    {
        GPIO_PullDownCmd(PadArray[i], NewState);
    }
}

/**
 * @brief 选择一组GPIO使能输入或禁能输入
 * 
 * @param PadArray GPIO_PAD数组
 * @param PadNum   数组元素个数
 * @param NewState DISABLE: 禁能，ENABLE: 使能
 */
void Gpio_InputConfig(GPIO_PadTypeDef *PadArray, uint8_t PadNum, FunctionalState NewState)
{
    for (uint8_t i = 0; i < PadNum; i++)
    {
        GPIO_InputCmd(PadArray[i], NewState);
    }
}

/**
 * @brief 选择一组GPIO使能输出或禁能输出
 * 
 * @param PadArray GPIO_PAD数组
 * @param PadNum   数组元素个数
 * @param NewState DISABLE: 禁能，ENABLE: 使能
 */
void Gpio_OutputConfig(GPIO_PadTypeDef *PadArray, uint8_t PadNum, FunctionalState NewState)
{
    for (uint8_t i = 0; i < PadNum; i++)
    {
        GPIO_OutputCmd(PadArray[i], NewState);
    }
}

extern void DisablePrimask(void);
extern void EnablePrimask(void);

/**
 * @brief 配置GPIO34、37、38为上下拉打开，输入输出失能，规避大功率发射IO口电压波动问题
 */
void Gpio_LeakCurrentEnable(void)
{
    //PRCM_ClockEnable(CORE_CKG_CTL_GPIO_EN);
    DisablePrimask();
    GPIO->MODE[1] &=      ~( (1<<(34-32)) | (1<<(37-32)) | (1<<(38-32)));
    GPIO->CTL[1]  &=      ~( (1<<(34-32)) | (1<<(37-32)) | (1<<(38-32)));
    GPIO->INPUT_EN[1] &=  ~( (1<<(34-32)) | (1<<(37-32)) | (1<<(38-32)));
    GPIO->OUTPUT_EN[1] &= ~( (1<<(34-32)) | (1<<(37-32)) | (1<<(38-32)));
    GPIO->PULL_UP[1] |=    ( (1<<(34-32)) | (1<<(37-32)) | (1<<(38-32)));
    GPIO->PULL_DOWN[1] &= ~( (1<<(34-32)) | (1<<(37-32)) | (1<<(38-32)));
    GPIO->DOUT[1] |=       ( (1<<(34-32)) | (1<<(37-32)) | (1<<(38-32)));
    EnablePrimask();
}

/**
 * @brief  配置GPIO34、37、38为输入下拉，取消漏电配置
 */
void Gpio_LeakCurrentDisable(void)
{
    // 配置GPIO33、34、37为输入下拉
    //PRCM_ClockEnable(CORE_CKG_CTL_GPIO_EN);
    DisablePrimask();
    GPIO->MODE[1] &=      ~( (1<<(34-32)) | (1<<(37-32)) | (1<<(38-32)));
    GPIO->CTL[1]  &=      ~( (1<<(34-32)) | (1<<(37-32)) | (1<<(38-32)));
    GPIO->INPUT_EN[1] |=   ( (1<<(34-32)) | (1<<(37-32)) | (1<<(38-32)));
    GPIO->OUTPUT_EN[1] |=  ( (1<<(34-32)) | (1<<(37-32)) | (1<<(38-32)));
    GPIO->PULL_UP[1] |=    ( (1<<(34-32)) | (1<<(37-32)) | (1<<(38-32)));
    GPIO->PULL_DOWN[1] &= ~( (1<<(34-32)) | (1<<(37-32)) | (1<<(38-32)));
    EnablePrimask();
}

/**
  * @brief Enable or disable the Analog function of GPIO.
  * @param PadNum: the GPIO number
  * @param NewState: new state of the  Analog function.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void GPIO_AnalogCmd(GPIO_PadTypeDef PadNum, FunctionalState NewState)
{
	uint8_t  PinRegPos  = (PadNum >= 32) ? 0x01 : 0x00;
    uint32_t PinBitPos =  (PadNum >= 32) ?
	                      ((uint32_t)0x01 << (PadNum & 31)):
	                      ((uint32_t)0x01 << PadNum);

	if(NewState != DISABLE)
    {
        GPIO->ANAE[PinRegPos] &= ~PinBitPos;
    }
    else
    {
        GPIO->ANAE[PinRegPos] |= PinBitPos;
    }
}

/**
  * @brief Enable or disable the interrupt sources of GPIO.
  * @param PadNum: The GPIO number
  * @param NewState: new state of the interrupt.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void GPIO_IntCmd(GPIO_PadTypeDef PadNum, FunctionalState NewState)
{
	uint8_t  PinRegPos  = (PadNum >= 32) ? 0x01 : 0x00;
    uint32_t PinBitPos =  (PadNum >= 32) ?
	                      ((uint32_t)0x01 << (PadNum & 31)):
	                      ((uint32_t)0x01 << PadNum);
    if(NewState != DISABLE)
    {
        GPIO->INTEN[PinRegPos] |= PinBitPos;
    }
    else
    {
        GPIO->INTEN[PinRegPos] &= ~PinBitPos;
    }

}

/**
  * @brief Enable or disable the interrupt mask for the specified GPIO.
  * @param PadNum: the GPIO number
  * @param NewState: new state of the interrupt mask.
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void GPIO_IntMaskCmd(GPIO_PadTypeDef PadNum, FunctionalState NewState)
{
	uint8_t  PinRegPos  = (PadNum >= 32) ? 0x01 : 0x00;
    uint32_t PinBitPos =  (PadNum >= 32) ?
	                      ((uint32_t)0x01 << (PadNum & 31)):
	                      ((uint32_t)0x01 << PadNum);

	if(NewState != DISABLE)
	{
		GPIO->INTM[PinRegPos] |= PinBitPos;
	}
	else
	{
		GPIO->INTM[PinRegPos] &= ~PinBitPos;
	}

}

/**
  * @brief Gets the current interrupt status of GPIO.
  * @param PadNum: the GPIO number	
  * @retval The current interrupt status.
  */
uint8_t GPIO_GetIntStatus(GPIO_PadTypeDef PadNum)
{	
	uint8_t  PinRegPos  = (PadNum >= 32) ? 0x01 : 0x00;
    uint32_t PinBitPos =  (PadNum >= 32) ? 
	                      ((uint32_t)0x01 << (PadNum & 31)):
	                      ((uint32_t)0x01 << PadNum);
	

	return ((GPIO->RINTS[PinRegPos] & PinBitPos) ? 1 : 0);	
}

/**
  * @brief Gets the current interrupt status of all GPIOs.
  * @param PadNum: the GPIO number	
  * @retval The current interrupt status.
  */
uint64_t GPIO_GetAllIntStatus(void)
{	
	return ((uint64_t)GPIO->RINTS[1] << 32 | GPIO->RINTS[0]);	
}

/**
  * @brief Clear the current interrupt status of GPIO.
  * @param PadNum: the GPIO number
  * @retval None
  */
void GPIO_IntStatusClear(GPIO_PadTypeDef PadNum)
{
	uint8_t  PinRegPos  = (PadNum >= 32) ? 0x01 : 0x00;
    uint32_t PinBitPos =  (PadNum >= 32) ? 
	                      ((uint32_t)0x01 << (PadNum & 31)):
	                      ((uint32_t)0x01 << PadNum);
	

	GPIO->RINTS[PinRegPos] = PinBitPos;
}

/**
  * @brief Clear the current interrupt status of GPIO.
  * @param PadNum: the GPIO number
  * @retval None
  */
void GPIO_ClearAllIntStatus(void)
{
	GPIO->RINTS[0] = 0xFFFFFFFF;
	GPIO->RINTS[1] = 0xFFFFFFFF;
}

void ADC_GPIO_Init(GPIO_PadTypeDef PadNum)
{
    //only GPIO8~GPIO13 can be used as ADC
    if( (PadNum < GPIO_PAD_NUM_8) || (PadNum > GPIO_PAD_NUM_13) )
    {
        return;
    }

    uint8_t PinRegPos = (PadNum >= 32) ? 0x01 : 0x00;
    uint32_t PinBitPos = (PadNum >= 32) ? ((uint32_t)0x01 << (PadNum & 31)) : ((uint32_t)0x01 << PadNum);

    //GPIO is configured as SW_MODE、controled by GPIO register 
    GPIO->MODE[PinRegPos] &= ~PinBitPos;
    GPIO->CTL[PinRegPos]  &= ~PinBitPos;
    GPIO->PERI[PadNum] = 0xFF;

    //GPIO input and output functions are disabled 
    GPIO->INPUT_EN[PinRegPos]  &= ~PinBitPos;
    GPIO->OUTPUT_EN[PinRegPos] |= PinBitPos;

    //GPIO pull-up and pull-down functions are disabled
    GPIO->PULL_DOWN[PinRegPos] |= PinBitPos;
    GPIO->PULL_UP[PinRegPos] |= PinBitPos;
}

/**
  * @brief Enable lcd mode for all the pad and prevent current backflow 
  * @note  The following gpio are involved
  *		   SEG[0]~[27]: GPIO27~54;  COM[0]~[7]: GPIO14,20~26
  * @param None
  * @retval None
  */
void GPIO_AllPad_Enable_Rcr(void)
{
	LCDC->LCD_SEG_PAD_CTRL = 0xFFFFFFF; //SEG[0]~[27]: GPIO27~54
	LCDC->LCD_COM_PAD_CTRL = 0xFF; //COM[0]~[7]: GPIO14,20~26			
	
	AONPRCM->AONPWR_CTRL &= ~(0x30);  
	AONPRCM->AONPWR_CTRL |= (0x01<<4);//force on lcd pwr for keep the configure of lcdc 
	LCDC->UVOL &= ~0x03;  // vlcd select external
	LCDC->FCR |= 0x08;    // lcd_pu = 1
	LCDC->UVOL &= ~0x80;  // supply_cnt = 0;	
}

/**
  * @brief Disable lcd mode for all the pad as general IO 
  * @param None
  * @retval None
  */
void GPIO_AllPad_Disable_Rcr(void)
{
	LCDC->LCD_SEG_PAD_CTRL = 0x00; //default
	LCDC->LCD_COM_PAD_CTRL = 0x00; //default			
	
	AONPRCM->AONPWR_CTRL &= ~(0x30);  //default, off in DEEPSLEEP
	LCDC->UVOL |= 0x02;    // default, vlcd select LCP	
	LCDC->UVOL |= 0x80;    // default, supply_cnt = 1;
	LCDC->FCR &= ~0x08;    // default, lcd_pu = 0	
}

/**
  * @brief Enable lcd mode of the lcd seg pad
  * @param Seg_BitPos: the bit mask when gpio is used as LCD_SEG function
  *        This parameter can be any combination of the following values:    
  *        @arg 0x0000001: SEG[0](GPIO27)
  *        @arg 0x0000002: SEG[1](GPIO28)
  *        @arg 0x0000004: SEG[2](GPIO29)
  *        @arg 0x0000008: SEG[3](GPIO30)
  *                      ...
  *        @arg 0x1000000: SEG[24](GPIO51)
  *        @arg 0x2000000: SEG[25](GPIO52)
  *        @arg 0x4000000: SEG[26](GPIO53)
  *        @arg 0x8000000: SEG[27](GPIO54)  
  * @retval None
  */
void GPIO_SegPad_Enable_LcdMode(uint32_t SegPos)
{
	LCDC->LCD_SEG_PAD_CTRL |= SegPos;	
}

/**
  * @brief Disable lcd mode of the lcd seg pad
  * @param Seg_BitPos: the bit mask when gpio is used as LCD_SEG function
  *        This parameter can be any combination of the following values:    
  *        @arg 0x0000001: SEG[0](GPIO27)
  *        @arg 0x0000002: SEG[1](GPIO28)
  *        @arg 0x0000004: SEG[2](GPIO29)
  *        @arg 0x0000008: SEG[3](GPIO30)
  *                      ...
  *        @arg 0x1000000: SEG[24](GPIO51)
  *        @arg 0x2000000: SEG[25](GPIO52)
  *        @arg 0x4000000: SEG[26](GPIO53)
  *        @arg 0x8000000: SEG[27](GPIO54)   
  * @retval None
  */
void GPIO_SegPad_Disable_LcdMode(uint8_t SegPos)
{
	LCDC->LCD_SEG_PAD_CTRL &= ~SegPos;
}

/**
  * @brief Enable lcd mode of the lcd com pad
  * @param ComPos: the bit mask when gpio is used as LCD_COM function
  *        This parameter can be any combination of the following values: 
  *        @arg 0x01: COM[0](GPIO14)
  *        @arg 0x02: COM[1](GPIO20)
  *        @arg 0x04: COM[2](GPIO21)
  *        @arg 0x08: COM[3](GPIO22)
  *        @arg 0x10: COM[4](GPIO23)
  *        @arg 0x20: COM[5](GPIO24)
  *        @arg 0x40: COM[6](GPIO25)
  *        @arg 0x80: COM[7](GPIO26)
  * @retval None
  */
void GPIO_ComPad_Enable_LcdMode(uint8_t ComPos)
{
	LCDC->LCD_COM_PAD_CTRL |= ComPos;	
}

/**
  * @brief Disable lcd mode of the lcd com pad
  * @param ComPos: the bit mask when gpio is used as LCD_COM function
  *        This parameter can be any combination of the following values: 
  *        @arg 0x01: COM[0](GPIO14)
  *        @arg 0x02: COM[1](GPIO20)
  *        @arg 0x04: COM[2](GPIO21)
  *        @arg 0x08: COM[3](GPIO22)
  *        @arg 0x10: COM[4](GPIO23)
  *        @arg 0x20: COM[5](GPIO24)
  *        @arg 0x40: COM[6](GPIO25)
  *        @arg 0x80: COM[7](GPIO26)
  * @retval None
  */
void GPIO_ComPad_Disable_LcdMode(uint8_t ComPos)
{
	LCDC->LCD_COM_PAD_CTRL &= ~ComPos;
}

/**
  * @brief Enable the pad prevent current backflow 
  * @note  This function should be called after GPIO_SegPad_Enable_LcdMode or GPIO_ComPad_Enable_LcdMode
  * @param None
  * @retval None
  */
void GPIO_Pad_Enable_Rcr(void)
{
	AONPRCM->AONPWR_CTRL &= ~(0x30);  
	AONPRCM->AONPWR_CTRL |= (0x01<<4);//force on lcd pwr for keep the configure of lcdc 
	LCDC->UVOL &= ~0x03;  // vlcd select external
	LCDC->FCR |= 0x08;    // lcd_pu = 1
	LCDC->UVOL &= ~0x80;  // supply_cnt = 0;	
}

/**
  * @brief Disable the pad prevent current backflow 
  * @note This function should be called after GPIO_SegPad_Disable_LcdMode or GPIO_ComPad_Disable_LcdMode 
  * @param None
  * @retval None
  */
void GPIO_Pad_Disable_Rcr(void)
{
	AONPRCM->AONPWR_CTRL &= ~(0x30);  //default, off in DEEPSLEEP
	LCDC->UVOL |= 0x02;    // default, vlcd select LCP	
	LCDC->UVOL |= 0x80;    // default, supply_cnt = 1;
	LCDC->FCR &= ~0x08;    // default, lcd_pu = 0		
}

