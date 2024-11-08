
/**
 ******************************************************************************
 * @file    hal_gpio.c
 * @brief   HAL库GPIO
 ******************************************************************************
 */

#include "hal_gpio.h"
#include "hal_def.h"
#include "nvic.h"

static void HAL_GPIO_IRQHandler(void);

//所有外设输入引脚的remap
#define INPUT_PERI_SELECT_REMAP1 (1ULL << GPIO_UART2_RXD | 1ULL << GPIO_UART2_CTS | 1ULL << GPIO_CSP1_SCLK | 1ULL << GPIO_CSP1_RXD | 1ULL << GPIO_CSP1_RFS | \
                                  1ULL << GPIO_CSP2_SCLK | 1ULL << GPIO_CSP2_RXD | 1ULL << GPIO_CSP2_RFS | 1ULL << GPIO_I2C1_SCL | 1ULL << GPIO_I2C1_SDA | \
                                  1ULL << GPIO_SPI_SCLK | 1ULL << GPIO_SPI_MOSI | 1ULL << GPIO_SPI_MISO | 1ULL << GPIO_SPI_CS_N | \
                                  1ULL << GPIO_TMR1Gate | 1ULL << GPIO_TMR2Gate | 1ULL << GPIO_TMR3Gate | 1ULL << GPIO_TMR4Gate | 1ULL << GPIO_LPTMR1_EXTCLK | 1ULL << GPIO_LPTMR1Gate | \
                                  1ULL << GPIO_AP_SWCLKTCK | 1ULL << GPIO_AP_SWDIOTMS | 1ULL << GPIO_CP_SWCLKTCK | 1ULL << GPIO_CP_SWDIOTMS | \
                                  1ULL << GPIO_UART1_CTS | 1ULL << GPIO_SM_CLK | 1ULL << GPIO_SM_RST | 1ULL << GPIO_SM_SIO | 1ULL << GPIO_I2C2_SCL | 1ULL << GPIO_I2C2_SDA | \
                                  1ULL << GPIO_CSP3_SCLK | 1ULL << GPIO_CSP3_RXD | 1ULL << GPIO_CSP3_RFS | 1ULL << GPIO_CSP4_SCLK | 1ULL << GPIO_CSP4_RXD | 1ULL << GPIO_CSP4_RFS | \
                                  1ULL << GPIO_UHWD_RXD | 1ULL << GPIO_UART1_RXD | 1ULL << GPIO_ABD_RXD)
#define INPUT_PERI_SELECT_REMAP2 (1ULL << (GPIO_LPTMR2_EXTCLK - 64) | 1ULL << (GPIO_LPTMR2Gate - 64))

//所有CSP输入引脚的remap
#define CSP_RXD (1ULL << GPIO_CSP1_RXD | 1ULL << GPIO_CSP2_RXD | 1ULL << GPIO_CSP3_RXD | 1ULL << GPIO_CSP4_RXD)

/**
 * @brief  初始化GPIO.
 *
 * @param  hgpio. 详情参考 @ref HAL_GPIO_InitTypeDef.
 * @return 函数执行状态.详情参考 @ref HAL_StatusTypeDef
 *         返回值可能是以下类型：
 *         @retval  HAL_OK    ：表示GPIO初始化成功
 *         @retval  HAL_ERROR ：表示入参错误
 */
__RAM_FUNC HAL_StatusTypeDef HAL_GPIO_Init(HAL_GPIO_InitTypeDef *hgpio)
{
    uint64_t is_peri_inputpin = 0;

    if (hgpio == NULL)
	{
		return HAL_ERROR;
	}

    PRCM_ClockEnable(CORE_CKG_CTL_GPIO_EN);

    //如果配置的是gpio3、4，则关闭lpuart，释放gpio3、4
    if(hgpio->Pin == GPIO_PAD_NUM_3 || hgpio->Pin == GPIO_PAD_NUM_4)
    {
        AONPRCM->LPUA1_CTRL &= ~0X03;
    }

    GPIO_Init(hgpio, FAST_SPEED);

    //GPIO被配置成硬件外设模式或者软件外设输入模式，则对外设输入映射进行操作
    if((hgpio->Mode == GPIO_MODE_HW_PER) || (hgpio->Mode == GPIO_MODE_SW_PER_INPUT))
    {
        //如果PinRemap是CSP_RXD，则配成SW_PER_INPUT、PULL_UP
        if ((CSP_RXD & (uint64_t)(1ULL << hgpio->PinRemap)))
        {
            uint8_t CurrentPin = 0x00, CurrentPeri = 0x00;
            uint32_t PinRegPos = 0x00, PinBitPos = 0x00;

            CurrentPin = hgpio->Pin;
            CurrentPeri = hgpio->PinRemap;
            PinRegPos  = (CurrentPin >= 32) ? 0x01 : 0x00;
            PinBitPos =  (CurrentPin >= 32) ? ((uint32_t)0x01 << (CurrentPin & 31)) : ((uint32_t)0x01 << CurrentPin);

            //hardware mode(register control)
            GPIO->MODE[PinRegPos] |= PinBitPos;
            GPIO->PERI[CurrentPin] =  CurrentPeri;
            GPIO->PULL_UP_SEL[PinRegPos] |= PinBitPos;
            GPIO->CTL[PinRegPos]  &= ~PinBitPos;
            //input
            GPIO->INPUT_EN[PinRegPos]  |= PinBitPos;
            GPIO->OUTPUT_EN[PinRegPos] |= PinBitPos;
            //pull_up
            GPIO->PULL_UP[PinRegPos]   &= ~PinBitPos;
            GPIO->PULL_DOWN[PinRegPos] |= PinBitPos;
        }

        //GPIO为外设输入映射则需要选择GPIO并使能
        is_peri_inputpin = (hgpio->PinRemap >= 64) ? (INPUT_PERI_SELECT_REMAP2 & (uint64_t)(1ULL << (hgpio->PinRemap - 64)))
                                                   : (INPUT_PERI_SELECT_REMAP1 & (uint64_t)(1ULL << hgpio->PinRemap));
        if (is_peri_inputpin)
        {
            GPIO_InputPeriSelect(hgpio->Pin, hgpio->PinRemap);
            GPIO_InputPeriSelectCmd(hgpio->PinRemap, HAL_ENABLE);
        }
    }

    //GPIO被配置成中断模式则注册中断
    if (hgpio->Int != (GPIO_IntTypeDef)0x00)
    {
        NVIC_IntRegister(GPIO_IRQn, HAL_GPIO_IRQHandler, 1);
    }

    return HAL_OK;
}

/**
 * @brief 设置指定GPIO的上下拉状态
 * 
 * @param GPIO_Pin GPIO_Pin 指定的GPIO引脚.详情参考 @ref HAL_GPIO_PinTypeDef.
 * @param pull 上下拉状态.详情参考 @ref HAL_GPIO_PullTypeDef.
 */
__RAM_FUNC void HAL_GPIO_SetPull(HAL_GPIO_PinTypeDef GPIO_Pin, HAL_GPIO_PullTypeDef pull)
{
    GPIO_SetPull(GPIO_Pin, pull);
}

/**
 * @brief  获取指定GPIO的电平状态.
 *
 * @param  GPIO_Pin 指定的GPIO引脚.详情参考 @ref HAL_GPIO_PinTypeDef.
 * @return 引脚状态.详情参考 @ref HAL_GPIO_StateTypeDef.
 *         参数可以是以下枚举值:
 *		   @retval GPIO_PIN_RESET : 指定GPIO为低电平
 *		   @retval GPIO_PIN_SET   : 指定GPIO为高电平
 */
inline __RAM_FUNC HAL_GPIO_StateTypeDef HAL_GPIO_ReadPin(HAL_GPIO_PinTypeDef GPIO_Pin)
{
    return GPIO_Read_InOutPin(GPIO_Pin);
}

/**
 * @brief  设置指定GPIO输出状态.
 *
 * @param  GPIO_Pin 指定的GPIO引脚.详情参考 @ref HAL_GPIO_PinTypeDef.
 * @param  PinState 设置的输出状态.详情参考 @ref HAL_GPIO_StateTypeDef.
 *         参数可以是以下枚举值:
 *		   @arg GPIO_PIN_RESET : 指定GPIO输出低电平
 *		   @arg GPIO_PIN_SET   : 指定GPIO输出高电平
 */
inline __RAM_FUNC void HAL_GPIO_WritePin(HAL_GPIO_PinTypeDef GPIO_Pin, HAL_GPIO_StateTypeDef PinState)
{
    GPIO_WritePin(GPIO_Pin, PinState);
}

/**
 * @brief  同时设置多个GPIO输出状态.
 *
 * @param  GPIO_PinArray 指向的一组GPIO引脚.详情参考 @ref HAL_GPIO_PinTypeDef.
 * @param  PinNum GPIO个数
 * @param  PinState 设置的输出状态.详情参考 @ref HAL_GPIO_StateTypeDef.
 *         参数可以是以下枚举值:
 *		   @arg GPIO_PIN_RESET : 指定GPIO输出低电平
 *		   @arg GPIO_PIN_SET   : 指定GPIO输出高电平
 */
inline __RAM_FUNC void HAL_GPIO_WritePinArray(HAL_GPIO_PinTypeDef *GPIO_PinArray, uint8_t PinNum, HAL_GPIO_StateTypeDef PinState)
{
    for (uint8_t i = 0; i < PinNum; i++)
    {
        GPIO_WritePin(GPIO_PinArray[i], PinState);
    }
}

/**
 * @brief  翻转指定GPIO输出状态.
 *
 * @param  GPIO_Pin 指定的GPIO引脚.详情参考 @ref HAL_GPIO_PinTypeDef.
 * @note   此API只能用于GPIO为输出模式
 */
inline __RAM_FUNC void HAL_GPIO_TogglePin(HAL_GPIO_PinTypeDef GPIO_Pin)
{
    GPIO_TogglePin(GPIO_Pin);
}

/**
 * @brief  翻转指定AGPIO输出状态.
 *
 * @param  AGPIO_Pin 指定的AGPIO引脚.
 * @note   此API只能用于AGPIO为输出模式
 */
inline __RAM_FUNC void HAL_AGPIO_TogglePin(uint8_t AGPIO_Pin)
{
    AGPIO_TogglePin(AGPIO_Pin);
}

/**
 * @brief  读取并清除所有GPIO中断值.
 *
 * @return GPIO中断状态寄存器的值
 */
inline __RAM_FUNC uint64_t HAL_GPIO_ReadAndClearIntFlag(void)
{
    volatile uint64_t int_status = GPIO_GetAllIntStatus();
    GPIO_ClearAllIntStatus();
    return int_status;
}

/**
 * @brief  读取GPIO中断状态.
 *
 * @return GPIO中断状态标志，
 */
inline uint8_t HAL_GPIO_ReadIntFlag(HAL_GPIO_PinTypeDef GPIO_Pin)
{
    return GPIO_GetIntStatus(GPIO_Pin);
}


/**
 * @brief  清除所有GPIO中断状态标志.
 */
inline void HAL_GPIO_ClearIntFlag(HAL_GPIO_PinTypeDef GPIO_Pin)
{
    GPIO_IntStatusClear(GPIO_Pin);
}


/**
 * @brief GPIO0-GPIO7的控制寄存器从AON区域切出，以便能重新控制GPIO0-GPIO7.
 *
 * @note 深睡唤醒时需要调用，否则唤醒后无法控制GPIO0-GPIO7.
 */
inline __RAM_FUNC void HAL_AGPIO0_7_UPDATE(void)
{
    AGPIO_GPIO0_7_Update();
}



/**
 * @brief  GPIO中断服务函数.
 */
static __RAM_FUNC void HAL_GPIO_IRQHandler(void)
{
    HAL_GPIO_InterruptCallback();
}
