/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "hw_types.h"
#include "xy_memmap.h"
#include "sys_config.h"
#include "hw_prcm.h"
/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/

/**
 * @brief 从retention中获取Sys Div
 */
uint32_t Get_Sys_Div()
{
	return ((COREPRCM->SYSCLK_CTRL & SYS_HCLK_DIV_Msk) >> SYS_HCLK_DIV_Pos) + 4;
}

/**
 * @brief 从retention中获取Peri1 Div
 */
uint32_t Get_Peri1_Div()
{
	return ((COREPRCM->SYSCLK_CTRL & PERI1_PCLK_DIV_Msk) >> PERI1_PCLK_DIV_Pos) + 1;
}

/**
 * @brief 从retention中获取Peri2 Div
 */
uint32_t Get_Peri2_Div()
{
	return ((COREPRCM->SYSCLK_CTRL & PERI2_PCLK_DIV_Msk) >> PERI2_PCLK_DIV_Pos) + 1;
}

/**
 * @brief 从retention中获取Peri2 Div
 */
extern volatile unsigned int g_freq_32k;
uint32_t Get_32K_Freq()
{
	return g_freq_32k;
}

void Set_32K_Freq(unsigned int freq_32k)
{
	osCoreEnterCritical();
	g_freq_32k = freq_32k;
	osCoreExitCritical();
}

/**
 * @brief 从retention中获取Peri2 Div
 */
uint32_t Get_32K_Div()
{
	return 32;
}
