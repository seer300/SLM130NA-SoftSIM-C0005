#include "watchdog.h"

/**
  * @brief  get if the watchdog timer is enabled.
  * @param  ulBase is the base address of the watchdog timer module.
  * @retval watchdog is enable or not
  */
uint8_t WDT_IsRunning(WDT_TypeDef* WDTx)
{
	return(WDTx->CON & WDT_CTL_WATCHDOG_EN);
}

/**
  * @brief  Enable the watchdog timer.
  * @param  ulBase is the base address of the watchdog timer module.
  * @retval None
  */
void WDT_Enable(WDT_TypeDef* WDTx)
{
	WDTx->CON |= WDT_CTL_WATCHDOG_EN;
}

/**
  * @brief  Disable the watchdog timer.
  * @param  ulBase is the base address of the watchdog timer module.
  * @retval None
  */
void WDT_Disable(WDT_TypeDef* WDTx)
{
	WDTx->CON &= (~WDT_CTL_WATCHDOG_EN);
}

/**
  * @brief  Enable the watchdog timer reset.
  * @param  ulBase is the base address of the watchdog timer module.
  * @retval None
  */
void WDT_ResetEnable(WDT_TypeDef* WDTx)
{
	WDTx->CON |= WDT_CTL_RST_EN;
}

/**
  * @brief  Disable the watchdog timer reset.
  * @param  ulBase is the base address of the watchdog timer module.
  * @retval None
  */
void WDT_ResetDisable(WDT_TypeDef* WDTx)
{
    WDTx->CON &= ~(WDT_CTL_RST_EN);
}

/**
  * @brief  Enable the watchdog timer repeat.
  * @param  ulBase is the base address of the watchdog timer module.
  * @retval None
  */
void WDT_TimerRepeatEnable(WDT_TypeDef* WDTx)
{
    WDTx->CON |= WDT_CTL_COUNTER_EN;
}

/**
  * @brief  Disable the watchdog timer repeat.
  * @param  ulBase is the base address of the watchdog timer module.
  * @retval None
  */
void WDT_TimerRepeatDisable(WDT_TypeDef* WDTx)
{
    WDTx->CON &= ~(WDT_CTL_COUNTER_EN);
}

/**
  * @brief  Set the watchdog timer reload value.
  * @param  ulBase is the base address of the watchdog timer module.
  * @param  ulLoadVal is the load value for the watchdog timer.
  * @note   This function sets the value to load into the watchdog timer when the count
            reaches zero for the first time; if the watchdog timer is running when this
            function is called, then the value will be immediately loaded into the
            watchdog timer counter.  If the ulLoadVal parameter is 0, then an
            interrupt is immediately generated.
            
            This function will have no effect if the watchdog timer has been locked.
  * @retval None
  */
void WDT_ReloadSet(WDT_TypeDef* WDTx, uint32_t LoadVal)
{
    WDTx->DAT = LoadVal;
}

/**
  * @brief  Gets the current watchdog timer value.
  * @param  ulBase is the base address of the watchdog timer module.
  * @retval the current value of the watchdog timer.
  */
uint32_t WDT_ValueGet(WDT_TypeDef* WDTx)
{
    return(WDTx->DAT);
}

/**
  * @brief  Registers an interrupt handler for watchdog timer interrupt.
  * @param  g_pRAMVectors is the global interrupt vectors table.
  * @param  pfnHandler is a pointer to the function to be called when the watchdog timer interrupt occurs.
  * @retval None
  */
void WDT_IntRegister(uint32_t *g_pRAMVectors, void (*pfnHandler)(void))
{
    IntRegister(INT_WDT, g_pRAMVectors, pfnHandler);

    IntEnable(INT_WDT);
}

/**
  * @brief  Unregisters an interrupt handler for the watchdog timer interrupt.
  * @param  g_pRAMVectors is the global interrupt vectors table.
  * @retval None
  */
void WDT_IntUnregister(uint32_t *g_pRAMVectors)
{
    IntDisable(INT_WDT);

    IntUnregister(INT_WDT, g_pRAMVectors);
}

/**
  * @brief  Enables the watchdog timer interrupt.
  * @param  ulBase is the base address of the watchdog timer module.
  * @retval None
  */
void WDT_IntEnable(WDT_TypeDef* WDTx)
{
    WDTx->CON |= WDT_CTL_INT_EN;
}

/**
  * @brief  Disable the watchdog timer interrupt.
  * @param  ulBase is the base address of the watchdog timer module.
  * @retval None
  */
void WDT_IntDisable(WDT_TypeDef* WDTx)
{
    WDTx->CON &= ~(WDT_CTL_INT_EN);
}

/**
  * @brief  Get the interrupt status and clear the watchdog timer interrupt.
  * @param  ulBase is the base address of the watchdog timer module.
  * @retval the interrupt status
  */
uint32_t WDT_ReadClearInt(WDT_TypeDef* WDTx)
{
    return WDTx->INT_STATUS;
}

