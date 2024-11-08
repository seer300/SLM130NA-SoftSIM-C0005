#include "tick.h"

/**
  * @brief  Enable the TickTimer.
  * @retval None
  */
void Tick_TimerEnable(void)
{
    TICK->CTRL = 0x1;
}

/**
  * @brief  Disable the TickTimer.
  * @retval None
  */
void Tick_TimerDisable(void)
{
    TICK->CTRL = 0x0;
}

/**
  * @brief  set the TickTimer prescale value.
  * @param  ucPrescale is the Utc clk prescale:
            TICK_PRESCALE_NONDIV:   no division;
            TICK_PRESCALE_DIV2:     div2;
            TICK_PRESCALE_DIV4:     div4;
            TICK_PRESCALE_DIV8:     div8;
            TICK_PRESCALE_DIV16:    div16;
            TICK_PRESCALE_DIV32:    div32.
  * @retval None
  */
void Tick_PrescaleSet(uint8_t ucPrescale)
{
    TICK->PRESCALE = ucPrescale;
}

/**
  * @brief  set the TickTimer AP reload value.
  * @param  ucReload is the AP reload value.
  * @retval None
  */
void Tick_APReloadSet(uint8_t ucReload)
{
    TICK->AP_RELOAD_SET = ucReload;
}

/**
  * @brief  set the TickTimer CP reload value.
  * @param  ucReload is the CP reload value.
  * @retval None
  */
void Tick_CPReloadSet(uint8_t ucReload)
{
    TICK->CP_RELOAD_SET = ucReload;
}

/**
  * @brief  get the TickTimer AP reload value.
  * @retval the TickTimer AP reload value
  */
uint32_t Tick_APReloadGet(void)
{
    return TICK->AP_RELOAD_READ;
}

/**
  * @brief  get the TickTimer CP reload value.
  * @retval the TickTimer CP reload value
  */
uint32_t Tick_CPReloadGet(void)
{
    return TICK->CP_RELOAD_READ;
}

/**
  * @brief  set the TickTimer counter value.
  * @param  ulCounter is the counter value.
  * @retval None
  */
void Tick_CounterSet(uint32_t ulCounter)
{
    TICK->COUNTER = ulCounter;

    while((TICK->CNTREG_WR_DONE & TICK_CNTREG_WR_DONE_Msk) != TICK_CNTREG_WR_DONE_Msk);
}

/**
  * @brief  get the TickTimer counter value.
  * @retval the counter value.
  */
uint32_t Tick_CounterGet(void)
{
    return TICK->COUNTER;
}


/**
  * @brief  set the TickTimer AP compare value.
  * @param  ulCompare is the AP compare value.
  * @retval None
  */
void Tick_APCompareSet(uint32_t ulCompare)
{
    TICK->AP_COMPARE = ulCompare;
}

/**
  * @brief  set the TickTimer CP compare value.
  * @param  ulCompare is the CP compare value.
  * @retval None
  */
void Tick_CPCompareSet(uint32_t ulCompare)
{
    TICK->CP_COMPARE = ulCompare;
}

/**
  * @brief  set the TickTimer AP Interrupt enable.
  * @param  ucConfig is the TickTimer Interrupt config.
  *   This parameter can be any combination of the following values:
  *     @arg TICK_INT_AP_PERIOD_Msk: period interrupt
  *     @arg TICK_INT_AP_COMPARE_Msk: compare interrupt
  *     @arg TICK_INT_AP_OVERFLOW_Msk: overflow interrupt
  * @retval None
  */
void Tick_APIntEnable(uint8_t ucConfig)
{
    TICK->AP_INT_EN |= ucConfig;
}

/**
  * @brief  set the TickTimer CP Interrupt enable.
  * @param  ucConfig is the TickTimer Interrupt config.
  *   This parameter can be any combination of the following values:
  *     @arg TICK_INT_CP_PERIOD_Msk: period interrupt
  *     @arg TICK_INT_CP_COMPARE_Msk: compare interrupt
  *     @arg TICK_INT_CP_OVERFLOW_Msk: overflow interrupt
  * @retval None
  */
void Tick_CPIntEnable(uint8_t ucConfig)
{
    TICK->CP_INT_EN |= ucConfig;
}


/**
  * @brief  set the TickTimer AP Interrupt disable.
  * @param  ucConfig is the TickTimer Interrupt config.
  *   This parameter can be any combination of the following values:
  *     @arg TICK_INT_AP_PERIOD_Msk: period interrupt
  *     @arg TICK_INT_AP_COMPARE_Msk: compare interrupt
  *     @arg TICK_INT_AP_OVERFLOW_Msk: overflow interrupt
  * @retval None
  */
void Tick_APIntDisable(uint8_t ucConfig)
{
    TICK->AP_INT_EN &= ~ucConfig;
}

/**
  * @brief  set the TickTimer CP Interrupt disable.
  * @param  ucConfig is the TickTimer Interrupt config.
  *   This parameter can be any combination of the following values:
  *     @arg TICK_INT_CP_PERIOD_Msk: period interrupt
  *     @arg TICK_INT_CP_COMPARE_Msk: compare interrupt
  *     @arg TICK_INT_CP_OVERFLOW_Msk: overflow interrupt
  * @retval None
  */
void Tick_CPIntDisable(uint8_t ucConfig)
{
    TICK->CP_INT_EN &= ~ucConfig;
}


/**
  * @brief  Registers an interrupt handler for TickTimer interrupt.
  * @param  g_pRAMVectors is the global interrupt vectors table.
  * @param  pfnHandler is a pointer to the function to be called when the TickTimer interrupt occurs.
  * @retval None
  */
void Tick_IntRegister(uint32_t *g_pRAMVectors, void (*pfnHandler)(void))
{
    IntRegister(INT_TICK, g_pRAMVectors, pfnHandler);

    IntEnable(INT_TICK);
}

/**
  * @brief  Unregisters an interrupt handler for the TickTimer interrupt.
  * @param  g_pRAMVectors is the global interrupt vectors table.
  * @retval None
  */
void Tick_IntUnregister(uint32_t *g_pRAMVectors)
{
    IntDisable(INT_TICK);

    IntUnregister(INT_TICK, g_pRAMVectors);
}

/**
  * @brief  Get the interrupt status and clear the TickTimer interrupt.
  * @retval the AP interrupt status
  */
uint8_t Tick_APReadAndClearInt(void)
{
    return TICK->AP_INTSTAT;
}

/**
  * @brief  Get the interrupt status and clear the TickTimer interrupt.
  * @retval the CP interrupt status
  */
uint8_t Tick_CPReadAndClearInt(void)
{
    return TICK->CP_INTSTAT;
}

