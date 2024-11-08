#include "prcm.h"
#include "sys_clk.h"
#include "hw_prcm.h"
#include "hw_memmap.h"
#include "rf_drv.h"
#include "xy_memmap.h"
#include "xy_flash.h"

volatile unsigned int g_freq_32k = 0;
uint32_t g_hrc_clock = 25665000, g_xtal_clock = XY_XTAL_CLK, g_pll_clock = XY_BBPLL_CLK;

uint32_t PRCM_HRCclkDivGet(void)
{
	return (AONPRCM->AONCLK_CTRL & HRC_CLK_DIV_Msk) >> HRC_CLK_DIV_Pos;
}

uint32_t RC26M_ClkDivGet(void)
{
	uint32_t ClockDiv = PRCM_HRCclkDivGet();
	return 1 << ClockDiv;
}

uint32_t GetRC26MClockFreq(void)
{
	//在开机初始化时读至g_hrc_clock全局
	//xy_Flash_Read(FLASH_26MHRCCALI_TBL_BASE, &g_hrc_clock, FLASH_26MHRCCALI_TBL_LEN);
	return g_hrc_clock / RC26M_ClkDivGet();
}

uint32_t GetlsioFreq(void)
{
	uint32_t clock[] = {g_freq_32k, g_xtal_clock, GetRC26MClockFreq()};

	return clock[PRCM_LSioclkSrcGet() - 1];
}

