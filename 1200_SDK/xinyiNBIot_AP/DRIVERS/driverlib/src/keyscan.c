#include "keyscan.h"

/**
  * @brief Start key scanning function.
  * @retval None
  */
void KeyScanStart(void)
{
	HWREGH(KEYSCAN_CTRL) |= KEYSCAN_CTRL_START_Msk;
}

/**
  * @brief Stop key scanning function.
  * @retval None
  */
void KeyScanStop(void)
{
	HWREGH(KEYSCAN_CTRL) &= ~KEYSCAN_CTRL_START_Msk;
}

/**
  * @brief Enable pad as keyscan pin
  * @param ulPadMsk: bit mask of the pads that are set to keyscan
  * @retval None
  */
void KeyScanPadEn(unsigned long ulPadMsk)
{
	HWREG(COREPRCM_BASE + 0x5C) |= ulPadMsk;
}

/**
  * @brief Disable pad as keyscan pin
  * @param ulPadMsk: bit mask of the pads that are set to keyscan
  * @retval None
  */
void KeyScanPadDis(unsigned long ulPadMsk)
{
	HWREG(COREPRCM_BASE + 0x5C) &= ~ulPadMsk;
}

/**
  * @brief Enable the column of keyscan
  * @param ulKeyColValid: the bit mask of the valid column 
  * @retval None
  */
void KeyColValidEn(unsigned long ulKeyColValid)
{
	HWREG(KEYSCAN_COL_VALID) |= ulKeyColValid;
}

/**
  * @brief Disable the column of keyscan
  * @param ulKeyColValid: the bit mask of the valid column 
  * @retval None
  */
void KeyColValidDis(unsigned long ulKeyColValid)
{
	HWREG(KEYSCAN_COL_VALID) &= ~ulKeyColValid;
}

/**
  * @brief Set the number of rows and columns of key.
  * @param ucRowNum: The number of rows(1 ~ 8)
  * @param ucColNum: The number of columns(1 ~ 16)
  * @retval Return 1 if the setting is successful,otherwise return 0.
  */
unsigned char KeyRowAndColNumSet(unsigned char ucRowNum, unsigned char ucColNum)
{
	if(ucRowNum == 0 || ucColNum == 0 || ucRowNum > 8 || ucRowNum > 16)
	{
		return 0;
	}
	
	HWREGB(KEYSCAN_COL_NUM) =  (HWREGB(KEYSCAN_COL_NUM) & ~(KEYSCAN_COL_NUM_Msk)) | ucColNum;
	HWREGB(KEYSCAN_ROW_NUM) =  (HWREGB(KEYSCAN_ROW_NUM) & ~(KEYSCAN_ROW_NUM_Msk)) | ucRowNum;
	
	return 1;
}	

/**
  * @brief Configure key detection parameters.
  * @param ucHoldCount: the interval between two detection phases.(2n+1)*1/clk
  * @param ucScanRowCount: the number of repetitions of scanning a row in a detection phase.(n+1)*1/clk
  * @param ucScanCount: the number of times that the successive scans are equal
  * @param ucVolState: the initial level
  *     @arg KEYSCAN_VOL_OUT_LOW
  *     @arg KEYSCAN_VOL_OUT_HIGH  
  * @param ucClkConfig: the clock configuration
  *     @arg KEYSCAN_CLK_CONFIG_ALWAY: The clock is always on
  *     @arg KEYSCAN_CLK_CONFIG_LIMT: The hardware automatically turns off part of the clock 
  *          according to the working condition
  * @retval None
  */
void KeyScanConfig( unsigned char ucHoldCount, unsigned char ucScanRowCount, unsigned char ucScanCount, unsigned char ucVolState, unsigned char ucClkConfig)
{
    HWREGB(KEYSCAN_SCAN_COUNT)   = (HWREGB(KEYSCAN_SCAN_COUNT) & ~(KEYSCAN_SCAN_COUNT_Msk)) | ucScanCount;
	HWREGB(KEYSCAN_SCAN_ROW_NUM) = (HWREGB(KEYSCAN_SCAN_ROW_NUM) & ~(KEYSCAN_SCAN_ROW_NUM_Msk)) | ucScanRowCount;
	
	HWREGH(KEYSCAN_CTRL) = (HWREGH(KEYSCAN_CTRL) &~(KEYSCAN_HOLD_COUNT_Msk | KEYSCAN_VOL_INVER_Msk | KEYSCAN_CLK_CONFIG_Msk)) | 
						   ((ucHoldCount << KEYSCAN_HOLD_COUNT_Pos) | (ucVolState << KEYSCAN_VOL_INVER_Pos) | ucClkConfig);
}

/**
  * @brief Enable the long press mode
  * @param ucLongPressCount: the number of times that the successive result of detecting are equal
  * @retval None
  */
void KeyScanLongPressEnable(unsigned char ucLongPressCount)
{
    HWREGH(KEYSCAN_CTRL) |= KEYSCAN_LONG_PRESS_ENABLE_Msk;
    HWREGB(KEYSCAN_ROW_NUM) = (HWREGB(KEYSCAN_ROW_NUM) & ~(KEYSCAN_LONG_PRESS_NUM_Msk)) | (ucLongPressCount << KEYSCAN_LONG_PRESS_NUM_Pos);
}

/**
  * @brief Disable the long press mode
  * @param None
  * @retval None
  */
void KeyScanLongPressDisable(void)
{
    HWREGH(KEYSCAN_CTRL) &= ~KEYSCAN_LONG_PRESS_ENABLE_Msk;  
}	


/**
  * @brief Gets the valid flag of the current key
  * @param None
  * @retval None
  */
unsigned char KeyScanValidStatus(void)
{
    return ((HWREGB(KEYSCAN_KEY_VALID) & 0x01)? 1 : 0);
}

/**
  * @brief Get the number of current valid key results
  * @param None
  * @retval None
  */
unsigned char KeyScanValidCountGet(void)
{
    return (HWREGB(KEYSCAN_DATA3_VALUE) >> KEYSCAN_KEY_VALID_CNT_Pos);
}

/**
  * @brief Get the coordinate values of all valid keys
  * @param None
  * @retval None
  */
unsigned long KeyScanValidKeyCoordinate(void)
{   
	return HWREG(KEYSCAN_DATA0_VALUE); 
}

/**
  * @brief Convert the coordinate values
  * @param ulKeyCoordinate: the coordinate values
  * @param ucKeyValue: a pointer to the buffer that saved the value 
  * @retval None
  */
void KeyCoordinateConvert(const unsigned long ulKeyCoordinate, unsigned char* const ucKeyValue)
{
	*ucKeyValue = ulKeyCoordinate & 0x1F;  //col0
	*(ucKeyValue + 1) = (ulKeyCoordinate >> KEYSCAN_KEY0_ROW_Pos) & 0x1F;//row0
	*(ucKeyValue + 2) = (ulKeyCoordinate >> KEYSCAN_KEY1_COL_Pos) & 0x1F;//col1
	*(ucKeyValue + 3) = (ulKeyCoordinate >> KEYSCAN_KEY1_ROW_Pos) & 0x1F;//row1	
	*(ucKeyValue + 4) = (ulKeyCoordinate >> KEYSCAN_KEY2_COL_Pos) & 0x1F;//col2
	*(ucKeyValue + 5) = (ulKeyCoordinate >> KEYSCAN_KEY2_ROW_Pos) & 0x1F;//row2  
}

/**
  * @brief Registers an interrupt handler for keyscan interrupt.
  * @param g_pRAMVectors: the global interrupt vectors table
  * @param pfnHandler: a pointer to the function to be called when 
  *        the g_at_lpuart interrupt occurs. 
  * @retval None
  */
void KeyScanIntRegister(unsigned long *g_pRAMVectors, void (*pfnHandler)(void))
{
	IntRegister(INT_KEYSCAN, g_pRAMVectors, pfnHandler);

	IntEnable(INT_KEYSCAN);
}

/**
  * @brief Unregisters an interrupt handler for keyscan interrupt.
  * @param g_pRAMVectors: the global interrupt vectors table
  * @retval None
  */
void KeyScanIntUnregister(unsigned long *g_pRAMVectors)
{
    IntDisable(INT_KEYSCAN);

    IntUnregister(INT_KEYSCAN, g_pRAMVectors);
}

/**
  * @brief Enable keyscan UART interrupt sources.
  * @param None
  * @retval None
  */
void KeyScanIntEnable(void)
{
    HWREGH(KEYSCAN_CTRL) |= KEYSCAN_INI_ENABLE_Msk;
}

/**
  * @brief Disable keyscan interrupt sources.
  * @param None
  * @retval None
  */
void KeyScanIntDisable(void)
{
    HWREGH(KEYSCAN_CTRL) &= ~KEYSCAN_INI_ENABLE_Msk;
}

