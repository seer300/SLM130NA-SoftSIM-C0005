#include "lptimer.h"
#include "interrupt.h"

/**
  * @brief Enables the lptimer.
  * @param ulBase: the base address of the lptimer
  * @retval None.
  */
void LPTimerEnable(unsigned long ulBase)
{
    HWREG(ulBase + LPTIMER_CTL) |= LPTIMER_CTL_TEN_EN;
}

/**
  * @brief Disables the lptimer.
  * @param ulBase: the base address of the lptimer
  * @retval None.
  */
void LPTimerDisable(unsigned long ulBase)
{
    HWREG(ulBase + LPTIMER_CTL) &= ~(LPTIMER_CTL_TEN_EN);
}

/**
  * @brief Enable the cascade function of lptimer.
  * @note This function make two 16bit lptimers into a 32bit lptimer 
  *       and lptimer1 work as the master timer, lptimer2 as high 
  *       order level of timer counter   
  * @param ulBase: the base address of the lptimer
  * @retval None.
  */
void LPTimerDualEnable(void)
{
    HWREG(LPTIMER1_BASE + LPTIMER_CTL) |= LPTIMER_CTL_DUAL_EN;   
	HWREG(LPTIMER2_BASE + LPTIMER_CTL) |= LPTIMER_CTL_DUAL_EN;	
}

/**
  * @brief Configures the lptimer.
  * @param ulBase: the base address of the timer
  * @param ulConfig: the configuration for timer
  *    This parameter can be any combination of the following values:
  *     @arg LPTIMER_CTL_TMODE_ONE_SHOT
  *     @arg LPTIMER_CTL_TMODE_CONTINUOUS
  *     @arg LPTIMER_CTL_TMODE_COUNTER
  *     @arg LPTIMER_CTL_TMODE_PWM_SINGLE
  *     @arg LPTIMER_CTL_TMODE_CAPTURE
  *     @arg LPTIMER_CTL_TMODE_COMPARE
  *     @arg LPTIMER_CTL_TMODE_GATED
  *     @arg LPTIMER_CTL_TMODE_CAP_AND_CMP
  *     @arg LPTIMER_CTL_TMODE_PWM_DUAL
  *
  *     @arg LPTIMER_CTL_PRES_DIVIDE_1
  *     @arg LPTIMER_CTL_PRES_DIVIDE_2
  *     @arg LPTIMER_CTL_PRES_DIVIDE_4
  *     @arg LPTIMER_CTL_PRES_DIVIDE_8
  *     @arg LPTIMER_CTL_PRES_DIVIDE_16
  *     @arg LPTIMER_CTL_PRES_DIVIDE_32
  *     @arg LPTIMER_CTL_PRES_DIVIDE_64
  *     @arg LPTIMER_CTL_PRES_DIVIDE_128
  *
  *     @arg LPTIMER_CTL_TPOL_TRUE: capture/count at falling,initial output low
  *     @arg LPTIMER_CTL_TPOL_FALSE: capture/count at rising,initial output high
  *
  *     This is only set in PWM-Dual mode
  *     @arg LPTIMER_CTL_PWMD_NO_DELAY
  *     @arg LPTIMER_CTL_PWMD_DELAY_2_CYCLE
  *     @arg LPTIMER_CTL_PWMD_DELAY_4_CYCLE
  *     @arg LPTIMER_CTL_PWMD_DELAY_8_CYCLE
  *     @arg LPTIMER_CTL_PWMD_DELAY_16_CYCLE
  *     @arg LPTIMER_CTL_PWMD_DELAY_32_CYCLE
  *     @arg LPTIMER_CTL_PWMD_DELAY_64_CYCLE
  *     @arg LPTIMER_CTL_PWMD_DELAY_128_CYCLE
  *
  *     This is only set in Capture mode
  *     @arg LPTIMER_CTL_RSTCNT_EN: reset counter
  *     @arg LPTIMER_CTL_RSTCNT_DIS: not reset counter
  * @retval None
  */
void LPTimerConfigure(unsigned long ulBase, unsigned long ulConfig)
{
    /* Disable the timers. */
    HWREG(ulBase + LPTIMER_CTL) &= ~(LPTIMER_CTL_TEN_EN);

    HWREG(ulBase + LPTIMER_CTL) = (HWREGH(ulBase + LPTIMER_CTL) & 
	                             ~ (LPTIMER_CTL_PWMD_Msk | 
	                                LPTIMER_CTL_RSTCNT_Msk | 
	                                LPTIMER_CTL_TMODE_Msk | 
	                                LPTIMER_CTL_PRES_Msk | 
	                                LPTIMER_CTL_TPOL_Msk)) | ulConfig;
}

/**
  * @brief Sets the initial count value of specified lptimer.
  * @note This function sets the lptimer count value; if the lptimer is running   
  *   then the value will be immediately loaded into the lptimer.
  * @param ulBase: the base address of the lptimer
  * @param ulValue: the initial count value
  * @retval None
  */
void LPTimerInitCountValueSet(unsigned long ulBase, unsigned short usValue)
{
    HWREG(ulBase + LPTIMER_CNT) = usValue;
}

/**
  * @brief Sets the lptimer reload value.
  * @param ulBase: the base address of the lptimer
  * @param ulValue: the reload value
  * @retval None.
  */
void LPTimerReloadValueSet(unsigned long ulBase, unsigned short usValue)
{
    HWREG(ulBase + LPTIMER_RLD) = usValue;
}

/**
  * @brief Sets the lptimer PWM value.
  * @param ulBase: the base address of the lptimer
  * @note In PWM-SINGLE OUTPUT and PWM-DUAL OUTPUT mode when the count value
  *   of lptimer reaching the PWM value,the output signal of lptimer toggles. 
  * @param ulValue: the PWM value
  * @retval None.
  */
void LPTimerPWMValueSet(unsigned long ulBase, unsigned short usValue)
{
    HWREG(ulBase + LPTIMER_PWM) = usValue;
}

/**
  * @brief Set the initial output level.
  * @param ulBase: the base address of the lptimer
  * @param ucInvert: specifies the initial output level
  *     @arg true: high
  *     @arg false: low  
  * @retval None.
  */
void LPTimerPolaritySet(unsigned long ulBase, unsigned short ulInvert)
{
    HWREG(ulBase + LPTIMER_CTL) = ((ulInvert) ?
                                   (HWREG(ulBase + LPTIMER_CTL) | LPTIMER_CTL_TPOL_TRUE) :
                                   (HWREG(ulBase + LPTIMER_CTL) & ~(LPTIMER_CTL_TPOL_TRUE)));
}

/**
  * @brief Set the lptimer prescale value.
  * @param ulBase: the base address of the lptimer
  * @param ulValue: the prescale value.
  *   This parameter is one of the following:
  *     @arg LPTIMER_CTL_PRES_DIVIDE_1
  *     @arg LPTIMER_CTL_PRES_DIVIDE_2
  *     @arg LPTIMER_CTL_PRES_DIVIDE_4
  *     @arg LPTIMER_CTL_PRES_DIVIDE_8
  *     @arg LPTIMER_CTL_PRES_DIVIDE_16
  *     @arg LPTIMER_CTL_PRES_DIVIDE_32
  *     @arg LPTIMER_CTL_PRES_DIVIDE_64
  *     @arg LPTIMER_CTL_PRES_DIVIDE_128
  * @retval None.
  */
void LPTimerPrescaleSet(unsigned long ulBase, unsigned short ulValue)
{
    HWREG(ulBase + LPTIMER_CTL) = (HWREG(ulBase + LPTIMER_CTL) & (~LPTIMER_CTL_ALL_Msk)) | ulValue;
}

/**
  * @brief Sets the PWM delay value(PWM-Dual mode only).
  * @note This function sets the value of the PWM delay to control the number of
  *   system clock cycles delay before the PWM Output and PWM Output Complement
  *   are forced to their active state. 
  * @param ulBase: the base address of the lptimer
  * @param ulValue: the PWM delay value.
  *   This parameter is one of the following:
  *     @arg LPTIMER_CTL_PWMD_NO_DELAY
  *     @arg LPTIMER_CTL_PWMD_DELAY_2_CYCLE
  *     @arg LPTIMER_CTL_PWMD_DELAY_4_CYCLE
  *     @arg LPTIMER_CTL_PWMD_DELAY_8_CYCLE
  *     @arg LPTIMER_CTL_PWMD_DELAY_16_CYCLE
  *     @arg LPTIMER_CTL_PWMD_DELAY_32_CYCLE
  *     @arg LPTIMER_CTL_PWMD_DELAY_64_CYCLE
  *     @arg LPTIMER_CTL_PWMD_DELAY_128_CYCLE
  * @retval None.
  */
void LPTimerPWMDelaySet(unsigned long ulBase, unsigned short ulValue)
{
    HWREG(ulBase + LPTIMER_CTL) = (HWREG(ulBase + LPTIMER_CTL) & (~LPTIMER_CTL_PWMD_Msk)) | ulValue;
}

/**
  * @brief Gets the current count value of specified lptimer.
  * @note This function reads the current value of the specified lptimer.
  * @param ulBase: the base address of the lptimer
  * @retval The current count value.
  */
unsigned short LPTimerCountValueGet(unsigned long ulBase)
{
    return(HWREG(ulBase + LPTIMER_CNT));
}

/**
  * @brief Gets the timer reload value.
  * @param ulBase: the base address of the timer
  * @retval the reload value.
  */
unsigned short LPTimerReloadValueGet(unsigned long ulBase)
{
    return(HWREG(ulBase + LPTIMER_RLD));
}

/**
  * @brief Gets the lptimer capture value.
  * @param ulBase: the base address of the lptimer
  * @retval the PWM value.
  */
unsigned short LPTimerCaptureValueGet(unsigned long ulBase)
{
    return(HWREG(ulBase + LPTIMER_PWM));
}

/**
  * @brief Get the initial output level.
  * @param ulBase: the base address of the lptimer
  * @retval Returns true if the initial output level is high.
  */
unsigned char LPTimerPolarityGet(unsigned long ulBase)
{
    return ((HWREG(ulBase + LPTIMER_CTL) & LPTIMER_CTL_TPOL_Msk) ? true : false); 
}

/**
  * @brief Configure the source clock for lptimer.
  * @param ulBase: the base address of the lptimer
  * @param ucClkConfig: the types of counting clock
  *   This parameter is one of the following:
  *     @arg LPTIMER_CONFIG_CLK_MUX_EXT_ONLY: only external clock exists. 
  *     @arg LPTIMER_CONFIG_CLK_MUX_APB_ONLY: apb clock as counting clock. 
  *     @arg LPTIMER_CONFIG_CLK_MUX_APB_EXT: APB clock sampling external clock.
  *     @arg LPTIMER_CONFIG_CLK_MUX_INTER_ONLY: only internal clock exists.   
  *     @arg LPTIMER_CONFIG_CLK_MUX_INTER_EXT: internal clock sampling external clock.      
  *     @arg LPTIMER_CONFIG_CLK_MUX_APB_INTER: apb clock sampling internal clock.      
  * @retval None.
  */
void LPTimerClockSrcMux(unsigned long ulBase, unsigned char ucClkConfig)
{
     HWREG(ulBase + LPTIMER_CONFIG) = (HWREG(ulBase + LPTIMER_CONFIG) & (~LPTIMER_CONFIG_CLK_MUX_Msk)) | ucClkConfig;
}

/**
  * @brief Enable the clock switch for lptimer.
  * @param ulBase: the base address of the lptimer
  * @retval None.
  */
void LPTimerClockSwitchEnable(unsigned long ulBase)
{
    HWREG(ulBase + LPTIMER_CONFIG) |= LPTIMER_CONFIG_CLK_SWITCH_EN_Msk;
}

/**
  * @brief Disable the clock switch for lptimer.
  * @param ulBase: the base address of the lptimer
  * @retval None.
  */
void LPTimerClockSwitchDisable(unsigned long ulBase)
{
    HWREG(ulBase + LPTIMER_CONFIG) &= ~LPTIMER_CONFIG_CLK_SWITCH_EN_Msk;
}

/**
  * @brief Start the clock switch for lptimer.
  * @note This function only used for LPTIMER_CONFIG_CLK_MUX_APB_EXT and
      LPTIMER_CONFIG_CLK_MUX_EXT_ONLY switching
  * @param ulBase: the base address of the lptimer
  * @param usAction: The action of clock switching
  *   This parameter is one of the following:
  *     @arg LPTIMER_CONFIG_CLK_SWITCH_START: APB_EXT switch to EXT_ONLY 
  *     @arg LPTIMER_CONFIG_CLK_SWITCH_RECOVER: EXT_ONLY switch to APB_EXT
  * @retval None.
  */
void LPTimerClockSwitch(unsigned long ulBase, unsigned short usAction)
{
    HWREG(ulBase + LPTIMER_CONFIG) = (HWREG(ulBase + LPTIMER_CONFIG) & ~(LPTIMER_CONFIG_CLK_SWITCH_Msk)) |  usAction;
}

/**
  * @brief Sets the effective edge of the counting clock for lptimer. 
  * @param ulBase: the base address of the lptimer
  * @param usEdge: the clock counting edge
  *   This parameter is one of the following: 
  *     @arg LPTIMER_CONFIG_CLK_POLARITY_RISE
  *     @arg LPTIMER_CONFIG_CLK_POLARITY_FALL
  *     @arg LPTIMER_CONFIG_CLK_POLARITY_BOTH
  * @retval None.
  */
void LPTimerClockPolarity(unsigned long ulBase, unsigned short usEdge)
{
    HWREG(ulBase + LPTIMER_CONFIG) = (HWREG(ulBase + LPTIMER_CONFIG) & ~LPTIMER_CONFIG_CLK_POLARITY_Msk) | usEdge;

}

/**
  * @brief Configure the delay of external clock glitch filter.
  * @param ulBase: the base address of the lptimer
  * @param ulValue: the delay value.
  *   This parameter is one of the following:
  *     @arg LPTIMER_CONFIG_EXTCLK_FILTER_NO_DELAY
  *     @arg LPTIMER_CONFIG_EXTCLK_FILTER_DELAY_2_CYCLE
  *     @arg LPTIMER_CONFIG_EXTCLK_FILTER_DELAY_4_CYCLE
  *     @arg LPTIMER_CONFIG_EXTCLK_FILTER_DELAY_8_CYCLE
  * @retval None.
  */
void LPTimerEXTClockFilterDelay(unsigned long ulBase, unsigned char ucValue)
{
     HWREG(ulBase + LPTIMER_CONFIG) = (HWREG(ulBase + LPTIMER_CONFIG) & (~LPTIMER_CONFIG_EXTCLK_FILTER_Msk)) | ucValue;
}

/**
  * @brief Configure the delay of clock gating glitch filter.
  * @param ulBase: the base address of the lptimer
  * @param ulValue: the delay value. When utc timer is disabled, the filter
      cannot choose 1ms~10ms precision.
  *   This parameter is one of the following:
  *     @arg LPTIMER_CONFIG_GATE_FILTER_NO_DELAY
  *     @arg LPTIMER_CONFIG_GATE_FILTER_DELAY_2_CYCLE
  *     @arg LPTIMER_CONFIG_GATE_FILTER_DELAY_4_CYCLE
  *     @arg LPTIMER_CONFIG_GATE_FILTER_DELAY_8_CYCLE
  *   choose one of the following only if UTC CLK exists
  *     @arg LPTIMER_CONFIG_GATE_FILTER_DELAY_1_MS
  *     @arg LPTIMER_CONFIG_GATE_FILTER_DELAY_2_MS
  *     @arg LPTIMER_CONFIG_GATE_FILTER_DELAY_4_MS
  *     @arg LPTIMER_CONFIG_GATE_FILTER_DELAY_10_MS
  * @retval None.
  */
void LPTimerClockGateFilterDelay(unsigned long ulBase, unsigned char ucValue)
{

     HWREG(ulBase + LPTIMER_CONFIG) = (HWREG(ulBase + LPTIMER_CONFIG) & (~LPTIMER_CONFIG_GATE_FILTER_Msk)) | ucValue;
}

/**
  * @brief Get the state of clock for lptimer. 
  * @param ulBase: the base address of the lptimer
  * @param ulClkFlags: the types of clock. 
  *   This parameter is one of the following:
  *     @arg LPTIMER_CLK_EXT_LEVEL_Msk: external clock state flag
  *     @arg LPTIMER_CLK_SWITCH_FLAG_Msk: clock switch state flag 
  *     @arg LPTIMER_CLK_APB_FLAG_Msk: apb clock state flag
  *     @arg LPTIMER_CLK_INTER_FLAG_Msk: internal clock state flag  
  * @retval Returns true if the flag is 1.
  */
unsigned char LPTimerClockStateGet(unsigned long ulBase, unsigned char ucClkFlags)
{
     return((HWREG(ulBase + LPTIMER_EXT_CLK_FLAG) & ucClkFlags) ? true : false ); 
}

/**
  * @brief Enables the external clock phase detect of lptimer.
  * @param ulBase: the base address of the lptimer
  * @retval None.
  */
void LPTimerExtPhaseDetectEnable(unsigned long ulBase)
{
    HWREG(ulBase + LPTIMER_CONFIG) |= LPTIMER_CONFIG_EXT_PHASE_EN_Msk;
}

/**
  * @brief Disable the external clock phase detect of lptimer.
  * @param ulBase: the base address of the lptimer
  * @retval None.
  */
void LPTimerExtPhaseDetectDisable(unsigned long ulBase)
{
    HWREG(ulBase + LPTIMER_CONFIG) &= ~LPTIMER_CONFIG_EXT_PHASE_EN_Msk;
}

/**
  * @brief Configure the mode of external clock phase detect.
  * @param ulBase: the base address of the lptimer
  * @param usMode: the mode of external clock phase detect
  *     @arg LPTIMER_CONFIG_EXT_PHASE_Mode_0: 
  *     when external clock2 counting edge ahead of external clock1, timer counter +1. 
  *     when external clock2 counting edge after external clock1, timer counter -1.   
  *     @arg LPTIMER_CONFIG_EXT_PHASE_Mode_1: 
  *     when external clock1 counting edge ahead of external clock2, timer counter +1. 
  *     when external clock1 counting edge after external clock2, timer counter -1.
  * @retval None.
  */
void LPTimerExtPhaseDetectMode(unsigned long ulBase, unsigned short usMode)
{
    HWREG(ulBase + LPTIMER_CONFIG) = (HWREG(ulBase + LPTIMER_CONFIG) & ~(LPTIMER_CONFIG_EXT_PHASE_Mode_Msk)) | usMode;                                
}

/**
  * @brief Registers an interrupt handler for the lptimer interrupt.
  * @param ulIntChannel: the interrupt assignments of lptimer
  * @param g_pRAMVectors: the global interrupt vectors table
  * @param pfnHandler: a pointer to the function to be called when 
  *        the Timer interrupt occurs. 
  * @retval None
  */
void LPTimerIntRegister(unsigned long ulIntChannel, unsigned long *g_pRAMVectors, void (*pfnHandler)(void))
{
	IntRegister(ulIntChannel, g_pRAMVectors, pfnHandler);

	IntEnable(ulIntChannel);
}

/**
  * @brief Unregisters an interrupt handler for the lptimer interrupt.
  * @param ulIntChannel: the interrupt assignments of lptimer
  * @param g_pRAMVectors: the global interrupt vectors table
  * @retval None
  */
void LPTimerIntUnregister(unsigned long ulIntChannel, unsigned long *g_pRAMVectors)
{
	IntDisable(ulIntChannel);

	IntUnregister(ulIntChannel, g_pRAMVectors);
}

/**
  * @brief Enables lptimer interrupt sources.
  * @param ulBase: the base address of the lptimer
  * @param ulIntFlags: the bit mask of the interrupt sources to be enabled.
  *   This parameter is one of the following:
  *     @arg LPTIMER_CTL_TICONFIG_OUTER_EVENT, Capture/Deassertion events interrupt
  *     @arg LPTIMER_CTL_TICONFIG_INNER_EVENT, Reload/Compare events interrupt
  *     @arg LPTIMER_CTL_TICONFIG_ALL_EVENT, Reload, Compare, Capture, Deassertion events interrupt
  * @retval None.
  */
static uint32_t lptimer_int_enable_mask = 0;
void LPTimerIntEnable(unsigned long ulBase, unsigned char ucIntFlags)
{
    /* Set the specified interrupts */
    HWREG(ulBase + LPTIMER_CTL) = (HWREG(ulBase + LPTIMER_CTL) & (~LPTIMER_CTL_TICONFIG_Msk)) | ucIntFlags;
	
    /* Enable the specified interrupts */
	if(ulBase == LPTIMER1_BASE)
	{
        lptimer_int_enable_mask |= LPTIMER_INT_LPTIMER1_Msk; //lptimer中断使能寄存器不可读，寄存器表注释错误
        // HWREG(LPTIMER_INT_ENABLE) |= LPTIMER_INT_LPTIMER1_Msk ;
	}
	else if(ulBase == LPTIMER2_BASE)
	{
        lptimer_int_enable_mask |= LPTIMER_INT_LPTIMER2_Msk;
		// HWREG(LPTIMER_INT_ENABLE) |= LPTIMER_INT_LPTIMER2_Msk ;
	}
    HWREG(LPTIMER_INT_ENABLE) = lptimer_int_enable_mask;
}

/**
  * @brief Disable lptimer interrupt sources.
  * @param ulBase: the base address of the lptimer
  * @retval None.
  */
void LPTimerIntDisable(unsigned long ulBase)
{
    //没有一个lptimer的中断被使能，此时全部lptimer的中断都禁能
    if(lptimer_int_enable_mask == 0) 
    {
        HWREG(LPTIMER_INT_ENABLE) = 0UL;
    }
    //若有lptimer的中断被使能，则按需中断禁能
    else
    {
        if(ulBase == LPTIMER1_BASE)
        {
            lptimer_int_enable_mask &= ~LPTIMER_INT_LPTIMER1_Msk ;
            // HWREG(LPTIMER_INT_ENABLE) &= ~LPTIMER_INT_LPTIMER1_Msk ;
        }
        else if(ulBase == LPTIMER2_BASE)
        {
            lptimer_int_enable_mask &= ~LPTIMER_INT_LPTIMER2_Msk ;
            //HWREG(LPTIMER_INT_ENABLE) &= ~LPTIMER_INT_LPTIMER2_Msk ;
        }
        HWREG(LPTIMER_INT_ENABLE) = lptimer_int_enable_mask;
    }
}

/**
  * @brief Gets the current interrupt state of lptimers.
  * @param None
  * @retval Returns interrupt status of lptimer1 & lptimer2
  */
unsigned char LPTimerIntStatus(void)
{
	return HWREG(LPTIMER_INT_STATUS);
}	

/**
  * @brief Determines whether the current interrupt is caused by a capture event.
  * @param ulBase: the base address of the lptimer
  * @retval Returns true if the interrupt is caused by a capture event 
  *   or false if the interrupt is caused by a reload event
  */
unsigned char LPTimerIntEventGet(unsigned long ulBase)
{
    /* Return wether the most recent interrupt is caused by Capture Event */
    return((HWREG(ulBase + LPTIMER_CTL) & LPTIMER_CTL_INPCAP_Msk) ? true : false);
}

	
	



