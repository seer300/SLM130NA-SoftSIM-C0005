#ifndef __HW_PRCM_H__
#define __HW_PRCM_H__
#include "xinyi2100.h"

//*****************************************************************************
//
// The following are defines for the AON PRCM register offsets.
//
//*****************************************************************************
#define AON_SLPMODE_CTRL				(AONPRCM_BASE + 0x00)
#define AON_CP_REMAP					(AONPRCM_BASE + 0x01)
#define AON_RST_CTRL					(AONPRCM_BASE + 0x02)
#define AON_UP_STAT						(AONPRCM_BASE + 0x03)
#define AON_WKUP_STAT0					(AONPRCM_BASE + 0x08)
#define AON_WKUP_STAT1					(AONPRCM_BASE + 0x09)
#define AON_WKUP_STAT2					(AONPRCM_BASE + 0x0A)
#define AON_WKUP_STAT3					(AONPRCM_BASE + 0x0B)

#define	AON_SYSCLK_CTRL0				(AONPRCM_BASE + 0x2C)
#define	AON_SYSCLK_CTRL1				(AONPRCM_BASE + 0x2D)
#define	AON_SYSCLK_CTRL2				(AONPRCM_BASE + 0x2E)
#define	AON_SYSCLK_CTRL3				(AONPRCM_BASE + 0x2F)


#define AON_SMEM_SLPCTRL0				(AONPRCM_BASE + 0x34)

#define AON_BOOTMODE					(AONPRCM_BASE + 0x3C)

#define AON_DEBUG_CTL					(AONPRCM_BASE + 0x3E)

#define AON_AONGPREG0					(AONPRCM_BASE + 0x40)
#define AON_AONGPREG1					(AONPRCM_BASE + 0x41)
#define AON_AONGPREG2					(AONPRCM_BASE + 0x42)
#define AON_AONGPREG3					(AONPRCM_BASE + 0x43)

#define AON_FLASH_VCC_IO_CTRL           (AONPRCM_BASE + 0xA4)
#define AON_USER_VCC_IO_CTRL            (AONPRCM_BASE + 0xA5)



//*****************************************************************************
//
// The following are defines for the CORE PRCM register offsets.
//
//*****************************************************************************
#define CORE_CKG_CTRL0					(COREPRCM_BASE + 0x08)
#define CORE_CKG_CTRL1					(COREPRCM_BASE + 0x0C)
#define CORE_SYSCLK_FLAG0			    (COREPRCM_BASE + 0x10)
#define CORE_CHIP_VER					(COREPRCM_BASE + 0x18)
#define CORE_WAKUP_STAT1				(COREPRCM_BASE + 0x1D)
#define CORE_WAKEUP                     (COREPRCM_BASE + 0x20)

//*****************************************************************************
//
// The following are defines for the bit fields in the CORE_WAKEUP register.
//
//*****************************************************************************
#define PRCM_WAKEUP_CP_INT_EN_Pos       0
#define PRCM_WAKEUP_CP_INT_EN_Msk       (1UL << PRCM_WAKEUP_CP_INT_EN_Pos)
#define PRCM_WAKEUP_AP_INT_EN_Pos       1
#define PRCM_WAKEUP_AP_INT_EN_Msk       (1UL << PRCM_WAKEUP_AP_INT_EN_Pos)
#define PRCM_CP_TRIGGER_AP_INT_Pos      4
#define PRCM_CP_TRIGGER_AP_INT_Msk      (1UL << PRCM_CP_TRIGGER_AP_INT_Pos)
#define PRCM_AP_TRIGGER_CP_INT_Pos      5
#define PRCM_AP_TRIGGER_CP_INT_Msk      (1UL << PRCM_AP_TRIGGER_CP_INT_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the AON_UP_STAT register.
//
//*****************************************************************************
#define AON_UP_STAT_SVDRST_Msk				0x40
#define AON_UP_STAT_DFTGLB_Msk				0x20
#define AON_UP_STAT_POR_Msk					0x10
#define AON_UP_STAT_SOFTRST_Msk				0x08
#define AON_UP_STAT_AONWDT_Msk				0x04
#define AON_UP_STAT_LVDRST_Msk				0x02
#define AON_UP_STAT_EXTPIN_Msk				0x01
#define	AON_UP_STAT_GLOBALRST_Msk			0x7F
#define	AON_UP_STAT_GLOBALRST_NO_POR_Msk	0x6F

//*****************************************************************************
//
// The following are defines for the bit fields in the WAKEUP_STATUS register.
//
//*****************************************************************************.
#define WAKEUP_STATUS_STANDBY_WKUP_Msk				    ((uint32_t)0x00000020)
#define WAKEUP_STATUS_DSLEEP_WKUP_Msk				((uint32_t)0x00000010)
#define WAKEUP_STATUS_LPMTMR_WKUP_Msk				    ((uint32_t)0x00000008)
#define WAKEUP_STATUS_LPUART_WKUP_Msk				    ((uint32_t)0x00000004)
#define WAKEUP_STATUS_EXTPIN_WKUP_Msk				    ((uint32_t)0x00000002)
#define WAKEUP_STATUS_UTC_WKUP_Msk				        ((uint32_t)0x00000001)
#define WAKEUP_STATUS_TMR2_WKUP_Msk				        ((uint32_t)0x00008000)
#define WAKEUP_STATUS_RC32KCALI_WKUP_Msk				((uint32_t)0x00004000)
#define WAKEUP_STATUS_CPTICK_WKUP_Msk				    ((uint32_t)0x00002000)
#define WAKEUP_STATUS_APTICK_WKUP_Msk				    ((uint32_t)0x00001000)
#define WAKEUP_STATUS_AGPI2_WKUP_Msk				    ((uint32_t)0x00000400)
#define WAKEUP_STATUS_AGPI1_WKUP_Msk				    ((uint32_t)0x00000200)
#define WAKEUP_STATUS_AGPI0_WKUP_Msk				    ((uint32_t)0x00000100)
#define WAKEUP_STATUS_AGPI_WKUP_FULL_Msk                ((uint32_t)0x00000700)
#define WAKEUP_STATUS_GPI7_WKUP_Msk				        ((uint32_t)0x00800000)
#define WAKEUP_STATUS_GPI6_WKUP_Msk				        ((uint32_t)0x00400000)
#define WAKEUP_STATUS_GPI5_WKUP_Msk				        ((uint32_t)0x00200000)
#define WAKEUP_STATUS_GPI4_WKUP_Msk				        ((uint32_t)0x00100000)
#define WAKEUP_STATUS_GPI3_WKUP_Msk				        ((uint32_t)0x00080000)
#define WAKEUP_STATUS_GPI2_WKUP_Msk				        ((uint32_t)0x00040000)
#define WAKEUP_STATUS_GPI1_WKUP_Msk				        ((uint32_t)0x00020000)
#define WAKEUP_STATUS_GPI0_WKUP_Msk				        ((uint32_t)0x00010000)
#define WAKEUP_STATUS_GPI_WKUP_FULL_Msk                 ((uint32_t)0x00ff0000)
#define WAKEUP_STATUS_KEYSCAN_WKUP_Msk				    ((uint32_t)0x02000000)
#define WAKEUP_STATUS_SVD_WKUP_Msk				        ((uint32_t)0x01000000)


//*****************************************************************************
//
// The following are defines for the bit fields in the AGPIWKUP_CTRL register.
//
//*****************************************************************************
#define AGPI0_WKUP_ENA_Msk				    ((uint32_t)0x00000001)
#define AGPI0_WKUP_ENA_ResetMsk				    (~AGPI0_WKUP_ENA_Msk)
#define AGPI0_POL_CFG_ENA_Msk				((uint32_t)0x00000002)
#define AGPI0_POL_CFG_ENA_ResetMsk				(~AGPI0_POL_CFG_ENA_Msk)
#define AGPI0_WKUP_CFG_ENA_Msk				((uint32_t)0x00000030)
#define AGPI0_WKUP_CFG_ENA_ResetMsk				(~AGPI0_WKUP_CFG_ENA_Msk)
#define AGPI0_PLS_CFG_ENA_Msk				((uint32_t)0x000000C0)
#define AGPI0_PLS_CFG_ENA_ResetMsk				(~AGPI0_PLS_CFG_ENA_Msk)

#define AGPI1_WKUP_ENA_Msk				    ((uint32_t)0x00000100)
#define AGPI1_WKUP_ENA_ResetMsk				    (~AGPI1_WKUP_ENA_Msk)
#define AGPI1_POL_CFG_ENA_Msk				((uint32_t)0x00000200)
#define AGPI1_POL_CFG_ENA_ResetMsk				(~AGPI1_POL_CFG_ENA_Msk)
#define AGPI1_WKUP_CFG_ENA_Msk				((uint32_t)0x00003000)
#define AGPI1_WKUP_CFG_ENA_ResetMsk				(~AGPI1_WKUP_CFG_ENA_Msk)
#define AGPI1_PLS_CFG_ENA_Msk				((uint32_t)0x0000C000)
#define AGPI1_PLS_CFG_ENA_ResetMsk				(~AGPI1_PLS_CFG_ENA_Msk)

#define AGPI2_WKUP_ENA_Msk				    ((uint32_t)0x00010000)
#define AGPI2_WKUP_ENA_ResetMsk				    (~AGPI2_WKUP_ENA_Msk)
#define AGPI2_POL_CFG_ENA_Msk				((uint32_t)0x00020000)
#define AGPI2_POL_CFG_ENA_ResetMsk				(~AGPI2_POL_CFG_ENA_Msk)
#define AGPI2_WKUP_CFG_ENA_Msk				((uint32_t)0x00300000)
#define AGPI2_WKUP_CFG_ENA_ResetMsk				(~AGPI2_WKUP_CFG_ENA_Msk)
#define AGPI2_PLS_CFG_ENA_Msk				((uint32_t)0x00C00000)
#define AGPI2_PLS_CFG_ENA_ResetMsk				(~AGPI2_PLS_CFG_ENA_Msk)

//*****************************************************************************
//
// The following are defines for the bit fields in the GPIWKUP_CTRL register.
//
//*****************************************************************************
#define GPI_WKUP_ENA_Pos				    ((uint32_t)0x00000000)
#define GPI_WKUP_ENA_Msk				    ((uint32_t)0x00000001)

#define GPI_WKUP_ASYNC_ENA_Pos				((uint32_t)0x00000001)
#define GPI_WKUP_ASYNC_ENA_Msk				((uint32_t)0x00000002)

#define GPI_POL_CFG_Pos				        ((uint32_t)0x00000002)
#define GPI_POL_CFG_Msk				        ((uint32_t)0x00000004)

#define GPI_FILTER_REMAP_Pos				((uint32_t)0x00000003)
#define GPI_FILTER_REMAP_Msk				((uint32_t)0x00000008)


//*****************************************************************************
//
// The following are defines for the bit fields in the GPIWKUP_CTRL register.
//
//*****************************************************************************
#define GPI0_WKUP_ENA_Msk				    ((uint32_t)0x00000001)
#define GPI0_WKUP_ENA_ResetMsk                  (~GPI0_WKUP_ENA_Msk)
#define GPI0_WKUP_ASYNC_ENA_Msk				((uint32_t)0x00000002)
#define GPI0_WKUP_ASYNC_ENA_ResetMsk            (~GPI0_WKUP_ASYNC_ENA_Msk)
#define GPI0_POL_CFG_ENA_Msk				((uint32_t)0x00000004)
#define GPI0_POL_CFG_ENA_ResetMsk               (~GPI0_POL_CFG_ENA_Msk)

#define GPI1_WKUP_ENA_Msk				    ((uint32_t)0x00000010)
#define GPI1_WKUP_ENA_ResetMsk                  (~GPI1_WKUP_ENA_Msk)
#define GPI1_WKUP_ASYNC_ENA_Msk				((uint32_t)0x00000020)
#define GPI1_WKUP_ASYNC_ENA_ResetMsk	        (~GPI1_WKUP_ASYNC_ENA_Msk)
#define GPI1_POL_CFG_ENA_Msk				((uint32_t)0x00000040)
#define GPI1_POL_CFG_ENA_ResetMsk               (~GPI1_POL_CFG_ENA_Msk)

#define GPI2_WKUP_ENA_Msk				    ((uint32_t)0x00000100)
#define GPI2_WKUP_ENA_ResetMsk                  (~GPI2_WKUP_ENA_Msk)
#define GPI2_WKUP_ASYNC_ENA_Msk				((uint32_t)0x00000200)
#define GPI2_WKUP_ASYNC_ENA_ResetMsk            (~GPI2_WKUP_ASYNC_ENA_Msk)
#define GPI2_POL_CFG_ENA_Msk				((uint32_t)0x00000400)
#define GPI2_POL_CFG_ENA_ResetMsk               (~GPI2_POL_CFG_ENA_Msk)

#define GPI3_WKUP_ENA_Msk				    ((uint32_t)0x00001000)
#define GPI3_WKUP_ENA_ResetMsk                  (~GPI3_WKUP_ENA_Msk)
#define GPI3_WKUP_ASYNC_ENA_Msk				((uint32_t)0x00002000)
#define GPI3_WKUP_ASYNC_ENA_ResetMsk	        (~GPI3_WKUP_ASYNC_ENA_Msk)
#define GPI3_POL_CFG_ENA_Msk				((uint32_t)0x00004000)
#define GPI3_POL_CFG_ENA_ResetMsk               (~GPI3_POL_CFG_ENA_Msk)

#define GPI4_WKUP_ENA_Msk				    ((uint32_t)0x00010000)
#define GPI4_WKUP_ENA_ResetMsk                  (~GPI4_WKUP_ENA_Msk)
#define GPI4_WKUP_ASYNC_ENA_Msk				((uint32_t)0x00020000)
#define GPI4_WKUP_ASYNC_ENA_ResetMsk            (~GPI4_WKUP_ASYNC_ENA_Msk)
#define GPI4_POL_CFG_ENA_Msk				((uint32_t)0x00040000)
#define GPI4_POL_CFG_ENA_ResetMsk               (~GPI4_POL_CFG_ENA_Msk)

#define GPI5_WKUP_ENA_Msk				    ((uint32_t)0x00100000)
#define GPI5_WKUP_ENA_ResetMsk                  (~GPI5_WKUP_ENA_Msk)
#define GPI5_WKUP_ASYNC_ENA_Msk				((uint32_t)0x00200000)
#define GPI5_WKUP_ASYNC_ENA_ResetMsk            (~GPI5_WKUP_ASYNC_ENA_Msk)
#define GPI5_POL_CFG_ENA_Msk				((uint32_t)0x00400000)
#define GPI5_POL_CFG_ENA_ResetMsk               (~GPI5_POL_CFG_ENA_Msk)
#define GPI5_FILTER_REMAP_Msk				((uint32_t)0x00800000)
#define GPI5_FILTER_REMAP_ResetMsk              (~GPI5_FILTER_REMAP_Msk)

#define GPI6_WKUP_ENA_Msk				    ((uint32_t)0x01000000)
#define GPI6_WKUP_ENA_ResetMsk                  (~GPI6_WKUP_ENA_Msk)
#define GPI6_WKUP_ASYNC_ENA_Msk				((uint32_t)0x02000000)
#define GPI6_WKUP_ASYNC_ENA_ResetMsk            (~GPI6_WKUP_ASYNC_ENA_Msk)
#define GPI6_POL_CFG_ENA_Msk				((uint32_t)0x04000000)
#define GPI6_POL_CFG_ENA_ResetMsk               (~GPI6_POL_CFG_ENA_Msk)
#define GPI6_FILTER_REMAP_Msk				((uint32_t)0x08000000)
#define GPI6_FILTER_REMAP_ResetMsk              (~GPI6_FILTER_REMAP_Msk)

#define GPI7_WKUP_ENA_Msk				    ((uint32_t)0x10000000)
#define GPI7_WKUP_ENA_ResetMsk                  (~GPI7_WKUP_ENA_Msk)
#define GPI7_WKUP_ASYNC_ENA_Msk				((uint32_t)0x20000000)
#define GPI7_WKUP_ASYNC_ENA_ResetMsk            (~GPI7_WKUP_ASYNC_ENA_Msk)
#define GPI7_POL_CFG_ENA_Msk				((uint32_t)0x40000000)
#define GPI7_POL_CFG_ENA_ResetMsk               (~GPI7_POL_CFG_ENA_Msk)
#define GPI7_FILTER_REMAP_Msk				((uint32_t)0x80000000)
#define GPI7_FILTER_REMAP_ResetMsk              (~GPI7_FILTER_REMAP_Msk)

//*****************************************************************************
//
// The following are defines for the bit fields in the RETWKP_CFG register.
//
//*****************************************************************************
#define EXPIN_WKUP_ENA_Pos                  ((uint8_t)0)
#define EXPIN_WKUP_ENA_Msk                  (1UL << EXPIN_WKUP_ENA_Pos)

#define EXPIN_POLCFG_Pos                    ((uint8_t)1)
#define EXPIN_POLCFG_Msk                    (1UL << EXPIN_POLCFG_Pos)

#define EXPIN_WKP_CFG_Pos                   ((uint8_t)2)    //wkup edge cfg
#define EXPIN_WKP_CFG_Msk                   (3UL << EXPIN_WKP_CFG_Pos)

#define EXPIN_WKP_PLS_CFG_Pos               ((uint8_t)4)   // wkup pluse cfg
#define EXPIN_WKP_PLS_CFG_Msk               (3UL << EXPIN_WKP_PLS_CFG_Pos)

#define EXPIN_RST_PLS_CFG_Pos               ((uint8_t)6)   // wkup pluse cfg
#define EXPIN_RST_PLS_CFG_Msk               (3UL << EXPIN_RST_PLS_CFG_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the RETWKP_CTRL register.
//
//*****************************************************************************

#define EXPIN_PULL_MSK                      3U


//*****************************************************************************
//
// The following are defines for the bit fields in the AON SYSCLK_CTRL register.
//
//*****************************************************************************
#define SYSCLK_SRC_SEL_Pos				    ((uint32_t)0x00000000)
#define SYSCLK_SRC_SEL_Msk				    ((uint32_t)0x00000003)
#define LS_IOCLK_SRC_SEL_Pos				((uint32_t)0x00000002)
#define LS_IOCLK_SRC_SEL_Msk				((uint32_t)0x0000000C)
#define FORCE_BYPASS_PLL_Msk				((uint32_t)0x00000040)
#define SLOWFREQ_DIV_ENA_Msk				((uint32_t)0x00000080)
#define FORCE_CLKSEL_ENA_Msk				((uint32_t)0x00000100)
#define CLKSRC_STP_Msk				        ((uint32_t)0x0000F000)
#define FORCE_XTALOFF_Msk				    ((uint32_t)0x00010000)
#define FORCE_PLLOFF_Msk				    ((uint32_t)0x00020000)
//*****************************************************************************
//
// The following are defines for the bit fields in the AON_AONGPREG0 register.
//
//*****************************************************************************
// bit 7,   Reserve
// bit 6,   AONGPREG0_CORE_LOAD_FLAG_SECONDARY_BOOT
// bit 5-3, External Flash Delay Config
// bit 2,   AONGPREG0_FORCE_XTAL_AFTER_RST
// bit 1,   AONGPREG0_CORE_LOAD_FLAG_ARM
// bit 0,   AONGPREG0_CORE_LOAD_FLAG_DSP
//#define AONGPREG0_BYPASS_XIP            0x80
#define AONGPREG0_CORE_LOAD_FLAG_SEC    0x40
#define AONGPREG0_FLASH_DELAY           0x38
#define AONGPREG0_FORCE_XTAL_AFTER_RST	0x04
#define AONGPREG0_CORE_LOAD_FLAG_AP     0x02
#define AONGPREG0_CORE_LOAD_FLAG_CP     0x01



//*****************************************************************************
//
// The following are defines for the bit fields in the AON_AONGPREG1 register.
//
//*****************************************************************************
// bit 3-0, 
#define AONGPREG1_FLASH_RDID            0x0F


//*****************************************************************************
//
// The following are defines for the bit fields in the CORE_CKG_CTRL register.
//
//*****************************************************************************
#define CORE_CKG_CTL_I2C1_EN			((uint64_t)0x00000001)
#define CORE_CKG_CTL_SPI_EN			    ((uint64_t)0x00000002)
#define CORE_CKG_CTL_UART2_EN			((uint64_t)0x00000004)
#define CORE_CKG_CTL_QSPI_EN			((uint64_t)0x00000008)
#define CORE_CKG_CTL_AP_WDT_EN			((uint64_t)0x00000010)
#define CORE_CKG_CTL_UTC_EN			    ((uint64_t)0x00000020)
#define CORE_CKG_CTL_MCNT_EN			((uint64_t)0x00000040)
#define CORE_CKG_CTL_CSP1_EN			((uint64_t)0x00000080)
#define CORE_CKG_CTL_CSP2_EN			((uint64_t)0x00000100)
#define CORE_CKG_CTL_CSP3_EN			((uint64_t)0x00000200)
#define CORE_CKG_CTL_CSP4_EN			((uint64_t)0x00000400)
#define CORE_CKG_CTL_AES_EN			    ((uint64_t)0x00000800)
#define CORE_CKG_CTL_SEMA_EN			((uint64_t)0x00001000)
#define CORE_CKG_CTL_DMAC_EN			((uint64_t)0x00002000)
#define CORE_CKG_CTL_AONPRCM_EN		    ((uint64_t)0x00004000)
#define CORE_CKG_CTL_LPUART_EN			((uint64_t)0x00008000)
#define CORE_CKG_CTL_LPTMR_EN			((uint64_t)0x00010000)
#define CORE_CKG_CTL_PHYTMR_EN			((uint64_t)0x00020000)
#define CORE_CKG_CTL_GPIO_EN			((uint64_t)0x00040000)
#define CORE_CKG_CTL_TMR1_EN			((uint64_t)0x00080000)
#define CORE_CKG_CTL_TMR2_EN			((uint64_t)0x00100000)
#define CORE_CKG_CTL_TMR3_EN			((uint64_t)0x00200000)
#define CORE_CKG_CTL_TMR4_EN			((uint64_t)0x00400000)
#define CORE_CKG_CTL_CP_WDT_EN			((uint64_t)0x00800000)
#define CORE_CKG_CTL_I2C2_EN			((uint64_t)0x01000000)
#define CORE_CKG_CTL_CRC_EN			    ((uint64_t)0x02000000)
#define CORE_CKG_CTL_ADC_EN			    ((uint64_t)0x04000000)
#define CORE_CKG_CTL_TRNG_EN			((uint64_t)0x08000000)
#define CORE_CKG_CTL_DFE_EN			    ((uint64_t)0x10000000)
#define CORE_CKG_CTL_BB_EN				((uint64_t)0x20000000)
#define CORE_CKG_CTL_SHA_EN			    ((uint64_t)0x40000000)
#define CORE_CKG_CTL_TICK_EN			((uint64_t)0x80000000)
#define CORE_CKG_CTL_ISO7816_EN		    ((uint64_t)0x0100000000)
#define CORE_CKG_CTL_RESERVE			((uint64_t)0x0200000000)
#define CORE_CKG_CTL_SSPI_MCLK_EN		((uint64_t)0x0400000000)
#define CORE_CKG_CTL_SIDO_EN			((uint64_t)0x0800000000)
#define CORE_CKG_CTL_KEYSCAN_EN		    ((uint64_t)0x1000000000)

#define CORE_CKG_CTL0_ALL_EN			((uint64_t)0xFFFFFFFFFFFFFFFF)

//*****************************************************************************
//
// The following are defines for the bit fields in the SYSCLK_FLAG register.
//
//*****************************************************************************
#define CORESYS_CLK_FLAG_Pos				    ((uint32_t)0)
#define CORESYS_CLK_FLAG_Msk				    ((uint32_t)0x0000000F)
#define LS_IOCLK_FLAG_Pos				        ((uint32_t)8)
#define LS_IOCLK_FLAG_Msk				        ((uint32_t)0x00000700)
#define AP_HCLK_DIV_FLAG_Pos				    ((uint32_t)16)
#define AP_HCLK_DIV_FLAG_Msk				    ((uint32_t)0x0F<<AP_HCLK_DIV_FLAG_Pos)
#define CP_HCLK_DIV_FLAG_Pos				    ((uint32_t)20)
#define CP_HCLK_DIV_FLAG_Msk				    ((uint32_t)0x0F<<CP_HCLK_DIV_FLAG_Pos)
//*****************************************************************************
//
// The following are defines for the bit fields in the SMEM_SLPCTRL register.
//
//*****************************************************************************
#define SH_SRAM8K_RET_ENA_Pos				            ((uint32_t)0)
#define SH_SRAM8K_RET_ENA_Msk				            ((uint32_t)1<<SH_SRAM8K_RET_ENA_Pos)
#define SH_SRAM8K_RET_ENA_ResetMsk				        ((uint32_t)~SH_SRAM8K_RET_ENA_Msk)
#define AP_SRAM0_RET_ENA_Pos				            ((uint32_t)1)
#define AP_SRAM0_RET_ENA_Msk				            ((uint32_t)1<<AP_SRAM0_RET_ENA_Pos)
#define AP_SRAM0_RET_ENA_ResetMsk				        ((uint32_t)~AP_SRAM0_RET_ENA_Msk)
#define AP_SRAM1_RET_ENA_Pos				            ((uint32_t)2)
#define AP_SRAM1_RET_ENA_Msk				            ((uint32_t)1<<AP_SRAM1_RET_ENA_Pos)
#define AP_SRAM1_RET_ENA_ResetMsk				        ((uint32_t)~AP_SRAM1_RET_ENA_Msk)

#define AP_SRAM0_SLPCTL_Pos				                ((uint32_t)8)
#define AP_SRAM0_SLPCTL_Msk				                ((uint32_t)3<<AP_SRAM0_SLPCTL_Pos)
#define AP_SRAM1_SLPCTL_Pos				                ((uint32_t)10)
#define AP_SRAM1_SLPCTL_Msk				                ((uint32_t)3<<AP_SRAM1_SLPCTL_Pos)
#define CP_SRAM0_SLPCTL_Pos				                ((uint32_t)12)
#define CP_SRAM0_SLPCTL_Msk				                ((uint32_t)3<<CP_SRAM0_SLPCTL_Pos)
#define CP_SRAM1_SLPCTL_Pos				                ((uint32_t)14)
#define CP_SRAM1_SLPCTL_Msk				                ((uint32_t)3<<CP_SRAM1_SLPCTL_Pos)
#define SH_SRAM64K_SLPCTL_Pos				            ((uint32_t)16)
#define SH_SRAM64K_SLPCTL_Msk				            ((uint32_t)3<<SH_SRAM64K_SLPCTL_Pos)
#define AP_SRAM0_BANK0_SLPCTL_Pos				        ((uint32_t)20)
#define AP_SRAM0_BANK0_SLPCTL_Msk				        ((uint32_t)3<<AP_SRAM0_BANK0_SLPCTL_Pos)
#define AP_SRAM0_BANK1_SLPCTL_Pos				        ((uint32_t)22)
#define AP_SRAM0_BANK1_SLPCTL_Msk				        ((uint32_t)3<<AP_SRAM0_BANK1_SLPCTL_Pos)
#define SH_SRAM8K_SLPCTL_Pos				            ((uint32_t)24)
#define SH_SRAM8K_SLPCTL_Msk				            ((uint32_t)3<<SH_SRAM8K_SLPCTL_Pos)
#define AP_SRAM1_BANK0_SLPCTL_Pos				        ((uint32_t)28)
#define AP_SRAM1_BANK0_SLPCTL_Msk				        ((uint32_t)3<<AP_SRAM1_BANK0_SLPCTL_Pos)
#define AP_SRAM1_BANK1_SLPCTL_Pos				        ((uint32_t)30)
#define AP_SRAM1_BANK1_SLPCTL_Msk				        ((uint32_t)3<<AP_SRAM1_BANK1_SLPCTL_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the coreprcm SYSCLK_CTRL register.
//
//*****************************************************************************
#define AP_HCLK_DIV_Pos				            ((uint32_t)0)
#define AP_HCLK_DIV_Msk				            ((uint32_t)0x0F<<AP_HCLK_DIV_Pos)
#define AP_HCLK_DIV_ResetMsk				    ((uint32_t)~AP_HCLK_DIV_Msk)

#define CP_HCLK_DIV_Pos				            ((uint32_t)4)
#define CP_HCLK_DIV_Msk				            ((uint32_t)0x0F<<CP_HCLK_DIV_Pos)
#define CP_HCLK_DIV_ResetMsk				    ((uint32_t)~CP_HCLK_DIV_Msk)

#define SYS_HCLK_DIV_Pos				        ((uint32_t)8)
#define SYS_HCLK_DIV_Msk				        ((uint32_t)0x0F<<SYS_HCLK_DIV_Pos)
#define SYS_HCLK_DIV_ResetMsk				    ((uint32_t)~SYS_HCLK_DIV_Msk)

#define PERI1_PCLK_DIV_Pos				        ((uint32_t)12)
#define PERI1_PCLK_DIV_Msk				        ((uint32_t)0x03<<PERI1_PCLK_DIV_Pos)
#define PERI1_PCLK_DIV_ResetMsk				    ((uint32_t)~PERI1_PCLK_DIV_Msk)

#define PERI2_PCLK_DIV_Pos				        ((uint32_t)14)
#define PERI2_PCLK_DIV_Msk				        ((uint32_t)0x03<<PERI2_PCLK_DIV_Pos)
#define PERI2_PCLK_DIV_ResetMsk				    ((uint32_t)~PERI2_PCLK_DIV_Msk)

#define SIDO_CLK_DIV_Pos				        ((uint32_t)16)
#define SIDO_CLK_DIV_Msk				        ((uint32_t)0x0F<<SIDO_CLK_DIV_Pos)
#define SIDO_CLK_DIV_ResetMsk				    ((uint32_t)~SIDO_CLK_DIV_Msk)

#define I2C1_REFCLK_SEL_Pos				        ((uint32_t)20)
#define I2C1_REFCLK_SEL_Msk				        ((uint32_t)0x01<<I2C1_REFCLK_SEL_Pos)
#define I2C1_REFCLK_SEL_ResetMsk				((uint32_t)~I2C1_REFCLK_SEL_Msk)

#define I2C2_REFCLK_SEL_Pos				        ((uint32_t)21)
#define I2C2_REFCLK_SEL_Msk				        ((uint32_t)0x01<<I2C2_REFCLK_SEL_Pos)
#define I2C2_REFCLK_SEL_ResetMsk				((uint32_t)~I2C2_REFCLK_SEL_Msk)

#define SSPI_CLK_DIV_Pos				        ((uint32_t)24)
#define SSPI_CLK_DIV_Msk				        ((uint32_t)0x0F<<SSPI_CLK_DIV_Pos)
#define SSPI_CLK_DIV_ResetMsk				    ((uint32_t)~SSPI_CLK_DIV_Msk)

#define QSPI_REFCLK_DIV_Pos				        ((uint32_t)28)
#define QSPI_REFCLK_DIV_Msk				        ((uint32_t)0x03<<QSPI_REFCLK_DIV_Pos)
#define QSPI_REFCLK_DIV_ResetMsk				((uint32_t)~QSPI_REFCLK_DIV_Msk)

#define TRNG_PCLK_DIV_Pos				        ((uint32_t)30)
#define TRNG_PCLK_DIV_Msk				        ((uint32_t)0x03<<TRNG_PCLK_DIV_Pos)
#define TRNG_PCLK_DIV_ResetMsk				    ((uint32_t)~TRNG_PCLK_DIV_Msk)

//*****************************************************************************
//
// The following are defines for the bit fields in the aonprcm SVD_CFG register.
//
//*****************************************************************************
#define SVD_FILT_CFG_Pos				            ((uint32_t)0)
#define SVD_FILT_CFG_Msk				            ((uint32_t)0x03<<SVD_FILT_CFG_Pos)
#define SVD_FILT_CFG_ResetMsk				    ((uint32_t)~SVD_FILT_CFG_Msk)

#define SVD_FILT_NUM_Pos				            ((uint32_t)4)
#define SVD_FILT_NUM_Msk				            ((uint32_t)0x07<<SVD_FILT_NUM_Pos)
#define SVD_FILT_NUM_ResetMsk				    ((uint32_t)~SVD_FILT_NUM_Msk)

#define SVD_PRD_CFG_Pos				            ((uint32_t)8)
#define SVD_PRD_CFG_Msk				            ((uint32_t)0xFF<<SVD_PRD_CFG_Pos)
#define SVD_PRD_CFG_ResetMsk				    ((uint32_t)~SVD_PRD_CFG_Msk)

#define SVD_PRDUNIT_SEL_Pos				            ((uint32_t)16)
#define SVD_PRDUNIT_SEL_Msk				            ((uint32_t)0x01<<SVD_PRDUNIT_SEL_Pos)
#define SVD_PRDUNIT_SEL_ResetMsk				    ((uint32_t)~SVD_PRDUNIT_SEL_Msk)

#define SVD_SAMPLE_NUM_Pos				            ((uint32_t)20)
#define SVD_SAMPLE_NUM_Msk				            ((uint32_t)0x07<<SVD_SAMPLE_NUM_Pos)
#define SVD_SAMPLE_NUM_ResetMsk				    ((uint32_t)~SVD_SAMPLE_NUM_Msk)

#define SVD_INVERT_POL_Pos				            ((uint32_t)24)
#define SVD_INVERT_POL_Msk				            ((uint32_t)0x01<<SVD_INVERT_POL_Pos)
#define SVD_INVERT_POL_ResetMsk				    ((uint32_t)~SVD_INVERT_POL_Msk)

#define SVD_THDMIN_CFG_Pos				            ((uint32_t)25)
#define SVD_THDMIN_CFG_Msk				            ((uint32_t)0x01<<SVD_THDMIN_CFG_Pos)
#define SVD_THDMIN_CFG_ResetMsk				    ((uint32_t)~SVD_THDMIN_CFG_Msk)

#define SVD_HCPD_CTL_Pos				            ((uint32_t)26)
#define SVD_HCPD_CTL_Msk				            ((uint32_t)0x01<<SVD_HCPD_CTL_Pos)
#define SVD_HCPD_CTL_ResetMsk				    ((uint32_t)~SVD_HCPD_CTL_Msk)

#define SVD_WARM_DLY_Pos				            ((uint32_t)28)
#define SVD_WARM_DLY_Msk				            ((uint32_t)0x01<<SVD_WARM_DLY_Pos)
#define SVD_WARM_DLY_ResetMsk				    ((uint32_t)~SVD_WARM_DLY_Msk)

//*****************************************************************************
//
// The following are defines for the bit fields in the aonprcm SVD_THRES_CTRL register.
//
//*****************************************************************************
#define SVD_THDMIN_VSEL_Pos				            ((uint32_t)0)
#define SVD_THDMIN_VSEL_Msk				            ((uint32_t)0x1F<<SVD_THDMIN_VSEL_Pos)
#define SVD_THDMIN_VSEL_ResetMsk				    ((uint32_t)~SVD_THDMIN_VSEL_Msk)

#define SVD_THDMIN_CAL_Pos				            ((uint32_t)5)
#define SVD_THDMIN_CAL_Msk				            ((uint32_t)0x07<<SVD_THDMIN_CAL_Pos)
#define SVD_THDMIN_CAL_ResetMsk				        ((uint32_t)~SVD_THDMIN_CAL_Msk)

#define SVD_THD1_VSEL_Pos				            ((uint32_t)8)
#define SVD_THD1_VSEL_Msk				            ((uint32_t)0x1F<<SVD_THD1_VSEL_Pos)
#define SVD_THD1_VSEL_ResetMsk				        ((uint32_t)~SVD_THD1_VSEL_Msk)

#define SVD_THD1_CAL_Pos				            ((uint32_t)13)
#define SVD_THD1_CAL_Msk				            ((uint32_t)0x07<<SVD_THD1_CAL_Pos)
#define SVD_THD1_CAL_ResetMsk				        ((uint32_t)~SVD_THD1_CAL_Msk)

#define SVD_THD2_VSEL_Pos				            ((uint32_t)16)
#define SVD_THD2_VSEL_Msk				            ((uint32_t)0x1F<<SVD_THD2_VSEL_Pos)
#define SVD_THD2_VSEL_ResetMsk				        ((uint32_t)~SVD_THD2_VSEL_Msk)

#define SVD_THD2_CAL_Pos				            ((uint32_t)21)
#define SVD_THD2_CAL_Msk				            ((uint32_t)0x07<<SVD_THD2_CAL_Pos)
#define SVD_THD2_CAL_ResetMsk				        ((uint32_t)~SVD_THD2_CAL_Msk)

#define SVD_THD3_VSEL_Pos				            ((uint32_t)24)
#define SVD_THD3_VSEL_Msk				            ((uint32_t)0x1F<<SVD_THD3_VSEL_Pos)
#define SVD_THD3_VSEL_ResetMsk				        ((uint32_t)~SVD_THD3_VSEL_Msk)

#define SVD_THD3_CAL_Pos				            ((uint32_t)29)
#define SVD_THD3_CAL_Msk				            ((uint32_t)0x07<<SVD_THD3_CAL_Pos)
#define SVD_THD3_CAL_ResetMsk				        ((uint32_t)~SVD_THD3_CAL_Msk)

//*****************************************************************************
//
// The following are defines for the bit fields in the aonprcm SVD_CTRL register.
//
//*****************************************************************************
#define SVD_CH1_SEL_Pos				            ((uint16_t)1)
#define SVD_CH1_SEL_Msk				            ((uint16_t)0x01<<SVD_CH1_SEL_Pos)
#define SVD_CH1_SEL_ResetMsk				    ((uint16_t)~SVD_CH1_SEL_Msk)

#define SVD_CH2_SEL_Pos				            ((uint16_t)2)
#define SVD_CH2_SEL_Msk				            ((uint16_t)0x01<<SVD_CH2_SEL_Pos)
#define SVD_CH2_SEL_ResetMsk				    ((uint16_t)~SVD_CH2_SEL_Msk)

#define SVD_CH3_SEL_Pos				            ((uint16_t)3)
#define SVD_CH3_SEL_Msk				            ((uint16_t)0x01<<SVD_CH3_SEL_Pos)
#define SVD_CH3_SEL_ResetMsk				    ((uint16_t)~SVD_CH3_SEL_Msk)

#define SVD_THD1_DPSLP_CTL_Pos				    ((uint16_t)5)
#define SVD_THD1_DPSLP_CTL_Msk				    ((uint16_t)0x01<<SVD_THD1_DPSLP_CTL_Pos)
#define SVD_THD1_DPSLP_CTL_ResetMsk				((uint16_t)~SVD_THD1_DPSLP_CTL_Msk)


//*****************************************************************************
//
//The following are defines for the bit fields in the LPUA_SLEEP_CTL register.
//
//*****************************************************************************
#define LPUA_PWR_MODE_CTL_Pos		     		((uint32_t)0)
#define LPUA_PWR_MODE_CTL_Msk			   		((uint32_t)0x03<<LPUA_PWR_MODE_CTL_Pos)
#define LPUA_PWR_MODE_CTL_ResetMsk			    ((uint32_t)~LPUA_PWR_MODE_CTL_Msk)

#define LCDC_PWR_MODE_CTL_Pos		     		((uint32_t)4)
#define LCDC_PWR_MODE_CTL_Msk			   		((uint32_t)0x03<<LCDC_PWR_MODE_CTL_Pos)
#define LCDC_PWR_MODE_CTL_ResetMsk			    ((uint32_t)~LCDC_PWR_MODE_CTL_Msk)

#define RC32K_CTRL_PWR_MODE_CTL_Pos		     	((uint32_t)6)
#define RC32K_CTRL_PWR_MODE_CTL_Msk			   	((uint32_t)0x03<<RC32K_CTRL_PWR_MODE_CTL_Pos)
#define RC32K_CTRL_PWR_MODE_CTL_ResetMsk		((uint32_t)~RC32K_CTRL_PWR_MODE_CTL_Msk)

//*****************************************************************************
//
//The following are defines for the bit fields in the CORE_PWR_CTL register.
//
//*****************************************************************************
#define IOLDO1_LPMODE_LP_CTL_Pos		        ((uint8_t)0)
#define IOLDO1_LPMODE_LP_CTL_Msk			   	((uint8_t)0x01<<IOLDO1_LPMODE_LP_CTL_Pos)
#define IOLDO1_LPMODE_LP_CTL_ResetMsk			((uint8_t)~IOLDO1_LPMODE_LP_CTL_Msk)

#define IOLDO1_LPMODE_PSM_CTL_Pos		        ((uint8_t)1)
#define IOLDO1_LPMODE_PSM_CTL_Msk			   	((uint8_t)0x01<<IOLDO1_LPMODE_PSM_CTL_Pos)
#define IOLDO1_LPMODE_PSM_CTL_ResetMsk			((uint8_t)~IOLDO1_LPMODE_PSM_CTL_Msk)

#define IOLDO1_NORMAL_CTL_Pos		            ((uint8_t)2)
#define IOLDO1_NORMAL_CTL_Msk			   	    ((uint8_t)0x01<<IOLDO1_NORMAL_CTL_Pos)
#define IOLDO1_NORMAL_CTL_ResetMsk			    ((uint8_t)~IOLDO1_NORMAL_CTL_Msk)

#define IOLDO1_PD_ANY_CTL_Pos		            ((uint8_t)3)
#define IOLDO1_PD_ANY_CTL_Msk			   	    ((uint8_t)0x01<<IOLDO1_PD_ANY_CTL_Pos)
#define IOLDO1_PD_ANY_CTL_ResetMsk			    ((uint8_t)~IOLDO1_PD_ANY_CTL_Msk)

#define IOLDO2_LPMODE_LP_CTL_Pos		        ((uint8_t)4)
#define IOLDO2_LPMODE_LP_CTL_Msk			   	((uint8_t)0x01<<IOLDO2_LPMODE_LP_CTL_Pos)
#define IOLDO2_LPMODE_LP_CTL_ResetMsk			((uint8_t)~IOLDO2_LPMODE_LP_CTL_Msk)

#define IOLDO2_LPMODE_PSM_CTL_Pos		        ((uint8_t)5)
#define IOLDO2_LPMODE_PSM_CTL_Msk			   	((uint8_t)0x01<<IOLDO2_LPMODE_PSM_CTL_Pos)
#define IOLDO2_LPMODE_PSM_CTL_ResetMsk			((uint8_t)~IOLDO2_LPMODE_PSM_CTL_Msk)

#define IOLDO2_NORMAL_CTL_Pos		            ((uint8_t)6)
#define IOLDO2_NORMAL_CTL_Msk			   	    ((uint8_t)0x01<<IOLDO2_NORMAL_CTL_Pos)
#define IOLDO2_NORMAL_CTL_ResetMsk			    ((uint8_t)~IOLDO2_NORMAL_CTL_Msk)

#define IOLDO2_PD_ANY_CTL_Pos		            ((uint8_t)7)
#define IOLDO2_PD_ANY_CTL_Msk			   	    ((uint8_t)0x01<<IOLDO2_PD_ANY_CTL_Pos)
#define IOLDO2_PD_ANY_CTL_ResetMsk			    ((uint8_t)~IOLDO2_PD_ANY_CTL_Msk)


//*****************************************************************************
//
//The following are defines for the bit fields in the PWRCTL_CFG register.
//
//*****************************************************************************
#define PERI1_GATE_CTL_Pos		                ((uint32_t)0)
#define PERI1_GATE_CTL_Msk			   	        ((uint32_t)0x01<<PERI1_GATE_CTL_Pos)
#define PERI1_GATE_CTL_ResetMsk			        ((uint32_t)~PERI1_GATE_CTL_Msk)

#define PERI2_GATE_CTL_Pos		                ((uint32_t)1)
#define PERI2_GATE_CTL_Msk			   	        ((uint32_t)0x01<<PERI2_GATE_CTL_Pos)
#define PERI2_GATE_CTL_ResetMsk			        ((uint32_t)~PERI2_GATE_CTL_Msk)

#define APCORE_GATE_CTL_Pos		                ((uint32_t)2)
#define APCORE_GATE_CTL_Msk			   	        ((uint32_t)0x01<<APCORE_GATE_CTL_Pos)
#define APCORE_GATE_CTL_ResetMsk	            ((uint32_t)~APCORE_GATE_CTL_Msk)

#define CPCORE_GATE_CTL_Pos		                ((uint32_t)3)
#define CPCORE_GATE_CTL_Msk			   	        ((uint32_t)0x01<<CPCORE_GATE_CTL_Pos)
#define CPCORE_GATE_CTL_ResetMsk			    ((uint32_t)~CPCORE_GATE_CTL_Msk)

#define MSYS_GATE_CTL_Pos		                ((uint32_t)4)
#define MSYS_GATE_CTL_Msk			   	        ((uint32_t)0x01<<MSYS_GATE_CTL_Pos)
#define MSYS_GATE_CTL_ResetMsk			        ((uint32_t)~MSYS_GATE_CTL_Msk)

#define BBSYS_GATE_CTL_Pos		                ((uint32_t)5)
#define BBSYS_GATE_CTL_Msk			   	        ((uint32_t)0x01<<BBSYS_GATE_CTL_Pos)
#define BBSYS_GATE_CTL_ResetMsk			        ((uint32_t)~BBSYS_GATE_CTL_Msk)

#endif // __HW_PRCM_H__
