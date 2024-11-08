#include "prcm.h"
#include "sys_clk.h"
#include "system.h"
#include "hw_prcm.h"
#include "xinyi2100.h"
#include "rf_drv.h"
#include "xy_memmap.h"
#include "common.h"
#include "qspi_flash.h"
#include "sys_mem.h"

uint32_t g_32k_clock = 32000, g_hrc_clock = 25665000, g_xtal_clock = XY_XTAL_CLK, g_pll_clock = XY_BBPLL_CLK;


uint32_t If_PRCM_SlowfreqDivEnable(void)
{
	return AONPRCM->SYSCLK_CTRL & SLOWFREQ_DIV_ENA_Msk;
}

void PRCM_Slectxtal32k(void)
{
	if (!(AONPRCM->AONCLK_FLAG & 0x02))
	{
		AONPRCM->AONCLK_CTRL |= 0x400;
		while (!(AONPRCM->AONCLK_FLAG & 0x100))
			;
		AONPRCM->AONCLK_CTRL |= 0x04;
		while (!(AONPRCM->AONCLK_FLAG & 0x02))
			;
	}
}

uint32_t PRCM_32KClkSrcGet(void)
{
	uint8_t clksrc;
	clksrc = (AONPRCM->AONCLK_CTRL & UTC_CLK_SRC_Msk) >> UTC_CLK_SRC_Pos;

	if (clksrc == 0x0)
		return RC_32K;
	else
		return XTAL_32K;
}

uint32_t PRCM_UTCclkDivGet(void)
{
	return (AONPRCM->AONCLK_CTRL & UTC_CLK_DIV_Msk) >> UTC_CLK_DIV_Pos;
}

void PRCM_UTCclkDivSet(uint32_t UTCclkDiv)
{
	AONPRCM->AONCLK_CTRL = (AONPRCM->AONCLK_CTRL & UTC_CLK_DIV_ResetMsk) | UTCclkDiv;
}

uint32_t PRCM_HRCclkDivGet(void)
{
	return (AONPRCM->AONCLK_CTRL & HRC_CLK_DIV_Msk) >> HRC_CLK_DIV_Pos;
}

void PRCM_HRCclkDivSet(uint32_t HRCclkDiv)
{
	AONPRCM->AONCLK_CTRL = (AONPRCM->AONCLK_CTRL & HRC_CLK_DIV_ResetMsk) | HRCclkDiv;
}

uint32_t PRCM_CPclkDivGet(void)
{
	return (COREPRCM->SYSCLK_FLAG & CP_HCLK_DIV_FLAG_Msk) >> CP_HCLK_DIV_FLAG_Pos;
}

uint32_t PRCM_APclkDivGet(void)
{
	return (COREPRCM->SYSCLK_FLAG & AP_HCLK_DIV_FLAG_Msk) >> AP_HCLK_DIV_FLAG_Pos;
}

/*DMA/AES/SHA等外设时钟*/
uint32_t PRCM_SYSclkDivGet(void)
{
	return ((COREPRCM->SYSCLK_CTRL & SYS_HCLK_DIV_Msk) >> SYS_HCLK_DIV_Pos) + 4;
}

uint32_t PRCM_LPUARTclkDivGet(void)
{
	return (AONPRCM->LPUA1_CTRL & 0X70) >> 4;
}

uint32_t PRCM_I2C1refclkGet(void)
{
	return (COREPRCM->SYSCLK_CTRL & I2C1_REFCLK_SEL_Msk) >> I2C1_REFCLK_SEL_Pos;
}

uint32_t PRCM_I2C2refclkGet(void)
{
	return (COREPRCM->SYSCLK_CTRL & I2C2_REFCLK_SEL_Msk) >> I2C2_REFCLK_SEL_Pos;
}

__FLASH_FUNC void Cp_HclkDivSet(uint32_t ClockDiv)
{
	if (5 == ClockDiv)
		ClockDiv = 1;
	else if (6 == ClockDiv)
		ClockDiv = 2;
	else if (7 == ClockDiv)
		ClockDiv = 3;
	else if (8 == ClockDiv)
		ClockDiv = 0;
	else if (9 == ClockDiv)
		ClockDiv = 4;
	else if (10 == ClockDiv)
		ClockDiv = 5;
	else if (12 == ClockDiv)
		ClockDiv = 6;
	else if (14 == ClockDiv)
		ClockDiv = 7;
	else if (16 == ClockDiv)
		ClockDiv = 8;
	else if (20 == ClockDiv)
		ClockDiv = 9;
	else if (24 == ClockDiv)
		ClockDiv = 10;
	else if (32 == ClockDiv)
		ClockDiv = 11;

	PRCM_CphclkDivSet(ClockDiv << CP_HCLK_DIV_Pos);
}

uint32_t Cp_HclkDivGet(void)
{
	uint32_t ClockDiv;
	if (SYSCLK_SRC_PLL == PRCM_SysclkSrcGet() || If_PRCM_SlowfreqDivEnable())
	{
		ClockDiv = PRCM_CPclkDivGet();
		if (1 == ClockDiv)
			ClockDiv = 6;
		else if (2 == ClockDiv)
			ClockDiv = 7;
		else if (3 == ClockDiv)
			ClockDiv = 8;
		else if (4 == ClockDiv)
			ClockDiv = 9;
		else if (5 == ClockDiv)
			ClockDiv = 10;
		else if (6 == ClockDiv)
			ClockDiv = 12;
		else if (7 == ClockDiv)
			ClockDiv = 14;
		else if (8 == ClockDiv)
			ClockDiv = 16;
		else if (9 == ClockDiv)
			ClockDiv = 20;
		else if (10 == ClockDiv)
			ClockDiv = 24;
		else if (11 == ClockDiv)
			ClockDiv = 32;

		return ClockDiv;
	}
	else
		return 1;
}

void Ap_HclkDivSet(uint32_t ClockDiv)
{
	if (1 != ClockDiv)
	{
		ClockDiv -= 4;
	}
	PRCM_AphclkDivSet(ClockDiv << AP_HCLK_DIV_Pos);
}

uint32_t Ap_HclkDivGet(void)
{
	if (SYSCLK_SRC_PLL == PRCM_SysclkSrcGet() || If_PRCM_SlowfreqDivEnable())
		return PRCM_APclkDivGet() + 4;
	else
		return 1;
}

/*DMA/AES/SHA等外设时钟*/
uint32_t Sys_HclkDivGet(void)
{
	if (SYSCLK_SRC_PLL == PRCM_SysclkSrcGet() || If_PRCM_SlowfreqDivEnable())
		return PRCM_SYSclkDivGet();
	else
		return 1;
}

/*上电、启CP、深睡流程、RC32校准等会发生时钟切换*/
void Sys_Clk_Src_Select(uint32_t ClockSource)
{
    qspi_wait_idle();
    #if (LLPM_VER == 1)
    if(SYSCLK_SRC_HRC != ClockSource)
    {
        AONPRCM->SYSCLK_CTRL &= ~0x1100;//表计模式force stop 32k as sysclk,加快唤醒，切换时钟源时需清除配置，否则切换时钟有问题   
    }
    #endif 
	// sysclk is not 32K
	PRCM_SysclkSrcSelect(ClockSource);

	while (ClockSource != PRCM_SysclkSrcGet())
		; // wait 32k

    #if (LLPM_VER == 1)
    if(SYSCLK_SRC_HRC == ClockSource)
    {
        AONPRCM->SYSCLK_CTRL |= 0x1100;//表计模式force stop 32k as sysclk,加快唤醒，切换时钟源时需清除配置，否则切换时钟有问题   
    }
    #endif 
	/*系统时钟主频发生切换，需要重新初始化主频全局*/
	delay_func_init();	
}

void Peri1_PclkDivSet(uint32_t ClockDiv)
{
	PRCM_Peri1pclkDivSet((ClockDiv - 1) << PERI1_PCLK_DIV_Pos);
}
void Peri2_PclkDivSet(uint32_t ClockDiv)
{
	PRCM_Peri2pclkDivSet((ClockDiv - 1) << PERI2_PCLK_DIV_Pos);
}
void _32K_ClkDivSet(uint32_t ClockDiv)
{
	if (1 == ClockDiv)
		ClockDiv = 0;
	else if (2 == ClockDiv)
		ClockDiv = 1;
	else if (4 == ClockDiv)
		ClockDiv = 2;
	else if (8 == ClockDiv)
		ClockDiv = 3;
	else if (16 == ClockDiv)
		ClockDiv = 4;
	else if (32 == ClockDiv)
		ClockDiv = 5;
	else if (64 == ClockDiv)
		ClockDiv = 6;
	else if (128 == ClockDiv)
		ClockDiv = 7;

	PRCM_UTCclkDivSet(ClockDiv << UTC_CLK_DIV_Pos);
}

uint32_t _32K_ClkDivGet(void)
{
	uint32_t ClockDiv = PRCM_UTCclkDivGet();

	return 1 << ClockDiv;
}

void _32k_Clk_Src_Select(uint8_t ClockSource, uint32_t Clockdiv)
{
	if (XTAL_32K == ClockSource)
	{
		PRCM_Slectxtal32k();
	}

	_32K_ClkDivSet(Clockdiv);
}

void RC26M_ClkDivSet(uint32_t ClockDiv)
{
	if (1 == ClockDiv)
		ClockDiv = 0;
	else if (2 == ClockDiv)
		ClockDiv = 1;
	else if (4 == ClockDiv)
		ClockDiv = 2;
	else if (8 == ClockDiv)
		ClockDiv = 3;
	PRCM_HRCclkDivSet(ClockDiv << HRC_CLK_DIV_Pos);
}

uint32_t RC26M_ClkDivGet(void)
{
	uint32_t ClockDiv = PRCM_HRCclkDivGet();
	return 1 << ClockDiv;
}

void LSio_Clk_Src_Select(uint32_t ClockSource)
{
    if (LS_IOCLK_SRC_XTAL == ClockSource)
	{
		// xtal on
        if (HWREGB(AON_SYSCLK_CTRL2) & 0x1)
        {
            HWREGB(0x40000059) = 0x7;   // itune = 7
            HWREGB(COREPRCM_ADIF_BASE + 0x5A) |= 0x10;  //xtalm power select ldo2
        }
		HWREGB(AON_SYSCLK_CTRL2) &= 0xFE;
		while (!(COREPRCM->ANAXTALRDY & 0x01));
	}

	PRCM_LSioclkSrcSelect(ClockSource);

	while (ClockSource != PRCM_LSioclkSrcGet())
		; // wait 32k
}

void I2c1_Clk_Src_Select(uint32_t ClockSource)
{
	PRCM_I2C1refclkSet(ClockSource << I2C1_REFCLK_SEL_Pos);
}

void I2c2_Clk_Src_Select(uint32_t ClockSource)
{
	PRCM_I2C2refclkSet(ClockSource << I2C2_REFCLK_SEL_Pos);
}

uint32_t I2c1_Clk_Src_Get(void)
{
	return PRCM_I2C1refclkGet();
}

uint32_t I2c2_Clk_Src_Get(void)
{
	return PRCM_I2C2refclkGet();
}

void Xtal_Pll_Ctl(uint32_t SysSource, uint32_t LSioSource)
{
	if (SYSCLK_SRC_PLL == SysSource)
	{
		SIDO_NORMAL_INIT();
		START_PLL();
		
		return;
	}
	else
	{
		HWREGB(AON_SYSCLK_CTRL2) |= 0x02; // pll force off
        HWREGB(COREPRCM_ADIF_BASE + 0x58) &= 0xE3; //disable clk to bbpll,rfpll,enable clk to rfpll and bbpll buf
	}

	if (SYSCLK_SRC_XTAL == SysSource || LS_IOCLK_SRC_XTAL == LSioSource)
	{
        if (HWREGB(AON_SYSCLK_CTRL2) & 0x1)
        {
            HWREGB(0x40000059) = 0x7;   // itune = 7
            HWREGB(COREPRCM_ADIF_BASE + 0x5A) |= 0x10;  //xtalm power select ldo2
        }         
		// xtal on
		HWREGB(AON_SYSCLK_CTRL2) &= 0xFE;
		while (!(COREPRCM->ANAXTALRDY & 0x01));
	}
	else
	{
		HWREGB(AON_SYSCLK_CTRL2) |= 0x01; // xtal force off
	}

}

uint32_t GetRC26MClockFreq(void)
{
	return g_hrc_clock / RC26M_ClkDivGet();
}

uint32_t GetLpuartClockFreq(void)
{
	uint32_t clock[] = {g_32k_clock, GetRC26MClockFreq()};

	uint32_t div = PRCM_LPUARTclkDivGet();

	if (div <= 4)
	{
        return clock[1] >> div;
	}
	else if (div == 5)
	{
		return clock[0];
	}

	return 0;
}


uint32_t GetCoresysClockFreq(void)
{
	uint32_t clock[] = {g_32k_clock, GetRC26MClockFreq(), g_xtal_clock, g_pll_clock};

	return clock[PRCM_SysclkSrcGet()];
}

/*DMA/AES/SHA等外设时钟*/
uint32_t GetSysClockFreq(void)
{
	return GetCoresysClockFreq() / Sys_HclkDivGet();
}

uint32_t GetAPClockFreq(void)
{
	return GetCoresysClockFreq() / Ap_HclkDivGet();
}

uint32_t GetPCLK1Freq(void)
{
	return GetSysClockFreq() / Peri1_PclkDivGet();
}

uint32_t GetPCLK2Freq(void)
{
	return GetSysClockFreq() / Peri2_PclkDivGet();
}

uint32_t GetlsioFreq(void)
{
	uint32_t lsio_clock = 0;

	uint32_t clock[] = {g_32k_clock, g_xtal_clock, GetRC26MClockFreq()};
	lsio_clock = clock[PRCM_LSioclkSrcGet() - 1];
	
	return lsio_clock;
}

uint32_t GetI2c1ClockFreq(void)
{
	if (I2c1_Clk_Src_Get())
		return GetPCLK1Freq();
	else
		return GetlsioFreq();
}

uint32_t GetI2c2ClockFreq(void)
{
	if (I2c2_Clk_Src_Get())
		return GetPCLK2Freq();
	else
		return GetlsioFreq();
}

uint32_t Get32kClockFreq(void)
{
	return g_32k_clock;
}

uint32_t GetlcdcClockFreq(void)
{
	return g_32k_clock / _32K_ClkDivGet();
}

uint32_t GetClockFreqByName(peri_t peri_name)
{
	fptr freq[] = {GetLpuartClockFreq, Get32kClockFreq, GetlcdcClockFreq, GetSysClockFreq, GetPCLK1Freq,
				   GetPCLK2Freq, GetI2c1ClockFreq, GetI2c2ClockFreq, GetlsioFreq};

	return freq[peri_name]();
}

void HRC_Clk_Src_Select(uint32_t Clockdiv)
{
	RC26M_ClkDivSet(Clockdiv);
}

__FLASH_FUNC void SystemClockDiv_Config(uint32_t ap_div, uint32_t cp_div, uint32_t per1_div, uint32_t per2_div)
{
	PRCM_SlowfreqDivEnable();
	
	Ap_HclkDivSet(ap_div);
	Cp_HclkDivSet(cp_div);

	if(ap_div == 1 || cp_div == 1)
	{
		PRCM_SlowfreqDivDisable();
	}
	
	Peri1_PclkDivSet(per1_div);
	Peri2_PclkDivSet(per2_div);
}

extern uint32_t sysclk_ctrl;
void Fast_SystemClockSrc_Select()
{
    //COREPRCM->SYSCLK_CTRL的恢复放在sido初始化前，防止sido的分频被修改
	COREPRCM->SYSCLK_CTRL = sysclk_ctrl;

	if (SYSCLK_SRC_PLL == SYS_CLK_SRC)
	{
		SIDO_NORMAL_INIT();
		START_PLL();
		Sys_Clk_Src_Select(SYS_CLK_SRC);
	}

}
__FLASH_FUNC void System_Clock_CFG(void)
{	
#if MODULE_VER
    // 新otp
    if(AONPRCM->AONGPREG2 & 0x10)
    {
        HRC_Clk_Src_Select(RC26MCLKDIV);
    }
    // 老otp唤醒系统时钟为hrc，防止lpuart溢出，hrc不分频
    else
    {
        HRC_Clk_Src_Select(1);
    }
#else
    HRC_Clk_Src_Select(RC26MCLKDIV);
#endif

    LSio_Clk_Src_Select(LSIO_CLK_SRC);
    //将HRC分频放在pll的sido初始化前，确保sido的初始化可以获取到这一点。LSIOCLK时钟始终应为HRC
	Xtal_Pll_Ctl(SYS_CLK_SRC, LSIO_CLK_SRC);
	if(READ_FAC_NV(uint8_t,test))  //get nv->cldo_set_vol)
	{
		SystemClockDiv_Config(AP_HCLK_DIV, 5, PERI1_PCLK_DIV, PERI2_PCLK_DIV); //cp最高频率为pll/2.5
	}
	else
	{
		SystemClockDiv_Config(AP_HCLK_DIV, CP_HCLK_DIV, PERI1_PCLK_DIV, PERI2_PCLK_DIV);
	}
	Sys_Clk_Src_Select(SYS_CLK_SRC);
}
