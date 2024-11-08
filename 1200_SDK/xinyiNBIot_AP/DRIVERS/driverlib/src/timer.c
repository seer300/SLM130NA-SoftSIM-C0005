#include "timer.h"
#include "interrupt.h"

/**
  * @brief Enables the timer.
  * @param ulBase: the base address of the timer
  * @retval None.
  */
void TimerEnable(unsigned long ulBase)
{
    HWREG(ulBase + TIMER_CTL) |= TIMER_CTL_TEN_EN;
}

/**
  * @brief Disables the timer.
  * @param ulBase: the base address of the timer
  * @retval None.
  */
void TimerDisable(unsigned long ulBase)
{
    HWREG(ulBase + TIMER_CTL) &= ~(TIMER_CTL_TEN_EN);
}

/**
  * @brief Configures the timer.
  * @param ulBase: the base address of the timer
  * @param ulConfig: the configuration for timer
  *    This parameter can be any combination of the following values:
  *     @arg TIMER_CTL_TMODE_ONE_SHOT
  *     @arg TIMER_CTL_TMODE_CONTINUOUS
  *     @arg TIMER_CTL_TMODE_COUNTER
  *     @arg TIMER_CTL_TMODE_PWM_SINGLE
  *     @arg TIMER_CTL_TMODE_CAPTURE
  *     @arg TIMER_CTL_TMODE_COMPARE
  *     @arg TIMER_CTL_TMODE_GATED
  *     @arg TIMER_CTL_TMODE_CAP_AND_CMP
  *     @arg TIMER_CTL_TMODE_PWM_DUAL
  *
  *     @arg TIMER_CTL_PRES_DIVIDE_1
  *     @arg TIMER_CTL_PRES_DIVIDE_2
  *     @arg TIMER_CTL_PRES_DIVIDE_4
  *     @arg TIMER_CTL_PRES_DIVIDE_8
  *     @arg TIMER_CTL_PRES_DIVIDE_16
  *     @arg TIMER_CTL_PRES_DIVIDE_32
  *     @arg TIMER_CTL_PRES_DIVIDE_64
  *     @arg TIMER_CTL_PRES_DIVIDE_128
  *
  *     @arg TIMER_CTL_TPOL_TRUE: capture/count at falling,initial output low
  *     @arg TIMER_CTL_TPOL_FALSE: capture/count at rising,initial output high
  *
  *     This is only set in PWM-Dual mode
  *     @arg TIMER_CTL_PWMD_NO_DELAY
  *     @arg TIMER_CTL_PWMD_DELAY_2_CYCLE
  *     @arg TIMER_CTL_PWMD_DELAY_4_CYCLE
  *     @arg TIMER_CTL_PWMD_DELAY_8_CYCLE
  *     @arg TIMER_CTL_PWMD_DELAY_16_CYCLE
  *     @arg TIMER_CTL_PWMD_DELAY_32_CYCLE
  *     @arg TIMER_CTL_PWMD_DELAY_64_CYCLE
  *     @arg TIMER_CTL_PWMD_DELAY_128_CYCLE
  *
  *     This is only set in Capture mode
  *     @arg TIMER_CTL_RSTCNT_EN: reset counter
  *     @arg TIMER_CTL_RSTCNT_DIS: not reset counter
  * @retval None
  */
void TimerConfigure(unsigned long ulBase, unsigned long ulConfig)
{
    /* Disable the timers */
    HWREG(ulBase + TIMER_CTL) &= ~(TIMER_CTL_TEN_EN);

    /* Set the global timer configuration */
    HWREG(ulBase + TIMER_CTL) = ( HWREG(ulBase + TIMER_CTL) & 
	                            ~(TIMER_CTL_PWMD_Msk | TIMER_CTL_RSTCNT_Msk | 
	                              TIMER_CTL_TMODE_Msk | TIMER_CTL_PRES_Msk | 
	                              TIMER_CTL_TPOL_Msk )) | ulConfig;
}

/**
  * @brief Sets the initial count value of specified timer.
  * @note This function sets the timer count value; if the timer is running   
  *   then the value will be immediately loaded into the timer.
  * @param ulBase: the base address of the timer
  * @param ulValue: the initial count value
  * @retval None
  */
void TimerInitCountValueSet(unsigned long ulBase, unsigned long ulValue)
{
    HWREG(ulBase + TIMER_CNT) = ulValue;
}

/**
  * @brief Sets the timer reload value.
  * @param ulBase: the base address of the timer
  * @param ulValue: the reload value
  * @retval None.
  */
void TimerReloadValueSet(unsigned long ulBase, unsigned long ulValue)
{
    HWREG(ulBase + TIMER_RLD) = ulValue;
}

/**
  * @brief Sets the timer PWM value.
  * @param ulBase: the base address of the timer
  * @note In PWM SINGLE OUTPUT and PWM DUAL OUTPUT mode when the count value
  *   of timer reaching the PWM value,the output signal of timer toggles. 
  * @param ulValue: the PWM value
  * @retval None.
  */
void TimerPWMValueSet(unsigned long ulBase, unsigned long ulValue)
{
    HWREG(ulBase + TIMER_PWM) = ulValue;
}

/**
  * @brief Set the initial output level.
  * @param ulBase: the base address of the timer
  * @param ucInvert: specifies the initial output level
  *     @arg true: high
  *     @arg false: low  
  * @retval None.
  */
void TimerPolaritySet(unsigned long ulBase, unsigned char ucInvert)
{
    HWREG(ulBase + TIMER_CTL) = ((ucInvert) ?
                                (HWREG(ulBase + TIMER_CTL) | TIMER_CTL_TPOL_Msk) :
                                (HWREG(ulBase + TIMER_CTL) & ~(TIMER_CTL_TPOL_Msk)));
}

/**
  * @brief Set the timer prescale value.
  * @param ulBase: the base address of the timer
  * @param ulValue: the prescale value.
  *   This parameter is one of the following:
  *     @arg TIMER_CTL_PRES_DIVIDE_1
  *     @arg TIMER_CTL_PRES_DIVIDE_2
  *     @arg TIMER_CTL_PRES_DIVIDE_4
  *     @arg TIMER_CTL_PRES_DIVIDE_8
  *     @arg TIMER_CTL_PRES_DIVIDE_16
  *     @arg TIMER_CTL_PRES_DIVIDE_32
  *     @arg TIMER_CTL_PRES_DIVIDE_64
  *     @arg TIMER_CTL_PRES_DIVIDE_128
  * @retval None.
  */
void TimerPrescaleSet(unsigned long ulBase, unsigned long ulValue)
{
    HWREG(ulBase + TIMER_CTL) = (HWREG(ulBase + TIMER_CTL) & ~(TIMER_CTL_PRES_Msk)) | ulValue;
}

/**
  * @brief Sets the PWM delay value(PWM-Dual mode only).
  * @note This function sets the value of the PWM delay to control the number of
  *   system clock cycles delay before the PWM Output and PWM Output Complement
  *   are forced to their active state. 
  * @param ulBase: the base address of the timer
  * @param ulValue: the PWM delay value.
  *   This parameter is one of the following:
  *     @arg TIMER_CTL_PWMD_NO_DELAY
  *     @arg TIMER_CTL_PWMD_DELAY_2_CYCLE
  *     @arg TIMER_CTL_PWMD_DELAY_4_CYCLE
  *     @arg TIMER_CTL_PWMD_DELAY_8_CYCLE
  *     @arg TIMER_CTL_PWMD_DELAY_16_CYCLE
  *     @arg TIMER_CTL_PWMD_DELAY_32_CYCLE
  *     @arg TIMER_CTL_PWMD_DELAY_64_CYCLE
  *     @arg TIMER_CTL_PWMD_DELAY_128_CYCLE
  * @retval None.
  */
void TimerPWMDelaySet(unsigned long ulBase, unsigned long ulValue)
{
    HWREG(ulBase + TIMER_CTL) = (HWREG(ulBase + TIMER_CTL) & ~(TIMER_CTL_PWMD_Msk)) | ulValue;
}

/**
  * @brief Sets the compensation value for the current timer counting.
  * @note This only takes effect at the reg writing cycle.
  * @param ulBase: the base address of the timer
  * @param ulOffsetDirect: the shift direction of the counter
  *   This parameter is one of the following:
  *     @arg TIMER_CNT_DIR_UP: add (ulValue + 1) to counter
  *     @arg TIMER_CNT_DIR_DOWN: subtract (ulValue + 1) to counter
  * @param ulValue: the offset value for current countting. This value should
  *   be less than Reload value.
  * @retval None.
  */
void TimerCountOffset(unsigned long ulBase, unsigned long ulOffsetDirect, unsigned long ulValue)
{
    HWREG(ulBase + TIMRE_CNT_OFFSET) = ulOffsetDirect| ulValue;
}

/**
  * @brief Gets the current count value of specified timer.
  * @note This function reads the current value of the specified timer.
  * @param ulBase: the base address of the timer
  * @retval The current count value.
  */
unsigned long TimerCountValueGet(unsigned long ulBase)
{
    return(HWREG(ulBase + TIMER_CNT));
}

/**
  * @brief Gets the timer reload value.
  * @param ulBase: the base address of the timer
  * @retval the reload value.
  */
unsigned long TimerReloadValueGet(unsigned long ulBase)
{
    return(HWREG(ulBase + TIMER_RLD));
}

/**
  * @brief Gets the timer capture value.
  * @param ulBase: the base address of the timer
  * @retval the PWM value.
  */
unsigned long TimerCaptureValueGet(unsigned long ulBase)
{
    return(HWREG(ulBase + TIMER_PWM));
}

/**
  * @brief Get the initial output level.
  * @param ulBase: the base address of the timer
  * @retval Returns true if the initial output level is high.
  */
unsigned char TimerPolarityGet(unsigned long ulBase)
{

   return ((HWREG(ulBase + TIMER_CTL) & TIMER_CTL_TPOL_Msk) ? true : false);
}

/**
  * @brief Registers an interrupt handler for the timer interrupt.
  * @param ulIntChannel: the interrupt assignments of Timer
  * @param g_pRAMVectors: the global interrupt vectors table
  * @param pfnHandler: a pointer to the function to be called when 
  *        the Timer interrupt occurs. 
  * @retval None
  */
void TimerIntRegister(unsigned long ulIntChannel, unsigned long *g_pRAMVectors, void (*pfnHandler)(void))
{
	IntRegister(ulIntChannel, g_pRAMVectors, pfnHandler);

	IntEnable(ulIntChannel);
}

/**
  * @brief Unregisters an interrupt handler for the timer interrupt.
  * @param ulIntChannel: the interrupt assignments of Timer
  * @param g_pRAMVectors£ºthe global interrupt vectors table
  * @retval None
  */
void TimerIntUnregister(unsigned long ulIntChannel, unsigned long *g_pRAMVectors)
{
	IntDisable(ulIntChannel);

	IntUnregister(ulIntChannel, g_pRAMVectors);
}

/**
  * @brief Enables individual timer interrupt sources.
  * @param ulBase: the base address of the timer
  * @param ulIntFlags: the bit mask of the interrupt sources to be enabled.
  *   This parameter is one of the following:
  *     @arg TIMER_CTL_TICONFIG_OUTER_EVENT, Capture/Deassertion events interrupt
  *     @arg TIMER_CTL_TICONFIG_INNER_EVENT, Reload/Compare events interrupt
  *     @arg TIMER_CTL_TICONFIG_ALL_EVENT, Reload, Compare, Capture, Deassertion events interrupt
  * @retval None.
  */
void TimerIntEnable(unsigned long ulBase, unsigned long ulIntFlags)
{
    HWREG(ulBase + TIMER_CTL) = (HWREG(ulBase + TIMER_CTL) & ~(TIMER_CTL_TICONFIG_Msk)) | ulIntFlags;
}

/**
  * @brief Determines whether the current interrupt is caused by a capture event.
  * @param ulBase: the base address of the timer
  * @retval Returns 1 if the interrupt is caused by a capture event 
  *   or 0 if the interrupt is caused by a reload event
  */
unsigned char TimerIntEventGet(unsigned long ulBase)
{
    return((HWREG(ulBase + TIMER_CTL) & TIMER_CTL_INPCAP_Msk) ? 1 : 0);
}


