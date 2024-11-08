#pragma once

#define XY_XTAL_CLK 26000000
#define XY_BBPLL_CLK 368640000
#define XY_UTC_CLK    (g_32k_clock)  //Get32kClockFreq()

#define UTC_CLK_SRC_Pos ((uint32_t)2)
#define UTC_CLK_SRC_Msk ((uint32_t)0x01 << UTC_CLK_SRC_Pos)
#define UTC_CLK_SRC_ResetMsk ((uint32_t)~UTC_CLK_SRC_Msk)

#define UTC_CLK_DIV_Pos ((uint32_t)28)
#define UTC_CLK_DIV_Msk ((uint32_t)0x07 << UTC_CLK_DIV_Pos)
#define UTC_CLK_DIV_ResetMsk ((uint32_t)~UTC_CLK_DIV_Msk)

#define HRC_CLK_DIV_Pos ((uint32_t)24)
#define HRC_CLK_DIV_Msk ((uint32_t)0x07 << HRC_CLK_DIV_Pos)
#define HRC_CLK_DIV_ResetMsk ((uint32_t)~HRC_CLK_DIV_Msk)

typedef uint32_t (*fptr)(void);

typedef enum
{
	RC_32K,
	XTAL_32K
} _32KCLK_SRC_TypeDef;

typedef enum
{
	I2C1_LSIO_CLK,
	PERI1_CLK
} I2C1CLK_SRC_TypeDef;

typedef enum
{
	I2C2_LSIO_CLK,
	PERI2_CLK
} I2C2CLK_SRC_TypeDef;


typedef enum
{
	P_LPUART,
	P_UTC,
	P_LPTIME1 = P_UTC,
	P_TICKCNT = P_UTC,
	P_LCDC,
	P_KEYSCAN = P_LCDC,
	P_DMAC,
	P_SHA = P_DMAC,
	P_AES = P_DMAC,
	P_MCNT,
	P_TIMER1 = P_MCNT,
	P_CRC = P_MCNT,
	P_CSP2,
	P_CSP3 = P_CSP2,
	P_ISO7816 = P_CSP2,
	P_SPI = P_CSP2,
	P_TIMER3 = P_CSP2,
	P_TIMER4 = P_CSP2,
	P_I2C1,
	P_I2C2,
	P_UART2,
	P_TRNG = P_UART2,
	P_CSP1 = P_UART2,
	P_TIMER2 = P_UART2,
	P_ADCCTRL = P_UART2
} peri_t;

#define _32K_CLK_SRC_XTAL 0x00010000
#define _32K_CLK_SRC_RC 0x00020000

#define SYS_CLK_SRC_32K 0x00000001
#define SYS_CLK_SRC_HRC 0x00000002
#define SYS_CLK_SRC_XTAL 0x00000004
#define SYS_CLK_SRC_PLL 0x00000008

#define LSIO_CLK_SRC_32K 0x00000010
#define LSIO_CLK_SRC_HRC 0x00000020
#define LSIO_CLK_SRC_XTAL 0x00000040

#define I2C1_CLK_SRC_PERI1 0x00000100
#define I2C1_CLK_SRC_LSIO 0x00000200

#define I2C2_CLK_SRC_PERI2 0x00001000
#define I2C2_CLK_SRC_LSIO 0x00002000

#define AP_HCLK_DIV_4 0x00000000
#define AP_HCLK_DIV_5 0x00000001
#define AP_HCLK_DIV_6 0x00000002
#define AP_HCLK_DIV_7 0x00000003
#define AP_HCLK_DIV_8 0x00000004
#define AP_HCLK_DIV_9 0x00000005
#define AP_HCLK_DIV_10 0x00000006
#define AP_HCLK_DIV_11 0x00000007
#define AP_HCLK_DIV_12 0x00000008
#define AP_HCLK_DIV_13 0x00000009
#define AP_HCLK_DIV_14 0x0000000A
#define AP_HCLK_DIV_15 0x0000000B
#define AP_HCLK_DIV_16 0x0000000C
#define AP_HCLK_DIV_17 0x0000000D
#define AP_HCLK_DIV_18 0x0000000E
#define AP_HCLK_DIV_19 0x0000000F

#define SYS_HCLK_DIV_4 0x00000000
#define SYS_HCLK_DIV_5 0x00000001
#define SYS_HCLK_DIV_6 0x00000002
#define SYS_HCLK_DIV_7 0x00000003
#define SYS_HCLK_DIV_8 0x00000004
#define SYS_HCLK_DIV_9 0x00000005
#define SYS_HCLK_DIV_10 0x00000006
#define SYS_HCLK_DIV_11 0x00000007
#define SYS_HCLK_DIV_12 0x00000008
#define SYS_HCLK_DIV_13 0x00000009
#define SYS_HCLK_DIV_14 0x0000000A
#define SYS_HCLK_DIV_15 0x0000000B
#define SYS_HCLK_DIV_16 0x0000000C
#define SYS_HCLK_DIV_17 0x0000000D
#define SYS_HCLK_DIV_18 0x0000000E
#define SYS_HCLK_DIV_19 0x0000000F

#define PERI1_PCLK_DIV_1 0x00000000
#define PERI1_PCLK_DIV_2 0x00000001
#define PERI1_PCLK_DIV_3 0x00000002
#define PERI1_PCLK_DIV_4 0x00000003

#define PERI2_PCLK_DIV_1 0x00000000
#define PERI2_PCLK_DIV_2 0x00000001
#define PERI2_PCLK_DIV_3 0x00000002
#define PERI2_PCLK_DIV_4 0x00000003

/*sys_hclk主频，DMA/AES/SHA等外设时钟*/
uint32_t GetSysClockFreq(void);
uint32_t GetAPClockFreq(void);
uint32_t GetPCLK1Freq(void);
uint32_t GetPCLK2Freq(void);
uint32_t _32K_ClkDivGet(void);
uint32_t RC26M_ClkDivGet(void);
uint32_t Ap_HclkDivGet(void);
uint32_t Sys_HclkDivGet(void);
uint32_t Cp_HclkDivGet(void);
void _32k_Clk_Src_Select(uint8_t ClockSource, uint32_t Clockdiv);
void HRC_Clk_Src_Select(uint32_t Clockdiv);
void Fast_SystemClockSrc_Select();
void System_Clock_CFG(void);
uint32_t GetLpuartClockFreq(void);
uint32_t GetlsioFreq(void);
uint32_t GetI2c1ClockFreq(void);
uint32_t GetI2c2ClockFreq(void);
uint32_t GetPCLK2Freq(void);
void Sys_Clk_Src_Select(uint32_t ClockSource);
void Xtal_Pll_Ctl(uint32_t SysSource, uint32_t LSioSource);
void Ap_HclkDivSet(uint32_t ClockDiv);
uint32_t PRCM_LPUARTclkDivGet(void);
extern uint32_t Get32kClockFreq(void);
extern uint32_t g_xtal_clock, g_32k_clock, g_hrc_clock, g_pll_clock;

