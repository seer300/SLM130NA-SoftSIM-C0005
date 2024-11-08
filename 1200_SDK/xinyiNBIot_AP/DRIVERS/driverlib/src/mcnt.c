#include "mcnt.h"

/**
  * @brief  start the mcnt gauging process.
  * @retval None
  */
void MCNT_Start(void)
{
    MCNT->MEASURECTRL = 0x1;
}

/**
  * @brief  stop the mcnt gauging process.
  * @retval None
  */
void MCNT_Stop(void)
{
    MCNT->MEASURECTRL = 0x0;
}

/**
  * @brief  set the mcnt clock counter number.
  * @param  ulCounter is the clock counter number.
  * @retval None
  */
void MCNT_SetCNT32k(uint32_t ulCounter)
{
    MCNT->CNT32K = ulCounter;
}

/**
  * @brief  get the mcnt clock counter number.
  * @retval the mcnt clock counter number
  */
uint32_t MCNT_GetCNT32k(void)
{
    return MCNT->CNT32K;
}

/**
  * @brief  get the mcnt measured count.
  * @retval the mcnt measured count
  */
uint32_t MCNT_GetMCNT(void)
{
    return (MCNT->CNTMEASURE);
}

/**
  * @brief  get the mcnt interrupt status.
  * @retval the mcnt interrupt status
  */
uint8_t MCNT_GetIntStatus(void)
{
    return (MCNT->MCNTINT & 0x01);
}

/**
  * @brief  set the mcnt clock source.
  * @param  ulClkSrc is the clock source.
            0:32k source
            1:misc_clk source
  * @retval None
  */
void MCNT_SetClkSrc(uint32_t ulClkSrc)
{
    MCNT->MCLKSRC = ulClkSrc;
}

/**
  * @brief  Registers an interrupt handler for mcnt interrupt.
  * @param  g_pRAMVectors is the global interrupt vectors table.
  * @param  pfnHandler is a pointer to the function to be called when the mcnt interrupt occurs.
  * @retval None
  */
void MCNT_IntRegister(uint32_t *g_pRAMVectors, void (*pfnHandler)(void))
{
    IntRegister(INT_MEASURECNT, g_pRAMVectors, pfnHandler);

    IntEnable(INT_MEASURECNT);
}

/**
  * @brief  Unregisters an interrupt handler for the mcnt interrupt.
  * @param  g_pRAMVectors is the global interrupt vectors table.
  * @retval None
  */
void MCNT_IntUnregister(uint32_t *g_pRAMVectors)
{
    IntDisable(INT_MEASURECNT);

    IntUnregister(INT_MEASURECNT, g_pRAMVectors);
}

/**
  * @brief  MCNT SelectMeasureClk.
  * @param  uint8_t value.
  *          0:  Select msysclk
  *          1:  Select 26M XTAL
  * @retval None
  */
void MCNT_SelectMeasureClk(uint8_t value)
{
	uint32_t sysCtrl;
    sysCtrl = COREPRCM->SYSCLK_CTRL;
	if(value)
	{   sysCtrl |= 0x40<<16;  }
	else
	{   sysCtrl &= ~(0x40<<16);}
	COREPRCM->SYSCLK_CTRL = sysCtrl;
}

/**
  * @brief  MCNT GetMeasureClk.
  * @param  None
  * @retval uint8_t value.
  *          0:  Select msysclk
  *          1:  Select 26M XTAL
  */
uint8_t MCNT_GetMeasureClk(void)
{
	uint32_t sysCtrl;
    sysCtrl = COREPRCM->SYSCLK_CTRL;
	return((sysCtrl&(0x40<<16))>>22);
}
/**
  * @brief  MCNT GetMeastureProcessStatus.
  * @param  None
  * @retval the mcnt MeastureProcess status
  */
uint8_t MCNT_GetMeasureProcessStatus(void)
{
	return (MCNT->MCNTINT & 0x02);
}
	

