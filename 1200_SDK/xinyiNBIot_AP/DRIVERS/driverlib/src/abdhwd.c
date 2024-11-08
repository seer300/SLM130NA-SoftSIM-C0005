#include "abdhwd.h"


/**
  * @brief Enable the auto baudrate dectect.
  * @retval None
  */
void ABD_Enable(void)
{
	ABDHWD->ABD_CTL |= ABDHWD_ABD_ENA_Msk;
}

/**
  * @brief Disable the auto baudrate dectect.
  * @retval None
  */
void ABD_Disable(void)
{
	ABDHWD->ABD_CTL &= ~ABDHWD_ABD_ENA_Msk;
}

/**
  * @brief Get the auto baudrate dectect completion status.
  * @retval The completion status.
  *     @arg 1: ABD is idle or finished, COUNT value has been updated.
  *     @arg 0: ABD is working, and do not read COUNT value while this value is 0
  */
uint8_t ABD_EndStatusGet(void)
{
	return (ABDHWD->ABD_STA & ABDHWD_ABD_END_Msk);
}

/**
  * @brief Get the count value of auto baudrate dectect.
  * @note This 
  * @retval The count value.
  */
uint32_t ABD_CountValueGet(void)
{	
	return ((uint32_t)ABDHWD->ABD_COUNTH&0xFF) << 16 | (ABDHWD->ABD_COUNTL&0xFF);
}

/**
  * @brief Configure the hardware dectect.
  * @param CSPNum: the CPS modular.
  * @param ValidBits: the valid bits of ucPatternfor sequence detection.
  *   This parameter can be one of the following values:
  *     @arg ABDHWD_CFG_Valid_Bit_5
  *     @arg ABDHWD_CFG_Valid_Bit_6
  *     @arg ABDHWD_CFG_Valid_Bit_7
  *     @arg ABDHWD_CFG_Valid_Bit_8
  * @param Pattern: a given pattern for UART sequence detection.
  * @retval None
  */
void HW_DetectConfig(uint8_t CSPNum, uint8_t ValidBits, uint8_t Pattern)
{
	ABDHWD->HWD_CFG = ((uint16_t)Pattern << 8 | ValidBits | CSPNum);
}

/**
  * @brief Enable the hardware dectect.
  * @retval None
  */
void HW_DetectEnable(void)
{
	ABDHWD->HWD_CFG |= ABDHWD_CFG_HW_En;
}

/**
  * @brief Disable the hardware dectect.
  * @retval None
  */
void HW_DetectDisable(void)
{
	ABDHWD->HWD_CFG &= ~ABDHWD_CFG_HW_En;
}


