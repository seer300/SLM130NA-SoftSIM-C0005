#pragma once
#include "xy_memmap.h"

#define XY_XTAL_CLK 26000000
#define XY_BBPLL_CLK 368640000

#define UTC_CLK_SRC_Pos ((uint32_t)2)
#define UTC_CLK_SRC_Msk ((uint32_t)0x01 << UTC_CLK_SRC_Pos)
#define UTC_CLK_SRC_ResetMsk ((uint32_t)~UTC_CLK_SRC_Msk)

#define UTC_CLK_DIV_Pos ((uint32_t)28)
#define UTC_CLK_DIV_Msk ((uint32_t)0x07 << UTC_CLK_DIV_Pos)
#define UTC_CLK_DIV_ResetMsk ((uint32_t)~UTC_CLK_DIV_Msk)

#define HRC_CLK_DIV_Pos ((uint32_t)24)
#define HRC_CLK_DIV_Msk ((uint32_t)0x07 << HRC_CLK_DIV_Pos)
#define HRC_CLK_DIV_ResetMsk ((uint32_t)~HRC_CLK_DIV_Msk)

#define	FLASH_26MHRCCALI_TBL_BASE	(CALIB_FREQ_BASE + 0x800)
#define FLASH_26MHRCCALI_TBL_LEN	4

uint32_t RC26M_ClkDivGet(void);
uint32_t GetlsioFreq(void);

extern uint32_t g_xtal_clock, g_hrc_clock, g_pll_clock;
